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

#define IRQBASE 32
#define IRQ(i) (IRQBASE+(i))
#define IRQ0  IRQ(0)
#define IRQ1  IRQ(1)
#define IRQ2  IRQ(2)
#define IRQ3  IRQ(3)
#define IRQ4  IRQ(4)
#define IRQ5  IRQ(5)
#define IRQ6  IRQ(6)
#define IRQ7  IRQ(7)
#define IRQ8  IRQ(8)
#define IRQ9  IRQ(9)
#define IRQ10 IRQ(10)
#define IRQ11 IRQ(11)
#define IRQ12 IRQ(12)
#define IRQ13 IRQ(13)
#define IRQ14 IRQ(14)
#define IRQ15 IRQ(15)

// TODO:  Simplify SYSCALL_CONSOLE and take out the PUTHEX and PUTDEC

#define SYSCALL_EXIT 0x0
#define SYSCALL_CONSOLE 0x1
	#define CONSOLE_PUTS 0x0
	#define CONSOLE_PUTCH 0x1
	#define CONSOLE_PUTHEX 0x2
	#define CONSOLE_PUTDEC 0x3
#define SYSCALL_KB 0x2
	#define KB_POLLCH 0x0
	#define KB_GETCH 0x1
#define SYSCALL_TASK 0x3
	#define TASK_GETPID 0x0
#define SYSCALL_OPEN 0x4

#define SYSCALL_WRITE 0x7

#endif
