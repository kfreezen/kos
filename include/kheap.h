#ifndef KHEAP_H
#define KHEAP_H

#include <KOSTypes.h>

Pointer kmalloc(int sz);
Pointer kmalloc_a(int sz, Bool align);
Pointer kmalloc_ap(int sz, Bool align, UInt32* phys);

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
} Heap;

void init_kheap();

Pointer heap_alloc(Heap* heap, UInt32 size);
void heap_free(Heap* heap, void* ptr);
Heap* createHeap(UInt32 size);
void setKernelHeap(Heap* heap);
Heap* getKernelHeap();

Pointer kalloc(UInt32 size);
void kfree(Pointer p);

#endif
