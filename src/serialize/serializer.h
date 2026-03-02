#pragma once

#include <stddef.h>

#include "core/node.h"

/**
 * @brief Serializes the entire tree to S-expression bytes.
 *
 * All top-level nodes (those with no parent) are emitted in array order, separated by spaces.
 * The returned buffer is heap-allocated, call free() on it when done.
 *
 * @param tree          Pointer to the tree.
 * @param output_length Output parameter for the length of the result in bytes.
 * @return              Heap-allocated serialized bytes (caller must free), or NULL on failure.
 */
char *sexp_serialize(const SExp *tree, size_t *output_length);

/**
 * @brief Serializes a single node (and its subtree) to S-expression bytes.
 *
 * The returned buffer is heap-allocated, call free() on it when done.
 *
 * @param tree          Pointer to the tree.
 * @param index         Index of the root node to serialize.
 * @param output_length Output parameter for the length of the result in bytes.
 * @return              Heap-allocated serialized bytes (caller must free), or NULL on failure.
 */
char *sexp_serialize_node(const SExp *tree, uint32_t index, size_t *output_length);
