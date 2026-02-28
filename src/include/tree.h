#pragma once

#include <stdint.h>

#include "intern.h"

/**
 * Sentinel index representing the absence of a node.
 */
#define SEXP_NULL_INDEX ((uint32_t)UINT32_MAX)

/**
 * Discriminant for a node in an S-expression tree.
 */
typedef enum NodeKind {
    NODE_ATOM,    /**< Leaf node carrying an interned atom string. */
    NODE_LIST,    /**< Interior node carrying a list of child nodes. */
    NODE_INVALID, /**< Sentinel returned for out-of-bounds or invalid indices. */
} NodeKind;

/**
 * A single node in an S-expression tree.
 *
 * Atom nodes carry an AtomId referencing interned string content. List nodes 
 * carry connectivity via the left-child right-sibling representation, allowing
 * arbitrary-arity trees with fixed node size. Absent children or siblings are 
 * indicated by SEXP_NULL_INDEX.
 */
typedef struct Node {
    NodeKind type;         /**< Discriminant for this node. */
    AtomId   atom_id;      /**< Interned atom id, valid only for NODE_ATOM. */
    uint32_t first_child;  /**< Index of first child, or SEXP_NULL_INDEX. */
    uint32_t next_sibling; /**< Index of next sibling, or SEXP_NULL_INDEX. */
    uint32_t parent;       /**< Index of parent node, or SEXP_NULL_INDEX. */
} Node;

/**
 * A parsed S-expression tree.
 *
 * Nodes are stored in a flat array indexed from zero. Node memory is managed 
 * by an internal Arena. The global intern pool is retained on creation and 
 * released on free.
 */
typedef struct SExp {
    Arena    arena; /**< Backing memory for the node array. */
    Node    *nodes; /**< Flat array of all nodes in the tree. */
    uint32_t count; /**< Number of nodes currently in the tree. */
    uint32_t cap;   /**< Capacity of the node array in nodes. */
} SExp;

/**
 * Parses an S-expression from a buffer, returning a new tree.
 *
 * Retains a reference to the global intern pool automatically.
 *
 * @param src      Pointer to the input buffer.
 * @param src_len  Length of the input buffer in bytes.
 * @return         A parsed SExp, or a zeroed struct on failure.
 */
SExp sexp_parse(const char *src, size_t src_len);

/**
 * Releases all memory owned by the tree.
 *
 * Decrements the intern pool reference count. Should be called automatically 
 * by the Python layer - not intended for direct use.
 *
 * @param tree  Pointer to the tree to release.
 */
void sexp_free(SExp *tree);

/**
 * Returns the index of the first child of a list node.
 *
 * @param tree  Pointer to the tree.
 * @param idx   Index of the list node.
 * @return      Index of the first child, or SEXP_NULL_INDEX if none.
 */
uint32_t sexp_first_child(const SExp *tree, uint32_t idx);

/**
 * Returns the index of the next sibling of a node.
 *
 * @param tree  Pointer to the tree.
 * @param idx   Index of the node.
 * @return      Index of the next sibling, or SEXP_NULL_INDEX if none.
 */
uint32_t sexp_next_sibling(const SExp *tree, uint32_t idx);

/**
 * Returns the index of the parent of a node.
 *
 * @param tree  Pointer to the tree.
 * @param idx   Index of the node.
 * @return      Index of the parent, or SEXP_NULL_INDEX if root.
 */
uint32_t sexp_parent(const SExp *tree, uint32_t idx);

/**
 * Returns the kind of a node.
 *
 * @param tree  Pointer to the tree.
 * @param idx   Index of the node.
 * @return      NODE_ATOM, NODE_LIST, or NODE_INVALID if idx is out of bounds.
 */
NodeKind sexp_kind(const SExp *tree, uint32_t idx);

/**
 * Returns the AtomId of an atom node.
 *
 * @param tree  Pointer to the tree.
 * @param idx   Index of the atom node.
 * @return      AtomId for the node, or 0 if node is not NODE_ATOM.
 */
AtomId sexp_atom(const SExp *tree, uint32_t idx);

/**
 * Sets the atom value of a leaf node.
 *
 * The new value is interned automatically.
 *
 * @param tree  Pointer to the tree.
 * @param idx   Index of the atom node.
 * @param str   Pointer to the new string bytes.
 * @param len   Length of the new string in bytes.
 */
void sexp_set_atom(SExp *tree, uint32_t idx, const char *str, size_t len);

/**
 * Allocates a new unattached node within the tree.
 *
 * The node is appended to the internal node array but has no parent, children,
 * or siblings. Use sexp_set_atom to set its content, then sexp_insert to 
 * attach it to the tree.
 *
 * @param tree  Pointer to the tree.
 * @param kind  NODE_ATOM or NODE_LIST.
 * @return      Index for a new node, or SEXP_NULL_INDEX on allocation failure.
 */
uint32_t sexp_node_alloc(SExp *tree, NodeKind kind);

/**
 *
 * If after is SEXP_NULL_INDEX the node is inserted as the first child. All 
 * parent and sibling links are updated automatically.
 *
 * @param tree    Pointer to the tree.
 * @param parent  Index of the parent list node.
 * @param after   Index of the sibling to insert after, or SEXP_NULL_INDEX.
 * @param child   Index of the node to insert.
 */
void sexp_insert(SExp *tree, uint32_t parent, uint32_t after, uint32_t child);

/**
 * Removes a node from the tree.
 *
 * The vacated slot is filled by swapping with the last node. All affected 
 * links are updated automatically.
 *
 * @param tree  Pointer to the tree.
 * @param idx   Index of the node to remove.
 */
void sexp_remove(SExp *tree, uint32_t idx);

/**
 * Serializes the tree to S-expression bytes.
 *
 * The returned pointer is valid for the lifetime of the tree and is 
 * invalidated by any mutation. No allocation or free is required by the 
 * caller.
 *
 * @param tree  Pointer to the tree.
 * @param len   Output parameter for the length of the result in bytes.
 * @return      Pointer to the serialized bytes, or NULL on failure.
 */
const char *sexp_serialize(const SExp *tree, size_t *len);