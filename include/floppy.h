#ifndef FLOPPY_H
#define FLOPPY_H

#include <KOSTypes.h>
#include <device.h>

int FloppyInit();
void FloppyLBAToCHS(int lba, int *head, int *track, int *sector);
UInt8* FloppyReadSector(int sector);
int FloppyReadSectorNoAlloc(int lba, void* buffer);

int Floppy_GetError();

Device* FloppyGetDevice();

#define NO_FLOPPY 1
#define FLOPPY_SECTORS_PER_TRACK 18

enum FloppyRegisters {
   STATUS_REGISTER_A                = 0x3F0, // read-only
   STATUS_REGISTER_B                = 0x3F1, // read-only
   DIGITAL_OUTPUT_REGISTER          = 0x3F2,
   TAPE_DRIVE_REGISTER              = 0x3F3,
   MAIN_STATUS_REGISTER             = 0x3F4, // read-only
   DATARATE_SELECT_REGISTER         = 0x3F4, // write-only
   DATA_FIFO                        = 0x3F5,
   DIGITAL_INPUT_REGISTER           = 0x3F7, // read-only
   CONFIGURATION_CONTROL_REGISTER   = 0x3F7  // write-only
};

enum FloppyCommands {
   READ_TRACK =                 2,	// generates IRQ6
   SPECIFY =                    3,      // * set drive parameters
   SENSE_DRIVE_STATUS =         4,
   WRITE_DATA =                 5,      // * write to the disk
   READ_DATA =                  6,      // * read from the disk
   RECALIBRATE =                7,      // * seek to cylinder 0
   SENSE_INTERRUPT =            8,      // * ack IRQ6, get status of last command
   WRITE_DELETED_DATA =         9,
   READ_ID =                    10,	// generates IRQ6
   READ_DELETED_DATA =          12,
   FORMAT_TRACK =               13,     // *
   SEEK =                       15,     // * seek both heads to cylinder X
   VERSION_CMD =                16,	// * used during initialization, once
   SCAN_EQUAL =                 17,
   PERPENDICULAR_MODE =         18,	// * used during initialization, once, maybe
   CONFIGURE =                  19,     // * set controller parameters
   LOCK =                       20,     // * protect controller params from a reset
   VERIFY =                     22,
   SCAN_LOW_OR_EQUAL =          25,
   SCAN_HIGH_OR_EQUAL =         29
};

enum ExtendedFloppyCommands {
	EXT_SKIP = 0x20,
	EXT_DENSITY = 0x40,
	EXT_MULTITRACK = 0x80
};

enum DORMasks {
	DRIVE_0 = 0,
	DRIVE_1 = 1,
	DRIVE_2 = 2,
	DRIVE_3 = 3,
	DOR_MASK_RESET = 4,
	DOR_MASK_DMA = 8,
	DOR_MASK_DRIVE0_MOTOR = 16,
	DOR_MASK_DRIVE1_MOTOR = 32,
	DOR_MASK_DRIVE2_MOTOR = 64,
	DOR_MASK_DRIVE3_MOTOR = 128
};

enum FloppySectorLengths {
	SECTOR_LEN_512=2,
	SECTOR_LEN_1024=4
};

enum FLOPPY_GAP3_LENGTH {
	GAP3_LENGTH_3_5 = 27
};

typedef struct __hts {
	Int32 head, track, sector;
} Hts;

typedef struct __floppy_geom {
	UInt32 spt;
	UInt32 heads;
} FloppyGeometry;

#endif
