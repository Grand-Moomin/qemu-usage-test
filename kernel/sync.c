#include "kernel/sync.h"
#include "kernel/utils.h"
#include "kernel/sched.h"

void spin_lock_init(spinlock_t *slp)
{
    spin_unlock(slp);
}

void spin_lock_alt(spinlock_t *slp)
{
    asm ("1: lock; decb %0\n\t"
        "jns 3f\n\t"
        "2: cmpb $0, %0\n\t"
        "jle 2b\n\t"
        "jmp 1b\n\t"
        "3:"
        ::"m" (slp->lock));
}

void spin_lock(spinlock_t *slp)
{
    asm ("1: movb $0, %%dl\n\t"
        "xchgb %%dl, %0\n\t"
        "cmpb $0, %%dl\n\t"
        "je 1b\n\t"
        :: "m" (slp->lock) : "edx");
}

void spin_unlock(spinlock_t *slp)
{
    asm("movb $1, %0":"=m" (slp->lock));
}

void sema_init(semaphore_t *sema, int32_t count)
{
    sema->count = count;
    spin_lock_init(&sema->slock);
    list_head_init(&sema->wait);
}

void sema_down(semaphore_t *sema)
{
    struct task_struct *curr = current;
    spin_lock(&sema->slock);
    if(sema->count <= 0){
        task_suspend(curr, &sema->wait);
        spin_unlock(&sema->slock);
        schedule();
    }
    else{
        sema->count--;
        spin_unlock(&sema->slock);
    }
}

void sema_up(semaphore_t *sema)
{
    struct task_struct *tsk_wait;
    
    spin_lock(&sema->slock);
    if(++sema->count > 0)
        if(!list_empty(&sema->wait)){
            tsk_wait = list_entry(list_pop(&sema->wait), struct task_struct, queued_tasks);
            task_resume(tsk_wait);
        }
    spin_unlock(&sema->slock);
}

