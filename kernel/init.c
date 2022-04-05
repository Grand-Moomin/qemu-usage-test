#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <system.h>

#include "kernel/mm.h"
#include "kernel/loader.h"
#include "kernel/init.h"
#include "kernel/utils.h"
#include "kernel/interrupt.h"
#include "kernel/sync.h"
#include "kernel/pte.h"
#include "kernel/sched.h"
#include "devices/fb.h"
#include "devices/hw_timer.h"
#include "devices/block.h"
#include "devices/ide.h"
#include "devices/serial.h"
#include "devices/kbd.h"

static void bss_init (void);
static void kernel_pg_dir_init(void);

spinlock_t lock;
static void divide_error(interrupt_stack_frame *stack __attribute__((unused)))
{
    spin_lock(&lock);
    printk("devide error %x\n", stack->error_code);
    spin_unlock(&lock);
}

int main(void)
{
    bss_init();
    mm_init(init_ram_pages);
    kernel_pg_dir_init();

    init_interrupt();
    hw_timer_init();
    serial_init();
    kbd_init();

    sched_init();

    block_init();
    ide_init();

    spin_lock_init(&lock);
    printk("isr addr %p\n", divide_error);
    set_trap_gate(0x0e, divide_error);

    /*
    uint32_t *ill_addr = (uint32_t *)0xC7FDFFFC;
    *ill_addr = 0;*/
    //sti();

    //printk("curr %p\n",current);
    //printk("priority %d\n",((uint32_t *)current)[7]);

    char buf[512];
    struct block *blk = blcok_get_by_name("hda");
    printk("blk %d\n",blk->nr_sectors);
    block_read(blk, buf, 0);
    printk("read sector 0,  %02x\n", (uint8_t)buf[510]);

    task_exit();
    return 0;
}

static void bss_init (void) 
{
    extern char _start_bss, _end_bss;
    memset (&_start_bss, 0, &_end_bss - &_start_bss);
}

static void kernel_pg_dir_init(void)
{
    uint32_t i, pt_idx, pd_idx;
    uint32_t flags;
    uint32_t *pd, *pt;
    uintptr_t paddr;
    void *vaddr;
    extern char _start, _end_kernel_text;

    pd = mm_kget_page(PF_ZERO | PF_ATOMIC);
    pt = NULL;

    for(i = 0; i < init_ram_pages; ++i){
        flags = PTE_P | PTE_W;
        paddr = i << PAGE_BITS;
        vaddr = ptov(paddr);
        pd_idx = virt_to_pd_idx(vaddr);

        if(!pd[pd_idx]){
            pt = mm_kget_page(PF_ZERO | PF_ATOMIC);
            pd[pd_idx] = mk_pde(vtop(pt), flags);
        }

        pt_idx = virt_to_pt_idx(vaddr);
        if((char *)vaddr >= &_start && (char *)vaddr < &_end_kernel_text)
            flags &= ~(PTE_W);
        pt[pt_idx] = mk_pte(vtop(vaddr), flags);
    }

    set_cr3(vtop(pd));
}