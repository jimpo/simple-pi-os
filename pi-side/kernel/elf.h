#ifndef __ELF_H__
#define __ELF_H__

#include <libpi/rpi.h>

#include "disk/fs.h"

// ELF Specification, Figure 1-2
typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef int32_t Elf32_Word;

// ELF Specification, Figure 1-3
#define EI_NIDENT 16

typedef struct {
    unsigned char  e_ident[EI_NIDENT];
    Elf32_Half     e_type;
    Elf32_Half     e_machine;
    Elf32_Word     e_version;
    Elf32_Addr     e_entry;
    Elf32_Off      e_phoff;
    Elf32_Off      e_shoff;
    Elf32_Word     e_flags;
    Elf32_Half     e_ehsize;
    Elf32_Half     e_phentsize;
    Elf32_Half     e_phnum;
    Elf32_Half     e_shentsize;
    Elf32_Half     e_shnum;
    Elf32_Half     e_shstrndx;
} __attribute__ ((packed)) Elf32_Ehdr;

#define ET_NONE          0  // No file type
#define ET_REL           1  // Relocatable file
#define ET_EXEC          2  // Executable file
#define ET_DYN           3  // Shared object file
#define ET_CORE          4  // Core file
#define ET_LOPROC   0xff00  // Processor-specific
#define ET_HIPROC   0xffff  // Processor-specific

// ELF Specification, Figure 1.9
typedef struct {
    Elf32_Word     sh_name;
    Elf32_Word     sh_type;
    Elf32_Word     sh_flags;
    Elf32_Addr     sh_addr;
    Elf32_Off      sh_offset;
    Elf32_Word     sh_size;
    Elf32_Word     sh_link;
    Elf32_Word     sh_info;
    Elf32_Word     sh_addralign;
    Elf32_Word     sh_entsize;
} __attribute__ ((packed)) Elf32_Shdr;

// ELF Specification, Figure 1.10
#define SHT_NULL                0
#define SHT_PROGBITS            1
#define SHT_SYMTAB              2
#define SHT_STRTAB              3
#define SHT_RELA                4
#define SHT_HASH                5
#define SHT_DYNAMIC             6
#define SHT_NOTE                7
#define SHT_NOBITS              8
#define SHT_REL                 9
#define SHT_SHLIB              10
#define SHT_DYNSYM             11
#define SHT_LOPROC     0x70000000
#define SHT_HIPROC     0x7fffffff
#define SHT_LOUSER     0x80000000
#define SHT_HIUSER     0xffffffff

// ELF Specification, Figure 1-16
typedef struct {
    Elf32_Word      st_name;
    Elf32_Addr      st_value;
    Elf32_Word      st_size;
    unsigned char   st_info;
    unsigned char   st_other;
    Elf32_Half      st_shndx;
} __attribute__ ((packed)) Elf32_Sym;

// ELF Specification, Figure 1-17
#define STB_LOCAL      0
#define STB_GLOBAL     1
#define STB_WEAK       2
#define STB_LOPROC    13
#define STB_HIPROC    15

// ELF Specification, Figure 2-1
typedef struct {
    Elf32_Word     p_type;
    Elf32_Off      p_offset;
    Elf32_Addr     p_vaddr;
    Elf32_Addr     p_paddr;
    Elf32_Word     p_filesz;
    Elf32_Word     p_memsz;
    Elf32_Word     p_flags;
    Elf32_Word     p_align;
} __attribute__ ((packed)) Elf32_Phdr;

// ELF Specification, Figure 2-2
#define PT_NULL               0
#define PT_LOAD               1
#define PT_DYNAMIC            2
#define PT_INTERP             3
#define PT_NOTE               4
#define PT_SHLIB              5
#define PT_PHDR               6
#define PT_LOPROC    0x70000000
#define PT_HIPROC    0x7fffffff

int elf_magic_valid(Elf32_Ehdr* ehdr);

/** Returns:
 * 0 on success
 * -1 on failure
 */
int elf_exec(pi_file_t* file);

const char* elf_type_str(Elf32_Half type);

#endif
