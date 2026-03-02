#include "core/tree.h"
#include "memory/intern.h"

uint32_t sexp_first_child(const SExp *tree, uint32_t index) {
    if (index >= tree->count) {
        return SEXP_NULL_INDEX;
    }
    return tree->nodes[index].first_child;
}

uint32_t sexp_next_sibling(const SExp *tree, uint32_t index) {
    if (index >= tree->count) {
        return SEXP_NULL_INDEX;
    }
    return tree->nodes[index].next_sibling;
}

uint32_t sexp_parent(const SExp *tree, uint32_t index) {
    if (index >= tree->count) {
        return SEXP_NULL_INDEX;
    }
    return tree->nodes[index].parent;
}

NodeType sexp_kind(const SExp *tree, uint32_t index) {
    if (index >= tree->count) {
        return NODE_INVALID;
    }
    return tree->nodes[index].type;
}

AtomId sexp_atom(const SExp *tree, uint32_t index) {
    if (index >= tree->count || tree->nodes[index].type != NODE_ATOM) {
        return 0;
    }
    return tree->nodes[index].atom_id;
}

void sexp_set_atom(SExp *tree, uint32_t index, const char *string, size_t length) {
    if (index >= tree->count || tree->nodes[index].type != NODE_ATOM) {
        return;
    }
    AtomId atom_id = intern_string(string, length);
    if (atom_id != 0) {
        /* Only update the node if interning succeeded, leave it unchanged on failure. */
        tree->nodes[index].atom_id = atom_id;
    }
}
