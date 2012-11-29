#include <gdt.h>

Bool gdt_isInit = false;

GdtDescriptor gdt_desc;
GdtEntry gdt[5];

extern void gdt_flush(UInt32 gdt_desc_ptr);

int GDT_Init() {
	return GDT_InitEx(false);
}

GdtEntry FillNullGdtEntry() {
	GdtEntry ret;
	ret.limit_low = 0;
	ret.base_low = 0;
	ret.base_middle = 0;
	ret.access = 0;
	ret.flags_limit_middle = 0;
	ret.base_high = 0;
	return ret;
}

int GDT_InitEx(Bool reinit) {
	if(gdt_isInit == true && reinit == false) {
		return 1;
	}
	
	// 0 = null
	// 1 = cs 0
	// 2 = ds 0
	// 3 = cs 3
	// 4 = ds 3
	
	gdt_desc.size = (sizeof(GdtEntry)*5)-1;
	gdt_desc.offset = (UInt32) &gdt;
	
	gdt[0] = FillNullGdtEntry();
	gdt[1] = FillGdtEntry(0x00000000, 0xFFFFFFFF, 0x9A, 0xC);
	gdt[2] = FillGdtEntry(0x00000000, 0xFFFFFFFF, 0x92, 0xC);
	gdt[3] = FillGdtEntry(0x00000000, 0xFFFFFFFF, 0xFA, 0xC);
	gdt[4] = FillGdtEntry(0x00000000, 0xFFFFFFFF, 0xF2, 0xC);

	gdt_flush((UInt32)&gdt_desc);
	gdt_isInit = true;
	
	return 0;
}

GdtEntry FillGdtEntry(UInt32 base, UInt32 limit, int access, int flags) {
	GdtEntry ret;
	limit = (limit>>12) & 0x000FFFFF;
	ret.limit_low = limit & 0xFFFF;
	ret.base_low = base & 0xFFFF;
	ret.base_middle = (base>>16) & 0xFF;
	ret.access = access;
	ret.flags_limit_middle = ( (flags&0xF)<<4 ) | ( (limit&0xF0000)>>16 );
	ret.base_high = (base>>24) & 0xFF;
	
	return ret;
}
