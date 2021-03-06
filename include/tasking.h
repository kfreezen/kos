#ifndef TASKING_H
#define TASKING_H

#include <KOSTypes.h>
#include <paging.h>

struct Task;

typedef struct ProcessInfo {
	ArrayList TYPE(File*)* files;
} ProcessInfo;

typedef struct Task {
	int id; // 0
	UInt32 esp, ebp; // 4-11
	UInt32 eip; // 12
	PageDirectory* dir; // 16
	struct Task* next; // 20
	int sleepTill; // 24:  The tick amount to sleep until.
	ProcessInfo* processInfo; // 28: The process info.  This includes open file descriptors.
} Task;

void InitTasking();

void TaskSwitch();

int CreateTask(UInt32 start, PageDirectory* dir);

int GetPID();

#endif
