#ifndef GDT_H
#define GDT_H

#include <KOSTypes.h>

struct __gdt_desc {
	UInt16 size;
	UInt32 offset;
} __attribute__((packed));

struct __gdt_entry {
	UInt16 limit_low;
	UInt16 base_low;
	UInt8 base_middle;
	UInt8 access;
	UInt8 flags_limit_middle;
	UInt8 base_high;
} __attribute__((packed));

typedef struct __gdt_desc GdtDescriptor;
typedef struct __gdt_entry GdtEntry;

GdtEntry FillGdtEntry(UInt32 base, UInt32 limit, int access, int flags);

int GDT_InitEx(Bool reinit);
int GDT_Init();

#endif
