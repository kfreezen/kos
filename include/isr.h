#ifndef ISR_H
#define ISR_H

#include <KOSTypes.h>

extern void _syscall(UInt32 eax, Pointer ebx, Pointer ecx, Pointer edx);

typedef struct registers {
	UInt32 ds;
	UInt32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
	UInt32 int_no, err_code;
	UInt32 eip, cs, eflags, useresp, ss;
} Registers;

typedef void (*IntHandler)(Registers regs);
typedef void (*Syscall)(Registers* regs);

void registerIntHandler(int interrupt, IntHandler h);
void registerSyscall(int eax, Syscall s);

int ISR_Init();

#define IRQ0  32
#define IRQ1  33
#define IRQ2  34
#define IRQ3  35
#define IRQ4  36
#define IRQ5  37
#define IRQ6  38
#define IRQ7  39
#define IRQ8  40
#define IRQ9  41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

// TODO:  Simplify SYSCALL_CONSOLE and take out the PUTHEX and PUTDEC

#define SYSCALL_EXIT 0x0
#define SYSCALL_CONSOLE 0x1
	#define CONSOLE_PUTS 0x0
	#define CONSOLE_PUTCH 0x1
	#define CONSOLE_PUTHEX 0x2
	#define CONSOLE_PUTDEC 0x3
#define SYSCALL_KB 0x2
	#define KB_POLLCH 0x0
#define SYSCALL_TASK 0x3
	#define TASK_GETPID 0x0
#endif
