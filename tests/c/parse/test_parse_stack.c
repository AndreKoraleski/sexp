#include <stdlib.h>

#include "unity.h"

#include "memory/intern.h"
#include "parse/parse_stack.h"
#include "sexp.h"

void setUp(void) {
    intern_init();
    intern_retain();
}

void tearDown(void) {
    intern_release();
}

static void stack_init(ParseStack *stack) {
    stack->data     = stack->inline_buffer;
    stack->top      = 0;
    stack->capacity = PARSE_STACK_INLINE_CAPACITY;
    stack->heap     = 0;
}

static void test_stack_push_pop_stays_inline(void) {
    ParseStack stack;
    stack_init(&stack);

    for (uint32_t i = 0; i < PARSE_STACK_INLINE_CAPACITY; i++) {
        TEST_ASSERT_EQUAL_INT(0, stack_push(&stack, i));
    }
    TEST_ASSERT_EQUAL_INT(0, stack.heap);
    TEST_ASSERT_EQUAL_UINT(PARSE_STACK_INLINE_CAPACITY, stack.top);

    ParseFrame *top = stack_peek(&stack);
    TEST_ASSERT_NOT_NULL(top);
    TEST_ASSERT_EQUAL_UINT(PARSE_STACK_INLINE_CAPACITY - 1, top->node);

    for (uint32_t i = 0; i < PARSE_STACK_INLINE_CAPACITY; i++) {
        TEST_ASSERT_EQUAL_INT(0, stack_pop(&stack));
    }
    TEST_ASSERT_EQUAL_UINT(0, stack.top);
    TEST_ASSERT_NULL(stack_peek(&stack));

    parse_stack_free(&stack);
}

static void test_stack_spills_to_heap_at_capacity_plus_one(void) {
    ParseStack stack;
    stack_init(&stack);

    for (uint32_t i = 0; i < PARSE_STACK_INLINE_CAPACITY; i++) {
        TEST_ASSERT_EQUAL_INT(0, stack_push(&stack, i));
    }
    TEST_ASSERT_EQUAL_INT(0, stack.heap);

    TEST_ASSERT_EQUAL_INT(0, stack_push(&stack, PARSE_STACK_INLINE_CAPACITY));
    TEST_ASSERT_EQUAL_INT(1, stack.heap);
    TEST_ASSERT_EQUAL_UINT(PARSE_STACK_INLINE_CAPACITY + 1, stack.top);
    TEST_ASSERT_EQUAL_UINT(PARSE_STACK_INLINE_CAPACITY * 2, stack.capacity);

    for (uint32_t i = 0; i < PARSE_STACK_INLINE_CAPACITY + 1; i++) {
        TEST_ASSERT_EQUAL_UINT(i, stack.data[i].node);
    }

    parse_stack_free(&stack);
}

static void test_stack_reallocs_on_second_overflow(void) {
    ParseStack stack;
    stack_init(&stack);

    for (uint32_t i = 0; i <= PARSE_STACK_INLINE_CAPACITY; i++) {
        TEST_ASSERT_EQUAL_INT(0, stack_push(&stack, i));
    }
    TEST_ASSERT_EQUAL_INT(1, stack.heap);
    uint32_t cap_after_first_spill = stack.capacity;

    while (stack.top < cap_after_first_spill) {
        TEST_ASSERT_EQUAL_INT(0, stack_push(&stack, stack.top));
    }
    TEST_ASSERT_EQUAL_UINT(cap_after_first_spill, stack.top);

    TEST_ASSERT_EQUAL_INT(0, stack_push(&stack, stack.top));
    TEST_ASSERT_EQUAL_UINT(cap_after_first_spill * 2, stack.capacity);
    TEST_ASSERT_EQUAL_INT(1, stack.heap);

    parse_stack_free(&stack);
}

static void test_stack_heap_free_releases_memory(void) {
    ParseStack stack;
    stack_init(&stack);

    for (uint32_t i = 0; i <= PARSE_STACK_INLINE_CAPACITY; i++) {
        stack_push(&stack, i);
    }
    TEST_ASSERT_EQUAL_INT(1, stack.heap);

    parse_stack_free(&stack);
}

static void test_stack_peek_empty_returns_null(void) {
    ParseStack stack;
    stack_init(&stack);
    TEST_ASSERT_NULL(stack_peek(&stack));
}

static void test_stack_pop_empty_returns_error(void) {
    ParseStack stack;
    stack_init(&stack);
    TEST_ASSERT_EQUAL_INT(-1, stack_pop(&stack));
}

static void test_spilled_stack_data_survives_round_trip(void) {
    ParseStack stack;
    stack_init(&stack);

    uint32_t count = PARSE_STACK_INLINE_CAPACITY + 4;
    for (uint32_t i = 0; i < count; i++) {
        TEST_ASSERT_EQUAL_INT(0, stack_push(&stack, i));
    }
    TEST_ASSERT_EQUAL_INT(1, stack.heap);

    for (uint32_t i = count; i > 0; i--) {
        ParseFrame *f = stack_peek(&stack);
        TEST_ASSERT_NOT_NULL(f);
        TEST_ASSERT_EQUAL_UINT(i - 1, f->node);
        stack_pop(&stack);
    }
    TEST_ASSERT_EQUAL_UINT(0, stack.top);
    TEST_ASSERT_NULL(stack_peek(&stack));

    parse_stack_free(&stack);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_stack_push_pop_stays_inline);
    RUN_TEST(test_stack_spills_to_heap_at_capacity_plus_one);
    RUN_TEST(test_stack_reallocs_on_second_overflow);
    RUN_TEST(test_stack_heap_free_releases_memory);
    RUN_TEST(test_stack_peek_empty_returns_null);
    RUN_TEST(test_stack_pop_empty_returns_error);
    RUN_TEST(test_spilled_stack_data_survives_round_trip);
    return UNITY_END();
}
