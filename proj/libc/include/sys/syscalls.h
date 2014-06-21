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
	#define CONSOLE_GETCH 0x1
#define SYSCALL_TASK 0x3
	#define TASK_GETPID 0x0
#define SYSCALL_OPEN 0x4
#define SYSCALL_WRITE 0x7

#define O_WRITE 0

int open(const char* pathName, int flags);
int write(int fd, const char* buf, int count);

#endif
