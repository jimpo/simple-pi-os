/*
 * interrupt-asm.s
 *
 * Code for interrupt handling.  Refer to armisa.pdf in docs/ for what
 * the opcodes mean.
 */

#include "arm-coprocessor-insts.h"
#include "layout.h"

/*
 * Enable/disable interrupts.
 *
 * CPSR = current program status register
 *        upper bits are different carry flags.
 *        lower 8:
 *           7 6 5 4 3 2 1 0
 *          +-+-+-+---------+
 *          |I|F|T|   Mode  |
 *          +-+-+-+---------+
 *
 *  I : disables IRQ when = 1.
 *  F : disables FIQ when = 1.
 *  T : = 0 indicates ARM execution, = 1 is thumb execution.
 *      Mode = current mode.
 */
.globl system_enable_interrupts
system_enable_interrupts:
    mrs r0,cpsr                 @ move process status register (PSR) to r0
    bic r0,r0,#(1<<7)		@ clear 7th bit.
    msr cpsr_c,r0		@ move r0 back to PSR
    bx lr		        @ return.

.globl system_disable_interrupts
system_disable_interrupts:
    mrs r0,cpsr		       
    orr r0,r0,#(1<<7)	       @ set 7th bit: or in 0b100 0000
    msr cpsr_c,r0
    bx lr


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@
@ put your code below.

.globl _interrupt_table
.globl _interrupt_table_end
_interrupt_table:
  ldr pc, _reset_asm
  ldr pc, _undefined_instruction_asm
  ldr pc, _software_interrupt_asm
  ldr pc, _prefetch_abort_asm
  ldr pc, _data_abort_asm
  ldr pc, _reset_asm
  ldr pc, _interrupt_asm
fast_interrupt_asm:
  sub   lr, lr, #4 @First instr of FIQ handler
  push  {lr}
  push  {r0-r12}
  mov   r0, lr              @ Pass old pc
  bl    fast_interrupt_vector    @ C function
  pop   {r0-r12}
  ldm   sp!, {pc}^

_reset_asm:                   .word reset_asm
_undefined_instruction_asm:   .word undefined_instruction_asm
_software_interrupt_asm:      .word software_interrupt_asm
_prefetch_abort_asm:          .word prefetch_abort_asm
_data_abort_asm:              .word data_abort_asm
_interrupt_asm:               .word interrupt_asm
_interrupt_table_end:

software_interrupt_asm:
    push {r0-r12,lr}         @ XXX: pushing too many registers: only need caller

    ldr r0, [lr, #-4]
    and r0, r0, #0xFF
    mov r1, lr
    mov r2, sp

    bl handle_swi

    pop {r0-r12,lr} 	    @ pop integer registers

    @ return from interrupt handler: will re-enable general ints.
    movs    pc, lr        @ moves the link register into the pc and implicitly
                          @ loads the PC with the result, then copies the
                          @ SPSR to the CPSR.

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ we don't generate any of these, will just panic and halt.
@
interrupt_asm:
    mov   sp, #(INT_STACK_ADDR)
    sub   lr, lr, #4
    bl    interrupt_vector
reset_asm:
    mov   sp, #(INT_STACK_ADDR)
    sub   lr, lr, #4
    bl    reset_vector
undefined_instruction_asm:
    mov   sp, #(INT_STACK_ADDR)
    sub   lr, lr, #4
    bl    undefined_instruction_vector
prefetch_abort_asm:
    mov   sp, #(INT_STACK_ADDR)
    sub   lr, lr, #4
    bl    prefetch_abort_vector
data_abort_asm:
    mov   sp, #(INT_STACK_ADDR)
    sub   lr, lr, #4
    bl    data_abort_vector

.globl cpsr_read
cpsr_read:
    mrs r0, cpsr
    bx lr

.globl swi_setup_stack
swi_setup_stack:
    @ mrs r1, cpsr
    mov r1, #(SYS_MODE)
    mov r2, sp
    mov r3, lr

    @ switch to super mode
    msr cpsr_c, #(SUPER_MODE)

    @ write stack pointer
    mov sp, r0

    @ switch back to original mode
    msr cpsr_c, r1
    mov sp, r2
    mov lr, r3

    bx lr

.globl swi_asm1
swi_asm1:
    swi 0x01
    bx lr
