cmake_minimum_required(VERSION 3.10)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED fuse)
add_definitions(${FUSE_CFLAGS})

set(SRC echo-until.c
        fs.c
        fs-support.c
        redirect.c)

add_executable(simple-pi-fs ${SRC})
target_link_libraries(simple-pi-fs m)
target_link_libraries(simple-pi-fs ${FUSE_LIBRARIES})
target_link_libraries(simple-pi-fs simple-pi-support)
target_include_directories(simple-pi-fs PUBLIC ..)