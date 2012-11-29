#ifndef RTC_H
#define RTC_H

#include <KOSTypes.h>

typedef struct __rtc_date {
	UInt8 seconds;
	UInt8 minutes;
	UInt8 hours;
	UInt8 weekday;
	UInt8 day_of_month;
	UInt8 month;
	UInt16 year;
} RtcDate;

RtcDate GetRtcDate();
void PrintRtcDate(RtcDate rd);

#endif
