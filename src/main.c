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

typedef struct EXE_Header {
	UInt32 magic;
	UInt32 loc;
	UInt32 loc_start;
	UInt32 loc_data;
} EXE_Header;

void thread1();
void thread2();
void thread(char c);

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
	
	Screen_Init();

	FloppyInit();
	
	kprintf("Scanning PCI Devices... ");
	checkAllBuses();
	kprintf("[ok]\n");
	
	DumpPCIDeviceData();
	
	ATA_Init();
	//ATA_EnumerateDevices();

	kprintf("Keyboard Init... ");
	KB_Init(0);
	kprintf("[ok]\n");

	FAT12_Init(FAT12_GetContext(FloppyGetDevice()), "/", "floppy");
	VFS_Node* floppy = GetNodeFromPath("/floppy");
	kprintf("floppy=%x\n", floppy);
	LoadDirectory(floppy);
	
	kprintf("Kernel init done...\n");

	Cls();
	kprintf("kOS v0.6.10\n");
	
	return 0;
}

void thread1() {
	thread('b');
}

void thread2() {
	thread('c');
}

void thread(char c) {
	int j;
	for(j=0;;j++) {
		if(!(j%8000)) {
			kprintf("%c", c);
		}
	}
}

#endif
