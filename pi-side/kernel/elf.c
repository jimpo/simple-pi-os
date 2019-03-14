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
static void init_bss_sections(Elf32_Shdr* shdr_tab, size_t n_shdr, char* sh_str_tab) {
    for (int i = 0; i < n_shdr; i++) {
        Elf32_Shdr* shdr = shdr_tab + i;

        char* name;
        switch (shdr->sh_type) {
            case SHT_NOBITS:
                name = sh_str_tab + shdr->sh_name;
                if (strcmp(name, ".text") == 0) {
                    memset(shdr->sh_addr, 0, shdr->sh_size);
                }
        }
    }
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

int elf_exec(pi_file_t* file) {
    if (file->n_data < sizeof(Elf32_Ehdr)) {
        printk("Not an ELF32 file (too short)\n");
        return -1;
    }

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)file->data;
    if (!elf_magic_valid(ehdr)) {
        ehdr->e_ident[4] = 0;
        printk("Not an ELF32 file (incorrect magic)\n");
        return -1;
    }

    switch (ehdr->e_type) {
        case ET_EXEC:
            break;
        default:
            printk("ELF type %s not supported\n", elf_type_str(ehdr->e_type));
            return -1;
    }

    Elf32_Phdr* phdr_tab = (Elf32_Phdr*)(file->data + ehdr->e_phoff);
    Elf32_Shdr* shdr_tab = (Elf32_Shdr*)(file->data + ehdr->e_shoff);
    char* sh_str_tab = file->data + shdr_tab[ehdr->e_shstrndx].sh_offset;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf32_Phdr* phdr = phdr_tab + i;
        switch (phdr->p_type) {
            case PT_LOAD:
                memcpy((char*) phdr->p_paddr, file->data + phdr->p_offset, phdr->p_filesz);
                break;
            default:
                printk("ELF program header type %d not supported\n", phdr->p_type);
                return -1;
        }
    }

    // According to ELF Specification, p1-10, there is at most one SYMTAB section.
    Elf32_Off sym_tab_off;
    size_t n_sym_tab = locate_symbol_table(shdr_tab, ehdr->e_shnum, &sym_tab_off);
    if (!n_sym_tab) {
        printk("Symbol table not found\n");
        return -1;
    }
    Elf32_Sym* sym_tab = (Elf32_Sym*)(file->data + sym_tab_off);

    init_bss_sections(shdr_tab, ehdr->e_shnum, sh_str_tab);

    Elf32_Shdr* text_sh = locate_text_section(shdr_tab, ehdr->e_shnum, sh_str_tab);
    if (!text_sh) {
        printk("Could not locate entry section\n");
        return -1;
    }

    BRANCHTO(text_sh->sh_addr);

    return 0;
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

