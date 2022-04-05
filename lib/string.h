#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h>
#include <stdarg.h>

extern void *memcpy(void *dest, const void *src, size_t size);
extern void *memmove(void *dest, const void *src, size_t size);
extern int memcmp(const void *dest, const void *src, size_t size);
extern void *memset(void *dest, int val, size_t size);
extern int strcmp(const char *str1, const char *str2);
extern char *strncat(char *dest, const char *src, size_t size);
extern size_t strlen(const char *str);
extern size_t strlcpy(char *dest, const char *src, size_t size);

#endif