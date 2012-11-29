#ifndef DMA_H
#define DMA_H

void DMA_Init();
void DMA_PrepareRead();
void DMA_PrepareWrite();

#define DMA_BUFFER (void*)0x1000

#endif
