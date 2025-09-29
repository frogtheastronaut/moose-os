/*
    MooseOS String.h implementation - string and memory operations
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "string/string.h"

// copy string from src to dest
void strcpy(char* dest, const char* src) {
    int i = 0;
    while (src[i] && i < MAX_NAME_LEN - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// check if two strings are equal
/**
 * @note this is different from standard strcmp, which returns
 *       <0, 0, >0 for less than, equal, greater than respectively
 *       this function returns 1 for equal, 0 for not equal
 */
int strcmp(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}

// get length of string
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

// split string by delimiter
int strtok(const char *input, char delimiter, char output[MAX_PARTS][MAX_PART_LEN]) {
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

// concat strings
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

// strip whitespace from string
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

// memory copy function
void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// memset function
void* memset(void* ptr, int value, size_t n) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < n; i++) {
        p[i] = (unsigned char)value;
    }
    return ptr;
}