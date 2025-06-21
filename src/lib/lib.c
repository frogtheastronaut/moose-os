#include <stdio.h>
#define MAX_NAME_LEN 128
typedef char *va_list;
#define _INTSIZEOF(n)    ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )
#define va_start(ap,v)   ( ap = (va_list)&v + _INTSIZEOF(v) )
#define va_arg(ap,t)     ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)       ( ap = (va_list)0 )

void copyStr(char* dest, const char* src) {
    int i = 0;
    while (src[i] && i < MAX_NAME_LEN - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// Basic string compare
int strEqual(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}

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