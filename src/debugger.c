#include <debugger.h>
#include <print.h>
#include <keyboard.h>
#include <kheap.h>
#include <cli_ui.h>
#include <tasking.h>
#include <common.h>
#include <disasm.h>

char debug_gets_buffer[512];
int dbg_buf_itr = 0;

/*
char* Debug_GetString() {
	for(;;) {
		char tmp = KB_GetChar();
		
		if(tmp == '\b' && dbg_buf_itr) {
			debug_gets_buffer[--dbg_buf_itr] = 0;
			PutChar('\b');
			continue;
		} else if(tmp == '\b') {
			continue;
		}
		
		debug_gets_buffer[dbg_buf_itr++] = tmp;
		PutChar(tmp);
		
		if(debug_gets_buffer[dbg_buf_itr-1] == '\n') {
			debug_gets_buffer[dbg_buf_itr-1] = 0;
			char* str = kalloc(strlen(debug_gets_buffer)+1);
			strcpy(str, debug_gets_buffer);
			dbg_buf_itr = 0;
			return str;
		}
	}
}

extern int screenWidth, screenHeight;

char* arg_array[32];

extern void DumpStack();

#define MEMORY_LOC 200

#define BYTE 1
#define SHORT 2
#define INT 4
#define INSTRUCTION 8

void ExamineMemory(unsigned addr, int size, int instr) {
	int i;
	int mod = 28/size;
	Rectangle rect = GetTerminalArea().r;
	
	if(size==INSTRUCTION) {
		Disassemble(addr, instr);
		return;
	}
	
	for(i=0; i<MEMORY_LOC; i++) {
		if(size==BYTE) {
			PutHexEx(((Byte*)addr)[i], true, false);
			mod = rect.width/3;
		} else if(size==SHORT) {
			PutHexEx(((UShort*)addr)[i], true, false);
			mod = rect.width/5;
		} else {
			mod = rect.width/9;
			PutHexEx(((UInt32*)addr)[i], false, false);
		}
		
		PutChar((!(i%mod) && i!=0) ? '\n' : ' ');
	}
}

void StartDebugger() {
	Cls();
	DisableTasking();
	SetNoEcho(1);
	
	SetTerminalArea(DEBUG_CONSOLE, (Terminal){{0,screenHeight-20, screenWidth, 20}, 0, 0});
	SetTerminalArea(DUMP_CONSOLE, (Terminal){{screenWidth-24, 1, 24, screenHeight-22}, 0, 0});
	SetTerminalArea(EXAMINE_CONSOLE, (Terminal){{0,1, screenWidth-24, screenHeight-22}, 0, 0});
	SwitchTerminalArea(DEBUG_CONSOLE);
	
	EnableCliUI();
	
	SwitchTerminalArea(DEBUG_CONSOLE);
	
	SetTitle("Kernel Debugger");
	
	asm volatile("sti");
	
	for(;;) {
		PutString("debug> ");
		char* str = Debug_GetString();
		
		int i = 0;
		int j = 0;
		int k = 0;
		for(i=0;;i++) {
			if(str[i]==' ' || str[i]=='\0') {
				if(str[i]=='\0') {
					arg_array[k++] = &str[j];
					break;
				}
				str[i] = 0;
				arg_array[k++] = &str[j];
				
				j = ++i;
			}
		}
		
		if(!strcmp(arg_array[0], "dumpstack")) {
			SwitchTerminalArea(DUMP_CONSOLE);
			DumpStack();
			SwitchTerminalArea(DEBUG_CONSOLE);
		} else if(arg_array[0][0]=='x') {
			unsigned long address = strtoul(arg_array[1], NULL, 16);
			int size;
			int instr = 0;
			
			if(arg_array[0][1]=='b') {
				size = BYTE;
			} else if(arg_array[0][1]=='s') {
				size = SHORT;
			} else if(arg_array[0][1]=='l') {
				size = INT;
			} else if(arg_array[0][1]=='i') {
				size = INSTRUCTION;
				kprintf("%x\n", k);
				instr = (k>=3) ? strtoul(arg_array[2], NULL, 0) : 24;
			}
			
			SwitchTerminalArea(EXAMINE_CONSOLE);
			Cls();
			ExamineMemory(address, size, instr);
			SwitchTerminalArea(DEBUG_CONSOLE);
			
		}
		
		kfree(str);
	}
	
	asm volatile("cli");
}*/