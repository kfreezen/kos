#include <floppy.h>
#include <isr.h>
#include <dma.h>
#include <kheap.h>
#include <print.h>
#include <pit.h>

int flpy_error = 0;
Bool floppy_done = false;
Bool nec765 = false;
FloppyGeometry geom;
volatile Bool receivedIrq=FALSE;
int currentDrive = 0;


void ResetFloppy();
void FDC_Specify(UInt32 steprate, UInt32 load, UInt32 unload, Bool dma);
int FDC_Calibrate(UInt32);

void FDC_Disable() {
	outb(DIGITAL_OUTPUT_REGISTER, 0);
}

void FDC_Enable() {
	outb(DIGITAL_OUTPUT_REGISTER, DOR_MASK_RESET | DOR_MASK_DMA);
}

static void floppy_irq_handler(Registers regs) {
	receivedIrq=TRUE;
}

void send_command(Byte cmd) {
	int i;
	int timeOut=500;
	for(i=0; i<timeOut; i++) {
		if(inb(MAIN_STATUS_REGISTER)&0x80) {
			outb(DATA_FIFO, cmd);
			return;
		}
	}
}

int read_data() {
	int i;
	for(i=0; i<500; i++) {
		if(inb(MAIN_STATUS_REGISTER)&0x80) {
			return inb(DATA_FIFO);
		}
	}
	
	return -1;
}

void FDC_SenseInterrupt(UInt32* st0, UInt32* cyl) {
	send_command(SENSE_INTERRUPT);
	*st0 = read_data();
	*cyl = read_data();
	
	#ifdef FLOPPY_DEBUG
	kprintf("FDC_SenseInterrupt DataDump Begin\n");
	kprintf("st0=%x\ncyl=%x\n", *st0, *cyl);
	kprintf("FDC_SenseInterrupt DataDump End\n");
	#endif
}

void FloppyLBAToCHS(int lba, int *head, int *track, int *sector) {
	*head = (lba %(FLOPPY_SECTORS_PER_TRACK*2)) / (FLOPPY_SECTORS_PER_TRACK);
	*track = lba / (FLOPPY_SECTORS_PER_TRACK*2);
	*sector = lba%FLOPPY_SECTORS_PER_TRACK+1;
}

int FloppyInit() {
	flpy_error = 0;
	
	registerIntHandler(IRQ6, &floppy_irq_handler);
	
	send_command(VERSION_CMD);
	UInt8 version = read_data();
	kprintf("FloppyVersion=%x\n", version);
	
	if(version!=0x90) {
		return -1;
	}
	
	DMA_Init();
	
	send_command(CONFIGURE);
	send_command(0);
	send_command(0x28); //0b001001000
	send_command(0);
	
	send_command(LOCK|0x80);
	read_data();
	
	ResetFloppy();
	//FDC_Specify(13,1,0xf, TRUE);
	
	return 0;
}

void ResetFloppy() {
	
	receivedIrq = FALSE;
	
	FDC_Disable();
	FDC_Enable();
	
	while(!receivedIrq);
	
	outb(CONFIGURATION_CONTROL_REGISTER,0x00);
	
	//FDC_Specify(3,16,240,TRUE);
	send_command(SPECIFY);
	send_command(0xdf);
	send_command(0x02);
	
	FDC_Calibrate(currentDrive);
}

void FDC_Specify(UInt32 steprate, UInt32 load, UInt32 unload, Bool dma) {
	UInt32 data = 0;
	send_command(SPECIFY);
	data = ( (steprate&0xf) << 4) | (unload&0xf);
	send_command(data);
	data = (load) << 1 | (dma==TRUE) ? 1 : 0;
	send_command(data);
}

inline void FDC_WaitIRQ() {
	int tmo=30;
	
	while(!receivedIrq && tmo>=0) {
		wait(1);
		--tmo;
	}
	
	receivedIrq = FALSE;
}

void FDC_ControlMotor(UInt32 drive, Bool toggle) {

	//! sanity check: invalid drive
	if (drive > 3)
		return;

	UInt32 motor = 0;

	//! select the correct mask based on current drive
	switch (currentDrive) {

		case 0:
			motor = DOR_MASK_DRIVE0_MOTOR;
			break;
		case 1:
			motor = DOR_MASK_DRIVE1_MOTOR;
			break;
		case 2:
			motor = DOR_MASK_DRIVE2_MOTOR;
			break;
		case 3:
			motor = DOR_MASK_DRIVE3_MOTOR;
			break;
	}

	//! turn on or off the motor of that drive
	if (toggle)
		outb(DIGITAL_OUTPUT_REGISTER, (currentDrive | motor | DOR_MASK_RESET | DOR_MASK_DMA));
	else
		outb(DIGITAL_OUTPUT_REGISTER, DOR_MASK_RESET);

	//! in all cases; wait a little bit for the motor to spin up/turn off
	wait (20);
}

