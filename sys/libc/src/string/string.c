/*
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#include "string/string.h"

// Copy string from src to dest
void copyStr(char* dest, const char* src) {
    int i = 0;
    while (src[i] && i < MAX_NAME_LEN - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// Check if two strings are equal
int strEqual(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}

// Get length of string
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

// Split string by delimiter
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

    output[part][j] = '\0'; // null terminate
    return part + 1; // returns number of parts found
}

// Concatenate strings
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

// Strip whitespace from string
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

// Memory copy function
void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// Memory set function
void* memset(void* ptr, int value, size_t n) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < n; i++) {
        p[i] = (unsigned char)value;
    }
    return ptr;
}

void itoa(int value, char* str, int base) {
    char* p = str;
    char* p1, *p2;
    unsigned int abs_value = (value < 0) ? -value : value;
    if (base < 2 || base > 36) {
        *str = '\0';
        return;
    }
    int is_negative = (value < 0 && base == 10);
    do {
        int rem = abs_value % base;
        *p++ = (rem < 10) ? rem + '0' : rem - 10 + 'a';
        abs_value /= base;
    } while (abs_value);
    if (is_negative) *p++ = '-';
    *p = '\0';
    // Reverse string
    for (p1 = str, p2 = p - 1; p1 < p2; ++p1, --p2) {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
    }
}

char* utoa(unsigned int value, char* str, int base) {
    char* rc = str;
    char* ptr = str;
    char* low;
    // Generate digits in reverse order
    do {
        int rem = value % base;
        *ptr++ = (rem < 10) ? rem + '0' : rem - 10 + 'a';
        value /= base;
    } while (value);
    *ptr-- = '\0';
    // Reverse the string
    for (low = str; low < ptr; ++low, --ptr) {
        char tmp = *low;
        *low = *ptr;
        *ptr = tmp;
    }
    return rc;
}