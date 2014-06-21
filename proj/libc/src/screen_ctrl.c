#include <sys/screen_ctrl.h>

#define SCREEN_CLEAR 0x00
#define SCREEN_MOVE 0x01

inline void SetupBuf(unsigned char* buf) {
	buf[0] = 0xaa;
	buf[1] = 0x55;
	buf[2] = '\e';
}

void ScreenCtrl_Clear(int fd, int color) {
	// \xaa\x55\e\x07
	unsigned char buf[5];
	SetupBuf(buf);

	buf[3] = SCREEN_CLEAR;
	buf[4] = (unsigned char) color;

	write(fd, buf, 5);
}

void ScreenCtrl_Move(int fd, int x, int y) {
	unsigned char buf[6];
	SetupBuf(buf);

	buf[3] = SCREEN_MOVE;
	buf[4] = x;
	buf[5] = y;

	write(fd, buf, 6);
}