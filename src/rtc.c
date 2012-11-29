#include <rtc.h>
#include <print.h>

#define SECONDS_REG 0
#define MINUTES_REG 2
#define HOURS_REG 4
#define WEEKDAY_REG 6
#define DOM_REG 7
#define MONTH_REG 8
#define YEAR_REG 9

RtcDate GetRtcDate() {
	RtcDate ret;
	
	outb(0x70, SECONDS_REG);
	ret.seconds = inb(0x71);
	
	outb(0x70, MINUTES_REG);
	ret.minutes = inb(0x71);
	
	outb(0x70, HOURS_REG);
	ret.hours = inb(0x71);
	
	outb(0x70, WEEKDAY_REG);
	ret.weekday = inb(0x71);
	
	outb(0x70, DOM_REG);
	ret.day_of_month = inb(0x71);
	
	outb(0x70, MONTH_REG);
	ret.month = inb(0x71);
	
	outb(0x70, YEAR_REG);
	ret.year = inb(0x71);

	outb(0x70, 0xb);
	UInt8 status_reg_b = inb(0x71);
	kprintf("status_reg_b==%x\n", status_reg_b);
	
	if((status_reg_b&2)==0 && (status_reg_b&4)==0) {
		ret.seconds = ((ret.seconds / 16) * 10) + (ret.seconds & 0xF);
		ret.minutes = ((ret.minutes / 16) * 10) + (ret.minutes & 0xF);
		ret.hours = ((ret.hours / 16) * 10) + (ret.hours & 0xF);
		ret.weekday = ((ret.weekday / 16) * 10) + (ret.weekday & 0xF);
		ret.day_of_month = ((ret.day_of_month / 16) * 10) + (ret.day_of_month & 0xF);
		ret.month = ((ret.month / 16) * 10) + (ret.month & 0xF);
		ret.year = ((ret.year / 16) * 10) + (ret.year & 0xF);
	} else if(status_reg_b&2) {
		
	}
	
	return ret;
}

void PrintRtcDate(RtcDate rd) {
	kprintf("%d:%d:%d, %d/%d/%d", rd.hours, rd.minutes, rd.seconds, rd.day_of_month, rd.month, rd.year);
}
