#ifndef __PIC_H__
#define __PIC_H__

/* 8259A Programmable Interrupt Controller registers */
#define PIC_MASTER_CTRL 0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_CTRL  0xa0
#define PIC_SLAVE_DATA  0xa1

extern void pic_init(void);
extern void pic_end_of_interrupt(int vec_no);

#endif