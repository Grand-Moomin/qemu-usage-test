#ifndef __PTE_H__
#define __PTE_H__

#include "kernel/vaddr.h"

#define PTE_MASK_PHY_ADDR 0xfffff000
#define PTE_MASK_FLAGS  ~PTE_MASK_PHY_ADDR

#define PAGE_TABLE_BITS   10
#define PAGE_DIR_BIT_SHIFT (PAGE_BITS + PAGE_TABLE_BITS)
#define PAGE_TABLE_IDX_MASK 0x3ff000

typedef enum {
    PTE_P = 0x1,    /* Present 1, not present 0. */
    PTE_W = 0x2,    /* Writable 1, read only 0. */
    PTE_U = 0x4,    /* Mode: user 1, supervisor 0.*/
    PTE_A = 0x20,   /* Accessed 1, not accessed 0.*/
    PTE_D = 0x40,   /* Dirty 1, not dirty 0. */
} page_table_flags;

inline uint32_t virt_to_pd_idx(void *vaddr)
{
    return (uint32_t)vaddr >> PAGE_DIR_BIT_SHIFT;
}

inline uint32_t virt_to_pt_idx(void *vaddr)
{
    return ((uint32_t)vaddr & PAGE_TABLE_IDX_MASK) >> PAGE_BITS;
}

inline uint32_t mk_pde(uintptr_t paddr, page_table_flags flags)
{
    return (paddr & PTE_MASK_PHY_ADDR) | (flags & PTE_MASK_FLAGS);
}

inline uint32_t mk_pte(uintptr_t paddr, page_table_flags flags)
{
    return mk_pde(paddr, flags);
}

#endif