#ifndef INITRD_H
#define INITRD_H

#define INITRD_MAGIC 0x12345678

struct __InitrdHeader {
	unsigned magic;
	int numFiles;
	unsigned filesOffset;
	char reserved[48];
} __attribute__((packed));

typedef struct __InitrdHeader InitrdHeader;

struct __InitrdFileHeader {
	unsigned magic;
	char name[60];
	unsigned dataOffset;
	unsigned fileLength;
	char reserved[24];
} __attribute__((packed));

typedef struct __InitrdFileHeader InitrdFileHeader;

#endif