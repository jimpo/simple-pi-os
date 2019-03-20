#include <libpi/rpi.h>
#include <libpi/syscall.h>

#include "disk/fat32.h"
#include "disk/fs.h"
#include "disk/sd.h"
#include "exec.h"
#include "layout.h"
#include "shell.h"
#include "syscall.h"
#include "vm.h"

// have pi send this back when it reboots (otherwise my-install exits).
static const char pi_done[] = "PI REBOOT!!!";
// pi sends this after a program executes to indicate it finished.
static const char cmd_done[] = "CMD-DONE";
// pi sends this when it's ready for another command.
static const char ready[] = "READY";

// read characters until we hit a newline.
static int readline(char *buf, int sz) {
	for(int i = 0; i < sz; i++) {
		if((buf[i] = uart_getc()) == '\n') {
			buf[i] = 0;
			return i;
		}
	}
	// just return partial read?
	panic("size too small\n");
}

void do_ls(fat32_fs_t* fs, const char* path) {
    if (path[0] != '/') {
        printk("Expected absolute path\n");
        return;
    }
    printk("%s:\n", path);

    pi_dir_t dir;
    dirent_t* entry = NULL;
    if (path[1] != '\0') {
        entry = path_lookup(fs, NULL, path + 1, &dir);
        if (!entry) {
            return;
        } else if (!entry->is_dir_p) {
            printk("%s is not a directory\n", path);
            kfree(dir.dirs);
            return;
        }
    }
    pi_dir_t list_dir = fat32_read_dir(fs, entry);
    for (int i = 0; i < list_dir.n; i++) {
        printk("%s\n", list_dir.dirs[i].name);
    }
    kfree(list_dir.dirs);
    kfree(dir.dirs);
}

void do_read(fat32_fs_t* fs, const char* path) {
    if (path[0] != '/') {
        printk("Expected absolute path\n");
        return;
    }
    printk("%s:\n", path);

    pi_dir_t dir;
    dirent_t* entry = NULL;
    if (path[1] == '\0') {
        return;
    }
    entry = path_lookup(fs, NULL, path + 1, &dir);
    if (!entry || entry->is_dir_p) {
        printk("%s is a directory\n", path);
        kfree(dir.dirs);
        return;
    }
    pi_file_t file = fat32_read_file(fs, entry);
    printk("%s\n", file);

    kfree(file.data);
    kfree(dir.dirs);
}

void do_run(fat32_fs_t* fs, const char* path, process_t* kernel_proc) {
    if (path[0] != '/') {
        printk("Expected absolute path\n");
        return;
    }
    printk("%s:\n", path);

    pi_dir_t dir;
    dirent_t* entry = NULL;
    if (path[1] == '\0') {
        return;
    }
    entry = path_lookup(fs, NULL, path + 1, &dir);
    if (!entry || entry->is_dir_p) {
        printk("%s is a directory\n", path);
        kfree(dir.dirs);
        return;
    }
    exec_file(fs, entry, kernel_proc);

    kfree(dir.dirs);
}

void notmain() {
	uart_init();
    demand(sd_init() == SD_OK, failed to initialize SD card);

    process_t kernel_proc;
    kernel_proc.page_tab = vm_enable();
    kernel_proc.heap_start = KERNEL_HEAP_ADDR;
    kernel_proc.heap_end = kernel_proc.heap_start;

    syscall_set_kernel_proc(&kernel_proc);
    syscall_set_current_proc(&kernel_proc);
    rpi_set_sbrk(kernel_sbrk);

    fat32_fs_t fs = fat32_init();

	int n;
	char buf[1024];

    printk("%s\n", ready);

    while((n = readline(buf, sizeof buf))) {
        if (strncmp(buf, "echo ", 5) == 0) {
            printk("%s\n", buf + 5);
            continue;
        }
        if (strncmp(buf, "reboot", 7) == 0) {
            printk("%s\n", pi_done);
            delay_ms(100);
            rpi_reboot();
		}
        if (strncmp(buf, "ls ", 3) == 0) {
            do_ls(&fs, buf + 3);
            printk("%s\n", cmd_done);
            continue;
        }
        if (strncmp(buf, "cat ", 4) == 0) {
            do_read(&fs, buf + 4);
            printk("%s\n", cmd_done);
            continue;
        }
        if (strncmp(buf, "run ", 4) == 0) {
            do_run(&fs, buf + 4, &kernel_proc);
            printk("%s\n", cmd_done);
            continue;
        }
        if (strncmp(buf, "load", 4) == 0) {
            unsigned addr = load_code();
            if (addr) {
                BRANCHTO(addr);
                printk("%s\n", cmd_done);
            }
            continue;
		}
	}
	clean_reboot();
}
