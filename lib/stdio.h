#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>

extern int vsprintf(char *buf, const char *fmt, va_list args);

extern int sprintf(char *buf, const char *fmt, ...);

#endif