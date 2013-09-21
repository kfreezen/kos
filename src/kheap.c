#include <kheap.h>
#include <print.h>
#include <paging.h>

//#define KHEAP_DEBUG

extern UInt32 end;
Pointer placement_address=&end;

Heap* kHeap;

void init_kheap() {
	placement_address= &end;
}

Pointer kmalloc_int(int sz, Bool pg_align, UInt32* phys) {
	if(kHeap) {
		return kalloc_ex(sz, pg_align, phys);
	}
	
	if(pg_align) {
		placement_address = (Pointer)((UInt32) placement_address & 0xFFFFF000);
		placement_address+=0x1000;
	}
	
	Pointer tmp = placement_address;
	placement_address += sz;
	
	if(phys) {
		*phys = (UInt32) tmp;
	}
	
	return tmp;
}

Pointer kmalloc(int sz) {
	return kmalloc_int(sz, false, NULL);
}

Pointer kmalloc_a(int sz) {
	return kmalloc_int(sz, true, NULL);
}

Pointer kmalloc_ap(int sz, UInt32* phys) {
	return kmalloc_int(sz, true, phys);
}

Pointer kmalloc_p(int sz, UInt32* phys) {
	return kmalloc_int(sz, false, phys);
}

void kfree(Pointer p) {
	if(kHeap==0) {
		return;
	} else {
		heap_free(kHeap, p);
	}
}

Pointer kalloc(UInt32 size) {
	if(kHeap==NULL) {
		kprintf("kHeap is NULL.  You can't do that.\n");
		return 0;
	} else {
		return heap_alloc(kHeap, size);
	}
}

Pointer kalloc_ex(UInt32 size, Bool pg_align, UInt32* phys) {
	if(kHeap==NULL) {
		kprintf("kHeap is NULL.  You can't do that.\n");
		return 0;
	} else {
		Pointer p = heap_alloc_ex(kHeap, size, pg_align);
		if(phys) {
			*phys = (UInt32) getPhysAddr(NULL, p);
		}
		
		return p;
	}
}

void addMemory(Heap* heap, UInt32 size) {
	#ifdef KHEAP_DEBUG
	kprintf("addMemory(%x,%x)\n", heap,size);
	#endif
	
	if(heap->flags&HEAP_EXTENDABLE) {
		if(size&(HEAP_ADD_AMOUNT-1)) {
			size = (size&~(HEAP_ADD_AMOUNT-1)) + HEAP_ADD_AMOUNT;
		}
	} else {
		kprintf("Heap not extendable.\n");
		return;
	}
	
	int i;
	for(i=0; i<size/HEAP_ADD_AMOUNT; i++) {
		MapAllocatedPageBlockTo(NULL, heap->end+(i*HEAP_ADD_AMOUNT), KERNEL_PAGE_FLAGS);
	}
	HeapHeader* header = (HeapHeader*) heap->end;
	
	heap->end = heap->end+(i*HEAP_ADD_AMOUNT);
	HeapFooter* footer = (HeapFooter*) ((void*)heap->end-sizeof(HeapFooter));
	
	#ifdef KHEAP_DEBUG
	kprintf("addMemory():footer = %x, heap->end=%x\n", footer, heap->end);
	#endif
	
	header->magic_flags = HEAP_MAGIC | HEAP_FREE;
	header->footer = heap->end-sizeof(HeapFooter);
	footer->header = header;
}


void InitKernelHeap() {
	Heap* heap = kmalloc(sizeof(Heap));
	
	MapAllocatedPageBlockTo(NULL, (void*)0xC0000000, KERNEL_PAGE_FLAGS);
	void* heapData = (void*)0xC0000000;
	
	memset(heapData, 0, HEAP_ADD_AMOUNT);
	heap->start = heapData;
	heap->end = heapData+HEAP_ADD_AMOUNT;
	HeapHeader* header = (HeapHeader*) heap->start;
	HeapFooter* footer = (HeapFooter*) ((unsigned)heap->end-sizeof(HeapFooter));
	footer->header = header;
	header->magic_flags = HEAP_MAGIC | HEAP_FREE;
	header->footer = footer;
	heap->flags = HEAP_EXTENDABLE;
	
	kHeap = heap;
	
	#ifdef KHEAP_DEBUG
	kprintf("kheap=%x\n", kHeap->start);
	#endif
	
}

