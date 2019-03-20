#ifndef __KERNEL_LAYOUT_H__
#define __KERNEL_LAYOUT_H__

/**
 * This defines the layout of the address space used by the kernel.
 */

#define KERNEL_STACK_ADDR   0x00008000
#define KERNEL_TEXT_ADDR    0x00008000
#define TRANS_TABLE_ADDR    0x00100000
#define KERNEL_HEAP_ADDR    0x00500000
#define KERNEL_LOAD_ADDR    0x10000000

#define PROC_LOAD_ADDR      0x80000000

#endif