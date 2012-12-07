#include <pci.h>
#include <cl5446.h>
#include <print.h>

UInt32 CL5446_memory = 0;
UInt32 CL5446_bitblt = 0;
UInt32 CL5446_gpio = 0;

void vgaOut(int port, int index, UInt8 value) {
	outb(port-1, (UInt8) index);
	outb(port, value);
}

UInt8 vgaIn(int port, int idx) {
	outb(port-1, (UInt8) idx);
	return inb(port);
}

int CL5446_Init() {
	return 0;
}
