#include <gdt.h>
#include <tss.h>
#include <paging.h>

Bool gdt_isInit = false;

//#define GDT_DEBUG
#define GDT_ENTRIES 6

GdtDescriptor gdt_desc;
GdtEntry gdt[GDT_ENTRIES];

TSS double_fault_tss __attribute__ ((aligned(4096)));

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

extern void load_tss(UInt32 segment_desc);
extern PageDirectory* staticKPageDir;
extern void isr8();

int GDT_InitEx(Bool reinit) {
	if(gdt_isInit == true && reinit == false) {
		return 1;
	}
	
	// 0 = null
	// 1 = cs 0
	// 2 = ds 0
	// 3 = cs 3
	// 4 = ds 3
	
	gdt_desc.size = (sizeof(GdtEntry)*GDT_ENTRIES)-1;
	gdt_desc.offset = (UInt32) &gdt;
	
	gdt[0] = FillNullGdtEntry();
	gdt[1] = FillGdtEntry(0x00000000, 0xFFFFFFFF, 0x9A, 0xC);
	gdt[2] = FillGdtEntry(0x00000000, 0xFFFFFFFF, 0x92, 0xC);
	gdt[3] = FillGdtEntry(0x00000000, 0xFFFFFFFF, 0xFA, 0xC);
	gdt[4] = FillGdtEntry(0x00000000, 0xFFFFFFFF, 0xF2, 0xC);
	
	//memset(&double_fault_tss, 0, sizeof(TSS));
	
	double_fault_tss.ss0 = 0x10;
	double_fault_tss.esp0 = 0x7c00; // This should be changed.
	double_fault_tss.iopb_offset = sizeof(TSS);
	//double_fault_tss.cr3 = staticKPageDir;
	double_fault_tss.eip = &isr8;
	double_fault_tss.cs = 0x8;
	double_fault_tss.ds = 0x10;
	double_fault_tss.es = double_fault_tss.fs = double_fault_tss.gs = 0x10;
	double_fault_tss.eflags = 2;
	
	gdt[5] = FillGdtEntry(((UInt32)&double_fault_tss), (sizeof(TSS)-1)<<12, 0x89, 0x40);
	
	//kprintf("gdt[5]=%x %x\n", ((UInt32*)&gdt[5])[0], ((UInt32*)&gdt[5])[1]);
	
	gdt_flush((UInt32)&gdt_desc);
	
	load_tss(0x28);
	
	gdt_isInit = true;
	
	return 0;
}

GdtEntry FillGdtEntry(UInt32 base, UInt32 limit, int access, int flags) {
	#ifdef GDT_DEBUG
	kprintf("FillGdtEntry(%x,%x,%x,%x) ", base, limit, access, flags);
	#endif 

	GdtEntry ret;
	limit = (limit>>12) & 0x000FFFFF;
	ret.limit_low = limit & 0xFFFF;
	ret.base_low = base & 0xFFFF;
	ret.base_middle = (base>>16) & 0xFF;
	ret.access = access;
	ret.flags_limit_middle = ( (flags&0xF)<<4 ) | ( (limit&0xF0000)>>16 );
	ret.base_high = (base>>24) & 0xFF;
	
	#ifdef GDT_DEBUG
	kprintf("ret %x %x\n", ((UInt32*)&ret)[1], ((UInt32*)&ret)[0]);
	#endif
	
	return ret;
}
