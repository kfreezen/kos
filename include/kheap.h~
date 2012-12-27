#ifndef KHEAP_H
#define KHEAP_H

#include <KOSTypes.h>

Pointer kmalloc(int sz);
Pointer kmalloc_a(int sz);
Pointer kmalloc_ap(int sz, UInt32* phys);
Pointer kmalloc_p(int sz, UInt32* phys);

#define HEAP_MAGIC 0xF00D0000
#define HEAP_DESTROYED_MAGIC 0x0DE52205

#define HEAP_FLAGS_MASK 0x0000FFFF

#define HEAP_MIN_SPACE_REQUEST 4096

#define HEAP_FREE 1<<0
#define HEAP_MIN_SIZE 0x80000

typedef struct __Heap_Header HeapHeader;
typedef struct __Heap_Footer HeapFooter;

struct __Heap_Footer {
	HeapHeader* header;
} __attribute__((packed));
 
struct __Heap_Header {
	unsigned magic_flags;
	HeapFooter* footer;
} __attribute__((packed));

typedef struct __Heap {
	void* start;
	void* end;
	int flags;
} Heap;

#define HEAP_EXTENDABLE 1<<0
#define HEAP_ADD_AMOUNT 0x20000 // 128K

void init_kheap();

Pointer heap_alloc(Heap* heap, UInt32 size);
Pointer heap_alloc_ex(Heap* heap, UInt32 size, Bool pg_align);

void heap_free(Heap* heap, void* ptr);
Heap* createHeap(UInt32 size);

void setKernelHeap(Heap* heap);
Heap* getKernelHeap();
void InitKernelHeap();

Pointer kalloc(UInt32 size);
Pointer kalloc_ex(UInt32 size, Bool pg, UInt32* phys);
void kfree(Pointer p);

Bool isInKernelHeap(Pointer p);

#endif
