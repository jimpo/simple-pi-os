#ifndef __EXEC_H__
#define __EXEC_H__

#include <libpi/rpi.h>

#include "disk/fs.h"
#include "vm.h"

typedef struct {
    unsigned heap_start;
    unsigned heap_end;
    page_table_t page_tab;
} process_t;

// Change ASID
// ELF exec
// Revert ASID
int exec_file(pi_file_t* file, page_table_t* kernel_pt);

#endif