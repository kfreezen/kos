[global dotaskswitch]

dotaskswitch:
	cli
	mov ebp, [esp+12]
	mov ecx, [esp+4]
	mov esp, [esp+8]
	mov eax, 0x12345
	
	sti
	jmp ecx
	
[global DefaultTaskThread]

DefaultTaskThread:
	jmp DefaultTaskThread
	
[global read_eip]

read_eip:
	pop eax
	jmp eax
	
[section .data]

[global default_stack]

default_stack dd 256