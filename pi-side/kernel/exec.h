#ifndef __EXEC_H__
#define __EXEC_H__

#include <libpi/rpi.h>

#include "disk/fs.h"
#include "process.h"
#include "vm.h"

#define LOAD_COPY 1
#define LOAD_ZERO 2
#define LOAD_JUMP 3

typedef struct {
    // Operation (eg. LOAD_*)
    uint8_t op;
    void* src_addr;
    void* dst_addr;
    size_t size;
} load_instr_t;

// Change ASID
// ELF exec
// Revert ASID
void exec_file(fat32_fs_t* fs, dirent_t* entry, process_t* kernel_proc);

#endif