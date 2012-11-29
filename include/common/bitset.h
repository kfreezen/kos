#ifndef BITSET_H
#define BITSET_H

#include <KOSTypes.h>

typedef struct {
	UInt32 length; // In UInt32's
	UInt32* bitData;
} Bitset;

void Bitset_Copy(Bitset* dest, Bitset* src);

Bitset* Bitset_Create(UInt32 length);

extern int Bitset_Test(Bitset* set, int nthBit);
extern int Bitset_Set(Bitset* set, int toSet, int val);

void Bitset_Resize(Bitset* bits, int size);

int Bitset_FirstWithValue(Bitset* bits, int val);

#endif
