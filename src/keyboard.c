#include <keyboard.h>
#include <isr.h>
#include <vfs.h>
#include <kheap.h>
#include <print.h>
#include <fat12.h>
#include <floppy.h>
#include <dev.h>

int curmap=0;
int nummaps=0;
KB_Map* maps;
int active_maps[2];

char* KB_Buffer;

int kb_buf_itr = 0;
int kb_buf_tail = 0;

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

#define KB_READ_SIZE 32

int GetAvailableBytes() {
	int tmp = kb_buf_itr;

	if(tmp < kb_buf_tail) {
		tmp += KB_BUF_SIZE;
	}

	return tmp - kb_buf_tail;
}

int kb_read(char* userBuf, int len, VFS_Node* node) {
	int toRead;

	if(node->options.flags&O_NONBLOCKING) {
		toRead = (GetAvailableBytes() < len) ? GetAvailableBytes() : len;
	} else {
		toRead = len;
	}

	// We want to split the reads into 32-char or smaller chunks.
	int reads = toRead / KB_READ_SIZE;
	int last_read_len = toRead % KB_READ_SIZE;

	// We may need to split the read between the last and the first of the buffer
	int userBufItr = 0;

	while(reads) {
		int readBeginning = 0;
		int readEnd = KB_READ_SIZE;

		while(GetAvailableBytes() < KB_READ_SIZE) {}

		if(kb_buf_tail+readEnd >= KB_BUF_SIZE) {
			readEnd = KB_BUF_SIZE - kb_buf_tail; // ?
			readBeginning = KB_READ_SIZE - readEnd;
		}

		memcpy(&userBuf[userBufItr], &KB_Buffer[kb_buf_tail], readEnd);
		userBufItr += readEnd;

		kb_buf_tail += readEnd;

		if(kb_buf_tail >= KB_BUF_SIZE) {
			kb_buf_tail = 0;
			memcpy(&userBuf[userBufItr], &KB_Buffer[kb_buf_tail], readBeginning);
			userBufItr += readBeginning;
		}

		reads --;
	}

	while(GetAvailableBytes() < last_read_len) {

	}

	memcpy(&userBuf[userBufItr], &KB_Buffer[kb_buf_tail], last_read_len);
	userBufItr += last_read_len;

	return userBufItr+1;
}

void KPutChar(char c) {
	/*if(c=='\b') {
		KB_Buffer[--kb_buf_itr] = 0;
	} else*/ if(kb_buf_itr<KB_BUF_SIZE) {
		KB_Buffer[kb_buf_itr++] = c;
	}
	
	if(kb_buf_itr >= KB_BUF_SIZE) {
		kb_buf_itr = 0;
	}
	
	wait_press = 0;
	
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
				
			default:
				KPutChar(maps[active_maps[curmap]].map[scan_code+((shift_state)?128:0)]);
				break;
		}
	} else {
		
		switch(scan_code&0x7f) {
			case LEFT_SHIFT_SCANCODE:
				shift_state = 0;
				break;
				
		}
	}
	
	outb(0x20, 0x20);
}

#define KEYBOARD_DEV_NAME "keyboard"

void KB_Init(int ne) {
	wait_press = 1;
	
	
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
			memcpy(&maps[i], &kbmaps[i], sizeof(KB_Map));
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

	// Set up our device file here.
	DeviceData kb_device;
	kb_device.name = kalloc(strlen(KEYBOARD_DEV_NAME)+1);
	strcpy(kb_device.name, KEYBOARD_DEV_NAME);
	kb_device.write = NULL;
	kb_device.read = kb_read;

	RegisterDevice(&kb_device);
}

/*
void WaitForKeypress() {
	while(wait_press) {}
	wait_press=1;
}

// Returns a character if there is one in the queue
Byte KB_PollChar() {
	if(kb_buf_itr>kb_buf_tail) {
		Byte tmp = KB_Buffer[kb_buf_tail];
		KB_Buffer[kb_buf_tail++] = 0;
		return tmp;
	} else {
		return 0;
	}
}

// Returns a character.  Blocks if there is none in the queue.
Byte KB_GetChar() {
	if(kb_buf_itr>kb_buf_tail) {
		Byte tmp = KB_Buffer[kb_buf_tail];
		KB_Buffer[kb_buf_tail++] = 0;
		return tmp;
	} else {
		WaitForKeypress();
		return KB_GetChar();
	}
}*/

void kb_syscall(Registers* regs) {
	asm volatile("sti");
	
	/*
	switch(regs->ebx) {
		case KB_POLLCH:
			regs->eax = KB_PollChar();
			//kprintf("%x\n", regs.eax);
			
			break;
		
		case KB_GETCH:
			regs->eax = KB_GetChar();
			break;
			
		default:
			regs->eax = 0;
			break;
	}*/
}
