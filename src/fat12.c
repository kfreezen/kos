#include <fat12.h>
#include <floppy.h>
#include <kheap.h>
#include <print.h>
#include <debugdef.h>
#include <common/arraylist.h>

#define FAT12_DEBUG
#define FAT12_DEBUG_VERBOSE

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

Byte fatBuffer[512];
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

UInt16 FAT12_GetClusterFromFAT(FAT12_Context* context, int prevCluster) {
	Bpb* bpb = context->bpb;
	int fatOffset = prevCluster + (prevCluster / 2);
	int fatSector = bpb->reservedSectorsNum+(fatOffset/512);
	int entryOffset = fatOffset%512;
	
	if(fatSectorInBuffer!=fatSector) {
		FloppyReadSectorNoAlloc(fatSector, fatBuffer);
		fatSectorInBuffer = fatSector;
	}
	
	UInt16 table_value = ((UInt16*)fatBuffer)[entryOffset];
	
	if(entryOffset&0x0001) {
		table_value = table_value >> 4;
	} else {
		table_value = table_value & 0x0FFF;
	}
	
	return table_value;
}

VFS_Node* FAT12_GetNode(VFS_Node* node, const char* name) {
	return NULL;
}

Byte dirBuffer[512];
/*FAT12_File* FAT12_GetFile(FAT12_File* dir, const char* file) {
	Bpb* bpb = context->bpb;

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

	DirEntry* entries = (DirEntry*) dirBuffer;
	wchar_t lfnameBuffer[256];
	char lfnameBuffer2[256];
	int len;
	Bool doneWithName = false;

	// Specify the first sector to read in
	if(!isRoot) {
		absDirSector = getAbsoluteSector(dirCluster, bpb);
	} else {
		absDirSector = getFirstDataSector(bpb);
	}

	int i;

	// Why do we have !isRoot?  Because the loop will manually "break" on the last cluster
	for(i=0; i<bpb->directoryEntries || !isRoot; i++) {
		if(!(i%32)) {
			// This takes care of loading our first sector for us.
			FloppyReadSectorNoAlloc(absDirSector, dirBuffer);
			// Now load our next cluster number into dirCluster and our next sector number into absDirSector.
			if(!isRoot) {
				// Not necessarily contiguous so we find the next cluster from the FAT.
				dirCluster = FAT12_GetClusterFromFAT(dir->context, dirCluster);
				if(dirCluster >= FAT12_EOF) {
					break;
				}

				absDirSector = getAbsoluteSector(dirCluster, bpb);
			} else {
				absDirSector ++;
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
			
			fat12_file->data = kalloc(sizeof(DirEntry));
			memcpy(fat12_file->data, entries[i], sizeof(DirEntry));
			
			#ifdef FAT12_DEBUG
			kprintf("ret %x\n", fat12_file);
			#endif
			
			return fat12_file;
		}
	}
	
	#ifdef FAT12_DEBUG
	kprintf("ret NULL\n");
	#endif
	
	return NULL;
}*/

#define rootBuffer dirBuffer
FAT12_File* FAT12_GetFile(FAT12_Context* context, const char* file) {
	#ifdef FAT12_DEBUG
	char space = ' ';
	#ifdef FAT12_DEBUG_VERBOSE
		space = '\n';
	#endif
	
	kprintf("FAT12_GetFile(%x,%s)%c", context, file, space);
	#endif
	
	Bpb* bpb = context->bpb;
	
	// This is the root directory's first data sector
	int rootDir = getFirstDataSector(bpb);
	
	DirEntry* entries = (DirEntry*) rootBuffer;
	
	wchar_t lfnameBuffer[256];
	char lfnameBuffer2[256];
	int len;
	Bool doneWithName = false;

	int i=0;
	for(i=0; i<bpb->directoryEntries; i++) {
		
		if(!(i%32)) {
			FloppyReadSectorNoAlloc(rootDir, rootBuffer);
			rootDir++;
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
			
			return fat12_file;
		}
	}
	
	#ifdef FAT12_DEBUG
	kprintf("ret NULL\n");
	#endif
	
	return NULL;
}
#undef rootBuffer

Byte readSectorBuffer[512];

