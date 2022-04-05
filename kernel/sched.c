#include "kernel/sched.h"
#include "kernel/sync.h"
#include "kernel/vaddr.h"
#include "kernel/switch.h"
#include "kernel/mm.h"
#include "kernel/interrupt.h"
#include "kernel/utils.h"
#include <system.h>
#include <string.h>
#include <stddef.h>

struct kernel_proc_stack{
    task_proc *proc;            /* kernel task procedure */
    void *kernel_task_ret_eip;  /* return eip from kernel task procedure */
    void *params;               /* params of kernel task procedure */
};

/* Tasks that are ready to run. */
static struct list_head ready_task_list[PRIO_NUM];
/* List of all tasks */
static struct list_head all_tasks_list;
/* Pid allocation lock */
static spinlock_t pid_slock;
/* List operation lock */
static spinlock_t list_slock;

/* The very first task we create manually. */
static struct task_struct *init_task;

uint32_t saved_sp_offset = offset_of(struct task_struct, sp);

static void task_struct_init(struct task_struct *tsk, const char *name, int priority);
static pid_t allocate_pid(void);
static void idle_proc(void *params);

void idle_proc(void* params){
    printk("--idle task--\n  esp : %p\t", current);
    printk("param : %p\n", params);
    if(params){
        sema_up((semaphore_t *)params);
    }
    while(1);
}

void sched_init(void)
{
    uint32_t i;
    semaphore_t sem;

    sema_init(&sem, 0);
    spin_lock_init(&pid_slock);
    spin_lock_init(&list_slock);
    for(i = 0; i < PRIO_NUM; ++i)
        list_head_init(&ready_task_list[i]);
    list_head_init(&all_tasks_list);

    init_task = current;
    task_struct_init(init_task, "init", PRIO_DEFAULT);
    init_task->status = TASK_RUNNING;
    list_add(&init_task->queued_tasks, &ready_task_list[init_task->priority]);

    task_create(idle_proc, &sem, "idle", PRIO_DEFAULT, TASK_READY);
    task_create(idle_proc, NULL, "idle", PRIO_DEFAULT, TASK_READY);
    /* Wait for the idle task. */
    sema_down(&sem);
}

static void task_struct_init(struct task_struct *tsk, const char *name, int priority)
{
    if(!tsk || !name || !valid_priority(priority))
        return;
    
    memset(tsk, 0, sizeof(*tsk));
    tsk->status = TASK_BLOCKED;
    
    tsk->sp = (uint8_t *)tsk + PAGE_SIZE;
    tsk->priority = priority;
    tsk->magic = TASK_MAGIC;
    strlcpy(tsk->name, name, sizeof(tsk->name));
    spin_lock(&list_slock);
    list_add(&tsk->all_tasks, &all_tasks_list);
    spin_unlock(&list_slock);
    tsk->pid = allocate_pid();
    tsk->time_slice_init = tsk->time_slice_res = DIV_ROUND_UP(priority, PRIO_DEFAULT);
}

void task_exit(void)
{
    struct task_struct *curr = current;
    
    spin_lock(&list_slock);
    curr->status = TASK_DYING;
    list_del(&curr->queued_tasks);
    spin_unlock(&list_slock);
    schedule();
}

pid_t task_create(task_proc *proc, void *params, const char *name, int priority, enum task_status status)
{
    struct kernel_proc_stack *proc_stack;
    struct switch_frame *switch_stack;
    struct task_struct *tsk;

    if(!proc)
        return -1;

    tsk = mm_kget_page(PF_ZERO);
    if(!tsk)
        return -1;

    task_struct_init(tsk, name, priority);

    /* Manually create a task saved stack */
    tsk->sp -= sizeof(struct kernel_proc_stack);
    proc_stack = tsk->sp;
    proc_stack->params = params;
    proc_stack->kernel_task_ret_eip = task_exit;
    proc_stack->proc = proc;

    tsk->sp -= sizeof(struct switch_frame);
    switch_stack = tsk->sp;
    switch_stack->eip = switch_initial;
    
    spin_lock(&list_slock);
    list_add_tail(&tsk->queued_tasks, &ready_task_list[tsk->priority]);
    spin_unlock(&list_slock);
    tsk->status = status;

    return tsk->pid;
}

void task_suspend(struct task_struct *tsk, struct list_head *wait)
{
    spin_lock(&list_slock);
    tsk->status = TASK_BLOCKED;
    list_del(&tsk->queued_tasks);
    list_add(&tsk->queued_tasks, wait);
    spin_unlock(&list_slock);
}

void task_resume(struct task_struct *tsk)
{
    if(!valid_task(tsk))
        return;
    
    spin_lock(&list_slock);
    if(tsk->status == TASK_BLOCKED){
        tsk->status = TASK_RUNNING;
        list_add_tail(&tsk->queued_tasks, &ready_task_list[tsk->priority]);
    }
    spin_unlock(&list_slock);
}

void task_sched_post_proc(struct task_struct *prev)
{
    /*  Here, we've already switched to the new running task, 
        so it's safe to delete the previous task if marked as DYING.    */
    if(!prev && prev->status == TASK_DYING && prev != init_task){
        spin_lock(&list_slock);
        list_del(&prev->all_tasks);
        spin_unlock(&list_slock);
        mm_kfree_page(prev);
    }
}

static pid_t allocate_pid(void){
    static pid_t next_pid = -1;

    spin_lock(&pid_slock);
    next_pid++;
    spin_unlock(&pid_slock);
    
    return next_pid;
}

static struct task_struct *next_task_to_run(void)
{
    //TODO: change algo, currently round robin for priority level PRIO_DEFAULT.
    struct task_struct *next = list_entry(ready_task_list[PRIO_DEFAULT].next, struct task_struct, queued_tasks);
    struct task_struct *curr = current;

    if(curr->status == TASK_RUNNING){
        spin_lock(&list_slock);
        list_del(&curr->queued_tasks);
        list_add_tail(&curr->queued_tasks, &ready_task_list[PRIO_DEFAULT]);
        spin_unlock(&list_slock);
    }
    
    //printk("next tsk to run %p\n", next);
    
    return next;
}

void task_tick(void)
{
    struct task_struct *curr = current;

    if(--curr->time_slice_res <= 0){
        set_flag_schedule_on_return();
        curr->time_slice_res = curr->time_slice_init;
    }
}

void schedule(void)
{
    struct task_struct *curr = current;
    struct task_struct *next = next_task_to_run();
    struct task_struct *prev = NULL;

    cli();
    next->status = TASK_RUNNING;

    if(curr != next){
        prev = switch_to(curr, next);
        task_sched_post_proc(prev);
    }
    sti();
}