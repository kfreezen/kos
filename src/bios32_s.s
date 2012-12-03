[section .text]
[GLOBAL DetectService]
[EXTERN kalloc]
[EXTERN bios32]
[EXTERN kprintf]

[GLOBAL GetPCIBiosInfo]

%define PCI_FUNCTION_ID 0xB1
%define PCI_BIOS_PRESENT 0x01


DetectService : ; Bios32Service* () (UInt32 id, UInt32 selector);
	push ebp
	push ebx
	mov ebp, esp
	
	mov eax, [bios32]
	test eax, eax
	jz .fail
		mov eax, [esp+12]
		mov ebx, [esp+16]
		mov ecx, [bios32]
		mov ecx, [ecx+4]
		
		push cs
		call ecx
		
		; Save the registers
		push edx
		push ebx
		push ecx
		push eax
		
		mov eax, 16
		push eax
		call kalloc
		add esp, 4
		
		mov ebx, eax
		
		pop eax
		pop ecx
		mov [ebx], al
		mov [ebx+4], ecx
		pop eax
		pop ecx
		mov [ebx+8], eax
		mov [ebx+12], ecx
		mov eax, ebx
	jmp .return
	.fail:
	mov eax, 0
	.return:
	pop ebx
	pop ebp
	ret
	
GetPCIBiosInfo: ; PCIBiosInfo* () (Bios32Service*)
	push ebx
	push ebp
	mov ebp, esp
	
	
[section .data]

strBiosCall db "BiosCall=%x", 0xa, 0

