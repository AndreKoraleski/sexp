cmake_minimum_required(VERSION 3.28)
project(sexp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(sexp_core STATIC
    src/arena.c
)

target_include_directories(sexp_core PUBLIC src/include)
target_compile_options(sexp_core PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_subdirectory(tests/c)