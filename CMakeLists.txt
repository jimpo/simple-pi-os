cmake_minimum_required(VERSION 3.13)
project(simple_pi_os C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(pi-side)
add_subdirectory(unix-side)

set(CMAKE_VERBOSE_MAKEFILE on)

add_custom_target(run
        COMMAND $<TARGET_FILE:simple-pi-install> -exec $<TARGET_FILE:simple-pi-shell> pi-side/kernel/kernel.bin
        DEPENDS simple-pi-install simple-pi-shell kernel.bin)
