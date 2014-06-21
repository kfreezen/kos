#include <sys/syscalls.h>

extern unsigned syscall1(int syscall);
extern unsigned syscall2(int syscall, const unsigned arg0);
extern unsigned syscall3(int syscall, const unsigned arg0, const unsigned arg1);
extern unsigned syscall4(int syscall, const unsigned arg0, const unsigned arg1, const unsigned arg2);

int open(const char* pathName, int flags) {
	return syscall3(SYSCALL_OPEN, (const unsigned) pathName, (const unsigned) flags);
}

int write(int fd, const char* buf, int count) {
	return syscall4(SYSCALL_WRITE, (unsigned) fd, (const unsigned) buf, (const unsigned) count);
}