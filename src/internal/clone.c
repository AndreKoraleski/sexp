#include <stdlib.h>

#include "core/tree.h"
#include "memory/intern.h"

/**
 * @brief BFS-traverse the subtree rooted at root, storing the original indices in BFS order in
 * queue, and mapping each original index to its BFS-order position in remap.
 *
 * Returns the subtree size.
 *
 * @param source    Pointer to the source tree containing the nodes.
 * @param root      Index of the root of the subtree to traverse.
 * @param queue     Array to store original node indices in BFS order. Must have capacity for at
 *                  least source->count elements.
 * @param remap     Array mapping original node indices to new indices in the destination array.
 * @return uint32_t Subtree size, i.e. number of nodes in the subtree rooted at root.
 */
static uint32_t
bfs_subtree_remap(const SExp *source, uint32_t root, uint32_t *queue, uint32_t *remap) {
    for (uint32_t i = 0; i < source->count; i++) {
        remap[i] = SEXP_NULL_INDEX;
    }

    uint32_t head = 0;
    uint32_t tail = 0;
    remap[root]   = tail;
    queue[tail++] = root;

    while (head < tail) {
        uint32_t current = queue[head++];
        uint32_t child   = source->nodes[current].first_child;
        while (child != SEXP_NULL_INDEX) {
            remap[child]  = tail;
            queue[tail++] = child;
            child         = source->nodes[child].next_sibling;
        }
    }
    return tail;
}

/**
 * @brief Copy count nodes from source (indexed via queue) into destination, rewriting
 * all parent, first_child, and next_sibling links through remap so they are valid in the new
 * array.
 *
 * The clone root at index 0 has its parent and next_sibling cleared.
 *
 * @param source      Pointer to the source tree containing the nodes to copy.
 * @param destination Pointer to the destination node array to copy into.
 * @param queue       Array of original node indices in BFS order, as produced by bfs_subtree_remap.
 * @param remap       Array mapping original node indices to new indices in the destination array.
 * @param count       Number of nodes to copy.
 */
static void copy_nodes_remapped(
    const SExp     *source,
    Node           *destination,
    const uint32_t *queue,
    const uint32_t *remap,
    uint32_t        count
) {
    for (uint32_t i = 0; i < count; i++) {
        Node node   = source->nodes[queue[i]];
        node.parent = (node.parent != SEXP_NULL_INDEX) ? remap[node.parent] : SEXP_NULL_INDEX;
        node.first_child =
            (node.first_child != SEXP_NULL_INDEX) ? remap[node.first_child] : SEXP_NULL_INDEX;
        node.next_sibling =
            (node.next_sibling != SEXP_NULL_INDEX) ? remap[node.next_sibling] : SEXP_NULL_INDEX;
        destination[i] = node;
    }
    /* Root of the clone is a top-level node - no parent or sibling above it. */
    destination[0].parent       = SEXP_NULL_INDEX;
    destination[0].next_sibling = SEXP_NULL_INDEX;
}

SExp sexp_clone_node(const SExp *source, uint32_t index) {
    SExp destination = {0};
    if (source == NULL || !source->valid || index >= source->count) {
        return destination;
    }

    uint32_t *queue = malloc(source->count * sizeof(uint32_t));
    uint32_t *remap = malloc(source->count * sizeof(uint32_t));
    if (queue == NULL || remap == NULL) {
        free(queue);
        free(remap);
        return destination;
    }

    uint32_t count = bfs_subtree_remap(source, index, queue, remap);
    Node    *nodes = malloc(count * sizeof(Node));
    if (nodes == NULL) {
        free(queue);
        free(remap);
        return destination;
    }

    copy_nodes_remapped(source, nodes, queue, remap, count);
    free(queue);
    free(remap);

    /* The clone owns its own node array and shares the intern pool via a retained reference. */
    intern_retain();
    destination.nodes    = nodes;
    destination.count    = count;
    destination.capacity = count;
    destination.valid    = 1;
    return destination;
}

SExp sexp_extract_node(SExp *source, uint32_t index) {
    SExp destination = sexp_clone_node(source, index);
    if (destination.valid) {
        /* Clone succeeded - remove the original from the source tree. */
        sexp_remove(source, index);
    }
    return destination;
}
