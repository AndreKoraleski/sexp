#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "intern.h"
#include "tree.h"

#define PARSE_STACK_INLINE_CAP  32
#define NODE_ARRAY_INIT_CAP     64
#define SERIALIZE_STACK_FACTOR   2
#define SCRATCH_ARENA_FACTOR     4

/** Token categories produced by the tokenizer. */
typedef enum TokenKind {
    TOKEN_LPAREN, /**< Opening parenthesis. */
    TOKEN_RPAREN, /**< Closing parenthesis. */
    TOKEN_ATOM,   /**< Bare atom (identifier). */
    TOKEN_END,    /**< End of input. */
    TOKEN_ERROR   /**< Unrecognised character. */
} TokenKind;

/** A single token with its kind, source pointer, and byte length. */
typedef struct Token {
    TokenKind   kind; /**< Category of the token. */
    const char *str;  /**< Pointer into the source string; NULL for non-atom
                        tokens. */
    size_t      len;  /**< Byte length of the token text. */
} Token;

/** Cursor state for the hand-written tokenizer. */
typedef struct Tokenizer {
    const char *cursor; /**< Current read position. */
    const char *end;    /**< One past the last byte of input. */
} Tokenizer;

static int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int is_atom_char(char c) {
    return !is_whitespace(c) && c != '(' && c != ')';
}

static Token next_token(Tokenizer *tz) {
    while (tz->cursor < tz->end && is_whitespace(*tz->cursor))
        tz->cursor++;

    if (tz->cursor >= tz->end)
        return (Token){ TOKEN_END, NULL, 0 };

    char c = *tz->cursor;

    if (c == '(') {
        tz->cursor++;
        return (Token){ TOKEN_LPAREN, NULL, 0 };
    }

    if (c == ')') {
        tz->cursor++;
        return (Token){ TOKEN_RPAREN, NULL, 0 };
    }

    if (is_atom_char(c)) {
        const char *start = tz->cursor;
        while (tz->cursor < tz->end && is_atom_char(*tz->cursor))
            tz->cursor++;
        return (Token){ TOKEN_ATOM, start, (size_t)(tz->cursor - start) };
    }

    return (Token){ TOKEN_ERROR, NULL, 0 };
}

/** Work item on the parse stack. */
typedef struct ParseFrame {
    uint32_t node;       /**< Index of the LIST node being parsed. */
    uint32_t last_child; /**< Index of the last appended child,
                            or SEXP_NULL_INDEX. */
} ParseFrame;

/** Dynamic stack of parse frames used during parsing. */
typedef struct ParseStack {
    ParseFrame  inline_buf[PARSE_STACK_INLINE_CAP]; /**< Inline storage for
                                                    typical parse depths. */
    ParseFrame *data;  /**< Points to inline_buf initially, or a heap
                        allocation on overflow. */
    uint32_t    top;   /**< Number of elements currently on the stack. */
    uint32_t    cap;   /**< Allocated capacity in elements. */
    int         heap;  /**< Non-zero if data points to a heap allocation. */
} ParseStack;

static int stack_push(ParseStack *stack, uint32_t node) {
    if (stack->top >= stack->cap) {
        uint32_t    new_cap  = stack->cap << 1;
        ParseFrame *new_data;

        if (stack->heap) {
            new_data = realloc(stack->data, new_cap * sizeof(ParseFrame));
            if (new_data == NULL)
                return -1;
        } else {
            new_data = malloc(new_cap * sizeof(ParseFrame));
            if (new_data == NULL)
                return -1;
            memcpy(
                new_data,
                stack->inline_buf,
                stack->top * sizeof(ParseFrame)
            );
            stack->heap = 1;
        }

        stack->data = new_data;
        stack->cap  = new_cap;
    }
    stack->data[stack->top++] = (ParseFrame){ node, SEXP_NULL_INDEX };
    return 0;
}

static int stack_pop(ParseStack *stack) {
    if (stack->top == 0)
        return -1;
    --stack->top;
    return 0;
}

