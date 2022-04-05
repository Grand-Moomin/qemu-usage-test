/* 8254 Programmable timer driver */

#ifndef __HW_TIMER_H__
#define __HW_TIMER_H__

#include <stdint.h>

#define TIMER_CTRL 0x43
#define TIMER_DATA(channel) (0x40 + (channel))

#define TIMER_INTR_NO   0x20

/* Ticks per second */
#define TICK_HZ 1193180

void hw_timer_config(uint32_t channel, uint32_t mode, uint32_t frequency);

void hw_timer_init(void);

#endif