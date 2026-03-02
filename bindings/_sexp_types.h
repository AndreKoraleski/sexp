#pragma once

#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <stdint.h>
#include <stdlib.h>

#include "sexp.h"

/* --- Structs --- */

/**
 * @brief Owning wrapper around a parsed S-expression tree.
 */
typedef struct {
    PyObject_HEAD SExp tree;
    uint32_t           generation; /**< Incremented on every compacting remove or extract. */
} SExpObject;

/**
 * @brief Non-owning view of a single node within an SExpObject's tree.
 */
typedef struct {
    PyObject_HEAD SExpObject *owner;
    uint32_t                  index;
    uint32_t                  generation; /**< Owner generation at time of creation. */
} SExpNodeObject;

/**
 * @brief Iterator state: the owning tree and the next node index to yield.
 */
typedef struct {
    PyObject_HEAD SExpObject *owner;
    uint32_t                  next;
    uint32_t                  generation; /**< Owner generation at time of creation. */
} SExpIterObject;

#define SEXPNODE_CHECK_VALID(node)                                    \
    do {                                                              \
        if ((node)->generation != (node)->owner->generation) {        \
            PyErr_SetString(                                          \
                PyExc_RuntimeError,                                   \
                "node is stale: the tree was mutated by remove() or " \
                "extract() after this node was obtained; re-query "   \
                "from the tree or a parent node"                      \
            );                                                        \
            return NULL;                                              \
        }                                                             \
    } while (0)

/** Variant for functions that return int (setters, tp_as_mapping.mp_length). */
#define SEXPNODE_CHECK_VALID_INT(node)                                \
    do {                                                              \
        if ((node)->generation != (node)->owner->generation) {        \
            PyErr_SetString(                                          \
                PyExc_RuntimeError,                                   \
                "node is stale: the tree was mutated by remove() or " \
                "extract() after this node was obtained; re-query "   \
                "from the tree or a parent node"                      \
            );                                                        \
            return -1;                                                \
        }                                                             \
    } while (0)

#define SEXPITER_CHECK_VALID(iter)                                    \
    do {                                                              \
        if ((iter)->generation != (iter)->owner->generation) {        \
            PyErr_SetString(                                          \
                PyExc_RuntimeError,                                   \
                "iterator is stale: the tree was mutated after this " \
                "iterator was created"                                \
            );                                                        \
            return NULL;                                              \
        }                                                             \
    } while (0)

/* --- Forward declarations --- */

extern PyTypeObject SExpType;
extern PyTypeObject SExpNodeType;
extern PyTypeObject SExpIterType;
extern PyTypeObject SExpTailIterType;
extern PyTypeObject SExpNodeIterType;
extern PyTypeObject SExpNodeTailIterType;
extern PyObject    *SExpParseError;

/**
 * @brief Saturating increment for the generation counter.
 *
 * Stops at UINT32_MAX rather than wrapping to zero. At saturation stale-node
 * detection stops working, but the counter will never silently make a genuinely
 * stale node appear valid again.
 */
#define SEXP_GENERATION_INC(gen) \
    do {                         \
        if ((gen) < UINT32_MAX)  \
            (gen)++;             \
    } while (0)

/* --- Helper prototypes --- */

/**
 * @brief Allocate a new SExpNodeObject referencing index within owner.
 *
 * Increments the owner's reference count so the tree outlives the node view. Returns NULL on
 * allocation failure with a Python exception set.
 *
 * @param owner Owning SExpObject whose tree contains the node.
 * @param index Index of the node within owner->tree.
 * @return      A new SExpNodeObject, or NULL on failure.
 */
SExpNodeObject *node_from_index(SExpObject *owner, uint32_t index);

/**
 * @brief Allocate an iterator of type tp over owner's tree starting at start.
 *
 * Increments the owner's reference count.
 *
 * @param tp    Concrete PyTypeObject for the iterator (SExpIterType or SExpTailIterType etc.).
 * @param owner Owning SExpObject.
 * @param start Index of the first node to yield, or SEXP_NULL_INDEX for an empty iterator.
 * @return      A new iterator object, or NULL on allocation failure.
 */
PyObject *make_iter(PyTypeObject *type_object, SExpObject *owner, uint32_t start);

/**
 * @brief tp_dealloc slot shared by all six iterator types.
 */
void sexpiter_dealloc(SExpIterObject *self);

/**
 * @brief tp_iternext slot shared by all six iterator types.
 */
PyObject *sexpiter_next(SExpIterObject *self);

/**
 * @brief Count the number of direct children of the node at root.
 *
 * @param tree Pointer to the tree.
 * @param root Index of the node whose children to count.
 * @return     Number of direct children.
 */
Py_ssize_t child_count(const SExp *tree, uint32_t root);

/**
 * @brief Dispatch a Python subscript key to the appropriate lookup on the node at root.
 *
 * Integer keys perform positional child lookup (negative indices supported). String keys find
 * the first child list whose head atom matches the key string. Any other key type raises
 * TypeError.
 *
 * @param owner Owning SExpObject, carried into the returned node view.
 * @param tree  Pointer to the tree.
 * @param root  Index of the node to subscript.
 * @param key   Python int or str key.
 * @return      A new SExpNodeObject for the matched child, or NULL with an exception set.
 */
PyObject *subscript_at(SExpObject *owner, const SExp *tree, uint32_t root, PyObject *key);

/**
 * @brief Recursive structural equality for two subtrees.
 *
 * Compares atoms by interned id (global pool ensures same id == same bytes).
 * Compares lists by their children in order.
 */
int subtree_equal(const SExp *a, uint32_t ai, const SExp *b, uint32_t bi);

/**
 * @brief Check whether a Python str (direct atom child) or SExpNode is a direct child of root.
 *
 * String: returns 1 if any direct child atom has that value.
 * SExpNode: returns 1 if the node is a direct child (stale nodes are treated as absent).
 * Returns -1 on error with exception set.
 */
int contains_at(const SExpObject *owner, const SExp *tree, uint32_t root, PyObject *item);
