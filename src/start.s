MBOOT_PAGE_ALIGN    equ 1<<0    ; Load kernel and modules on a page boundary
MBOOT_MEM_INFO      equ 1<<1    ; Provide your kernel with memory info
MBOOT_HEADER_MAGIC  equ 0x1BADB002 ; Multiboot Magic value
; NOTE: We do not use MBOOT_AOUT_KLUDGE. It means that GRUB does not
; pass us a symbol table.
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)


[BITS 32]                       ; All instructions should be 32-bit.

[GLOBAL mboot]                  ; Make 'mboot' accessible from C.
[EXTERN code]                   ; Start of the '.text' section.
[EXTERN bss]                    ; Start of the .bss section.
[EXTERN end]                    ; End of the last loadable section.

section .mbHeader
align 8
mboot:
  dd  MBOOT_HEADER_MAGIC        ; GRUB will search for this value on each
                                ; 4-byte boundary in your kernel file
  dd  MBOOT_HEADER_FLAGS        ; How GRUB should load your file / settings
  dd  MBOOT_CHECKSUM            ; To ensure that the above values are correct
   
  ; dd  mboot                     ; Location of this descriptor
  ; dd  code                      ; Start of kernel '.text' (code) section.
  ; dd  bss                       ; End of kernel '.data' section.
  ; dd  end                       ; End of kernel.
  ; dd  start                     ; Kernel entry point (initial EIP).

[GLOBAL start]                  ; Kernel entry point.
[EXTERN kmain]                   ; This is the entry point of our C code
[GLOBAL _invlpg]
[GLOBAL mov_eax]
[GLOBAL full_invlpg]

section .text
mov_eax:
	mov eax, [esp+4]
	ret

_invlpg:
	mov eax, [esp+4]
	invlpg [eax]
	ret

full_invlpg:
	push eax
	mov eax, cr3
	mov cr3, eax
	pop eax
	ret
	
start:
  push	  eax
  push    ebx                   ; Load multiboot header location
  push	  esp
  
  ; Execute the kernel:
  cli                         ; Disable interrupts.
  call kmain                   ; call our main() function.
  
  jmp $                       ; Enter an infinite loop, to stop the processor
                              ; executing whatever rubbish is in the memory
                              ; after our kernel!

; u64 _rdtsc()

[global _rdtsc]

_rdtsc:
	rdtsc
	ret
	
[global timer_callback]
[extern tick]
[extern interval_enable]
[extern u_interval]
[extern PutChar]
[extern interval_c]
[extern pithook]
[extern mstime]
[extern current_task]
[extern __rdtsc]
[extern last_rdtsc]
[extern TaskSwitch]
[extern FloppyMotorCallback]

timer_callback:
	push esi
	
	mov eax, [current_task]
	
	or eax, eax
	jz .noSwitch
	
	call TaskSwitch
	
	.noSwitch:
	call FloppyMotorCallback
	
	xor eax, eax
	xor edx, edx
	mov ecx, [u_interval]
	mov esi, [interval_enable]
	mov al, [interval_c]
	
	inc DWORD [tick]
	test esi, 1
	jz .endif_interval
	test ecx, ecx
	jz .endif_interval
		div ecx
		test edx, edx
		jnz .endif_interval
			push eax
			call PutChar
			add esp, 4
	.endif_interval:
	
	mov esi, [pithook]
	
	test esi, esi
	jz .endif_pithook
		mov eax, [mstime]
		push DWORD [tick]
		push eax
		call esi
		add esp, 8
	.endif_pithook:
	
	rdtsc
	xchg eax, [__rdtsc]
	xchg edx, [__rdtsc+4]
	mov [last_rdtsc], eax
	mov [last_rdtsc+4], edx
	
	pop esi
	ret
	
[global getDWord]
[global getWord]

getDWord:
	mov edx, [esp+4]
	mov eax, [edx]
	ret
	
getWord:
	mov edx, [esp+4]
	mov ax, [edx]
	ret
	
[global setDWord]

setDWord:
	mov eax, [esp+4]
	mov edx, [esp+8]
	mov [eax], edx
	ret
	
[global bswap16]

bswap16:
	mov eax, [esp+4]
	xchg al, ah
	ret
	
[section .data]
[global int_handlers]
[extern genericHandler]

int_handlers:
	times 256 dd genericHandler
	
