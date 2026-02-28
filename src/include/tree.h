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
    NODE_ATOM, /**< Leaf node carrying an interned atom string. */
    NODE_LIST, /**< Interior node carrying a list of child nodes. */
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
} Node;

/**
 * A parsed S-expression tree.
 *
 * Nodes are stored in a flat array indexed from zero. The root node is always 
 * at index zero. Node memory is managed by an internal Arena. The intern pool 
 * is shared and reference counted - the tree increments the count on creation 
 * and decrements on free.
 */
typedef struct SExp {
    Arena       arena;  /**< Backing memory for the node array. */
    Node   *nodes;  /**< Flat array of all nodes in the tree. */
    uint32_t    count;  /**< Number of nodes currently in the tree. */
    uint32_t    cap;    /**< Capacity of the node array in nodes. */
    InternPool *intern; /**< Shared intern pool, reference counted. */
} SExp;