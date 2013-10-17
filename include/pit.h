#ifndef PIT_H
#define PIT_H

#include <KOSTypes.h>

void PIT_Init(UInt32 freq);
UInt32 GetTicks();
UInt32 GetSeconds();
void wait(int ticks);

void EnableLoadIndicator(char c, UInt32 interval);
void DisableLoadIndicator();

typedef UInt32 (*PITHook)(UInt32 numTick, UInt32 timeMS);

void InitPITHook(PITHook h);
void DeactivatePITHook();

UInt32 GetSpeed();
#endif
