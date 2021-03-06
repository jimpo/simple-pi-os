cmake_minimum_required(VERSION 3.10)

option(USE_PIC "Enable position independent code" NO)

if(USE_PIC)
    set(CMAKE_C_FLAGS
            "${CMAKE_C_FLAGS}
            -frecord-gcc-switches \
            -gdwarf-2 \
            -fdata-sections -ffunction-sections \
            -Wl,--warn-common \
            -Wl,--gc-sections \
            -Wl,--emit-relocs \
            -fPIC \
            -msingle-pic-base \
            -mpic-register=r9 \
            -mno-pic-data-is-text-relative")
endif()

set(SRC asm.s
        cache.c
        crc32.c
        cstart.c
        gpio.c
        kmalloc.c
        math.c
        mem-barrier.s
        printf.c
        put-get.s
        rand.c
        reboot.c
        stdlib.c
        syscall.s
        timer.c
        uart.c)

add_library(pi STATIC ${SRC})