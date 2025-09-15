/*
    MooseOS 
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

/*    
    ====================== OS THEORY ======================
    If you haven't read other OS theory files, basically MooseOS is an educational OS, so comments at the top of each 
    file will explain the relevant OS theory. This is so that users can learn about OS concepts while reading the code, 
    and maybe even make their own OS some day. 
    Usually, there are external websites that describe OS Theory excellently. They will be quoted, and a link
    will be provided.

    WHAT IS RTC?
    The Real-Time Clock (RTC) is a hardware component that keeps track of the current time
    and date, even when the computer is powered off. Within the chip is 64 bits of CMOS ram.

    Typically, an OS will use the APIC or PIT for timing purposes and will depend on the RTC 
    only to sync its clock at boot time. However, MooseOS does not yet have APIC or PIT support.

    Our RTC driver reads the current time from the RTC chip.
/*

/**
    @todo Some comments here do not always use proper grammar/are concise.
          They are still readable, so that is not an immediate priority.
 */

#include "../include/rtc.h"

static const uint8_t days_in_month[13] = {
    0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
// timezone offset
int timezone_offset = 0;  // UTC +0

// read from RTC register
uint8_t rtc_readregister(uint8_t reg) {
    outb(RTC_INDEX_PORT, reg);
    return inb(RTC_DATA_PORT);
}

// check if RTC is updating 
int rtc_is_updating(void) {
    outb(RTC_INDEX_PORT, RTC_STATUS_A);
    return (inb(RTC_DATA_PORT) & 0x80);
}

// Gets the rtc time
rtc_time rtc_gettime(void) {
    rtc_time time;
    
    // wait for rtc to stop updating
    while (rtc_is_updating());
    
    // Get the time
    time.seconds = rtc_readregister(RTC_SECONDS);
    time.minutes = rtc_readregister(RTC_MINUTES);
    time.hours   = rtc_readregister(RTC_HOURS);
    time.day     = rtc_readregister(RTC_DAY);
    time.month   = rtc_readregister(RTC_MONTH);
    time.year    = rtc_readregister(RTC_YEAR);

    int hours = (int)time.hours + timezone_offset;

    // Wrap if hours > 24 or < -24
    while (hours > 24) {
        hours -= 24;
        time.day++;
        uint8_t max_day = days_in_month[time.month];
        if (time.month == 2) {
            // checkleap year
            uint16_t full_year = 2000 + time.year; // crude, but RTC returns 0-99
            if ((full_year % 4 == 0 && full_year % 100 != 0) || (full_year % 400 == 0)) {
                max_day = 29;
            }
        }
        // adds adds ADDS
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
            // Recalculate max_day for new month
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
    // Disable interrupts
    cli();
    
    // set status for register b
    outb(RTC_INDEX_PORT, RTC_STATUS_B);
    uint8_t status_b = inb(RTC_DATA_PORT);
    
    // set rtc to 24 hour, binary mode
    /** @note: 0x02 = 24-hour format, 0x04 = binary mode */
    outb(RTC_INDEX_PORT, RTC_STATUS_B);
    outb(RTC_DATA_PORT, status_b | 0x02 | 0x04); 
    // Disables rtc interrupts for now
    outb(RTC_INDEX_PORT, RTC_STATUS_C);
    inb(RTC_DATA_PORT);  // clears interrupts

    // Enable interrupts
    sti();
    
    // verify rtc is working
    rtc_time test_time = rtc_gettime();
}

