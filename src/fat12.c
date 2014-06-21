#include <fat12.h>
#include <floppy.h>
#include <kheap.h>
#include <print.h>
#include <debugdef.h>
#include <common/arraylist.h>
#include <err.h>

//#define FAT12_DEBUG
//#define FAT12_DEBUG_VERBOSE

#define FAT12_CONTEXTS_NUM 32
FAT12_Context* FAT12_Contexts[FAT12_CONTEXTS_NUM];
int nextContextToAdd = 0;

// This is where the root directory is stored.
inline int getFirstDataSector(Bpb* bpb) {
	return bpb->reservedSectorsNum + (bpb->fatTables*bpb->numSectorsPerFat);
}

// This is the absolute cluster after the first data sector
inline int getAbsoluteCluster(int relSector, Bpb* bpb) {
	return relSector - 2 + getFirstDataSector(bpb);
}

// This is the LBA representation of the sector to be read or written for the specified relative sector LBA.
inline int getAbsoluteSector(int relSector, Bpb* bpb) {
	return getAbsoluteCluster(relSector, bpb) + (bpb->directoryEntries*32/bpb->bytesPerSector);
}
// These inline functions here and their usages are confusing.
// We should use getAbsoluteSector() to get the sector at which the data is stored on the floppy.

// But, to make it more confusing, we should NOT use getAbsoluteSector() if we are accessing the root
// directory, but instead we should use getFirstDataSector() and use that to calculate our sector to
// read or write.

Byte fatBuffer[1024];
int fatSectorInBuffer=-1;

FAT12_Context* FAT12_GetContext(Device* device) {
	int i;
	for(i=0; i<FAT12_CONTEXTS_NUM; i++) {
		if(FAT12_Contexts[i]->linkedDevice==device) {
			return FAT12_Contexts[i];
		}
	}

	if(nextContextToAdd>=FAT12_CONTEXTS_NUM) {
		kprintf("FAT12 Error:  No more contexts supported.\n");
		return NULL;
	}
	
	FAT12_Context* context = kalloc(sizeof(FAT12_Context));
	context->linkedDevice = device;
	context->bpb = device->getDeviceInfo();
	
	FAT12_Contexts[nextContextToAdd++] = context;
	
	return context;
}

// TODO:  Use a caching system similar to this in the floppy driver.
// Have a keepAlive integer that is decremented every time the cache entry is not used, and added to every time the
// cache entry is used.
typedef struct {
	Bool used;
	UInt16 sectorNum;
	UInt8 sector[FAT12_SECTOR_SIZE];
} FAT12_CacheEntry;

#define CACHE_ENTRIES 32
FAT12_CacheEntry* cache = NULL;

UInt16 FAT12_GetClusterFromFAT(FAT12_Context* context, int prevCluster) {
	if(cache == NULL) {
		cache = kalloc(sizeof(FAT12_CacheEntry)*CACHE_ENTRIES);
		memset(cache, 0, sizeof(FAT12_CacheEntry)*CACHE_ENTRIES);
	}

	Bpb* bpb = context->bpb;
	int fatOffset = prevCluster + (prevCluster / 2);

	int fatSector = bpb->reservedSectorsNum+(fatOffset/bpb->bytesPerSector);
	
	#ifdef FAT12_DEBUG_VERBOSE
	kprintf("fatSector = %x, fatOffset = %x, prevCluster=%x\n", fatSector, fatOffset, prevCluster);
	#endif

	int entryOffset = fatOffset%bpb->bytesPerSector;
	
	if(fatSectorInBuffer!=fatSector) {
		FloppyReadSectorNoAlloc(fatSector, fatBuffer, FAT12_SECTOR_SIZE);
		fatSectorInBuffer = fatSector;
	}
	
	if(entryOffset >= bpb->bytesPerSector) {
		FloppyReadSectorNoAlloc(fatSector+1, fatBuffer+bpb->bytesPerSector, FAT12_SECTOR_SIZE);
	}

	UInt16 table_value = *((UInt16*)(&fatBuffer[entryOffset]));
	
	if(prevCluster&0x0001) {
		table_value = table_value >> 4;
	} else {
		table_value = table_value & 0x0FFF;
	}
	
	#ifdef FAT12_DEBUG_VERBOSE
	kprintf("table_value=%x\n", table_value);
	#endif

	return table_value;
}

