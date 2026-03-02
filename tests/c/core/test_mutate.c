#include <stdio.h>
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

static void test_insert_as_first_child(void) {
    SExp     tree      = sexp_parse("(a)", 3);
    uint32_t old_child = sexp_first_child(&tree, 0);
    TEST_ASSERT_EQUAL_UINT(2, tree.count);

    uint32_t z = sexp_allocate_node(&tree, NODE_ATOM);
    TEST_ASSERT_NOT_EQUAL(SEXP_NULL_INDEX, z);
    sexp_set_atom(&tree, z, "z", 1);
    sexp_insert(&tree, 0, SEXP_NULL_INDEX, z);

    TEST_ASSERT_EQUAL_UINT(3, tree.count);
    TEST_ASSERT_EQUAL_UINT(z, sexp_first_child(&tree, 0));
    TEST_ASSERT_EQUAL_UINT(old_child, sexp_next_sibling(&tree, z));
    TEST_ASSERT_EQUAL_UINT(0, sexp_parent(&tree, z));

    size_t      length = 0;
    const char *string = intern_lookup(sexp_atom(&tree, z), &length);
    TEST_ASSERT_EQUAL_STRING("z", string);
    sexp_free(&tree);
}

static void test_remove_middle_child(void) {
    SExp     tree = sexp_parse("(a b c)", 7);
    uint32_t root = 0;
    uint32_t a    = sexp_first_child(&tree, root);
    uint32_t b    = sexp_next_sibling(&tree, a);

    sexp_remove(&tree, b);

    TEST_ASSERT_EQUAL_UINT(3, tree.count);
    uint32_t a_next = sexp_next_sibling(&tree, a);
    TEST_ASSERT_NOT_EQUAL(SEXP_NULL_INDEX, a_next);
    size_t      length = 0;
    const char *string = intern_lookup(sexp_atom(&tree, a_next), &length);
    TEST_ASSERT_EQUAL_STRING("c", string);

    sexp_free(&tree);
}

static void test_remove_last_node(void) {
    SExp     tree         = sexp_parse("(a b)", 5);
    uint32_t root         = 0;
    uint32_t a            = sexp_first_child(&tree, root);
    uint32_t b            = sexp_next_sibling(&tree, a);
    uint32_t before_count = tree.count;

    sexp_remove(&tree, b);
    TEST_ASSERT_EQUAL_UINT(before_count - 1, tree.count);
    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_next_sibling(&tree, a));

    sexp_free(&tree);
}

static void test_insert_into_atom_is_noop(void) {
    SExp     tree = sexp_parse("(a)", 3);
    uint32_t root = 0;
    uint32_t a    = sexp_first_child(&tree, root);
    uint32_t n    = sexp_allocate_node(&tree, NODE_ATOM);
    sexp_set_atom(&tree, n, "z", 1);
    /* Inserting under an atom node must be a no-op. */
    sexp_insert(&tree, a, SEXP_NULL_INDEX, n);
    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_first_child(&tree, a));
    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_parent(&tree, n));
    sexp_free(&tree);
}

static void test_insert_after_wrong_parent_is_noop(void) {
    SExp     tree   = sexp_parse("((a) (b))", 9);
    uint32_t root   = 0;
    uint32_t inner1 = sexp_first_child(&tree, root);
    uint32_t inner2 = sexp_next_sibling(&tree, inner1);
    uint32_t a      = sexp_first_child(&tree, inner1);
    uint32_t b      = sexp_first_child(&tree, inner2);
    uint32_t n      = sexp_allocate_node(&tree, NODE_ATOM);
    sexp_set_atom(&tree, n, "z", 1);
    /* after=b belongs to inner2, not inner1 — must be a no-op. */
    sexp_insert(&tree, inner1, b, n);
    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_parent(&tree, n));
    TEST_ASSERT_EQUAL_UINT(a, sexp_first_child(&tree, inner1));
    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_next_sibling(&tree, a));
    (void)b;
    sexp_free(&tree);
}

static void test_insert_auto_detaches_child(void) {
    /* Move atom a from inner1 into inner2 using a single sexp_insert call. */
    SExp     tree   = sexp_parse("((a) (b))", 9);
    uint32_t root   = 0;
    uint32_t inner1 = sexp_first_child(&tree, root);
    uint32_t inner2 = sexp_next_sibling(&tree, inner1);
    uint32_t a      = sexp_first_child(&tree, inner1);
    sexp_insert(&tree, inner2, SEXP_NULL_INDEX, a);
    /* inner1 is now empty. */
    TEST_ASSERT_EQUAL_UINT(SEXP_NULL_INDEX, sexp_first_child(&tree, inner1));
    /* inner2 has a as first child. */
    TEST_ASSERT_EQUAL_UINT(a, sexp_first_child(&tree, inner2));
    TEST_ASSERT_EQUAL_UINT(inner2, sexp_parent(&tree, a));
    sexp_free(&tree);
}

