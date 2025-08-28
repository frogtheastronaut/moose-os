/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/


#include "lib.h"

#define MAX_NAME_LEN 128
#define MAX_PARTS 10
#define MAX_PART_LEN 32


// don't worry, i don't get this either
// taken from GNU C library because i don't have these includes
typedef char *va_list;
#define _INTSIZEOF(n)    ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )
#define va_start(ap,v)   ( ap = (va_list)&v + _INTSIZEOF(v) )
#define va_arg(ap,t)     ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)       ( ap = (va_list)0 )

// copy string from src to dest
void copyStr(char* dest, const char* src) {
    int i = 0;
    while (src[i] && i < MAX_NAME_LEN - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// basically "" == ""
int strEqual(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}
// snprintf but its mine HAHAHAHA
#include <stddef.h>
#ifndef MOOSEOS_TYPES_H
#define MOOSEOS_TYPES_H

#ifndef __cplusplus
#ifndef bool
#define bool _Bool
#define true 1
#define false 0
#endif
#endif
#include <stddef.h>
typedef unsigned short uint16_t;
typedef short int16_t;

#endif // MOOSEOS_TYPES_H

int msnprintf(char *buffer, int size, const char *format, ...) {
    va_list args;
    va_start(args, format);

    int written = 0;
    char *buf_ptr = buffer;
    const char *fmt_ptr = format;

    while (*fmt_ptr) {
        if (*fmt_ptr == '%' && *(fmt_ptr + 1) == 's') {
            const char *str = va_arg(args, const char *);
            while (*str) {
                if (written + 1 < size) {
                    *buf_ptr++ = *str;
                }
                written++;
                str++;
            }
            fmt_ptr += 2;
        } else if (*fmt_ptr == '%' && *(fmt_ptr + 1) == '%') {
            if (written + 1 < size) {
                *buf_ptr++ = '%';
            }
            written++;
            fmt_ptr += 2;
        } else {
            if (written + 1 < size) {
                *buf_ptr++ = *fmt_ptr;
            }
            written++;
            fmt_ptr++;
        }
    }
    if (size > 0) {
        *buf_ptr = '\0';
    }
    va_end(args);
    return written;
}
// get length of string
size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

int split_string(const char *input, char delimiter, char output[MAX_PARTS][MAX_PART_LEN]) {
    int part = 0;   
    int i = 0;      
    int j = 0;       

    while (input[i] != '\0') {
        if (input[i] == delimiter) {
            output[part][j] = '\0'; // null terminate
            part++;
            if (part >= MAX_PARTS) break;
            j = 0;
        } else {
            if (j < MAX_PART_LEN - 1) {
                output[part][j] = input[i];
                j++;
            }
        }
        i++;
    }

    output[part][j] = '\0'; // nulls terminates
    return part + 1; // returns number of parts found
}

int int2str(int num, char* buffer, int buffer_size) {
    if (buffer_size <= 1) {
        if (buffer_size == 1) buffer[0] = '\0';
        return 0;
    }
    
    int is_negative = 0;
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    int i = 0;
    

    if (num == 0) {
        buffer[i++] = '0';
    } else {
        while (num > 0 && i < buffer_size - 1) {
            buffer[i++] = '0' + (num % 10);
            num /= 10;
        }
    }
    
    if (is_negative && i < buffer_size - 1) {
        buffer[i++] = '-';
    }
    
    buffer[i] = '\0';
    
    // reverse string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
    
    return i;
}

char* strcat(char* dest, const char* src) {
    int dest_len = 0;
    while (dest[dest_len] != '\0') {
        dest_len++;
    }
    
    int i = 0;
    while (src[i] != '\0') {
        dest[dest_len + i] = src[i];
        i++;
    }
    
    dest[dest_len + i] = '\0';
    
    return dest;
}

uint8_t inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

void cli(void) {
    asm volatile("cli");
}

void sti(void) {
    asm volatile("sti");
}

const char* strip_whitespace(const char* str) {
    static char trimmed[MAX_NAME_LEN];
    int start = 0;
    int end;
    int len = strlen(str);
    
    while (start < len && (str[start] == ' ' || str[start] == '\t')) {
        start++;
    }
    
    if (start >= len) {
        trimmed[0] = '\0';
        return trimmed;
    }
    
    end = len - 1;
    while (end >= start && (str[end] == ' ' || str[end] == '\t')) {
        end--;
    }
    
    int i;
    for (i = 0; i <= end - start; i++) {
        trimmed[i] = str[start + i];
    }
    trimmed[i] = '\0';
    
    return trimmed;
}