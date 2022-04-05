#include <string.h>

void *memcpy(void *dest, const void *src, size_t size)
{
    char *_dest = dest;
    const char *_src = src;

    if(!_dest || !_src || !size)
        return NULL;

    while(size-- > 0)
        *_dest++ = *_src++;

    return dest;
}

void *memmove(void *dest, const void *src, size_t size)
{
    return memcpy(dest, src, size);
}

int memcmp(const void *dest, const void *src, size_t size)
{
    const char *_dest = dest;
    const char *_src = src;

    if(!_dest || !_src || !size)
        return -2;

    while(size-- > 0){
        if(*_dest != *_src)
            return *_dest > *_src ? 1 : -1;
    }
    return 0;
}

void *memset(void *dest, int val, size_t size)
{
    char *_dest = dest;
    if(!_dest)
        return NULL;

    while(size-- > 0)
        *_dest++ = val;

    return dest;
}

size_t strlen(const char *str)
{
    const char *p = str;
    if(!p)
        return 0;

    for(p = str; *p != '\0'; p++);

    return p - str;
}

size_t strlcpy(char *dest, const char *src, size_t size)
{
    size_t src_len = strlen(src);
    size_t dest_len = size - 1;

    if(size > 0){
        if(src_len < dest_len)
            dest_len = src_len;
        memcpy(dest, src, dest_len);
        dest[dest_len] = '\0';
    }

    return dest_len;
}

int strcmp(const char *str1, const char *str2)
{
    while(*str1 != '\0' && *str1++ == *str2++);
    return *str1 < *str2 ? -1 : *str1 > *str2;
}