FAT12_File* FAT12_GetFile(FAT12_Context* context, const char* file) {
	#ifdef FAT12_DEBUG
	char space = ' ';
	#ifdef FAT12_DEBUG_VERBOSE
		space = '\n';
	#endif
	
	kprintf("FAT12_GetFile(%x,%s)%c", context, file, space);
	#endif
	
	Byte* rootBuffer = kalloc(FAT12_SECTOR_SIZE);

	Bpb* bpb = context->bpb;
	
	// This is the root directory's first data sector
	int rootDir = getFirstDataSector(bpb);
	
	DirEntry* entries = (DirEntry*) rootBuffer;
	
	wchar_t lfnameBuffer[256];
	char lfnameBuffer2[256];
	int len;
	Bool doneWithName = false;
	FAT12_File* ret = NULL;

	int i=0;
	for(i=0; i<bpb->directoryEntries; i++) {
		
		if(!(i%32)) {
			FloppyReadSectorNoAlloc(rootDir, rootBuffer, FAT12_SECTOR_SIZE);
			rootDir++;
		}
		
		if(entries[i].name[0] == FAT12_ENTRY_NULL || entries[i].name[0] == FAT12_ENTRY_EMPTY) {
			continue;
		}

		if(entries[i].attribute==0xF) {
			LongFileNameEntry* lfnEntry = (LongFileNameEntry*) &entries[i];
			memcpy(&lfnameBuffer[((lfnEntry->order&0xF)-1)*13], &lfnEntry->first5, sizeof(wchar_t)*5);
			memcpy(&lfnameBuffer[((lfnEntry->order&0xF)-1)*13+5], &lfnEntry->next6, sizeof(wchar_t)*6);
			memcpy(&lfnameBuffer[((lfnEntry->order&0xF)-1)*13+11], &lfnEntry->final2, sizeof(wchar_t)*2);
			len+=13;
			
			if(lfnEntry->order&0xF0) {
				// Done.
				
				// OK, this converts the 2-byte character lfnameBuffer into a 1-byte character buffer.
				int j;
				for(j=0; lfnameBuffer[j]!=0; j++) {
					lfnameBuffer2[j] = (char) lfnameBuffer[j];
				}
				
				lfnameBuffer2[j] = 0;
				
				// So now we need to check to see if this is the long file name entry.
				if(!strcmp(lfnameBuffer2, file)) {
					// If it checks out, we want to load it.
					#ifdef FAT12_DEBUG
					kprintf("lfnameBuffer2=%s\n", lfnameBuffer2);
					#endif
					
					doneWithName = true;
					i++;
				}
				
				#ifdef FAT12_DEBUG
				kprintf("lfname=%s\n", lfnameBuffer2);
				#endif
				
				//memset(lfnameBuffer, 0, 256);
			}
		}

		#ifdef FAT12_DEBUG_VERBOSE
		
		char name_debug[12];
		strncpy(name_debug, 11, entries[i].name);
		name_debug[11]=0;
		
		kprintf("name_debug=%s, attr=%x\n", name_debug, entries[i].attribute);
		#endif

		// Here we have two checks for the 8.3 or for the long file name.
		if(strncmp(file, 11, entries[i].name)==0 || (doneWithName && strcmp(file, lfnameBuffer2)==0)) {
			FAT12_File* fat12_file = (FAT12_File*)kalloc(sizeof(FAT12_File));
			fat12_file->context = context;

			fat12_file->locationData = kalloc(sizeof(DirEntry));
			memcpy(fat12_file->locationData, &entries[i], sizeof(DirEntry));
			
			#ifdef FAT12_DEBUG
			kprintf("ret %x\n", fat12_file);
			#endif
			
			ret = fat12_file;
			break;
		}
	}
	
	#ifdef FAT12_DEBUG
	kprintf("ret NULL\n");
	#endif
	
	kfree(rootBuffer);
	return ret;
}

Byte readSectorBuffer[512];

