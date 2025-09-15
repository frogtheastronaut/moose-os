/*
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#include "../include/stdlib.h"

// Convert integer to string
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

// Convert string to integer
int atoi(const char* str) {
    if (!str) return 0;
    
    int result = 0;
    int sign = 1;
    int i = 0;
    
    // Skip leading whitespace
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
        i++;
    }
    
    // Check for sign
    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }
    
    // Convert digits
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return result * sign;
}