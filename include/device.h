#ifndef DEVICE_H
#define DEVICE_H

typedef UInt8* (*readSectorCaller)(int); // lba
typedef int (*readSectorNoAllocCaller)(int, void*, int); // lba,buffer,max
typedef void* (*getDeviceInfoCaller)();

typedef struct Device {
	int sectorSize;
	getDeviceInfoCaller getDeviceInfo;
	readSectorCaller readSector;
	readSectorNoAllocCaller readSectorNoAlloc;
} Device;

#endif
