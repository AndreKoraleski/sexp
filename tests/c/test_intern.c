#include "unity.h"
#include "intern.h"

void setUp(void) {
    intern_init();
    intern_retain();
}

void tearDown(void) {
    intern_release();
}

static void test_intern_string_new_returns_nonzero(void) {
    AtomId id = intern_string("foo", 3);
    TEST_ASSERT_NOT_EQUAL(0, id);
}

static void test_intern_string_deduplication(void) {
    AtomId a = intern_string("foo", 3);
    AtomId b = intern_string("foo", 3);
    TEST_ASSERT_EQUAL_UINT32(a, b);
}

static void test_intern_string_distinct(void) {
    AtomId a = intern_string("foo", 3);
    AtomId b = intern_string("bar", 3);
    TEST_ASSERT_NOT_EQUAL(a, b);
}

static void test_intern_lookup_valid(void) {
    AtomId id = intern_string("hello", 5);
    size_t len = 0;
    const char *str = intern_lookup(id, &len);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_UINT(5, len);
    TEST_ASSERT_EQUAL_STRING("hello", str);
}

static void test_intern_lookup_invalid_zero(void) {
    const char *str = intern_lookup(0, NULL);
    TEST_ASSERT_NULL(str);
}

static void test_intern_lookup_out_of_bounds(void) {
    const char *str = intern_lookup(99999, NULL);
    TEST_ASSERT_NULL(str);
}

static void test_intern_table_growth(void) {
    char buf[16];
    AtomId ids[40];
    for (int i = 0; i < 40; i++) {
        int len = snprintf(buf, sizeof(buf), "atom%d", i);
        ids[i] = intern_string(buf, (size_t)len);
        TEST_ASSERT_NOT_EQUAL(0, ids[i]);
    }
    for (int i = 0; i < 40; i++) {
        int len = snprintf(buf, sizeof(buf), "atom%d", i);
        AtomId id = intern_string(buf, (size_t)len);
        TEST_ASSERT_EQUAL_UINT32(ids[i], id);
    }
}

static void test_intern_refcount_pool_freed(void) {
    AtomId id = intern_string("x", 1);
    TEST_ASSERT_NOT_EQUAL(0, id);
    intern_release();
    intern_release();
    TEST_ASSERT_EQUAL_INT(0, intern_init());
    intern_retain();
    AtomId id2 = intern_string("x", 1);
    TEST_ASSERT_NOT_EQUAL(0, id2);
}

void run_intern_tests(void) {
    RUN_TEST(test_intern_string_new_returns_nonzero);
    RUN_TEST(test_intern_string_deduplication);
    RUN_TEST(test_intern_string_distinct);
    RUN_TEST(test_intern_lookup_valid);
    RUN_TEST(test_intern_lookup_invalid_zero);
    RUN_TEST(test_intern_lookup_out_of_bounds);
    RUN_TEST(test_intern_table_growth);
    RUN_TEST(test_intern_refcount_pool_freed);
}

int main(void) {
    UNITY_BEGIN();
    run_intern_tests();
    return UNITY_END();
}