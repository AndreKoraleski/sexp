add_executable(test_sexp
    placeholder_test.c
)

target_link_libraries(test_sexp sexp_core)

add_test(NAME sexp_tests COMMAND test_sexp)