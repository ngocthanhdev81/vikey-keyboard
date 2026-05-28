cmake_minimum_required(VERSION 3.22)

project(florisboard)

add_library(fl_native SHARED src/lib_placeholder.c)

target_link_libraries(fl_native android log)
