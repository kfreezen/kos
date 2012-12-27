[section .text]
[global _start]

%define SYSCALL_CONSOLE 0x1
	%define CONSOLE_PUTS 0x0
	%define CONSOLE_PUTCH 0x1
	%define CONSOLE_PUTHEX 0x2
	%define CONSOLE_PUTDEC 0x3
%define SYSCALL_TASK 0x3
	%define TASK_GETPID 0x0
	
_start:
	mov eax, SYSCALL_CONSOLE
	mov ebx, CONSOLE_PUTS
	mov ecx, hello_world_text
	int 70
	
	mov eax, SYSCALL_TASK
	mov ebx, TASK_GETPID
	int 70
	
	mov ecx, eax
	mov eax, SYSCALL_CONSOLE
	mov ebx, CONSOLE_PUTDEC
	int 70
	
	mov eax, SYSCALL_CONSOLE
	mov ebx, CONSOLE_PUTS
	mov ecx, newline
	int 70
	
	xor eax, eax
	xor ebx, ebx
	int 70

[section .data]

hello_world_text: db "Hello, world ", 0
newline: db 0xa, 0
