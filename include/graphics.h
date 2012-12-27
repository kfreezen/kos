#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <KOSTypes.h>

void changeTo320x200_Graphics();

typedef struct {
	int width, height, bpp;
	void* data;
} Image;

struct Graphics;

// int x, int y, UInt32 pixelData
typedef void (*WritePixelFunc)(struct Graphics*, int,int, UInt32);

// int x, int y, Image* image
typedef void (*BlitImageFunc)(struct Graphics*, int,int, Image*);

typedef struct Graphics {
	int width, height;
	UInt32 data;
	WritePixelFunc WritePixel;
	BlitImageFunc BlitImage;
} Graphics;

void WritePixel(Graphics* g, int x, int y, UInt32 pixelData);
void BlitImage(Graphics* g, int x, int y, Image* image);
Graphics* GetCurrentMode();

#endif
