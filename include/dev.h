#ifndef DEV_H
#define DEV_H

#include <vfs.h>
#include <common/arraylist.h>

/*
typedef struct {
	ArrayList* files;
} DevFS_Data;*/ // This is the same as directory data, so we'll just use DirectoryData instead.

typedef struct {
	char* name;

	write_func write;
	read_func read;

	// These two are not strictly necessary.  They were added for block device support.
	seek_func seek;
	tell_func tell;
} DeviceData;

int DevFS_Init();

VFS_Node* RegisterDevice(DeviceData* node);

#endif