#include "rpi.h"
#include "gpio.h"

// From BCM2835 Data Sheet p11
// baudrate = system_clock_freq / 8 * (baudrate_reg + 1)
// baudrate_reg = system_clock_freq / 8 * baudrate - 1
// system_clock_freq = 250 MHz
// baudrate = 115200
const unsigned int BAUD_RATE = 270;

// Auxiliary peripherals Register Map offset (BCM2835 Data Sheet p8).
// Instead of using 0x7E215000 as written, which is the bus address, the value
// is translated to the physical address beginning with 0x20
// (BCM2835 Data Sheet p6).
#define AUX_BASE 0x20215000
const unsigned int AUX_IRQ = AUX_BASE + 0x00;
const unsigned int AUX_ENB = AUX_BASE + 0x04;
const unsigned int AUX_MU_IO   = AUX_BASE + 0x40;
const unsigned int AUX_MU_IER  = AUX_BASE + 0x44;
const unsigned int AUX_MU_IIR  = AUX_BASE + 0x48;
const unsigned int AUX_MU_LCR  = AUX_BASE + 0x4c;
const unsigned int AUX_MU_CNTL = AUX_BASE + 0x60;
const unsigned int AUX_MU_STAT = AUX_BASE + 0x64;
const unsigned int AUX_MU_BAUD = AUX_BASE + 0x68;

void uart_init(void) {
    // Memory barrier before GPIO operations.
    dev_barrier();

    // GPIO 14 alternate function 5 is TXD1 (BCM2835 Data Sheet p102).
    gpio_set_function(GPIO_PIN14, GPIO_FUNC_ALT5);

    // GPIO 15 alternate function 5 is RXD1 (BCM2835 Data Sheet p102).
    gpio_set_function(GPIO_PIN15, GPIO_FUNC_ALT5);

    // Memory barrier between GPIO operations and AUX operations.
    dev_barrier();

    // Enable the UART while configuring.
    // Setting bit 0 enables miniUART (BCM2835 Data Sheet p9).
    unsigned int enable = GET32(AUX_ENB);
    PUT32(AUX_ENB, enable | (1 << 0));

    // Memory barrier between AUX operations and UART operations.
    dev_barrier();

    // Disable transmitter and receiver during configuration
    // (BCM2835 Data Sheet p17).
    PUT32(AUX_MU_CNTL, 0b00000000);

    // Disable interrupts (BCM2835 Data Sheet p12).
    PUT32(AUX_MU_IER,  0b00000000);

    // Clear transmit and receive FIFOs (BCM2835 Data Sheet p13).
    PUT32(AUX_MU_IIR,  0b00000110);

    // Set DLAB=0, data size=8-bit mode (BCM2835 Data Sheet/Errata p14).
    PUT32(AUX_MU_LCR,  0b00000011);

    // Set Baud register (BCM2835 Data Sheet p19).
    PUT32(AUX_MU_BAUD, BAUD_RATE);

    // Enable transmitter and receiver after configuration
    // (BCM2835 Data Sheet p17).
    PUT32(AUX_MU_CNTL, 0b00000011);

    // Memory barrier after UART operations.
    dev_barrier();
}

int uart_getc(void) {
    // Poll until "Symbol available" bit is set (BCM2835 Data Sheet p19).
    unsigned int status = 0;
    while (!(status & (1 << 0))) {
        status = GET32(AUX_MU_STAT);
    }

    // Bits 0-7 of AUX_MU_IO are receive data (BCM2835 Data Sheet p11).
    unsigned int data = GET32(AUX_MU_IO) & 0b11111111;
	return data;
}

void uart_putc(unsigned c) {
    // Poll until "Space available" bit is set (BCM2835 Data Sheet p19).
    unsigned int status = 0;
    while (!(status & (1 << 1))) {
        status = GET32(AUX_MU_STAT);
    }

    // Bits 8-15 of AUX_MU_IO are transmit data (BCM2835 Data Sheet p11).
    unsigned int data = c & 0b11111111;
    PUT32(AUX_MU_IO, data);
}