/**
Returns:	1 if the last sector read was the last sector in the chain.
			-1 if FAT12_Read_LL had some weird unexpected stuff happen.
			0 if FAT12_Read_LL had everything go well.
**/
int FAT12_Read_LL(FAT12_File* node, UInt32 offset, UInt32 length, UInt8* buffer) {
	if(node == NULL) {
		SetErr(ERR_INVALID_ARG);
		return -1;
	}
	
	#ifdef FAT12_DEBUG
	kprintf("FAT12_Read_LL(%x, %x, %x, %x)\n", node, offset, length, buffer);
	#endif
	
	if(offset >= node->locationData->fileSize) {
		SetErr(ERR_EOF_ENCOUNTERED);
		return -1;
	}

	// Let's make sure our offset into the file has a 512 byte granularity.
	offset &= ~(FAT12_SECTOR_SIZE-1);
	
	if(node->locationData->attribute==FAT12_ATTR_DIR) {
		SetErr(ERR_FILE_EXPECTED);
		return -1;
	}
	
	if(length==0) {
		length = node->locationData->fileSize;
	}
	
	//int size = (node->locationData->fileSize&~0x1FF)+0x200;
	
	FAT12_Context* context = node->context;
	DirEntry* entry = (DirEntry*) node->locationData;
	
	UInt16 relSector = entry->firstCluster;
	int sectorOfFileToRead = offset / FAT12_SECTOR_SIZE;
	int i;
	for(i=0; i<sectorOfFileToRead; i++) {
		/*if(relSector == 0) {
			SetErr(ERR_NULL_VALUE_ENCOUNTERED);
			return -1;
		}*/

		relSector = FAT12_GetClusterFromFAT(context, relSector);

		if(relSector >= FAT12_EOF) {
			SetErr(ERR_EOF_ENCOUNTERED);
			return -1;
		} else if(relSector == 0) {
			SetErr(ERR_REL_SECTOR_0);
			return -1;
		}
	}

	int numToRead = length/FAT12_SECTOR_SIZE;
	if(numToRead==0) {
		numToRead = 1;
	}
	
	for(i=0; i<numToRead; i++) {
		// Error checking.
		if(relSector >= FAT12_EOF) {
			SetErr(ERR_EOF_ENCOUNTERED);
			return -1;

		} else if(relSector == 0x00) {
			#ifdef FAT12_DEBUG
			kprintf("relSector==0, breaking\n");
			#endif
			
			SetErr(ERR_REL_SECTOR_0);
			return -1;
		}
		
		int absSector = getAbsoluteSector(relSector, context->bpb);
		
		#ifdef FAT12_DEBUG
		kprintf("reading %x,%x,rel=%x\n", &buffer[i*512], absSector, relSector);
		#endif

		FloppyReadSectorNoAlloc(absSector, readSectorBuffer, FAT12_SECTOR_SIZE);

		memcpy(&buffer[i*512], readSectorBuffer, 512);

		relSector = FAT12_GetClusterFromFAT(context, relSector);
		
		//Commented out because this appears to be redundant.
		//Besides, the loop will take care of returning.
		/*
		if(relSector>=FAT12_EOF) {
			return 0;
		}*/
	}
	
	#ifdef FAT12_DEBUG_VERBOSE
	kprintf(" ret\n");
	#endif

	SetErr(SUCCESS);
	return 0;
}

// TODO:  When FAT12_Read_LL is improved, make sure this function is changed to take 'length' at face value.
FileBuffer FAT12_Read_FB(FAT12_File* node, UInt32 off, UInt32 length) {
	#ifdef FAT12_DEBUG
	kprintf("FAT12_Read_FB(%x, %x, %x)\n", node, off, length);
	#endif
	
	if(node == NULL) {
		FileBuffer fb;
		fb.length = 0;
		fb.buffer = NULL;
		return fb;
	}
	
	FileBuffer fb;
	fb.length = node->locationData->fileSize;
	
	int len = fb.length;
	if(len&0x1ff) {
		len = len&~0x1ff;
		len+=0x200;
	}
	
	fb.buffer = kalloc(len);
	FAT12_Read_LL(node, off, len, fb.buffer);
	
	return fb;
}

// VFS Functions

