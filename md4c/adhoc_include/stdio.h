#ifndef FAKE_STDIO_H
#define FAKE_STDIO_H

#include <stddef.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct FILE FILE;

int _snprintf(char* buffer, size_t count, const char *format, ...);
int fprintf(FILE* stream, const char* format, ...);

FILE* __acrt_iob_func(unsigned);

#define stdin  (__acrt_iob_func(0))
#define stdout (__acrt_iob_func(1))
#define stderr (__acrt_iob_func(2))

#ifdef __cplusplus
    }
#endif

#endif