static void test_stress_insert_many(void) {
#define INSERT_N 200
    SExp tree = sexp_parse("()", 2);
    TEST_ASSERT_NOT_NULL(tree.nodes);

    char     buf[16];
    uint32_t prev = SEXP_NULL_INDEX;
    for (int i = 0; i < INSERT_N; i++) {
        snprintf(buf, sizeof(buf), "n%d", i);
        uint32_t n = sexp_allocate_node(&tree, NODE_ATOM);
        TEST_ASSERT_NOT_EQUAL(SEXP_NULL_INDEX, n);
        sexp_set_atom(&tree, n, buf, strlen(buf));
        sexp_insert(&tree, 0, prev, n);
        prev = n;
    }

    TEST_ASSERT_EQUAL_UINT(INSERT_N + 1, tree.count);

    uint32_t c    = sexp_first_child(&tree, 0);
    int      seen = 0;
    while (c != SEXP_NULL_INDEX) {
        TEST_ASSERT_EQUAL_INT(NODE_ATOM, sexp_kind(&tree, c));
        TEST_ASSERT_NOT_EQUAL(0, sexp_atom(&tree, c));
        seen++;
        c = sexp_next_sibling(&tree, c);
    }
    TEST_ASSERT_EQUAL_INT(INSERT_N, seen);
    sexp_free(&tree);
#undef INSERT_N
}

static void test_stress_remove_sequential(void) {
#define REMOVE_N 100
    char input[REMOVE_N * 5 + 4];
    int  pos     = 0;
    input[pos++] = '(';
    for (int i = 0; i < REMOVE_N; i++) {
        if (i > 0)
            input[pos++] = ' ';
        pos += snprintf(input + pos, sizeof(input) - (size_t)pos, "a%d", i);
    }
    input[pos++] = ')';
    input[pos]   = '\0';

    SExp tree = sexp_parse(input, (size_t)pos);
    TEST_ASSERT_NOT_NULL(tree.nodes);
    TEST_ASSERT_EQUAL_UINT(REMOVE_N + 1, tree.count);

    for (int remaining = REMOVE_N; remaining > 0; remaining--) {
        uint32_t target = sexp_first_child(&tree, 0);
        for (int step = 0; step < remaining / 2; step++)
            target = sexp_next_sibling(&tree, target);
        TEST_ASSERT_NOT_EQUAL(SEXP_NULL_INDEX, target);

        sexp_remove(&tree, target);
        TEST_ASSERT_EQUAL_UINT((uint32_t)remaining, tree.count);

        uint32_t c    = sexp_first_child(&tree, 0);
        int      seen = 0;
        while (c != SEXP_NULL_INDEX) {
            TEST_ASSERT_EQUAL_INT(NODE_ATOM, sexp_kind(&tree, c));
            seen++;
            c = sexp_next_sibling(&tree, c);
        }
        TEST_ASSERT_EQUAL_INT(remaining - 1, seen);
    }

    sexp_free(&tree);
#undef REMOVE_N
}

static void test_stress_interleaved_insert_remove(void) {
#define INTERLEAVE_N 50
    char input[INTERLEAVE_N * 5 + 4];
    int  pos     = 0;
    input[pos++] = '(';
    for (int i = 0; i < INTERLEAVE_N; i++) {
        if (i > 0)
            input[pos++] = ' ';
        pos += snprintf(input + pos, sizeof(input) - (size_t)pos, "x%d", i);
    }
    input[pos++] = ')';
    input[pos]   = '\0';

    SExp tree = sexp_parse(input, (size_t)pos);
    TEST_ASSERT_NOT_NULL(tree.nodes);
    TEST_ASSERT_EQUAL_UINT((uint32_t)(INTERLEAVE_N + 1), tree.count);

    /* Remove half by always grabbing the fresh first child, avoiding
     * stale indices caused by compaction in sexp_remove. */
    int to_remove = INTERLEAVE_N / 2;
    for (int i = 0; i < to_remove; i++) {
        uint32_t c = sexp_first_child(&tree, 0);
        TEST_ASSERT_NOT_EQUAL(SEXP_NULL_INDEX, c);
        sexp_remove(&tree, c);
    }
    TEST_ASSERT_EQUAL_UINT((uint32_t)(INTERLEAVE_N - to_remove + 1), tree.count);

    /* Re-append to_remove new atoms to the end. */
    uint32_t tail = SEXP_NULL_INDEX;
    uint32_t cx   = sexp_first_child(&tree, 0);
    while (cx != SEXP_NULL_INDEX) {
        tail = cx;
        cx   = sexp_next_sibling(&tree, cx);
    }

    for (int i = 0; i < to_remove; i++) {
        uint32_t n = sexp_allocate_node(&tree, NODE_ATOM);
        TEST_ASSERT_NOT_EQUAL(SEXP_NULL_INDEX, n);
        sexp_set_atom(&tree, n, "re", 2);
        sexp_insert(&tree, 0, tail, n);
        tail = n;
    }

    TEST_ASSERT_EQUAL_UINT((uint32_t)(INTERLEAVE_N + 1), tree.count);
    sexp_free(&tree);
#undef INTERLEAVE_N
}

void run_mutate_tests(void) {
    RUN_TEST(test_insert_as_first_child);
    RUN_TEST(test_remove_middle_child);
    RUN_TEST(test_remove_last_node);
    RUN_TEST(test_insert_into_atom_is_noop);
    RUN_TEST(test_insert_after_wrong_parent_is_noop);
    RUN_TEST(test_insert_auto_detaches_child);
    RUN_TEST(test_stress_insert_many);
    RUN_TEST(test_stress_remove_sequential);
    RUN_TEST(test_stress_interleaved_insert_remove);
}

int main(void) {
    UNITY_BEGIN();
    run_mutate_tests();
    return UNITY_END();
}
