#include <dev.h>
#include <kheap.h>

//#define DEV_DEBUG

VFS_Node* devfsRoot = NULL;

UInt32 devFSId = 0;

void* GetDevFSId() {
	return (void*) devFSId ++;
}

VFS_Node* DevFS_AddFile(int fileType, const char* name, VFS_Node* parent) {
	// /dev/ caters only to device files and directories.
	if(fileType == FILE_FILE) {
		return NULL; // We don't support proper files in devfs.
	}

	if(!isdir(parent)) {
		return NULL;
	}

	VFS_Node* node = kalloc(sizeof(VFS_Node));
	memset(node, 0, sizeof(VFS_Node));

	node->fileType = fileType;

	strncpy(node->name, NAME_LENGTH-1, name);

	if(isdir(node)) {
		node->data = kalloc(sizeof(DirectoryData));
		node->addfile = parent->addfile;
		node->dirload = parent->dirload;
		node->listfiles = parent->listfiles;
		node->getnode = parent->getnode;
		node->write = parent->write;
		node->read = parent->read;
		node->seek = parent->seek;
		node->tell = parent->tell;
	} else {
		node->write = parent->write;
		node->read = parent->read;
		node->seek = parent->seek;
		node->tell = parent->tell;
	}

	DirectoryData* dir = (DirectoryData*) parent->data;
	ALAdd(dir->files, node);

	#ifdef DEV_DEBUG
	kprintf("dir->length=%d\n", dir->files->length);
	#endif
	
	return node;
}

VFS_Node* DevFS_GetNode(VFS_Node* node, const char* name) {
	if(isdir(node)) {
		DirectoryData* dir = (DirectoryData*) node->data;

		ArrayList* TYPE(VFS_Node*) fileList = dir->files;
		ALIterator* itr = ALGetItr(fileList);
		VFS_Node* ret = NULL;

		while(ALItrHasNext(itr)) {
			VFS_Node* node = (VFS_Node*) ALItrNext(itr);

			#ifdef DEV_DEBUG
			kprintf("devfs->nodename=%s\n", node->name);
			#endif

			if(!strcmp(node->name, name)) { // XXX Should probably use strncmp
				ret = node; // XXX Should we give a copy of the object?
				break;
			}
		}

		ALFreeItr(itr);
		return ret;
	} else {
		return NULL;
	}
}

int DevFS_Init() {
	if(devfsRoot != NULL) {
		return 0;
	}

	VFS_Node* root = VFS_GetRoot();
	VFS_Node* devMount = AddFile(FILE_DIRECTORY, "dev", root);
	
	CreateMountPoint(devMount, NULL, 
		DevFS_AddFile, NULL,
		DevFS_GetNode, NULL,
		NULL, NULL,
		NULL, NULL
	);

	devfsRoot = devMount;

	return 0;
}

VFS_Node* RegisterDevice(DeviceData* dev) {
	if(devfsRoot == NULL) {
		return NULL;
	}

	VFS_Node* devNode = AddFile(FILE_DEVICE, dev->name, devfsRoot);

	devNode->write = dev->write;
	devNode->read = dev->read;

	#ifdef DEV_DEBUG
	kprintf("dev %x, %x\n", devNode->read, dev->read);
	#endif

	return devNode;
}