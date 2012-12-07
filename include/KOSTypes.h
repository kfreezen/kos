#ifndef KOSTYPES_H
#define KOSTYPES_H

#define true 1
#define false 0
#define TRUE true
#define FALSE false

#define NULL 0

#define MAX(a,b) ((a)<(b)) ? (b) : (a)

#define MB_TO_KB(i) ((i)*1024)
//#define DEBUG
//#define PAGING_DEBUG

typedef char Char;
typedef Char* String;

typedef int Bool;
typedef int size_t;
typedef int Int32;

typedef short Short;
typedef short Int16;
typedef unsigned short UInt16;
typedef UInt16 UShort;
typedef short wchar_t;

typedef unsigned char Byte;
typedef char Int8;
typedef unsigned char UInt8;

typedef unsigned int UInt32;
typedef unsigned long long UInt64;

typedef Int32 Error;
typedef void* Pointer;
typedef void* GeneralType;

void memset(Pointer p, UInt8 value, size_t size);
void memcpy(Pointer dest, Pointer src, size_t size);
Int32 strlen(const char* s);
void strcpy(char* dest, const char* source);
int strncpy(char* dest, size_t size, const char* source);

int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, size_t size, const char* s2);

void outb(UInt16 port, UInt8 value);
void outw(UInt16 port, UInt16 value);
void outl(UInt16 port, UInt32 value);

UInt8 inb(UInt16 port);
UInt16 inw(UInt16 port);
UInt32 inl(UInt16 port);
#endif
