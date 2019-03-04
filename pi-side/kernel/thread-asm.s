@empty stub routines.  use these, or make your own.

.globl test_csave
test_csave:
    sub r0, r0, #0x40

    str r0,  [r0, #0x04]
    str r1,  [r0, #0x08]
    str r2,  [r0, #0x0C]
    str r3,  [r0, #0x10]
    str r4,  [r0, #0x14]
    str r5,  [r0, #0x18]
    str r6,  [r0, #0x1C]
    str r7,  [r0, #0x20]
    str r8,  [r0, #0x24]
    str r9,  [r0, #0x28]
    str r10, [r0, #0x2C]
    str r11, [r0, #0x30]
    str r12, [r0, #0x34]
    str r13, [r0, #0x38]
    str r14, [r0, #0x3C]

    @store CPSR state
    mrs r1, cpsr
    str r1, [r0, #0x40]

	bx lr

.globl test_csave_stmfd
test_csave_stmfd:
	bx lr

.globl rpi_get_cpsr
rpi_get_cpsr:
    mrs r0, cpsr
    bx lr

.globl rpi_cswitch
rpi_cswitch:
    @ sub sp, sp, #0x0C
    @ str r0, [sp, #0x04]
    @ str r1, [sp, #0x08]
    @ str lr, [sp, #0x0C]
    @ mov r0, sp
    @ bx test_csave
	@ mov r2, r0
    @ ldr r0, [sp, #0x04]
    @ ldr r1, [sp, #0x08]
    @ ldr lr, [sp, #0x0C]
    @ mov sp, r2
    sub sp, sp, #0x3C

    str r0,  [sp, #0x04]
    str r1,  [sp, #0x08]
    str r2,  [sp, #0x0C]
    str r3,  [sp, #0x10]
    str r4,  [sp, #0x14]
    str r5,  [sp, #0x18]
    str r6,  [sp, #0x1C]
    str r7,  [sp, #0x20]
    str r8,  [sp, #0x24]
    str r9,  [sp, #0x28]
    str r10, [sp, #0x2C]
    str r11, [sp, #0x30]
    str r12, [sp, #0x34]
    str lr,  [sp, #0x38]

    @store CPSR state
    mrs r2, cpsr
    str r2, [sp, #0x3C]

    @return old stack pointer in r0
    str sp, [r0]

    @switch stack to new location
    ldr sp, [r1]

    @load CPSR state
    ldr r2, [sp, #0x3C]
    msr cpsr, r2

    ldr r0,  [sp, #0x04]
    ldr r1,  [sp, #0x08]
    ldr r2,  [sp, #0x0C]
    ldr r3,  [sp, #0x10]
    ldr r4,  [sp, #0x14]
    ldr r5,  [sp, #0x18]
    ldr r6,  [sp, #0x1C]
    ldr r7,  [sp, #0x20]
    ldr r8,  [sp, #0x24]
    ldr r9,  [sp, #0x28]
    ldr r10, [sp, #0x2C]
    ldr r11, [sp, #0x30]
    ldr r12, [sp, #0x34]
    ldr lr,  [sp, #0x38]

    add sp, sp, #0x3C
	bx lr


@ [Make sure you can answer: why do we need to do this?]
@
@ use this to setup each thread for the first time.
@ setup the stack so that when cswitch runs it will:
@	- load address of <rpi_init_trampoline> into LR
@	- <code> into r1,
@	- <arg> into r0
@
.globl rpi_init_trampoline
rpi_init_trampoline:
    blx r1
    bl rpi_exit
