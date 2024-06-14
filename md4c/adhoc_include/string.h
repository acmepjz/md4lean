#ifndef FAKE_STRING_H
#define FAKE_STRING_H

#include <stddef.h>

#ifdef __cplusplus
    extern "C" {
#endif

size_t strlen(const char* str);
int strncmp(const char* str1, const char* str2, size_t num);
char* strchr(const char* str, int character);
int memcmp(const void* ptr1, const void* ptr2, size_t num);
void* memcpy(void* destination, const void* source, size_t num);
void* memmove(void* destination, const void* source, size_t num);
void* memset(void* ptr, int value, size_t num);

#ifdef __cplusplus
    }
#endif

#endif
