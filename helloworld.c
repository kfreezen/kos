#include <stdio.h>
#include <sys/syscalls.h>

/*
void _start() {
	syscall3(SYSCALL_CONSOLE, CONSOLE_PUTS, "Hello, world ");
	syscall3(SYSCALL_CONSOLE, CONSOLE_PUTDEC, syscall2(SYSCALL_TASK, TASK_GETPID));
	
	syscall3(SYSCALL_CONSOLE, CONSOLE_PUTS, "\n");
	syscall2(SYSCALL_EXIT, 0);
}*/

void _start() {
	//printf("Hello, world v1 %d\n", syscall2(SYSCALL_TASK, TASK_GETPID));
	puts("Hello, world v1\n");
	syscall2(SYSCALL_EXIT, 0);
}
