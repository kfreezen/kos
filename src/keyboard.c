#include <keyboard.h>
#include <isr.h>
#include <vfs.h>
#include <kheap.h>
#include <print.h>
#include <fat12.h>
#include <floppy.h>

int curmap=0;
int nummaps=0;
KB_Map* maps;
int active_maps[2];

char* KB_Buffer;

int buf_itr = 0;
int buf_tail = 0;

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
		KB_Buffer[--buf_itr] = 0;
	} else if(buf_itr<KB_BUF_SIZE) {
		KB_Buffer[buf_itr++] = c;
	}
	
	if(buf_itr >= KB_BUF_SIZE) {
		buf_itr = 0;
	}
	
	if(!noecho) PutChar(c);
}	

static void kb_callback(Registers regs) {
	//kprintf("kbint\n");
	
	Byte scan_code = inb(0x60);
	if(wait_press) wait_press=0;
	
	if(!(scan_code&0x80)) {
		switch(scan_code) {
			case LEFT_SHIFT_SCANCODE:
				shift_state = 1;
				break;
				
		}
		
		KPutChar(maps[active_maps[curmap]].map[scan_code+((shift_state)?128:0)]);
	} else {
		
		switch(scan_code&0x7f) {
			case LEFT_SHIFT_SCANCODE:
				shift_state = 0;
				break;
				
		}
	}
	
	outb(0x20, 0x20);
}

void KB_Init(int ne) {
	FAT12_Context* context = FAT12_GetContext(FloppyGetDevice());	
	FAT12_File* file = FAT12_GetFile(context, "kbmaps.dat");
	
	int size = file->data.fileSize;
	if(size&0x1ff) {
		size = size&(~0x1FF);
		size+=0x200;
	}
	
	FileBuffer buffer = FAT12_Read_FB(file, 0, 0);
	UInt8* buf = buffer.buffer;
	
	// Read in kbmaps.
	KB_Map* kbmaps = (KB_Map*) buf;
	int num = file->data.fileSize/sizeof(KB_Map);
	maps = kalloc(sizeof(KB_Map)*num);
	
	nummaps = num;
	if(file->data.fileSize%sizeof(KB_Map)) {
		kprintf("kbmaps.dat invalid.\n");
		return;
	} else {
		int i;
		for(i=0; i<num; i++) {
			maps[i] = kbmaps[i];
		}
	}
	
	active_maps[0] = 0;
	active_maps[1] = 1;
	curmap = 0;
	
	// Basics are now working.
	noecho = ne;
	
	kfree(buf);
	KB_Buffer = kalloc(512);
	memset(KB_Buffer, 0, 512);
	
	inb(0x60);
	registerIntHandler(33, kb_callback);
}

void WaitForKeypress() {
	wait_press=1;
	while(wait_press) {}
}

// Returns a character if there is one in the queue
Byte KB_PollChar() {
	if(buf_itr>buf_tail) {
		Byte tmp = KB_Buffer[buf_tail];
		KB_Buffer[buf_tail++] = 0;
		return tmp;
	} else {
		return 0;
	}
}

void kb_syscall(Registers* regs) {
	switch(regs->ebx) {
		case KB_POLLCH:
			regs->eax = KB_PollChar();
			//kprintf("%x\n", regs.eax);
			
			break;
		
		default:
			regs->eax = 0;
			break;
	}
}
