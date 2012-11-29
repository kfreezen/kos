#ifndef ORDERED_ARRAY_H
#define ORDERED_ARRAY_H

#include <KOSTypes.h>

typedef Bool (*LessthanPredicate)(GeneralType, GeneralType);

typedef struct {
	GeneralType* array;
	UInt32 size;
	UInt32 max_size;
	LessthanPredicate lessThan;
} OrderedArray;

Bool standardLessthan(GeneralType a, GeneralType b);

OrderedArray PlaceOrderedArray(Pointer addr, UInt32 size, LessthanPredicate lessThan);

void OA_Insert(OrderedArray* oa, GeneralType item);
GeneralType OA_Lookup(OrderedArray* oa, UInt32 i);
void OA_Remove(OrderedArray* oa, UInt32 i);
void OA_RemoveMatch(OrderedArray* oa, GeneralType item);
void OA_RemoveMatches(OrderedArray* oa, GeneralType item);

#endif
