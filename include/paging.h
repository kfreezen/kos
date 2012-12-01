#ifndef PAGING_H
#define PAGING_H

#include <KOSTypes.h>

typedef UInt32 PageDirectoryEntry;
typedef UInt32 PageTableEntry;

typedef struct __page_dir {
	PageDirectoryEntry d[1024];
	PageDirectoryEntry kd[1024]; // All the addresses as seen by the kernel. (physical or virtual?)
	UInt32 phys;
} PageDirectory;

typedef struct __page_table {
	PageTableEntry t[1024];
} PageTable;

PageDirectoryEntry AssemblePDE(PageTable* pt, UInt32 flags);
PageTableEntry AssemblePTE(void* page, UInt32 flags);

Pointer getPhysAddr(PageDirectory* dir, Pointer virt);

void SwitchPageDirectory(PageDirectory* dir);

PageDirectory* CloneDirectory(PageDirectory* src);
PageDirectory* CreateNewAddressSpace(PageDirectory* krnl);

void InitPaging(int mem_kb);
int MapAllocatedPageTo(PageDirectory* dir, void* virtAddr);
int MapAllocatedPageBlockTo(PageDirectory* dir, void* virtAddr);
#define PAGE_DIR_SIZE (sizeof(PageDirectoryEntry)*1024)
#define PAGE_TABLE_SIZE (sizeof(PageTableEntry)*1024)

#define PAGE_SIZE 0x1000
#define MEGABYTE (1024*1024)
#define PAGE_MB_SIZE (4*MEGABYTE)

#define PAGE_GRAN(i) ((i) / PAGE_SIZE)
#define ADDRESS_OF_PAGE(i) ((i)*PAGE_SIZE)

#define READ_WRITE_PAGE 1<<1
#define PRESENT PAGE_PRESENT // DEPRECATED:  Use PAGE_PRESENT instead.
#define PAGE_PRESENT 1<<0

#define PAGE_AVAILABLE_BIT0 1<<9
#define PAGE_AVAILABLE_BIT1 1<<10
#define PAGE_AVAILABLE_BIT2 1<<11

#define PAGE_KERNEL PAGE_AVAILABLE_BIT0

#define KERNEL_PDE_FLAGS READ_WRITE_PAGE | PRESENT
#define KERNEL_PAGE_FLAGS PAGE_KERNEL | READ_WRITE_PAGE | PRESENT

#endif
