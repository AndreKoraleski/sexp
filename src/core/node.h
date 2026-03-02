#pragma once

#include <stdint.h>

#include "memory/intern.h"

/** Sentinel index representing the absence of a node. */
#define SEXP_NULL_INDEX ((uint32_t)UINT32_MAX)

/**
 * @brief Discriminant for a node in an S-expression tree.
 */
typedef enum NodeType {
    NODE_ATOM,    /**< Leaf node carrying an interned atom string. */
    NODE_LIST,    /**< Interior node carrying a list of child nodes. */
    NODE_INVALID, /**< Sentinel returned for out-of-bounds or invalid indices. */
} NodeType;

/**
 * @brief A single node in an S-expression tree.
 *
 * Atom nodes carry an AtomId referencing interned string content. List nodes carry connectivity via
 * the left-child right-sibling representation, allowing arbitrary-arity trees with fixed node size.
 *
 * Absent children or siblings are indicated by SEXP_NULL_INDEX.
 */
typedef struct Node {
    NodeType type;         /**< Discriminant for this node. */
    AtomId   atom_id;      /**< Interned atom id, valid only for NODE_ATOM. */
    uint32_t first_child;  /**< Index of first child, or SEXP_NULL_INDEX. */
    uint32_t next_sibling; /**< Index of next sibling, or SEXP_NULL_INDEX. */
    uint32_t parent;       /**< Index of parent node, or SEXP_NULL_INDEX. */
} Node;

/**
 * @brief A parsed S-expression tree.
 *
 * Nodes are stored in a flat array indexed from zero. Node memory is heap-allocated via realloc.
 * The global intern pool is retained on creation and released on free.
 */
typedef struct SExp {
    Node    *nodes;    /**< Flat array of all nodes, heap-allocated. */
    uint32_t count;    /**< Number of nodes currently in the tree. */
    uint32_t capacity; /**< Allocated capacity of the node array in elements. */
    uint8_t  valid;    /**< Non-zero if the tree was successfully parsed. */
} SExp;

/**
 * @brief Allocates a new blank node, growing the node array if needed.
 *
 * Internal function used by the parser and public node allocation API. The returned node is
 * initialised to NODE_ATOM with all links set to SEXP_NULL_INDEX.
 *
 * @param tree  Pointer to the target tree.
 * @return      Index of the new node, or SEXP_NULL_INDEX on allocation failure.
 */
uint32_t allocate_node(SExp *tree);
