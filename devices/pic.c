#include "devices/pic.h"
#include <asm/io.h>

/* We map external interrupts 0 ~ 15 issued by pic to idt vectors 0x20 ~ 0x2f. */
void pic_init(void)
{
    /* Mask all interrupts. */
    outb(PIC_MASTER_DATA, 0xff);
    outb(PIC_SLAVE_DATA, 0xff);

    /* Master pic initialization. */
    outb(PIC_MASTER_CTRL, 0x11);/* ICW1: Edge triggered, cascade mode, ICW4 needed*/
    outb(PIC_MASTER_DATA, 0x20);/* ICW2: IRQ 0 ~ 7 -> Intr vector 0x20 ~ 0x27 */
    outb(PIC_MASTER_DATA, 0x04);/* ICW3: Slave pic wired to IRQ 2.*/
    outb(PIC_MASTER_DATA, 0x01);/* ICW4: Non buffered, normal eoi, 8086 mode*/

    /* Slave pic initialization*/
    outb(PIC_SLAVE_CTRL, 0x11);/* ICW1: Edge triggered, cascade mode, ICW4 needed*/
    outb(PIC_SLAVE_DATA, 0x28);/* ICW2: IRQ 8 ~ 15 -> Intr vector 0x28 ~ 0x2f */
    outb(PIC_SLAVE_DATA, 0x02);/* ICW3: Slave ID equal to the corresponding master IRQ no.*/
    outb(PIC_SLAVE_DATA, 0x01);/* ICW4: Non buffered, normal eoi, 8086 mode*/

    /* Unmask all interrupts. */
    outb(PIC_MASTER_DATA, 0);
    outb(PIC_SLAVE_DATA, 0);
}

void pic_end_of_interrupt(int vec_no)
{
    // Acknowledge master pic
    outb(PIC_MASTER_CTRL, 0x20);    /* Non-specific eoi command. */

    // Acknowledge slave pic
    if(vec_no >= 0x28 && vec_no <= 0x2f)
        outb(PIC_SLAVE_CTRL, 0x20); /* Non-specific eoi command. */
}