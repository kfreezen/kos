#include <device.h>
#include <ata.h>
#include <pci.h>
#include <common/arraylist.h>
#include <print.h>
#include <kheap.h>
#include <isr.h>
#include <pic.h>

#define ATA_DEBUG

#define MAX_BUSES 2

UInt32 portBase[MAX_BUSES];
UInt32 secondary_port_base;

UInt32 devCtrl[MAX_BUSES];
UInt32 secondary_dev_ctrl;

UInt32 last_drive_selected[MAX_BUSES];

IdentifyStruct* driveInfo[MAX_BUSES][2]; // [bus][drive]

enum ATA_PORTS {
	DATA, ERROR, SECTOR_COUNT, LBA_LO, LBA_MID, LBA_HI, DRIVE, COMMAND
};

enum STATUS_BITS {
	ERR_BIT = 0,
	DRQ_BIT = (1<<3),
	SRV_BIT = (1<<4), // Overlapped mode service request (?)
	DF_BIT=(1<<5), RDY_BIT=(1<<6), // DF_BIT:  Drive fault:  does not set ERR.
	BSY_BIT=(1<<7) // Indicates the drive is preparing to send or receive data.
};

enum CONTROL_REG_BITS {
	nIEN_BIT = (1<<1), // Set this to stop the current device from
	// sending interrupts.

	SRST_BIT = (1<<2), // Set this to do a "Software Reset" on all
	//ATA drives on a bus, if one is misbehaving.

	HOB_BIT = (1<<7), // Set this to read back the high order byte
	// of the last LBA48 value sent to an IO port.
};

static inline int validateBusSelect(int busSelect) {
	if(busSelect > SECONDARY_BUS) {
		// We don't support third and fourth right now.
		return SECONDARY_BUS;
	} else {
		return busSelect;
	}
}

UInt8 ATA_ReadPort8(int busSelect, int port) {
	return inb(portBase[validateBusSelect(busSelect)]+port);	
}

UInt8 ATA_ReadAlternateStatus(int busSelect) {
	return inb(devCtrl[validateBusSelect(busSelect)]);
}

UInt16 ATA_ReadPort16(int busSelect, int port) {
	return inw(portBase[validateBusSelect(busSelect)]+port);
}

void ATA_WritePort8(int busSelect, int port, UInt8 value) {
	return outb(portBase[validateBusSelect(busSelect)]+port, value);
}

UInt8 ATA_ReadStatus(int busSelect) {
	ATA_ReadPort8(busSelect, COMMAND);
	ATA_ReadPort8(busSelect, COMMAND);
	ATA_ReadPort8(busSelect, COMMAND);
	ATA_ReadPort8(busSelect, COMMAND);

	return ATA_ReadPort8(busSelect, COMMAND);
}

int ATA_IrqTriggered[MAX_BUSES];

void ATA_WaitIRQ(int busSelect, int drive) {
	if(busSelect >= MAX_BUSES) {
		busSelect = MAX_BUSES - 1;
	} else if(busSelect < 0) {
		busSelect = 0;
	}

	drive &= 1;

	while(!ATA_IrqTriggered[busSelect]) {

	}
}

void ATA_ClearIrqTriggered(int busSelect, int drive) {
	if(busSelect >= MAX_BUSES) {
		busSelect = MAX_BUSES - 1;
	} else if(busSelect < 0) {
		busSelect = 0;
	}

	drive &= 1;

	ATA_IrqTriggered[busSelect] = 0;
}

void ATA_IrqHandler(Registers regs) {
	// TODO:  Implement IRQ sharing stuff.
	if(regs.int_no == IRQ(14)) {
		ATA_IrqTriggered[PRIMARY_BUS] = 1;
	} else {
		ATA_IrqTriggered[SECONDARY_BUS] = 1;
	}
}

int ATA_Identify(IdentifyStruct* ident, int busSelect, int drive) {
	// Now just a note, the osdev wiki says to set bits 7 and 5 (0xA0), but
	// the ATA7 spec states that those bits should be ignored by the device.
	UInt8 drivePortValue = ((drive & 1) << DEV_BITNUM);

	ATA_WritePort8(busSelect, SECTOR_COUNT, 0);
	ATA_WritePort8(busSelect, LBA_LO, 0);
	ATA_WritePort8(busSelect, LBA_MID, 0);
	ATA_WritePort8(busSelect, LBA_HI, 0);

	ATA_WritePort8(busSelect, DRIVE, drivePortValue);
	// Now let's wait for DRQ and BSY to set or something.

	while(ATA_ReadStatus(busSelect) & BSY_BIT) {
		// I don't know what value DRQ needs to be
		// so we'll just do BSY_BIT.
		// FIXME
	}

	ATA_WritePort8(busSelect, COMMAND, IDENTIFY);

	if(ATA_ReadStatus(busSelect) == 0) {

	} else {
		while(ATA_ReadStatus(busSelect) & BSY_BIT) {
			// Wait.
		}
	}

	//ATA_ClearIrqTriggered(busSelect, drive);
	//int status = ATA_ReadStatus(busSelect);

	// Make sure that this drive is not an ATAPI drive.
	UInt8 signature[5];

	signature[0] = ATA_ReadPort8(busSelect, SECTOR_COUNT);
	signature[1] = ATA_ReadPort8(busSelect, LBA_LO);
	signature[2] = ATA_ReadPort8(busSelect, LBA_MID);
	signature[3] = ATA_ReadPort8(busSelect, LBA_HI);
	signature[4] = ATA_ReadPort8(busSelect, DRIVE);

	if(signature[2] != 0 && signature[3] != 0) {
		int i;
		for(i=0; i<5; i++) {
			kprintf("sig %x\n", signature[i]);
		}

		return NOT_PROPER_DRIVE;
	}

	while(1) {
		int status = ATA_ReadStatus(busSelect);
		if(status & DRQ_BIT) {
			break;
		} else if(status & ERR_BIT) {
			return STATUS_ERR;
		}
	}

	UInt16* buffer = (UInt16*) ident;

	if(ident == NULL) {
		buffer = kalloc(sizeof(IdentifyStruct));
	}
	//IdentifyStruct* ident = kalloc(sizeof(IdentifyStruct));

	int i;
	for(i=0; i<(sizeof(IdentifyStruct)/2); i++) {
		buffer[i] = ATA_ReadPort16(busSelect, DATA);
	}

	if(ident == NULL) {
		kfree(buffer);
		return EARG_NULL;
	}
	return 0;
}

