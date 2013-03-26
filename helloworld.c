#include <sys/syscalls.h>
#include <stdio.h>

/*
void _start() {
	syscall3(SYSCALL_CONSOLE, CONSOLE_PUTS, "Hello, world ");
	syscall3(SYSCALL_CONSOLE, CONSOLE_PUTDEC, syscall2(SYSCALL_TASK, TASK_GETPID));
	
	syscall3(SYSCALL_CONSOLE, CONSOLE_PUTS, "\n");
	syscall2(SYSCALL_EXIT, 0);
}*/

void _start() {
	syscall3(SYSCALL_CONSOLE, CONSOLE_PUTS, "Hello, world0\n");
	puts("Hello, world 234");
	syscall2(SYSCALL_EXIT, 0);
}
