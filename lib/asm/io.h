#ifndef __IO_H__
#define __IO_H__

#include <stddef.h>
#include <stdint.h>

static inline uint8_t inb(uint16_t port)
{
    uint8_t val;
    __asm__ volatile ("inb %w1, %b0" : "=a" (val) : "Nd" (port));
    return val;
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t val;
    __asm__ volatile ("inw %w1, %w0" : "=a" (val) : "Nd" (port));
    return val;
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t val;
    __asm__ volatile ("inl %w1, %0" : "=a" (val) : "Nd" (port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %b0, %w1" : : "a" (val), "Nd" (port));
}

static inline void outw(uint16_t port, uint16_t val)
{
    __asm__ volatile ("outw %w0, %w1" : : "a" (val), "Nd" (port));
}

static inline void outl(uint16_t port, uint32_t val)
{
    __asm__ volatile ("outl %0, %w1" : : "a" (val), "Nd" (port));
}

static inline void insw(uint16_t port, void *addr, size_t cnt)
{
    __asm__ volatile ("rep insw" : "+D" (addr), "+c" (cnt) : "d" (port) : "memory");
}

static inline void outsw(uint16_t port, const void *addr, size_t cnt)
{
    __asm__ volatile ("rep outsw" : "+S" (addr), "+c" (cnt) : "d" (port));
}

#endif