#include <paging.h>
#include <common/bitset.h>
#include <kheap.h>
#include <print.h>
#include <debugdef.h>

#define PAGING_DEBUG

PageDirectory staticKPageDir __attribute__((aligned(0x1000)));

Bitset* pages;
PageDirectory* kernelPageDir = NULL;
PageDirectory* currentPageDir;

extern UInt32 placement_address;

extern void _invlpg(void* ptr);

UInt32 AllocPage() {
	#ifdef PAGING_DEBUG
	kprintf("AllocPage() ");
	#endif
	
	UInt32 ret = Bitset_FirstWithValue(pages, 0);
	Bitset_Set(pages, ret, 1);
	
	#ifdef PAGING_DEBUG
	kprintf("ret %x\n", ret);
	#endif
	
	return ret;
}

void FreeAllUserPages(PageDirectory* dir) {
	int i;
	for(i=0; i<1024; i++) {
		if(dir->d[i]&PAGE_KERNEL) {
			// Don't do anything
		} else if(dir->d[i]&PAGE_PRESENT) {
			PageTable* table = (PageTable*) dir->kd[i];
			
			int j;
			for(j=0; j<1024; j++) {
				if(!(table->t[j]&PAGE_KERNEL) && table->t[j]&PAGE_PRESENT) {
					#ifdef PAGING_DEBUG
					kprintf("table=%x, table->t[%d]=%x\n", table, j, table->t[j]);
					#endif
					
					FreePage((table->t[j]&~0xFFF)>>12);
				}
			}
			
			if(isInKernelHeap(table)) {
				kfree(table);
			}
		}
	}
}

void FreePage(UInt32 page) {
	#ifdef PAGING_DEBUG
	kprintf("FreePage(%x)\n", page);
	if(page>pages->length<<5) {
		kprintf("FreePage() error.\n");
		for(;;);
	}
	#endif
	
	Bitset_Set(pages, page, 0);
}

UInt32 AllocPageBlock() {
	#ifdef PAGING_DEBUG
	//kprintf("AllocPageBlock() ");
	#endif
	
	UInt32 i;
	for(i=0; i<pages->length; i++) {
		if(pages->bitData[i]==0) { // This is an entire free page block.
			pages->bitData[i] = ~0;
			break;
		}
	}
	
	#ifdef PAGING_DEBUG
	//kprintf("ret %x,%x,%d\n", i*32, pages->bitData, pages->length);
	#endif
	
	return i*32;
}

PageDirectory* CloneDirectory(PageDirectory* dir, int type) {
	if(dir==NULL) {
		dir = currentPageDir;
	}
	
	UInt32 phys;
	UInt32 pde_flags = (type==DIR_KERNEL_TASK) ? KERNEL_PDE_FLAGS : USER_PDE_FLAGS;
	//UInt32 pte_flags = (type==DIR_KERNEL_TASK) ? KERNEL_PAGE_FLAGS : USER_PAGE_FLAGS;
	
	PageDirectory* newPageDir = kalloc_ex(sizeof(PageDirectory), true, &phys);
	memset(newPageDir, 0, sizeof(PageDirectory));
	
	newPageDir->phys = phys;
	
	int i;
	for(i=0; i<1024; i++) {
		if(dir->d[i]&PAGE_PRESENT) {
			PageTable* table = (PageTable*) ((unsigned)dir->d[i]&0xFFFFF000);
			int j;
			
			if(dir->d[i]&PAGE_KERNEL) {
				PageTable* newTable = (PageTable*) kalloc_ex(sizeof(PageTable), true, &phys);
				newPageDir->d[i] = AssemblePDE((UInt32)getPhysAddr(currentPageDir, newTable), pde_flags);
				newPageDir->kd[i] = (UInt32) newTable;
				
				for(j=0; j<1024; j++) {
					if(table->t[j]&PAGE_PRESENT && table->t[j]&PAGE_KERNEL) {
						if(table->t[j]&PAGE_STACK) {
							// Don't share.
						} else {
							// Share.
							newTable->t[j] = table->t[j];
						}
					}
				}
			} else {
				newPageDir->d[i] = dir->d[i];
				newPageDir->kd[i] = dir->kd[i];
			}
		}
	}
	
	return newPageDir;
}
		
		
PageDirectoryEntry AssemblePDE(UInt32 addr, UInt32 flags) {
	#ifdef PAGING_DEBUG_VERBOSE
	kprintf("AssemblePDE(%x,%x) ", addr, flags);
	#endif
	
	if(addr&0xFFF) {
		kprintf("Firing int 13.  AssemblePDE().\n");
		asm volatile("int $13");
		return 0;
	}
	
	#ifdef PAGING_DEBUG_VERBOSE
	kprintf("ret %x\n", (UInt32) addr | (flags&0xFFF));
	#endif
	
	return (UInt32) addr | (flags&0xFFF);
}

