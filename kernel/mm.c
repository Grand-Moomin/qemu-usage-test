#include "kernel/mm.h"
#include "kernel/utils.h"
#include "kernel/vaddr.h"
#include <kernel/bitmap.h>
#include <string.h>
#include <round.h>

static mem_pool kernel_pool;

static bool mem_pool_init(mem_pool *mem_pool, void *start_addr, size_t page_cnt)
{
    size_t bm_pages = DIV_ROUND_UP(bitmap_required_buf_size(page_cnt) + sizeof(bitmap), PAGE_SIZE);
    page_cnt -= bm_pages;

    spin_lock_init(&mem_pool->slock);
    mem_pool->bm = bitmap_create_in_buf(page_cnt, start_addr, bm_pages * PAGE_SIZE);
    if(!mem_pool->bm)
        return false;
    bitmap_config_all(mem_pool->bm, false);
    mem_pool->base = start_addr + bm_pages * PAGE_SIZE;

    return true;
}

void mm_init(size_t page_num)
{
    /* Exclude Memory from 0 to 0x100000. */
    page_num -= pg_no((void *)KERNEL_MEM_POOL_PHY_START);
    printk("Initial free memory pages %d.\n", page_num);
    if(!mem_pool_init(&kernel_pool, ptov(KERNEL_MEM_POOL_PHY_START), page_num)){
        printk("Memory pool creation failed. Kernel startup aborted. \n");
        while(1);
    }
    printk("Current free kernel pages %d, kernel memory pool base %08p\n", bitmap_get_bits_count(kernel_pool.bm), kernel_pool.base);
}

void *mm_kget_page(page_frame_flag pf_flag)
{
    return mm_kget_pages(pf_flag, 1);
}

void *mm_kget_pages(page_frame_flag pf_flag, size_t page_count)
{
    void *pages = NULL;
    size_t page_idx;

    if(!page_count)
        return NULL;

    spin_lock(&kernel_pool.slock);
    page_idx = bitmap_find_first_bits_group(kernel_pool.bm, 0, page_count, false);
    if(page_idx == BITMAP_ERR){
        if(pf_flag & PF_ATOMIC)
            printk("Kernel pool error: out of memory.\n");
        spin_unlock(&kernel_pool.slock);
        return NULL;
    }
    bitmap_set_multi_bits(kernel_pool.bm, page_idx, page_count);
    spin_unlock(&kernel_pool.slock);

    pages = kernel_pool.base + page_idx * PAGE_SIZE;

    if(pf_flag & PF_ZERO)
        memset(pages, 0, page_count * PAGE_SIZE);

    return pages;
}

void mm_kfree_page(void *pages)
{
    mm_kfree_pages(pages, 1);
}

void mm_kfree_pages(void *pages, size_t page_count)
{
    void *kernel_pool_end = kernel_pool.base + bitmap_get_bits_count(kernel_pool.bm) * PAGE_SIZE;
    size_t page_no_start;

    pages = (void *)ROUND_DOWN((uint32_t)pages, PAGE_SIZE);
    if(pages < (void *)kernel_pool.base || pages >= kernel_pool_end)
        return;

    page_no_start = pg_no(pages) - pg_no(kernel_pool.base);
    if(page_no_start + page_count > bitmap_get_bits_count(kernel_pool.bm))
        return;
    
    spin_lock(&kernel_pool.slock);
    /*  Caution: Free without any verification. 
        The following function could fail, if indice exceed bitmap boundary. */
    bitmap_reset_multi_bits(kernel_pool.bm, page_no_start, page_count);
    spin_unlock(&kernel_pool.slock);
}