Heap* createHeap(UInt32 size) {
	void* heapData;
	Heap* heap;
	
	heap = kmalloc(sizeof(Heap));
	heapData = kmalloc_a(size);
	memset(heapData, 0, size);
	heap->start = heapData;
	heap->end = heapData+size;
	HeapHeader* header = (HeapHeader*) heap->start;
	HeapFooter* footer = (HeapFooter*) ((unsigned)heap->end-sizeof(HeapFooter));
	footer->header = header;
	header->magic_flags = HEAP_MAGIC | HEAP_FREE;
	header->footer = footer;
	
	heap->flags =0;
	
	return heap;
}

// This function should guarantee that the pointer returned has a minimum space of size and that the space is free.
HeapHeader* findFreeMemory(Heap* heap, UInt32 size) {
	HeapHeader* header = heap->start;
	
	HeapHeader* smallestHoleThatFits=0;
	unsigned smallestHoleSize=~0x0;
	while(1) {
		if((unsigned)header+sizeof(HeapHeader)>=(unsigned)heap->end || (unsigned)header->footer+sizeof(HeapFooter)>(unsigned)heap->end) {
			break;
		}

		int holeSize = ((unsigned)header->footer)-((unsigned)header+sizeof(HeapHeader));
		
		// This is kind-of a work around special-case that I would rather not have to cover.
		if(size==holeSize && (header->magic_flags&HEAP_FREE)==HEAP_FREE) {
			return header;
		}
		
		if(holeSize>=size && holeSize<smallestHoleSize && (header->magic_flags&HEAP_FREE)==HEAP_FREE) {
			
			smallestHoleSize = holeSize;
			smallestHoleThatFits = header;
			
			if((header->magic_flags&0xFFFF0000)!=HEAP_MAGIC) {
				// Should probably throw some horrid error, for now, we just spit out a warning and continue.
				kprintf("findFreeMemory(%x,%x).warning:  INVALID_MAGIC\n", heap, size);
				header = (HeapHeader*) ((unsigned)header->footer+sizeof(HeapFooter));
				continue;
			}
			
		}
		
		if((unsigned)header>=(unsigned)heap->end || (unsigned)header<(unsigned)heap->start) {
			break;
		}
		
		header = (HeapHeader*) ((unsigned)header->footer+sizeof(HeapFooter));
	}
	
	if((smallestHoleThatFits->magic_flags&HEAP_FREE)==0) {
		kprintf("Uh-oh, findFreeMemory screwed up bigtime.  magic_flags&HEAP_FREE==0\n");
		return 0;
	}
	
	if((Pointer)smallestHoleThatFits>=(Pointer)heap->end) {
		return 0;
	}
	
	//kprintf("smallestHoleThatFits==%x\n", smallestHoleThatFits);
	
	return smallestHoleThatFits;
}

void* heap_alloc(Heap* heap, UInt32 size) {
	return heap_alloc_ex(heap, size, FALSE);
}

