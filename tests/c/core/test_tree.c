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

static void test_accessor_out_of_bounds(void) {
    SExp tree = sexp_parse("(a)", 3);
    TEST_ASSERT_EQUAL_INT(NODE_INVALID, sexp_kind(&tree, 999));
    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_first_child(&tree, 999));
    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_next_sibling(&tree, 999));
    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_parent(&tree, 999));
    TEST_ASSERT_EQUAL_UINT(0, sexp_atom(&tree, 999));
    sexp_free(&tree);
}

static void test_accessor_atom_on_list(void) {
    SExp tree = sexp_parse("(a)", 3);
    TEST_ASSERT_EQUAL_UINT(0, sexp_atom(&tree, 0));
    sexp_free(&tree);
}

static void test_set_atom(void) {
    SExp        tree   = sexp_parse("foo", 3);
    size_t      length = 0;
    const char *string;
    sexp_set_atom(&tree, 0, "bar", 3);
    AtomId id = sexp_atom(&tree, 0);
    string    = intern_lookup(id, &length);
    TEST_ASSERT_EQUAL_STRING("bar", string);
    sexp_free(&tree);
}

static void test_set_atom_noop_on_list(void) {
    SExp   tree   = sexp_parse("(a)", 3);
    AtomId before = sexp_atom(&tree, 0);
    sexp_set_atom(&tree, 0, "x", 1);
    TEST_ASSERT_EQUAL_UINT(before, sexp_atom(&tree, 0));
    sexp_free(&tree);
}

void run_tree_tests(void) {
    RUN_TEST(test_accessor_out_of_bounds);
    RUN_TEST(test_accessor_atom_on_list);
    RUN_TEST(test_set_atom);
    RUN_TEST(test_set_atom_noop_on_list);
}

int main(void) {
    UNITY_BEGIN();
    run_tree_tests();
    return UNITY_END();
}
