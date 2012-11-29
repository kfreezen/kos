#ifndef PRINT_H
#define PRINT_H

#include <KOSTypes.h>

int PutChar(Char c);
int PutCharEx(Char c, Bool str_use);

int PutStringEx(String s, Bool no_move_csr);
int PutString(String s);

void PutHexEx(UInt32 num, Bool noZeroes);
void PutHex(UInt32 num);

void PutDec(UInt32 num);

void PutFloat(float n);

void kprintf(const char* fmt, ...);

void Cls();
void ClsEx(UInt8 color);

#endif
