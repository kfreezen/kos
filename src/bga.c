#include <KOSTypes.h>
#include <print.h>

#define VBE_IO_IDX 0x1ce
#define VBE_IO_DATA 0x1cf

#define VBE_ID 0

int BGA_Init() {
	outw(VBE_IO_IDX, VBE_ID);
	int res = inw(VBE_IO_DATA);
	
	kprintf("%x\n", res);
	
	return 0;
}
