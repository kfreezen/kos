[SECTION .text]

[global floppy_re_entrant]

floppy_re_entrant: ; (FloppyFunc, ...)
	mov eax, [esp+4]
	call eax
	ret
	
