//#define TEST

#ifndef TEST
#include <KOSTypes.h>

#include <screenapi.h>
#include <print.h>
#include <gdt.h>
#include <idt.h>
#include <paging.h>
#include <multiboot.h>
#include <kheap.h>
#include <keyboard.h>
#include <vfs.h>
#include <isr.h>
#include <rtc.h>
#include <jit.h>
#include <pit.h>
#include <vfs.h>
#include <floppy.h>
#include <fat12.h>
#include <paging.h>
#include <pci.h>
#include <tasking.h>
#include <elf.h>
#include <graphics.h>
#include <cli_ui.h>
#include <error.h>
#include <ata.h>
#include <dev.h>
#include <drivers.h>
#include <common.h>

#define PIT_MSTIME 20

extern Pointer placement_address;

UInt32 initial_esp;

MultibootHeader* boot;
UInt32 initrdloc;

extern UInt64 _rdtsc();
extern void new_start(UInt32 stack, MultibootHeader* mboot);

void jit_compile(void*, void*);

Byte iKernel[2048];
Byte iKernelOut[2048];

typedef void (*function)();

void thread1();
void thread2();
void thread(char c);

//#define INIT_DEBUG

int kmain(UInt32 initial_stack, MultibootHeader* mboot, UInt32 mboot_magic) {
	initial_esp = initial_stack;
	
	CLI_Init();
	Cls();
	
	UInt32 initrd_end = *(UInt32*)(mboot->mods_addr+4);
	
	placement_address = (Pointer) initrd_end;
	
	// Set up our new stack here.
	//UInt32 stack = (UInt32) kmalloc_a(8192, TRUE);
	MultibootHeader* mboot_hdr = mboot; //kmalloc(sizeof(MultibootHeader));
	//memcpy(mboot_hdr, mboot, sizeof(MultibootHeader));
	
	//new_start(stack, mboot_hdr);
	GDT_Init();
	IDT_Init();
	ISR_Init();
	asm volatile("sti");
	
	PIT_Init(PIT_MSTIME);

	init_kheap();
	InitPaging((mboot_hdr->mem_lower+mboot_hdr->mem_upper)&~3);
	InitKernelHeap();

	VFS_Init();
	DevFS_Init();
	
	DriversInit();

	Screen_Init();

	//FloppyInit();
	
	kprintf("Scanning PCI Devices... ");
	checkAllBuses();
	kprintf("[ok]\n");
	
	DumpPCIDeviceData();

	/*kprintf("Keyboard Init... ");
	KB_Init(0);
	kprintf("[ok]\n");*/

	FAT12_Init(FAT12_GetContext(FloppyGetDevice()), "/", "sys");
	
	InitTasking();

	KernelSymbolsLoad();

	//Cls();

	kprintf("kOS v0.6.12\n");

	/*
	//kprintf("kprintf symbol = %x\n", getKernelSymbol("kprintf"));
	File* initScript = GetFileFromPath("/sys/init.script");
	FileSeek(0, initScript); // Due to these being global objects, we have to do such ugly things as this.

	#ifdef INIT_DEBUG
	kprintf("initScript=%x\n", initScript);
	#endif

	char* lineBuf = kalloc(256);
	int doBreak = 0;
	while(!doBreak) {
		if(fgetline(initScript, lineBuf, 256, '\n')==-1) {
			if(strlen(lineBuf) > 0) {
				doBreak = 1;
			} else {
				break; // We've processed everything that needs to be processed.
			}
		}

		// Now parse it.
		char* tok = strtok(lineBuf, " ");
		kprintf("%s, %x\n", tok,tok);
		if(!strcmp(tok, "load_driver")) {
			#ifdef INIT_DEBUG
			kprintf("load_driver ");
			#endif

			tok = strtok(NULL, " ");

			// Load the driver specified.
			File* drv = GetFileFromPath(tok);
			if(drv != NULL) {
				int drvLength = FileSeek(SEEK_EOF, drv);
				FileSeek(0, drv);
				void* drvBuf = kalloc(drvLength);

				#ifdef INIT_DEBUG
				kprintf("%s\n", GetNodeFromFile(drv)->name);
				#endif

				ReadFile(drvBuf, drvLength, drv);
				ELF* elf = LoadKernelDriver(drvBuf);

				#ifdef INIT_DEBUG
				kprintf("elf->start=%x\n", elf->start);
				#endif

				if(elf->error == 0) {
					void (*driverInit)() = (void (*)()) elf->start;
					driverInit();
				}

				kfree(drvBuf);
				drvBuf = NULL;

				CloseFile(drv);
			}
		}
	}

	CloseFile(initScript);
	kfree(lineBuf);

	/*File* file = GetFileFromPath("/sys/hw_module");
	int fileLength = FileSeek(SEEK_EOF, file);
	FileSeek(0, file);

	void* exe = kalloc(fileLength);
	ReadFile(exe, fileLength, file);
	ELF* elf = LoadKernelDriver(exe);
	void (*doItPtr)() = elf->start;
	doItPtr();*/

	kprintf("Kernel init done...\n");

	while(1) {
		TaskSleep(1000);
		kprintf("thread wake\n");
	}

	return 0;
}

#endif
