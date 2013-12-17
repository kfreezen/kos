#include <block_dev.h>
#include <common/dictionary.h>
#include <dev.h>

Dictionary* blockDevices = NULL;

int RegisterBlockDevice(BlockDeviceData* blockDev) {
	if(!blockDevices) {
		blockDevices = Dict_Create();
	}

	BlockDeviceData* newDev = kalloc(sizeof(BlockDeviceData)); // We don't want some stupid (or malicious) programmer
	// coming along and freeing our essential device data and causing havoc to the system.

	memcpy(newDev, dev, sizeof(BlockDeviceData));
	newDev->name = kalloc(strlen(blockDev->name) + 1);
	strcpy(newDev->name, blockDev->name);

	// Now, since our block device will only be accessed by inode, let's register
	// it in the devfs.
	DeviceData dev;
	dev.read = BlockDevice_Read;
	dev.write = BlockDevice_Write;
	dev.seek = BlockDevice_Seek;
	dev.tell = BlockDevice_Tell;
	dev.name = blockDev->name;

	VFS_Node* node = RegisterDevice(&dev);

	Dict_SetByHash(blockDevices, GetInodeFromNode(), newDev);

}

// For now it's just /dev/null stuff.
int BlockDevice_Write(const void* buf, int len, File* file) {
	return len;
}

int BlockDevice_Read(void* buf, int len, File* file) {

	return len;
}


filePosType BlockDevice_Seek(filePosType newPos, File* file) {
	// So, we're cramming a 64-bit inode into a 32-bit key.
	// That can't really be good, can it?

	UInt64 inode = GetInodeFromNode(GetNodeFromFile(file));
	BlockDeviceData* data = (BlockDeviceData*) Dict_GetFromHash(blockDevices, inode);
	UInt64 blocks = data->bd_getblocksnum(data->driveId);
	UInt64 blockSize = data->bd_getblocksize(data->driveId);

	if(newPos >= blocks * blockSize) {
		newPos = blocks * blockSize - 1;
	}

	return newPos;
}

filePosType BlockDevice_Tell(File* node) {
	return node->filePos;
}