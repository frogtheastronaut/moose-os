#ifndef STRING_H_
#define STRING_H_

#include <stddef.h>

#define MAX_NAME_LEN 128
#define MAX_PARTS 10
#define MAX_PART_LEN 32

// copy string from src to dest
static inline void copyStr(char* dest, const char* src) {
    int i = 0;
    while (src[i] && i < MAX_NAME_LEN - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// basically "" == ""
static inline int strEqual(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}

// get length of string
static inline size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

// split string by delimiter
static inline int split_string(const char *input, char delimiter, char output[MAX_PARTS][MAX_PART_LEN]) {
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

// concatenate strings
static inline char* strcat(char* dest, const char* src) {
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
static inline const char* strip_whitespace(const char* str) {
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

#endif
