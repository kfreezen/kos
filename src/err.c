#include <err.h>

int errno;

int GetErr() {
	return errno;
}

void SetErr(int err) {
	errno = err;
}