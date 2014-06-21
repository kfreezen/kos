#include <vfs.h>
#include <common.h>
#include <print.h>
#include <err.h>

//#define VFS_DEBUG

VFS_Node* vfsRoot = NULL;
UInt64 vfsId = 0;

inline UInt64 VFS_GetID() {
	return vfsId++;
}

inline UInt64 GetInode() {
	return vfsId++;
}

// If we want to create a mountpoint, we call another function
// after this one.
VFS_Node* VFS_AddFile(int fileType, const char* name, VFS_Node* parent) {
	if(isdir(parent)) {
		DirectoryData* dir = (DirectoryData*) parent->data;

		VFS_Node* node = kalloc(sizeof(VFS_Node));
		node->fileType = fileType;
		strcpy(node->name, name);

		// Inheirit all the functions from the parent.
		if(isdir(node)) {
			DirectoryData* data = kalloc(sizeof(DirectoryData));
			data->files = ALCreate();
			node->data = data;

			node->addfile = parent->addfile;
			node->dirload = parent->dirload;
			node->listfiles = parent->listfiles;
			node->getnode = parent->getnode;
			node->write = parent->write;
			node->read = parent->read;
		} else {
			node->write = parent->write;
			node->read = parent->read;
			// TODO:  Implement file maniplation stuff.
		}

		ALAdd(dir->files, node);
		
		return node;
	} else {
		return NULL;
	}

	return NULL;
}

