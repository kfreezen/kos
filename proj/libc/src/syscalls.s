[section .text]
	[global syscall1]
	[global syscall2]
	[global syscall3]
	[global syscall4]
	
	syscall1:
		mov eax, [esp+4]
		int 70
		ret
	
	syscall2:
		mov eax, [esp+4]
		mov ebx, [esp+8]
		int 70
		ret
		
	syscall3:
		mov eax, [esp+4]
		mov ebx, [esp+8]
		mov ecx, [esp+12]
		int 70
		ret
		
	syscall4:
		mov eax, [esp+4]
		mov ebx, [esp+8]
		mov ecx, [esp+12]
		mov edx, [esp+16]
		int 70
		ret