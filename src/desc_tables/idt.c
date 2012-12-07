#include <idt.h>

IdtDescriptor idt_desc;
IdtEntry idt[256];

extern void idt_flush(UInt32 ptr);

int IDT_Init() {
	idt_desc.limit = (sizeof(IdtEntry)*256)-1;
	idt_desc.base = (UInt32)idt;
	
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	outb(0x21, 0x20);
	outb(0xA1, 0x28);
	outb(0x21, 0x04);
	outb(0xA1, 0x02);
	outb(0x21, 0x01);
	outb(0xA1, 0x01);
	outb(0x21, 0x0);
	outb(0xA1, 0x0);
	
	idt[0 ] = FillIdtEntry(0x08, (UInt32)&isr0 , 0x8E);
	idt[1 ] = FillIdtEntry(0x08, (UInt32)&isr1 , 0x8E);
	idt[2 ] = FillIdtEntry(0x08, (UInt32)&isr2 , 0x8E);
	idt[3 ] = FillIdtEntry(0x08, (UInt32)&isr3 , 0x8E);
	idt[4 ] = FillIdtEntry(0x08, (UInt32)&isr4 , 0x8E);
	idt[5 ] = FillIdtEntry(0x08, (UInt32)&isr5 , 0x8E);
	idt[6 ] = FillIdtEntry(0x08, (UInt32)&isr6 , 0x8E);
	idt[7 ] = FillIdtEntry(0x08, (UInt32)&isr7 , 0x8E);
	idt[8 ] = FillIdtEntry(0x28, (UInt32)0     , 0x8E);
	//idt[8].offset_low = ((UInt32)&isr8) & 0xFFFF;
	//idt[8].offset_high = ((UInt32)&isr8) >> 16;
	//idt[8].zero = 0;
	//idt[8].type_attr = 0x85;
	//idt[8].sel = 0x28;
	idt[9 ] = FillIdtEntry(0x08, (UInt32)&isr9 , 0x8E);
	idt[10] = FillIdtEntry(0x08, (UInt32)&isr10, 0x8E);
	idt[11] = FillIdtEntry(0x08, (UInt32)&isr11, 0x8E);
	idt[12] = FillIdtEntry(0x08, (UInt32)&isr12, 0x8E);
	idt[13] = FillIdtEntry(0x08, (UInt32)&isr13, 0x8E);
	idt[14] = FillIdtEntry(0x08, (UInt32)&isr14, 0x8E);
	idt[15] = FillIdtEntry(0x08, (UInt32)&isr15, 0x8E);
	idt[16] = FillIdtEntry(0x08, (UInt32)&isr16, 0x8E);
	idt[17] = FillIdtEntry(0x08, (UInt32)&isr17, 0x8E);
	idt[18] = FillIdtEntry(0x08, (UInt32)&isr18, 0x8E);
	idt[19] = FillIdtEntry(0x08, (UInt32)&isr19, 0x8E);
	idt[20] = FillIdtEntry(0x08, (UInt32)&isr20, 0x8E);
	idt[21] = FillIdtEntry(0x08, (UInt32)&isr21, 0x8E);
	idt[22] = FillIdtEntry(0x08, (UInt32)&isr22, 0x8E);
	idt[23] = FillIdtEntry(0x08, (UInt32)&isr23, 0x8E);
	idt[24] = FillIdtEntry(0x08, (UInt32)&isr24, 0x8E);
	idt[25] = FillIdtEntry(0x08, (UInt32)&isr25, 0x8E);
	idt[26] = FillIdtEntry(0x08, (UInt32)&isr26, 0x8E);
	idt[27] = FillIdtEntry(0x08, (UInt32)&isr27, 0x8E);
	idt[28] = FillIdtEntry(0x08, (UInt32)&isr28, 0x8E);
	idt[29] = FillIdtEntry(0x08, (UInt32)&isr29, 0x8E);
	idt[30] = FillIdtEntry(0x08, (UInt32)&isr30, 0x8E);
	idt[31] = FillIdtEntry(0x08, (UInt32)&isr31, 0x8E);
	idt[32] = FillIdtEntry(0x08, (UInt32)&irq0, 0x8e);
	idt[33] = FillIdtEntry(0x08, (UInt32)&irq1, 0x8e);
	idt[34] = FillIdtEntry(0x08, (UInt32)&irq2, 0x8e);
	idt[35] = FillIdtEntry(0x08, (UInt32)&irq3, 0x8e);
	idt[36] = FillIdtEntry(0x08, (UInt32)&irq4, 0x8e);
	idt[37] = FillIdtEntry(0x08, (UInt32)&irq5, 0x8e);
	idt[38] = FillIdtEntry(0x08, (UInt32)&irq6, 0x8e);
	idt[39] = FillIdtEntry(0x08, (UInt32)&irq7, 0x8e);
	idt[40] = FillIdtEntry(0x08, (UInt32)&irq8, 0x8e);
	idt[41] = FillIdtEntry(0x08, (UInt32)&irq9, 0x8e);
	idt[42] = FillIdtEntry(0x08, (UInt32)&irq10, 0x8e);
	idt[43] = FillIdtEntry(0x08, (UInt32)&irq11, 0x8e);
	idt[44] = FillIdtEntry(0x08, (UInt32)&irq12, 0x8e);
	idt[45] = FillIdtEntry(0x08, (UInt32)&irq13, 0x8e);
	idt[46] = FillIdtEntry(0x08, (UInt32)&irq14, 0x8e);
	idt[47] = FillIdtEntry(0x08, (UInt32)&irq15, 0x8e);
	idt[70] = FillIdtEntry(0x08, (UInt32)&sys70, 0x8e);
	idt[71] = FillIdtEntry(0x08, (UInt32)&isr71, 0x8e);
	
	idt_flush((UInt32)&idt_desc);
	
	return 0;
}

IdtEntry FillIdtEntry(UInt16 sel, UInt32 offset, UInt8 type_attr) {
	IdtEntry ret;
	ret.offset_low = offset&0xFFFF;
	ret.sel = sel;
	ret.zero = 0;
	ret.type_attr = type_attr;
	ret.offset_high = (offset>>16)&0xFFFF;
	
	return ret;
}
