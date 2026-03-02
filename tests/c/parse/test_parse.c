#include "unity.h"

#include "memory/intern.h"
#include "core/tree.h"

void setUp(void) {
    intern_init();
    intern_retain();
}

void tearDown(void) {
    intern_release();
}

static void test_parse_empty_input(void) {
    SExp tree = sexp_parse("", 0);
    TEST_ASSERT_EQUAL_UINT(0, tree.count);
    TEST_ASSERT_NULL(tree.nodes);
}

static void test_parse_single_atom(void) {
    SExp   tree   = sexp_parse("foo", 3);
    AtomId id     = sexp_atom(&tree, 0);
    size_t length = 0;
    TEST_ASSERT_EQUAL_UINT(1, tree.count);
    TEST_ASSERT_EQUAL_INT(NODE_ATOM, sexp_kind(&tree, 0));
    TEST_ASSERT_NOT_EQUAL(0, id);
    const char *string = intern_lookup(id, &length);
    TEST_ASSERT_EQUAL_STRING("foo", string);
    sexp_free(&tree);
}

static void test_parse_empty_list(void) {
    SExp tree = sexp_parse("()", 2);
    TEST_ASSERT_EQUAL_UINT(1, tree.count);
    TEST_ASSERT_EQUAL_INT(NODE_LIST, sexp_kind(&tree, 0));
    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_first_child(&tree, 0));
    sexp_free(&tree);
}

static void test_parse_flat_list(void) {
    SExp tree = sexp_parse("(a b c)", 7);
    TEST_ASSERT_EQUAL_UINT(4, tree.count);
    TEST_ASSERT_EQUAL_INT(NODE_LIST, sexp_kind(&tree, 0));

    uint32_t a = sexp_first_child(&tree, 0);
    TEST_ASSERT_NOT_EQUAL(SEXP_NULL_INDEX, a);
    TEST_ASSERT_EQUAL_UINT(0, sexp_parent(&tree, a));

    uint32_t b = sexp_next_sibling(&tree, a);
    TEST_ASSERT_NOT_EQUAL(SEXP_NULL_INDEX, b);
    TEST_ASSERT_EQUAL_UINT(0, sexp_parent(&tree, b));

    uint32_t c = sexp_next_sibling(&tree, b);
    TEST_ASSERT_NOT_EQUAL(SEXP_NULL_INDEX, c);
    TEST_ASSERT_EQUAL_UINT(0, sexp_parent(&tree, c));

    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_next_sibling(&tree, c));
    sexp_free(&tree);
}

static void test_parse_nested_list(void) {
    SExp tree = sexp_parse("(a (b c) d)", 11);
    TEST_ASSERT_EQUAL_UINT(6, tree.count);

    uint32_t root  = 0;
    uint32_t a     = sexp_first_child(&tree, root);
    uint32_t inner = sexp_next_sibling(&tree, a);
    TEST_ASSERT_EQUAL_INT(NODE_ATOM, sexp_kind(&tree, a));
    TEST_ASSERT_EQUAL_INT(NODE_LIST, sexp_kind(&tree, inner));
    TEST_ASSERT_EQUAL_UINT(root, sexp_parent(&tree, inner));

    uint32_t b = sexp_first_child(&tree, inner);
    TEST_ASSERT_EQUAL_INT(NODE_ATOM, sexp_kind(&tree, b));
    TEST_ASSERT_EQUAL_UINT(inner, sexp_parent(&tree, b));

    uint32_t c = sexp_next_sibling(&tree, b);
    TEST_ASSERT_EQUAL_INT(NODE_ATOM, sexp_kind(&tree, c));
    TEST_ASSERT_EQUAL_UINT(inner, sexp_parent(&tree, c));
    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_next_sibling(&tree, c));

    uint32_t d = sexp_next_sibling(&tree, inner);
    TEST_ASSERT_EQUAL_INT(NODE_ATOM, sexp_kind(&tree, d));
    TEST_ASSERT_EQUAL_UINT(root, sexp_parent(&tree, d));
    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_next_sibling(&tree, d));

    sexp_free(&tree);
}

static void test_parse_unclosed_paren(void) {
    SExp tree = sexp_parse("(a b", 4);
    TEST_ASSERT_EQUAL_UINT(0, tree.count);
    TEST_ASSERT_NULL(tree.nodes);
}

static void test_parse_stray_close_paren(void) {
    SExp tree = sexp_parse(")", 1);
    TEST_ASSERT_EQUAL_UINT(0, tree.count);
    TEST_ASSERT_NULL(tree.nodes);
}

void run_parse_tests(void) {
    RUN_TEST(test_parse_empty_input);
    RUN_TEST(test_parse_single_atom);
    RUN_TEST(test_parse_empty_list);
    RUN_TEST(test_parse_flat_list);
    RUN_TEST(test_parse_nested_list);
    RUN_TEST(test_parse_unclosed_paren);
    RUN_TEST(test_parse_stray_close_paren);
}

int main(void) {
    UNITY_BEGIN();
    run_parse_tests();
    return UNITY_END();
}
