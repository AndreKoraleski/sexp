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
 * @brief Reason code stored in a failed SExp returned by sexp_parse.
 */
typedef enum SExpParseErrorCode {
    SEXP_PARSE_ERROR_NONE           = 0, /**< No error, or unspecified internal failure. */
    SEXP_PARSE_ERROR_UNCLOSED       = 1, /**< Input has an unclosed parenthesis. */
    SEXP_PARSE_ERROR_STRAY_CLOSE    = 2, /**< Input has a stray closing parenthesis. */
    SEXP_PARSE_ERROR_MULTIPLE_ROOTS = 3, /**< Input contains more than one top-level form. */
} SExpParseErrorCode;

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
    SExpParseErrorCode
        parse_error; /**< Reason for failure; zero on success or unspecified error. */
} SExp;
