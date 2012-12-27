#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <KOSTypes.h>

#define KB_BUF_SIZE 512

#define LEFT_SHIFT_SCANCODE 0x2a

#define KB_SYS_POLLCHAR 0x0

void KB_Init(int ne);
void SetNoEcho(int ne);
void SetNonblocking(int nb);

typedef struct __kb_map {
	char name[8];
	Byte map[256];
} KB_Map;

void WaitForKeypress();
char* GetBuffer(); // Gets the current state of the buffer and clears it.
char* GetLine(char c); // Returns the buffer and clears it if the last non-null character is 'c', else returns NULL

Byte KB_GetChar();
#endif
