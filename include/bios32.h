#ifndef BIOS32_H
#define BIOS32_H

#include <KOSTypes.h>

#define BIOS32_MAGIC 0x5F32335F

Bool FindBios32();

struct Bios32 {
	UInt32 magic; // equal to "_32_" or "_23_" in little endian format.
	UInt32 entry;
	UInt8 revisionLevel;
	UInt8 length;
	UInt8 checksum;
	UInt8 reserved[5];
} __attribute__((packed));

typedef struct Bios32 Bios32;

Bios32* GetBios32();

typedef struct {
	UInt32 retCode;
	UInt32 physAddr;
	UInt32 serviceLength;
	UInt32 serviceEntrypoint;
} Bios32Service;

extern Bios32Service* DetectService(UInt32 id, UInt32 funcSelector);

#endif
