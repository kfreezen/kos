#ifndef PAGING_H
#define PAGING_H

#include <KOSTypes.h>

typedef UInt32 PageDirectoryEntry;
typedef UInt32 PageTableEntry;

typedef struct __page_dir {
	PageDirectoryEntry d[1024];
	PageDirectoryEntry kd[1024]; // All the addresses as seen by the kernel.
	UInt32 phys;
} PageDirectory;

typedef struct __page_table {
	PageTableEntry t[1024];
} PageTable;

PageDirectoryEntry AssemblePDE(Pointer pt, UInt32 flags);
PageTableEntry AssemblePTE(Pointer page, UInt32 flags);

Pointer getPhys(Pointer virt);

void SwitchPageDirectory(PageDirectory* dir);

int alloc_page(PageDirectory* pd, Pointer virt_addr);
int alloc_page_ex(PageDirectory* pd, Pointer virt_addr, Bool do_firstSwitchDir, Bool do_lastSwitchDir);
int alloc_pages(PageDirectory* pd, Pointer virt_addr_start, Pointer virt_addr_end);
PageDirectory* CloneDirectory(PageDirectory* src);
PageDirectory* CreateNewAddressSpace(PageDirectory* krnl);

void init_orig_dir(); // Initializes the original kernel dir reference for multitasking.
#define PAGE_DIR_SIZE (sizeof(PageDirectoryEntry)*1024)
#define PAGE_TABLE_SIZE (sizeof(PageTableEntry)*1024)

#define PAGE_SIZE 0x1000
#define MEGABYTE (1024*1024)
#define PAGE_MB_SIZE (4*MEGABYTE)

#define PAGE_GRAN(i) ((i) / PAGE_SIZE)
#define ADDRESS_OF_PAGE(i) ((i)*PAGE_SIZE)

#endif
