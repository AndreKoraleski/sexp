#include <string.h>
#include "unity.h"
#include "intern.h"
#include "tree.h"

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
    SExp tree = sexp_parse("foo", 3);
    TEST_ASSERT_EQUAL_UINT(1, tree.count);
    TEST_ASSERT_EQUAL_INT(NODE_ATOM, sexp_kind(&tree, 0));
    AtomId id = sexp_atom(&tree, 0);
    TEST_ASSERT_NOT_EQUAL(0, id);
    size_t len = 0;
    const char *str = intern_lookup(id, &len);
    TEST_ASSERT_EQUAL_STRING("foo", str);
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

    uint32_t root = 0;
    uint32_t a    = sexp_first_child(&tree, root);
    TEST_ASSERT_EQUAL_INT(NODE_ATOM, sexp_kind(&tree, a));

    uint32_t inner = sexp_next_sibling(&tree, a);
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
    SExp tree = sexp_parse("foo", 3);
    sexp_set_atom(&tree, 0, "bar", 3);
    AtomId id = sexp_atom(&tree, 0);
    size_t len = 0;
    const char *str = intern_lookup(id, &len);
    TEST_ASSERT_EQUAL_STRING("bar", str);
    sexp_free(&tree);
}

static void test_set_atom_noop_on_list(void) {
    SExp tree = sexp_parse("(a)", 3);
    AtomId before = sexp_atom(&tree, 0);
    sexp_set_atom(&tree, 0, "x", 1);
    TEST_ASSERT_EQUAL_UINT(before, sexp_atom(&tree, 0));
    sexp_free(&tree);
}

static void test_insert_as_first_child(void) {
    SExp tree = sexp_parse("(a)", 3);
    TEST_ASSERT_EQUAL_UINT(2, tree.count);
    uint32_t old_child = sexp_first_child(&tree, 0);

    uint32_t z = sexp_node_alloc(&tree, NODE_ATOM);
    TEST_ASSERT_NOT_EQUAL(SEXP_NULL_INDEX, z);
    sexp_set_atom(&tree, z, "z", 1);
    sexp_insert(&tree, 0, SEXP_NULL_INDEX, z);

    TEST_ASSERT_EQUAL_UINT(3, tree.count);
    TEST_ASSERT_EQUAL_UINT(z, sexp_first_child(&tree, 0));
    TEST_ASSERT_EQUAL_UINT(old_child, sexp_next_sibling(&tree, z));
    TEST_ASSERT_EQUAL_UINT(0, sexp_parent(&tree, z));

    size_t len = 0;
    const char *str = intern_lookup(sexp_atom(&tree, z), &len);
    TEST_ASSERT_EQUAL_STRING("z", str);
    sexp_free(&tree);
}

static void test_remove_middle_child(void) {
    SExp tree = sexp_parse("(a b c)", 7);
    uint32_t root = 0;
    uint32_t a    = sexp_first_child(&tree, root);
    uint32_t b    = sexp_next_sibling(&tree, a);

    sexp_remove(&tree, b);

    TEST_ASSERT_EQUAL_UINT(3, tree.count);
    uint32_t a_next = sexp_next_sibling(&tree, a);
    TEST_ASSERT_NOT_EQUAL(SEXP_NULL_INDEX, a_next);

    size_t len = 0;
    const char *str = intern_lookup(sexp_atom(&tree, a_next), &len);
    TEST_ASSERT_EQUAL_STRING("c", str);

    sexp_free(&tree);
}

static void test_remove_last_node(void) {
    SExp tree = sexp_parse("(a b)", 5);
    uint32_t root = 0;
    uint32_t a    = sexp_first_child(&tree, root);
    uint32_t b    = sexp_next_sibling(&tree, a);

    uint32_t before_count = tree.count;
    sexp_remove(&tree, b);
    TEST_ASSERT_EQUAL_UINT(before_count - 1, tree.count);
    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_next_sibling(&tree, a));

    sexp_free(&tree);
}

static void test_serialize_single_atom(void) {
    SExp tree = sexp_parse("foo", 3);
    size_t len = 0;
    const char *out = sexp_serialize(&tree, &len);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(3, len);
    TEST_ASSERT_EQUAL_STRING("foo", out);
    sexp_free(&tree);
}

static void test_serialize_flat_list(void) {
    SExp tree = sexp_parse("(a b c)", 7);
    size_t len = 0;
    const char *out = sexp_serialize(&tree, &len);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("(a b c)", out);
    sexp_free(&tree);
}

static void test_serialize_nested_list(void) {
    SExp tree = sexp_parse("(a (b c) d)", 11);
    size_t len = 0;
    const char *out = sexp_serialize(&tree, &len);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("(a (b c) d)", out);
    sexp_free(&tree);
}

static void test_serialize_len_matches_strlen(void) {
    SExp tree = sexp_parse("(a b c)", 7);
    size_t len = 0;
    const char *out = sexp_serialize(&tree, &len);
    TEST_ASSERT_EQUAL_UINT(strlen(out), len);
    sexp_free(&tree);
}

static void test_serialize_empty_tree(void) {
    SExp tree = {0};
    size_t len = 0;
    const char *out = sexp_serialize(&tree, &len);
    TEST_ASSERT_NULL(out);
}

static void test_serialize_roundtrip(void) {
    const char *input = "(player (pos 1 2) (vel 3 4))";
    SExp tree = sexp_parse(input, strlen(input));
    size_t len = 0;
    const char *out = sexp_serialize(&tree, &len);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING(input, out);
    sexp_free(&tree);
}

void run_tree_tests(void) {
    RUN_TEST(test_parse_empty_input);
    RUN_TEST(test_parse_single_atom);
    RUN_TEST(test_parse_empty_list);
    RUN_TEST(test_parse_flat_list);
    RUN_TEST(test_parse_nested_list);
    RUN_TEST(test_parse_unclosed_paren);
    RUN_TEST(test_parse_stray_close_paren);
    RUN_TEST(test_accessor_out_of_bounds);
    RUN_TEST(test_accessor_atom_on_list);
    RUN_TEST(test_set_atom);
    RUN_TEST(test_set_atom_noop_on_list);
    RUN_TEST(test_insert_as_first_child);
    RUN_TEST(test_remove_middle_child);
    RUN_TEST(test_remove_last_node);
    RUN_TEST(test_serialize_single_atom);
    RUN_TEST(test_serialize_flat_list);
    RUN_TEST(test_serialize_nested_list);
    RUN_TEST(test_serialize_len_matches_strlen);
    RUN_TEST(test_serialize_empty_tree);
    RUN_TEST(test_serialize_roundtrip);
}

int main(void) {
    UNITY_BEGIN();
    run_tree_tests();
    return UNITY_END();
}