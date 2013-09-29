#include <elf.h>
#include <kheap.h>
#include <paging.h>
#include <tasking.h>
#include <drivers.h>

//#define ELF_DEBUG

extern PageDirectory* currentPageDir;

Bool CheckMagic(unsigned char* ident) {
	// Filter out all the unsupported and invalid ELFs.
	if(ident[EI_MAG0]==ELFMAG0 && ident[EI_MAG1]==ELFMAG1 && ident[EI_MAG2]==ELFMAG2 && ident[EI_MAG3]==ELFMAG3) {
		return TRUE;
	} else {
		#ifdef ELF_DEBUG
		int i;
		
		for(i=0; i<4; i++) {
			kprintf("hdr->e_ident[%d] = %x\n", i, hdr->e_ident[i]);
		}
		#endif
		
		return FALSE;
	}
}

ELF* Parse_ELF(Pointer executable) {
	Elf32_Ehdr* hdr = (Elf32_Ehdr*) executable;
	ELF* elf = (ELF*) kalloc(sizeof(ELF));
	
	elf->error = NO_ERROR;
	
	if(CheckMagic(hdr->e_ident)) {

	} else {
		#ifdef ELF_DEBUG
		int i;
		
		for(i=0; i<4; i++) {
			kprintf("hdr->e_ident[%d] = %x\n", i, hdr->e_ident[i]);
		}
		#endif
		
		elf->error = INVALID_MAGIC_BYTES;
		return elf;
	}
	
	if(hdr->e_ident[EI_CLASS] != ELFCLASS32) {
		elf->error = INVALID_ELF_CLASS;
		return elf;
	}
	
	if(hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		elf->error = INVALID_ENDIAN;
		return elf;
	}
	
	if(hdr->e_ident[EI_VERSION] != EV_CURRENT) {
		elf->error = WRONG_VERSION;
		return elf;
	}
	
	if(hdr->e_machine != EM_386) {
		elf->error = UNSUPPORTED_CPU_ARCH;
		return elf;
	}
	
	Elf32_Phdr* p_headers = (Elf32_Phdr*) ((UInt32)executable+hdr->e_phoff);
	
	PageDirectory* dir = CloneDirectory(NULL, DIR_OTHER_TASK);
	PageDirectory* saved = currentPageDir;
	
	#ifdef ELF_DEBUG
	kprintf("e_phnum = %x\n", hdr->e_phnum);
	#endif
	
	if(hdr->e_phentsize != sizeof(Elf32_Phdr)) {
		kprintf("This loader does not support program headers whose sizes deviate from %d bytes.\n", sizeof(Elf32_Phdr));
		return NULL;
	}
	
	SwitchPageDirectory(dir);
	int i;
	for(i=0; i<hdr->e_phnum; i++) {
		// Find .text, load it, and load all others appropriately
		#ifdef ELF_DEBUG
		kprintf("p_headers[%d].p_flags = %x.  hdr->e_phnum=%d\n", i, p_headers[i].p_flags, hdr->e_phnum);
		#endif
		
		if(p_headers[i].p_vaddr == NULL) {
			// TODO:  Figure out what to do with this.  The one that is null (at least in my helloworld) is GNU_STACK
			continue;
		}

		int pagesNum;
		void* ptr = (void*) ((UInt32)p_headers[i].p_vaddr&~0xFFF);
		int memsz = p_headers[i].p_memsz;
	
		if(memsz&0xFFF) {
			memsz += 0x1000;
		}
		pagesNum = memsz>>12;
	
		int j;
		for(j=0; j<=pagesNum; j++) {
			if(!IsMapped(dir, ptr)) {
				MapAllocatedPageTo(dir, ptr, USER_TEXT_FLAGS);
			}

			ptr+=0x1000;
		}
		
		memcpy(p_headers[i].p_vaddr, executable+p_headers[i].p_offset, p_headers[i].p_filesz);
		/*
		if(p_headers[i].p_flags&PF_W) {
			data = &p_headers[i];
		} else {
			text = &p_headers[i];
		}
		
		if(data!=NULL && text!=NULL) {
			break;
		}*/
	}
	
	SwitchPageDirectory(saved);
		
	elf->dir = dir;
	elf->start = hdr->e_entry;
	
	#ifdef ELF_DEBUG
	kprintf("ELF file parsing complete. ret %x\n", elf);
	#endif
	
	return elf;
}

