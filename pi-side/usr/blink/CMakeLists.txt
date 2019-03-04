cmake_minimum_required(VERSION 3.13)

set(LINKER_SCRIPT memmap)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_CURRENT_SOURCE_DIR}/${LINKER_SCRIPT}")

set(SRC blink.c)

add_executable(blink.elf ${SRC})
target_link_libraries(blink.elf pi)
target_include_directories(blink.elf PUBLIC ../..)

# https://stackoverflow.com/a/42138375
set_target_properties(blink.elf PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${LINKER_SCRIPT})

add_custom_target(blink.list
        COMMAND ${ARM_PREFIX}-objdump -D $<TARGET_FILE_NAME:blink.elf> > blink.list
        DEPENDS blink.elf
        BYPRODUCTS blink.list)

add_custom_target(blink.bin
        COMMAND ${ARM_PREFIX}-objcopy -D $<TARGET_FILE_NAME:blink.elf> -O binary blink.bin
        DEPENDS blink.elf
        BYPRODUCTS blink.bin)
