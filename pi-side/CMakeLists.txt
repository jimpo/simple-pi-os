cmake_minimum_required(VERSION 3.13)
project(simple_pi_os C ASM)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(ARM_PREFIX /usr/bin/arm-none-eabi)
set(CMAKE_C_COMPILER ${ARM_PREFIX}-gcc)
set(CMAKE_ASM_COMPILER ${ARM_PREFIX}-as)

set(CMAKE_C_FLAGS
        "-Wall \
        -nostdlib \
        -nostartfiles \
        -ffreestanding \
        -march=armv6")
set(CMAKE_ASM_FLAGS
        "--warn \
        --fatal-warnings \
        -mcpu=arm1176jzf-s \
        -march=armv6zk")

add_subdirectory(bootloader)
add_subdirectory(kernel)
add_subdirectory(libpi)
add_subdirectory(usr)
