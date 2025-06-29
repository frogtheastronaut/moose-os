/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/

#include <stdio.h>
#include <stdint.h>

#define MAX_NAME_LEN 128


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




#define MAX_PARTS 10
#define MAX_PART_LEN 32
int split_string(const char *input, char delimiter, char output[MAX_PARTS][MAX_PART_LEN]) {
    int part = 0;    // Index for output array
    int i = 0;       // Index in input string
    int j = 0;       // Index in current output part

    while (input[i] != '\0') {
        if (input[i] == delimiter) {
            output[part][j] = '\0'; // Null-terminate the current string
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

    output[part][j] = '\0'; // Null-terminate the last part
    return part + 1; // Return number of parts found
}

/**
 * Simple integer to string conversion function
 * 
 * @param num The integer to convert
 * @param buffer The output buffer
 * @param buffer_size Size of the buffer
 * @return The length of the resulting string
 */
int int_to_str(int num, char* buffer, int buffer_size) {
    // Handle edge cases
    if (buffer_size <= 1) {
        if (buffer_size == 1) buffer[0] = '\0';
        return 0;
    }
    
    // Handle negative numbers
    int is_negative = 0;
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    // Find end of buffer for reverse writing
    int i = 0;
    
    // Handle special case of 0
    if (num == 0) {
        buffer[i++] = '0';
    } else {
        // Convert digits
        while (num > 0 && i < buffer_size - 1) {
            buffer[i++] = '0' + (num % 10);
            num /= 10;
        }
    }
    
    // Add negative sign if needed
    if (is_negative && i < buffer_size - 1) {
        buffer[i++] = '-';
    }
    
    // Null-terminate the string
    buffer[i] = '\0';
    
    // Reverse the string
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

/**
 * Concatenates source string to destination string
 * 
 * @param dest The destination string
 * @param src The source string to append to dest
 * @return Pointer to the destination string
 */
char* strcat(char* dest, const char* src) {
    // Find the end of the destination string
    int dest_len = 0;
    while (dest[dest_len] != '\0') {
        dest_len++;
    }
    
    // Copy the source string to the end of the destination string
    int i = 0;
    while (src[i] != '\0') {
        dest[dest_len + i] = src[i];
        i++;
    }
    
    // Null-terminate the concatenated string
    dest[dest_len + i] = '\0';
    
    return dest;
}

/**
 * Read a byte from an I/O port
 * 
 * @param port The port number to read from
 * @return The byte value read from the port
 */
uint8_t inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/**
 * Write a byte to an I/O port
 * 
 * @param port The port number to write to
 * @param data The byte value to write to the port
 */
void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

/**
 * Disable interrupts
 */
void cli(void) {
    asm volatile("cli");
}

/**
 * Enable interrupts
 */
void sti(void) {
    asm volatile("sti");
}

/**
 * Strip leading whitespace from a string
 */
static const char* strip_leading_spaces(const char* str) {
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    return str;
}

/**
 * Strip leading and trailing whitespace from a string
 * Returns a pointer to a static buffer with the trimmed string
 */
const char* strip_whitespace(const char* str) {
    static char trimmed[MAX_NAME_LEN];
    int start = 0;
    int end;
    int len = strlen(str);
    
    // Find first non-whitespace character
    while (start < len && (str[start] == ' ' || str[start] == '\t')) {
        start++;
    }
    
    // If string is all whitespace
    if (start >= len) {
        trimmed[0] = '\0';
        return trimmed;
    }
    
    // Find last non-whitespace character
    end = len - 1;
    while (end >= start && (str[end] == ' ' || str[end] == '\t')) {
        end--;
    }
    
    // Copy trimmed string
    int i;
    for (i = 0; i <= end - start; i++) {
        trimmed[i] = str[start + i];
    }
    trimmed[i] = '\0';
    
    return trimmed;
}