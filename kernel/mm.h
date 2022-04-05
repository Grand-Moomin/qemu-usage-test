#ifndef __MM_H__
#define __MM_H__

#include <stddef.h>
#include "kernel/sync.h"
#include <kernel/bitmap.h>
#include "kernel/vaddr.h"

#define KERNEL_MEM_POOL_PHY_START   0x100000    /* Start phys addr of dynamically allocated page frames */

typedef enum {
    PF_ATOMIC = 0x01,
    PF_ZERO = 0x02,
} page_frame_flag;

typedef struct {
    spinlock_t slock;
    bitmap *bm;
    uint8_t *base;
} mem_pool;

/* Page frame related methods */

extern void mm_init(size_t page_num);

extern void *mm_kget_page(page_frame_flag pf_flag);                     /* Get single page */
extern void mm_kfree_page(void *pages);                                 /* Free single page */
extern void *mm_kget_pages(page_frame_flag pf_flag, size_t page_count); /* Get multiple pages */
extern void mm_kfree_pages(void *pages, size_t page_count);             /* Free multiple pages */

#endif