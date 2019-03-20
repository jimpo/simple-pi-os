cmake_minimum_required(VERSION 3.10)

set(SRC hello.c)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Ttext 0x000100000")

add_executable(hello.elf ${SRC})
target_link_libraries(hello.elf pi)
target_include_directories(hello.elf PUBLIC ../..)

add_custom_target(hello.list
        COMMAND ${ARM_PREFIX}-objdump -D $<TARGET_FILE_NAME:hello.elf> > hello.list
        DEPENDS hello.elf
        BYPRODUCTS hello.list)