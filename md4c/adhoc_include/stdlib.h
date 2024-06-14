#ifndef FAKE_STDLIB_H
#define FAKE_STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
    extern "C" {
#endif

void* bsearch(const void* key, const void* base, size_t num, size_t size, int (*compar)(const void*, const void*));
void qsort(void* base, size_t num, size_t size, int (*compar)(const void*, const void*));
void* malloc(size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);

#ifdef __cplusplus
    }
#endif

#endif
