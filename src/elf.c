#include <elf.h>
#include <kheap.h>
#include <paging.h>
#include <tasking.h>

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

	// Now we need to navigate the sections to find .text and .data
	#ifdef ELF_DEBUG
	kprintf("e_shoff=%x, e_shentsize=%d, e_shnum=%d\n", hdr->e_shoff, hdr->e_shentsize, hdr->e_shnum);
	#endif

	// Get our section header table.
	Elf32_Shdr* sections = (Elf32_Shdr*) (file + hdr->e_shoff);
	// We should probably check e_shentsize

	// What this loop does is loads the relocation tables into their respective ArrayList
	ArrayList* TYPE(Elf32_Shdr*) rela = ALCreate();
	ArrayList* TYPE(Elf32_Shdr*) rel = ALCreate();
	ArrayList* TYPE(ProgBitsInfo*) progBits = ALCreate();

	Elf32_Shdr* symtab = NULL;

	int i;
	for(i=0; i<hdr->e_shnum; i++) {
		switch(sections[i].sh_type) {
			default: {

			} break;

			case SHT_RELA: {
				// Good old relocations.
				ALAdd(rela, &sections[i]);
			} break;

			case SHT_REL: {
				// Good old relocations.
				ALAdd(rel, &sections[i]);
			} break;

			case SHT_SYMTAB: {
				symtab = &sections[i];
			} break;

			case SHT_PROGBITS: {
				// We need to put this into an allocated space
				void* addr = GetDriverAllocatedSpace();
				// Copy the "progbits" over to the new addr.
				void* fileAddr = (file + sections[i].sh_offset);
				memcpy(addr, fileAddr, sections[i].sh_size);
				ProgBitsInfo* info = kalloc(sizeof(ProgBitsInfo));
				info->addr = addr;
				info->requestedAddr = sections[i].sh_addr;
				info->length = sections[i].sh_size;
				info->sectionHeader = &sections[i];
				info->sh_idx = i;
				ALAdd(progBits, info);
			} break;
		}
	}

	ALIterator* itr = ALGetItr(rel);
	while(ALItrHasNext(itr)) {
		Elf32_Shdr* rel_hdr = (Elf32_Shdr*) ALItrNext(itr);
		int rel_length = rel_hdr->sh_size / rel_hdr->sh_entsize;
		Elf32_Rel* rel = (Elf32_Rel*) (file + rel_hdr->addr);
		// Process the relocations
		int i;
		for(i=0; i<rel_length; i++) {
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
}

char* elfErrors[] = {"NO_ERROR", "UNSUPPORTED_FEATURE", "UNSUPPORTED_CPU_ARCH", "WRONG_VERSION", "INVALID_MAGIC_BYTES", "INVALID_ELF_CLASS", "INVALID_ENDIAN"};

char** GetElfErrors() {
	return elfErrors;
}

int CreateTaskFromELF(ELF* elf) {
	return CreateTask((UInt32) elf->start, elf->dir);
}