// TODO:  Create a VFS tell function to tell us (lol) where the file position is.
filePosType FAT12_Tell(File* file) {
	if(isdir(GetNodeFromFile(file))) {
		SetErr(ERR_FILE_EXPECTED);
		return -1;
	}
	
	SetErr(SUCCESS);
	return file->filePos;
}

filePosType FAT12_Seek(filePosType newPos, File* file) {
	if(file==NULL) {
		SetErr(ERR_INVALID_ARG);
		return -1;
	}

	VFS_Node* node = GetNodeFromFile(file);

	if(!node || node->data == NULL) {
		SetErr(ERR_INVALID_ARG);
		return -1;
	}

	FAT12_File* fat12File = (FAT12_File*) node->data;
	
	if(isdir(node)) {
		SetErr(ERR_FILE_EXPECTED);
		return -1;
	}

	if(fat12File->locationData == NULL) {
		SetErr(ERR_NULL_VALUE_ENCOUNTERED);
		return -1;
	}

	if(newPos < 0) {
		SetErr(ERR_INVALID_ARG);
		return -1;
	}

	// There should be a function to get fileSize in the VFS.
	if(newPos > fat12File->locationData->fileSize) {
		newPos = fat12File->locationData->fileSize;
		if(!fat12File->locationData) {
			kprintf("Location Data is null.\n");
		} 

		#ifdef FAT12_DEBUG
		else {
			kprintf("Location data %s == %x.\n", node->name, fat12File->locationData);
		}
		kprintf("newPos = %d\n", newPos);
		#endif
		
	}

	file->filePos = newPos;

	SetErr(SUCCESS);
	return newPos;
}

int FAT12_VFSRead(void* _buf, int len, File* vfsFile) {
	int bufPos = 0;

	VFS_Node* node = GetNodeFromFile(vfsFile);

	char* buf = (char*) _buf;

	FAT12_File* file = (FAT12_File*) node->data;
	int filePos = vfsFile->filePos;

	//int startClusterPos = filePos / FAT12_SECTOR_SIZE;
	//int cluster = file->locationData->firstCluster;

	UInt8* readBuffer = kalloc(FAT12_SECTOR_SIZE);

	int totalRead = 0;

	while(totalRead < len) {

		// filePos&(~FAT12_SECTOR_SIZE) is the file position that it starts from.
		// filePos%FAT12_SECTOR_SIZE is because the buffer only loads 512 bytes at a time, and
		// readBuffer[513] would not be gotten.  "would not be get"?  Seriously, I must have been tired.
		int toRead = FAT12_SECTOR_SIZE - (filePos % FAT12_SECTOR_SIZE);
		
		if(toRead+totalRead > len) {
			toRead = len - totalRead;
		}

		// Let's read the sector that filePos is in
		if(FAT12_Read_LL(file, filePos, FAT12_SECTOR_SIZE, readBuffer)==-1) {
			// We can't really handle any of the errors here so just exit the loop
			#ifdef FAT12_DEBUG
			kprintf("FAT12_Read_LL err %d\n", GetErr());
			#endif

			break;
		}

		int fileBufPos = filePos % FAT12_SECTOR_SIZE;

		memcpy(&buf[bufPos], &readBuffer[fileBufPos], toRead);
		totalRead += toRead;
		bufPos += toRead;
		filePos += toRead;
		vfsFile->filePos = filePos;
	}

	kfree(readBuffer);
	return totalRead;
}

int FAT12_Init(FAT12_Context* context, const char* parentPath, const char* mountpointName) {
	// Get our parent from parentPath
	File* parent = GetFileFromPath(parentPath);
	if(parent == NULL) {
		kprintf("Error in fat12:  parent==NULL, parentPath=%s\n", parentPath);
	}

	VFS_Node* mount = AddFile(FILE_DIRECTORY, mountpointName, GetNodeFromFile(parent));
	FAT12_File* dir = kalloc(sizeof(FAT12_File));
	dir->data = ALCreate();
	dir->context = context;

	CreateMountPoint(mount, dir,
		FAT12_AddFile, FAT12_LoadDirectory,
		FAT12_GetNode, FAT12_ListFiles,
		NULL/*FAT12_VFSWrite*/, FAT12_VFSRead,
		FAT12_Seek, FAT12_Tell
	);

	return 0;
}

