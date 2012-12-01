#include <emulator_syscalls.h>
#include <print.h>
#include <screenapi.h>

typedef unsigned (*SystemCall)(void* ptr);

unsigned PutString_SysCall(unsigned syscall_addr, Byte n, void* str) {
	PutString((char*) str);
	return 0;
}

unsigned ClearScreen_SysCall(unsigned syscall_addr, Byte n) {
	ClearScreen();
	return 0;
}

extern unsigned syscall(void);

unsigned syscallna(Byte n) {
	return syscall();
}
