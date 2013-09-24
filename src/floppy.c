#include <debugdef.h>
#include <floppy.h>
#include <isr.h>
#include <dma.h>
#include <kheap.h>
#include <print.h>
#include <pit.h>
#include <fat12.h>
#include <error.h>

//#define FLOPPY_DEBUG

#define FLOPPY_CACHE_SIZE 128
#define KEEPALIVE_INCREMENT 4

typedef struct {
	Bool used;
	int sectorNum;
	int keepAlive;
	UInt8 sector[512];
} FloppyCacheEntry;

FloppyCacheEntry* floppyCache = NULL;

int flpy_error = 0;
Bool floppy_done = false;
Bool nec765 = false;
FloppyGeometry geom;
volatile Bool receivedIrq=FALSE;
int currentDrive = 0;

int floppy_ticks_motor_disable = 0;

void ResetFloppy();
void FDC_Specify(UInt32 steprate, UInt32 load, UInt32 unload, Bool dma);
int FDC_Calibrate(UInt32);

// This function also increments the keepalive variable.
FloppyCacheEntry* Floppy_FindCacheEntry(int lba) {
	if(floppyCache == NULL) {
		return NULL;
	}

	FloppyCacheEntry* ret = NULL;

	int i;
	for(i=0; i<FLOPPY_CACHE_SIZE; i++) {
		if(floppyCache[i].used) {
			if(floppyCache[i].sectorNum == lba) {
				floppyCache[i].keepAlive += KEEPALIVE_INCREMENT;
				ret = &floppyCache[i];
			} else {
				if(floppyCache[i].keepAlive) {
					floppyCache[i].keepAlive --;
				}
			}
		}
	}

	return ret;
}

int Floppy_AddCacheEntry(int lba, UInt8* sector) {
	if(floppyCache == NULL) {
		return -1;
	}

	int i;
	for(i=0; i<FLOPPY_CACHE_SIZE; i++) {
		if(!floppyCache[i].used || !floppyCache[i].keepAlive) {
			floppyCache[i].used = TRUE;
			floppyCache[i].keepAlive = KEEPALIVE_INCREMENT;
			memcpy(floppyCache[i].sector, sector, 512);
			return 0;
		}
	}

	return -1;
}

void FDC_Disable() {
	outb(DIGITAL_OUTPUT_REGISTER, 0);
}

void FDC_Enable() {
	outb(DIGITAL_OUTPUT_REGISTER, DOR_MASK_RESET | DOR_MASK_DMA);
}

void FloppyMotorCallback() {
	/*if(floppy_ticks_motor_disable) {
		--floppy_ticks_motor_disable;
	} else {
		outb(DIGITAL_OUTPUT_REGISTER, DOR_MASK_RESET);
	}*/
}

static void floppy_irq_handler(Registers regs) {
	receivedIrq=TRUE;
}

