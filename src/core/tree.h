#pragma once

#include <stdint.h>

#include "core/node.h"

/**
 * @brief Parses an S-expression from a buffer, returning a new tree.
 *
 * Retains a reference to the global intern pool automatically.
 *
 * @param source        Pointer to the input buffer.
 * @param source_length Length of the input buffer in bytes.
 * @return              A parsed SExp, or a zeroed struct on failure.
 */
SExp sexp_parse(const char *source, size_t source_length);

/**
 * @brief Releases all memory owned by the tree.
 *
 * Decrements the intern pool reference count. Must be called exactly once for every SExp
 * returned by sexp_parse, sexp_clone_node, or sexp_extract_node to avoid leaking pool memory.
 *
 * @param tree Pointer to the tree to release.
 */
void sexp_free(SExp *tree);

/**
 * @brief Returns the index of the first child of a list node.
 *
 * @param tree  Pointer to the tree.
 * @param index Index of the list node.
 * @return      Index of the first child, or SEXP_NULL_INDEX if none or index is out of bounds.
 */
uint32_t sexp_first_child(const SExp *tree, uint32_t index);

/**
 * @brief Returns the index of the next sibling of a node.
 *
 * @param tree  Pointer to the tree.
 * @param index Index of the node.
 * @return      Index of the next sibling, or SEXP_NULL_INDEX if none or index is out of bounds.
 */
uint32_t sexp_next_sibling(const SExp *tree, uint32_t index);

/**
 * @brief Returns the index of the parent of a node.
 *
 * @param tree  Pointer to the tree.
 * @param index Index of the node.
 * @return      Index of the parent, or SEXP_NULL_INDEX if root or index is out of bounds.
 */
uint32_t sexp_parent(const SExp *tree, uint32_t index);

/**
 * @brief Returns the type of a node.
 *
 * @param tree  Pointer to the tree.
 * @param index Index of the node.
 * @return      NODE_ATOM, NODE_LIST, or NODE_INVALID if index is out of bounds.
 */
NodeType sexp_kind(const SExp *tree, uint32_t index);

/**
 * @brief Returns the AtomId of an atom node.
 *
 * @param tree  Pointer to the tree.
 * @param index Index of the atom node.
 * @return      AtomId for the node, or 0 if index is out of bounds or node is not NODE_ATOM.
 */
AtomId sexp_atom(const SExp *tree, uint32_t index);

/**
 * @brief Sets the atom value of a leaf node.
 *
 * The new value is interned automatically. Does nothing if index is out of bounds or the node is
 * not NODE_ATOM. If interning fails the node is left unchanged.
 *
 * @param tree   Pointer to the tree.
 * @param index  Index of the atom node.
 * @param string Pointer to the new string bytes.
 * @param length Length of the new string in bytes.
 */
void sexp_set_atom(SExp *tree, uint32_t index, const char *string, size_t length);

/**
 * @brief Allocates a new unattached node within the tree.
 *
 * The node is appended to the internal node array but has no parent, children, or siblings. Use
 * sexp_set_atom to set its content, then sexp_insert to attach it to the tree.
 *
 * @param tree Pointer to the tree.
 * @param type NODE_ATOM or NODE_LIST.
 * @return     Index for a new node, or SEXP_NULL_INDEX on failure.
 */
uint32_t sexp_allocate_node(SExp *tree, NodeType type);

/**
 * @brief Inserts a node as a child of a list node, auto-detaching it first.
 *
 * If after is SEXP_NULL_INDEX the node is inserted as the first child, otherwise it is spliced in
 * immediately after the given sibling. The child is automatically detached from its current parent
 * before insertion, making this a safe move operation.
 *
 * Does nothing if:
 *   - parent or child are out of bounds,
 *   - parent is not a NODE_LIST,
 *   - child == parent (cycle prevention),
 *   - after is not SEXP_NULL_INDEX and is not a direct child of parent.
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