typedef struct {
	void* addr; // The address at which the progbits are loaded in memory.
	void* requestedAddr;
	int length; // Length of the progbits section in bytes.
	Elf32_Shdr* sectionHeader;
	int sh_idx;
	// Half of this stuff is to make our life easier so we don't have to recalculate stuff every single time
	// we want to use it. sectionHeader isn't necessary, and neither is length, as sectionHeader can be derived
	// from sh_idx, and length can derived from sectionHeader.
} ProgBitsInfo;

typedef struct {
	Elf32_Shdr* header;
	void* addr; // The address that the header needs to be relocated to.
} SectionInfo;

ELF* LoadKernelDriver(Pointer file) {
	Elf32_Ehdr* hdr = (Elf32_Ehdr*) file;
	ELF* elf = (ELF*) kalloc(sizeof(ELF));
	
	elf->error = NO_ERROR;
	
	if(CheckMagic(hdr->e_ident)) {

	} else {
		#ifdef ELF_DEBUG
		int i;
		
		for(i=0; i<4; i++) {
			kprintf("hdr->e_ident[%d] = %x\n", i, hdr->e_ident[i]);
		}
		#endif
		
		elf->error = INVALID_MAGIC_BYTES;
		return elf;
	}
	
	if(hdr->e_ident[EI_CLASS] != ELFCLASS32) {
		elf->error = INVALID_ELF_CLASS;
		return elf;
	}
	
	if(hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		elf->error = INVALID_ENDIAN;
		return elf;
	}
	
	if(hdr->e_ident[EI_VERSION] != EV_CURRENT) {
		elf->error = WRONG_VERSION;
		return elf;
	}
	
	if(hdr->e_machine != EM_386) {
		elf->error = UNSUPPORTED_CPU_ARCH;
		return elf;
	}

	elf->error = -1;

	// Now we need to navigate the sections to find .text and .data
	#ifdef ELF_DEBUG
	kprintf("e_shoff=%x, e_shentsize=%d, e_shnum=%d\n", hdr->e_shoff, hdr->e_shentsize, hdr->e_shnum);
	#endif

	// Get our section header table.
	Elf32_Shdr* sections = (Elf32_Shdr*) (file + hdr->e_shoff);
	// We should probably check e_shentsize

	// What this loop does is loads the relocation tables into their respective ArrayList
	ArrayList* TYPE(Elf32_Shdr*) relaSections = ALCreate();
	ArrayList* TYPE(Elf32_Shdr*) relSections = ALCreate();
	ArrayList* TYPE(ProgBitsInfo*) progBits = ALCreate();

	Elf32_Shdr* symtabSection = NULL;
	Elf32_Shdr* strtabSection = NULL;

	Elf32_Sym* symtab = NULL;
	char* strtab = NULL;
	char* shstrtab = NULL;

	if(hdr->e_shstrndx != SHN_UNDEF) {
		shstrtab = (file + sections[hdr->e_shstrndx].sh_offset);
	}

	SectionInfo* sectionInfo = kalloc(sizeof(SectionInfo)*hdr->e_shnum);
	memset(sectionInfo, 0, sizeof(SectionInfo)*hdr->e_shnum);

	int i;
	for(i=0; i<hdr->e_shnum; i++) {
		sectionInfo[i].header = &sections[i];
		// If the section needs to be loaded into memory, allocate ourselves driver space for it.
		if(sections[i].sh_flags & SHF_ALLOC) {
			sectionInfo[i].addr = AllocateDriverSpace(sections[i].sh_size / PAGE_SIZE + 1);
			if(sectionInfo[i].addr == NULL) {
				kprintf("AllocateDriverSpace error %d.\n", GetErr());
			}

			memcpy(sectionInfo[i].addr, (file+sections[i].sh_offset), sections[i].sh_size);
		}

		switch(sections[i].sh_type) {
			default: {

			} break;

			case SHT_RELA: {
				// Good old relocations.
				ALAdd(relaSections, &sections[i]);
			} break;

			case SHT_REL: {
				// Good old relocations.
				ALAdd(relSections, &sections[i]);
			} break;

			case SHT_SYMTAB: {
				symtabSection = &sections[i];
				symtab = (Elf32_Sym*) (symtabSection->sh_offset + file);
			} break;

			case SHT_STRTAB: {
				if(i == hdr->e_shstrndx) {
					break; // We are breaking from the switch, not the "for" loop.
				}
				strtabSection = &sections[i];
				strtab = (sections[i].sh_offset + file);
			} break;
		}
	}

	ALIterator* itr = ALGetItr(relSections);
	while(ALItrHasNext(itr)) {
		Elf32_Shdr* rel_hdr = (Elf32_Shdr*) ALItrNext(itr);
		int rel_length = rel_hdr->sh_size / rel_hdr->sh_entsize;
		Elf32_Rel* rel = (Elf32_Rel*) ((UInt32)file + rel_hdr->sh_offset);
		Elf32_Shdr* sectionToRelocate = &sections[rel_hdr->sh_info];

		// Process the relocations
		kprintf("newRelocationTable\n");

		int i;
		for(i=0; i<rel_length; i++) {
			int relType = ELF32_R_TYPE(rel[i].r_info);
			int relOffset = (int) rel[i].r_offset;

			switch(relType) {
				case R_386_32: {
					// Ok so we find the symbol.
					int relSym = ELF32_R_SYM(rel[i].r_info);
					Elf32_Sym* sym = &symtab[relSym];
					//Elf32_Shdr* shdr = &sections[sym->st_shndx];
					// Find the address that the relocation should be applied to
					UInt32 relocationAddrDiff = (sectionInfo[rel_hdr->sh_info].addr - sectionInfo[rel_hdr->sh_info].header->sh_addr);

					Elf32_Word* relApplicationAddr = (Elf32_Word*) (sectionInfo[rel_hdr->sh_info].addr + relOffset);

					// We want to add sym->st_value to the value at relOffset, also relocate it to the address specified.
					*relApplicationAddr = (Elf32_Word) (sym->st_value + *relApplicationAddr + relocationAddrDiff); // *relApplicationAddr is the implicit addend.

					kprintf("Here we have _ %s, %x\n", &shstrtab[sectionToRelocate->sh_name], *relApplicationAddr);
				} break;

				case R_386_PC32: {
					int relSym = ELF32_R_SYM(rel[i].r_info);
					Elf32_Sym* sym = &symtab[relSym];

					kprintf("Here we have %s,%x\n", &shstrtab[sectionToRelocate->sh_name], &shstrtab[sectionToRelocate->sh_name]);

					Elf32_Word* relApplicationAddr = (Elf32_Word*) (sectionInfo[rel_hdr->sh_info].addr + relOffset);

					// S + A - P
					*relApplicationAddr = (Elf32_Word) (sym->st_value + *relApplicationAddr - relOffset);

					kprintf("rel2 = %x\n", *relApplicationAddr);
					
				} break;

				default: {
					kprintf("Unsupported relocation type.  %d\n", relType);
				} break;
			}
			/*
			r_offset This member gives the location at which to apply the relocation action. For a relocatable
				file, the value is the byte offset from the beginning of the section to the storage unit affected
				by the relocation. For an executable file or a shared object, the value is the virtual address of
				the storage unit affected by the relocation.
			r_info This member gives both the symbol table index with respect to which the relocation must be
				made, and the type of relocation to apply. For example, a call instruction’s relocation entry
				would hold the symbol table index of the function being called. If the index is STN_UNDEF,
				the undefined symbol index, the relocation uses 0 as the ‘‘symbol value.’’ Relocation types
				are processor-specific. When the text refers to a relocation entry’s relocation type or symbol
				table index, it means the result of applying ELF32_R_TYPE or ELF32_R_SYM, respectively,
				to the entry’s r_info member.
			*/
			
		}

		// TODO: FINISH
	}


	elf->start = hdr->e_entry;
	elf->dir = NULL;
	elf->error = 0;

	// TODO:  Search for an entry ponit.
	// Loop through our symbol table looking for a symbol which is linked to "_start"

	if(strtab != NULL) {
		kprintf("strtab=%x, symtab=%x\n", strtab, symtab);
		for(i=0; i<symtabSection->sh_size/symtabSection->sh_entsize; i++) {
			kprintf("sym_name=%s, %x\n", &strtab[symtab[i].st_name], &strtab[symtab[i].st_name]);
		}
	} else {
		kprintf("strtab==NULL\n");
	}

	kprintf("%x, %x, %x\n", hdr->e_entry, elf->dir, elf->error);
	return elf;
}

char* elfErrors[] = {"NO_ERROR", "UNSUPPORTED_FEATURE", "UNSUPPORTED_CPU_ARCH", "WRONG_VERSION", "INVALID_MAGIC_BYTES", "INVALID_ELF_CLASS", "INVALID_ENDIAN"};

char** GetElfErrors() {
	return elfErrors;
}

int CreateTaskFromELF(ELF* elf) {
	return CreateTask((UInt32) elf->start, elf->dir);
}