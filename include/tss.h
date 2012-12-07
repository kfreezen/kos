#ifndef TSS_H
#define TSS_H

#include <KOSTypes.h>

struct TSS {
	UInt16 link;
	UInt16 reserved_0;
	
	UInt32 esp0;
	UInt32 ss0;
	
	UInt32 esp1;
	UInt32 ss1;
	
	UInt32 esp2;
	UInt32 ss2;
	
	UInt32 cr3;
	UInt32 eip;
	UInt32 eflags;
	UInt32 eax, ecx, edx, ebx, esp, ebp, esi, edi;
	
	UInt32 es, cs, ss, ds, fs, gs, ldtr;
	
	UInt16 reserved_4;
	UInt16 iopb_offset;
};

typedef struct TSS TSS;

#endif
