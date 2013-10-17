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
			kprintf("hdr->e_ident[%d] = %x\n", i, ident[i]);
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

// WARNING:  THIS FUNCTION HAS UGLY CODE!!! DO NOT ENTER IF YOU VALUE YOUR SANITY!
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
	//ArrayList* TYPE(ProgBitsInfo*) progBits = ALCreate();

	Elf32_Shdr* symtabSection = NULL;
	//Elf32_Shdr* strtabSection;

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

			#ifdef ELF_DEBUG
			kprintf("sectionInfo[%d].addr=%x, str=%s\n", i, sectionInfo[i].addr, shstrtab[sectionInfo[i].header->sh_name]);
			#endif

			// Probably not completely necessary, but it is for BSS, and I don't feel like doing anything different
			// right now.
			memset(sectionInfo[i].addr, 0, sections[i].sh_size);
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
				//strtabSection = &sections[i];
				strtab = (sections[i].sh_offset + file);
			} break;

		}
	}

	// Go through the symtab and tally up everything that belongs to the common section.
	UInt32 commonSectionSize = 0;
	int iSymtab = 0;
	for(iSymtab = 0; iSymtab < symtabSection->sh_size / symtabSection->sh_entsize; iSymtab++) {
		if(symtab[iSymtab].st_shndx == SHN_COMMON) {
			if(commonSectionSize % (UInt32) symtab[iSymtab].st_value) {
				if(commonSectionSize > 0) {
					commonSectionSize = commonSectionSize / (UInt32) symtab[iSymtab].st_value;
					commonSectionSize = commonSectionSize * (UInt32) symtab[iSymtab].st_value + (UInt32) symtab[iSymtab].st_value;
				}

			}

			Elf32_Addr value = (Elf32_Addr) commonSectionSize;

			commonSectionSize += symtab[iSymtab].st_size;
			symtab[iSymtab].st_value = value;

		}
	}

	void* commonSectionAddr = AllocateDriverSpace(commonSectionSize / PAGE_SIZE + 1);
	memset(commonSectionAddr, 0, commonSectionSize);

	ALIterator* itr = ALGetItr(relSections);
	while(ALItrHasNext(itr)) {
		Elf32_Shdr* rel_hdr = (Elf32_Shdr*) ALItrNext(itr);
		int rel_length = rel_hdr->sh_size / rel_hdr->sh_entsize; // Number of relocation entries.
		Elf32_Rel* rel = (Elf32_Rel*) ((UInt32)file + rel_hdr->sh_offset);

		// This is here because the compiler throws a warning (which is an error in our configuration).
		#ifdef ELF_DEBUG
		Elf32_Shdr* sectionToRelocate = &sections[rel_hdr->sh_info];
		#endif

		int i;
		for(i=0; i<rel_length; i++) {
			int relType = ELF32_R_TYPE(rel[i].r_info);
			int relOffset = (int) rel[i].r_offset;

			// I WARNED YOU!!! I'M SO, SO SORRY.
			switch(relType) {
				case R_386_32: {
					// Ok so we find the symbol.
					int relSym = ELF32_R_SYM(rel[i].r_info);
					Elf32_Sym* sym = &symtab[relSym];
					//Elf32_Shdr* shdr = &sections[sym->st_shndx];

					UInt32 relocationAddrDiff;

					if(sym->st_shndx == SHN_COMMON) {
						relocationAddrDiff = ((UInt32) commonSectionAddr); // We changed st_value to the new 
						// value calculated in the iSymtab for loop above.
					} else {
						// Find the address that the relocation should be applied to
						relocationAddrDiff = (sectionInfo[sym->st_shndx].addr - sectionInfo[rel_hdr->sh_info].header->sh_addr);
					}

					Elf32_Word* relApplicationAddr = (Elf32_Word*) (sectionInfo[rel_hdr->sh_info].addr + relOffset);

					#ifdef ELF_DEBUG
					kprintf("relAddr = %x\n", relApplicationAddr);
					#endif

					// We want to add sym->st_value to the value at relOffset, also relocate it to the address specified.
					*relApplicationAddr = (Elf32_Word) (sym->st_value + *relApplicationAddr + relocationAddrDiff); // *relApplicationAddr is the implicit addend.

					#ifdef ELF_DEBUG
					if(sym->st_shndx != SHN_COMMON) {
						kprintf("R_386_32 %s, %x\n", &shstrtab[sectionInfo[sym->st_shndx].header->sh_name], *relApplicationAddr);
					} else {
						kprintf("R_386_32 %s, %x\n", ".common", *relApplicationAddr);
					}
					#endif

				} break;

				case R_386_PC32: {
					int relSym = ELF32_R_SYM(rel[i].r_info);
					int externalSymbol = 0;
					void* externalSymbolValue;

					if(symtab[relSym].st_shndx == SHN_UNDEF) {
						// We want to look through the kernel 
						externalSymbolValue = getKernelSymbol(&strtab[symtab[relSym].st_name]);
						symtab[relSym].st_value = externalSymbolValue;
						if(externalSymbolValue == NULL) {
							elf->start = 0;
							elf->dir = NULL;
							elf->error = UNDEFINED_SYMBOL;
							kprintf("UNDEFINED_SYMBOL_ERROR %s\n", &strtab[symtab[relSym].st_name]);
							return elf;
						} else {
							externalSymbol = 1;
						}
					}

					Elf32_Sym* sym = &symtab[relSym];

					#ifdef ELF_DEBUG
					kprintf("Here we have %s\n", &shstrtab[sectionToRelocate->sh_name]);
					#endif

					Elf32_Word* relApplicationAddr = (Elf32_Word*) (sectionInfo[rel_hdr->sh_info].addr + relOffset);

					// S + A - P
					if(!externalSymbol) {
						*relApplicationAddr = (Elf32_Word) (sym->st_value + *relApplicationAddr - relOffset);
					} else {
						*relApplicationAddr = (Elf32_Word) (externalSymbolValue + *relApplicationAddr - (sectionInfo[rel_hdr->sh_info].addr + relOffset));
					}
					#ifdef ELF_DEBUG
					kprintf("rel2 = %x\n", *relApplicationAddr);
					#endif
				} break;

				default: {
					kprintf("Unsupported relocation type.  %d\n", relType);
				} break;
			}
			
		}

		// TODO: FINISH
	}

	// Clean up the stuff related to the relocation.
	ALFreeItr(itr);
	//ALFreeList(relSections);
	//ALFreeList(relaSections);

	relSections = NULL;
	relaSections = NULL;

	elf->start = hdr->e_entry;
	elf->dir = NULL;
	elf->error = 0;

	// Loop through our symbol table looking for a symbol which is linked to "_start"

	if(strtab != NULL) {
		for(i=0; i<symtabSection->sh_size/symtabSection->sh_entsize; i++) {
			#ifdef ELF_DEBUG
			kprintf("sym_name=%s, %x\n", &strtab[symtab[i].st_name], &strtab[symtab[i].st_name]);
			#endif

			if(!strcmp("_start", &strtab[symtab[i].st_name])) {
				// Section's virtual address + the symbol address.
				elf->start = (void*) ((void*)sectionInfo[symtab[i].st_shndx].addr + (unsigned)symtab[i].st_value);
			}
		}
	} else {
		kprintf("strtab==NULL\n");
	}

	// XXX:  I'm sure there are some other kallocs that don't have matching kfrees
	// CLEANUP
	kfree(sectionInfo);
	// END CLEANUP

	kprintf("%x, %x, %x\n", elf->start, elf->dir, elf->error);
	return elf;
}

char* elfErrors[] = {
	"NO_ERROR", "UNSUPPORTED_FEATURE", "UNSUPPORTED_CPU_ARCH", "WRONG_VERSION",
	"INVALID_MAGIC_BYTES", "INVALID_ELF_CLASS", "INVALID_ENDIAN",
	"UNDEFINED_SYMBOL"
};

char** GetElfErrors() {
	return elfErrors;
}

int CreateTaskFromELF(ELF* elf) {
	return CreateTask((UInt32) elf->start, elf->dir);
}