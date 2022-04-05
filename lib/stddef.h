#ifndef __STDDEF_H__
#define __STDDEF_H__

#include <stdbool.h>

#define NULL ((void *)0)

#define offset_of(type, member) (size_t)(&(((type *)0)->member))
#define container_of(ptr, type, member) ({ \
        (type *)((char *)ptr - offset_of(type, member)); })

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#endif
