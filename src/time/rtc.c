#include <stdint.h>
#include "../lib/lib.h"
#define RTC_INDEX_PORT 0x70
#define RTC_DATA_PORT  0x71
#define RTC_STATUS_C    0x0C

// RTC register addresses
#define RTC_SECONDS     0x00
#define RTC_MINUTES     0x02
#define RTC_HOURS       0x04
#define RTC_DAY         0x07
#define RTC_MONTH       0x08
#define RTC_YEAR        0x09
#define RTC_STATUS_A    0x0A
#define RTC_STATUS_B    0x0B

// Global timezone offset in hours (can be negative)
static int timezone_offset = 0;  // UTC +0

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} rtc_time_t;

// Read from RTC register
uint8_t rtc_read_register(uint8_t reg) {
    outb(RTC_INDEX_PORT, reg);
    return inb(RTC_DATA_PORT);
}

// Convert BCD to binary (RTC often stores time in BCD format)
uint8_t bcd_to_binary(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Check if RTC is updating (wait for stable read)
int rtc_is_updating(void) {
    outb(RTC_INDEX_PORT, RTC_STATUS_A);
    return (inb(RTC_DATA_PORT) & 0x80);
}

// Get current time from RTC
rtc_time_t rtc_get_time(void) {
    rtc_time_t time;
    
    // Wait for RTC to not be updating
    while (rtc_is_updating());
    
    time.seconds = rtc_read_register(RTC_SECONDS);
    time.minutes = rtc_read_register(RTC_MINUTES);
    time.hours   = rtc_read_register(RTC_HOURS);
    time.day     = rtc_read_register(RTC_DAY);
    time.month   = rtc_read_register(RTC_MONTH);
    time.year    = rtc_read_register(RTC_YEAR);
    
    // Check if RTC is in BCD mode
    uint8_t status_b = rtc_read_register(RTC_STATUS_B);
    if (!(status_b & 0x04)) {
        // Convert from BCD to binary
        time.seconds = bcd_to_binary(time.seconds);
        time.minutes = bcd_to_binary(time.minutes);
        time.hours   = bcd_to_binary(time.hours);
        time.day     = bcd_to_binary(time.day);
        time.month   = bcd_to_binary(time.month);
        time.year    = bcd_to_binary(time.year);
    }
    
    // Handle 12-hour format if needed
    if (!(status_b & 0x02) && (time.hours & 0x80)) {
        time.hours = ((time.hours & 0x7F) + 12) % 24;
    }
    
    return time;
}

void rtc_init(void) {
    // Disable interrupts while configuring RTC
    cli();
    
    // Select status register B
    outb(RTC_INDEX_PORT, RTC_STATUS_B);
    uint8_t status_b = inb(RTC_DATA_PORT);
    
    // Set the RTC to 24-hour format and binary mode
    outb(RTC_INDEX_PORT, RTC_STATUS_B);
    outb(RTC_DATA_PORT, status_b | 0x02 | 0x04);  // Set bits 1 and 2
    // Bit 1 (0x02): 24-hour format
    // Bit 2 (0x04): Binary mode (not BCD)
    
    // Clear any pending RTC interrupts
    outb(RTC_INDEX_PORT, RTC_STATUS_C);
    inb(RTC_DATA_PORT);  // Reading this register clears interrupts
    
    // Re-enable interrupts
    sti();
    
    // Verify RTC is working by reading time once
    rtc_time_t test_time = rtc_get_time();
    
    // You could add validation here to ensure time is reasonable
    // For example, check if year is between 2020-2030, etc.
}

/**
 * Set timezone offset in hours
 */
void rtc_set_timezone_offset(int offset_hours) {
    timezone_offset = offset_hours;
}

/**
 * Get timezone offset in hours
 */
int rtc_get_timezone_offset(void) {
    return timezone_offset;
}

/**
 * Get current time with timezone offset applied
 */
rtc_time_t rtc_get_local_time(void) {
    rtc_time_t time = rtc_get_time();
    
    // Apply timezone offset to hours
    int adjusted_hours = (int)time.hours + timezone_offset;
    
    time.hours = (uint8_t)adjusted_hours;
    return time;
}
