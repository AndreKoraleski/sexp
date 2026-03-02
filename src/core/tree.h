#pragma once

#include <stdint.h>

#include "core/node.h"

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
