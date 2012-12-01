#include <kheap.h>

#include <kheap.t.h>
#include <test.h>
#include <print.h>

#define VERIFY_BLOCK_FREE "VERIFIED BLOCK:  FREE"
#define VERIFY_BLOCK_USED "VERIFIED BLOCK:  USED"

#define VERIFY_FAILED "VERIFY NOT PASSED:  "
#define VERIFY_PASSED "VERIFY PASSED:  "

#define VERIFY_FAILED_WRONG_MAGIC VERIFY_FAILED "INVALID MAGIC"
#define VERIFY_FAILED_WRONG_LENGTH VERIFY_FAILED "WRONG LENGTH"
#define VERIFY_FAILED_HEADER_NO_MATCH VERIFY_FAILED "header!=header->footer->header"

#define VERIFY_PASSED_FREE VERIFY_PASSED "FREE"
#define VERIFY_PASSED_USED VERIFY_PASSED "USED"
#define VERIFY_SUCCESS_BLOCK_NOT_VALID VERIFY_PASSED "BLOCK NOT VALID, HEADER HAS BEEN DESTROYED"

#define KALLOC 0
#define KFREE 1

typedef struct {
	int errCode;
	char* err;
} Verification;

Verification verifyKAllocProper(void* ptr, int len, int alloc) {
	HeapHeader* header = ptr-sizeof(HeapHeader);
	
	if(header->magic_flags == HEAP_DESTROYED_MAGIC && alloc==KFREE) {
		return (Verification) {SUCCESS, VERIFY_SUCCESS_BLOCK_NOT_VALID};
	}
	
	if((header->magic_flags & 0xFFFF0000) != HEAP_MAGIC) {
		return (Verification) {FAIL, VERIFY_FAILED_WRONG_MAGIC};
	}
	
	// Now verify the length.
	if(len!=-1) {
		void* footer = (void*) header->footer;
		if(footer-ptr!=len) {
			return (Verification) {FAIL, VERIFY_FAILED_WRONG_LENGTH};
		}
	}
	
	// Integrity check.  Make sure the footer's header pointer matches the actual header.
	if(header->footer->header != header) {
		kprintf("Header=%x ", header);
		return (Verification) {FAIL, VERIFY_FAILED_HEADER_NO_MATCH};
	}
	
	if(header->magic_flags & HEAP_FREE) {
		return (Verification) {SUCCESS, VERIFY_PASSED_FREE};
	} else {
		return (Verification) {SUCCESS, VERIFY_PASSED_USED};
	}
}

int testKHeap() {
	void* p[16];
	
	int i;
	int totalAllocated=0;
	int totalOverhead=0;
	for(i=0; i<16; i++) {
		p[i] = kalloc(i*4);
		kprintf("AfterKalloc");
		totalAllocated+=i*4;
		totalOverhead += sizeof(HeapHeader)+sizeof(HeapFooter);
		
		Verification v = verifyKAllocProper(p[i], i*4, KALLOC);
		
		kprintf("%d: %d, %s\n", i, v.errCode, v.err);
		
		if(v.errCode==FAIL) {
			return FAIL;
		}
	}
	
	kprintf("TOTAL ALLOCATED+OVERHEAD=%x\n", totalAllocated+totalOverhead);
	
	// TEST UNIFY-LEFT
	int accumulatedTotal=0;
	int accumulatedOverhead = 0;
	
	for(i=0; i<16; i++) {
		accumulatedTotal+=i*4;
		accumulatedOverhead+=sizeof(HeapHeader)+sizeof(HeapFooter);
		
		kfree(p[i]);
		HeapHeader* header = ((void*)p[i]-sizeof(HeapHeader));

		Verification v = verifyKAllocProper(p[i], -1, KFREE);
		if(v.errCode==FAIL) {
			kprintf("kfree: %d: %s,%x,%x\n", i, v.err, header->magic_flags, p[i]);
			return FAIL;
		}
		
	}
	
	HeapHeader* header = (HeapHeader*)(getKernelHeap()->start);
	void* pointer = (void*)header+sizeof(HeapHeader);
	int size = (void*)header->footer-pointer;
	if(size+sizeof(HeapHeader)+sizeof(HeapFooter)!=accumulatedTotal+accumulatedOverhead) {
		kprintf("ACCUMULATED TOTAL+OVERHEAD=%x, size=%x\n", accumulatedTotal + accumulatedOverhead, size+sizeof(HeapHeader)+sizeof(HeapFooter));
	}
	
	totalAllocated=0;
	totalOverhead = 0;
	
	for(i=0; i<16; i++) {
		p[i] = kalloc(i*4);
		totalAllocated+=i*4;
		totalOverhead += sizeof(HeapHeader)+sizeof(HeapFooter);
		
		Verification v = verifyKAllocProper(p[i], i*4, KALLOC);
		
		if(v.errCode==FAIL) {
			kprintf("%d: %d, %s\n", i, v.errCode, v.err);
			return FAIL;
		}
	}
	
	kprintf("TOTAL ALLOCATED+OVERHEAD=%x\n", totalAllocated+totalOverhead);
	
	// TEST UNIFY-RIGHT
	accumulatedTotal=0;
	accumulatedOverhead = 0;
	
	for(i=15; i>=0; i--) {
		accumulatedTotal+=i*4;
		accumulatedOverhead+=sizeof(HeapHeader)+sizeof(HeapFooter);
		
		kfree(p[i]);
		HeapHeader* pHeader = ((void*)p[i]-sizeof(HeapHeader));

		Verification v = verifyKAllocProper(p[i], -1, KFREE);
		if(v.errCode==FAIL) {
			kprintf("kfree: %d: %s,%x,%x\n", i, v.err, pHeader->magic_flags, p[i]);
			return FAIL;
		}
		
	}
	return SUCCESS;
}


