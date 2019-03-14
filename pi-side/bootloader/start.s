.section ".text.boot"

.globl _start
_start:
    b skip
.space 0x200000-0x8004,0
skip:
    mov sp,#0x8000
    bl notmain
hang: b rpi_reboot
