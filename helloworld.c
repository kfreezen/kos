#include <print.h>

void _start() {
	kprintf("Hello, world%s\n", "It's working");
	void* p = kalloc(sizeof(4));
	kprintf("allocated %x\n", p);
	kfree(p);
	return;
}