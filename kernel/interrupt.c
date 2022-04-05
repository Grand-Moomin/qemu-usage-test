#include "kernel/interrupt.h"
#include "kernel/isr-stubs.h"
#include "kernel/loader.h"
#include "kernel/utils.h"
#include "kernel/sched.h"
#include "devices/pic.h"
#include <stddef.h>

extern uint32_t idt_start;

static idt_entry *idt_table;

static bool volatile in_interrupt_flag;
static bool sched_on_return;

static interrupt_handler *isr_table[INTR_COUNT];

bool in_interrupt(void)
{
    return in_interrupt_flag;
}

void set_flag_schedule_on_return(void)
{
    sched_on_return = true;
}

void generic_isr(interrupt_stack_frame *stack)
{
    interrupt_handler *isr;
    bool external_intr = stack->vec_no >= HARDWARE_INTR_START && stack->vec_no < INTR_COUNT;

    // Set interrupt flag for hardware interrupts
    if(external_intr){
        //printk("hw intr %d\n", stack->vec_no);
        in_interrupt_flag = true;
        sched_on_return = false;
    }

    isr = isr_table[stack->vec_no];
    if(isr)
        isr(stack);

    if(external_intr){
        pic_end_of_interrupt(stack->vec_no);
        in_interrupt_flag = false;
        
        if(sched_on_return){
            schedule();
        }
    }
}

static void default_isr(interrupt_stack_frame *stack)
{
    printk("Interrupt no. %d, error code %x\n", stack->vec_no, stack->error_code);
    /*int cr2; //Page fault test.
    asm ("mov %%cr2, %0":"=r"(cr2));
    printk("cr2 %x", cr2);
    while(1);*/
}

/* Initialize interrupt descriptor table. */
void init_interrupt(void)
{
    int i;

    idt_table = (idt_entry *)&idt_start;

    for(i = 0; i < INTR_COUNT; ++i)
        set_interrupt_gate(i, default_isr);

    pic_init();
}

static idt_entry* set_gate(idt_entry *entry, interrupt_handler isr, int dpl, int type)
{
    if(!entry)
        return NULL;

    uint32_t n = entry - idt_table;
    
    isr_table[n] = isr;
    entry->offset0_15 = (uint32_t)isr_stubs[n];
    entry->offset16_31 = ((uint32_t)isr_stubs[n] >> 16);
    entry->seg_sel = SEL_KCSEG;
    entry->gate_config = 0x8000 | (dpl << 13) | (type << 8);

    return entry;
}

void set_interrupt_gate(uint32_t n, interrupt_handler isr)
{
    set_gate(&idt_table[n], isr, 0, GATE_TYPE_INTR);
}

void set_trap_gate(uint32_t n, interrupt_handler isr)
{
    set_gate(&idt_table[n], isr, 0, GATE_TYPE_TRAP);
}

void set_system_gate(uint32_t n, interrupt_handler isr)
{
    set_gate(&idt_table[n], isr, 3, GATE_TYPE_TRAP);
}