PageTableEntry AssemblePTE(void* address, UInt32 flags) {
	#ifdef PAGING_DEBUG_VERBOSE
	//kprintf("AssemblePTE(%x, %x)\n", address, flags);
	#endif
	
	address = (void*) ((unsigned)address & 0xFFFFF000);
	return (UInt32) address | (flags&0xFFF);
}

extern void switch_page_dir(void* ptr);

void SwitchPageDirectory(PageDirectory* dir) {
	currentPageDir = dir;
	switch_page_dir((void*)dir->phys);
}

void InitPaging(int mem_kb) {
	#ifdef PAGING_DEBUG
	kprintf("InitPaging(%d)\n", mem_kb);
	#endif
	
	UInt32 phys = 0;
	
	pages = kmalloc(sizeof(Bitset));
	int pagesAmount = mem_kb/4;
	pages->bitData = (void*) kmalloc(pagesAmount/32);
	pages->length = pagesAmount/32;
	
	#ifdef PAGING_DEBUG
	kprintf("pages->length=%d\n", pages->length);
	#endif
	
	// Do 1 to 1 paging on everything up to placement_address.
	kernelPageDir = (PageDirectory*) kmalloc_ap(sizeof(PageDirectory), &phys);
	memset(kernelPageDir, 0, sizeof(PageDirectory));
	
	kernelPageDir->d[1023] = (PageDirectoryEntry) kernelPageDir->d;
	kernelPageDir->phys = phys;
	kernelPageDir->dirType = DIR_KERNEL_TASK;
	
	#ifdef PAGING_DEBUG
	kprintf("pages=%x\npageDir=%x\n", pages, kernelPageDir);
	#endif
	
	memset(pages->bitData, 0, pages->length);
	
	// Get the amount of Page tables we will need to map up to placement address.
	if(placement_address&0xFFFFF000) {
		placement_address = (placement_address&0xFFFFF000)+0x1000;
	}
	
	// Horrible code readability by having this variable take on two roles in seperate parts of the code.
	
	pagesAmount = placement_address>>12;
	int pageTablesAmount = pagesAmount>>10;
	if(pagesAmount & 0x3FF) {
		pageTablesAmount+=1;
	}
	
	int pagesNeededToRepresentTables = pageTablesAmount;
	int extraPageTables = pagesNeededToRepresentTables>>10;
	if(pagesNeededToRepresentTables & 0x3FF) {
		extraPageTables += 1;
	}
	
	int totalPageTables = pageTablesAmount+extraPageTables;
	
	int i,j;
	for(i=0; i<totalPageTables; i++) {
		PageTable* table = (PageTable*) kmalloc_ap(sizeof(PageTable), &phys);
		
		memset(table, 0, sizeof(PageTable));
		
		for(j=0; j<1024; j++) {
			// kernel page flags = AAAGS0ADWURP
			//                   = 000000000011
			table->t[j] = AssemblePTE((void*)(i<<22)+(j<<12), KERNEL_PAGE_FLAGS);
			
			// The extra 0x4000 is for working room for page tables.
			if((i<<22)+(j<<12)>placement_address+0x4000) {
				#ifdef PAGING_DEBUG
				kprintf("InitPaging().break %d,%d,%x,%x\n", i,j,placement_address, (i<<22)+(j<<12));
				#endif
				
				break;	
			}
		}
		
		kernelPageDir->d[i] = AssemblePDE(phys, KERNEL_PDE_FLAGS);
		kernelPageDir->kd[i] = (UInt32) table;
		
		if(j<1024) {
			#ifdef PAGING_DEBUG
			kprintf("%d<1024\n", j);
			#endif
			
			break;
		}
		#ifdef PAGING_DEBUG
		kprintf("d[%d]=%x, kd[%d]=%x\n", i, kernelPageDir->d[i], i, kernelPageDir->kd[i]);
		#endif
	}
	
	int k;
	for(k=0; k<(i*1024+j)/32; k++) {
		pages->bitData[k] = ~0;
	}
	
	UInt32 used = (2<<(j&0x1F))-1;
	pages->bitData[k] = used;
	
	// On real hardware there will be memory holes.  That is why we need a map of memory that is available to the CPU
	// so we can mark the memory holes as used. TODO Obtain a memory map and use it to mark physical pages as used if
	// there is a memory hole on those pages.
	
	SwitchPageDirectory(kernelPageDir);
}

