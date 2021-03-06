#ifndef SCREENAPI_H
#define SCREENAPI_H

#include <KOSTypes.h>

#define ERROR_NONE 0
#define ERROR_ALREADY_INITIALIZED 1
#define ERROR_INVALID_COORDINATES 2
#define ERROR_NOT_INITIALIZED 3
#define TOP_ERROR 3

#define HI_RES 1
#define LO_RES 0

#define DEFAULT_COLOR_ATTRIBUTE 0x07
#define SCREEN_DEV_NAME "screen"

#define SCREEN_WRITE_ESCAPE '\e'
#define CMD_CLEARSCREEN 0x00
#define CMD_MOVE 0x01

#define VERIFY_BYTE_0 0xAA
#define VERIFY_BYTE_1 0x55

int CLI_Init();
void CLI_SetTextMode();

int PrintChar(Char c);
int PrintString(String s);
int Move(int _x, int _y);
int MoveCursor(int _x, int _y);
int MoveCursorToCurrentCoordinates();

void ClearScreen();
void ClearError();
void SetColorAttribute(UInt8 c);

Error GetError();

void SetLineColor(int line, UInt8 color);

void EnablePrintingToStdout();
void DisablePrintingToStdout();

int Screen_Init();

#endif