int send_command(Byte cmd) {
	int i;
	int timeOut=500;
	for(i=0; i<timeOut; i++) {
		int msr = inb(MAIN_STATUS_REGISTER);
		if((msr&0xc0) == 0x80) {
			outb(DATA_FIFO, cmd);
			return 0;
		}
	}
	
	return 1;
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

#define DO_CMD(func, cmd) \
	if(send_command((cmd))) DoRetry((func))
	
#define NUMREADS_DUMPREG 10

DumpRegResult FDC_DumpReg() {
	DumpRegResult result;
	send_command(DUMPREG);
	
	UInt8* registers = (UInt8*) &result;
	
	int i;
	for(i=0; i<NUMREADS_DUMPREG; i++) {
		registers[i] = read_data();
	}
	
	return result;
}

#define NUMREADS_READID 7
ReadIDResult FDC_ReadID(int head) {
	ReadIDResult result;
	
	UInt8* registers = (UInt8*) &result;
	
	send_command(READ_ID);
	send_command(((head&1)<<2)|currentDrive);
	
	int i;
	for(i=0; i<NUMREADS_READID; i++) {
		registers[i] = read_data();
	}
	
	return result;
}

#define NORMAL_TERMINATION "If you see this there's another bug in the code."
#define ABNORMAL_TERMINATION "Abnormal termination.  Command not successful."
#define INVALID_COMMAND "Invalid command."
#define ABNORMAL_TERMINATION_BY_POLLING "Abnormal termination.  Caused by polling."

char* IC_MESSAGES[] = {NORMAL_TERMINATION, ABNORMAL_TERMINATION, INVALID_COMMAND, ABNORMAL_TERMINATION_BY_POLLING};

#define BIT_GET(a,b) (((a)>>(b))&0x1)

void FDC_SenseInterrupt_Internal(UInt32* st0, UInt32* cyl, int reset) {
	send_command(SENSE_INTERRUPT);
	*st0 = read_data();
	*cyl = read_data();
	
	int equipment_check = BIT_GET(*st0, 4);
	if(equipment_check) {
		kprintf("FDC:  Equipment check failed.\n");
	}
}

void FDC_SenseInterrupt(UInt32* st0, UInt32* cyl) {
	FDC_SenseInterrupt_Internal(st0, cyl, 0);
}


void StartFloppyMotorCountdown() {
	floppy_ticks_motor_disable = 20;
}

void FloppyLBAToCHS(int lba, int *head, int *track, int *sector) {
	*head = (lba %(FLOPPY_SECTORS_PER_TRACK*2)) / (FLOPPY_SECTORS_PER_TRACK);
	*track = lba / (FLOPPY_SECTORS_PER_TRACK*2);
	*sector = lba%FLOPPY_SECTORS_PER_TRACK+1;
}

int FloppyInit() {
	if(floppyCache == NULL) {
		floppyCache = kalloc(sizeof(FloppyCacheEntry)*FLOPPY_CACHE_SIZE);
		memset(floppyCache, 0, sizeof(FloppyCacheEntry)*FLOPPY_CACHE_SIZE);
	}

	flpy_error = 0;
	
	registerIntHandler(IRQ6, &floppy_irq_handler);
	
	send_command(VERSION_CMD);
	UInt8 version = read_data();
	
	#ifdef FLOPPY_DEBUG
	kprintf("FloppyVersion=%x\n", version);
	#endif
	
	if(version!=0x90) {
		kprintf("FloppyVersion != 0x90.  FloppyVersion == %x\n", version);
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

int FDC_Calibrate_Internal(int drive, int reset);

void ResetFloppy() {
	
	receivedIrq = FALSE;
	
	FDC_Disable();
	FDC_Enable();
	
	UInt32 st0, cyl;
	//FDC_SenseInterrupt_Internal(&st0, &cyl, 1);
	
	while(!receivedIrq);
	
	int i;
	for(i=0; i<4; i++) {
		FDC_SenseInterrupt_Internal(&st0, &cyl, 0);
	}
	
	outb(CONFIGURATION_CONTROL_REGISTER,0x00);
	
	//FDC_Specify(3,16,240,TRUE);
	send_command(SPECIFY);
	send_command(0xdf);
	send_command(0x02);
	
	FDC_Calibrate_Internal(currentDrive, 1);
}

// This basically discards the variable stack frame and calls the function again.
extern void DoRetry(void* function);

void FDC_Specify(UInt32 steprate, UInt32 load, UInt32 unload, Bool dma) {
	UInt32 data = 0;

	DO_CMD(FDC_Specify, data);
	
	data = ( (steprate&0xf) << 4) | (unload&0xf);
	
	DO_CMD(FDC_Specify, data);
	
	data = (load) << 1 | (dma==TRUE) ? 1 : 0;
	DO_CMD(FDC_Specify, data);
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
	if (toggle) {
		outb(DIGITAL_OUTPUT_REGISTER, (currentDrive | motor | DOR_MASK_RESET | DOR_MASK_DMA));
		
		if(!floppy_ticks_motor_disable) {
			floppy_ticks_motor_disable = 0;
			
			wait (20);
		} else {
			//floppy_ticks_motor_disable = 0;
			
		}
	} else {
		outb(DIGITAL_OUTPUT_REGISTER, DOR_MASK_RESET);
	}
	
}

int FDC_Calibrate(UInt32 drive) {
	return FDC_Calibrate_Internal(drive, 0);
}

void* FloppyGetMediaInfo();

int maxCyl = 0;

int FDC_Seek(UInt32 cyl, UInt32 head) {
	#ifdef FLOPPY_DEBUG
	kprintf("FDC_Seek(%x, %x) ", cyl, head);
	#endif
	
	UInt32 st0, cyl0;
	
	if(currentDrive >= 4) {
		return -1;
	}
	
	int j;
	for(j=0; j<3; j++) {
	
		int i;
		for(i=0; i<10; i++) {
			send_command(SEEK);
			send_command((head)<<2 | currentDrive);
			send_command(cyl);
		
			FDC_WaitIRQ();
			FDC_SenseInterrupt(&st0, &cyl0);
		
			//if(st0
		
			if(cyl0==cyl) {
				#ifdef FLOPPY_DEBUG
				kprintf("ret 0\n");
				#endif
			
				return 0;
			}
		}
	
	}
	#ifdef FLOPPY_DEBUG
	kprintf("ret -1\n");
	#endif
	
	return -1;
}

void FDC_ReadSectorInternal(UInt8 head, UInt8 track, UInt8 sector) {
	UInt32 st0, cyl;
	
	#ifdef FLOPPY_DEBUG
	kprintf("FDC_ReadSectorInternal(%x, %x, %x)\n", head, track, sector);
	#endif
	
	if(sector==0) {
		kprintf("Error.  sector==0 and should not be.\n");
		
		// We'll need to take this for out.
		for(;;);
	}
	
start_readsector:
	DMA_PrepareRead();
	
	int stat = send_command(READ_DATA | EXT_MULTITRACK | EXT_SKIP | EXT_DENSITY);

	stat+=send_command(head<<2 | currentDrive);
	stat+=send_command(track);
	stat+=send_command(head);
	stat+=send_command(sector);
	stat+=send_command(SECTOR_LEN_512);
	//stat+=send_command(2);
	stat+=send_command( ((sector + 1) >= FLOPPY_SECTORS_PER_TRACK) ? FLOPPY_SECTORS_PER_TRACK : sector + 1);
	stat+=send_command(GAP3_LENGTH_3_5);
	stat+=send_command(0xff);
	
	if(stat!=0) {
		kprintf("failure=%x\n", stat);
		ResetFloppy();
		goto start_readsector;
	}
	
	FDC_WaitIRQ();
	
	// Make sure the RQM byte is set.
	for(;;) {
		Byte status = inb(MAIN_STATUS_REGISTER);
		if(status&0x80) {
			break;
		}
	}
	
	/*Byte st1, st2, end_head_n, end_sct_n, two;
	
	st0 = read_data();
	st1 = read_data();
	st2 = read_data();
	cyl = read_data();
	end_head_n = read_data();
	end_sct_n = read_data();
	two = read_data();*/
	
	#ifdef FLOPPY_DEBUG
	kprintf("st0 == %x\n", st0);
	#endif
	
	int j=0;
	for(j=0; j<7; j++) {
		read_data();
	}
	
	FDC_SenseInterrupt(&st0, &cyl);
}

int FloppyReadSectorNoAlloc(int lba, void* buffer) {
	if(currentDrive>=4) {
		return -1;
	}
	
	// First thing we want to do is look in the cache to see if our lba is in there.
	FloppyCacheEntry* cacheEntry = Floppy_FindCacheEntry(lba);

	if(!cacheEntry) {
		int head=0, track=0, sector=1;
		FloppyLBAToCHS(lba, &head, &track, &sector);
		
		FDC_ControlMotor(currentDrive, TRUE);
		if(FDC_Seek(track, head)!=0) {
			FDC_ControlMotor(currentDrive, FALSE);
			return -1;
		}

		FDC_ReadSectorInternal(head, track, sector);
		#ifdef FLOPPY_DEBUG
		kprintf("fdc.hts=%x,%x,%x\n", head, track, sector);
		#endif
		
		FDC_ControlMotor(currentDrive, FALSE);
	}

	memcpy(buffer, DMA_BUFFER, FloppyGetDevice()->sectorSize);
	if(!cacheEntry) {
		Floppy_AddCacheEntry(lba, buffer);
	}

	#ifdef FLOPPY_DEBUG
	kprintf("flp.memcpy(%x, %x) %x\n", buffer, FloppyGetDevice()->sectorSize, lba);
	#endif
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

int FDC_Calibrate_Internal(int drive, int reset) {
	#ifdef FLOPPY_DEBUG
	kprintf("FDC_Calibrate(%x)\n", drive);
	#endif
	
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
