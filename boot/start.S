#include "kernel/loader.h"

#### Kernel startup code.

#### The loader (in loader.S) loads the kernel at physical address
#### 0x20000 (128 kB) and jumps to "start", defined here.  This code
#### switches from real mode to 32-bit protected mode and calls
#### main().

/* Flags in control register 0. */
#define CR0_PE 0x00000001      /* Protection Enable. */
#define CR0_EM 0x00000004      /* (Floating-point) Emulation. */
#define CR0_PG 0x80000000      /* Paging. */
#define CR0_WP 0x00010000      /* Write-Protect enable in kernel mode. */

/* Memory desciptor size used for function e820h. */
#define MEMORY_DESC_SIZE 20

	.section .start

# The following code runs in real mode, which is a 16-bit code segment.
	.code16

.func start
.globl start
start:

# The loader called into us with CS = 0x2000, SS = 0x0000, ESP = 0xf000,
# but we should initialize the other segment registers.

	mov $0x2000, %ax
	mov %ax, %ds
	mov %ax, %es

# Set string instructions to go upward.
	cld

#### Get memory size, via interrupt 15h function 88h,
#### which returns AX = (kB of physical memory) - 1024.  This only
#### works for memory sizes <= 65 MB, which should be fine for our
#### purposes.  We cap memory at 64 MB because that's all we prepare
#### page tables for, below.

	movb $0x88, %ah
	int $0x15
	addl $1024, %eax	# Total kB memory
	cmp $0x10000, %eax	# Cap at 64 MB
	jbe 1f
	mov $0x10000, %eax
1:	shrl $2, %eax		# Total 4 kB pages
	addr32 movl %eax, init_ram_pages - LOADER_PHYS_BASE - 0x20000
	

####	
#### Get extended memory size via interrupt int 15h function E820h.
####

	xor %ebx, %ebx					# Initially, ebx must be 0.
	xor %eax, %eax
	mov %ax, %es					# 
	sub $MEMORY_DESC_SIZE, %esp		# Using stack to store the descriptor.
	mov %esp, %edi					# es:edi points to the descriptor.
1:	mov $MEMORY_DESC_SIZE, %ecx		# ecx = Buffer size for descriptor. 
	mov $0x534d4150, %edx			# 'SMAP' signature. edx = "SMAP"
	mov $0xe820, %eax				# Function code eax = e820h.
	int $0x15

# Print retrieved memory region.

# Offset in Bytes		Name		Description
#	0	    BaseAddrLow		Low 32 Bits of Base Address
#	4	    BaseAddrHigh	High 32 Bits of Base Address (Ignored here.)
#	8	    LengthLow		Low 32 Bits of Length in Bytes
#	12	    LengthHigh		High 32 Bits of Length in Bytes (Ignored here.)
#	16	    Type			Address type of this range.
	xor %ecx, %ecx
2:
	mov %es:(%edi, %ecx, 8), %eax
	push %eax
	call print_uint32
	add $4, %esp
	inc %ecx
	cmp $3, %ecx
	jb 2b

	mov $'\r', %al
	call write_char_to_screen
	mov $'\n', %al
	call write_char_to_screen

	cmpl $0x100000, %es:(%edi)	# Lower 32 bits of base addr
	jb 3f
	cmpl $0x1, %es:16(%edi)		# Available RAM region
	jne 3f
	mov %es:8(%edi), %eax
	cmpl $0x100000 * 63, %eax	# If this RAM region is greater than 64M.
	jbe 3f
	add $0x100000, %eax			# Lower 1M memory range.
	shr $12, %eax				# Number of total available 4K pages.
	addr32 mov %eax, init_ram_pages - LOADER_PHYS_BASE - LOADER_KERN_BASE
	jmp 4f
	
3:	cmp $0, %ebx
	jnz 1b

4:	add $MEMORY_DESC_SIZE, %esp

#### Enable A20.  Address line 20 is tied low when the machine boots,
#### which prevents addressing memory about 1 MB.  This code fixes it.

