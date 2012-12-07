#include <fat12.h>
#include <floppy.h>
#include <kheap.h>
#include <print.h>

//#define FAT12_DEBUG
//#define FAT12_DEBUG_VERBOSE

inline int getFirstDataSector(Bpb* bpb) {
	return bpb->reservedSectorsNum + (bpb->fatTables*bpb->numSectorsPerFat);
}

inline int getAbsoluteCluster(int relSector, Bpb* bpb) {
	return relSector - 2 + getFirstDataSector(bpb);
}

inline int getAbsoluteSector(int relSector, Bpb* bpb) {
	return getAbsoluteCluster(relSector, bpb) + (bpb->directoryEntries*32/bpb->bytesPerSector);
}

Byte fatBuffer[512];
int fatSectorInBuffer=-1;

FAT12_Context* FAT12_GetContext(Device* device) {
	FAT12_Context* context = kalloc(sizeof(FAT12_Context));
	context->linkedDevice = device;
	context->bpb = device->getDeviceInfo();
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

Byte rootBuffer[512];
FAT12_File* FAT12_GetFile(FAT12_Context* context, const char* file) {
	#ifdef FAT12_DEBUG
	char space = ' ';
	#ifdef FAT12_DEBUG_VERBOSE
		space = '\n';
	#endif
	
	kprintf("FAT12_GetFile(%x,%s)%c", context, file, space);
	#endif
	
	Bpb* bpb = context->bpb;
	
	int rootDir = bpb->reservedSectorsNum + (bpb->fatTables*bpb->numSectorsPerFat);
	
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
				int j;
				for(j=0; lfnameBuffer[j]!=0; j++) {
					lfnameBuffer2[j] = (char) lfnameBuffer[j];
				}
				
				lfnameBuffer2[j] = 0;
				
				#ifdef FAT12_DEBUG
				kprintf("lfname=%s\n", lfnameBuffer2);
				#endif
				
				//memset(lfnameBuffer, 0, 256);
				doneWithName = true;
				i++;
			}
		}
		
		#ifdef FAT12_DEBUG_VERBOSE
		
		char name_debug[12];
		strncpy(name_debug, 11, entries[i].name);
		name_debug[11]=0;
		
		kprintf("name_debug=%s, attr=%x\n", name_debug, entries[i].attribute);
		#endif
		
		if(strncmp(file, 11, entries[i].name)==0 || (doneWithName && strcmp(file, lfnameBuffer2)==0)) {
			FAT12_File* fat12_file = (FAT12_File*)kalloc(sizeof(FAT12_File));
			fat12_file->context = context;
			fat12_file->data = entries[i];
			
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

Byte readSectorBuffer[512];

UInt32 FAT12_Read_LL(FAT12_File* node, UInt32 offset, UInt32 length, UInt8* buf) {
	offset &= ~511;
	
	if(node->data.attribute==FAT12_ATTR_DIR) {
		return -1;
	}
	
	if(length==0) {
		length = node->data.fileSize;
	}
	
	//int size = (node->data.fileSize&~0x1FF)+0x200;
	
	UInt8* buffer = buf;
	
	FAT12_Context* context = node->context;
	DirEntry entry = node->data;
	
	UInt16 relSector = entry.firstCluster;
	
	int i;
	int numToRead = length/0x200;
	if(numToRead==0) {
		numToRead = 1;
	}
	
	for(i=0; i<numToRead; i++) {
		int absSector = getAbsoluteSector(relSector, context->bpb);
		
		#ifdef FLOPPY_DEBUG
		kprintf("reading %x,%x\n", &buffer[i*512], absSector);
		#endif
		
		FloppyReadSectorNoAlloc(absSector, readSectorBuffer);
		memcpy(&buffer[i*512], readSectorBuffer, 512);
		relSector = FAT12_GetClusterFromFAT(context, relSector);
		if(relSector>=FAT12_EOF) {
			break;
		}
	}
	
	return -1;
}

// TODO:  When FAT12_Read_LL is improved, make sure this function is changed to take 'length' at face value.
FileBuffer FAT12_Read_FB(FAT12_File* node, UInt32 off, UInt32 length) {
	FileBuffer fb;
	fb.length = node->data.fileSize;
	
	int len = fb.length;
	if(len&0x1ff) {
		len = len&~0x1ff;
		len+=0x200;
	}
	
	fb.buffer = kalloc(len);
	FAT12_Read_LL(node, off, length, fb.buffer);
	
	return fb;
}
