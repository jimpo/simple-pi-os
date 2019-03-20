#include "elf.h"

int elf_magic_valid(Elf32_Ehdr* ehdr) {
    return !strncmp((char*)ehdr->e_ident, "\x7F""ELF", 4);
}

// Returns the number of symbol table entries or 0 if not found.
// Also writes the object file offset of the symbol table to off_out.
static size_t locate_symbol_table(Elf32_Shdr* shdr_tab, size_t n_shdr, Elf32_Off* off_out) {
    for (int i = 0; i < n_shdr; i++) {
        Elf32_Shdr* shdr = shdr_tab + i;
        switch (shdr->sh_type) {
            case SHT_SYMTAB:
                *off_out = shdr->sh_offset;
                demand(shdr->sh_entsize == sizeof(Elf32_Sym), symbol table entsize is wrong);
                return shdr->sh_size / sizeof(Elf32_Sym);
        }
    }
    return 0;
}

// Find all .bss sections and zero them out.
// ELF Specification, p1-14
static load_instr_t* init_bss_sections(page_table_t* pt, Elf32_Shdr* shdr_tab, size_t n_shdr,
                                       char* sh_str_tab,
                                       load_instr_t* instr_lst, load_instr_t* instr_lst_end) {
    for (int i = 0; i < n_shdr; i++) {
        Elf32_Shdr* shdr = shdr_tab + i;

        char* name;
        switch (shdr->sh_type) {
            case SHT_NOBITS:
                name = sh_str_tab + shdr->sh_name;
                if (strcmp(name, ".bss") == 0) {
                    mmu_map_to_mem(pt, shdr->sh_addr, shdr->sh_size, false);

                    demand(instr_lst != instr_lst_end, load instruction list capacity too small);
                    instr_lst->op = LOAD_ZERO;
                    instr_lst->dst_addr = (void*) shdr->sh_addr;
                    instr_lst->size = shdr->sh_size;
                    instr_lst++;
                }
        }
    }

    return instr_lst;
}

// Heap begins after the .data and .bss sections.
static Elf32_Addr get_heap_address(page_table_t* pt, Elf32_Shdr* shdr_tab, size_t n_shdr,
                                   char* sh_str_tab) {
    Elf32_Addr heap_addr = 0;

    for (int i = 0; i < n_shdr; i++) {
        Elf32_Shdr* shdr = shdr_tab + i;

        char* name = sh_str_tab + shdr->sh_name;
        if ((shdr->sh_type == SHT_NOBITS && strcmp(name, ".bss") == 0) ||
                (shdr->sh_type == SHT_PROGBITS &&
                        (strcmp(name, ".data") == 0 ||
                                strcmp(name, ".data1") == 0))) {
            heap_addr = MAX(heap_addr, shdr->sh_addr + shdr->sh_size);
        }
    }

    return heap_addr;
}

static Elf32_Shdr* locate_text_section(Elf32_Shdr* shdr_tab, size_t n_shdr, char* sh_str_tab) {
    for (int i = 0; i < n_shdr; i++) {
        Elf32_Shdr* shdr = shdr_tab + i;

        char* name;
        switch (shdr->sh_type) {
            case SHT_PROGBITS:
                name = sh_str_tab + shdr->sh_name;
                if (strcmp(name, ".text") == 0) {
                    return shdr;
                }
        }
    }
    return NULL;
}


load_instr_t* elf_load(pi_file_t* file, process_t* proc,
                       load_instr_t* instr_lst, load_instr_t* instr_lst_end) {
    if (file->n_data < sizeof(Elf32_Ehdr)) {
        printk("Not an ELF32 file (too short)\n");
        return NULL;
    }

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)file->data;
    if (!elf_magic_valid(ehdr)) {
        ehdr->e_ident[4] = 0;
        printk("Not an ELF32 file (incorrect magic)\n");
        return NULL;
    }

    switch (ehdr->e_type) {
        case ET_EXEC:
            break;
        default:
            printk("ELF type %s not supported\n", elf_type_str(ehdr->e_type));
            return NULL;
    }

    Elf32_Phdr* phdr_tab = (Elf32_Phdr*)(file->data + ehdr->e_phoff);
    Elf32_Shdr* shdr_tab = (Elf32_Shdr*)(file->data + ehdr->e_shoff);
    char* sh_str_tab = file->data + shdr_tab[ehdr->e_shstrndx].sh_offset;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf32_Phdr* phdr = phdr_tab + i;
        switch (phdr->p_type) {
            case PT_LOAD:
                mmu_map_to_mem(&proc->page_tab, phdr->p_vaddr, phdr->p_filesz, false);

                demand(instr_lst != instr_lst_end, load instruction list capacity too small);
                instr_lst->op = LOAD_COPY;
                instr_lst->dst_addr = (void*) phdr->p_vaddr;
                instr_lst->src_addr = file->data + phdr->p_offset;
                instr_lst->size = phdr->p_filesz;
                instr_lst++;

                break;
            default:
                printk("ELF program header type %d not supported\n", phdr->p_type);
                return NULL;
        }
    }

    // According to ELF Specification, p1-10, there is at most one SYMTAB section.
    Elf32_Off sym_tab_off;
    size_t n_sym_tab = locate_symbol_table(shdr_tab, ehdr->e_shnum, &sym_tab_off);
    if (!n_sym_tab) {
        printk("Symbol table not found\n");
        return NULL;
    }
    // Elf32_Sym* sym_tab = (Elf32_Sym*)(file->data + sym_tab_off);

    instr_lst = init_bss_sections(&proc->page_tab, shdr_tab, ehdr->e_shnum, sh_str_tab,
            instr_lst, instr_lst_end);

    proc->heap_start = get_heap_address(&proc->page_tab, shdr_tab, ehdr->e_shnum, sh_str_tab);

    Elf32_Shdr* text_sh = locate_text_section(shdr_tab, ehdr->e_shnum, sh_str_tab);
    if (!text_sh) {
        printk("Could not locate entry section\n");
        return NULL;
    }

    demand(instr_lst != instr_lst_end, load instruction list capacity too small);
    instr_lst->op = LOAD_JUMP;
    instr_lst->dst_addr = (void*) text_sh->sh_addr;
    instr_lst++;

    return instr_lst;
}

const char* elf_type_str(Elf32_Half type) {
    switch (type) {
        case ET_NONE:
            return "NONE";
        case ET_REL:
            return "NONE";
        case ET_EXEC:
            return "EXEC";
        case ET_DYN:
            return "DYN";
        case ET_CORE:
            return "CORE";
        case ET_LOPROC:
            return "LOPROC";
        case ET_HIPROC:
            return "HIPROC";
        default:
            return "UNKNOWN";
    }
}