PageTable* createNewPageTable(PageDirectory* dir, void* virtualAddr) {
	#ifdef PAGING_DEBUG
	kprintf("createNewPageTable(%x, %x) ", dir, virtualAddr);
	#endif
	
	if(dir==NULL) {
		dir = currentPageDir;
	}
	
	UInt32 phys=0;
	
	PageTable* table = kmalloc_ap(0x1000, &phys);
	
	#ifdef PAGING_DEBUG
	kprintf("table=%x\n", table);
	#endif
	
	int pdindex = ((unsigned)virtualAddr)>>22;
	memset(table, 0, sizeof(PageTable));
	
	#ifdef PAGING_DEBUG
	kprintf("pdindex=%x ", pdindex);
	#endif
	
	if(dir->d[pdindex]&PAGE_PRESENT) {
		kprintf("ERROR:  Table already present. value %x\n", dir->d[pdindex]);
		return (PageTable*) ((unsigned)dir->d[pdindex]&0xFFFFF000);
	}
	
	dir->d[pdindex] = AssemblePDE(phys, KERNEL_PDE_FLAGS);	
	dir->kd[pdindex] = (UInt32) table;
	
	#ifdef PAGING_DEBUG
	kprintf("ret %x\n", table);
	kprintf("&dir->d[pdindex]=%x\n", &dir->d[pdindex]);
	#endif
	
	return table;
}

int MapAllocatedPageBlockTo(PageDirectory* dir, void* virtualAddr, int flags) {
	#ifdef PAGING_DEBUG
	//kprintf("MapAllocatedPageBlockTo(%x, %x)\n", dir, virtualAddr);
	#endif
	
	if(dir==NULL) {
		dir = currentPageDir;
	}
	
	if(!flags) {
		flags = KERNEL_PAGE_FLAGS;
	}
	
	if((unsigned)virtualAddr&0xFFF) {
		virtualAddr = (void*) ((unsigned)virtualAddr & 0xFFFFF000);
	}
	
	UInt32 pd_index = ((unsigned)virtualAddr)>>22;
	UInt32 pt_index = ((unsigned)virtualAddr)>>12 & 0x3FF;
	
	if(!(dir->d[pd_index] & PAGE_PRESENT)) {
		#ifdef PAGING_DEBUG
		kprintf("dir->d[%d]=%x\n", pd_index, dir->d[pd_index]);
		#endif
		
		createNewPageTable(dir, virtualAddr);
	}
	
	PageTable* table = (PageTable*) (dir->kd[pd_index]&0xFFFFF000);
	
	// Now make sure that all are empty.
	int i;
	for(i=0; i<32; i++) {
		if(pt_index+i>=1024) {
			break;
		}
		
		if(table->t[pt_index+i]&PAGE_PRESENT) {
			kprintf("ERROR:  Page %d at the virtual address is already mapped.\n", i);
			return -1;
		}
	}
	
	if(i<32) {
		table = (PageTable*) (dir->d[pd_index+1]&0xFFFFF000);
	
		int j;
		for(j=0; j<(32-i); j++) {
			if(table->t[pt_index+i]&PAGE_PRESENT) {
				kprintf("ERROR:  Page %d in page block is already mapped.2\n", i);
				return -1;
			}
		}
		table = (PageTable*) (dir->d[pd_index]&0xFFFFF000);
	}
	
	UInt32 page = AllocPageBlock();
	
	for(i=0; i<32; i++) {
		if(pt_index+i>=1024) {
			break;
		}
		table->t[pt_index+i] = AssemblePTE((void*)((page+i)<<12), flags);
		_invlpg(virtualAddr+((page+i)<<12));
	}
	
	if(!(dir->d[pd_index+1]&PAGE_PRESENT) && i<32) {
		#ifdef PAGING_DEBUG
		kprintf("dir->d[%d]=%x\n", pd_index+1, dir->d[pd_index+1]);
		#endif
		
		createNewPageTable(dir, (void*) (virtualAddr+(i<<12)));
	}
	
	if(i<32) {
		int j;
		table = (PageTable*) (dir->d[pd_index+1]&0xFFFFF000);
		for(j=0; j<(32-i); j++) {
			table->t[pt_index+i] = AssemblePTE((void*)((page+i)<<12), flags);
			_invlpg(virtualAddr);
		}
	}
	
	return 0;
}

