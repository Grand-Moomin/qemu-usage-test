#include "devices/hw_timer.h"
#include <asm/io.h>
#include <asm/system.h>
#include "kernel/interrupt.h"
#include "kernel/sched.h"
#include "kernel/utils.h"

/* We used channel 0 for system tick. */

void hw_timer_config(uint32_t channel, uint32_t mode, uint32_t frequency)
{
    uint16_t counter;

    if(channel != 0 && channel != 2)
        return;
    if(mode != 2 && mode != 3)
        return;

    if(frequency < TICK_HZ / (1 << (8 * sizeof(uint16_t))))
        counter = 0; /* The hw timer treats 0 as 65536. */
    
    else if(frequency > TICK_HZ)
        counter = 2;

    else
        counter = TICK_HZ / frequency;

    outb(TIMER_CTRL, (channel << 6) | 0x30 | (mode << 1));
    outb(TIMER_DATA(channel), counter);
    outb(TIMER_DATA(channel), counter >> 8);
}

static void systick_intr(interrupt_stack_frame *stack UNUSED)
{
    static int ticks = 0;
    task_tick();
    ticks++;
    //printk("sys tick %d\n", ticks);
}

void hw_timer_init(void)
{
    hw_timer_config(0, 2, 100);
    set_interrupt_gate(TIMER_INTR_NO, systick_intr);
}