[section .text]

[GLOBAL Bitset_Test]
[GLOBAL Bitset_Set]

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
		
Bitset_Set: ; int (Bitset* set, int idx, int value)
	push ebx
	push ebp
	mov ebp, esp
	sub esp, 8
	; ebp-4 = idx/32 division result.
	; ebp-8 = idx/32 remainder result.
	mov eax, [ebp+16]
	mov ecx, 32
	xor edx, edx
	div ecx
	
	mov [ebp-4], eax
	mov [ebp-8], edx
	
	; Check if set is null and check eax with set->length
	or ebx, ebx
	jz .fail
	
	cmp eax, [ebx+4]
	jae .fail
	
	mov ecx, [ebp+12] ; set
	mov ebx, [ecx]
	mov ecx, [ebx+eax*4]
	
	push eax
	
	mov eax, [ebp+20]
	and al, 1
	mov ah, al
	lahf
	mov eax, [ebp-12]
	jnc .reset
	
	.set:
		bts ecx, edx
		jmp .after_set_reset
	.reset:
		btr ecx, edx
	.after_set_reset:
		pop eax
		mov [ebx+eax*4], ecx
	
	jmp .success
	
	.fail:
		mov eax, -1
	.success:
		add esp, 8
		pop ebp
		pop ebx
		ret
		
after_all:
	int 3
