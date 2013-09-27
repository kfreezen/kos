#ifndef ELF_H
#define ELF_H

#define EI_NIDENT 16

#include <KOSTypes.h>
#include <paging.h>

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
	Elf32_Word sh_name;
	Elf32_Word sh_type;
	Elf32_Word sh_flags;
	Elf32_Addr sh_addr;
	Elf32_Off sh_offset;
	Elf32_Word sh_size;
	Elf32_Word sh_link;
	Elf32_Word sh_info;
	Elf32_Word sh_addralign;
	Elf32_Word sh_entsize;
} Elf32_Shdr;

struct Elf32_Phdr {
	Elf32_Word p_type;
	Elf32_Off p_offset;
	Elf32_Addr p_vaddr;
	Elf32_Addr p_paddr;
	Elf32_Word p_filesz;
	Elf32_Word p_memsz;
	Elf32_Word p_flags;
	Elf32_Word p_align;
} __attribute__((packed));

typedef struct Elf32_Phdr Elf32_Phdr;

struct Elf32_Sym {
	Elf32_Word st_name;
	Elf32_Addr st_value;
	Elf32_Word st_size;
	unsigned char st_info;
	unsigned char st_other;
	Elf32_Half st_shndx;
};

typedef struct Elf32_Sym Elf32_Sym;
typedef struct {
	int error;
	Pointer start;
	PageDirectory* dir;
} ELF;

ELF* Parse_ELF(Pointer executable); // error is nonzero if the executable type is not supported.  It can have different values as it tells us what went wrong also.
int CreateTaskFromELF(ELF* elf);

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
#define WRONG_VERSION 3
#define INVALID_MAGIC_BYTES 4
#define INVALID_ELF_CLASS 5
#define INVALID_ENDIAN 6

#define SHN_UNDEF 0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define SHN_HIRESERVE 0xffff

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7FFFFFFF
#define SHT_LOUSER 0x80000000
#define SHT_HIUSER 0xffffffff

#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MASKPROC 0xf0000000

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOPROC 13
#define STB_HIPROC 15

#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_LOPROC 13
#define STT_HIPROC 15

#define PF_W 0x1
#define PF_R 0x2
#define PF_X 0x4

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

// Determine symbol index and type of relocation with these two defines
#define ELF32_R_SYM(i) ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))

// Compile symbol index and type into one ELF-compliant format
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))

// Get symbol binding, which determines linkage
// visibility and behaviour.
#define ELF32_ST_BIND(i) ((i)>>4)

// Get symbol type, which provides a general
// classification for the associated entity.
#define ELF32_ST_TYPE(i) ((i)&0xF)

// Combine binding and type into one integer.
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))


char** GetElfErrors();
#endif
