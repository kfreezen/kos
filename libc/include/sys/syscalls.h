#ifndef SYSCALLS_H
#define SYSCALLS_H

#define SYSCALL_EXIT 0x0
#define SYSCALL_CONSOLE 0x1
	#define CONSOLE_PUTS 0x0
	#define CONSOLE_PUTCH 0x1
	#define CONSOLE_PUTHEX 0x2
	#define CONSOLE_PUTDEC 0x3
#define SYSCALL_KB 0x2
	#define CONSOLE_POLLCH 0x0
#define SYSCALL_TASK 0x3
	#define TASK_GETPID 0x0

extern unsigned syscall1(int syscall);
extern unsigned syscall2(int syscall, unsigned arg0);
extern unsigned syscall3(int syscall, unsigned arg0, unsigned arg1);

#endif