ArrayList* FAT12_ListFiles(VFS_Node* dir) {
	if(dir == NULL) {
		return NULL;
	}
	
	LoadDirectory(dir);

	FAT12_File* dirData = (FAT12_File*) dir->data;
	ArrayList* TYPE(VFS_Node*) retList = ALCopy((ArrayList*)dirData->data);

	return retList;
}

VFS_Node* FAT12_GetNode(VFS_Node* dir, const char* name) {
	#ifdef FAT12_DEBUG
	kprintf("FAT12_GetNode(%x, %s)\n", dir, name);
	#endif

	if(dir == NULL) {
		kprintf("FAT12_GetNode:  dir==null\n");
		return NULL;
	}

	if(!isdir(dir)) {
		kprintf("FAT12_GetNode:  dir is not a directory\n");
		return NULL;
	}

	LoadDirectory(dir);

	// Load the directory's arraylist
	FAT12_File* dirData = (FAT12_File*) dir->data;

	ArrayList* TYPE(VFS_Node*) files = (ArrayList*) dirData->data;
	
	kprintf("files: %x\n", files);

	ALIterator* itr = ALGetItr(files);

	while(ALItrHasNext(itr)) {
		VFS_Node* node = ALItrNext(itr);

		#ifdef FAT12_DEBUG
		kprintf("\"%s\": \"%s\"\n", name, node->name);
		#endif

		if(!strcmp(name, node->name)) {
			return node;
		}
	}

	return NULL;
}

VFS_Node* FAT12_AddFile(int fileType, const char* name, VFS_Node* parent) {
	if(isdir(parent)) {
		FAT12_File* dir = (FAT12_File*) parent->data;
		ArrayList* TYPE(VFS_Node*) dirFiles = (ArrayList*) dir->data;

		VFS_Node* node = kalloc(sizeof(VFS_Node));
		node->fileType = fileType;
		strcpy(node->name, name);

		FAT12_File* file = kalloc(sizeof(FAT12_File));

		if(isdir(node)) {
			file->data = ALCreate();

			node->addfile = parent->addfile;
			node->dirload = parent->dirload;
			node->listfiles = parent->listfiles;
			node->getnode = parent->getnode;
			node->write = parent->write;
			node->read = parent->read;
			node->seek = parent->seek;
			node->tell = parent->tell;
		} else {
			FAT12_FileData* fileData = kalloc(sizeof(FAT12_FileData));
			memset(fileData, 0, sizeof(FAT12_FileData));
			fileData->filePos = 0;
			file->data = fileData;

			node->write = parent->write;
			node->read = parent->read;
			node->seek = parent->seek;
			node->tell = parent->tell;
		}

		node->data = file;

		ALAdd(dirFiles, node);
		return node;
	} else {
		return NULL;
	}
}

