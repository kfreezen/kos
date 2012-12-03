[global switch_page_dir]

; The C function SwitchPageDirectory wraps this function.
switch_page_dir: ; Page Directory pointer
	mov eax,  [esp+4]
	mov cr3, eax
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	ret
	