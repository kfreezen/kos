#include <bios32.h>
#include <print.h>

Bool bios32_alreadySearched = FALSE;
Bios32* bios32 = NULL;

Bool FindBios32() {
	if(bios32_alreadySearched) {
		return (bios32) ? TRUE : FALSE;
	} else {
		Bios32* bios32_loc = (Bios32*) 0xe0000;
		while(bios32_loc<(Bios32*)0xFFFFF) {
			if(bios32_loc->magic==BIOS32_MAGIC) {
				bios32 = bios32_loc;
				kprintf("loc=%x\n", bios32);
				break;
			}
			
			bios32_loc ++;
		}
		
		bios32_alreadySearched = TRUE;
		
		kprintf("locc=%x\n", bios32);
		
		return (bios32) ? TRUE : FALSE;
	}
}

Bios32* GetBios32() {
	if(!bios32_alreadySearched) {
		FindBios32();
	}
	
	return bios32;
}
