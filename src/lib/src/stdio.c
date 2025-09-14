/*
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#include "../include/stdio.h"

// snprintf implementation for MooseOS
int msnprintf(char *buffer, int size, const char *format, ...) {
    if (!buffer || size <= 0) return 0;
    if (size == 1) {
        buffer[0] = '\0';
        return 0;
    }
    
    va_list args;
    va_start(args, format);

    int written = 0;
    char *buf_ptr = buffer;
    const char *fmt_ptr = format;

    while (*fmt_ptr && written < size - 1) {
        if (*fmt_ptr == '%' && *(fmt_ptr + 1) == 's') {
            const char *str = va_arg(args, const char *);
            if (str) {  // Check for NULL pointer
                while (*str && written < size - 1) {
                    *buf_ptr++ = *str++;
                    written++;
                }
            }
            fmt_ptr += 2;
        } else if (*fmt_ptr == '%' && *(fmt_ptr + 1) == 'd') {
            int num = va_arg(args, int);
            char temp[12];  // enough for 32-bit int
            int temp_len = 0;
            
            if (num == 0) {
                temp[temp_len++] = '0';
            } else {
                int is_negative = 0;
                if (num < 0) {
                    is_negative = 1;
                    num = -num;
                }
                
                while (num > 0) {
                    temp[temp_len++] = '0' + (num % 10);
                    num /= 10;
                }
                
                if (is_negative) {
                    temp[temp_len++] = '-';
                }
                
                // reverse the digits
                for (int i = 0; i < temp_len / 2; i++) {
                    char swap = temp[i];
                    temp[i] = temp[temp_len - 1 - i];
                    temp[temp_len - 1 - i] = swap;
                }
            }
            
            for (int i = 0; i < temp_len && written < size - 1; i++) {
                *buf_ptr++ = temp[i];
                written++;
            }
            fmt_ptr += 2;
        } else if (*fmt_ptr == '%' && *(fmt_ptr + 1) == '%') {
            if (written < size - 1) {
                *buf_ptr++ = '%';
                written++;
            }
            fmt_ptr += 2;
        } else {
            *buf_ptr++ = *fmt_ptr;
            written++;
            fmt_ptr++;
        }
    }
    
    *buf_ptr = '\0';  // Always null terminate
    va_end(args);
    return written;
}