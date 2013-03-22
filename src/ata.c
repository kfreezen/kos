#include <ata.h>
#include <pci.h>
#include <common/arraylist.h>
#include <print.h>
#include <kheap.h>

#define ATA_DEBUG

#define PRIMARY 0
#define SECONDARY 1

#define ATA_PRIMARY_BUS_PORT_BASE 0x1F0
#define ATA_SECONDARY_BUS_PORT_BASE 0x170
#define ATA_PRIMARY_DEV_CTRL_PORT 0x3F6
#define ATA_SECONDARY_DEV_CTRL_PORT 0x376

UInt32 port_base[2];
UInt32 secondary_port_base;

UInt32 dev_ctrl[2];
UInt32 secondary_dev_ctrl;

UInt32 last_drive_selected[2];

enum ATA_Ports {
	PIO_DATA, ERR_INFO, SECT_CNT, LBA_LO, LBA_MID, LBA_HI, DRV_SEL, CMD_PORT
};

void ATA_WritePort(int sel, int port_off, UInt8 val) {
	outb(port_base[sel]+port_off, val);
}

void ATA_WriteDeviceControl(int sel, UInt8 val) {
	outb(dev_ctrl[sel], val);
}

UInt8 ATA_ReadPort(int sel, int port_off) {
	return inb(port_base[sel]+port_off);
}

/**
	sel:  Bus number.
	drive:  Raw drive number that the ATA bus expects.  I believe it would either be 0xA0 or 0xB0
**/
void ATA_SelectDrive(int sel, int drive) {
	if(drive!=last_drive_selected[sel]) {
		ATA_WritePort(sel, DRV_SEL, drive);
		
		// 400ns delay to give the drive time to push its new status out to the bus
		ATA_ReadPort(sel, CMD_PORT);
		ATA_ReadPort(sel, CMD_PORT);
		ATA_ReadPort(sel, CMD_PORT);
		ATA_ReadPort(sel, CMD_PORT);
		
		last_drive_selected[sel] = drive;
	}
}

UInt32 ATA_ReadPIO(int sel, void* p_data, int size) {
	Byte* data = (Byte*) p_data;
	
	int itr=0;
	
	while(1) {
		Byte status = ATA_ReadPort(sel, CMD_PORT);
		
		if(status&1) {
			return 0x80000000 | itr;
		}
		
		if( (status & ( (1<<3) | (1<<7) )) == (1<<3) ) {
			if(itr < size) {
				data[itr++] = ATA_ReadPort(sel, PIO_DATA);
			} else {
				break;
			}
		}
	}
	
	return 0;
}

void ATA_Wait() {
	int i;
	for(i=0; i<4; i++) {
		ATA_ReadPort(0, CMD_PORT);
	}
}
IdentifyCommandStruct* ATA_Identify(int sel, int drive) {
	IdentifyCommandStruct* ident = NULL;
	
	ATA_SelectDrive(sel, drive);
	ATA_WritePort(sel, SECT_CNT, 0);
	ATA_WritePort(sel, LBA_LO, 0);
	ATA_WritePort(sel, LBA_MID, 0);
	ATA_WritePort(sel, LBA_HI, 0);
	
	ATA_WritePort(sel, CMD_PORT, IDENTIFY);
	
	ATA_Wait();
	
	Byte status = ATA_ReadPort(sel, CMD_PORT);
	if(status == 0) {
		kprintf("%d:%s Drive does not exist.\n", sel, (drive==0xA0) ? "MASTER" : "SLAVE");
		return NULL;
	}
	
	while((ATA_ReadPort(sel, CMD_PORT)&0x80)==0x80) {
	
	}
	
	Byte lba_mid, lba_hi;
	lba_mid = ATA_ReadPort(sel, LBA_MID);
	lba_hi = ATA_ReadPort(sel, LBA_HI);
	
	if(lba_mid || lba_hi) {
		return NULL;
	}
	
	while(1) {
		status = ATA_ReadPort(sel, CMD_PORT);
		if(status&1) {
			status = 1;
			break;
		} else if(status&8) {
			status = 8;
			break;
		}
	}
	
	if(status != 1) {
		// For now we do polling stuff.
		ident = (IdentifyCommandStruct*) kalloc(sizeof(IdentifyCommandStruct));
		memset(ident, 0, sizeof(IdentifyCommandStruct));
		
		if(ATA_ReadPIO(sel, ident, sizeof(IdentifyCommandStruct))) {
			kprintf("Some kind of error occurred.\n");
			return NULL;
		} else {
			#ifdef ATA_DEBUG
			kprintf("All good!\n");
			#endif
			
			return ident;
		}
	} else {
		kprintf("Status==1. aborting.\n");
	}
	
	return ident;
}

PCIDevice* ATA_FindBus() {
	// The ATA Bus is most likely a  Mass storage controller.  Look for class code 0x01
	PCIDevice* ret = 0;
	ArrayList* devices = FindPCIDevicesWithClassCode(CLASS_MASS_STORAGE_CTRL);
	
	// TODO:  Implement the search.
	
	ALFreeList(devices);
	
	return ret;
}

void ATA_Init() {
	PCIDevice* ret = ATA_FindBus();
	
	if(ret==NULL) {
		kprintf("warning:  No ATA Device detected. Using default ports.\n");
	} else {
		kprintf("The device was detected, however, I'm to lazy to look for port space in it.\n");
	}
	
	port_base[PRIMARY] = ATA_PRIMARY_BUS_PORT_BASE;
	port_base[SECONDARY] = ATA_SECONDARY_BUS_PORT_BASE;
	
	dev_ctrl[PRIMARY] = ATA_PRIMARY_DEV_CTRL_PORT;
	dev_ctrl[SECONDARY] = ATA_SECONDARY_DEV_CTRL_PORT;
	
	if(ATA_ReadPort(PRIMARY, CMD_PORT) == 0xFF) {
		kprintf("No drives are available on the ATA buses.\n");
		return; // 1;
	} else {
		// Do an IDENTIFY Command.
		IdentifyCommandStruct* ident[4];
		ident[0] = ATA_Identify(PRIMARY, 0xA0);
		ident[1] = ATA_Identify(PRIMARY, 0xB0);
		ident[2] = ATA_Identify(SECONDARY, 0xA0);
		ident[3] = ATA_Identify(SECONDARY, 0xB0);
		
		int i;
		for(i=0; i<4; i++) {
			kprintf("[%d]=%x ", i, ident[i]);
		}
		
		kprintf("\n");
	}
		
}
