#include <common/arraylist.h>
#include <kheap.h>
#include <print.h>

#define ARRAYLIST_DEFAULT_START_CAPACITY 8

ArrayList* ALCreate() {
	ArrayList* newList = kalloc(sizeof(ArrayList));
	newList->length = 0;
	newList->capacity = ARRAYLIST_DEFAULT_START_CAPACITY;
	newList->allocatedListPtr = newList->listData = kalloc(newList->capacity*sizeof(void*));
	newList->bits = Bitset_Create((newList->capacity/32==0) ? 1 : newList->capacity/32);
	
	return newList;
}

int ALAdd(ArrayList* list, void* value) {
	if(list==NULL) {
		kprintf("ALAdd error:  list==NULL.  ALAdd(%x, %x)\n", list, value);
		return -1;
	}
	
	if(list->length>=list->capacity) {
		expand(list);
	}
	
	int idx = Bitset_FirstWithValue(list->bits, 0);
	if(idx==-1) {
		((void**)list->listData)[list->length++] = value;
	} else {
		((void**)list->listData)[idx] = value;
	}
	
	if(idx>=list->length) {
		list->length = idx+1;
	}
	
	Bitset_Set(list->bits, idx, 1);
	return idx;
}

void ALRemove(ArrayList* list, int idx) {
	void** listData = (void**) list->listData;
	listData[idx] = 0;
	Bitset* bits = list->bits;
	Bitset_Set(bits, idx, 0);
	
	// If the removal is at the end, decrement list->length.
	if(idx==list->length-1) {
		--list->length;
	}
}

void expand(ArrayList* list) {
	int newCap = list->capacity*2;
	void* newListData = kalloc(newCap*sizeof(void*));
	memcpy(newListData, list->listData, list->length*sizeof(void*));
	list->listData = newListData;
	list->capacity = newCap;
	
	kfree((void*) list->allocatedListPtr);
	
	list->allocatedListPtr = newListData;
	Bitset_Resize(list->bits, list->capacity);
}

void contract(ArrayList* list) {
	int newCap = list->capacity/2;
	if(newCap<list->length) {
		return;
	}
	
	void* newListData = kalloc(newCap*sizeof(void*));
	memcpy(newListData, list->listData, list->length*sizeof(void*));
	kfree(list->allocatedListPtr);
	
	list->allocatedListPtr = list->listData = newListData;
	list->capacity = newCap;
	Bitset_Resize(list->bits, list->capacity);
}

void* ALGetPtr(ArrayList* list, int idx) {
	return (idx>list->length) ? 0 : ((void**) list->listData)[idx];
}

UInt32 ALGetInt(ArrayList* list, int idx) {
	return (UInt32) ALGetPtr(list, idx);
}

ALIterator* ALGetItr(ArrayList* list) {
	ALIterator* itr = (ALIterator*) kalloc(sizeof(ALIterator));
	itr->list = list;
	ALItrReset(itr);
	
	return itr;
}

void ALItrReset(ALIterator* itr) {
	itr->idx = 0;
	// itr->nextIdx = -1;
	// Now I have no clue why I kept setting itr->nextIdx to -1
	// but I suppose I had a use case for it that I will find
	// someday.
	itr->nextIdx = 0;
}

Bool ALItrHasNext(ALIterator* itr) {
	//void** listData = (void**) itr->list->listData;
	int i = itr->idx;
	
	if(itr->nextIdx==-1) {
		for(i=itr->idx+1; i<itr->list->length; i++) {
			if(Bitset_Test(itr->list->bits, i)) {
				itr->nextIdx = i;
				break;
			}
		}
	} else {
		if(itr->nextIdx >= itr->list->length) {
			return FALSE;
		}
	}

	if(itr->nextIdx == -1) {
		return FALSE;
	} else {
		return TRUE;
	}
}

void* ALItrNext(ALIterator* itr) {
	if(itr->nextIdx==-1) {
		if(!ALItrHasNext(itr)) {
			return NULL;
		}
	}
	
	void** listData = (void**) itr->list->listData;
	void* tmp = listData[itr->nextIdx];
	itr->idx = itr->nextIdx;
	itr->nextIdx++;
	// itr->nextIdx = -1;
	return tmp;
}

void ALFreeItr(ALIterator* itr) {
	kfree(itr);
}

void ALFreeList(ArrayList* list) {
	kfree(list->allocatedListPtr);
	Bitset_Free(list->bits);
	kfree(list);
}
