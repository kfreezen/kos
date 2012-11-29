#include <cmos.h>

Byte nmi_disable = 0;

void SetNMIStatus(Bool disable) {
	nmi_disable = (disable & 1) << 7;
}

Byte GetRegister(int reg) {
	outb(0x70, (nmi_disable) | reg);
	return inb(0x71);
}
