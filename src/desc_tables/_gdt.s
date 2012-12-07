[GLOBAL gdt_flush]
[GLOBAL load_tss]

gdt_flush:
	mov eax, [esp+4]
	lgdt [eax]
	
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	jmp 0x08:.flush
	
.flush:
	ret
	
load_tss:
	mov eax, [esp+4]
	ltr ax
	ret
