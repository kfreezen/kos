#include <KOSTypes.h>

void memset(Pointer p, UInt8 value, size_t size) {
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
    while(n--)
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
