#ifndef ATA_H
#define ATA_H

#define MASTER_DRIVE 0
#define SLAVE_DRIVE 1

#define PRIMARY_BUS 0
#define SECONDARY_BUS 1
#define THIRD_BUS 2
#define FOURTH_BUS 3

#define PRIMARY_BUS_PORT_BASE 0x1F0
#define SECONDARY_BUS_PORT_BASE 0x170
#define THIRD_BUS_PORT_BASE 0x1E8
#define FOURTH_BUS_PORT_BASE 0x168

#define PRIMARY_DEV_CTRL_PORT 0x3F6
#define SECONDARY_DEV_CTRL_PORT 0x376
#define THIRD_DEV_CTRL_PORT 0x3E6
#define FOURTH_DEV_CTRL_PORT 0x366

#define LBA_BITNUM 6
#define DEV_BITNUM 4

#define IDENTIFY 0xEC

#define NOT_PROPER_DRIVE -1
#define EARG_NULL -2
#define STATUS_ERR -3

struct IdentifyStruct {
	UInt16 generalConfig;
	UInt16 obsolete_1;
	UInt16 specificConfig;
	UInt16 obsolete_3;
	UInt16 retired_4[2]; // 4 - 5
	UInt16 obsolete_6; // 6
	UInt16 reserved_compactflash_7[2]; // 7-8
	UInt16 retired_9; // 9
	char serialNumber[20]; // 10-19
	UInt16 retired_20[2]; // 20-21
	UInt16 obsolete_22; // 22
	char firmwareRevision[8]; // 23-26
	char modelNumber[40]; // 27-46
	UInt16 word_47; // 47; top byte = 0x80, bottom = number
	// of sectors to be transferred per interrupt on
	// READ/WRITE MULTIPLE commands.  if the bottom is 0, it
	// is reserved.
	UInt16 reserved_48; // 48
	UInt32 capabilities; // 49-50
	UInt32 obsolete_51; // 51-52
	UInt16 word_53; // bit 1 indicates whether fields reported
	// in word 88 are valid.  bit 2 indicates whether fields
	// reported in words 70:64 are valid.
	UInt16 obsolete_54[5]; // 54-58
	UInt16 word_59; // 59; 15-9 reserved.  8 indicates whether
	// multiple sector setting is valid.
	// 7-0 is current setting for number of sectors that shall
	// be transferred per interrupt on R/W multiple command.
	UInt32 totalNumberOfSectors; // 60-61
	UInt16 obsolete_62; // 62
	UInt16 word_63; // 63; see docs for details.
	UInt16 word_64; // 64; see docs for details.
	UInt16 word_65; // 65; see docs for details.
	UInt16 word_66; // 66; see docs for details.
	UInt16 word_67; // 67; see docs for details.
	UInt16 word_68; // 68; see docs for details.
	UInt32 reserved_69; // 69-70
	UInt64 reserved_71; // 71-74; reserved for
	// IDENTIFY PACKET DEVICE command.
	
	UInt16 queueDepth; // 75; 15-5 reserved.  4-0 max queue
	// depth minus 1.

	UInt64 reserved_76; // 76-79.  Reserved for serial ATA.
	UInt16 versionBits; // 80; bit 7 corresponds to ATA-7
	// specs, bit 8 corresponds to ATA-8 specs, etc.
	UInt16 minorVersionNum; // 81; see docs for details.
	UInt16 supportedCommandSet; // 82; see docs for details.
	UInt16 words_83_to_87[5]; // 83-87; see docs.
	UInt16 word_88; // 88.  Something to do with Ultra DMA.
	UInt16 word_89; // 89.  See documents for details.
	UInt16 word_90; // 90.  See documents for details.
	UInt16 word_91; // 91.  See documents for details.
	UInt16 word_92; // 92.  See documents for details.
	UInt16 word_93; // 93.  Hardware reset result.
	UInt16 words_94_to_99[6]; // 94-99;  See documents for details.
	UInt64 maxLBA48Address; // 100-103

	UInt16 wordsLeft[152];
} __attribute__((packed));

typedef struct IdentifyStruct IdentifyStruct;

#endif