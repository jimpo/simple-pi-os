cmake_minimum_required(VERSION 3.13)
project(simple_pi_os C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(pi-side)
add_subdirectory(unix-side)

set(CMAKE_VERBOSE_MAKEFILE ON)

add_custom_target(run
        COMMAND $<TARGET_FILE_NAME:simple-pi-install> pi-side/usr/blink/blink.bin
        DEPENDS simple-pi-install blink.bin)