static ParseFrame *stack_peek(ParseStack *stack) {
    if (stack->top == 0)
        return NULL;
    return &stack->data[stack->top - 1];
}

static void frame_append_child(SExp *tree, ParseFrame *frame, uint32_t child) {
    tree->nodes[child].parent = frame->node;
    if (frame->last_child == SEXP_NULL_INDEX)
        tree->nodes[frame->node].first_child = child;
    else
        tree->nodes[frame->last_child].next_sibling = child;
    frame->last_child = child;
}


static uint32_t node_alloc(SExp *tree) {
    if (tree->count >= tree->cap) {
        uint32_t  new_cap   = tree->cap == 0
                            ? NODE_ARRAY_INIT_CAP
                            : tree->cap << 1;
        Node     *new_nodes = realloc(tree->nodes, new_cap * sizeof(Node));

        if (new_nodes == NULL)
            return SEXP_NULL_INDEX;

        tree->nodes = new_nodes;
        tree->cap   = new_cap;
    }
    uint32_t idx = tree->count++;
    tree->nodes[idx] = (Node){
        .type         = NODE_ATOM,
        .atom_id      = 0,
        .first_child  = SEXP_NULL_INDEX,
        .next_sibling = SEXP_NULL_INDEX,
        .parent       = SEXP_NULL_INDEX
    };
    return idx;
}



/** Work item pushed onto the serialization stack. */
typedef struct SerializeFrame {
    uint32_t idx;         /**< Index of the node to emit. */
    uint8_t  needs_close; /**< Non-zero if a ')' should be emitted instead of
                            the node. */
    uint8_t  needs_space; /**< Non-zero if a space should be emitted before
                            the node. */
} SerializeFrame;

static size_t measure_node(const SExp *tree, uint32_t root, Arena *scratch) {
    if (root >= tree->count)
        return 0;

    uint32_t *work = arena_alloc(scratch, tree->count * sizeof(uint32_t));
    if (work == NULL)
        return 0;

    size_t   total = 0;
    uint32_t top   = 0;
    work[top++]    = root;

    while (top > 0) {
        uint32_t idx = work[--top];

        if (tree->nodes[idx].type == NODE_ATOM) {
            size_t atom_len = 0;
            intern_lookup(tree->nodes[idx].atom_id, &atom_len);
            total += atom_len;
            continue;
        }

        total += 2;
        uint32_t child      = tree->nodes[idx].first_child;
        uint32_t n_children = 0;
        while (child != SEXP_NULL_INDEX) {
            work[top++] = child;
            child = tree->nodes[child].next_sibling;
            n_children++;
        }
        if (n_children > 0)
            total += n_children - 1;
    }

    return total;
}

static void write_node(
    const SExp *tree,
    uint32_t root,
    char *dest,
    size_t *pos,
    Arena *scratch
) {
    if (root >= tree->count)
        return;

    SerializeFrame *stack    = arena_alloc(
        scratch,
        SERIALIZE_STACK_FACTOR * tree->count * sizeof(SerializeFrame)
    );
    uint32_t       *children = arena_alloc(
        scratch,
        tree->count * sizeof(uint32_t)
    );

    if (stack == NULL || children == NULL)
        return;

    uint32_t top = 0;
    stack[top++] = (SerializeFrame){ root, 0, 0 };

    while (top > 0) {
        SerializeFrame frame = stack[--top];

        if (frame.needs_close) {
            dest[(*pos)++] = ')';
            continue;
        }

        if (frame.needs_space)
            dest[(*pos)++] = ' ';

        uint32_t idx = frame.idx;

        if (tree->nodes[idx].type == NODE_ATOM) {
            size_t      atom_len = 0;
            const char *str      = intern_lookup(
                tree->nodes[idx].atom_id,
                &atom_len
            );

            if (str != NULL) {
                memcpy(dest + *pos, str, atom_len);
                *pos += atom_len;
            }
            continue;
        }

        dest[(*pos)++] = '(';
        stack[top++] = (SerializeFrame){ 0, 1, 0 };

        uint32_t n_children = 0;
        uint32_t child      = tree->nodes[idx].first_child;
        while (child != SEXP_NULL_INDEX) {
            children[n_children++] = child;
            child = tree->nodes[child].next_sibling;
        }

        for (uint32_t i = n_children; i > 0; i--) {
            uint8_t needs_space = (i - 1 > 0) ? 1 : 0;
            stack[top++] = (SerializeFrame){ children[i - 1], 0, needs_space };
        }
    }

}