int FDC_Calibrate(UInt32 drive) {
	UInt32 st0, cyl;
	
	if(drive>=4) {
		return -2;
	}
	
	// Turn on the motor.
	FDC_ControlMotor(drive, TRUE);
	int i=0;
	for(i=0; i<10; i++) {
		send_command(RECALIBRATE);
		send_command(drive);
		FDC_WaitIRQ();
		FDC_SenseInterrupt(&st0,&cyl);
		
		if(!cyl) {
			FDC_ControlMotor(drive, FALSE);
			return 0;
		}
	}
	
	FDC_ControlMotor(drive, FALSE);
	return -1;
}

int FDC_Seek(UInt32 cyl, UInt32 head) {
	UInt32 st0, cyl0;
	
	if(currentDrive >= 4) {
		return -1;
	}
	
	int i;
	for(i=0; i<10; i++) {
		send_command(SEEK);
		send_command((head)<<2 | currentDrive);
		send_command(cyl);

		kprintf("FDC_WAITIRQ\n");
		
		FDC_WaitIRQ();
		FDC_SenseInterrupt(&st0, &cyl0);
		
		if(cyl0==cyl) {
			return 0;
		}
	}
	
	return -1;
}

void FDC_ReadSectorInternal(UInt8 head, UInt8 track, UInt8 sector) {
	UInt32 st0, cyl;
	
	#ifdef FLOPPY_DEBUG
	kprintf("ReadSectorInternal\n");
	#endif
	
	DMA_PrepareRead();
	
	send_command(READ_DATA | EXT_MULTITRACK | EXT_SKIP | EXT_DENSITY);
	send_command(head<<2 | currentDrive);
	send_command(track);
	send_command(head);
	send_command(sector);
	send_command(SECTOR_LEN_512);
	send_command(2);
	send_command(GAP3_LENGTH_3_5);
	send_command(0xff);
	
	FDC_WaitIRQ();
	
	#undef FLOPPY_DEBUG
	#ifdef FLOPPY_DEBUG
	kprintf("ReadSector DataDump Begin\n");
	#endif
	
	int j=0;
	for(j=0; j<7; j++) {
		#ifndef FLOPPY_DEBUG
		read_data();
		#endif
		
		#ifdef FLOPPY_DEBUG
		kprintf("%x\n", read_data());
		#endif
		
	}
	
	#ifdef FLOPPY_DEBUG
	kprintf("ReadSector DataDump End\n");
	#endif
	#define FLOPPY_DEBUG
	
	FDC_SenseInterrupt(&st0, &cyl);
}

int FloppyReadSectorNoAlloc(int lba, void* buffer) {
	if(currentDrive>=4) {
		return -1;
	}
	
	int head=0, track=0, sector=1;
	FloppyLBAToCHS(lba, &head, &track, &sector);
	
	FDC_ControlMotor(currentDrive, TRUE);
	if(FDC_Seek(track, head)!=0) {
		return -1;
	}

	FDC_ReadSectorInternal(head, track, sector);
	FDC_ControlMotor(currentDrive, FALSE);
	
	memcpy(buffer, DMA_BUFFER, FloppyGetDevice()->sectorSize);
	
	return 0;
}

UInt8* FloppyReadSector(int sectorLBA) {
	UInt8* sectorData = (UInt8*) kalloc(512);
	if(FloppyReadSectorNoAlloc(sectorLBA, sectorData)==-1) {
		kfree(sectorData);
		return NULL;
	}
	return sectorData;
}

void* FloppyGetMediaInfo() {
	int head=0, track=0, sector=1;

	FDC_ControlMotor(currentDrive, TRUE);
	FDC_ReadSectorInternal(head, track, sector);
	FDC_ControlMotor(currentDrive, FALSE);
	
	void* buffer = kalloc(512);
	
	memcpy(buffer, DMA_BUFFER, FloppyGetDevice()->sectorSize);
	return buffer;
}

Device floppy = {
	512,
	FloppyGetMediaInfo,
	FloppyReadSector,
	FloppyReadSectorNoAlloc,
};

Device* FloppyGetDevice() {
	return &floppy;
}
