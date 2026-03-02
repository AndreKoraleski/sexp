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
} SExpObject;

/**
 * @brief Non-owning view of a single node within an SExpObject's tree.
 */
typedef struct {
    PyObject_HEAD SExpObject *owner;
    uint32_t                  index;
} SExpNodeObject;

/**
 * @brief Iterator state: the owning tree and the next node index to yield.
 */
typedef struct {
    PyObject_HEAD SExpObject *owner;
    uint32_t                  next;
} SExpIterObject;

/* --- Forward declarations --- */

extern PyTypeObject SExpType;
extern PyTypeObject SExpNodeType;
extern PyTypeObject SExpIterType;
extern PyTypeObject SExpTailIterType;
extern PyTypeObject SExpNodeIterType;
extern PyTypeObject SExpNodeTailIterType;

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
