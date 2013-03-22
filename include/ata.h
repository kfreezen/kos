#ifndef ATA_H
#define ATA_H

#include <pci.h>

PCIDevice* ATA_FindBus();
void ATA_Init();

struct Identify_Command_Struct {
	UInt16 generalConfiguration; // 0 // TODO:  Split this into a struct with bitfields.
	UInt16 unknown0[59]; // 1-59
	UInt32 totalSectors_LBA28; // 60-61
	UInt16 unknown1[21]; // 62-82
	UInt16 word_83; // 83
	UInt16 unknown2[4]; // 84-87
	UInt16 word_88; // 88
	UInt16 unknown3[4]; // 89-92
	UInt16 word_93; // 93
	UInt16 unknown4[6]; // 94-99
	UInt64 totalSectors_LBA48; // 100-103
	UInt16 unknown5[152]; // 104-255
} __attribute__((packed));

typedef struct Identify_Command_Struct IdentifyCommandStruct;

IdentifyCommandStruct* ATA_Identify(int sel, int drive);

#define IDENTIFY 0xEC

#endif
