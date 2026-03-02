cmake_minimum_required(VERSION 3.28)
project(sexp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

add_library(sexp_core STATIC
    src/memory/arena.c
    src/memory/intern.c
    src/core/node.c
    src/core/tree.c
    src/parse/tokenizer.c
    src/parse/parse_stack.c
    src/parse/parser.c
    src/serialize/serializer.c
    src/internal/mutate.c
    src/internal/clone.c
)

target_include_directories(sexp_core PUBLIC src)
target_compile_options(sexp_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(sexp_core PUBLIC Threads::Threads)

enable_testing()
add_subdirectory(tests/c)