#define LFNAME_MAX 256
int FAT12_LoadDirectory(VFS_Node* node) {
	#ifdef FAT12_DEBUG
	kprintf("F12_LD(%x)\n", node);
	#endif

	if(node->options.flags & NOT_STALE) {
		return 0;
	}

	Byte* dirBuffer = (Byte*) kalloc(512);
	memset(dirBuffer, 0, FAT12_SECTOR_SIZE);

	int retVal = 0;

	#ifdef FAT12_DEBUG
	kprintf("dirBuffer = %x\n", dirBuffer);
	#endif

	if(!isdir(node)) {
		retVal = -1;
		goto cleanup;
	}

	FAT12_File* dir = (FAT12_File*) node->data;
	if(!(dir->locationData->attribute&FAT12_ATTR_DIR)) {
		retVal = -1;
		goto cleanup;
	}

	ArrayList* TYPE(VFS_Node*) dirFiles = (ArrayList*)dir->data;

	Bpb* bpb = dir->context->bpb;

	// OK since the first data sector and the first cluster are not the same, we have
	// to keep track of whether the directory is root or not.
	int isRoot = FALSE;
	int absDirSector = 0;

	// This variable is used to keep track of the next sector in the FAT if the directory we are traversing
	// is not the root directory.
	int dirCluster = 0; 

	if(dir->locationData == NULL) {
		// This is the FAT12 root, our directory sector is the first data sector.
		absDirSector = getFirstDataSector(bpb);
		isRoot = TRUE;
	} else {
		dirCluster = dir->locationData->firstCluster;
	}

	#ifdef FAT12_DEBUG
	kprintf("node->name=%s, isRoot=%d\n", node->name, isRoot);
	#endif

	DirEntry* entries = (DirEntry*) dirBuffer;
	wchar_t lfnameBuffer[LFNAME_MAX];
	char lfnameBuffer2[LFNAME_MAX];
	int len;
	Bool doneWithName = false;

	memset(lfnameBuffer, 0, LFNAME_MAX);
	memset(lfnameBuffer, 0, LFNAME_MAX * sizeof(wchar_t));

	// Specify the first sector to read in
	if(!isRoot) {
		absDirSector = getAbsoluteSector(dirCluster, bpb);
	} else {
		absDirSector = getFirstDataSector(bpb);
	}

	int i,iDir;
	iDir = 0;

	ArrayList* TYPE(LongFileNameEntry*) longFileNameCollection = ALCreate();
	LongFileNameEntry** longFileNameArray = NULL;
	int lfnArrayLength = 0;

	// Why do we have !isRoot?  Because the loop will manually "break" on the last cluster
	for(i=0; i<bpb->directoryEntries || !isRoot; i++) {
		if(!(i%16)) {
			// Now load our next cluster number into dirCluster and our next sector number into absDirSector.
			// We also need to wait until i is 16 before getting the next cluster.
			if(!isRoot && i) {
				// Not necessarily contiguous so we find the next cluster from the FAT.
				dirCluster = FAT12_GetClusterFromFAT(dir->context, dirCluster);
				if(dirCluster >= FAT12_EOF) {
					break;
				}

				absDirSector = getAbsoluteSector(dirCluster, bpb);
			} else if(i) {
				absDirSector ++;
			}

			// This takes care of loading our first sector for us.
			FloppyReadSectorNoAlloc(absDirSector, dirBuffer, FAT12_SECTOR_SIZE);
		}
		iDir = i % 16;

		// If the first byte of the name is 0x00, that means that the rest of the entries are NULL as well.

		if(entries[iDir].name[0] == (char) FAT12_ENTRY_NULL) {
			break;
		} else if(entries[iDir].name[0] == (char) FAT12_ENTRY_EMPTY) {
			continue;
		}

		if(entries[iDir].attribute==0xF) {
			// You dummy, &entries[i] was being overwritten, oh well, now I've fixed it.

			LongFileNameEntry* lfnEntry = (LongFileNameEntry*) kalloc(sizeof(LongFileNameEntry));
			memcpy(lfnEntry, &entries[iDir], sizeof(LongFileNameEntry));

			ALAdd(longFileNameCollection, lfnEntry);

			if(lfnEntry->order&0xF0) {
				longFileNameArray = kalloc(sizeof(LongFileNameEntry*)*lfnEntry->order&0x0F);
				
				#ifdef FAT12_DEBUG
				kprintf("kalloc_lfnArray = %x\n", longFileNameArray);
				#endif
				
				lfnArrayLength = lfnEntry->order & 0xF;
				memset(longFileNameArray, 0, sizeof(LongFileNameEntry*)*lfnArrayLength);

				// I bet you it's this itr that's causing us our problems.
				ALIterator* itr = ALGetItr(longFileNameCollection);

				// Copy everything in the ArrayList to the array.
				longFileNameArray[(lfnEntry->order&0xF)-1] = lfnEntry;
				while(ALItrHasNext(itr)) {

					LongFileNameEntry* tmpLfnEntry = (LongFileNameEntry*) ALItrNext(itr);

					#ifdef FAT12_DEBUG_VERBOSE
					kprintf("tmplfn=%x\n", tmpLfnEntry);
					#endif
					
					longFileNameArray[(tmpLfnEntry->order&0xF)-1] = tmpLfnEntry;
				}

				ALFreeItr(itr);
			} else {
				if(longFileNameArray) {
					longFileNameArray[(lfnEntry->order&0xF)-1] = lfnEntry;
				}
			}

			// If none of the long file name entries stored in the array are null, we are done with the name.
			doneWithName = false;
			if(longFileNameArray != NULL) {
				doneWithName = true;

				int k;
				for(k=0; k<lfnArrayLength; k++) {
					if(longFileNameArray[k] == NULL) {
						doneWithName = false;
					}
				}
			}
		}

		// The second clause is there for the second time around, when longFileNameArray
		// is null.
		if(doneWithName && longFileNameArray) {
			len = 0;

			int j;
			for(j=0; j<lfnArrayLength; j++) {
				LongFileNameEntry* lfnEntry = longFileNameArray[j];
				memcpy(&lfnameBuffer[((lfnEntry->order&0xF)-1)*13], &lfnEntry->first5, sizeof(wchar_t)*5);
				memcpy(&lfnameBuffer[((lfnEntry->order&0xF)-1)*13+5], &lfnEntry->next6, sizeof(wchar_t)*6);
				memcpy(&lfnameBuffer[((lfnEntry->order&0xF)-1)*13+11], &lfnEntry->final2, sizeof(wchar_t)*2);
				len+=13;
			}

			// OK, this converts the 2-byte character lfnameBuffer into a 1-byte character buffer.
			for(j=0; lfnameBuffer[j]!=0; j++) {
				lfnameBuffer2[j] = (char) lfnameBuffer[j];
			}

			lfnameBuffer2[j] = 0;

			#ifdef FAT12_DEBUG
			kprintf("lfname=%s, lfnArrayLength=%d\n", lfnameBuffer2, lfnArrayLength);
			#endif

			// This is freeing the array of long file name entries, NOT the buffer for the name.
			kfree(longFileNameArray);
			longFileNameArray = NULL;
			lfnArrayLength = 0;
			// We're done with the long file names, so we can free the pointers in the list.
			ALClear(longFileNameCollection, TRUE);
		}

		if(entries[iDir].attribute != FAT12_LONG_FILENAME) {
			int fileType = (entries[iDir].attribute == FAT12_ATTR_DIR) ? FILE_DIRECTORY : FILE_FILE;

			char* name = NULL;
			if(doneWithName) {
				name = lfnameBuffer2;
			} else {
				// Use 8.3 internal file name.
				name = kalloc(12);
				memset(name, 0, 12);

				strncpy(name, 11, entries[iDir].name);
			}

			int foundFile = 0;
			ALIterator* itr = ALGetItr(dirFiles);
			while(ALItrHasNext(itr)) {
				VFS_Node* search = ALItrNext(itr);
				if(!strcmp(search->name, name)) {
					foundFile = 1;
				}
			}

			VFS_Node* file;

			// If we didn't find the file already in the directory representation, add it.
			if(!foundFile) {
				file = AddFile(fileType, name, node);
			}

			if(file != NULL && !foundFile) {
				#ifdef FAT12_DEBUG
				kprintf("new file %s\n", file->name);
				#endif
				
				// Set up our FAT12_File
				FAT12_File* fileData = (FAT12_File*) file->data;
				fileData->locationData = kalloc(sizeof(DirEntry));

				memcpy(fileData->locationData, &entries[iDir], sizeof(DirEntry));
				fileData->context = dir->context;
			} else if(!foundFile) {
				kprintf("AddFile(%x, %s, %x) produced null\n", fileType, name, node);
			}

			if(!doneWithName) {
				// We had to allocate a string so clean up.
				kfree(name);
			} else {
				// Do any cleanup here.
				// Can't think of any at the moment.
			}

			doneWithName = false;
		}
		
		#ifdef FAT12_DEBUG_VERBOSE
		
		char name_debug[12];
		strncpy(name_debug, 11, entries[iDir].name);
		name_debug[11]=0;
		
		kprintf("name_debug=%s, attr=%x\n", name_debug, entries[iDir].attribute);
		#endif
	}
	
	#ifdef FAT12_DEBUG
	kprintf("ret %d\n", retVal);
	#endif

	node->options.flags |= NOT_STALE;
cleanup:
	// cleanup and return
	kfree(dirBuffer);

	return retVal;
}