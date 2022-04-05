#ifndef __SWITCH_H__
#define __SWITCH_H__

#include <stdint.h>
#include "kernel/sched.h"

/* Task switch stack frame */
/* Preserve edi, esi, ebp and ebx. */
struct switch_frame{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t ebx;
    void *eip;
    struct task_struct *cur;    
    struct task_struct *next;
};

extern struct task_struct *switch_to(struct task_struct *cur, struct task_struct *next);
extern void switch_initial(void);

#endif