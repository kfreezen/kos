#include <cli_ui.h>
#include <print.h>

#define HORIZ_BORDER_CHAR 205
#define CORNER_TOPLEFT_CHAR 201
#define CORNER_BOTTOMLEFT_CHAR 200
#define CORNER_BOTTOMRIGHT_CHAR 188
#define CORNER_TOPRIGHT_CHAR 189
#define VERTICAL_BORDER_CHAR 186
#define HORIZ_VERTICAL_MEET_CHAR 202

#define CHAR_LEN 7
int BorderChars[CHAR_LEN] = {VERTICAL_BORDER_CHAR, CORNER_BOTTOMRIGHT_CHAR, CORNER_TOPRIGHT_CHAR, CORNER_BOTTOMLEFT_CHAR, CORNER_TOPLEFT_CHAR, HORIZ_BORDER_CHAR, HORIZ_VERTICAL_MEET_CHAR};

Bool isEnabled = false;
Terminal terminalRect[32];
int terminalRectItr;

extern int screenWidth;
extern int screenHeight;
extern UInt8 color;
extern Byte* vidmem;

Bool CheckChars(char c) {
	int i;
	for(i=0; i<CHAR_LEN; i++) {
		if(c==BorderChars[i]) {
			return true;
		}
	}
	
	return false;
}

void EnableCliUI() {
	isEnabled = true;
}

void DisableCliUI() {
	isEnabled = false;
}

Bool IsEnabledCliUI() {
	return isEnabled;
}

void SetTerminalArea(int i, Terminal rect) {
	terminalRect[i] = rect;
}

void SwitchTerminalArea(int i) {
	SaveTerminal(&terminalRect[terminalRectItr]);
	terminalRectItr = i;
	LoadTerminal(&terminalRect[terminalRectItr]);
}

Terminal GetTerminalArea() {
	return terminalRect[terminalRectItr];
}

void SetTitle(const char* str) {
	SetLineColor(0, ~color);
	
	int strLen = strlen(str);
	
	int startX = (screenWidth/2)-(strLen/2);
	int i = 0;
	for(; i<strLen; i++) {
		vidmem[(startX+i)<<1] = str[i];
	}
}

void CreateBorder(Rectangle rect) {
	DisableCliUI();
	Bool drawTop, /*drawBottom,*/ drawLeft /*, drawRight*/;
	//int lineTop, lineBottom;
	
	if(rect.x == 0) {
		drawLeft = false;
	} else {
		drawLeft = true;
	}
	
	if(rect.y <= 1) {
		drawTop = false;
	} else {
		drawTop = true;
	}
	
	if(rect.x+rect.width<screenWidth) {
		//drawRight = true;
	} else {
		//drawRight = false;
	}
	
	if(rect.y+rect.height<screenHeight) {
		//drawBottom = true;
	} else {
		//drawBottom = false;
	}
	
	Terminal term;
	SaveTerminal(&term);
	
	if(drawTop) {
		int itr = ((rect.y-1)*rect.width)+rect.x;
		
		int i;
		for(i=0; i<rect.width; i++) {
			if(CheckChars(vidmem[(itr+i)<<1])) {
				// Change it how it needs to be.
			} else {
				vidmem[(itr+i)<<1] = HORIZ_BORDER_CHAR;
			}
		}
	}
	
	if(drawLeft) {
		int itr = ((rect.y)*rect.width)+rect.x;
		int skip = screenWidth;
		int i;
		for(i=0; i<rect.height; i++) {
			if(CheckChars(vidmem[itr<<1])==true) {
				kprintf("HI");
				if(vidmem[(itr+skip)<<1] == HORIZ_BORDER_CHAR) {
					vidmem[itr<<1] = HORIZ_VERTICAL_MEET_CHAR;
				}
				// Change it accordingly.
			} else {
				vidmem[(itr)<<1] = VERTICAL_BORDER_CHAR;
			}
			
			itr+=skip;
		}
	}
	LoadTerminal(&term);
	
	EnableCliUI();
}
