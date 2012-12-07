dd 0F00DFACEh
dd 0x40000000
dd start
dd data

[ORG 0x40000000]
[BITS 32]

start:
	push ebx
	push esi
	push edi
	
	mov eax, 0
	mov ebx, 0
	mov ecx, helloWorld
	int 70
	
	.loop:
		mov eax, 1
		mov ebx, 0
		int 70
		
		movzx ecx, al
		
		cmp ecx, 0
		jz .loop
		
		mov eax, 0
		mov ebx, 1
		int 70
		
		jmp .loop
		
data:

buffer times 256 db 0

align 4
buffer_itr dd 0
helloWorld db "Hello, kOS!", 10, 0

