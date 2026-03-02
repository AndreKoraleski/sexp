#include <stdlib.h>
#include <string.h>

#include "serialize/serializer.h"

#include "core/tree.h"
#include "memory/arena.h"
#include "memory/intern.h"

/** Multiplier applied to tree->count when sizing the iterative DFS stack. */
#define SERIALIZE_STACK_FACTOR 2
/** Multiplier applied to tree->count when sizing the scratch arena. */
#define SCRATCH_ARENA_FACTOR 4

/**
 * @brief Frame representing a node to emit during serialization, along with flags for formatting.
 */
typedef struct SerializeFrame {
    uint32_t index;       /**< Index of the node to emit. */
    uint8_t  needs_close; /**< Non-zero if a ')' should be emitted instead of the node. */
    uint8_t  needs_space; /**< Non-zero if a space should be emitted before the node. */
} SerializeFrame;

/**
 * @brief Compute the number of bytes needed to serialize the subtree rooted at root.
 *
 * Uses a BFS work queue allocated from scratch. Returns 0 on empty or out-of-bounds input.
 *
 * @param tree    Pointer to the tree containing the node.
 * @param root    Index of the root of the subtree to measure.
 * @param scratch Arena used for temporary allocations during measurement. Must have capacity for at
 * least tree->count * sizeof(uint32_t) bytes.
 * @return size_t Number of bytes needed to serialize the subtree, or 0 on empty or out-of-bounds
 * input.
 */
static size_t measure_node(const SExp *tree, uint32_t root, Arena *scratch) {
    if (root >= tree->count) {
        return 0;
    }

    uint32_t *work = arena_alloc(scratch, tree->count * sizeof(uint32_t));
    if (work == NULL) {
        return 0;
    }

    size_t   total = 0;
    uint32_t top   = 0;
    work[top++]    = root;

    while (top > 0) {
        uint32_t index = work[--top];

        if (tree->nodes[index].type == NODE_ATOM) {
            size_t atom_length = 0;
            intern_lookup(tree->nodes[index].atom_id, &atom_length);
            total += atom_length;
            continue;
        }

        total += 2;
        uint32_t child      = tree->nodes[index].first_child;
        uint32_t child_count = 0;
        while (child != SEXP_NULL_INDEX) {
            work[top++] = child;
            child       = tree->nodes[child].next_sibling;
            child_count++;
        }
        if (child_count > 0) {
            total += child_count - 1;
        }
    }

    return total;
}

/**
 * @brief Emit the interned string for the atom at index into destination, advancing
 * position by the string's byte length. No-op if the atom id is unresolvable.
 *
 * @param tree        Pointer to the tree containing the node.
 * @param index       Index of the atom node.
 * @param destination Buffer to write the atom string into.
 * @param position    Pointer to the current position in the buffer, which will be advanced by the
 * string's byte length.
 */
static void emit_atom_node(const SExp *tree, uint32_t index, char *destination, size_t *position) {
    size_t      atom_length = 0;
    const char *string      = intern_lookup(tree->nodes[index].atom_id, &atom_length);

    if (string != NULL) {
        memcpy(destination + *position, string, atom_length);
        *position += atom_length;
    }
}

/**
 * @brief Push a closing ')' sentinel and then each child of the list node at index onto the
 * serialization stack in reverse order so that they are emitted left-to-right.
 *
 * Returns the updated stack top.
 *
 * @param tree     Pointer to the tree containing the node.
 * @param index    Index of the list node.
 * @param stack    Serialization stack.
 * @param top      Current top of the stack.
 * @param children Temporary buffer for storing child indices.
 * @return uint32_t Updated top of the stack.
 */
static uint32_t push_list_children(
    const SExp *tree, uint32_t index, SerializeFrame *stack, uint32_t top, uint32_t *children
) {
    stack[top++] = (SerializeFrame){0, 1, 0};

    uint32_t child_count = 0;
    uint32_t child      = tree->nodes[index].first_child;
    while (child != SEXP_NULL_INDEX) {
        children[child_count++] = child;
        child                  = tree->nodes[child].next_sibling;
    }

    for (uint32_t i = child_count; i > 0; i--) {
        uint8_t needs_space = (i - 1 > 0) ? 1 : 0;
        stack[top++]        = (SerializeFrame){children[i - 1], 0, needs_space};
    }

    return top;
}

/**
 * @brief Serialize the subtree rooted at root into destination using an iterative DFS. Space
 * and closing parenthesis insertion are driven by the needs_space and needs_close flags on each
 * frame.
 *
 * Requires destination to have at least the byte count returned by measure_node.
 *
 * @param tree        Pointer to the tree containing the node.
 * @param root        Index of the root node of the subtree to serialize.
 * @param destination Buffer to write the serialized subtree into.
 * @param position    Pointer to the current position in the buffer, which will be advanced as bytes
 *                    are written.
 * @param scratch     Temporary arena for allocating serialization stack and child buffers.
 */
