#include <elf.h>
#include <kheap.h>

ELF* Parse_ELF(Pointer executable) {
	Elf32_Ehdr* hdr = (Elf32_Ehdr*) executable;
	ELF* elf = (ELF*) kmalloc(sizeof(ELF));
	
	elf->error = NO_ERROR;
	
	// Filter out all the unsupported and invalid ELFs.
	if(hdr->e_ident[EI_MAG0]==ELFMAG0 && hdr->e_ident[EI_MAG1]==ELFMAG1 && hdr->e_ident[EI_MAG2]==ELFMAG2 && hdr->e_ident[EI_MAG3]==ELFMAG3) {
		
	} else {
		elf->error = INVALID_MAGIC_BYTES;
		return elf;	
	}
	
	if(hdr->e_ident[EI_CLASS] != ELFCLASS32) {
		elf->error = INVALID_ELF_CLASS;
		return elf;
	}
	
	if(hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		elf->error = INVALID_DATA_ENCODING;
		return elf;
	}
	
	if(hdr->e_type != ET_NONE) {
		if(hdr->e_type<=ET_CORE) {
			elf->error = UNSUPPORTED_FEATURE;
			return elf;
		}
	}
	
	if(hdr->e_type != EM_386) {
		elf->error = UNSUPPORTED_CPU_ARCH;
		return elf;
	}
	
	if(hdr->e_version == EV_NONE) {
		elf->error = NONCURRENT_VERSION;
		return elf;
	}
	
	elf->executable = executable;
	
	elf->entry = hdr->e_entry;
	elf->program_headers = hdr->e_phoff;
	elf->section_headers = hdr->e_shoff;
	
	// TODO: Finish implementation;
	return NULL;
}
