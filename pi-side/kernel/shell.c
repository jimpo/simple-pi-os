#include "libpi/rpi.h"
#include "sd-card.h"
#include "shell.h"

// have pi send this back when it reboots (otherwise my-install exits).
static const char pi_done[] = "PI REBOOT!!!";
// pi sends this after a program executes to indicate it finished.
static const char cmd_done[] = "CMD-DONE";
// pi sends this before shell loops begins.
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

void notmain() {
	uart_init();
	int n;
	char buf[1024];

    SDRESULT init_result = sdInitCard(printk, printk, 1);
    printk("init_result = %d\n", init_result);
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
        if (strncmp(buf, "ls", 3) == 0) {
            FIND_DATA file_data;
            for (HANDLE file_handle = sdFindFirstFile("*", &file_data);
                 file_handle;
                 file_handle = sdFindNextFile(file_handle, &file_data)) {
                printk("%s\n", file_data.cFileName);
            }
            printk("%s\n", cmd_done);
            continue;
        }
        if (strncmp(buf, "run", 4) == 0) {
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
