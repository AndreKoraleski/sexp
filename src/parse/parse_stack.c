#include <stdlib.h>
#include <string.h>

#include "parse/parse_stack.h"

int stack_push(ParseStack *stack, uint32_t node) {
    if (stack->top >= stack->capacity) {
        uint32_t    new_capacity = stack->capacity << 1;
        ParseFrame *new_buffer;

        if (stack->heap) {
            /* Already on the heap - realloc in place. */
            new_buffer = realloc(stack->data, new_capacity * sizeof(ParseFrame));
            if (new_buffer == NULL) {
                return -1;
            }
        } else {
            /* First spill from the inline buffer - allocate a heap copy. */
            new_buffer = malloc(new_capacity * sizeof(ParseFrame));
            if (new_buffer == NULL) {
                return -1;
            }
            memcpy(new_buffer, stack->inline_buffer, stack->top * sizeof(ParseFrame));
            stack->heap = 1;
        }

        stack->data     = new_buffer;
        stack->capacity = new_capacity;
    }
    stack->data[stack->top++] = (ParseFrame){node, SEXP_NULL_INDEX};
    return 0;
}

int stack_pop(ParseStack *stack) {
    if (stack->top == 0) {
        return -1;
    }
    --stack->top;
    return 0;
}

ParseFrame *stack_peek(ParseStack *stack) {
    if (stack->top == 0) {
        return NULL;
    }
    return &stack->data[stack->top - 1];
}

void parse_stack_free(ParseStack *stack) {
    if (stack->heap) {
        free(stack->data);
    }
}

void frame_append_child(SExp *tree, ParseFrame *frame, uint32_t child) {
    tree->nodes[child].parent = frame->node;
    if (frame->last_child == SEXP_NULL_INDEX) {
        /* First child of this list node. */
        tree->nodes[frame->node].first_child = child;
    } else {
        tree->nodes[frame->last_child].next_sibling = child;
    }
    frame->last_child = child;
}
