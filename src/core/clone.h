#pragma once

#include <stdint.h>

#include "core/node.h"

/**
 * @brief Deep-copies a subtree into a new independent SExp.
 *
 * All descendants of index are included. The new tree shares the same intern pool (reference count
 * is incremented) but owns its own node array. The root of the clone has no parent or sibling.
 * The returned tree must be freed with sexp_free.
 *
 * @param source Pointer to the source tree.
 * @param index  Index of the root node to copy.
 * @return       A new SExp containing the copied subtree, or a zeroed struct on failure.
 */
SExp sexp_clone_node(const SExp *source, uint32_t index);

/**
 * @brief Removes a subtree from the tree and returns it as a new independent SExp.
 *
 * Clones the subtree first, then removes the original. If cloning fails the source tree is left
 * unchanged. The returned tree must be freed with sexp_free.
 *
 * @param source Pointer to the source tree.
 * @param index  Index of the root node to extract.
 * @return       A new SExp containing the extracted subtree, or a zeroed struct on failure.
 */
SExp sexp_extract_node(SExp *source, uint32_t index);