void* getPhysAddr(PageDirectory* dir, void* virt) {
	if(dir==NULL) {
		dir=currentPageDir;
	}
	
	int pdindex = ((unsigned)virt)>>22;
	int ptindex = ((unsigned)virt)>>12 & 0x3FF;
	
	PageTable* table = (PageTable*) ((unsigned)dir->d[pdindex]&0xFFFFF000);
	return (void*)((unsigned)table->t[ptindex] & 0xFFFFF000);
}

Bool IsMapped(PageDirectory* dir, void* virtualAddr) {
	if(dir==NULL) {
		dir = currentPageDir;
	}

	if((unsigned)virtualAddr&0xfff) {
		virtualAddr = (void*) ((unsigned)virtualAddr & 0xFFFFF000);
	}

	int pd_index = ((unsigned)virtualAddr)>>22;
	int pt_index = ((unsigned)virtualAddr)>>12 & 0x03FF;
	
	if(!(dir->d[pd_index] & PAGE_PRESENT)) {
		return FALSE;
	}
	
	PageTable* table = (PageTable*) (dir->kd[pd_index]&0xFFFFF000);
	if(table->t[pt_index]&PAGE_PRESENT) {
		return TRUE;
	} else {
		return FALSE;
	}
}

int MapAllocatedPageTo(PageDirectory* dir, void* virtualAddr, int flags) {
	#ifdef PAGING_DEBUG
	kprintf("MapAllocatedPageTo(%x, %x)\n", dir, virtualAddr);
	#endif
	
	if(dir==NULL) {
		dir = currentPageDir;
	}
	if(!flags) {
		flags = KERNEL_PAGE_FLAGS;
	}
	
	if((unsigned)virtualAddr&0xFFF) {
		virtualAddr = (void*) ((unsigned)virtualAddr & 0xFFFFF000);
	}
	
	int pd_index = ((unsigned)virtualAddr)>>22;
	int pt_index = ((unsigned)virtualAddr)>>12 & 0x03FF;
	
	if(!(dir->d[pd_index] & PAGE_PRESENT)) {
		createNewPageTable(dir, virtualAddr);
		//kprintf("Error:  Adding page table not supported (yet).");
	}
	
	PageTable* table = (PageTable*) (dir->kd[pd_index]&0xFFFFF000);

	if(table->t[pt_index]&PAGE_PRESENT) {
		// You should know better than to try mapping something to an already present page.
		kprintf("ERROR:  Page at virtual address %x is already mapped.\n", virtualAddr);
		return -1;
	} else {
		UInt32 physAddr = AllocPage()<<12;
		kprintf("physAddr=%x, %x\n", physAddr, table->t);
		
		table->t[pt_index] = AssemblePTE((void*)physAddr, flags);
		_invlpg(virtualAddr);
	}
	
	return 0;
}

void UnmapPageFrom(PageDirectory* dir, void* ptr) {
	if(dir==NULL) {
		dir = currentPageDir;
	}
	
	int pt_index, pd_index;
	
	pd_index = ((UInt32)ptr)>>22;
	pt_index = ((UInt32)ptr>>12)&0x3ff;
	
	PageTable* table = (PageTable*) dir->kd[pd_index];
	
	UInt32 page = table->t[pt_index] & ~0xFFF;
	
	table->t[pt_index] = 0;
	FreePage(page>>12);
}
