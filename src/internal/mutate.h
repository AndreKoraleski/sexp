#pragma once

#include <stdint.h>

#include "core/node.h"

/**
 * @brief Inserts a node as a child of a list node, auto-detaching it first.
 *
 * If after is SEXP_NULL_INDEX the node is inserted as the first child, otherwise it is spliced in
 * immediately after the given sibling. The child is automatically detached from its current parent
 * before insertion, making this a safe move operation.
 *
 * Does nothing if:
 *   - Parent or child are out of bounds,
 *   - Parent is not a NODE_LIST,
 *   - Child == parent (cycle prevention),
 *   - After is not SEXP_NULL_INDEX and is not a direct child of parent.
 *
 * @param tree   Pointer to the tree.
 * @param parent Index of the parent list node.
 * @param after  Index of the direct child to insert after, or SEXP_NULL_INDEX for first.
 * @param child  Index of the node to insert.
 */
void sexp_insert(SExp *tree, uint32_t parent, uint32_t after, uint32_t child);

/**
 * @brief Removes a node and its entire subtree from the tree.
 *
 * All descendants are removed along with the node. Surviving nodes are compacted into the front
 * of the node array and all parent, child, and sibling links are updated. All indices previously
 * obtained from this tree are invalidated after this call.
 *
 * @param tree  Pointer to the tree.
 * @param index Index of the root node to remove.
 */
void sexp_remove(SExp *tree, uint32_t index);
