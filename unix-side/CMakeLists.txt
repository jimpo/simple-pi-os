cmake_minimum_required(VERSION 3.13)
project(simple_pi_control C)

set(CMAKE_C_FLAGS "-Wall -Werror")

add_subdirectory(fs)
add_subdirectory(shell)
add_subdirectory(support)
add_subdirectory(install)
