cmake_minimum_required(VERSION 3.13)
project(simple_pi_control C)

set(CMAKE_C_FLAGS "-Wall -Werror -g")

set(SRC my-install.c simple-boot.c support.c trace.c tty.c)

add_executable(simple-pi-install ${SRC})
target_link_libraries(simple-pi-install m)