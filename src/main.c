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

int kmain(UInt32 initial_stack, MultibootHeader* mboot, UInt32 mboot_magic) {
	initial_esp = initial_stack;
	
	CLI_Init();
	ClearScreen();
	//PutString("kOS v0.5.2\n");
	
	if(mboot->mods_count==0) {
		kprintf("ERROR:  No Initrd\n");
		for(;;);
	}
	
	UInt32 initrd_end = *(UInt32*)(mboot->mods_addr+4);
	
	placement_address = (Pointer) initrd_end;
	
	// Set up our new stack here.
	//UInt32 stack = (UInt32) kmalloc_a(8192, TRUE);
	MultibootHeader* mboot_hdr = kmalloc(sizeof(MultibootHeader));
	memcpy(mboot_hdr, mboot, sizeof(MultibootHeader));
	
	//new_start(stack, mboot_hdr);
	kprintf("kOS v0.6.2\n");
	
	GDT_Init();
	IDT_Init();
	ISR_Init();
	asm volatile("sti");
	
	PIT_Init(PIT_MSTIME);
	
	/*
	fs_node_t* krnl_file = open_file(fs_root, "oskrnl.bin");
	
	UInt32 sz = read_fs(krnl_file, 0, 2048, iKernel);
	*/
	
	/*
	jit_compile(iKernel, iKernelOut);
	function func = iKernelOut;
	func();*/
	
	init_kheap();
	Heap* heap = createHeap(HEAP_MIN_SIZE);
	setKernelHeap(heap);
	
	FloppyInit();
	
	FAT12_Context* context = FAT12_GetContext(FloppyGetDevice());
	FAT12_File* file = FAT12_GetFile(context, "HLOWRLD BIN");
	UInt8* buf = (UInt8*) kalloc(512);
	FAT12_Read_LL(file, 0, 0, buf);
	memcpy((void*)0x500, buf, 512);
	
	kprintf("buf=%x\n", buf);
	function f = (function) 0x500;
	f();
	
	return 0;
}

#else
#include <KOSTypes.h>

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
#include <kheap.t.h>
#include <test.h>

extern Pointer placement_address;

UInt32 initial_esp;

MultibootHeader* boot;
UInt32 initrdloc;

Byte iKernel[2048];
Byte iKernelOut[2048];


int kmain(UInt32 initial_stack, MultibootHeader* hdr, UInt32 mboot_magic) {
	CLI_Init();
	ClearScreen();
	
	UInt32 initrd_loc = *((UInt32*)hdr->mods_addr);
	UInt32 initrd_end = *(UInt32*)(hdr->mods_addr+4);
	
	placement_address = (Pointer) initrd_end;
	
	//new_start(stack, mboot_hdr);
	kprintf("kos_test\n");
	
	GDT_Init();
	IDT_Init();
	ISR_Init();
	asm volatile("sti");
	
	PIT_Init(20);
	
	init_kheap();
	Heap* heap = createHeap(HEAP_MIN_SIZE);
	setKernelHeap(heap);
	
	int kheap_test = testKHeap();
	kprintf("%s\n", (kheap_test==SUCCESS) ? "KHEAP_SUCCESS" : "KHEAP_FAILURE");
}
#endif
