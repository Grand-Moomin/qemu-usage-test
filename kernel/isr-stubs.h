#ifndef __ISR_STUBS_H__
#define __ISR_STUBS_H__

#include <stdint.h>
#include "kernel/interrupt.h"

extern uint32_t *isr_stubs[INTR_COUNT];

#endif