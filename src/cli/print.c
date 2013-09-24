#include <print.h>

#include <screenapi.h>
#include <KOSArgs.h>
#include <vfs.h>

extern void ClearScreen();

VFS_Node* printStream = NULL;

void SetPrintStream(VFS_Node* stream) {
	printStream = stream;
}

int PutCharEx(Char c, Bool no_move_csr) {
	int ret;

	if(printStream) {
		// The proper success return is 0 for this PutCharEx
		ret = !WriteFile(&c, 1, printStream);
	} else {
		ret = PrintChar(c);
	}

	if(no_move_csr == false) {
		ret |= MoveCursorToCurrentCoordinates();
	}
	
	return ret;
}

int PutChar(Char c) {
	return PutCharEx(c, false);
}

int PutStringEx(String s, Bool no_move_csr) {
	int ret;
	if(printStream) {
		ret = !WriteFile(s, strlen(s), printStream);
	} else {
		ret = PrintString(s);
	}
	
	ret |= (no_move_csr == false) ? MoveCursorToCurrentCoordinates() : 0;
	return ret;
}

int PutString(String s) {
	return PutStringEx(s, false);
}

void _PrintString(String s) {
	if(printStream) {
		WriteFile(s, strlen(s), printStream);
	} else {
		PrintString(s);
	}
}

void _PrintChar(Char c) {
	if(printStream) {
		WriteFile(&c, 1, printStream);
	} else {
		PrintChar(c);
	}
}

void PutHexEx(UInt32 num, Bool noZeroes, Bool hexIdent) {
	Int32 tmp;
	
	if(hexIdent) {
		_PrintString("0x");
	}
	
	int i;
	for(i = 28; i>0; i-=4) {
		tmp = (num >> i) & 0xF;
		if(tmp == 0 && noZeroes != false) {
			continue;
		}
		
		if(tmp >= 0xA) {
			noZeroes = false;
			_PrintChar((Char) tmp-0xA+'a');
		} else {
			noZeroes = false;
			_PrintChar((Char) tmp+'0');
		}
	}
	
	tmp = num & 0xF;
	if(tmp >= 0xA) {
		_PrintChar(tmp-0xA+'a');
	} else {
		_PrintChar(tmp+'0');
	}
	
	MoveCursorToCurrentCoordinates();
}

void PutHex(UInt32 num) {
	PutHexEx(num, true, true);
}

void PutDec(UInt32 num) {
	if(num == 0) {
		PutChar('0');
		return;
	}
	
	Int32 acc = num;
	Char c[32];
	int i = 0;
	while(acc>0) {
		c[i] = '0' + acc%10;
		acc /= 10;
		i++;
	}
	c[i] = 0;
	
	Char c2[32];
	c2[i--] = 0;
	int j = 0;
	while(i >= 0) {
		c2[i--] = c[j++];
	}
	_PrintString(c2);
	MoveCursorToCurrentCoordinates();
}

void PutFloat(float n) {
	UInt32* pn = (UInt32*) &n;
	UInt32 sign = (*pn&(1<<31))>>31;
	UInt32 exponent = (*pn&(0xFF<<23)>>23);
	UInt32 fraction = (*pn&0x4FFFFF);
	
	kprintf("\n%d %d %d\n", sign, exponent, fraction);
}

void kprintf(const char* fmt, ...) {
	va_list va;
	va_start(va, fmt);
	
	int i;
	for(i=0; fmt[i]!='\0';) {
		switch(fmt[i]) {
			case '%': 
				{
					
					switch(fmt[++i]) {
						case '%':
							_PrintChar('%');
							i++;
							break;
							
						case 'c': {
							char c = va_arg(va, char);
							_PrintChar(c);
							i++;
							break;
							
						}
						
						case 's': {
							char* s = va_arg(va, char*);
							_PrintString(s);
							i++;
							break;
						}
							
						case 'd':
						case 'u': {
							UInt32 u = va_arg(va, UInt32);
							PutDec(u);
							i++;
							break;
							
						}
						
						case 'x': {
							UInt32 x = va_arg(va, UInt32);
							PutHex(x);
							i++;
							break;
						}
						
						case 'f': {
							float f = va_arg(va, float);
							PutFloat(f);
							i++;
							break;
						}
					}
					
				}
				break;
				
			default:
				_PrintChar(fmt[i++]);
				break;
				
		}
		
	}
	va_end(va);
	MoveCursorToCurrentCoordinates();
}

void ClsEx(UInt8 color) {
	if(printStream) {
		char command[11] = {VERIFY_BYTE_0, VERIFY_BYTE_1, SCREEN_WRITE_ESCAPE,
			CMD_CLEARSCREEN, color,
			SCREEN_WRITE_ESCAPE, CMD_MOVE, 0, 0, 0, 0
		};
		WriteFile(command, 9, printStream);
	} else {
		SetColorAttribute(color);
		ClearScreen();
		Move(0,0);
	}
}

void Cls() {
	ClsEx(DEFAULT_COLOR_ATTRIBUTE);
}

	