static void
write_node(const SExp *tree, uint32_t root, char *destination, size_t *position, Arena *scratch) {
    if (root >= tree->count) {
        return;
    }

    SerializeFrame *stack =
        arena_alloc(scratch, (size_t)SERIALIZE_STACK_FACTOR * tree->count * sizeof(SerializeFrame));
    uint32_t *children = arena_alloc(scratch, tree->count * sizeof(uint32_t));

    if (stack == NULL || children == NULL) {
        return;
    }

    uint32_t top = 0;
    stack[top++] = (SerializeFrame){root, 0, 0};

    while (top > 0) {
        SerializeFrame frame = stack[--top];

        if (frame.needs_close) {
            destination[(*position)++] = ')';
            continue;
        }

        if (frame.needs_space) {
            destination[(*position)++] = ' ';
        }

        if (tree->nodes[frame.index].type == NODE_ATOM) {
            emit_atom_node(tree, frame.index, destination, position);
        } else {
            destination[(*position)++] = '(';
            top = push_list_children(tree, frame.index, stack, top, children);
        }
    }
}

/**
 * @brief Sum the serialized byte length of every top-level node (parent == SEXP_NULL_INDEX),
 * including one space between consecutive top-level forms. Stores the root count in
 * output_root_count.
 *
 * @param tree           Pointer to the tree containing the nodes.
 * @param scratch        Temporary arena for allocating serialization stack and child buffers.
 * @param output_root_count Pointer to store the number of top-level roots.
 * @return size_t        Total serialized byte length of all top-level nodes.
 */
static size_t measure_top_level(const SExp *tree, Arena *scratch, uint32_t *output_root_count) {
    size_t   total   = 0;
    uint32_t root_count = 0;
    for (uint32_t i = 0; i < tree->count; i++) {

        if (tree->nodes[i].parent != SEXP_NULL_INDEX) {
            continue;
        }

        if (root_count > 0) {
            total += 1; /* Space between top-level forms. */
        }
        total += measure_node(tree, i, scratch);
        root_count++;
    }
    *output_root_count = root_count;
    return total;
}

/**
 * @brief Write all top-level nodes (parent == SEXP_NULL_INDEX) into buffer separated by spaces.
 *
 * Returns the number of bytes written.
 *
 * @param tree    Pointer to the tree containing the nodes.
 * @param buffer  Buffer to write the serialized top-level nodes into.
 * @param scratch Temporary arena for allocating serialization stack and child buffers.
 * @return size_t Number of bytes written.
 */
static size_t write_top_level(const SExp *tree, char *buffer, Arena *scratch) {
    size_t position = 0;
    int    first    = 1;

    for (uint32_t i = 0; i < tree->count; i++) {
        if (tree->nodes[i].parent != SEXP_NULL_INDEX) {
            continue;
        }

        if (!first) {
            buffer[position++] = ' ';
        }

        first = 0;
        write_node(tree, i, buffer, &position, scratch);
    }
    return position;
}

char *sexp_serialize_node(const SExp *tree, uint32_t index, size_t *output_length) {
    if (tree->count == 0 || index >= tree->count) {
        if (output_length != NULL) {
            *output_length = 0;
        }
        return NULL;
    }

    Arena scratch = arena_init((size_t)SCRATCH_ARENA_FACTOR * tree->count * sizeof(uint32_t));
    if (scratch.base == NULL) {
        return NULL;
    }

    size_t needed = measure_node(tree, index, &scratch);
    if (needed == 0) {
        arena_free(&scratch);
        return NULL;
    }

    char *buffer = malloc(needed + 1);
    if (buffer == NULL) {
        arena_free(&scratch);
        return NULL;
    }

    /* Reset the scratch arena between the measure and write passes to reuse the same memory. */
    arena_reset(&scratch);
    size_t position = 0;
    write_node(tree, index, buffer, &position, &scratch);
    arena_free(&scratch);
    buffer[position] = '\0';

    if (output_length != NULL) {
        *output_length = position;
    }

    return buffer;
}

char *sexp_serialize(const SExp *tree, size_t *output_length) {
    if (tree->count == 0) {
        if (output_length != NULL) {
            *output_length = 0;
        }
        return NULL;
    }

    Arena scratch = arena_init((size_t)SCRATCH_ARENA_FACTOR * tree->count * sizeof(uint32_t));
    if (scratch.base == NULL) {
        return NULL;
    }

    uint32_t root_count = 0;
    size_t   total   = measure_top_level(tree, &scratch, &root_count);

    if (root_count == 0 || total == 0) {
        arena_free(&scratch);
        if (output_length != NULL) {
            *output_length = 0;
        }
        return NULL;
    }

    char *buffer = malloc(total + 1);
    if (buffer == NULL) {
        arena_free(&scratch);
        return NULL;
    }

    /* Reset the scratch arena between the measure and write passes to reuse the same memory. */
    arena_reset(&scratch);
    size_t position = write_top_level(tree, buffer, &scratch);
    arena_free(&scratch);
    buffer[position] = '\0';

    if (output_length != NULL) {
        *output_length = position;
    }

    return buffer;
}
