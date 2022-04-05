#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <stdint.h>
#include <round.h>
#include <stdbool.h>
#include "kernel/vaddr.h"

#define TASK_MAGIC 0xae5f83d2

#define current ({uint32_t esp; \
                  asm ("mov %%esp, %0":"=m" (esp)); \
                  (struct task_struct *)ROUND_DOWN(esp, PAGE_SIZE);})

typedef int32_t pid_t;

enum {
    PRIO_HIGH = 0,
    PRIO_DEFAULT = 31,
    PRIO_LOW = 63,
    PRIO_NUM
};

enum task_status {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_DYING
};

struct task_struct {
    pid_t pid;
    enum task_status status;
    char name[16];      /* Abbreviated task name */
    void *sp;           /* Saved stack pointer */
    int32_t priority;
    int32_t time_slice_init;        /* Initial time slice */
    int32_t time_slice_res;         /* Residual time slice */
    struct list_head all_tasks;
    struct list_head queued_tasks;  /* A task can be queued either in a ready list or a waiting list */
    uint32_t *pg_dir;   /* Virt addr of current task page directory*/
    uint32_t magic;
};

typedef void task_proc(void *params);

static inline bool valid_priority(int priority)
{
    return (priority >= PRIO_HIGH) && (priority <= PRIO_LOW);
}

static inline bool valid_task(struct task_struct *tsk)
{
    return tsk->magic == TASK_MAGIC;
}

extern void sched_init(void);
extern pid_t task_create(task_proc proc, void *params, const char *name, int priority, enum task_status status);
extern void task_exit(void);
extern void task_suspend(struct task_struct *tsk, struct list_head *wait); 
extern void task_resume(struct task_struct *tsk);
extern void schedule(void);
extern void task_sched_post_proc(struct task_struct *prev);
extern void task_tick(void);

#endif