cmake_minimum_required(VERSION 3.13)

set(LINKER_SCRIPT memmap)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_CURRENT_SOURCE_DIR}/${LINKER_SCRIPT}")

set(SRC shell.c
        disk/fat32-helpers.c
        disk/fs.c
        disk/platform.c
        disk/sd.c
        disk/unicode-utf8.c
        loader.c
        rpi-thread.c
        start.s
        thread-asm.s)

add_executable(kernel.elf ${SRC})
target_link_libraries(kernel.elf pi)
target_include_directories(kernel.elf PUBLIC ..)

# https://stackoverflow.com/a/42138375
set_target_properties(kernel.elf PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${LINKER_SCRIPT})

add_custom_target(kernel.list
        COMMAND ${ARM_PREFIX}-objdump -D $<TARGET_FILE_NAME:kernel.elf> > kernel.list
        DEPENDS kernel.elf
        BYPRODUCTS kernel.list)

add_custom_target(kernel.bin
        COMMAND ${ARM_PREFIX}-objcopy -D $<TARGET_FILE_NAME:kernel.elf> -O binary kernel.bin
        DEPENDS kernel.elf
        BYPRODUCTS kernel.bin)