add_executable(test_sexp
    main.c
    test_arena.c
    test_intern.c
    test_tree.c
    ../unity/unity.c
)

target_include_directories(test_sexp PRIVATE
    ../unity
    ../../src/include
)


target_link_libraries(test_sexp sexp_core)
target_compile_options(test_sexp PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME sexp_tests COMMAND test_sexp)