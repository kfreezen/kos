#ifndef CMOS_H
#define CMOS_H

#include <KOSTypes.h>

void SetNMIStatus(Bool disable);

Byte GetRegister(int reg);

#endif
