#ifndef BLOCK_DEV_H
#define BLOCK_DEV_H

#include <KOSTypes.h>
#include <dev.h>

// The point of the block device abstraction layer is to
// be able to open the block device like a file
// and read, write, seek, and tell as if it was a file
// on a disk.

// TODO:  Add support for an inode in the VFS structures, so that two identically named files can be told apart.

typedef int (*blockdevice_read_t)(int driveId, UInt64 lba, int sectorCount, void* buffer);
typedef int (*blockdevice_write_t)(int driveId, UInt64 lba, int sectorCount, const void* buffer);
typedef UInt64 (*blockdevice_getblocksnum_t)(int driveId);
typedef UInt64 (*blockdevice_getblocksize_t)(int driveId);

typedef struct {
	blockdevice_read_t bd_read;
	blockdevice_write_t bd_write;
	blockdevice_getblocksnum_t bd_getblocksnum;
	blockdevice_getblocksize_t bd_getblocksize;

	const char* name;
	int driveId;
} BlockDeviceData;

#define CMD_GET_BLOCKSIZE 0x02
#define CMD_READ 0x01
#define CMD_WRITE 0x03

// CMD_GET_BLOCKSIZE:
//	.command = CMD_GET_BLOCKSIZE,
//  .address = 0,
//	.length = 0
//	Gets the blocksize of the device.
// CMD_READ:
//	.command = CMD_READ,
// 	.address = LBA address to read.
//	.length = length of data to read in blocks.
// CMD_WRITE:
//	.command = CMD_WRITE,
//	.address = LBA address to write.
//	.length = length of data to write in blocks.
//	CMD_WRITE does not send a response, but should put
//	the write command into a queue and wait until all data
//	has been sent.

struct BlockDeviceRequest {
	UInt32 command;
	
	UInt64 address;
	UInt64 length;

} __attribute__((packed));

typedef struct BlockDeviceRequest BlockDeviceRequest;

// On error, .status is -(ERRCODE)
// BlockDeviceResponseHeader.length does not include the UInt64 value.
// CMD_GET_BLOCKSIZE:
//	.length = 0, .status = blocksize
// CMD_READ:
//	.length = length of data in bytes, .status = 0 unless error.
// CMD_WRITE:
// .length = 0, .status = 0

struct BlockDeviceResponseHeader {
	UInt32 length;
	Int64 status;
} __attribute__((packed));

typedef struct BlockDeviceResponseHeader BlockDeviceResponseHeader;

int RegisterBlockDevice(BlockDeviceData* dev);

#endif