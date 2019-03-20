#include "elf.h"
#include "exec.h"
#include "layout.h"
#include "syscall.h"
#include "vm.h"

static unsigned exec_load_instr(process_t* proc, load_instr_t* instr) {
    // Adjust the source address from the location in the parent's address
    // space to the child's.
    instr->src_addr += PROC_LOAD_ADDR - KERNEL_LOAD_ADDR;

    switch (instr->op) {
        case LOAD_COPY:
            memcpy(instr->dst_addr, instr->src_addr, instr->size);
            return 1;
        case LOAD_ZERO:
            memset(instr->dst_addr, 0, instr->size);
            return 1;
        case LOAD_JUMP:
            if (!proc->heap_start) {
                printk("No heap address set\n");
                return 0;
            }
            proc->heap_end = proc->heap_start;

            process_t* original_proc = syscall_get_current_proc();
            syscall_set_current_proc(proc);
            BRANCHTO((unsigned) instr->dst_addr);
            syscall_set_current_proc(original_proc);

            return 1;
        default:
            printk("Unknown load instruction op: %d\n", instr->op);
            return 0;
    }
}

void exec_file(fat32_fs_t* fs, dirent_t* entry, process_t* kernel_proc) {
    process_t proc;
    proc.heap_start = 0;
    proc.heap_end = 0;

    page_table_t* pt = &proc.page_tab;
    pt->asid = 2;
    pt->vaddr_map = TT_BASE + pt->asid;
    memset(pt->vaddr_map, 0, sizeof(proc.page_tab));
    mmu_init(pt->vaddr_map);
    mmu_map_kernel_sections(pt);

    // Load the file into a special section of virtual memory that can be shared
    // with the new process, then set the heap back to normal after the allocation.
    // No need to free the memory at the end as it will be forgotten and reused
    // on the next call to exec_load. We are just going to load the file data first
    // into the heap then copy to the load section because I'm lazy.
    pi_file_t file = fat32_read_file(fs, entry);
    char* old_data = file.data;
    file.data = (char*) KERNEL_LOAD_ADDR;
    mmu_map_to_mem(&kernel_proc->page_tab, (unsigned) file.data, file.n_alloc, true);
    memcpy(file.data, old_data, file.n_data);

    // Now map the file pages into the child process.
    unsigned file_start_page = ((unsigned) file.data) >> 20;
    unsigned file_end_page = (((unsigned) file.data) + file.n_data - 1) >> 20;
    for (unsigned page = file_start_page; page <= file_end_page; page++) {
        fld_t* pte = ((fld_t*) kernel_proc->page_tab.vaddr_map) + page;
        unsigned child_va = (page << 20) - KERNEL_LOAD_ADDR + PROC_LOAD_ADDR;
        mmu_map_section(&proc.page_tab, child_va, pte->sec_base_addr << 20, MAP_FLAG_SHARED);
    }

    load_instr_t instr_lst[256];
    load_instr_t* instr_lst_end = elf_load(&file, &proc, instr_lst, instr_lst + 256);
    if (instr_lst_end) {
        // Context switch to new process. No need to invalidate any caches
        // according to ARM spec B1-6.
        set_procid_ttbr0(pt->asid, (fld_t*) pt->vaddr_map);

        // Execute all load instructions in order in the child process's address space.
        for (load_instr_t* instr = instr_lst; instr != instr_lst_end; instr++) {
            if (!exec_load_instr(&proc, instr)) {
                break;
            }
        }

        // Context switch back
        page_table_t* kernel_pt = &kernel_proc->page_tab;
        set_procid_ttbr0(kernel_pt->asid, (fld_t*) kernel_pt->vaddr_map);

        // Flush all TLB entries for the process.
        mmu_inv_asid(proc.page_tab.asid);
    }

    mmu_unmap(pt);
}
