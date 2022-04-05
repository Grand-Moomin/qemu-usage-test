#define OFFSET_CURR 20
#define OFFSET_NEXT 24
# Current and next task struct pointer offset from esp.

.globl switch_to
.func switch_to
switch_to:
# Intel requires those regs to be preserved. 
    push %ebx
    push %ebp
    push %esi
    push %edi

    mov OFFSET_CURR(%esp), %eax
.globl saved_sp_offset
    mov saved_sp_offset, %edx
    mov %esp, (%eax, %edx, 1)

    mov OFFSET_NEXT(%esp), %ecx
    mov (%ecx, %edx, 1), %esp

    pop %edi
    pop %esi
    pop %ebp
    pop %ebx
    ret

.endfunc

# First procedure when switching to the newly created task.
.globl switch_initial
.func switch_initial
switch_initial:

    add $8, %esp    # Stack restoration of 'switch_frame'.
    push %eax       # previous task_struct pointer in eax.
    call task_sched_post_proc
    add $4, %esp
    
    /*As all preparation to create a new task has been done, re-enable interrupt. */
    sti
    
    ret

.endfunc