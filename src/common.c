#include <KOSTypes.h>
#include <common.h>

void memset(Pointer p, UInt8 value, size_t size) {
	//kprintf("memset(%x,%x,%x)\n", p, value, size);
	
	Byte* b = (Byte*) p;
	int i;
	for(i=0; i<size; i++) {
		b[i] = value;
	}
}

void outb(UInt16 port, UInt8 value) {
	asm volatile("outb %1, %0" : : "dN" (port), "a" (value));
}

void outw(UInt16 port, UInt16 value) {
	asm volatile("outw %1, %0" : : "dN" (port), "a" (value));
}

void outl(UInt16 port, UInt32 value) {
	asm volatile("outl %1, %0" : : "dN" (port), "a" (value));
}

UInt8 inb(UInt16 port) {
	UInt8 ret;
	asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

UInt16 inw(UInt16 port) {
	UInt16 ret;
	asm volatile("inw %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

UInt32 inl(UInt16 port) {
	UInt32 ret;
	asm volatile("inl %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

void memcpy(Pointer p1, Pointer p2, size_t size) {
	Byte* b1, *b2;
	
	b1 = p1;
	b2 = p2;
	
	int i;
	for(i=0; i<size; i++) {
		b1[i] = b2[i];
	}
	
}

int strcmp(const char* str1, const char* str2)
{
      int i = 0;
      int failed = 0;
      while(str1[i] != '\0' && str2[i] != '\0')
      {
          if(str1[i] != str2[i])
          {
              failed = 1;
              break;
          }
          i++;
      }
      // why did the loop exit?
      if( (str1[i] == '\0' && str2[i] != '\0') || (str1[i] != '\0' && str2[i] == '\0') )
          failed = 1;
  
      return failed;
}

int strncmp(const char* s1, size_t n, const char* s2)
{
    while(n-- && *s1 != '\0' && *s2 != '\0')
        if(*s1++!=*s2++)
            return *(unsigned char*)(s1 - 1) - *(unsigned char*)(s2 - 1);
    return 0;
}

void strcpy(char* s1, const char* s2) {
	int i;
	for(i=0; s2[i]!=0; i++) {
		s1[i] = s2[i];
	}
	s1[i] = 0;
	
}

int strncpy(char* s1, size_t sz, const char* s2) {
	int i;
	for(i=0; s2[i]!=0 && i<sz; i++) {
		s1[i] = s2[i];
	}
	
	if(i<sz) {
		s1[i] = 0;
	}
	
	return i;
}

Int32 strlen(const char* s) {
	Int32 i=0;
	
	while(s[i]!=0) {
		i++;
	}
	
	return i;
}

/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

char* strtok_r(char* s, const char* delim, char** last);

// UGLY CODE, DO NOT ENTER IF YOU VALUE YOUR LIFE
char * strtok(char *s, const char *delim) {
	static char *last;

	return strtok_r(s, delim, &last);
}

char *
strtok_r(char *s, const char *delim, char **last) {
	char *spanp;
	int c, sc;
	char *tok;


	if (s == NULL && (s = *last) == NULL)
		return (NULL);

	/*
	 * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
	 */
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {		/* no non-delimiter characters */
		*last = NULL;
		return (NULL);
	}
	tok = s - 1;

	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*last = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}