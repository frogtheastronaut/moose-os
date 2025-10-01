/*
    MooseOS Stdio code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "stdio/stdio.h"

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
            if (str) {  // check for NULL pointer
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
        } else if (*fmt_ptr == '%' && *(fmt_ptr + 1) == 'u') {
            unsigned int num = va_arg(args, unsigned int);
            char temp[12];  // enough for 32-bit unsigned int
            int temp_len = 0;
            
            if (num == 0) {
                temp[temp_len++] = '0';
            } else {
                while (num > 0) {
                    temp[temp_len++] = '0' + (num % 10);
                    num /= 10;
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
        } else if (*fmt_ptr == '%' && fmt_ptr[1] == '0' && 
                   (fmt_ptr[3] == 'x' || fmt_ptr[3] == 'X' || 
                    fmt_ptr[4] == 'x' || fmt_ptr[4] == 'X')) {
            // handle padded hex like %08X or %06X
            int width = 0;
            int fmt_len = 2;  // start after %0
            
            // parse width
            while (fmt_ptr[fmt_len] >= '0' && fmt_ptr[fmt_len] <= '9') {
                width = width * 10 + (fmt_ptr[fmt_len] - '0');
                fmt_len++;
            }
            
            char hex_base = (fmt_ptr[fmt_len] == 'X') ? 'A' : 'a';
            unsigned int num = va_arg(args, unsigned int);
            
            char temp[10];  // enough for 32-bit hex
            int temp_len = 0;
            
            if (num == 0) {
                temp[temp_len++] = '0';
            } else {
                while (num > 0) {
                    int digit = num % 16;
                    if (digit < 10) {
                        temp[temp_len++] = '0' + digit;
                    } else {
                        temp[temp_len++] = hex_base + (digit - 10);
                    }
                    num /= 16;
                }
                
                // reverse the digits
                for (int i = 0; i < temp_len / 2; i++) {
                    char swap = temp[i];
                    temp[i] = temp[temp_len - 1 - i];
                    temp[temp_len - 1 - i] = swap;
                }
            }
            
            // add leading zeros if needed
            int padding = width - temp_len;
            while (padding > 0 && written < size - 1) {
                *buf_ptr++ = '0';
                written++;
                padding--;
            }

            // output the hex digits
            for (int i = 0; i < temp_len && written < size - 1; i++) {
                *buf_ptr++ = temp[i];
                written++;
            }
            fmt_ptr += fmt_len + 1;
        } else if (*fmt_ptr == '%' && (*(fmt_ptr + 1) == 'x' || *(fmt_ptr + 1) == 'X')) {
            unsigned int num = va_arg(args, unsigned int);
            char temp[10];  // enough for 32-bit hex (8 chars + null)
            int temp_len = 0;
            char hex_base = (*(fmt_ptr + 1) == 'X') ? 'A' : 'a';
            
            if (num == 0) {
                temp[temp_len++] = '0';
            } else {
                while (num > 0) {
                    int digit = num % 16;
                    if (digit < 10) {
                        temp[temp_len++] = '0' + digit;
                    } else {
                        temp[temp_len++] = hex_base + (digit - 10);
                    }
                    num /= 16;
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
    
    *buf_ptr = '\0';  // null terminate
    va_end(args);
    return written;
}