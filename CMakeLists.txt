cmake_minimum_required(VERSION 3.28)
project(sexp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(sexp_core STATIC
    src/placeholder.c
)

target_include_directories(sexp_core PUBLIC src/)

enable_testing()
add_subdirectory(tests/c)