SExp sexp_parse(const char *src, size_t src_len) {
    SExp tree = {0};

    if (intern_init() != 0)
        return tree;

    intern_retain();

    ParseStack stack = {0};
    stack.data = stack.inline_buf;
    stack.cap  = PARSE_STACK_INLINE_CAP;

    Tokenizer tz = { src, src + src_len };
    Token token;
    while ((token = next_token(&tz)).kind != TOKEN_END) {
        if (token.kind == TOKEN_ERROR)
            goto error;

        if (token.kind == TOKEN_LPAREN) {
            uint32_t idx = node_alloc(&tree);
            if (idx == SEXP_NULL_INDEX)
                goto error;

            tree.nodes[idx].type = NODE_LIST;

            ParseFrame *top = stack_peek(&stack);
            if (top != NULL)
                frame_append_child(&tree, top, idx);

            if (stack_push(&stack, idx) != 0)
                goto error;
            continue;
        }

        if (token.kind == TOKEN_RPAREN) {
            if (stack_pop(&stack) != 0)
                goto error;
            continue;
        }

        if (token.kind == TOKEN_ATOM) {
            AtomId id = intern_string(token.str, token.len);
            if (id == 0)
                goto error;

            uint32_t idx = node_alloc(&tree);
            if (idx == SEXP_NULL_INDEX)
                goto error;

            tree.nodes[idx].type    = NODE_ATOM;
            tree.nodes[idx].atom_id = id;

            ParseFrame *top = stack_peek(&stack);
            if (top != NULL)
                frame_append_child(&tree, top, idx);
            continue;
        }
    }

    if (stack.top > 0)
        goto error;

    if (stack.heap)
        free(stack.data);
    tree.valid = 1;
    return tree;

error:
    free(tree.nodes);
    if (stack.heap)
        free(stack.data);
    intern_release();
    return (SExp){0};
}

void sexp_free(SExp *tree) {
    if (tree == NULL || !tree->valid)
        return;
    free(tree->nodes);
    intern_release();
    *tree = (SExp){0};
}

uint32_t sexp_first_child(const SExp *tree, uint32_t idx) {
    if (idx >= tree->count)
        return SEXP_NULL_INDEX;
    return tree->nodes[idx].first_child;
}

uint32_t sexp_next_sibling(const SExp *tree, uint32_t idx) {
    if (idx >= tree->count)
        return SEXP_NULL_INDEX;
    return tree->nodes[idx].next_sibling;
}

uint32_t sexp_parent(const SExp *tree, uint32_t idx) {
    if (idx >= tree->count)
        return SEXP_NULL_INDEX;
    return tree->nodes[idx].parent;
}

NodeKind sexp_kind(const SExp *tree, uint32_t idx) {
    if (idx >= tree->count)
        return NODE_INVALID;
    return tree->nodes[idx].type;
}

AtomId sexp_atom(const SExp *tree, uint32_t idx) {
    if (idx >= tree->count || tree->nodes[idx].type != NODE_ATOM)
        return 0;
    return tree->nodes[idx].atom_id;
}

void sexp_set_atom(SExp *tree, uint32_t idx, const char *str, size_t len) {
    if (idx >= tree->count || tree->nodes[idx].type != NODE_ATOM)
        return;
    AtomId id = intern_string(str, len);
    if (id != 0)
        tree->nodes[idx].atom_id = id;
}

