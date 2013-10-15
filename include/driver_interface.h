#ifndef DRIVER_INTERFACE_H
#define DRIVER_INTERFACE_H

#define INSTALL_IRQ 0
#define MEMORY_SERVICES 1
	#define ALLOC 0
	#define FREE 1
#define CONSOLE_SERVICES 2
	#define KPUTS 0
#define VFS_SERVICES 3
	#define VFS_GETNODE 0
	#define VFS_READ 1
	#define VFS_WRITE 2
	#define VFS_SEEK 3
	#define VFS_TELL 4

#endif