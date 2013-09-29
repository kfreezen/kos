#include <drivers.h>
#include <paging.h>
#include <err.h>

// This keeps track of where the driver space allocator is at.
// The only problem with this approach is you could overrun when 0xB0000000
// is reached or whenever something else is in the way.
// Oh well, let's just plan on drivers being mostly static until we run into problems.
void* driverAllocPtr = NULL;


int DriversInit() {
	driverAllocPtr = (void*) DRIVER_SPACE;
	return 0;
}

//int MapAllocatedPageTo(PageDirectory* dir, void* virtAddr, int flags);
//int MapAllocatedPageBlockTo(PageDirectory* dir, void* virtAddr, int flags);

void* AllocateDriverSpace(int numPages) {
	if(driverAllocPtr == NULL) {
		// We aren't set up yet.
		// return.

		SetErr(ERR_NULL_VALUE_ENCOUNTERED);
		return NULL;
	}

	if(numPages == 0) {
		// Invalid number of pages to allocate,
		// as you can't allocate 0 pages.
		SetErr(ERR_INVALID_ARG);
		return NULL;
	}

	void* returnPtr = driverAllocPtr;

	int pageBlocksToAllocate = numPages >> 5; // 32 pages in a page block, >> 5 = / 32.
	int pagesToAllocate = numPages - (pageBlocksToAllocate << 5); // basically numPages % 32.

	kprintf("pagesToAllocate=%d\n", pagesToAllocate);

	int i;
	for(i=0; i<pageBlocksToAllocate; i++) {
		// The paging code turns this NULL into whatever the current directory is.
		MapAllocatedPageBlockTo(NULL, driverAllocPtr, PAGE_KERNEL);
		driverAllocPtr += PAGE_SIZE << 5; // One page block is PAGE_SIZE * 32 
		// and shifting left by 5 yields the same result.
	}

	for(i=0; i<pagesToAllocate; i++) {
		if(MapAllocatedPageTo(NULL, driverAllocPtr, PAGE_KERNEL)==-1) {
			kprintf("MapAllocatedPage Failure\n");
		}
		
		driverAllocPtr += PAGE_SIZE;
	}

	SetErr(SUCCESS);
	return returnPtr;

}