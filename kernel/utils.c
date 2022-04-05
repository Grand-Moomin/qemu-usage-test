#include <stdarg.h>
#include <stdio.h>
#include "kernel/utils.h"
#include "devices/fb.h"
#include "devices/serial.h"

static char printk_buf[1024];

int printk(const char *fmt, ...)
{
    va_list args;
    int i, num;

    va_start(args, fmt);
    num = vsprintf(printk_buf, fmt, args);
    va_end(args);

    for(i = 0; i < num; ++i){
        fb_putc(printk_buf[i]);
    }
    
    return num;
}