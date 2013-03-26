[section .text]
[global _start]

[extern syscall3]
[extern syscall2]
[extern puts]

%define SYSCALL_EXIT 0x0
%define SYSCALL_CONSOLE 0x1
	%define CONSOLE_PUTS 0x0
	%define CONSOLE_PUTCH 0x1
	%define CONSOLE_PUTHEX 0x2
	%define CONSOLE_PUTDEC 0x3
%define SYSCALL_TASK 0x3
	%define TASK_GETPID 0x0
	
_start:
	push ebp
	mov ebp, esp
	
	push hello_world_text
	push CONSOLE_PUTS
	push SYSCALL_CONSOLE
	call syscall3
	add esp, 12
	
	push hello_world2_text
	call puts
	add esp, 4
	
	push  0
	push SYSCALL_EXIT
	call syscall2
	
[section .data]

hello_world_text: db "Hello, world0", 0xa, 0
hello_world2_text: db "Hello, world 234", 0xa, 0
newline: db 0xa, 0