VFS_Node* VFS_GetNode(VFS_Node* node, const char* name) {
	#ifdef VFS_DEBUG
	kprintf("VFS_GetNode(%x, %s)\n", node, name);
	#endif

	if(isdir(node)) {
		DirectoryData* dir = (DirectoryData*) node->data;

		ArrayList* TYPE(VFS_Node*) fileList = dir->files;
		ALIterator* itr = ALGetItr(fileList);
		VFS_Node* ret = NULL;

		while(ALItrHasNext(itr)) {
			VFS_Node* node = (VFS_Node*) ALItrNext(itr);
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

ArrayList* VFS_ListFiles(VFS_Node* node) {
	if(isdir(node)) {
		ArrayList* TYPE(VFS_Node*) fileList = ALCreate();
		DirectoryData* dir = (DirectoryData*) node->data;

		ALIterator* itr = ALGetItr(dir->files);
		// Copy it over now.
		while(ALItrHasNext(itr)) {
			ALAdd(fileList, ALItrNext(itr));
		}

		ALFreeItr(itr);
		return fileList;
	} else {
		return NULL;
	}

	return NULL;
}

void VFS_Init() {
	vfsRoot = kalloc(sizeof(VFS_Node));
	memset(vfsRoot, 0, sizeof(VFS_Node));
	vfsRoot->inode = GetInode();
	vfsRoot->parent = NULL;
	vfsRoot->fileType = FILE_DIRECTORY | MOUNTPOINT;

	vfsRoot->data = kalloc(sizeof(DirectoryData));
	DirectoryData* data = (DirectoryData*) vfsRoot->data;
	data->files = ALCreate();

	// Now let's assign ourselves all the functions
	vfsRoot->addfile = &VFS_AddFile;
	vfsRoot->listfiles = &VFS_ListFiles;
	vfsRoot->getnode = &VFS_GetNode;
}

int CreateMountPoint(VFS_Node* node, void* data,
		addfile_func addfile, dirload_func dirload,
		getnode_func getnode, listfiles_func listfiles,
		write_func write, read_func read,
		seek_func seek, tell_func tell
		) {
	#ifdef VFS_DEBUG
	kprintf("CreateMountPoint(%x, %x, %x, %x, %x, %x, %x, %x);\n", node, data, addfile, dirload,
		getnode, listfiles, write, read
	);
	#endif

	if(isdir(node)) {
		// We're going to be a mountpoint so
		// get rid of all the files before this.
		if(data) {
			kfree(node->data);
			node->data = data;
		}

		node->addfile = addfile;
		node->dirload = dirload;
		node->getnode = getnode;
		node->listfiles = listfiles;
		node->write = write;
		node->read = read;
		node->seek = seek;
		node->tell = tell;

		node->fileType |= MOUNTPOINT;
		return 0;
	} else {
		return 1;
	}
}

VFS_Node* AddFile(int fileType, const char* name, VFS_Node* parent) {
	if(parent && parent->addfile) {
		VFS_Node* node = parent->addfile(fileType, name, parent);
		node->inode = GetInode();
		return node;
	} else {
		kprintf("addfile null");
		return NULL;
	}
}

ArrayList* ListFiles(VFS_Node* dir) {
	if(dir && dir->listfiles) {
		return dir->listfiles(dir);
	} else {
		return NULL;
	}
}

VFS_Node* GetNode(VFS_Node* node, const char* name) {
	#ifdef VFS_DEBUG
	kprintf("GetNode(%x, %s) ", node, name);
	#endif

	if(node!=NULL && node->getnode) {
		#ifndef VFS_DEBUG
		return node->getnode(node, name);
		#else
		VFS_Node* returnedNode = node->getnode(node, name);
		kprintf("returnedNode = %x\n", returnedNode);
		return returnedNode;
		#endif
	} else {
		kprintf("VFS_GetNode:  node or node->getnode is NULL.  node=%x, node->getnode=%x\n", node, (node) ? node->getnode : NULL);
		return NULL;
	}
}

VFS_Node* VFS_GetRoot() {
	return vfsRoot;
}

// This algorithm is horrible, because worst case it has to iterate through every single file on each level.
File* GetFileFromPath(const char* path) {
	if(vfsRoot == NULL) {
		return NULL;
	}

	// From root.
	char* buf = kalloc(strlen(path)+2);
	strcpy(buf, path);

	VFS_Node* node = vfsRoot;
	VFS_Node* ret = node;

	char* tok = strtok(buf, "/");

	while(tok != NULL) {
		if(strlen(tok) == 0) {
			// Let's just continue, in case there is more to the string.
			//continue;
		} else {
			VFS_Node* newNode = GetNode(node, tok);

			#ifdef VFS_DEBUG
			kprintf("tok=%s\n", tok);
			#endif

			if(newNode == NULL) {
				// Doesn't exist, so return NULL.
				return NULL;
			}

			if(!isdir(newNode)) {
				#ifdef VFS_DEBUG
				kprintf("ret=%s\n", newNode->name);
				#endif

				ret = newNode; // This may not be the last in the path, but
				// this is a bottom-node file, so let's just return it.
				break;
			} else {
				node = newNode; // Continue in our FS hierarchy
			}
		}

		ret = node;

		tok = strtok(NULL, "/");
	}

	kfree(buf);
	
	// Create ourselves a file handle for this.
	File* fret = kalloc(sizeof(File)); // Tee-hee fret. stop fretting about it.
	fret->node = ret;
	fret->filePos = 0;

	return fret;
}

int fgetline(File* file, char* buf, int maxlen, char end) {
	int base = FileTell(file);
	int readItr = 0;

	/*kprintf("fgetline file.length=%x\n", FileSeek(SEEK_EOF, file));
	FileSeek(base, file);*/
	
	while(1) {
		int retRead = ReadFile(buf+readItr, 32, file);

		if(retRead < 32) {
			// encountered EOF, probably...
			return -1;
		}

		readItr += 32;
		
		// Now we want to copy over our stuff.
		int i;
		for(i=readItr-32; i<readItr; i++) {
			if(buf[i] == end) {
				buf[i] = 0;
				FileSeek(base+i+1, file);
				return readItr;
			}
		}

		if(readItr >= maxlen) {
			return maxlen;
		}
	}
}

int ReadFile(void* buf, int len, File* file) {
	if(!file) {
		kprintf("readfile:file==NULL\n");
		return -1;
	}

	#ifdef VFS_DEBUG
	kprintf("file->node=%x, file->node->read=%x, file->node->name=%s\n", file->node, file->node->read, file->node->name);
	#endif

	if(file && file->node && file->node->read) {
		return file->node->read(buf, len, file);
	}

	return -1;
}

int WriteFile(const void* buf, int len, File* file) {
	if(file && file->node && file->node->write) {
		return file->node->write(buf, len, file);
	}

	return -1;
}

int LoadDirectory(VFS_Node* dir) {
	if(dir && dir->dirload) {
		int ret = dir->dirload(dir);
		return ret;
	}
	return -1;
}

int FileSeek(int newPos, File* file) {
	if(file && file->node && file->node->seek) {
		return file->node->seek(newPos, file);
	}

	SetErr(ERR_NULL_VALUE_ENCOUNTERED);
	return -1;
}

int FileTell(File* file) {
	if(file && file->node && file->node->tell) {
		return file->node->tell(file);
	}

	SetErr(ERR_NULL_VALUE_ENCOUNTERED);
	return -1;
}

// We want to use this instead of f->node in our code, so we break
// fewer things when the internal representation changes.
inline VFS_Node* GetNodeFromFile(File* f) {
	return (f) ? f->node : NULL;
}

File* GetFileFromNode(VFS_Node* node) {
	File* file = kalloc(sizeof(File));
	file->node = node;
	file->filePos = 0;

	return file;
}

UInt64 GetInodeFromNode(VFS_Node* node) {
	return node->inode;
}

void CloseFile(File* f) {
	// For now, a file is a pure VFS driver abstraction.  There is no external drivers that the
	// VFS will need for this, so let's just free the file.
	kfree(f);
}