void* heap_alloc_ex(Heap* heap, UInt32 size, Bool pg_align) {
	#ifdef KHEAP_DEBUG
	kprintf("heap_alloc_ex(%x,%x,%x)\n", heap, size, pg_align);
	#endif
	
	// Round up to a 4-byte alignment.
	if(size&0x3) {
		size = (size&~3)+4;
	}
	
	if(pg_align) {
		size = size+0x1000;
	}
	
	HeapHeader* header = findFreeMemory(heap, size);
	if(header==0) {
		addMemory(heap, (size>HEAP_ADD_AMOUNT) ? size : HEAP_ADD_AMOUNT);
		header = findFreeMemory(heap, size);
		if(header==0) {
			// TODO: Throw major error, go crazy.
			kprintf("AAAAHHHH!!!!! INVALID HEADER!!!!\n");
			return 0;
		}
	}
	
	HeapFooter* footer = header->footer;
	UInt32 block_size = ((unsigned)header->footer)-(((unsigned)header)+sizeof(HeapHeader));
	
	if(pg_align) {
			HeapHeader* newHeader = header;
			if(((unsigned)header+sizeof(HeapHeader))&0xFFF) { 
				newHeader = (HeapHeader*)((((unsigned) header)&0xFFFFF000)+0x1000-sizeof(HeapHeader));
			}
			
			newHeader->footer = footer;
			newHeader->magic_flags = HEAP_MAGIC;
			HeapFooter* leftFooter = (HeapFooter*)((void*)newHeader-sizeof(HeapFooter));
			if(newHeader!=header) {
				leftFooter->header = header;
				header->footer = leftFooter;
			}
			
			return (void*) newHeader+sizeof(HeapHeader);
	} else if(block_size==size && (header->magic_flags&HEAP_FREE)==HEAP_FREE) {
		header->magic_flags &= ~HEAP_FREE;
		
		#ifdef KHEAP_DEBUG
		kprintf("rc0,%x,%x\n",size,block_size);
		#endif
		
		return (void*) ((unsigned)header+sizeof(HeapHeader));
	} else if(block_size>size && block_size<size+sizeof(HeapHeader)+sizeof(HeapFooter)) {
		header->magic_flags &= ~HEAP_FREE;
		
		#ifdef KHEAP_DEBUG
		kprintf("rc1\n");
		#endif
		
		return (void*) ((unsigned)header+sizeof(HeapHeader));
	} else {
		HeapFooter* newFooter = (HeapFooter*)((unsigned)header+sizeof(HeapHeader)+size);
		HeapHeader* newHeader = (HeapHeader*)((unsigned)newFooter+sizeof(HeapFooter));
		newFooter->header = header;
		newHeader->footer = footer;
		header->footer = newFooter;
		footer->header = newHeader;
		newHeader->magic_flags = HEAP_MAGIC | HEAP_FREE;
		//kprintf("newHeader->magic_flags=%x\n", newHeader->magic_flags);
		
		header->magic_flags &= ~HEAP_FREE;
		//kprintf("header=%x, footer=%x, newHeader=%x, newFooter=%x\n", header, footer, newHeader, newFooter);
		return (void*) ((unsigned)header+sizeof(HeapHeader));
	}
	
	kprintf("Out of memory. or something like that.\n");
	return 0;
}

UInt32 getAllocSize(void* pointer) {
	HeapHeader* header = pointer-sizeof(HeapHeader);
	HeapFooter* footer = header->footer;
	if((header->magic_flags&0xFFFF0000)!=HEAP_MAGIC) {
		return 0;
	}
	
	return ((unsigned)footer)-((unsigned)header+sizeof(HeapHeader));
}

void unifyRight(Heap* heap, HeapHeader* header) {
	if(header->magic_flags&HEAP_FREE) {
		HeapFooter* footer = header->footer;
		if((unsigned)footer+sizeof(HeapHeader)+sizeof(HeapFooter)<=(unsigned)heap->end) {
			HeapHeader* rightHeader = (HeapHeader*)((void*)footer+sizeof(HeapFooter));
			if((rightHeader->magic_flags&0xFFFF0000)!=HEAP_MAGIC) {
				kprintf("rightHeader=%x. rightHeader magic invalid.\n", rightHeader);
			} else if(rightHeader->magic_flags&HEAP_FREE) {
				// footer is destroyed.
				// rightHeader is destroyed.
				header->footer = rightHeader->footer;
			
				#ifdef KHEAP_DEBUG
				kprintf("header->footer=%x\n", header->footer);
				#endif
			
				rightHeader->footer->header = header;
			
				// FIXME I don't like having to use two magics. this code should work without using two magics.
			}
		}
	}
}

