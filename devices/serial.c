#include "devices/serial.h"
#include <asm/io.h>
#include "kernel/sync.h"
#include "kernel/interrupt.h"

#define SERIAL_BASE 0x3f8   /* Serial port 1 base addr. */

/* DLAB = 0 */
#define RBR_REG (SERIAL_BASE)   /* Receiver buffer register (Read only). */
#define THR_REG (SERIAL_BASE)   /* Transmitter holding register. (Write only). */
#define IER_REG (SERIAL_BASE + 1)   /* Interrupt enable register. */

/* DLAB = 1 */
#define LS_REG (SERIAL_BASE)    /* Divisor latch (LSB). */
#define MS_REG (SERIAL_BASE + 1)    /* Divisor latch (MSB). */

#define IIR_REG (SERIAL_BASE + 2)   /* Interrupt identification register */
#define FCR_REG (SERIAL_BASE + 3)   /* FIFO control register. */
#define LCR_REG (SERIAL_BASE + 4)   /* Line control register. */
#define MCR_REG (SERIAL_BASE + 5)   /* Modem control register. */
#define LSR_REG (SERIAL_BASE + 6)   /* Line status register. */

/* Interrupt enable register bits. */
#define IER_RX 0x01 /* Received data available interrupt */
#define IER_TX 0x02 /* Transmitter data available interrupt. */
#define IER_LS 0x40 /* Receiver line status interrupt. */
#define IER_MS 0x80 /* Modem status interrupt. */

/* Line control register bits. */
#define LCR_WLS0 0x01   /* Word length select bit 0. */
#define LCR_WLS1 0x02   /* Word length select bit 1. */
#define LCR_STB 0x04    /* Number of stop bits. */
#define LCR_PEN 0x08    /* Parity enable. */
#define LCR_DLAB 0x80   /* Divisor latch access bit (DLAB). */

/* Modem controle register bits. */
#define MCR_OUT2 0x08   /* Output line 2. */

/* Line status register bits. */
#define LSR_DR 0x01     /* Data ready: received data in RBR. */
#define LSR_OE 0x02     /* Overrun error. */
#define LSR_PE 0x04     /* Parity error. */
#define LSR_FE 0x08     /* Framing error. */
#define LSR_BI 0x10     /* Break interrupt. */
#define LSR_THRE 0x20   /* THR empty. */
#define LSR_TEMT 0x40   /* Transmitter empty. */

static semaphore_t sema_write;

static void serial_init_poll(void);
static void serial_set_rate(uint32_t bps);
static void serial_putc_poll(char ch);
static void serial_enable_intr(void);
static void serial_interrupt(interrupt_stack_frame *stack);

static void serial_init_poll()
{
    outb(IER_REG, 0);   /* Disable all interrupts. */
    outb(FCR_REG, 0);   /* Disable FIFO. */

    /* Initialization: Settings are hard coded. */
    outb(LCR_REG, LCR_WLS0 | LCR_WLS1); /* 8 bits, 1 stop bit, no parity bit. */
    serial_set_rate(9600);  /* Baud rate 9600. */
}

static void serial_set_rate(uint32_t bps)
{
    uint32_t base = 115200; /* Base rate of 16550A, in Hz. */
    uint32_t divisor = base / bps;

    /* Set DLAB = 1. */
    uint8_t lcr = inb(LCR_REG);
    lcr |= LCR_DLAB;
    outb(LCR_REG, lcr);
    
    /* Set divisor register. */
    outb(LS_REG, divisor & 0xff);
    outb(MS_REG, divisor & 0xff);

    /* Set DLAB = 0. */
    lcr &= ~LCR_DLAB;
    outb(LCR_REG, lcr);
}

static void serial_putc_poll(char ch)
{
    while(!(inb(LSR_REG) & LSR_THRE))
        continue;
    outb(THR_REG, ch);
}

static void serial_interrupt(interrupt_stack_frame *stack __attribute__((unused)))
{
    char rx_ch;
    inb(IIR_REG);

    //while(inb(LSR_REG) & LSR_DR) //TODO: Error when recieving data.
    rx_ch = inb(RBR_REG);
    serial_putc(rx_ch);
    if(rx_ch == '\r')
        serial_putc('\n');

    outb(IER_REG, IER_RX);
}

static void serial_enable_intr()
{
    interrupt_disable();
    /* Enable recv interrupt only. */
    outb(IER_REG, IER_RX);
    interrupt_enable();
}

void serial_init()
{
    serial_init_poll();
    //outb(MCR_REG, MCR_OUT2);
    sema_init(&sema_write, 1);
    set_interrupt_gate(0x20 + 4, serial_interrupt);
    serial_enable_intr();
}

void serial_putc(char ch)
{
    sema_down(&sema_write);
    serial_putc_poll(ch);
    sema_up(&sema_write);
}