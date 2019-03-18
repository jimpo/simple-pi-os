#include "elf.h"
#include "exec.h"
#include "vm.h"

int exec_file(pi_file_t* file, page_table_t* kernel_pt) {
    process_t proc;

    page_table_t* pt = &proc.page_tab;
    pt->asid = 2;
    pt->vaddr_map = TT_BASE + pt->asid;
    memset(pt->vaddr_map, 0, sizeof(proc.page_tab));
    mmu_init(pt->vaddr_map);
    mmu_map_kernel_sections(pt);

    unsigned addr = elf_load(file, pt);
    if (addr) {
        // Context switch to new process. No need to invalidate any caches
        // according to ARM spec B1-6.
        set_procid_ttbr0(pt->asid, (fld_t*) pt->vaddr_map);

        // Jump to start address of new section.
        BRANCHTO(addr);

        // Context switch back
        set_procid_ttbr0(kernel_pt->asid, (fld_t*) pt->vaddr_map);
    }

    mmu_unmap(pt);
}
