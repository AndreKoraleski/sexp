#include <stdlib.h>

#include "core/node.h"
#include "memory/intern.h"

/** Initial node array capacity allocated on first use. */
#define NODE_ARRAY_INITIAL_CAPACITY 64

uint32_t allocate_node(SExp *tree) {
    if (tree->count >= tree->capacity) {
        /* Double capacity on each growth, starting from the initial fixed capacity. */
        uint32_t new_capacity =
            tree->capacity == 0 ? NODE_ARRAY_INITIAL_CAPACITY : tree->capacity << 1;
        Node *new_nodes = realloc(tree->nodes, new_capacity * sizeof(Node));

        if (new_nodes == NULL) {
            return SEXP_NULL_INDEX;
        }

        tree->nodes    = new_nodes;
        tree->capacity = new_capacity;
    }
    uint32_t index     = tree->count++;
    tree->nodes[index] = (Node){
        .type         = NODE_ATOM,
        .atom_id      = 0,
        .first_child  = SEXP_NULL_INDEX,
        .next_sibling = SEXP_NULL_INDEX,
        .parent       = SEXP_NULL_INDEX,
    };
    return index;
}

uint32_t sexp_allocate_node(SExp *tree, NodeType type) {
    if (type != NODE_ATOM && type != NODE_LIST) {
        return SEXP_NULL_INDEX;
    }

    uint32_t index = allocate_node(tree);
    if (index != SEXP_NULL_INDEX) {
        /* allocate_node always sets type to NODE_ATOM. Overwrite if a list was requested. */
        tree->nodes[index].type = type;
    }

    return index;
}

void sexp_free(SExp *tree) {
    if (tree == NULL || !tree->valid) {
        return;
    }
    free(tree->nodes);
    intern_release();
    *tree = (SExp){0}; /* Zero the struct so stale pointers and flags are cleared. */
}
