#include <keyboard.h>
#include <isr.h>
#include <vfs.h>
#include <kheap.h>
#include <print.h>
#include <fat12.h>
#include <floppy.h>
#include <dev.h>

//#define KEYBOARD_DEBUG

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

int kb_read(void* _userBuf, int len, File* file) {
	#ifdef KEYBOARD_DEBUG
	kprintf("kb_read(%x, %x, %x)\n", userBuf, len, file);
	#endif
	char* userBuf = (char*) _userBuf;
	
	int toRead;

	// TODO:  Move the options into the file struct.
	if(GetNodeFromFile(file)->options.flags&O_NONBLOCKING) {
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

		#ifdef KEYBOARD_DEBUG
		kprintf("readEnd = %d, readBeginning = %d\n", readEnd, readBeginning);
		#endif

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

	#ifdef KEYBOARD_DEBUG
	kprintf("last_read_len=%d", last_read_len);
	#endif 

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

int kb_stall = 0;
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
				kb_stall = !kb_stall;
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

int KB_Init() {
	kprintf("ss %x, %x\n", shift_state, &shift_state);

	File* file = GetFileFromPath("/sys/kbmaps.dat");

	if(!file) {
		kprintf("kbmaps.dat not found.\n");
		return -1;
	}

	int length = FileSeek(SEEK_EOF, file);
	FileSeek(0, file);

	UInt8* buf = kalloc(length);
	ReadFile(buf, length, file);
	KB_Map* kbmaps = (KB_Map*) buf;
	
	int num = length / sizeof(KB_Map);

	#ifdef KEYBOARD_DEBUG
	kprintf("num = %x, fileSize=%x\n", num, file->locationData->fileSize);
	#endif

	maps = kalloc(sizeof(KB_Map)*num);

	nummaps = num;
	if(length%sizeof(KB_Map)) {
		kprintf("kbmaps.dat invalid\n");
		return -1;
	} else {
		int i;
		for(i=0; i<num; i++) {
			memcpy(&maps[i], &kbmaps[i], sizeof(KB_Map));
		}
	}

	active_maps[0] = 0;
	active_maps[1] = 1;
	curmap = 0;

	noecho = 0;

	kfree(buf);

	KB_Buffer = kalloc(512);
	memset(KB_Buffer, 0, 512);
	
	inb(0x60);
	registerIntHandler(33, kb_callback);

	// Set up our device file here.
	DeviceData kb_device;
	memset(&kb_device, 0, sizeof(DeviceData));

	kb_device.name = kalloc(strlen(KEYBOARD_DEV_NAME)+1);
	strcpy(kb_device.name, KEYBOARD_DEV_NAME);
	kb_device.write = NULL;
	kb_device.read = kb_read;

	#ifdef KEYBOARD_DEBUG
	kprintf("kb.read=%x\n", kb_read);
	#endif

	RegisterDevice(&kb_device);

	kfree(kb_device.name);
	return 0;
}

void _start() {
	kprintf("Keyboard init ");
	if(KB_Init() == 0) {
		kprintf("[ok]\n");
	} else {
		kprintf("[fail]\n");
	}
}