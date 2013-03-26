#include <common/linkedlist.h>
#include <kheap.h>

void DLL_Prepend(DoublyLinkedList* list, DoublyLinkedNode* node) {
	DoublyLinkedNode* oldStartNode = list->start;
	list->start = node;
	oldStartNode->prev = list->start;
	list->start->next = oldStartNode;
	list->start->prev = NULL;
}

void DLL_Append(DoublyLinkedList* list, DoublyLinkedNode* node) {
	DoublyLinkedNode* oldEndNode = list->end;
	list->end = node;
	oldEndNode->next = list->end;
	list->end->prev = oldEndNode;
	list->end->next = NULL;
}

Bool DLL_Remove(DoublyLinkedList* list, DoublyLinkedNode* node) {
	DoublyLinkedNode* prevNode, *nextNode;
	prevNode = node->prev;
	nextNode = node->next;
	prevNode->next = nextNode;
	nextNode->prev = prevNode;
	kfree(node);
	
	return true;
}

DoublyLinkedNode* DLN_Create(void* ptr) {
	DoublyLinkedNode* node;
	node = kalloc(sizeof(DoublyLinkedNode));
	node->prev = node->next = NULL;
	node->data = ptr;
	return node;
}

DoublyLinkedList* DLL_Create() {
	DoublyLinkedList* list;
	list = kalloc(sizeof(DoublyLinkedList));
	list->start = NULL;
	list->end = NULL;
	return list;
}
