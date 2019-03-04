cmake_minimum_required(VERSION 3.13)
project(simple_pi_os C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(pi-side)
add_subdirectory(unix-side)

add_custom_target(run
        COMMAND $<TARGET_FILE_NAME:simple-pi-install> pi-side/kernel/kernel.bin
        DEPENDS simple-pi-install kernel.bin)
