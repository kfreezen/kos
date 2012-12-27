[section .text]

[GLOBAL DoRetry]

; This function assumes that there is a stack frame set up.
DoRetry:  ; func
	mov ecx, [esp+4]
	mov esp, ebp
	pop ebp
	jmp ecx
