/*
    MooseOS RTC Driver
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#ifndef RTC_H
#define RTC_H

#include "libc/lib.h"

typedef unsigned short uint16_t;
typedef short int16_t;

// CMOS I/O ports
#define CMOS_REGISTER_A 0x70
#define CMOS_REGISTER_B  0x71

// RTC register Addresses
#define RTC_SECONDS     0x00
#define RTC_MINUTES     0x02
#define RTC_HOURS       0x04
#define RTC_DAY         0x07
#define RTC_MONTH       0x08
#define RTC_YEAR        0x09
#define RTC_STATUS_A    0x0A
#define RTC_STATUS_B    0x0B
#define RTC_STATUS_C    0x0C

// RTC time
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} rtc_time;

// functions
rtc_time rtc_get_time(void);
void rtc_init(void);

// external variables
extern int timezone_offset;

#endif // RTC_H