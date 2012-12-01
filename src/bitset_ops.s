[section .text]

[GLOBAL Bitset_Test]
;[GLOBAL Bitset_Set]

Bitset_Test: ; int (Bitset* set, int idx)
	push ebx
	; ebx = Bitset* set
	; eax = int idx
	mov ebx, [esp+8]
	mov eax, [esp+12]
	xor edx, edx
	mov ecx, 32
	div ecx
	
	or ebx, ebx
	jz .fail
	
	; Make sure that the idx is less than set->length
	mov ecx, [ebx+4]
	cmp eax, ecx
	jae .fail
	
	; idx is less.
	mov ebx, [ebx] ; set->length
	mov ecx, [ebx+eax*4] ; set->length[idx]
	bt ecx, edx ; cf = ecx&1>>edx
	setc al
	movzx eax, al
	jmp .success
	
	.fail:
		mov eax, -1
	.success:
		pop ebx
		ret
		
after_all:
	int 3