# Poll status register while busy.

1:	inb $0x64, %al
	testb $0x2, %al
	jnz 1b

# Send command for writing output port.

	movb $0xd1, %al
	outb %al, $0x64

# Poll status register while busy.

1:	inb $0x64, %al
	testb $0x2, %al
	jnz 1b

# Enable A20 line.

	movb $0xdf, %al
	outb %al, $0x60

# Poll status register while busy.

1:	inb $0x64, %al
	testb $0x2, %al
	jnz 1b

#### Create temporary page directory and page table and set page
#### directory base register.

# Create page directory at 0xf000 (60 kB) and fill with zeroes.
# rep stosl
# For ecx repetitions, stores the contents of eax into where edi points to, 
# incrementing or decrementing edi (depending on the direction flag) by 4 bytes each time.
	mov $0xf00, %ax
	mov %ax, %es	# Default segment register is 'es' for instruction 'stos'.
	xor %eax, %eax
	xor %edi, %edi
	mov $1024, %ecx
	rep stosl

# Memory mapping:
# Phy					Vir
# 0 ~ upto 896M			0 ~ upto 896M
# 0 ~ upto 896M			3G ~ upto 3G + 896M
#
# Initial memory mapping: 
# Phy					Vir
# 0 ~ 64M				0 ~ 64M
# 0 ~ 64M				3G ~ 3G + 64M

# Add PDEs to point to page tables for the first 64 MB of RAM.
# Also add identical PDEs starting at LOADER_PHYS_BASE.
# See [IA32-v3a] section 3.7.6 "Page-Directory and Page-Table Entries"
# for a description of the bits in %eax.

	addr32 movl init_ram_pages - LOADER_PHYS_BASE - LOADER_KERN_BASE, %ecx
	shr $10, %ecx	# Meaningful page entry number.	
	cmp $0x10, %ecx	# Map the first 64 MB
	jbe 1f
	mov $0x10, %ecx

1:	mov $0x00010007, %eax	# Temporary page table starts from 0x00010000.
	xor %edi, %edi
1:	mov %eax, %es:(%edi)
	mov %eax, %es:LOADER_PHYS_BASE >> 20(%edi)
	add $4, %edi
	add $0x1000, %eax
	loop 1b

# Set up page tables for one-to-map linear to physical map for the
# first 64 MB of RAM.
# See [IA32-v3a] section 3.7.6 "Page-Directory and Page-Table Entries"
# for a description of the bits in %eax.
	mov $0x1000, %ax
	mov %ax, %es
	mov $0x007, %eax
	xor %edi, %edi
	mov $0x4000, %ecx	
1:	mov %eax, %es:(%edi)
	add $0x1000, %eax
	add $4, %edi
	loop 1b

# Set page directory base register.

	mov $0xf000, %eax
	mov %eax, %cr3

#### Switch to protected mode.

# First, disable interrupts.  We won't set up the IDT until we get
# into C code, so any interrupt would blow us away.

	cli

# Protected mode requires a GDT, so point the GDTR to our GDT.
# We need a data32 prefix to ensure that all 32 bits of the GDT
# descriptor are loaded (default is to load only 24 bits).
# The CPU doesn't need an addr32 prefix but ELF doesn't do 16-bit
# relocations.

	data32 addr32 lgdt gdt_reg - LOADER_PHYS_BASE - LOADER_KERN_BASE

# Then we turn on the following bits in CR0:
#    PE (Protect Enable): this turns on protected mode.
#    PG (Paging): turns on paging.
#    WP (Write Protect): if unset, ring 0 code ignores
#       write-protect bits in page tables (!).
#    EM (Emulation): forces floating-point instructions to trap.
#       We don't support floating point.

	mov %cr0, %eax
	orl $CR0_PE | CR0_PG | CR0_WP | CR0_EM, %eax
	mov %eax, %cr0

