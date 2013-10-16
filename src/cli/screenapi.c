#include <screenapi.h>

#include <vga.h>
#include <cli_ui.h>
#include <vfs.h>
#include <dev.h>
#include <print.h>

UInt32 framebuffer = 0x0;

const Int32 DEFAULT_WIDTH = 80;
const Int32 DEFAULT_HEIGHT = 25;

Error error;
Bool isInit = false;

Byte* vidmem = NULL;
int x, y;
int screenWidth, screenHeight;
UInt8 color;

inline void SetError(Error e) { error = e; }

inline Int32 Coor(int x, int y) {
	return (x+(y*screenWidth));
}

void CLI_SetTextMode(int hi_res) {
	set_text_mode(hi_res);
	
	screenWidth = (hi_res) ? 90 : 80;
	screenHeight = (hi_res) ? 60 : 25;
	framebuffer = get_fb_loc();
	vidmem = (Byte*) framebuffer;
}

void SaveTerminal(Terminal* term) {
	term->termX = x;
	term->termY = y;
}

void LoadTerminal(Terminal* term) {
	x = term->termX;
	y = term->termY;
}

void SetColorAttribute(UInt8 c) {
	color = c;
}

inline int TermCoor(Terminal rect, int x, int y) {
	return Coor(rect.r.x, rect.r.y) + ((y*screenWidth)+x);
}

void termScroll(int lines) {
	Terminal rect = GetTerminalArea();
	
	if(IsEnabledCliUI()) {
		Int32 x=0, y=0;
		for(; y<rect.r.height-1; y++) {
			for(x=0; x<rect.r.width; x++) {
				int tc = TermCoor(rect, x, y)<<1;
				int tc1 = TermCoor(rect, x, y+1)<<1;
				vidmem[tc] = vidmem[tc1];
				vidmem[tc+1] = vidmem[tc1+1];
				//x++;
			}
		}
		
		for(x=0; x<rect.r.width; x++) {
			int tc = TermCoor(rect, x, y)<<1;
			vidmem[tc] = 0x20;
			vidmem[tc+1] = color;
			//x++;
		}
	}
	
	x = 0;
	y = rect.r.height-1;
}

void scroll(int lines) {
	// 1.  Move every line up "lines" lines.
	// 2.  Clear the last line.
	Int32 maxCoor = (Coor(screenWidth, screenHeight)-Coor(0,lines))*2;
	
	Int32 i = 0;
	for(; i<maxCoor; i++) {
		vidmem[i] = vidmem[i+(lines*screenWidth*2)];
	}
	
	// Now clear out the rest.
	i = maxCoor;
	maxCoor = Coor(screenWidth,screenHeight)*2;
	for(; i<maxCoor; i++) {
		vidmem[i++] = ' ';
		vidmem[i] = color;
	}
	
	x=0;
	y=screenHeight-lines;
}

// This only sets the "barebones" driver up, call ScreenInit for a device file.
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

/* The escape byte is 0xFF, followed by a command and its data.
	0x00:  CLEARSCREEN
		1 byte:  color
	0x01:  MOVE
		4 bytes:  2 shorts representing x and y
		short[0] = x
		short[1] = y
		sample for Move(x,y)
		{SCREEN_WRITE_ESCAPE, MOVE, x&0xFF, x>>8, y&0xFF, y>>8}

*/

int ScreenWrite(const void* _userBuf, int len, VFS_Node* node) {
	const char* userBuf = (const char*) _userBuf;
	
	// This one is refreshingly simple, we just loop through the userBuf
	int start = 0;
	Bool doEscapes = FALSE;

	if(userBuf[0] == VERIFY_BYTE_0 && userBuf[1] == VERIFY_BYTE_1) {
		start = 2;
		doEscapes = TRUE;
	}

	int i;
	for(i=start; i<len; i++) {
		if(userBuf[i] == SCREEN_WRITE_ESCAPE && doEscapes) {
			switch(userBuf[++i]) {
				case SCREEN_WRITE_ESCAPE: {
					PrintChar(SCREEN_WRITE_ESCAPE);
				} break;

				case CMD_CLEARSCREEN: {
					SetColorAttribute(userBuf[++i]);
					ClearScreen();
				} break;

				case CMD_MOVE: {
					short* coord = (short*) &userBuf[++i];
					Move(coord[0], coord[1]);
					i+=(sizeof(short)<<1) - 1;
				} break;
			}
		} else {
			PrintChar(userBuf[i]);
		}
	}

	return len;
}

int Screen_Init() {
	VFS_Node* screen = GetFileFromPath("/dev/screen");
	if(screen == NULL) {
		DeviceData scrDev;

		memset(&scrDev, 0, sizeof(DeviceData));

		scrDev.name = kalloc(strlen(SCREEN_DEV_NAME)+2);
		strcpy(scrDev.name, SCREEN_DEV_NAME);
		scrDev.write = ScreenWrite;
		scrDev.read = NULL;

		screen = RegisterDevice(&scrDev);
	}

	SetPrintStream(screen);

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
	
	int char_itr;
	Terminal term;
	
	if(IsEnabledCliUI()) {
		term = GetTerminalArea();
		char_itr = TermCoor(term, x, y) << 1;
	} else {
		char_itr = Coor(x,y)<<1;
	}
	
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
		//vidmem[char_itr+1] = color;
		x++;
	}
	
	if(x>=screenWidth || (IsEnabledCliUI() && x>=GetTerminalArea().r.width)) {
		x = 0;
		y++;
	}
	
	if(y>=screenHeight && !IsEnabledCliUI()) {
		scroll(1);
	}
	
	if(IsEnabledCliUI() && y>=GetTerminalArea().r.height) {
		termScroll(1);
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
	if(Coor(_x, _y) < Coor(screenWidth,screenHeight)) {
		x = _x;
		y = _y;
		return 0;
	} else {
		SetError(ERROR_INVALID_COORDINATES);
		return 1;
	}
}

int MoveCursor(int _x, int _y) {
	if(IsEnabledCliUI()) {
		Terminal term = GetTerminalArea();
		_x += term.r.x;
		_y += term.r.y;
	}
	
	if(Coor(_x, _y) < Coor(screenWidth,screenHeight)) {
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
	Int32 maxCoor = Coor(screenWidth,screenHeight);
	if(IsEnabledCliUI()) {
		Rectangle rect = GetTerminalArea().r;
		maxCoor = (rect.width*rect.height)+rect.width;
	}
	
	Move(0,0);
	
	Int32 i;
	for(i=0; i<maxCoor; i++) {
		PrintChar(' ');
	}
}

void SetLineColor(int line, UInt8 color) {
	int itr = Coor(0, line);
	int i;
	int tempWidth = screenWidth<<1;
	for(i=1; i<tempWidth; i+=2) {
		vidmem[itr+i] = color;
	}
}

Error GetError() {
	return error;
}

void ClearError() {
	SetError(ERROR_NONE);
}
