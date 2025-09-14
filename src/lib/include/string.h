#ifndef STRING_H_
#define STRING_H_

#include <stddef.h>

#define MAX_NAME_LEN 128
#define MAX_PARTS 10
#define MAX_PART_LEN 32

// String function prototypes
void copyStr(char* dest, const char* src);
int strEqual(const char* a, const char* b);
size_t strlen(const char* str);
int split_string(const char *input, char delimiter, char output[MAX_PARTS][MAX_PART_LEN]);
char* strcat(char* dest, const char* src);
const char* strip_whitespace(const char* str);

// Memory function prototypes
void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* ptr, int value, size_t n);

#endif
