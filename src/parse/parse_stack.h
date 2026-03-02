#pragma once

#include <stdint.h>

#include "core/node.h"


/** Number of frames the stack can hold without a heap allocation. */
#define PARSE_STACK_INLINE_CAPACITY 32

/**
 * @brief A single frame on the parse stack, representing a list node being constructed.
 */
typedef struct ParseFrame {
    uint32_t node;       /**< Index of the LIST node being parsed. */
    uint32_t last_child; /**< Index of the last appended child, or SEXP_NULL_INDEX. */
} ParseFrame;

/**
 * @brief Dynamic stack of parse frames used during parsing.
 */
typedef struct ParseStack {
    ParseFrame
        inline_buffer[PARSE_STACK_INLINE_CAPACITY]; /**< Inline storage for typical depths. */
    ParseFrame *data; /**< Points to inline_buffer initially, or a heap allocation on overflow. */
    uint32_t    top;  /**< Number of elements currently on the stack. */
    uint32_t    capacity; /**< Allocated capacity in elements. */
    int         heap;     /**< Non-zero if data points to a heap allocation. */
} ParseStack;

/**
 * @brief Pushes a new frame for the given list node index onto the parse stack.
 *
 * @param stack Pointer to the parse stack.
 * @param node  Index of the list node being parsed.
 * @return int  0 on success, -1 on out-of-memory error.
 */
int stack_push(ParseStack *stack, uint32_t node);

/**
 * @brief Pops the top frame from the parse stack.
 *
 * @param stack Pointer to the parse stack.
 * @return int  0 on success, -1 if the stack is empty.
 */
int stack_pop(ParseStack *stack);

/**
 * @brief Returns a pointer to the top frame on the parse stack.
 *
 * @param stack Pointer to the parse stack.
 * @return ParseFrame* Pointer to the top frame, or NULL if the stack is empty.
 */
ParseFrame *stack_peek(ParseStack *stack);

/**
 * @brief Frees any heap allocation owned by the parse stack.
 *
 * @param stack Pointer to the parse stack.
 */
void parse_stack_free(ParseStack *stack);

/**
 * @brief Appends a child node to the list being built in the given frame, updating parent and
 * sibling links.
 *
 * @param tree Pointer to the S-expression tree.
 * @param frame Pointer to the parse frame.
 * @param child Index of the child node to append.
 */
void frame_append_child(SExp *tree, ParseFrame *frame, uint32_t child);
