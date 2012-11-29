#include <common/bitset.h>
#include <kheap.h>
#include <print.h>

void Bitset_Copy(Bitset* dest, Bitset* src) {
	if(dest==NULL || src==NULL) {
		return;
	}
	
	int length = MAX(dest->length, src->length);
	memcpy(dest->bitData, src->bitData, length);
}

Bitset* Bitset_Create(UInt32 length) {
	Bitset* set = kalloc(sizeof(Bitset));
	if(set==NULL) {
		return NULL;
	}
	
	set->bitData = kalloc(length*sizeof(UInt32));
	set->length = length;
	return set;
}

// A faster implementation.
int Bitset_FirstWithValue(Bitset* bits, int value) {
	if(bits==NULL) {
		return -1;
	}
	int testvalue = (value&1) ? 0 : ~0;
	
	int i;
	for(i=0; i<bits->length; i++) {
		if(bits->bitData[i] != testvalue) {
			int j;
			for(j=0; j<32; j++) {
				int bit = (bits->bitData[i]>>j)&1;
				if(bit == value) {
					return i*32+j;
				}
			}
		}
	}
	
	return -1;
}

void Bitset_Resize(Bitset* bits, int size) {
	if(bits==NULL) {
		kprintf("Invalid argument: bits==NULL\n");
		return;
	}
	
	if(size<32) {
		size = 32;
	}
	
	UInt32* bitsData = kalloc(size/32*sizeof(UInt32));
	memcpy(bitsData, bits->bitData, size/32*sizeof(UInt32));
	
	bits->length = size/32;
	bits->bitData = bitsData;
}
