#include "syscall.h"
#include "vm.h"

static process_t* kernel_proc;
static process_t* current_proc;

void syscall_set_kernel_proc(process_t* proc) {
    kernel_proc = proc;
}

void syscall_set_current_proc(process_t* proc) {
    current_proc = proc;
}

void* kernel_sbrk(unsigned incr) {
    assert(kernel_proc && current_proc);

    if (incr) {
        if (current_proc != kernel_proc) {
            // Context switch to the kernel.
            set_procid_ttbr0(kernel_proc->page_tab.asid, (fld_t *) kernel_proc->page_tab.vaddr_map);
        }

        mmu_map_to_mem(&current_proc->page_tab, current_proc->heap_end, incr, true);
        current_proc->heap_end += incr;

        if (current_proc != kernel_proc) {
            // Context switch back to current process.
            set_procid_ttbr0(current_proc->page_tab.asid,
                             (fld_t *) current_proc->page_tab.vaddr_map);
        }
    }

    return (void*) current_proc->heap_end;
}
