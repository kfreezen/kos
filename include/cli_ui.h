#ifndef CLI_UI_H
#define CLI_UI_H

#include <KOSTypes.h>

void EnableCliUI();
void DisableCliUI();

Bool IsEnabledCliUI();

typedef struct {
	int x, y;
	int width, height;
} Rectangle;

typedef struct {
	Rectangle r;
	//int x, y;
	//int width, height;
	int termX, termY;
} Terminal;

void SetTerminalArea(int i, Terminal rect);
void SwitchTerminalArea(int i);

Terminal GetTerminalArea();

void SetTitle(const char* title);

void SaveTerminal(Terminal* term);
void LoadTerminal(Terminal* term);
#endif
