/* simplified bootloader.  */
#include <libpi/rpi.h>

#include "shell.h"

#define __SIMPLE_IMPL__
#include <bootloader/simple-boot.h>

static void send_byte(unsigned char uc) {
    uart_putc(uc);
}
static unsigned char get_byte(void) {
    return uart_getc();
}

unsigned get_uint(void) {
    unsigned u = get_byte();
    u |= get_byte() << 8;
    u |= get_byte() << 16;
    u |= get_byte() << 24;
    return u;
}
void put_uint(unsigned u) {
    send_byte((u >> 0)  & 0xff);
    send_byte((u >> 8)  & 0xff);
    send_byte((u >> 16) & 0xff);
    send_byte((u >> 24) & 0xff);
}

// load_code:
//	1. figure out if the requested address range is available.
//	2. copy code to that region.
//	3. return address of first executable instruction: note we have
//	a 8-byte header!  (see ../hello-fixed/memmap)
unsigned load_code(void) {
    // let unix know we are ready.
    put_uint(ACK);

    // bootloader code.
    unsigned version = get_uint();
    unsigned addr = get_uint();
    unsigned nbytes = get_uint();
    unsigned checksum = get_uint();

    if (version != PROTOCOL_VERSION) {
        put_uint(NAK);
        return 0;
    }
    if (addr < LAST_USED_ADDRESSES || addr + nbytes > MAX_ADDRESS) {
        put_uint(NAK);
        return 0;
    }
    if (nbytes % 4 != 0) {
        put_uint(NAK);
        return 0;
    }

    put_uint(ACK);

    for (int i = 0; i < nbytes; i += 4) {
        PUT32(addr + i, get_uint());
    }
    if (get_uint() != EOT) {
        put_uint(NAK);
        return 0;
    }
    if (crc32((void*)addr, nbytes) != checksum) {
        put_uint(NAK);
        return 0;
    }

    put_uint(ACK);

    // give time to flush out; ugly.   implement `uart_flush()`
    delay_ms(100);

    return addr + 8;
}