# We're now in protected mode in a 16-bit segment.  The CPU still has
# the real-mode code segment cached in %cs's segment descriptor.  We
# need to reload %cs, and the easiest way is to use a far jump.
# Because we're not running in a 32-bit segment the data32 prefix is
# needed to jump to a 32-bit offset in the target segment.

	data32 ljmp $SEL_KCSEG, $1f

# We're now in protected mode in a 32-bit segment.
# Let the assembler know.

	.code32

# Reload all the other segment registers and the stack pointer to
# point into our new GDT.

1:	mov $SEL_KDSEG, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	add $LOADER_PHYS_BASE, %esp
	mov %esp, %ebp

# Set default interrupt descriptor table entries.
	call set_init_idt
	lidt idt_reg

#### Call main().

	call main

# main() shouldn't ever return.  If it does, spin.

	jmp .
.endfunc

# Helper functions in read mode.
	.code16

.func print_uint32
print_uint32:
	push %ebp
	mov %esp, %ebp
	pusha

	movl 6(%ebp), %edx		# Real mode, the ip saved in stack is of 2 bytes.
	bswap %edx
	mov $4, %ecx
	
1:
	mov %dl, %al
	shr $4, %al
	call one_digit_hex_to_char
	call write_char_to_screen

	mov %dl, %al
	call one_digit_hex_to_char
	call write_char_to_screen

	shr $8, %edx
	dec %ecx
	cmp $0, %ecx
	ja 1b

	# Write a space
	mov $' ', %al
	call write_char_to_screen

	popa
	mov %ebp, %esp
	pop %ebp
	ret
.endfunc

# Change the 4 lower bits of AL to ASCII character
.func one_digit_hex_to_char
one_digit_hex_to_char:
	and $0x0f, %al
	cmp $0x0a, %al
	jae 1f
	add $0x30, %al
	jmp 2f
1:
	add $'A' - 0x0a, %al 
2:
	ret
.endfunc

# AH = 0eh : Write a character to the screen. 
# al - character, bh - page.
.func write_char_to_screen
write_char_to_screen:
	xor %bh, %bh	# Page 0.
	mov $0x0e, %ah	# Function 0eh
	int $0x10
	ret
.endfunc

	.code32

# Set all initial entries to interrupt gate whose handler is 'unknown_isr'.
.func set_init_idt
set_init_idt:
	mov $(idt_end - idt_start), %ecx
	shr $3, %ecx
	lea unknown_isr, %edx
	mov $SEL_KCSEG << 16, %eax
	mov %dx, %ax
	mov $0x8e00, %dx
	lea idt_start, %edi

1:
	mov %eax, (%edi)
	mov %edx, 4(%edi)
	add $8, %edi
	loop 1b
	ret
.endfunc

.func unknown_isr
unknown_isr:
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp
	push %ds
	push %es
	push %fs
	push %gs
	mov $SEL_KDSEG, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	push $unknown_isr_msg
	call printk
	add $4, %esp
	pop %gs
	pop %fs
	pop %es
	pop %ds
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret
.endfunc

	.data

#### GDT

	.align 8
gdt_start:
	.quad 0x0000000000000000	# Null descriptor
	.quad 0x00cf9a000000ffff	# Kernel code segment, base = 0, limit 4G.
	.quad 0x00cf92000000ffff	# Kernel data segment, base = 0, limit 4G.

gdt_end:
gdt_reg:
	.word	gdt_end - gdt_start - 1 # The limit is 1 less than the length of the table.
	.long	gdt_start

#### IDT

.global idt_start
idt_start:
	.fill 256,8,0

idt_end:
idt_reg:
	.word	256 * 8 - 1
	.long 	idt_start

#### Physical memory size in 4 kB pages.  This is exported to the rest
#### of the kernel.
.globl init_ram_pages
init_ram_pages:
	.long 0

#### Unimplemented interrupt message
unknown_isr_msg:
	.asciz	"Unknown interrupt\r\n"
