cmake_minimum_required(VERSION 3.13)

set(LINKER_SCRIPT memmap)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_CURRENT_SOURCE_DIR}/${LINKER_SCRIPT}")

set(SRC start.s bootloader.c)

add_executable(bootloader.elf ${SRC})
target_link_libraries(bootloader.elf pi)

# https://stackoverflow.com/a/42138375
set_target_properties(bootloader.elf PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${LINKER_SCRIPT})

add_custom_target(bootloader.list
        COMMAND ${ARM_PREFIX}-objdump -D $<TARGET_FILE_NAME:bootloader.elf> > bootloader.list
        DEPENDS bootloader.elf
        BYPRODUCTS bootloader.list)

add_custom_target(bootloader.bin
        COMMAND ${ARM_PREFIX}-objcopy -D $<TARGET_FILE_NAME:bootloader.elf> -O binary bootloader.bin
        DEPENDS bootloader.elf
        BYPRODUCTS bootloader.bin)
