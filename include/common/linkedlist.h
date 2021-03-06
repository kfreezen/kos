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

typedef struct DoublyLinkedNode {
	struct DoublyLinkedNode* prev;
	void* data;
	struct DoublyLinkedNode* next;
} DoublyLinkedNode;

typedef struct {
	DoublyLinkedNode* start;
	DoublyLinkedNode* end;
} DoublyLinkedList;

void DLL_Prepend(DoublyLinkedList* list, DoublyLinkedNode* node);
void DLL_Append(DoublyLinkedList* list, DoublyLinkedNode* node);
Bool DLL_Remove(DoublyLinkedList* list, DoublyLinkedNode* node);
DoublyLinkedList* DLL_Create();

DoublyLinkedNode* DLN_Create(void* ptr);

#endif
