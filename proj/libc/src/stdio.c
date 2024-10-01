#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/syscalls.h>

int screenFd = -1;

static inline void initLib() {
	if(screenFd == -1) {
		screenFd = open("/dev/screen", O_WRITE);
	}
}

void internal_puts(const char* str) {
	initLib();

	write(screenFd, str, strlen(str));
}

int puts(const char* str) {
	internal_puts(str);
	
	putchar('\n');

	return 1;
}

int putchar(int ch) {
	initLib();

	char c;
	c = (char) ch;

	write(screenFd, &c, 1);

	return ch;
}

int printf(const char* fmt, ...) {
	va_list va;
	va_start(va, fmt);
	
	initLib();

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
							internal_puts("(unimp)");
							i++;
							break;
							
						}
						
						case 'x': {
							unsigned x = va_arg(va, unsigned);
							internal_puts("(unimp)");
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
