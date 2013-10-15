#ifndef DRIVERS_H
#define DRIVERS_H

#include <KOSTypes.h>

int DriversInit();

// This function allocates n pages from the physical memory manager
// and returns the allocated virtual address to the module requesting it.
void* AllocateDriverSpace(int numPages);

void KernelSymbolsLoad();

#define DRIVER_SPACE 0xA0000000

#endif