/*
    MooseOS RTC Driver
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "rtc/rtc.h"

// days in each month
/**
 * @todo: fix this when we land on Mars
 */
static const uint8_t days_in_month[13] = {
    0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

// timezone offset
int timezone_offset = 0;  // UTC +0

// read from RTC register
uint8_t rtc_read_register(uint8_t reg) {
    outb(CMOS_REGISTER_A, reg);
    return inb(CMOS_REGISTER_B);
}

// check if RTC is updating 
int rtc_is_updating(void) {
    outb(CMOS_REGISTER_A, RTC_STATUS_A);
    return (inb(CMOS_REGISTER_B) & 0x80);
}

// get the rtc time
rtc_time rtc_get_time(void) {
    rtc_time time;
    
    // wait for rtc to stop updating
    while (rtc_is_updating());
    
    // get the time
    time.seconds = rtc_read_register(RTC_SECONDS);
    time.minutes = rtc_read_register(RTC_MINUTES);
    time.hours   = rtc_read_register(RTC_HOURS);
    time.day     = rtc_read_register(RTC_DAY);
    time.month   = rtc_read_register(RTC_MONTH);
    time.year    = rtc_read_register(RTC_YEAR);

    int hours = (int)time.hours + timezone_offset;

    // wrap if hours > 24 or < -24
    while (hours > 24) {
        hours -= 24;
        time.day++;
        uint8_t max_day = days_in_month[time.month];
        if (time.month == 2) {
            // check leap year
            // this stops at 2100, but oh well (Y2K again)
            uint16_t full_year = 2000 + time.year;
            if ((full_year % 4 == 0 && full_year % 100 != 0) || (full_year % 400 == 0)) {
                max_day = 29;
            }
        }
        // day overflow handling
        if (time.day > max_day) {
            time.day = 1;
            time.month++;
            if (time.month > 12) {
                time.month = 1;
                time.year++;
            }
        }
    }

    while (hours <= -24) {
        hours += 24;
        time.day--;
        uint8_t max_day = days_in_month[time.month];
        if (time.month == 2) {
            // check leap year
            uint16_t full_year = 2000 + time.year;
            if ((full_year % 4 == 0 && full_year % 100 != 0) || (full_year % 400 == 0)) {
                max_day = 29;
            }
        }
        if (time.day < 1) {
            time.month--;
            if (time.month < 1) {
                time.month = 12;
                time.year--;
            }
            // recalculate max_day for new month
            max_day = days_in_month[time.month];
            if (time.month == 2) {
                uint16_t full_year = 2000 + time.year;
                if ((full_year % 4 == 0 && full_year % 100 != 0) || (full_year % 400 == 0)) {
                    max_day = 29;
                }
            }
            time.day = max_day;
        }
    }

    if (hours < 0) hours += 24;
    if (hours >= 24) hours -= 24;
    time.hours = (uint8_t)hours;
    return time;
}

void rtc_init(void) {
    // disable interrupts
    cli();

    // set status for register B
    outb(CMOS_REGISTER_A, RTC_STATUS_B);
    uint8_t status_b = inb(CMOS_REGISTER_B);
    
    // set rtc to 24 hour, binary mode
    /** @note: 0x02 = 24-hour format, 0x04 = binary mode */
    outb(CMOS_REGISTER_A, RTC_STATUS_B);
    outb(CMOS_REGISTER_B, status_b | 0x02 | 0x04); 

    // disables rtc interrupts for now
    outb(CMOS_REGISTER_A, RTC_STATUS_C);
    inb(CMOS_REGISTER_B);  // clears interrupts

    // enable interrupts
    sti();

    // verify RTC is working by testing the time
    rtc_time test_time = rtc_get_time();
}

