#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#define sti()   asm ("sti"::)
#define cli()   asm ("cli"::)
#define nop()   asm ("nop"::)

#define set_cr3(phys_addr)    asm ("mov %0, %%cr3"::"r" (phys_addr))

#endif
