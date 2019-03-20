/* 
 * very simple bootloader.  more robust than xmodem.   (that code seems to 
 * have bugs in terms of recovery with inopportune timeouts.)
 */

#define __SIMPLE_IMPL__
#include "simple-boot.h"

#include "../libpi/rpi.h"

static void send_byte(unsigned char uc) {
    uart_putc(uc);
}
static unsigned char get_byte(void) { 
    return uart_getc();
}

static unsigned get_uint(void) {
    unsigned u = get_byte();
    u |= get_byte() << 8;
    u |= get_byte() << 16;
    u |= get_byte() << 24;
    return u;
}
static void put_uint(unsigned u) {
    send_byte((u >> 0)  & 0xff);
    send_byte((u >> 8)  & 0xff);
    send_byte((u >> 16) & 0xff);
    send_byte((u >> 24) & 0xff);
}

uint8_t receive_image(void) {
    // 1. wait for SOH, size, cksum from unix side.
    if (get_uint() != SOH) {
        return NAK;
    }

    uint32_t size = get_uint();
    uint32_t code_checksum = get_uint();
    uint32_t size_checksum = crc32(&size, sizeof(size));

    // 2. echo SOH, checksum(size), cksum back.
    put_uint(SOH);
    put_uint(size_checksum);
    put_uint(code_checksum);

    // 3. wait for ACK.
    if (get_uint() != ACK) {
        return NAK;
    }

    // 4. read the bytes, one at a time, copy them to ARMBASE.
    for (int i = 0; i < size; i += sizeof(uint32_t)) {
        PUT32(ARMBASE + i, get_uint());
    }

    if (get_uint() != EOT) {
        return NAK;
    }

    //	5. verify checksum.
    if (crc32((void*)ARMBASE, size) != code_checksum) {
        return NAK;
    }

    //	6. send ACK back.
    return ACK;
}

void notmain(void) {
    uart_init();

    // XXX: cs107e has this delay_cycles; doesn't seem to be required if
    // you drain the uart.
    delay_ms(500);


    /* XXX put your bootloader implementation here XXX */
    uint8_t response;
    do {
        response = receive_image();
        put_uint(response);
    } while (response != ACK);

    // XXX: appears we need these delays or the unix side gets confused.
    // I believe it's b/c the code we call re-initializes the uart; could
    // disable that to make it a bit more clean.
    delay_ms(500);

    // run what client sent.
    BRANCHTO(ARMBASE);
    // should not get back here, but just in case.
    rpi_reboot();
}
