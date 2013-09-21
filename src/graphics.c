#include <vga.h>
#include <graphics.h>

Graphics currentMode;

void BlitImage320x200(Graphics* g, int x, int y, Image* image) {
	
	UInt8* memloc = (UInt8*) 0xa0000 + (y*g->width + x);
	UInt8* itrloc = memloc;
	
	register int i;
	register int j;
	int totalImage = image->width*image->height;
	for(i=0, j=0; i<totalImage; i++, j++) {
		*(itrloc++) = *((UInt8*)image->data+i);
		if(j>=image->width) {
			j = 0;
			itrloc += (g->width-image->width);
		}
	}
}

void WritePixel320x200(Graphics* handle, int x, int y, UInt32 pixelData) {
	UInt8* memloc = (UInt8*)0xa0000 + (y*handle->width + x);
	*memloc = (UInt8) pixelData;
}

void WritePixel(Graphics* g, int x, int y, UInt32 pixelData) {
	if(g && g->WritePixel) {
		g->WritePixel(g, x, y, pixelData);
	}
}

void BlitImage(Graphics* g, int x, int y, Image* image) {
	if(g && g->BlitImage) {
		g->BlitImage(g, x, y, image);
	}
}

void SwitchToGraphicsMode() {
	changeTo320x200_Graphics();
	currentMode.WritePixel = WritePixel320x200;
	currentMode.BlitImage = BlitImage320x200;
	currentMode.width = 320;
	currentMode.height = 200;
	currentMode.data = get_fb_loc();
}

Graphics* GetCurrentMode() {
	return &currentMode;
}


