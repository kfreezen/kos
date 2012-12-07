#include <screenapi.h>

#include <vga.h>

UInt32 framebuffer = 0x0;

const Int32 DEFAULT_WIDTH = 80;
const Int32 DEFAULT_HEIGHT = 25;

Error error;
Bool isInit = false;

Byte* vidmem = NULL;
int x, y;
int w, h;
UInt8 color;

inline void SetError(Error e) { error = e; }

inline Int32 Coor(int x, int y) {
	return (x+(y*w));
}

void CLI_SetTextMode(int hi_res) {
	set_text_mode(hi_res);
	
	w = (hi_res) ? 90 : 80;
	h = (hi_res) ? 60 : 25;
	framebuffer = get_fb_loc();
	vidmem = framebuffer;
}

void SetColorAttribute(UInt8 c) {
	color = c;
}

void scroll(int lines) {
	// 1.  Move every line up "lines" lines.
	// 2.  Clear the last line.
	Int32 maxCoor = (Coor(w, h)-Coor(0,lines))*2;
	
	Int32 i = 0;
	for(; i<maxCoor; i++) {
		vidmem[i] = vidmem[i+(lines*w*2)];
	}
	
	// Now clear out the rest.
	i = maxCoor;
	maxCoor = Coor(w,h)*2;
	for(; i<maxCoor; i++) {
		vidmem[i++] = ' ';
		vidmem[i] = color;
	}
	
	x=0;
	y=h-lines;
}

int CLI_Init() { // TODO:  Figure out a way in which to find the width and height
	if(isInit == true) {
		SetError(ERROR_ALREADY_INITIALIZED);
		return 1;
	} else {
		color = DEFAULT_COLOR_ATTRIBUTE;
		vidmem = (Byte*) framebuffer;
		isInit = true;
		
		ClearError();
		CLI_SetTextMode(HI_RES);
	}
	
	return 0;
}

int PrintChar(Char c) {
	if(isInit==false) {
		SetError(ERROR_NOT_INITIALIZED);
		return 1;
	}
	
	if(c == NULL) {
		return 2;
	}
	
	int char_itr = Coor(x,y)*2;

	if(c == '\n') {
		y++;
		x = 0;
	} else if(c == '\r') {
		x = 0;
	} else if(c == '\b' && x) {
		x--;
		PrintChar(' ');
		x--;
	} else if(c == '\t') {
		x = (x&0xFFFFFFF8) + 8;
	} else {
		vidmem[char_itr] = c;
		vidmem[char_itr+1] = color;
		x++;
	}
	
	if(x>=w) {
		x = 0;
		y++;
	}
	
	if(y>=h) {
		scroll(1);
	}
	
	return 0;
}

int PrintString(String s) {
	int error = 0;
	
	while(error != 2) {
		error = PrintChar(*s++);
		if(error == 1) {
			return 1;
		}
	}
	
	return 0;
}

int Move(int _x, int _y) {
	if(Coor(_x, _y) < Coor(w,h)) {
		x = _x;
		y = _y;
		return 0;
	} else {
		SetError(ERROR_INVALID_COORDINATES);
		return 1;
	}
}

int MoveCursor(int _x, int _y) {
	if(Coor(_x, _y) < Coor(w,h)) {
		UInt16 cursorLocation = Coor(_x, _y);
		outb(0x3D4, 14);
		outb(0x3d5, cursorLocation >> 8);
		outb(0x3d4, 15);
		outb(0x3d5, cursorLocation);
		return 0;
	} else {
		return -1;
		SetError(ERROR_INVALID_COORDINATES);
	}
	
}

int MoveCursorToCurrentCoordinates() {
	return MoveCursor(x, y);
}

void ClearScreen() {
	Int32 maxCoor = Coor(w,h)*2;
	
	Int32 i;
	for(i=0; i<maxCoor; i++) {
		vidmem[i++] = ' ';
		vidmem[i] = color;
	}
}

Error GetError() {
	return error;
}

void ClearError() {
	SetError(ERROR_NONE);
}
