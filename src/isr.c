#include <print.h>
#include <screenapi.h>
#include <keyboard.h>
#include <paging.h>
#include <kheap.h>
#include <tasking.h>
#include <debugger.h>
#include <debugdef.h>

#include <isr.h>

#define BSOD_ATTR 0x1F

Bool isr_isInit = false;
Bool kb_interrupt = false;

extern IntHandler int_handlers[256];
Syscall syscalls[512];

extern void kb_syscall(Registers* regs);

extern Task* current_task;
extern Task* ready_queue;

// TODO:  Create some kind of queue for the kernel to get rid of the left over stuff from the task in its spare time, so as to prevent lagging when a task is closed.

extern void TaskSwitch();

static void kb_handler_for_exceptions(Registers regs) {
	kb_interrupt = true;
}

void StackTrace(Registers* regs) {
	kprintf("eax=%x, ebx=%x, ecx=%x, edx=%x\nesp=%x, ebp=%x", regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esp, regs->ebp);
}

// This will only be done if a kernel mode application has caused the fault.
void SetupIRQHandlersForExceptions() {
	int irq = IRQ0;
	
	// First we set them all to NULL so we don't mess stuff up when interrupts are re-enabled.
	for(; irq<IRQ15+1; irq++) {
		int_handlers[irq] = NULL;
	}
	
	int_handlers[IRQ1] = kb_handler_for_exceptions;
}

static void syscall_exit(Registers* regs) {
	Task* toExit = current_task;
	PageDirectory* dir = toExit->dir;
	
	if(toExit->id==1) {
		// This is the kernel task.
		return;
	}
	
	// take toExit out of the queue.
	Task* tmp = ready_queue;
	while(tmp->next != toExit);
	
	// tmp->next now equals toExit.
	
	tmp->next = toExit->next;
	
	FreeAllUserPages(toExit->dir);
	
	if(isInKernelHeap(dir)) {
		kfree(dir);
	}

	TaskSwitch();
}


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
	kprintf("INVALID_OPCODE:  eip=%x, esp=%x\npid=%d\n", regs.eip, regs.useresp, GetPID());
	
	if(GetPID()==1) { // KERNEL
		kprintf("kernel panic!\n");
		for(;;);
	} else {
		syscall_exit(&regs);
	}
}

static void page_fault_handler(Registers regs) {
	//ClsEx(BSOD_ATTR);
	
	UInt32 cr2;
	asm volatile( "mov %%cr2, %0" : "=r"(cr2) );
	
	kprintf("PAGE_FAULT:  eip=%x, errCode=%x, address=%x\n", regs.eip, regs.err_code, cr2);
	
	/*SetupIRQHandlersForExceptions();
	kb_interrupt = false;
	asm volatile("sti");
	
	while(!kb_interrupt);*/
	
	StackTrace(&regs);
	
	asm volatile("cli");
	
	for(;;) {
		asm volatile("hlt");
	}
}

static void null_irq7_handler(Registers regs) {

}

static void null_kb_handler(Registers regs) {

}

static void syscall_console(Registers* regs) {
	switch(regs->ebx) {
		case CONSOLE_PUTS:
			PutString((char*)regs->ecx);
			break;
		case CONSOLE_PUTCH:
			PutChar(regs->ecx);
			break;
		case CONSOLE_PUTHEX:
			PutHex(regs->ecx);
			break;
		case CONSOLE_PUTDEC:
			PutDec(regs->ecx);
			break;
	}
}

static void syscall_task(Registers* regs) {
	regs->eax = GetPID();
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
	
	registerSyscall(SYSCALL_EXIT, syscall_exit);
	registerSyscall(SYSCALL_CONSOLE, syscall_console);
	registerSyscall(SYSCALL_KB, kb_syscall);
	registerSyscall(SYSCALL_TASK, syscall_task);
	
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
	#ifdef SYSCALL_DEBUG
	kprintf("syscall! ");
	#endif
	
	if(syscalls[regs.eax] != 0) {
		Syscall syscall = syscalls[regs.eax];
		syscall(&regs);
	} else {
		kprintf("Received invalid syscall request:  %d\n", regs.eax);
	}
}

void registerIntHandler(int interrupt, IntHandler h) {
	int_handlers[interrupt] = h;
}

void registerSyscall(int eax, Syscall s) {
	if(!syscalls[eax]) {
		syscalls[eax] = s;
	}
}

void genericHandler(Registers regs) {
	kprintf("Received interrupt:  %x\n", regs.int_no);
}
