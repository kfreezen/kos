#ifndef ELF_H
#define ELF_H

#define EI_NIDENT 16

#include <KOSTypes.h>

typedef Pointer Elf32_Addr;
typedef UInt16 Elf32_Half;
typedef UInt32 Elf32_Off;
typedef int Elf32_Sword;
typedef UInt32 Elf32_Word;

typedef struct { 
	unsigned char e_ident[EI_NIDENT];
	Elf32_Half e_type;
	Elf32_Half e_machine;
	Elf32_Word e_version;
	Elf32_Addr e_entry;
	Elf32_Off e_phoff;
	Elf32_Off e_shoff;
	Elf32_Word e_flags;
	Elf32_Half e_ehsize;
	Elf32_Half e_phentsize;
	Elf32_Half e_phnum;
	Elf32_Half e_shentsize;
	Elf32_Half e_shnum;
	Elf32_Half e_shstrndx;
} Elf32_Ehdr; 

typedef struct {
	int error;
	Pointer executable;
	Elf32_Addr entry;
	Elf32_Off program_headers;
	Elf32_Off section_headers;
} ELF;

ELF* Parse_ELF(Pointer executable); // error is nonzero if the executable type is not supported.  It can have different values as it tells us what went wrong also.

// Various defines to make our life easier
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_PAD 7

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

#define ET_NONE	0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4

#define EM_NONE 0
#define EM_386 3

#define EV_NONE 0
#define EV_CURRENT 1

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define NO_ERROR 0
#define UNSUPPORTED_FEATURE 1
#define UNSUPPORTED_CPU_ARCH 2
#define NONCURRENT_VERSION 3
#define INVALID_MAGIC_BYTES 4
#define INVALID_ELF_CLASS 5
#define INVALID_DATA_ENCODING 6

#endif
