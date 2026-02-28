#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void run_arena_tests(void);
void run_intern_tests(void);
void run_tree_tests(void);

int main(void) {
    UNITY_BEGIN();
    run_arena_tests();
    run_intern_tests();
    run_tree_tests();
    return UNITY_END();
}