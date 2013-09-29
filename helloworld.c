/*
void _start() {
	syscall3(SYSCALL_CONSOLE, CONSOLE_PUTS, "Hello, world ");
	syscall3(SYSCALL_CONSOLE, CONSOLE_PUTDEC, syscall2(SYSCALL_TASK, TASK_GETPID));
	
	syscall3(SYSCALL_CONSOLE, CONSOLE_PUTS, "\n");
	syscall2(SYSCALL_EXIT, 0);
}*/

int syscall1(int syscall) {
	asm volatile("mov %0, %%eax; int $70;" :: "a" (syscall));
}

int syscall2(int syscall, unsigned arg0) {
	asm volatile("mov %0, %%eax; mov %1, %%ebx; int $70" :: "a" (syscall), "b" (arg0));
}

int syscall3(int syscall, unsigned arg0, unsigned arg1) {
	asm volatile("mov %0, %%eax; mov %1, %%ebx; mov %2, %%ecx; int $70" :: "a" (syscall), "b" (arg0), "c" (arg1));
}

void _start() {
	syscall3(0x1, 0x0, "Hello, world0\n");
	
	while(1) {}
}
