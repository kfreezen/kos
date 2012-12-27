[section .text]

[GLOBAL DumpStack]
[EXTERN kprintf]

%define STACK_NUMS 40
;%define DEBUG_DEBUG_FUNCS

DumpStack:
	%ifdef DEBUG_DEBUG_FUNCS
	pusha
	push DumpStackStr
	call kprintf
	add esp, 4
	popa
	%endif

	mov ecx, STACK_NUMS
	mov eax, esp
	
	.loop:
		mov edx, [eax+ecx*4]
		
		pusha
		push edx
		lea edx, [eax+ecx*4]
		push edx
		push stack_str
		call kprintf
		add esp, 12
		popa
		
		dec ecx
		jecxz .end_loop
		jmp .loop
	.end_loop:
	
	ret
	
[GLOBAL signExtendImm8]

signExtendImm8:
	mov eax, [esp+4]
	movsx eax, al
	ret

[section .data]

stack_str db "%x:  %x", 10, 0

%ifdef DEBUG_DEBUG_FUNCS
DumpStackStr db "DumpStack()", 10, 0
%endif