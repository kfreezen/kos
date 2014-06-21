#include <print.h>
#include <screenapi.h>
#include <keyboard.h>
#include <paging.h>
#include <kheap.h>
#include <tasking.h>
#include <debugger.h>
#include <debugdef.h>
#include <driver_interface.h>

#include <isr.h>
#include <vfs.h>

#define BSOD_ATTR 0x1F

Bool isr_isInit = false;
Bool kb_interrupt = false;

extern IntHandler int_handlers[256];
Syscall syscalls[512];

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

extern Task* current_task;

static void syscall_open(Registers* regs) {
	// ebx = file to open.
	// ecx = reserved
	// edx = reserved
	
	// Return in eax.

	// Make sure our unscrupulous user doesn't do a bad thing and try to read from NULL memory.
	if(regs->ebx == NULL) {
		regs->eax = INVALID_FILEDESCRIPTOR;
		return;
	}

	File* f = GetFileFromPath((char*) regs->ebx);

	if(f == NULL) {
		// Failed to open file for whatever reason.
		// This is where it'd be handy to have an errno. TODO
		regs->eax = INVALID_FILEDESCRIPTOR;
		return;
	}

	int fd = ALAdd(current_task->processInfo->files, (void*) f);

	regs->eax = fd;
}

static void syscall_write(Registers* regs) {
	// ebx = file descriptor.
	// ecx = buffer to write to file.
	// edx = length of buffer to write.
	
	// Returns status in eax.

	// These should probably be register vars.
	int fd = regs->ebx;
	void* buf = (void*) regs->ecx;
	int len = regs->edx;

	if(fd < 0) {
		regs->eax = -1;
		return;
	}

	if(buf == NULL) {
		regs->eax = -1;
		return;
	}

	if(len < 0) {
		regs->eax = -1;
		return;
	}

	File* f = (File*) ALGetPtr(current_task->processInfo->files, regs->ebx);
	regs->eax = WriteFile(buf, len, f);
}

static void syscall_task(Registers* regs) {
	regs->eax = GetPID();
}

#include <driver_interface.h>

void driver_interface_handler(Registers regs) {
	switch(regs.eax) {
		case INSTALL_IRQ: {
			registerIntHandler(IRQ(regs.ebx&0xF), (IntHandler)regs.ecx);
		} break;

		case MEMORY_SERVICES: {
			switch(regs.ebx) {
				case ALLOC: {
					regs.eax = (unsigned) kalloc(regs.ecx);
				} break;

				case FREE: {
					kfree((Pointer)regs.ecx);
				} break;
			}
		} break;

		case CONSOLE_SERVICES: {
			switch(regs.ebx) {
				case KPUTS: {
					PutString((String)regs.ecx);
				} break;
			}
		}

		case VFS_SERVICES: {
			switch(regs.ebx) {
				case VFS_GETNODE: {
					regs.eax = (unsigned) GetFileFromPath((const char*) regs.ecx);
				} break;

				case VFS_READ: {
					regs.eax = (int) ReadFile((void*) regs.ecx, (int) regs.edx, (File*) regs.esi);					
				} break;

				case VFS_WRITE: {
					regs.eax = (int) WriteFile((void*) regs.ecx, (int) regs.edx, (File*) regs.esi);
					break;
				}

				case VFS_SEEK: {
					regs.eax = (int) FileSeek((int) regs.ecx, (File*) regs.ebx);
				} break;

				case VFS_TELL: {
					regs.eax = (int) FileTell((File*) regs.ecx);
				}
			}
		}
	}
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
	
	registerIntHandler(IRQ(1), null_kb_handler);
	registerIntHandler(IRQ(7), null_irq7_handler);
	
	registerIntHandler(71, driver_interface_handler);

	registerSyscall(SYSCALL_EXIT, syscall_exit);
	registerSyscall(SYSCALL_TASK, syscall_task);
	registerSyscall(SYSCALL_OPEN, syscall_open);
	registerSyscall(SYSCALL_WRITE, syscall_write);

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
	// DisablePrintingToStdout(); // No clue what this was for.
	
	if(int_handlers[regs.int_no] != 0) {
		IntHandler handler = int_handlers[regs.int_no];
		handler(regs);
	} else {
		kprintf("received int:  %d\n", regs.int_no);
		for(;;) {}
	}
	
	// EnablePrintingToStdout(); // No clue what this was for.
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