void unifyLeft(Heap* heap, HeapHeader* header) {
	if(header->magic_flags&HEAP_FREE) {
		// See if there's enough room to do a unify left.
		if((unsigned)header-sizeof(HeapHeader)-sizeof(HeapFooter)>=(unsigned)heap->start) {
			HeapFooter* leftFooter = (HeapFooter*)((void*)header-sizeof(HeapFooter));
			HeapHeader* leftHeader = leftFooter->header;
			if(leftHeader->magic_flags & HEAP_FREE) {
				HeapFooter* footer = header->footer;
				// leftFooter is destroyed.
				// header is destroyed.
				leftHeader->footer = footer;
				footer->header = leftHeader;
			}
		}
	}
}

// FIXME  There is another function that is accessing unpaged memory after this is called.
void contractHeap(Heap* heap, int amountToLeave) {
	HeapHeader* lastHeader = ((HeapFooter*)((UInt32)heap->end-sizeof(HeapFooter)))->header;
	
	#ifdef KHEAP_DEBUG
	kprintf("lastHeader=%x\n", lastHeader);
	#endif
	
	/*if(lastHeader->magic_flags&HEAP_FREE) {
		UInt32 ptr = (UInt32) heap->end;
		ptr &= 0xFFFFF000;
		UInt32 stop_ptr = (UInt32) (heap->start + amountToLeave);
		if(stop_ptr<=(UInt32)lastHeader->footer+sizeof(HeapFooter)) {
			stop_ptr = ((UInt32)lastHeader->footer)+sizeof(HeapHeader)+sizeof(HeapFooter);
		}
		
		if(stop_ptr&0xFFF) {
			stop_ptr = (stop_ptr&0xFFFFF000) + 0x1000;
		}
	
		while(1) {
			if(ptr<stop_ptr) {
				break;
			}
		
			UnmapPageFrom(NULL, (void*) ptr);
			ptr -= 0x1000;
		}
		
		heap->end = (void*)ptr+0x1000;
		HeapFooter* footer = (HeapFooter*) ((void*)heap->end-sizeof(HeapFooter));
		footer->header = lastHeader;
		lastHeader->footer = footer;
	} else {
		kprintf("Last heap entry is not free.\n");
	}*/
}

void heap_free(Heap* heap, void* pointer) {
	HeapHeader* header = pointer-sizeof(HeapHeader);
	if((header->magic_flags & 0xFFFF0000) != HEAP_MAGIC) {
		kprintf("Invaild free! %x\n", pointer);
		return;
	}
	
	header->magic_flags |= HEAP_FREE;
	
	while(1) {
		if((void*)header<heap->start) {
			break;
		}
		
		unifyRight(heap, header);
		
		HeapFooter* footer = (HeapFooter*)((UInt32)header-sizeof(HeapFooter));
		if((void*)footer<heap->start) {
			break;
		}
		
		header = ((HeapFooter*)((UInt32)header-sizeof(HeapFooter)))->header;
	}
	while(((UInt32)header->footer)+sizeof(HeapFooter)<(UInt32)heap->end) {
		unifyRight(heap, header);
		header = (HeapHeader*) ((UInt32)header->footer+sizeof(HeapFooter));
		
		if((void*)header>=heap->end) {
			break;
		}
	}
	
	HeapFooter* lastFooter = (HeapFooter*)((UInt32)heap->end-sizeof(HeapFooter));
	if(lastFooter->header->magic_flags & HEAP_FREE) {
		contractHeap(heap, HEAP_ADD_AMOUNT);
	}
}

void setKernelHeap(Heap* heap) {
	kHeap = heap;
	
	#ifdef OS_DEBUG
	kprintf("KernelHeap=%x\n", kHeap);
	#endif
}

Heap* getKernelHeap() {
	return kHeap;
}

Bool isInKernelHeap(Pointer p) {
	if(p>kHeap->start && p<kHeap->end) {
		return TRUE;
	} else {
		return FALSE;
	}
}
