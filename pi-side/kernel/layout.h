#ifndef __KERNEL_LAYOUT_H__
#define __KERNEL_LAYOUT_H__

/**
 * This defines the layout of the address space used by the kernel.
 */

// where the interrupt handlers go.
#define RPI_VECTOR_START    0x00000000

#define KERNEL_STACK_ADDR   0x00008000
#define KERNEL_TEXT_ADDR    0x00008000
#define TRANS_TABLE_ADDR    0x00100000
#define KERNEL_HEAP_ADDR    0x00500000
#define KERNEL_LOAD_ADDR    0x10000000

#define PROC_LOAD_ADDR      0x80000000

#define SYS_STACK_ADDR      0x00100000
#define SWI_STACK_ADDR      0x000F0000
// general interrupts.
#define INT_STACK_ADDR      0x000E0000

// These don't belong here, but it's an OK place for constants that are
// required by assembly files.
// from A2-2
#define USER_MODE       0b10000
#define FIQ_MODE        0b10001
#define IRQ_MODE        0b10010
#define SUPER_MODE      0b10011
#define ABORT_MODE      0b10111
#define UNDEF_MODE      0b11011
#define SYS_MODE         0b11111
#define MODE_MASK        0b11111

#endif