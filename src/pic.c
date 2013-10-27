#include <pic.h>

#include <KOSTypes.h>

void PIC_Remap() {
	outb(PIC1_CMD, 0x11);
	outb(PIC2_CMD, 0x11);
	outb(PIC1_DATA, 0x20);
	outb(PIC2_DATA, 0x28);
	outb(PIC1_DATA, 0x04);
	outb(PIC2_DATA, 0x02);
	outb(PIC1_DATA, 0x01);
	outb(PIC2_DATA, 0x01);
	outb(PIC1_DATA, 0x0);
	outb(PIC2_DATA, 0x0);
}

void PIC_ToggleMask(int line, int lineValue) {
	UInt16 port;
	UInt8 value;

	if(line < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		line -= 8;
		if(line > 8) {
			return; // Invalid IRQ number
		}
	}

	value = inb(port);
	value = (lineValue) ? (value | (1<<line)) : (value & ~(1<<line));
	outb(port, value);

}