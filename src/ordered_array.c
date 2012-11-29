#include <ordered_array.h>

Bool standardLessthan(GeneralType a, GeneralType b) {
	return (a<b) ? true:false;
}

OrderedArray PlaceOrderedArray(Pointer addr, UInt32 size, LessthanPredicate lessThan) {
	OrderedArray ret;
	ret.array = (GeneralType*) addr;
	ret.size = 0;
	ret.max_size = size;
	if(lessThan) {
		ret.lessThan = lessThan;
	} else {
		ret.lessThan = standardLessthan;
	}
	
	return ret;
}

void OA_Insert(OrderedArray* oa, GeneralType item) {
	if(!oa->lessThan) {
		oa->lessThan = standardLessthan;
	}
	
	UInt32 itr=0;
	while(itr<oa->size && oa->lessThan(oa->array[itr], item)) {
		itr++;
	}
	if(itr==oa->size) {
		// Add at the end of array.
		oa->array[oa->size++] = item;
	} else {
		GeneralType tmp = oa->array[itr];
		oa->array[itr] = item;
		while(itr < oa->size) {
			itr++;
			GeneralType tmp2 = oa->array[itr];
			oa->array[itr] = tmp;
			tmp = tmp2;
		}
		oa->size++;
	}
}

extern void mov_eax(UInt32 i);

GeneralType OA_Lookup(OrderedArray* oa, UInt32 i) {
	if(i>=oa->size) {
		// TODO:  Implement kernel error interrupt.  for now, freeze.
		for(;;) {}
	}
	
	return oa->array[i];
}

void OA_Remove(OrderedArray* oa, UInt32 i) {
	while(i<oa->size) {
		oa->array[i] = oa->array[i+1];
		i++;
	}
	oa->size--;
}

void OA_RemoveMatch(OrderedArray* oa, GeneralType item) {
	int i=0;
	// Find the match.
	while(i<oa->size) {
		if(oa->array[i] == item) {
			OA_Remove(oa, i);
			break;
		}
	}
}

void OA_RemoveMatches(OrderedArray* oa, GeneralType item) {
	int i=0;
	// Find the match.
	while(i<oa->size) {
		if(oa->array[i] == item) {
			OA_Remove(oa, i);
		}
	}
}
