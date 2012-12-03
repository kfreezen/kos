#include <common/linkedlist.h>

void SLL_Add(SinglyLinkedList* list, SinglyLinkedNode* node) {
	list->end->next = node;
	list->end = node;
	node->next = 0;
}

Bool SLL_Remove(SinglyLinkedList* list, SinglyLinkedNode* node) {
	SinglyLinkedNode* search = list->start;
	SinglyLinkedNode* searchNext = list->start->next;
	while(searchNext!=node) {
		search = searchNext;
		searchNext = searchNext->next;
		
		if(searchNext==NULL) {
			return FALSE;
		}
	}
	
	search->next = searchNext->next;
	searchNext->next = 0;
	
	return TRUE;
}

SinglyLinkedNode* SLN_Create(void* ptr) {
	SinglyLinkedNode* node = (SinglyLinkedNode*) kalloc(sizeof(SinglyLinkedNode));
	node->ptr = ptr;
	return node;
}
