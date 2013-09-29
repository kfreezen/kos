#ifndef ARRAYLIST_H
#define ARRAYLIST_H

#define TYPE(a) // <sarcasm>Use for "readability" (like #define's contribute to code readability)</sarcasm>  Actually though, I think it could cut down on bugs, so USE IT IN EVERY ARRAYLIST DECLARATION PLEASE.

#include <common/bitset.h>

typedef struct {
	void* listData;
	void* allocatedListPtr; // The pointer returned by kalloc().  Used so that the listData pointer can change.
	Bitset* bits;
	int length;
	int capacity;
} ArrayList;

typedef struct {
	int idx;
	int nextIdx;
	ArrayList* list;
} ALIterator;

ArrayList* ALCreate();

void* ALGetPtr(ArrayList* list, int idx);
UInt32 ALGetInt(ArrayList* list, int idx);

ArrayList* ALCopy(ArrayList* list);

int ALAdd(ArrayList* list, void* value);

void expand(ArrayList* list);
void contract(ArrayList* list);

ALIterator* ALGetItr(ArrayList* list);
void ALItrReset(ALIterator* itr);
Bool ALItrHasNext(ALIterator* itr);
void* ALItrNext(ALIterator* itr);
void ALFreeItr(ALIterator* itr);
void ALFreeList(ArrayList* list);
void ALClear(ArrayList* list, int doFreePointers);
void ALFreePointers(ArrayList* list);

#define ALFreeIterator(a) ALFreeItr(a)
#endif
