#ifndef __VADDR_H__
#define __VADDR_H__

#include <stdint.h>
#include <stdbool.h>
#include <kernel/loader.h>

#define PAGE_BITS 12
#define PAGE_SIZE (1 << PAGE_BITS)
#define KERNEL_VADDR_BASE   ((void *)LOADER_PHYS_BASE)

static inline bool is_user_vaddr(const void *vaddr)
{
    return vaddr < KERNEL_VADDR_BASE;
}

static inline bool is_kernel_vaddr(const void *vaddr)
{
    return vaddr >= KERNEL_VADDR_BASE;
}

static inline void *ptov(uintptr_t paddr)
{
    return (void *)(paddr + (char *)KERNEL_VADDR_BASE);
}

static inline uintptr_t vtop(const void *vaddr)
{
    return (uintptr_t)vaddr - (uintptr_t)KERNEL_VADDR_BASE;
}

static inline uintptr_t pg_no(const void *vaddr)
{
    return (uintptr_t)vaddr >> PAGE_BITS;
}

#endif