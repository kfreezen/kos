#include <common/dictionary.h>
#include <common/arraylist.h>

#include <kheap.h>

UInt32 hashCode(const char* str) {
	// Speed is of utmost importance... right?
	register UInt32 hash = 0;

	while(*str) {
		hash = ((hash << 5) - hash) + *(str++);
	}

	return hash;
}

Dictionary* Dict_Create() {
	Dictionary* dict = kalloc(sizeof(Dictionary));
	dict->dictList = ALCreate();

	return dict;
}

void* Dict_Get(Dictionary* dict, const char* key) {
	UInt32 keyHash = hashCode(key);
	return Dict_GetByHash(dict, keyHash);
}

void Dict_Set(Dictionary* dict, const char* key, void* item) {
	UInt32 keyHash = hashCode(key);

	Dict_SetByHash(dict, keyHash, item);
}

Bool Dict_Delete(Dictionary* dict, const char* key) {
	UInt32 keyHash = hashCode(key);

	return Dict_DeleteByHash(dict, keyHash);
}

void* Dict_GetByHash(Dictionary* dict, UInt32 keyHash) {
	ALIterator* itr = ALGetItr(dict->dictList);
	void* foundValue = NULL;
	while(ALItrHasNext(itr)) {
		DictionarySet* set = (DictionarySet*) ALItrNext(itr);
		if(set->keyHash == keyHash) {
			// We found our match.
			foundValue = set->item;
			break;
		}
	}

	ALFreeItr(itr);

	return foundValue;
}

void Dict_SetByHash(Dictionary* dict, UInt32 keyHash, void* item) {
	ALIterator* itr = ALGetItr(dict->dictList);
	int didSetItem = 0;
	while(ALItrHasNext(itr)) {
		DictionarySet* set = (DictionarySet*) ALItrNext(itr);
		if(set->keyHash == keyHash) {
			set->item = item;
			didSetItem = 1;
		}
	}

	ALFreeItr(itr);

	if(!didSetItem) {
		DictionarySet* set = kalloc(sizeof(DictionarySet));
		set->keyHash = keyHash;

		ALAdd(dict->dictList, set);
	}
}

int Dict_DeleteByHash(Dictionary* dict, UInt32 keyHash) {
	ALIterator* itr = ALGetItr(dict->dictList);
	while(ALItrHasNext(itr)) {
		DictionarySet* set = (DictionarySet*) ALItrNext(itr);
		if(set->keyHash == keyHash) {
			ALRemove(dict->dictList, ALItrGetIndex(itr));
			return TRUE;
		}
	}

	return FALSE;
}