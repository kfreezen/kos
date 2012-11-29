#include <keyboard.h>
#include <isr.h>
#include <vfs.h>
#include <kheap.h>
#include <print.h>

int curmap=0;
int nummaps=0;
KB_Map* maps;
int active_maps[2];

char* buffer;
int buf_itr = 0;

int shift_state=0;
int alt_state=0;
int ctrl_state=0;
int reenable=1;
int wait_press=0;

int noecho=0;
int nonblocking=0;

void SetNoEcho(int ne) {
	noecho = (ne>0) ? 1 : 0;
}

void SetNonblocking(int nb) {
	nonblocking = (nb>0) ? 1 : 0;
}

void KPutChar(char c) {
	if(c=='\b') {
		buffer[--buf_itr] = 0;
	} else if(buf_itr<KB_BUF_SIZE) buffer[buf_itr++] = c;
	if(buf_itr >= KB_BUF_SIZE) {
		buf_itr = 0;
	}
	
	if(!noecho) PutChar(c);
}	

static void kb_callback(Registers regs) {
	Byte scan_code = inb(0x60);
	if(wait_press) wait_press=0;
	
	unsigned code = scan_code;
	PutHex(code);
	PutChar(' ');
	
	if(!(scan_code&0x80)) {
		KPutChar(maps[active_maps[curmap]].map[scan_code]);
	}
	
	outb(0x20, 0x20);
}

void KB_Init(int ne) {
	registerIntHandler(33, kb_callback);
	inb(0x60);
	
	// Basics are now working.
}

void WaitForKeypress() {
	wait_press=1;
	while(wait_press) {}
}
