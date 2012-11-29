#include <pit.h>
#include <isr.h>
#include <print.h>

UInt32 tick = 0;
UInt32 seconds = 0;

UInt32 mstime = 0;
UInt32 frequency = 0;

PITHook pithook = NULL;

Bool interval_enable = 0;
char interval_c;
UInt32 u_interval;

UInt64 last_rdtsc = 0;
UInt64 __rdtsc = 0;

UInt32 GetTicks() {
	return tick;
}

UInt32 GetSeconds() {
	return tick/(1000/mstime);
}

void wait(int ticks) {
	UInt32 wait_ticks = tick+ticks;
	while(tick<=wait_ticks) {}
}

extern void timer_callback();

void EnableLoadIndicator(char c, UInt32 interval) {
	interval_c = (c) ? c : interval_c;
	u_interval = (interval) ? interval : u_interval;
	interval_enable = 1;
}

void DisableLoadIndicator() {
	interval_enable = false;
}

void PIT_Init(UInt32 ms_time) {
	registerIntHandler(IRQ0, &timer_callback);
	
	UInt32 freq = frequency = 1000/ms_time;
	mstime = ms_time;
	
	UInt32 divisor = 1193180 / freq;
	
	outb(0x43, 0x36);
	
	UInt8 l = (UInt8)(divisor & 0xFF);
	UInt8 h = (UInt8)(divisor>>8) & 0xFF;
	
	outb(0x40, l);
	outb(0x40, h);
	
}

void InitPITHook(PITHook h) {
	pithook = h;
}

void DeactivatePITHook() {
	pithook = NULL;
}

UInt32 GetSpeed() {
	return (__rdtsc-last_rdtsc)*frequency;
}
