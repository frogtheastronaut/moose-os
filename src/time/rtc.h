#ifndef RTC_H
#define RTC_H

#include "../lib/lib.h"

typedef unsigned short uint16_t;
typedef short int16_t;

// some ports
#define RTC_INDEX_PORT 0x70
#define RTC_DATA_PORT  0x71
#define RTC_STATUS_C    0x0C

// register addresses 
#define RTC_SECONDS     0x00
#define RTC_MINUTES     0x02
#define RTC_HOURS       0x04
#define RTC_DAY         0x07
#define RTC_MONTH       0x08
#define RTC_YEAR        0x09
#define RTC_STATUS_A    0x0A
#define RTC_STATUS_B    0x0B

// this is rtc_time
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} rtc_time;

rtc_time rtc_gettime(void);
void rtc_init(void);

extern int timezone_offset;

#endif