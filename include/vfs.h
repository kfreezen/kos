#ifndef VFS_H
#define VFS_H

#include <KOSTypes.h>
#include <device.h>
#include <kheap.h>
#include <common/arraylist.h>

// There are 3 types of "files":
// streams, directories, and files

/**

A bottom-node file is one which cannot have children.
Streams and files are bottom-node files.

Functions list in the files
fopen():
	This function is used to open a bottom-node file.
dirload():
	This function is used to load the IDs of the node's children.

How do I want directories and mount points to keep track of the files within them?
Each mount-point has it's own system of tracking file nodes.
The VFS increments an integer by one every time a new VFS node is created.

Every file object has a "list" of its manipulating functions, given to it by its opener, as well as a file stream position.

device files:
	Device files hook the calls into their device drivers.
	By design, drivers can hook directly into the file
	manipulation functions.
directories:
	These store a list of every file in the directory.
	Valid functions are fopen(), dirload(), addfile()
	In order to specify this directory as a mountpoint,
	do FILE_DIRECTORY | MOUNTPOINT
	A directory needs to keep track of all its nodes.
	The way that this VFS will do it is to load the nodes
	into an ArrayList.

All non-bottom-node files should keep every function in
their VFS_Node.

END_FILE_DOC

fileType:
	FILE_FILE
	FILE_STREAM
	FILE_DIRECTORY
**/

#define FILE_FILE 1
#define FILE_DEVICE 2
#define FILE_DIRECTORY 3
#define MOUNTPOINT 1<<7
#define FLAGS (MOUNTPOINT)

#define NAME_LENGTH 128

#define isdir(n) ( ((n)->fileType&(~FLAGS)) == FILE_DIRECTORY)

#define O_NONBLOCKING 1<<0

typedef struct VFS_Options {
	int flags;
	int timeout; // In ms
} VFS_Options;

// This is ugly code, but I guess that's what you get
// for referencing a struct from within itself.
typedef struct VFS_Node {
	int fileType;
	void* id;
	char name[NAME_LENGTH]; // XXX:  Pointer or fixed length?
	struct VFS_Node* parent;
	void* data;

	VFS_Options options;

	struct VFS_Node* (*addfile)(int, const char*, struct VFS_Node*);
	int (*dirload)(struct VFS_Node*);
	ArrayList* TYPE(VFS_Node*) (*listfiles)(struct VFS_Node*);
	struct VFS_Node* (*getnode)(struct VFS_Node*, const char*);
	int (*write)(const char* buf, int len, struct VFS_Node* node);
	int (*read)(char* buf, int len, struct VFS_Node* node);
} VFS_Node;

typedef struct {
	ArrayList* TYPE(VFS_Node*) files;
} DirectoryData;

typedef VFS_Node* (*addfile_func)(int fileType, const char* name, VFS_Node* parent);
typedef int (*dirload_func)(VFS_Node* node);

// This should give us a node to manipulate.
typedef VFS_Node* (*getnode_func)(VFS_Node* node, const char* name);

// Should return an arraylist of child nodes.
typedef ArrayList* TYPE(VFS_Node*) (*listfiles_func)(VFS_Node* dir);

typedef int (*write_func)(const char* buf, int len, VFS_Node* node);
typedef int (*read_func)(char* buf, int len, VFS_Node* node);

void VFS_Init();
VFS_Node* VFS_GetRoot();

VFS_Node* AddFile(int fileType, const char* name, VFS_Node* parent);
int CreateMountPoint(VFS_Node* node, void* data,
		addfile_func addfile, dirload_func dirload,
		getnode_func getnode, listfiles_func listfiles,
		write_func write, read_func read
		);

int LoadDirectory(VFS_Node* dir);
ArrayList* ListFiles(VFS_Node* dir);
VFS_Node* GetNode(VFS_Node* node, const char* name);

int ReadFile(char* buf, int len, VFS_Node* node);
int WriteFile(const char* buf, int len, VFS_Node* node);

VFS_Node* GetNodeFromPath(const char* path);

#endif
