[section .text]
[global _start]

%define SYSCALL_EXIT 0x0
%define SYSCALL_CONSOLE 0x1
	%define CONSOLE_PUTS 0x0
	%define CONSOLE_PUTCH 0x1
	%define CONSOLE_PUTHEX 0x2
	%define CONSOLE_PUTDEC 0x3
%define SYSCALL_TASK 0x3
	%define TASK_GETPID 0x0
	
_start:
	mov ebp, esp
	
	;push hello_world_text
	;push CONSOLE_PUTS
	;push SYSCALL_CONSOLE
	;call syscall3
	
	mov eax, SYSCALL_CONSOLE
	mov ebx, CONSOLE_PUTS
	mov ecx, hello_world_text
	int 70
	
	add esp, 4
	
	mov eax, SYSCALL_EXIT
	mov ebx, 0
	int 70
	
[section .data]

hello_world_text: db "Hello, world v1 asm %d", 0xa, 0
newline: db 0xa, 0
