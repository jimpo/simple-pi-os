cmake_minimum_required(VERSION 3.13)

set(SRC install.c tty.c)

add_executable(simple-pi-install ${SRC})
target_link_libraries(simple-pi-install m)
target_link_libraries(simple-pi-install simple-pi-support)
target_include_directories(simple-pi-install PUBLIC ..)
