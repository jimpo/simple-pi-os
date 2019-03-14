#include <libpi/rpi.h>

#include "control.h"
#include "disk/sd.h"

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
    demand(sd_init() == SD_OK, failed to initialize SD card);

	int n;
	char buf[1024];

    printk("%s\n", ready);

    while((n = readline(buf, sizeof buf))) {
        if (strncmp(buf, "read", 5) == 0) {
            printk("%s\n", buf + 5);
            continue;
        }
        if (strncmp(buf, "write", 6) == 0) {
            printk("%s\n", pi_done);
            delay_ms(100);
            rpi_reboot();
		}
        if (strncmp(buf, "list", 5) == 0) {
            printk("%s\n", cmd_done);
            continue;
        }
	}
	clean_reboot();
}