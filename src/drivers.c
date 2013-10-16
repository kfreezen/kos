#include <drivers.h>
#include <paging.h>
#include <err.h>
#include <vfs.h>

// #define DRIVERS_DEBUG

// This keeps track of where the driver space allocator is at.
// The only problem with this approach is you could overrun when 0xB0000000
// is reached or whenever something else is in the way.
// Oh well, let's just plan on drivers being mostly static until we run into problems.
void* driverAllocPtr = NULL;

typedef struct {
	unsigned address;
	int str_offset;
} SymEntry;

#define HDR_MAGIC 0xFACEF00D

typedef struct {
	unsigned magic; // 0xFACEF00D
	int symentries_offset;
	int strtab_offset;
	int entries;
} Header;

Header* kernelMapHeader = NULL;
int numKernelSyms = 0;
SymEntry* kernelSyms = NULL;
char* kernelMapStrtab = NULL;


int DriversInit() {
	driverAllocPtr = (void*) DRIVER_SPACE;
	return 0;
}

void KernelSymbolsLoad() {
	// Load kernel.map
	File* file = GetFileFromPath("/sys/kernel.map");
	
	FileSeek(SEEK_EOF, file);
	int length = FileTell(file);

	FileSeek(0, file);
	
	#ifdef DRIVERS_DEBUG
	kprintf("length=%d,%x\n", length, length);
	#endif

	void* buf = kalloc(length);

	ReadFile(buf, length, file);
	
	Header* hdr = (Header*) buf;
	kernelSyms = buf + hdr->symentries_offset;
	kernelMapStrtab = buf+hdr->strtab_offset;
	
	numKernelSyms = hdr->entries;
	CloseFile(file);
}

void* getKernelSymbol(const char* sym) {
	int i;
	for(i=0; i < numKernelSyms; i++) {
		if(!strcmp(sym, &kernelMapStrtab[kernelSyms[i].str_offset])) {
			return (void*) kernelSyms[i].address;
		}
	}

	return NULL;
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

	#ifdef DRIVERS_DEBUG
	kprintf("pagesToAllocate=%d\n", pagesToAllocate);
	#endif

	int i;
	for(i=0; i<pageBlocksToAllocate; i++) {
		// The paging code turns this NULL into whatever the current directory is.
		MapAllocatedPageBlockTo(NULL, driverAllocPtr, KERNEL_PAGE_FLAGS);
		driverAllocPtr += PAGE_SIZE << 5; // One page block is PAGE_SIZE * 32 
		// and shifting left by 5 yields the same result.
	}

	for(i=0; i<pagesToAllocate; i++) {
		if(MapAllocatedPageTo(NULL, driverAllocPtr, KERNEL_PAGE_FLAGS)==-1) {
			kprintf("MapAllocatedPage Failure\n");
		}
		
		driverAllocPtr += PAGE_SIZE;
	}

	SetErr(SUCCESS);
	return returnPtr;

}