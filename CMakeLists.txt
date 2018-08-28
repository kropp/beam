cmake_minimum_required(VERSION 3.12)
project(beam C)

set(CMAKE_C_STANDARD 11)

find_package(X11 REQUIRED)

add_executable(beam main.c)

target_link_libraries(beam X11)
