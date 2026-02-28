#include "unity.h"
#include "arena.h"

void setUp(void) {}
void tearDown(void) {}

static void test_arena_init_default_cap(void) {
    Arena a = arena_init(0);
    TEST_ASSERT_NOT_NULL(a.base);
    TEST_ASSERT_EQUAL_UINT(ARENA_DEFAULT_CAP, a.cap);
    TEST_ASSERT_EQUAL_UINT(0, a.pos);
    TEST_ASSERT_NULL(a.prev);
    arena_free(&a);
}

static void test_arena_init_custom_cap(void) {
    Arena a = arena_init(1024);
    TEST_ASSERT_NOT_NULL(a.base);
    TEST_ASSERT_EQUAL_UINT(1024, a.cap);
    arena_free(&a);
}

static void test_arena_alloc_returns_aligned(void) {
    Arena a = arena_init(1024);
    void *p = arena_alloc(&a, 1);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_UINT(0, (uintptr_t)p % _Alignof(max_align_t));
    arena_free(&a);
}

static void test_arena_alloc_sequential(void) {
    Arena a = arena_init(1024);
    void *p1 = arena_alloc(&a, 8);
    void *p2 = arena_alloc(&a, 8);
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_TRUE(p2 > p1);
    arena_free(&a);
}

static void test_arena_alloc_grows(void) {
    Arena a = arena_init(64);
    for (int i = 0; i < 10; i++) {
        void *p = arena_alloc(&a, 16);
        TEST_ASSERT_NOT_NULL(p);
    }
    TEST_ASSERT_NOT_NULL(a.prev);
    arena_free(&a);
}

static void test_arena_reset_resets_pos(void) {
    Arena a = arena_init(1024);
    arena_alloc(&a, 64);
    arena_reset(&a);
    TEST_ASSERT_EQUAL_UINT(0, a.pos);
    TEST_ASSERT_NULL(a.prev);
    arena_free(&a);
}

static void test_arena_reset_reuses_first_chunk(void) {
    Arena a = arena_init(64);
    uint8_t *first_base = a.base;
    for (int i = 0; i < 10; i++)
        arena_alloc(&a, 16);
    arena_reset(&a);
    TEST_ASSERT_EQUAL_PTR(first_base, a.base);
    TEST_ASSERT_NULL(a.prev);
    arena_free(&a);
}

static void test_arena_free_zeroes_struct(void) {
    Arena a = arena_init(1024);
    arena_free(&a);
    TEST_ASSERT_NULL(a.base);
    TEST_ASSERT_EQUAL_UINT(0, a.cap);
    TEST_ASSERT_EQUAL_UINT(0, a.pos);
}

static void test_arena_alloc_after_reset(void) {
    Arena a = arena_init(1024);
    void *p1 = arena_alloc(&a, 64);
    arena_reset(&a);
    void *p2 = arena_alloc(&a, 64);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_EQUAL_PTR(p1, p2);
    arena_free(&a);
}

void run_arena_tests(void) {
    RUN_TEST(test_arena_init_default_cap);
    RUN_TEST(test_arena_init_custom_cap);
    RUN_TEST(test_arena_alloc_returns_aligned);
    RUN_TEST(test_arena_alloc_sequential);
    RUN_TEST(test_arena_alloc_grows);
    RUN_TEST(test_arena_reset_resets_pos);
    RUN_TEST(test_arena_reset_reuses_first_chunk);
    RUN_TEST(test_arena_free_zeroes_struct);
    RUN_TEST(test_arena_alloc_after_reset);
}

int main(void) {
    UNITY_BEGIN();
    run_arena_tests();
    return UNITY_END();
}