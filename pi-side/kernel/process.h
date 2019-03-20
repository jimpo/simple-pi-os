#ifndef __KERNEL_PROCESS_H__
#define __KERNEL_PROCESS_H__

#include "vm.h"

typedef struct {
    unsigned heap_start;
    unsigned heap_end;
    page_table_t page_tab;
} process_t;

#endif
