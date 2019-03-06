cmake_minimum_required(VERSION 3.13)

set(SRC simple-boot.c support.c trace.c)

add_library(simple-pi-support STATIC ${SRC})
