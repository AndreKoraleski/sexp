#include <stdlib.h>

#include "parse/parser.h"

#include "core/node.h"
#include "core/tree.h"
#include "memory/intern.h"
#include "parse/parse_stack.h"
#include "parse/tokenizer.h"

/**
 * @brief Handle a left parenthesis token by allocating a new list node, attaching it to the current
 * frame if one is active, and pushing a new frame onto the stack for subsequent children.
 *
 * @param tree  Pointer to the tree being constructed.
 * @param stack Pointer to the parse stack.
 * @return int  0 on success, -1 on allocation failure.
 */
static int parse_handle_lparen(SExp *tree, ParseStack *stack) {
    uint32_t node_index = allocate_node(tree);

    if (node_index == SEXP_NULL_INDEX) {
        return -1;
    }

    tree->nodes[node_index].type = NODE_LIST;
    ParseFrame *frame            = stack_peek(stack);

    if (frame != NULL) {
        frame_append_child(tree, frame, node_index);
    }

    return stack_push(stack, node_index);
}

/**
 * @brief Intern the token's string, allocate an atom node, and attach it to the current frame.
 *
 * @param tree  Pointer to the tree being constructed.
 * @param stack Pointer to the parse stack.
 * @param token Token containing the atom string to intern and node to create.
 * @return int  0 on success, -1 on intern or allocation failure.
 */
static int parse_handle_atom(SExp *tree, ParseStack *stack, Token token) {
    AtomId atom_id = intern_string(token.string, token.length);

    if (atom_id == 0) {
        return -1;
    }

    uint32_t node_index = allocate_node(tree);

    if (node_index == SEXP_NULL_INDEX) {
        return -1;
    }

    tree->nodes[node_index].type    = NODE_ATOM;
    tree->nodes[node_index].atom_id = atom_id;
    ParseFrame *frame               = stack_peek(stack);

    if (frame != NULL) {
        frame_append_child(tree, frame, node_index);
    }

    return 0;
}

/**
 * @brief Dispatch a token to the appropriate handler. TOKEN_END is never passed here, it terminates
 * the caller's loop.
 *
 * @param tree  Pointer to the tree being constructed.
 * @param stack Pointer to the parse stack.
 * @param token Token to dispatch.
 * @return int  0 on success, -1 on any error that should abort the parse.
 */
static int parse_dispatch_token(SExp *tree, ParseStack *stack, Token token) {
    if (token.kind == TOKEN_ERROR) {
        return -1;
    }
    if (token.kind == TOKEN_LEFT_PARENTHESIS) {
        return parse_handle_lparen(tree, stack);
    }
    if (token.kind == TOKEN_RIGHT_PARENTHESIS) {
        return stack_pop(stack);
    }
    if (token.kind == TOKEN_ATOM) {
        return parse_handle_atom(tree, stack, token);
    }
    return 0;
}

SExp sexp_parse(const char *source, size_t source_length) {
    SExp tree = {0};

    if (intern_init() != 0) {
        return tree;
    }

    /* Claim ownership of the pool for the lifetime of this tree. */
    intern_retain();

    ParseStack stack = {0};
    /* Bootstrap the stack onto the inline frame buffer before any tokens are read. */
    stack.data     = stack.inline_buffer;
    stack.capacity = PARSE_STACK_INLINE_CAPACITY;

    Tokenizer tokenizer = {source, source + source_length};
    Token     token;
    while ((token = next_token(&tokenizer)).kind != TOKEN_END) {
        if (parse_dispatch_token(&tree, &stack, token) != 0) {
            goto error;
        }
    }

    /* Unclosed parenthesis - the input is malformed. */
    if (stack.top > 0) {
        goto error;
    }

    parse_stack_free(&stack);
    tree.valid = 1;
    return tree;

error:
    free(tree.nodes); /* Discard the partially built node array. */
    parse_stack_free(&stack);
    intern_release();
    return (SExp){0};
}
