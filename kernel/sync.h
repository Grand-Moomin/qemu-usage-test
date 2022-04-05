#ifndef __SYNC_H__
#define __SYNC_H__

#include <stdint.h>
#include <stdbool.h>
#include <list.h>

#define barrier() asm volatile ("":::"memory")

typedef struct spinlock {
    int8_t lock;
} spinlock_t;

extern void spin_lock_init(spinlock_t *slp);
extern void spin_lock(spinlock_t *slp);
extern void spin_lock_alt(spinlock_t *slp);
extern void spin_unlock(spinlock_t *slp);

typedef struct semaphore {
    spinlock_t slock;
    int32_t count;
    struct list_head wait;  /* List of tasks waiting for this semaphore */
} semaphore_t;

extern void sema_init(semaphore_t *sema, int32_t count);
extern void sema_down(semaphore_t *sema);
extern void sema_up(semaphore_t *sema);

#endif