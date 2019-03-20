#ifndef __SIMPLE_BOOT_H__
#define  __SIMPLE_BOOT_H__

void simple_boot(int fd, const unsigned char * buf, unsigned n);

/*
 * Protocol, Send:
 * 	SOH
 * 	bytes
 *	cksum
 *	<program>
 *	EOT
 */
enum {
    ARMBASE=0x8000, // where program gets linked.  we could send this.
    SOH = 0x12345678,   // Start Of Header

    BAD_CKSUM = 0x1,
    BAD_START,
    BAD_END,
    TOO_BIG,
    ACK,   // client ACK
    NAK,   // Some kind of error, restart
    EOT,   // end of transmission
};

#endif
