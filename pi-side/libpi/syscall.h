#ifndef __RPI_SYSCALL_H__
#define __RPI_SYSCALL_H__

/**
 * These are poor man's syscalls without interrupts.
 */

void rpi_set_sbrk(void* (*sbrk)(unsigned));

void* rpi_sbrk(unsigned incr);

#endif