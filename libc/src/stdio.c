#include <stdio.h>
#include <stdarg.h>
#include <sys/syscalls.h>

void internal_puts(const char* str) {
	syscall3(SYSCALL_CONSOLE, CONSOLE_PUTS, (unsigned)str);
}

int puts(const char* str) {
	internal_puts(str);
	syscall3(SYSCALL_CONSOLE, CONSOLE_PUTCH, '\n');
	return 1;
}

int putchar(int ch) {
	syscall3(SYSCALL_CONSOLE, CONSOLE_PUTCH, '\n');
	return ch;
}

int printf(const char* fmt, ...) {
	va_list va;
	va_start(va, fmt);
	
	int i;
	for(i=0; fmt[i]!='\0';) {
		switch(fmt[i]) {
			case '%': 
				{
					
					switch(fmt[++i]) {
						case '%':
							putchar('%');
							i++;
							break;
							
						case 'c': {
							char c = va_arg(va, char);
							putchar(c);
							i++;
							break;
							
						}
						
						case 's': {
							char* s = va_arg(va, char*);
							internal_puts(s);
							i++;
							break;
						}
							
						case 'd':
						case 'u': {
							unsigned u = va_arg(va, unsigned);
							syscall3(SYSCALL_CONSOLE, CONSOLE_PUTDEC, u);
							i++;
							break;
							
						}
						
						case 'x': {
							unsigned x = va_arg(va, unsigned);
							syscall3(SYSCALL_CONSOLE, CONSOLE_PUTHEX, x);
							i++;
							break;
						}
						
						case 'f': {
							float f = va_arg(va, float);
							//PutFloat(f);
							i++;
							break;
						}
					}
					
				}
				break;
				
			default:
				putchar(fmt[i++]);
				break;
				
		}
		
	}
	va_end(va);
}
