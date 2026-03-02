#include <stdlib.h>

#include "core/tree.h"

/**
 * @brief Walk the child list of parent to find the sibling immediately before target.
 *
 * Returns SEXP_NULL_INDEX if target is the first child or not found.
 *
 * @param tree      Pointer to the tree containing the nodes.
 * @param parent    Index of the parent node whose children to search.
 * @param target    Index of the child node to find the previous sibling of.
 * @return uint32_t Index of the previous sibling, or SEXP_NULL_INDEX if target is the first
 *                  child or not found.
 */
static uint32_t get_previous_sibling(const SExp *tree, uint32_t parent, uint32_t target) {
    uint32_t previous = tree->nodes[parent].first_child;

    while (previous != SEXP_NULL_INDEX && tree->nodes[previous].next_sibling != target) {
        previous = tree->nodes[previous].next_sibling;
    }

    return previous;
}

/**
 * @brief Splice index out of its parent's child list.
 *
 * Does not clear the node's own parent/sibling fields - callers are responsible for patching those
 * if needed. No-op if the node has no parent.
 *
 * @param tree  Pointer to the tree containing the nodes.
 * @param index Index of the node to unlink from its parent.
 */
static void unlink_from_parent(SExp *tree, uint32_t index) {
    uint32_t parent = tree->nodes[index].parent;
    if (parent == SEXP_NULL_INDEX) {
        return;
    }

    if (tree->nodes[parent].first_child == index) {
        tree->nodes[parent].first_child = tree->nodes[index].next_sibling;

    } else {
        uint32_t previous = get_previous_sibling(tree, parent, index);

        if (previous != SEXP_NULL_INDEX) {
            tree->nodes[previous].next_sibling = tree->nodes[index].next_sibling;
        }
    }
}

/**
 * @brief BFS-traverse the subtree rooted at root, recording every visited index in work and
 * setting the corresponding removed flag. Returns the number of nodes collected.
 *
 * @param tree      Pointer to the tree containing the nodes.
 * @param root      Index of the root of the subtree to traverse.
 * @param work      Output array in which visited node indices are stored in BFS order.
 * @param removed   Output byte array flagging each collected node with 1. Must be zeroed by the
 *                  caller.
 * @return uint32_t Number of nodes collected.
 */
static uint32_t
collect_removed_bfs(const SExp *tree, uint32_t root, uint32_t *work, uint8_t *removed) {
    uint32_t head = 0;
    uint32_t tail = 0;
    work[tail++]  = root;
    removed[root] = 1;

    while (head < tail) {
        uint32_t current = work[head++];
        uint32_t child   = tree->nodes[current].first_child;

        while (child != SEXP_NULL_INDEX) {
            work[tail++]   = child;
            removed[child] = 1;
            child          = tree->nodes[child].next_sibling;
        }
    }
    return tail;
}

/**
 * @brief Build a remapping from original node indices to new compacted indices after removal, where
 * removed nodes are assigned SEXP_NULL_INDEX.
 *
 * @param count   Total number of nodes in the original tree.
 * @param remap   Array to store the new indices for each node.
 * @param removed Array indicating which nodes have been removed.
 */
static void build_index_remap(uint32_t count, uint32_t *remap, const uint8_t *removed) {
    uint32_t new_position = 0;
    for (uint32_t i = 0; i < count; i++) {
        remap[i] = removed[i] ? SEXP_NULL_INDEX : new_position++;
    }
}

/**
 * @brief Move each surviving node to its compacted position in-place and patch all parent,
 * first_child, and next_sibling links using remap.
 *
 * Removed nodes are skipped.
 *
 * @param tree  Pointer to the tree containing the nodes.
 * @param remap Array mapping original node indices to new compacted indices.
 */
static void compact_nodes(SExp *tree, const uint32_t *remap) {
    for (uint32_t i = 0; i < tree->count; i++) {
        uint32_t new_index = remap[i];

        if (new_index == SEXP_NULL_INDEX) {
            continue;
        }
        Node *node = &tree->nodes[i];

        if (node->parent != SEXP_NULL_INDEX) {
            node->parent = remap[node->parent];
        }

        if (node->first_child != SEXP_NULL_INDEX) {
            node->first_child = remap[node->first_child];
        }

        if (node->next_sibling != SEXP_NULL_INDEX) {
            node->next_sibling = remap[node->next_sibling];
        }
        tree->nodes[new_index] = *node;
    }
}

void sexp_insert(SExp *tree, uint32_t parent, uint32_t after, uint32_t child) {
    if (parent >= tree->count || child >= tree->count) {
        return;
    }

    /* Only list nodes can have children. */
    if (tree->nodes[parent].type != NODE_LIST) {
        return;
    }

    /* Inserting a node as its own child would create a cycle. */
    if (child == parent) {
        return;
    }

    if (after != SEXP_NULL_INDEX) {
        if (after >= tree->count) {
            return;
        }
        /* The 'after' node must be a direct child of parent. */
        if (tree->nodes[after].parent != parent) {
            return;
        }
    }

    /* Detach child from its current parent (no-op if already floating). */
    unlink_from_parent(tree, child);

    tree->nodes[child].parent = parent;

    if (after == SEXP_NULL_INDEX) {
        /* Prepend: push child in front of the current first child. */
        tree->nodes[child].next_sibling = tree->nodes[parent].first_child;
        tree->nodes[parent].first_child = child;
    } else {
        /* Splice: insert child after the given sibling. */
        tree->nodes[child].next_sibling = tree->nodes[after].next_sibling;
        tree->nodes[after].next_sibling = child;
    }
}

void sexp_remove(SExp *tree, uint32_t index) {
    if (index >= tree->count) {
        return;
    }

    /* Sever the subtree root from its parent before modifying the array. */
    unlink_from_parent(tree, index);

    /*
     * The work[] array serves as the BFS queue during collection, then is reused as the old->new
     * index remap table (large enough for both roles).
     */
    uint32_t *work    = malloc(tree->count * sizeof(uint32_t));
    uint8_t  *removed = calloc(tree->count, sizeof(uint8_t));
    if (work == NULL || removed == NULL) {
        free(work);
        free(removed);
        return;
    }

    uint32_t removed_count = collect_removed_bfs(tree, index, work, removed);

    if (removed_count == tree->count) {
        /* Every node was removed — reset the tree to empty without compaction. */
        tree->count = 0;
        free(work);
        free(removed);
        return;
    }

    build_index_remap(tree->count, work, removed);
    free(removed);

    compact_nodes(tree, work);
    tree->count -= removed_count;
    free(work);
}
