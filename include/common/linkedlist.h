#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <KOSTypes.h>

struct SinglyLinkedNode;

typedef struct SinglyLinkedNode {
	void* ptr;
	struct SinglyLinkedNode* next;
} SinglyLinkedNode;

typedef struct {
	SinglyLinkedNode* start;
	SinglyLinkedNode* end;
} SinglyLinkedList;

void SLL_Add(SinglyLinkedList* list, SinglyLinkedNode* node);
Bool SLL_Remove(SinglyLinkedList* list, SinglyLinkedNode* node);

SinglyLinkedNode* SLN_Create(void* ptr);

#endif
