#include <stdlib.h>

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

static void test_clone_node_atom(void) {
    SExp        src    = sexp_parse("foo", 3);
    SExp        clone  = sexp_clone_node(&src, 0);
    size_t      length = 0;
    const char *string;
    TEST_ASSERT_TRUE(clone.valid);
    TEST_ASSERT_EQUAL_UINT(1, clone.count);
    TEST_ASSERT_EQUAL_INT(NODE_ATOM, sexp_kind(&clone, 0));
    string = intern_lookup(sexp_atom(&clone, 0), &length);
    TEST_ASSERT_EQUAL_STRING("foo", string);
    sexp_free(&src);
    sexp_free(&clone);
}

static void test_clone_node_subtree(void) {
    SExp   src     = sexp_parse("(a (b c) d)", 11);
    SExp   clone   = sexp_clone_node(&src, 0);
    size_t src_len = 0, clone_len = 0;
    TEST_ASSERT_TRUE(clone.valid);
    TEST_ASSERT_EQUAL_UINT(src.count, clone.count);
    char *src_s   = sexp_serialize(&src, &src_len);
    char *clone_s = sexp_serialize(&clone, &clone_len);
    TEST_ASSERT_EQUAL_STRING(src_s, clone_s);
    free(src_s);
    free(clone_s);
    sexp_free(&src);
    sexp_free(&clone);
}

static void test_clone_leaves_source_intact(void) {
    SExp     src    = sexp_parse("(a (b c) d)", 11);
    SExp     clone  = sexp_clone_node(&src, 0);
    uint32_t a_c    = sexp_first_child(&clone, 0);
    uint32_t a_s    = sexp_first_child(&src, 0);
    size_t   length = 0;
    /* Mutate the clone's first atom, source must be unchanged. */
    sexp_set_atom(&clone, a_c, "X", 1);
    const char *string = intern_lookup(sexp_atom(&src, a_s), &length);
    TEST_ASSERT_EQUAL_STRING("a", string);
    sexp_free(&src);
    sexp_free(&clone);
}

static void test_extract_node(void) {
    SExp     src    = sexp_parse("(a (b c) d)", 11);
    uint32_t inner  = sexp_next_sibling(&src, sexp_first_child(&src, 0));
    SExp     ext    = sexp_extract_node(&src, inner);
    size_t   length = 0;
    TEST_ASSERT_TRUE(ext.valid);
    char *s = sexp_serialize(&ext, &length);
    TEST_ASSERT_EQUAL_STRING("(b c)", s);
    free(s);
    s = sexp_serialize(&src, &length);
    TEST_ASSERT_EQUAL_STRING("(a d)", s);
    free(s);
    sexp_free(&src);
    sexp_free(&ext);
}

static void test_extract_node_clone_fails_leaves_source_intact(void) {
    /* Extracting an out-of-bounds index must be a no-op. */
    SExp src = sexp_parse("(a b)", 5);
    SExp ext = sexp_extract_node(&src, 999);
    TEST_ASSERT_FALSE(ext.valid);
    TEST_ASSERT_EQUAL_UINT(3, src.count);
    sexp_free(&src);
}

void run_clone_tests(void) {
    RUN_TEST(test_clone_node_atom);
    RUN_TEST(test_clone_node_subtree);
    RUN_TEST(test_clone_leaves_source_intact);
    RUN_TEST(test_extract_node);
    RUN_TEST(test_extract_node_clone_fails_leaves_source_intact);
}

int main(void) {
    UNITY_BEGIN();
    run_clone_tests();
    return UNITY_END();
}