uint32_t sexp_node_alloc(SExp *tree, NodeKind kind) {
    if (kind != NODE_ATOM && kind != NODE_LIST)
        return SEXP_NULL_INDEX;

    uint32_t idx = node_alloc(tree);
    if (idx != SEXP_NULL_INDEX)
        tree->nodes[idx].type = kind;

    return idx;
}

void sexp_insert(SExp *tree, uint32_t parent, uint32_t after, uint32_t child) {
    if (parent >= tree->count || child >= tree->count)
        return;

    if (after != SEXP_NULL_INDEX && after >= tree->count)
        return;

    tree->nodes[child].parent = parent;

    if (after == SEXP_NULL_INDEX) {
        tree->nodes[child].next_sibling = tree->nodes[parent].first_child;
        tree->nodes[parent].first_child = child;
    } else {
        tree->nodes[child].next_sibling = tree->nodes[after].next_sibling;
        tree->nodes[after].next_sibling = child;
    }
}

void sexp_remove(SExp *tree, uint32_t idx) {
    if (idx >= tree->count)
        return;

    uint32_t parent = tree->nodes[idx].parent;
    if (parent != SEXP_NULL_INDEX) {
        if (tree->nodes[parent].first_child == idx) {
            tree->nodes[parent].first_child = tree->nodes[idx].next_sibling;
        } else {
            uint32_t prev = tree->nodes[parent].first_child;
            while (prev != SEXP_NULL_INDEX
                    && tree->nodes[prev].next_sibling != idx)
                prev = tree->nodes[prev].next_sibling;
            if (prev != SEXP_NULL_INDEX)
                tree->nodes[prev].next_sibling = tree->nodes[idx].next_sibling;
        }
    }

    uint32_t child = tree->nodes[idx].first_child;
    while (child != SEXP_NULL_INDEX) {
        tree->nodes[child].parent = SEXP_NULL_INDEX;
        child = tree->nodes[child].next_sibling;
    }

    uint32_t last = tree->count - 1;
    if (idx != last) {
        tree->nodes[idx] = tree->nodes[last];

        uint32_t moved_parent = tree->nodes[idx].parent;
        if (moved_parent != SEXP_NULL_INDEX) {

            if (tree->nodes[moved_parent].first_child == last) {
                tree->nodes[moved_parent].first_child = idx;
            } else {
                uint32_t prev = tree->nodes[moved_parent].first_child;

                while (prev != SEXP_NULL_INDEX
                        && tree->nodes[prev].next_sibling != last)
                    prev = tree->nodes[prev].next_sibling;

                if (prev != SEXP_NULL_INDEX)
                    tree->nodes[prev].next_sibling = idx;
            }
        }

        child = tree->nodes[idx].first_child;
        while (child != SEXP_NULL_INDEX) {
            tree->nodes[child].parent = idx;
            child = tree->nodes[child].next_sibling;
        }
    }

    tree->count--;
}

char *sexp_serialize_node(const SExp *tree, uint32_t idx, size_t *out_len) {
    if (tree->count == 0 || idx >= tree->count) {
        if (out_len != NULL)
            *out_len = 0;
        return NULL;
    }

    Arena scratch = arena_init(
        SCRATCH_ARENA_FACTOR * tree->count * sizeof(uint32_t)
    );

    if (scratch.base == NULL)
        return NULL;

    size_t needed = measure_node(tree, idx, &scratch);
    if (needed == 0) {
        arena_free(&scratch);
        return NULL;
    }

    char *buf = malloc(needed + 1);
    if (buf == NULL) {
        arena_free(&scratch);
        return NULL;
    }

    arena_reset(&scratch);
    size_t pos = 0;
    write_node(tree, idx, buf, &pos, &scratch);
    arena_free(&scratch);
    buf[pos] = '\0';

    if (out_len != NULL)
        *out_len = pos;

    return buf;
}

char *sexp_serialize(const SExp *tree, size_t *out_len) {
    return sexp_serialize_node(tree, 0, out_len);
}
