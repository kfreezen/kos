[ORG 0x500]
[BITS 32]

start:
	mov eax, 0
	mov ebx, hello_world_text
	int 70
	ret
	
nop
nop
nop
nop

hello_world_text: db "Hello, world", 0xa, 0