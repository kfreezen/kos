#include <sys/syscalls.h>
#include <sys/screen_ctrl.h>

void _start() {
	int screenFd = open("/dev/screen", O_WRITE);
	if(screenFd >= 0) {
		ScreenCtrl_Clear(screenFd, 0x70);
		ScreenCtrl_Move(screenFd, 0, 0);

		write(screenFd, "Hello\n", 6);
	}

	while(1) {
		asm("hlt");
	}
}