int FAT12_Read_LL(FAT12_File* node, UInt32 offset, UInt32 length, UInt8* buf) {
	if(node == NULL) {
		return -1;
	}
	
	#ifdef FAT12_DEBUG
	kprintf("FAT12_Read_LL(%x, %x, %x, %x)\n", node, offset, length, buf);
	#endif
	
	offset &= ~511;
	
	if(((DirEntry*)node->data)->attribute==FAT12_ATTR_DIR) {
		return -1;
	}
	
	if(length==0) {
		length = ((DirEntry*)node->data)->fileSize;
	}
	
	//int size = (node->data.fileSize&~0x1FF)+0x200;
	
	UInt8* buffer = buf;
	
	FAT12_Context* context = node->context;
	DirEntry* entry = (DirEntry*) node->data;
	
	UInt16 relSector = entry->firstCluster;
	
	if(relSector==0) {
		kprintf("entry.firstCluster==0\n");
		return -1;
	}
	
	int i;
	int numToRead = length/0x200;
	if(numToRead==0) {
		numToRead = 1;
	}
	
	for(i=0; i<numToRead; i++) {
		// Error checking.
		if(relSector >= FAT12_EOF) {
			break;
		} else if(relSector == 0x00) {
			#ifdef FAT12_DEBUG
			kprintf("relSector==0, breaking\n");
			#endif
			
			break;
		}
		
		int absSector = getAbsoluteSector(relSector, context->bpb);
		
		#ifdef FAT12_DEBUG
		kprintf("reading %x,%x,rel=%x\n", &buffer[i*512], absSector, relSector);
		#endif
		
		FloppyReadSectorNoAlloc(absSector, readSectorBuffer);
		
		memcpy(&buffer[i*512], readSectorBuffer, 512);
		relSector = FAT12_GetClusterFromFAT(context, relSector);
		
		#ifdef FAT12_DEBUG
		kprintf("relSector=%x\n", relSector);
		#endif
		
		if(relSector>=FAT12_EOF) {
			return 0;
		}
	}
	
	#ifdef FAT12_DEBUG_VERBOSE
	kprintf(" ret\n");
	#endif

	return -1;
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
	fb.length = ((DirEntry*)node->data)->fileSize;
	
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

int FAT12_Init(FAT12_Context* context, const char* parentPath, const char* mountpointName) {
	// Get our parent from parentPath
	VFS_Node* parent = GetNodeFromPath(parentPath);
	if(parent == NULL) {
		kprintf("Error in fat12:  parent==NULL, parentPath=%s\n", parentPath);
	}

	VFS_Node* mount = AddFile(FILE_DIRECTORY, mountpointName, parent);
	FAT12_File* dir = kalloc(sizeof(FAT12_File));
	dir->data = ALCreate();
	dir->context = context;

	CreateMountPoint(mount, dir,
		FAT12_AddFile, FAT12_LoadDirectory,
		NULL, NULL,
		NULL, NULL
	);

	return 0;
}

ArrayList* FAT12_ListFiles(VFS_Node* dir);

VFS_Node* FAT12_GetNode(VFS_Node* node, const char* name);

VFS_Node* FAT12_AddFile(int fileType, const char* name, VFS_Node* parent) {
	if(isdir(parent)) {
		FAT12_File* dir = (FAT12_File*) parent->data;
		ArrayList* TYPE(VFS_Node*) dirFiles = (ArrayList*) dir->data;

		VFS_Node* node = kalloc(sizeof(VFS_Node));
		node->fileType = fileType;
		strcpy(node->name, name);

		FAT12_File* fileData = kalloc(sizeof(FAT12_File));

		if(isdir(node)) {
			fileData->data = ALCreate();

			node->addfile = parent->addfile;
			node->dirload = parent->dirload;
			node->listfiles = parent->listfiles;
			node->getnode = parent->getnode;
			node->write = parent->write;
			node->read = parent->read;
		} else {
			node->write = parent->write;
			node->read = parent->read;
		}

		ALAdd(dirFiles, node);
		
		return node;
	} else {
		return NULL;
	}
}

int FAT12_LoadDirectory(VFS_Node* node) {
	if(!isdir(node)) {
		return -1;
	}

	FAT12_File* dir = (FAT12_File*) node->data;
	if(!(dir->locationData->attribute&FAT12_ATTR_DIR)) {
		return -1;
	}

	ALFreeList((ArrayList*)dir->data);
	
	ArrayList* TYPE(VFS_Node*) dirFiles = ALCreate();
	dir->data = dirFiles;

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

	DirEntry* entries = (DirEntry*) dirBuffer;
	wchar_t lfnameBuffer[256];
	char lfnameBuffer2[256];
	int len;
	Bool doneWithName = false;

	// Specify the first sector to read in
	if(!isRoot) {
		absDirSector = getAbsoluteSector(dirCluster, bpb);
	} else {
		absDirSector = getFirstDataSector(bpb);
	}

	int i;

	// Why do we have !isRoot?  Because the loop will manually "break" on the last cluster
	for(i=0; i<bpb->directoryEntries || !isRoot; i++) {
		if(!(i%32)) {
			// This takes care of loading our first sector for us.
			FloppyReadSectorNoAlloc(absDirSector, dirBuffer);
			// Now load our next cluster number into dirCluster and our next sector number into absDirSector.
			if(!isRoot) {
				// Not necessarily contiguous so we find the next cluster from the FAT.
				dirCluster = FAT12_GetClusterFromFAT(dir->context, dirCluster);
				if(dirCluster >= FAT12_EOF) {
					break;
				}

				absDirSector = getAbsoluteSector(dirCluster, bpb);
			} else {
				absDirSector ++;
			}
		}

		if(entries[i].attribute==0xF) {
			LongFileNameEntry* lfnEntry = (LongFileNameEntry*) &entries[i];
			memcpy(&lfnameBuffer[((lfnEntry->order&0xF)-1)*13], &lfnEntry->first5, sizeof(wchar_t)*5);
			memcpy(&lfnameBuffer[((lfnEntry->order&0xF)-1)*13+5], &lfnEntry->next6, sizeof(wchar_t)*6);
			memcpy(&lfnameBuffer[((lfnEntry->order&0xF)-1)*13+11], &lfnEntry->final2, sizeof(wchar_t)*2);
			len+=13;
			
			if(lfnEntry->order&0xF0) {
				// Done.
				doneWithName = true;
				// OK, this converts the 2-byte character lfnameBuffer into a 1-byte character buffer.
				int j;
				for(j=0; lfnameBuffer[j]!=0; j++) {
					lfnameBuffer2[j] = (char) lfnameBuffer[j];
				}
				
				lfnameBuffer2[j] = 0;
				
				/*
				// So now we need to check to see if this is the long file name entry.
				if(!strcmp(lfnameBuffer2, file)) {
					// If it checks out, we want to load it.
					#ifdef FAT12_DEBUG
					kprintf("lfnameBuffer2=%s\n", lfnameBuffer2);
					#endif
					
					i++;
				}*/ // Belongs in an individual file search.
				
				#ifdef FAT12_DEBUG
				kprintf("lfname=%s\n", lfnameBuffer2);
				#endif
				
				continue;
			} else {
				int fileType = (entries[i].attribute == FAT12_ATTR_DIR) ? FILE_DIRECTORY : FILE_FILE;

				char* name = NULL;
				if(doneWithName) {
					name = lfnameBuffer2;
				} else {
					// Use 8.3 internal file name.
					name = kalloc(12);
					memset(name, 0, 12);

					strncpy(name, 11, entries[i].name);
				}

				VFS_Node* file = AddFile(fileType, name, node);

				if(file != NULL) {
					// Set up our FAT12_File
					FAT12_File* fileData = (FAT12_File*) file->data;
					fileData->locationData = kalloc(sizeof(DirEntry));
					memcpy(fileData->locationData, &entries[i], sizeof(DirEntry));
				} else {
					kprintf("AddFile(%x, %s, %x) produced null\n", fileType, name, node);
				}

				if(!doneWithName) {
					// We had to allocate a string so clean up.
					kfree(name);
				}

				doneWithName = false;
			}
		}
		
		#ifdef FAT12_DEBUG_VERBOSE
		
		char name_debug[12];
		strncpy(name_debug, 11, entries[i].name);
		name_debug[11]=0;
		
		kprintf("name_debug=%s, attr=%x\n", name_debug, entries[i].attribute);
		#endif
	}
	
	#ifdef FAT12_DEBUG
	kprintf("ret NULL\n");
	#endif
	
	kprintf("File list");
	ALIterator* itr = ALGetItr(dirFiles);
	while(ALItrHasNext(itr)) {
		VFS_Node* __node = (VFS_Node*) ALItrNext(itr);
		kprintf("node->name=%s", __node->name);
	}

	return NULL;
}