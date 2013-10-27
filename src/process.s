[global dotaskswitch]
[global TaskStackSetup]
[extern kprintf]

%define SLEEP_TICKS 24

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
	hlt
	jmp DefaultTaskThread

[global idleProcess]

; How do you like taking a page from the Microsoft book?
idleProcess:
	hlt
	jmp idleProcess

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
[extern tick]

TaskSwitch:
	push ebp
	mov ebp, esp
	sub esp, 12
	push ebx
	
	mov ebx, [current_task]
	
.doNext:
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
	
	mov ecx, [tick]
	mov eax, [ebx+SLEEP_TICKS]
	cmp ecx, eax
	jb .doNext

.doSwitch:
	; TODO Figure out how to create a struct in nasm assembly code.
	cli
	mov eax, [ebx+16] ; pageDirectory
	mov esp, [ebx+4]
	mov ebp, [ebx+8]
	mov eax, [eax+8192] ; pageDirectory->phys
	mov ecx, [ebx+12]
	mov cr3, eax
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