#ifndef FAT12_H
#define FAT12_H

#include <KOSTypes.h>
#include <vfs.h>
#include <common/arraylist.h>

#define FILE_ATTR_LONG_FILE_NAME 0xF // FIXME:  Get rid of all instances of this, then delete.  Use FAT12_LONG_FILENAME
#define FAT12_LONG_FILENAME 0xF
#define FAT12_ATTR_DIR 0x10

#define FAT12_EOF 0xFF8

#define FAT12_ENTRY_NULL 0x00
#define FAT12_ENTRY_EMPTY 0xE5
#define FAT12_SECTOR_SIZE 512

#include <err.h>
#define ERR_REL_SECTOR_0 (ERR_VFS_DRV_DEFINED_BOTTOM + 0)

struct __DirEntry {
	char name[11];
	Byte attribute;
	Byte reservedZero;
	Byte creationTime;
	UShort msdosTimeCreated;
	UShort msdosDateCreated;
	UShort msdosDateLastAccessed;
	UShort firstCluster_high;
	UShort msdosTimeLastModified;
	UShort msdosDateLastModified;
	UShort firstCluster;
	UInt32 fileSize;
} __attribute__((packed));

struct __LongFileNameEntry {
	UInt8 order;
	wchar_t first5[5];
	UInt8 attribute;
	UInt8 longEntryType;
	UInt8 checksum;
	wchar_t next6[6];
	UInt16 zero;
	wchar_t final2[2];
} __attribute__((packed));

typedef struct __LongFileNameEntry LongFileNameEntry;
typedef struct __DirEntry DirEntry;
typedef struct __Bpb Bpb;

typedef struct {
	Device* linkedDevice;
	Bpb* bpb;
} FAT12_Context;

typedef struct {
	int filePos;
} FAT12_FileData;

typedef struct {
	FAT12_Context* context;
	DirEntry* locationData;
	void* data;
} FAT12_File;

typedef struct {
	int length;
	UInt8* buffer;
} FileBuffer;

int FAT12_Read_LL(FAT12_File* node, UInt32 off, UInt32 length, UInt8* buffer);
FileBuffer FAT12_Read_FB(FAT12_File* node, UInt32 off, UInt32 length);

FAT12_File* FAT12_GetFile(FAT12_Context* context, const char* file);

struct __Bpb {
	Byte jmp[3];
	char oemIdent[8];
	UShort bytesPerSector;
	Byte sectorsPerCluster;
	UShort reservedSectorsNum;
	Byte fatTables;
	UShort directoryEntries;
	UShort totalSectorsShort;
	Byte mediaDescriptor;
	UShort numSectorsPerFat;
	UShort numSectorsPerTrack;
	UShort numHeads;
	UInt32 numHiddenSectors;
	UInt32 totalSectorsLong;
} __attribute__((packed));

// The FAT12 system can have no more than 65536 files.
// The inode then has the (hexadecimal) format of:
// 0xDDDDFFFF
// DDDD:  Context identifier.
// FFFF:  File identifier.

FAT12_Context* FAT12_GetContext(Device* device);

// Lowlevel driver functions for file write support.
int FAT12_GetFreeClusterFromFat(FAT12_Context* context);
int FAT12_GetFreeDirectoryEntry(FAT12_Context* context);
void FAT12_WriteDirectoryEntry(FAT12_Context* context, DirEntry* entry);
void FAT12_WriteCluster(FAT12_Context* context, int cluster, Byte* buffer);

// VFS Functions

/*
Mounts a FAT12 context at a certain path.
*/
int FAT12_Init(FAT12_Context* context, const char* parentPath, const char* mountpointName);

ArrayList* FAT12_ListFiles(VFS_Node* dir);

VFS_Node* FAT12_AddFile(int fileType, const char* name, VFS_Node* parent);
VFS_Node* FAT12_GetNode(VFS_Node* node, const char* name);
int FAT12_LoadDirectory(VFS_Node* dir);
#endif
