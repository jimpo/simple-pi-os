#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "exec.h"

void syscall_set_kernel_proc(process_t* proc);
void syscall_set_current_proc(process_t* proc);

void* kernel_sbrk(unsigned incr);

#endif