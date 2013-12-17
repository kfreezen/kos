#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <common/arraylist.h>

typedef struct {
	ArrayList* TYPE(DictionarySet*) dictList;
} Dictionary;

typedef struct {
	UInt32 keyHash; // See the internal hashCode function...
	void* item;
} DictionarySet;

Dictionary* Dict_Create();

void* Dict_Get(Dictionary* dict, const char* key);
void Dict_Set(Dictionary* dict, const char* key, void* item);
int Dict_Delete(Dictionary* dict, const char* key);

void* Dict_GetByHash(Dictionary* dict, UInt32 keyHash);
void Dict_SetByHash(Dictionary* dict, UInt32 keyHash, void* item);
int Dict_DeleteByHash(Dictionary* dict, UInt32 keyHash);

#endif