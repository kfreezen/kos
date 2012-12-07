[global dotaskswitch]
[global TaskStackSetup]
[extern kprintf]

TaskStackSetup:
	mov eax, [esp+4]
	mov ecx, [esp+8]
	mov [eax], ecx
	mov [eax+4], ecx
	ret
	

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
	
process_start_stub:
	sti
	ret

[global TaskSwitch]
[extern current_task]
[extern ready_queue]

TaskSwitch:
	push ebp
	mov ebp, esp
	sub esp, 12
	push ebx
	
	mov ebx, [current_task]
	
	; Save pointers
	mov [ebx+4], esp
	mov [ebx+8], ebp
	
	call read_eip
	
	cmp eax, 0x12345
	jz .ret
	
	mov [ebx+12], eax ;eip
	
	mov ebx, [ebx+20] ;ebx = ebx->next
	test ebx, ebx
	jz .curtaskisnull
	
	mov [current_task], ebx
	
	jmp .end_curtaskisnull
	
	.curtaskisnull:
		mov ebx, [ready_queue]
		mov [current_task], ebx
	.end_curtaskisnull:
	
	;mov ecx, 70000000
	;._loop_delay:
	;	dec ecx
	;	jecxz ._loop_delay_done
	;	jmp ._loop_delay
	;._loop_delay_done:
	
	mov ebx, [current_task]
	
	cli
	mov esp, [ebx+4]
	mov ebp, [ebx+8]
	mov ecx, [ebx+12]
	mov eax, 0x12345
	sti
	jmp ecx
	
	.ret:
		pop ebx
		add esp, 12
		pop ebp
		ret
		
[section .data]

[global default_stack]

default_stack dd 256

