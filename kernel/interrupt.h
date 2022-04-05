#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <stdint.h>
#include <stdbool.h>
#include <system.h>

#define GATE_TYPE_INTR  0xe
#define GATE_TYPE_TRAP  0xf

#define HARDWARE_INTR_START 0x20
#define INTR_COUNT   256

typedef struct __attribute__((__packed__)){
    uint16_t gs;
    uint16_t fs;
    uint16_t es;
    uint16_t ds;
    uint32_t ebp;
    uint32_t edi;
    uint32_t esi;
    uint32_t edx;
    uint32_t ecx;
    uint32_t ebx;
    uint32_t eax;
    //Vector no.
    uint32_t vec_no;
    //Hardware error code
    uint32_t error_code;
    //Hardware pushed registers
    void (*eip)(void);
    uint16_t cs;
    uint32_t eflag;
    void *esp;
    uint16_t ss;
} interrupt_stack_frame;

typedef void interrupt_handler(interrupt_stack_frame *);

typedef struct __attribute__((__packed__)){
    union{
        uint32_t lower_u32;
        struct{
            uint16_t offset0_15;
            uint16_t seg_sel;
        };
    };
    union{
        uint32_t upper_u32;
        struct{
            uint16_t gate_config;
            uint16_t offset16_31;
        };
    };
} idt_entry;

extern void generic_isr(interrupt_stack_frame *stack);

extern void init_interrupt(void);
extern void set_interrupt_gate(uint32_t n, interrupt_handler isr);
extern void set_trap_gate(uint32_t n, interrupt_handler isr);
extern void set_system_gate(uint32_t n, interrupt_handler isr);
extern bool in_interrupt(void);

extern void set_flag_schedule_on_return(void);

static inline void interrupt_enable(void)
{
    sti();
}

static inline void interrupt_disable(void)
{
    cli();
}

#endif