int ATA_Read(int busSelect, int drive, UInt64 lba, int sectorCount, void* buffer) {
	// TODO:  We need to cache an IdentifyStruct
	// for each detected drive, so that we can determine
	// which LBA system to use.

	int internalSectorCount = (sectorCount >= 65536) ? 0 : sectorCount;
	sectorCount = (sectorCount >= 65536) ? 65536 : sectorCount;

	UInt8 lbaBytes[6];
	lbaBytes[0] = lba & 0xFF;
	lbaBytes[1] = (lba >> 8) & 0xFF;
	lbaBytes[2] = (lba >> 16) & 0xFF;
	lbaBytes[3] = (lba >> 24) & 0xFF;
	lbaBytes[4] = (lba >> 32) & 0xFF;
	lbaBytes[5] = (lba >> 40) & 0xFF;

	UInt8 sctCountBytes[2];
	sctCountBytes[0] = internalSectorCount & 0xFF;
	sctCountBytes[1] = (internalSectorCount >> 8) & 0xFF;

	int drivePortValue = (1 << LBA_BITNUM) | (drive << DEV_BITNUM);

	ATA_WritePort8(busSelect, DRIVE, drivePortValue&0xFF);

	ATA_WritePort8(busSelect, SECTOR_COUNT, sctCountBytes[1]);
	ATA_WritePort8(busSelect, LBA_HI, lbaBytes[5]);
	ATA_WritePort8(busSelect, LBA_MID, lbaBytes[4]);
	ATA_WritePort8(busSelect, LBA_LO, lbaBytes[3]);

	ATA_WritePort8(busSelect, SECTOR_COUNT, sctCountBytes[0]);
	ATA_WritePort8(busSelect, LBA_HI, lbaBytes[2]);
	ATA_WritePort8(busSelect, LBA_MID, lbaBytes[1]);
	ATA_WritePort8(busSelect, LBA_LO, lbaBytes[0]);

	ATA_WritePort8(busSelect, COMMAND, READ_SECTORS_EXT);

	int bufferItr = 0;
	UInt16* buf = (UInt16*) buffer;
	int i;
	for(i=0; i < sectorCount; i++) {
		ATA_WaitIRQ(busSelect, drive);

		kprintf("ATA_Read IRQ done\n");

		ATA_ClearIrqTriggered(busSelect, drive);

		// Read stuff into buffer.
		for(; bufferItr<256; bufferItr++) {
			buf[bufferItr] = ATA_ReadPort16(busSelect, DATA);
		}
	}

	return 0;
}

void ATA_Init() {
	// TODO:  Add ATA controller detection code.

	// TODO:  Make sure that buses are available and assign
	// port bases as such.

	portBase[PRIMARY_BUS] = PRIMARY_BUS_PORT_BASE;
	portBase[SECONDARY_BUS] = SECONDARY_BUS_PORT_BASE;
	devCtrl[PRIMARY_BUS] = PRIMARY_DEV_CTRL_PORT;
	devCtrl[SECONDARY_BUS] = SECONDARY_DEV_CTRL_PORT;

	registerIntHandler(IRQ(14), ATA_IrqHandler);

	int busItr;
	int driveItr;
	for(busItr = 0; busItr < MAX_BUSES; busItr++) {
		for(driveItr = 0; driveItr < 2; driveItr++) {
			IdentifyStruct* ident = kalloc(sizeof(IdentifyStruct));

			int status = ATA_Identify(ident, busItr, driveItr);
			if(status == 0) {
				driveInfo[busItr][driveItr] = ident;
			} else {
				kfree(ident);
				driveInfo[busItr][driveItr] = NULL;
			}
		}
	}

	// TODO:  Implement a block device abstraction layer.

	UInt8* buffer = kalloc(1024);
	ATA_Read(PRIMARY_BUS, 0, 0, 1, buffer);
	kprintf("%x\n", buffer);
}
