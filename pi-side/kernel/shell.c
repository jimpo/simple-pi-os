#include <libpi/rpi.h>

#include "disk/sd.h"
#include "shell.h"

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

void notmain() {
	uart_init();
	int n;
	char buf[1024];

	demand(sd_init() == SD_OK, failed to initialize SD card);

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
