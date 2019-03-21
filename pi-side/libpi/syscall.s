.globl rpi_sbrk
rpi_sbrk:
    swi 0x01
    bx lr
