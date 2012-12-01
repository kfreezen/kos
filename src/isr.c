#include <print.h>
#include <screenapi.h>
#include <keyboard.h>

#include <isr.h>

#define BSOD_ATTR 0x1F

Bool isr_isInit = false;

extern IntHandler int_handlers[256];
Syscall syscalls[512];

static void double_fault_handler(Registers regs) { // int 8
	ClsEx(BSOD_ATTR);
	kprintf("DOUBLE FAULT:  saved eip=%x\n", regs.eip);
	
	for(;;) {}
}

static void dbz_handler(Registers regs) { // int 0
	kprintf("\nDivide-by-zero error:  %x\n", regs.eip);
	
	for(;;) {}
}

static void debug_handler(Registers regs) { // int 1
	kprintf("Debug:  eip=%x, esp=%x, ebp=%x\n", regs.eip, regs.esp, regs.ebp);
}

static void nmi_handler(Registers regs) { // int 2
	kprintf("NMI fault\n");
	
	for(;;) {}
}

static void bound_range_exceeded_handler(Registers regs) {
	kprintf("Bound range exceeded:  EIP=%x\n", regs.eip);
	
	UInt32* esp = (UInt32*) regs.esp;
	
	int i;
	for(i=0; i<10; i++) {
		kprintf("*esp=%x, esp=%x\n", *esp, esp);
		esp--;
	}
	UInt32 cr0;
	asm volatile("movl %%cr0, %0" : "=r" (cr0));
	
	kprintf("DEBUG:  eip=%x, esp=%x, ebp=%x\neax=%x,ebx=%x,ecx=%x,edx=%x,cr0=%x\n", regs.eip, regs.esp, regs.ebp,regs.eax,regs.ebx, regs.ecx, regs.edx,cr0);
	
	//asm volatile("sti");
	for(;;) {}
}

static void breakpoint(Registers regs) {
	kprintf("BREAKPOINT:  eip=%x, esp=%x, ebp=%x\neax=%x,ebx=%x,ecx=%x,edx=%x\n", regs.eip, regs.esp, regs.ebp,regs.eax,regs.ebx, regs.ecx, regs.edx);
}

static void gpf_handler(Registers regs) {
	kprintf("GENERAL_PROTECTION_FAULT:  eip=%x, esp=%x, ebp=%x\n", regs.eip, regs.esp, regs.ebp);
	
	int external = (regs.err_code&1);
	int tbl = (regs.err_code>>1)&3;
	int index = (regs.err_code>>3)&(0x1FFF);
	
	kprintf("GPF_INFO:  errcode=%x, external=%x, tbl=%x, index=%x\n", regs.err_code, external, tbl, index);
	
	for(;;);
}

static void checkpoint_handler(Registers regs) {
	kprintf("CHECKPOINT %x\n", regs.ebx);
}

static void invalid_opcode_handler(Registers regs) {
	kprintf("INVALID_OPCODE:  eip=%x\n", regs.eip);
	
	for(;;);
}

static void page_fault_handler(Registers regs) {
	//ClsEx(BSOD_ATTR);
	
	UInt32 cr2;
	asm volatile( "mov %%cr2, %0" : "=r"(cr2) );
	
	kprintf("PAGE_FAULT:  errCode=%x, address=%x\n", regs.err_code, cr2);
	
	for(;;);
}

static void null_irq7_handler(Registers regs) {

}

static void null_kb_handler(Registers regs) {

}

static void syscall_puts(Registers regs) {
	String s = (String) regs.ebx;
	PutString(s);
}

int ISR_Init() {
	if(isr_isInit) {
		return 1;
	}
	
	memset(int_handlers, 0, sizeof(IntHandler)*256);
	
	registerIntHandler(0, dbz_handler);
	registerIntHandler(1, debug_handler);
	registerIntHandler(2, nmi_handler);
	registerIntHandler(3, breakpoint);
	
	registerIntHandler(5, bound_range_exceeded_handler);
	registerIntHandler(6, invalid_opcode_handler);
	
	registerIntHandler(8, double_fault_handler);
	
	registerIntHandler(13, gpf_handler);
	registerIntHandler(14, page_fault_handler);
	
	registerIntHandler(IRQ1, null_kb_handler);
	registerIntHandler(IRQ7, null_irq7_handler);
	
	registerIntHandler(71, checkpoint_handler);
	
	registerSyscall(SYSCALL_PUTSTRING, syscall_puts);
	isr_isInit = true;
	return 0;
}

void irq_handler(Registers regs) {
	if(regs.int_no >= 40) {
		outb(0xA0, 0x20);
	}
	
	outb(0x20, 0x20);
	
	if(int_handlers[regs.int_no] != 0) {
		IntHandler handler = int_handlers[regs.int_no];
		handler(regs);
	}
}

void isr_handler(Registers regs) {
	
	if(int_handlers[regs.int_no] != 0) {
		IntHandler handler = int_handlers[regs.int_no];
		handler(regs);
	} else {
		kprintf("received int:  %d\n", regs.int_no);
		for(;;) {}
	}
	
}

void syscall_handler(Registers regs) {
	if(syscalls[regs.eax] != 0) {
		Syscall syscall = syscalls[regs.eax];
		syscall(regs);
	} else {
		kprintf("Received invalid syscall request:  %d\n", regs.eax);
	}
}

void registerIntHandler(int interrupt, IntHandler h) {
	int_handlers[interrupt] = h;
}

void registerSyscall(int eax, Syscall s) {
	syscalls[eax] = s;
}

void genericHandler(Registers regs) {
	kprintf("Received interrupt:  %x\n", regs.int_no);
}
