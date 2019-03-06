cmake_minimum_required(VERSION 3.13)

set(SRC shell.c shell-support.c)

add_executable(simple-pi-shell ${SRC})
target_link_libraries(simple-pi-shell m)
target_link_libraries(simple-pi-shell simple-pi-support)
target_include_directories(simple-pi-shell PUBLIC ..)