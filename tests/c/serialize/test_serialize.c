#include <stdlib.h>
#include <string.h>

#include "unity.h"

#include "memory/intern.h"
#include "sexp.h"

void setUp(void) {
    intern_init();
    intern_retain();
}

void tearDown(void) {
    intern_release();
}

static void test_serialize_single_atom(void) {
    SExp   tree   = sexp_parse("foo", 3);
    size_t length = 0;
    char  *out    = sexp_serialize(&tree, &length);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(3, length);
    TEST_ASSERT_EQUAL_STRING("foo", out);
    free(out);
    sexp_free(&tree);
}

static void test_serialize_flat_list(void) {
    SExp   tree   = sexp_parse("(a b c)", 7);
    size_t length = 0;
    char  *out    = sexp_serialize(&tree, &length);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("(a b c)", out);
    free(out);
    sexp_free(&tree);
}

static void test_serialize_nested_list(void) {
    SExp   tree   = sexp_parse("(a (b c) d)", 11);
    size_t length = 0;
    char  *out    = sexp_serialize(&tree, &length);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("(a (b c) d)", out);
    free(out);
    sexp_free(&tree);
}

static void test_serialize_len_matches_strlen(void) {
    SExp   tree   = sexp_parse("(a b c)", 7);
    size_t length = 0;
    char  *out    = sexp_serialize(&tree, &length);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(strlen(out), length);
    free(out);
    sexp_free(&tree);
}

static void test_serialize_empty_tree(void) {
    SExp        tree   = {0};
    size_t      length = 0;
    const char *out    = sexp_serialize(&tree, &length);
    TEST_ASSERT_NULL(out);
}

static void test_serialize_roundtrip(void) {
    const char *input  = "(player (pos 1 2) (vel 3 4))";
    SExp        tree   = sexp_parse(input, strlen(input));
    size_t      length = 0;
    char       *out    = sexp_serialize(&tree, &length);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING(input, out);
    free(out);
    sexp_free(&tree);
}

void run_serialize_tests(void) {
    RUN_TEST(test_serialize_single_atom);
    RUN_TEST(test_serialize_flat_list);
    RUN_TEST(test_serialize_nested_list);
    RUN_TEST(test_serialize_len_matches_strlen);
    RUN_TEST(test_serialize_empty_tree);
    RUN_TEST(test_serialize_roundtrip);
}

int main(void) {
    UNITY_BEGIN();
    run_serialize_tests();
    return UNITY_END();
}
