[global syscall]

[section .text]
syscall:
	mov eax, [esp+4]
	and eax, 0xFF
	shl eax, 2
	mov eax, [syscalls+eax]
	call eax
	ret
	
Nop_SysCall:
	xor eax, eax
	ret
	
[section .data]

[extern PutString_SysCall]
[extern ClearScreen_SysCall]

syscalls:
	dd PutString_SysCall
	dd ClearScreen_SysCall
	times 254 dd Nop_SysCall