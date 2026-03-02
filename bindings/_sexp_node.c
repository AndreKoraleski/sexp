#include "_sexp_types.h"

/* --- Tree navigation helpers --- */

Py_ssize_t child_count(const SExp *tree, uint32_t root) {
    Py_ssize_t count = 0;
    uint32_t   child = sexp_first_child(tree, root);
    while (child != SEXP_NULL_INDEX) {
        count++;
        child = sexp_next_sibling(tree, child);
    }
    return count;
}

/**
 * @brief Return the index of the index-th direct child of root, supporting negative indexing.
 *
 * Negative values of index are normalised Python-style (e.g. -1 is the last child). Returns
 * SEXP_NULL_INDEX when index is out of range.
 *
 * @param tree Pointer to the tree.
 * @param root Index of the parent node.
 * @param index Child position. Negative values count from the end.
 * @return     Index of the requested child, or SEXP_NULL_INDEX if out of range.
 */
static uint32_t child_at(const SExp *tree, uint32_t root, Py_ssize_t index) {
    Py_ssize_t total = child_count(tree, root);
    /* Normalise negative indices Python-style before bounds-checking. */
    if (index < 0) {
        index += total;
    }

    if (index < 0 || index >= total) {
        return SEXP_NULL_INDEX;
    }

    uint32_t child = sexp_first_child(tree, root);

    for (Py_ssize_t j = 0; j < index; j++) {
        child = sexp_next_sibling(tree, child);
    }
    return child;
}

/**
 * @brief Find the first direct child of root that is a list whose head atom matches target.
 *
 * Used to implement string-key subscript access (e.g. tree["pos"] finds the first child list
 * starting with the atom "pos").
 *
 * Note: target must already be a valid, interned AtomId. The caller is responsible for
 * interning the key string before calling this function.
 *
 * @param tree   Pointer to the tree.
 * @param root   Index of the node whose children to search.
 * @param target AtomId of the head atom to match.
 * @return       Index of the matching child list, or SEXP_NULL_INDEX if not found.
 */
static uint32_t child_by_atom(const SExp *tree, uint32_t root, AtomId target) {
    uint32_t child = sexp_first_child(tree, root);

    while (child != SEXP_NULL_INDEX) {

        if (sexp_kind(tree, child) == NODE_LIST) {
            uint32_t head = sexp_first_child(tree, child);

            if (head != SEXP_NULL_INDEX && sexp_kind(tree, head) == NODE_ATOM &&
                sexp_atom(tree, head) == target) {
                return child;
            }
        }
        child = sexp_next_sibling(tree, child);
    }
    return SEXP_NULL_INDEX;
}

int subtree_equal(const SExp *a, uint32_t ai, const SExp *b, uint32_t bi) {
    NodeType ta = sexp_kind(a, ai);
    NodeType tb = sexp_kind(b, bi);
    if (ta != tb) {
        return 0;
    }
    if (ta == NODE_ATOM) {
        /* Intern pool is global: matching AtomIds guarantee identical string content. */
        return sexp_atom(a, ai) == sexp_atom(b, bi);
    }
    uint32_t ca = sexp_first_child(a, ai);
    uint32_t cb = sexp_first_child(b, bi);
    while (ca != SEXP_NULL_INDEX && cb != SEXP_NULL_INDEX) {
        if (!subtree_equal(a, ca, b, cb)) {
            return 0;
        }
        ca = sexp_next_sibling(a, ca);
        cb = sexp_next_sibling(b, cb);
    }
    return ca == SEXP_NULL_INDEX && cb == SEXP_NULL_INDEX;
}

int contains_at(const SExpObject *owner, const SExp *tree, uint32_t root, PyObject *item) {
    if (PyUnicode_Check(item)) {
        Py_ssize_t  key_length = 0;
        const char *key_string = PyUnicode_AsUTF8AndSize(item, &key_length);
        if (!key_string) {
            return -1;
        }
        AtomId target = intern_string(key_string, (size_t)key_length);
        if (!target) {
            return 0; /* Not interned -- cannot match any node. */
        }
        uint32_t child = sexp_first_child(tree, root);
        while (child != SEXP_NULL_INDEX) {
            if (sexp_kind(tree, child) == NODE_ATOM && sexp_atom(tree, child) == target) {
                return 1;
            }
            child = sexp_next_sibling(tree, child);
        }
        return 0;
    }
    if (PyObject_TypeCheck(item, &SExpNodeType)) {
        const SExpNodeObject *node = (const SExpNodeObject *)item;
        if (node->owner != owner || node->generation != owner->generation) {
            return 0; /* Different tree or stale handle. */
        }
        uint32_t child = sexp_first_child(tree, root);
        while (child != SEXP_NULL_INDEX) {
            if (child == node->index) {
                return 1;
            }
            child = sexp_next_sibling(tree, child);
        }
        return 0;
    }
    return 0;
}

PyObject *subscript_at(SExpObject *owner, const SExp *tree, uint32_t root, PyObject *key) {
    if (PyLong_Check(key)) {
        Py_ssize_t position = PyLong_AsSsize_t(key);
        if (position == -1 && PyErr_Occurred()) {
            return NULL;
        }

        uint32_t child_index = child_at(tree, root, position);
        if (child_index == SEXP_NULL_INDEX) {
            PyErr_SetString(PyExc_IndexError, "index out of range");
            return NULL;
        }
        return (PyObject *)node_from_index(owner, child_index);
    }

    if (PyUnicode_Check(key)) {
        Py_ssize_t  key_length = 0;
        const char *key_string = PyUnicode_AsUTF8AndSize(key, &key_length);
        if (!key_string) {
            return NULL;
        }
        AtomId target = intern_string(key_string, (size_t)key_length);
        /* intern_string interns the key as a side effect. This is acceptable
         * since the key must exist in the pool to ever match any node. */
        if (!target) {
            PyErr_NoMemory();
            return NULL;
        }

        uint32_t child_index = child_by_atom(tree, root, target);
        if (child_index == SEXP_NULL_INDEX) {
            PyErr_SetObject(PyExc_KeyError, key);
            return NULL;
        }
        return (PyObject *)node_from_index(owner, child_index);
    }

    if (PySlice_Check(key)) {
        Py_ssize_t total = child_count(tree, root);
        Py_ssize_t start, stop, step, slicelen;
        if (PySlice_GetIndicesEx(key, total, &start, &stop, &step, &slicelen) < 0) {
            return NULL;
        }
        PyObject *result = PyList_New(slicelen);
        if (!result) {
            return NULL;
        }
        for (Py_ssize_t i = 0; i < slicelen; i++) {
            Py_ssize_t idx   = start + i * step;
            uint32_t   child = child_at(tree, root, idx);
            PyObject  *node  = (PyObject *)node_from_index(owner, child);
            if (!node) {
                Py_DECREF(result);
                return NULL;
            }
            PyList_SET_ITEM(result, i, node);
        }
        return result;
    }

    PyErr_SetString(PyExc_TypeError, "indices must be int, str, or slice");
    return NULL;
}

/* --- SExpNodeType --- */

static void sexpnode_dealloc(SExpNodeObject *self) {
    Py_DECREF(self->owner);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *sexpnode_repr(SExpNodeObject *self) {
    SEXPNODE_CHECK_VALID(self);
    size_t length     = 0;
    char  *serialized = sexp_serialize_node(&self->owner->tree, self->index, &length);
    if (serialized == NULL) {
        return PyUnicode_FromString("");
    }
    PyObject *result = PyUnicode_FromStringAndSize(serialized, (Py_ssize_t)length);
    free(serialized);
    return result;
}

static Py_ssize_t sexpnode_length(SExpNodeObject *self) {
    SEXPNODE_CHECK_VALID_INT(self);
    if (sexp_kind(&self->owner->tree, self->index) == NODE_ATOM) {
        PyErr_SetString(PyExc_TypeError, "atom nodes have no length");
        return -1;
    }
    return child_count(&self->owner->tree, self->index);
}

static PyObject *sexpnode_subscript(SExpNodeObject *self, PyObject *key) {
    SEXPNODE_CHECK_VALID(self);
    return subscript_at(self->owner, &self->owner->tree, self->index, key);
}

static PyObject *sexpnode_iter(SExpNodeObject *self) {
    SEXPNODE_CHECK_VALID(self);
    uint32_t start = sexp_first_child(&self->owner->tree, self->index);
    return make_iter(&SExpNodeIterType, self->owner, start);
}

static PyObject *sexpnode_head(SExpNodeObject *self, void *Py_UNUSED(closure)) {
    SEXPNODE_CHECK_VALID(self);
    uint32_t child = sexp_first_child(&self->owner->tree, self->index);
    if (child == SEXP_NULL_INDEX) {
        PyErr_SetString(PyExc_IndexError, "node has no children");
        return NULL;
    }
    return (PyObject *)node_from_index(self->owner, child);
}

static PyObject *sexpnode_tail(SExpNodeObject *self, void *Py_UNUSED(closure)) {
    SEXPNODE_CHECK_VALID(self);
    const SExp *tree  = &self->owner->tree;
    uint32_t    first = sexp_first_child(tree, self->index);
    /* Skip the head, yielding children[1:]. */
    uint32_t start = (first != SEXP_NULL_INDEX) ? sexp_next_sibling(tree, first) : SEXP_NULL_INDEX;
    return make_iter(&SExpNodeTailIterType, self->owner, start);
}

static PyObject *sexpnode_value_get(SExpNodeObject *self, void *Py_UNUSED(closure)) {
    SEXPNODE_CHECK_VALID(self);
    const SExp *tree = &self->owner->tree;
    if (sexp_kind(tree, self->index) != NODE_ATOM) {
        PyErr_SetString(PyExc_TypeError, "node is not an atom");
        return NULL;
    }
    size_t      length = 0;
    const char *string = intern_lookup(sexp_atom(tree, self->index), &length);
    if (string == NULL) {
        return PyUnicode_FromString("");
    }
    return PyUnicode_FromStringAndSize(string, (Py_ssize_t)length);
}

static int sexpnode_value_set(SExpNodeObject *self, PyObject *value, void *Py_UNUSED(closure)) {
    SEXPNODE_CHECK_VALID_INT(self);
    if (!PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "value must be a str");
        return -1;
    }
    Py_ssize_t  length = 0;
    const char *string = PyUnicode_AsUTF8AndSize(value, &length);
    if (string == NULL) {
        return -1;
    }
    sexp_set_atom(&self->owner->tree, self->index, string, (size_t)length);
    return 0;
}

static PyObject *sexpnode_parent_get(SExpNodeObject *self, void *Py_UNUSED(closure)) {
    SEXPNODE_CHECK_VALID(self);
    uint32_t parent_index = sexp_parent(&self->owner->tree, self->index);
    if (parent_index == SEXP_NULL_INDEX) {
        Py_RETURN_NONE;
    }
    return (PyObject *)node_from_index(self->owner, parent_index);
}

static PyObject *sexpnode_is_atom_get(SExpNodeObject *self, void *Py_UNUSED(closure)) {
    SEXPNODE_CHECK_VALID(self);
    return PyBool_FromLong(sexp_kind(&self->owner->tree, self->index) == NODE_ATOM);
}

static PyObject *sexpnode_richcompare(SExpNodeObject *self, PyObject *other, int op) {
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (!PyObject_TypeCheck(other, &SExpNodeType)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    SEXPNODE_CHECK_VALID(self);
    SExpNodeObject *other_node = (SExpNodeObject *)other;
    SEXPNODE_CHECK_VALID(other_node);
    int eq =
        subtree_equal(&self->owner->tree, self->index, &other_node->owner->tree, other_node->index);
    return PyBool_FromLong(op == Py_EQ ? eq : !eq);
}

static int sexpnode_contains(SExpNodeObject *self, PyObject *item) {
    if (self->generation != self->owner->generation) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "node is stale: the tree was mutated by remove() or "
            "extract() after this node was obtained; re-query "
            "from the tree or a parent node"
        );
        return -1;
    }
    return contains_at(self->owner, &self->owner->tree, self->index, item);
}

static PyObject *sexpnode_remove(SExpNodeObject *self, PyObject *Py_UNUSED(args)) {
    SEXPNODE_CHECK_VALID(self);
    sexp_remove(&self->owner->tree, self->index);
    SEXP_GENERATION_INC(self->owner->generation);
    Py_RETURN_NONE;
}

static PyObject *sexpnode_clone(SExpNodeObject *self, PyObject *Py_UNUSED(args)) {
    SEXPNODE_CHECK_VALID(self);
    SExp cloned = sexp_clone_node(&self->owner->tree, self->index);
    if (!cloned.valid) {
        PyErr_NoMemory();
        return NULL;
    }
    SExpObject *object = PyObject_New(SExpObject, &SExpType);
    if (!object) {
        sexp_free(&cloned);
        return NULL;
    }
    object->tree       = cloned;
    object->generation = 0;
    return (PyObject *)object;
}

static PyObject *sexpnode_extract(SExpNodeObject *self, PyObject *Py_UNUSED(args)) {
    SEXPNODE_CHECK_VALID(self);
    SExp extracted = sexp_extract_node(&self->owner->tree, self->index);
    if (!extracted.valid) {
        PyErr_NoMemory();
        return NULL;
    }
    SExpObject *object = PyObject_New(SExpObject, &SExpType);
    if (!object) {
        sexp_free(&extracted);
        return NULL;
    }
    object->tree       = extracted;
    object->generation = 0;
    SEXP_GENERATION_INC(self->owner->generation);
    return (PyObject *)object;
}

static PyObject *sexpnode_prepend(SExpNodeObject *self, PyObject *arg) {
    if (!PyObject_TypeCheck(arg, &SExpNodeType)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a SExpNode");
        return NULL;
    }
    const SExpNodeObject *child = (const SExpNodeObject *)arg;
    if (child->owner != self->owner) {
        PyErr_SetString(PyExc_ValueError, "nodes must belong to the same tree");
        return NULL;
    }
    SEXPNODE_CHECK_VALID(self);
    SEXPNODE_CHECK_VALID(child);
    sexp_insert(&self->owner->tree, self->index, SEXP_NULL_INDEX, child->index);
    Py_RETURN_NONE;
}

static PyObject *sexpnode_append(SExpNodeObject *self, PyObject *arg) {
    if (!PyObject_TypeCheck(arg, &SExpNodeType)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a SExpNode");
        return NULL;
    }
    const SExpNodeObject *child = (const SExpNodeObject *)arg;
    if (child->owner != self->owner) {
        PyErr_SetString(PyExc_ValueError, "nodes must belong to the same tree");
        return NULL;
    }
    SEXPNODE_CHECK_VALID(self);
    SEXPNODE_CHECK_VALID(child);
    SExp    *tree  = &self->owner->tree;
    uint32_t after = SEXP_NULL_INDEX;
    /* Walk to the last child to find the append position. */
    uint32_t current_child = sexp_first_child(tree, self->index);
    while (current_child != SEXP_NULL_INDEX) {
        after         = current_child;
        current_child = sexp_next_sibling(tree, current_child);
    }
    sexp_insert(tree, self->index, after, child->index);
    Py_RETURN_NONE;
}

static PyObject *sexpnode_insert_after(SExpNodeObject *self, PyObject *args) {
    PyObject *after_object;
    PyObject *child_object;
    if (!PyArg_ParseTuple(args, "OO", &after_object, &child_object)) {
        return NULL;
    }

    if (!PyObject_TypeCheck(child_object, &SExpNodeType)) {
        PyErr_SetString(PyExc_TypeError, "child must be a SExpNode");
        return NULL;
    }

    const SExpNodeObject *child = (const SExpNodeObject *)child_object;
    if (child->owner != self->owner) {
        PyErr_SetString(PyExc_ValueError, "nodes must belong to the same tree");
        return NULL;
    }
    SEXPNODE_CHECK_VALID(self);
    SEXPNODE_CHECK_VALID(child);
    uint32_t after_index = SEXP_NULL_INDEX;
    if (after_object != Py_None) {
        if (!PyObject_TypeCheck(after_object, &SExpNodeType)) {
            PyErr_SetString(PyExc_TypeError, "after must be a SExpNode or None");
            return NULL;
        }

        const SExpNodeObject *after = (const SExpNodeObject *)after_object;
        if (after->owner != self->owner) {
            PyErr_SetString(PyExc_ValueError, "nodes must belong to the same tree");
            return NULL;
        }
        SEXPNODE_CHECK_VALID(after);
        after_index = after->index;
    }
    /* after_index == SEXP_NULL_INDEX when after_obj is None, which inserts as the first child. */
    sexp_insert(&self->owner->tree, self->index, after_index, child->index);
    Py_RETURN_NONE;
}

/* ---- Tree-allocation helpers on SExpNode ---- */

static PyObject *sexpnode_new_atom(SExpNodeObject *self, PyObject *arg) {
    Py_ssize_t  length = 0;
    const char *string = PyUnicode_AsUTF8AndSize(arg, &length);
    if (!string) {
        return NULL;
    }
    uint32_t index = sexp_allocate_node(&self->owner->tree, NODE_ATOM);
    if (index == SEXP_NULL_INDEX) {
        PyErr_NoMemory();
        return NULL;
    }
    sexp_set_atom(&self->owner->tree, index, string, (size_t)length);
    return (PyObject *)node_from_index(self->owner, index);
}

static PyObject *sexpnode_new_list(SExpNodeObject *self, PyObject *Py_UNUSED(args)) {
    uint32_t index = sexp_allocate_node(&self->owner->tree, NODE_LIST);
    if (index == SEXP_NULL_INDEX) {
        PyErr_NoMemory();
        return NULL;
    }
    return (PyObject *)node_from_index(self->owner, index);
}

static PyGetSetDef sexpnode_getset[] = {
    {"head", (getter)sexpnode_head, NULL, "First child node.", NULL},
    {"tail", (getter)sexpnode_tail, NULL, "Iterator over children after the first.", NULL},
    {"value",
     (getter)sexpnode_value_get,
     (setter)sexpnode_value_set,
     "String value of an atom node.",
     NULL},
    {"parent", (getter)sexpnode_parent_get, NULL, "Parent node, or None if this is the root.", NULL
    },
    {"is_atom",
     (getter)sexpnode_is_atom_get,
     NULL,
     "True if this is an atom node, False if it is a list.",
     NULL},
    {NULL}
};

static PyMethodDef sexpnode_methods[] = {
    {"remove",
     (PyCFunction)sexpnode_remove,
     METH_NOARGS,
     "remove() -> None\n\nRemove this node and its entire subtree from the tree."},
    {"clone",
     (PyCFunction)sexpnode_clone,
     METH_NOARGS,
     "clone() -> SExp\n\n"
     "Deep-copy this subtree into a new independent SExp.\n"
     "The clone owns its own memory; modifying it does not affect this tree."},
    {"extract",
     (PyCFunction)sexpnode_extract,
     METH_NOARGS,
     "extract() -> SExp\n\n"
     "Remove this subtree from the tree and return it as a new independent SExp.\n"
     "Equivalent to clone() followed by remove()."},
    {"append",
     (PyCFunction)sexpnode_append,
     METH_O,
     "append(child) -> None\n\nAppend child as the last child of this list node."},
    {"prepend",
     (PyCFunction)sexpnode_prepend,
     METH_O,
     "prepend(child) -> None\n\nInsert child as the first child of this list node."},
    {"insert_after",
     (PyCFunction)sexpnode_insert_after,
     METH_VARARGS,
     "insert_after(after, child) -> None\n\n"
     "Insert child as a child of this list node immediately after the given sibling.\n"
     "Pass None as after to insert as the first child (equivalent to prepend).\n"
     "The child is automatically detached from its current parent first."},
    {"new_atom",
     (PyCFunction)sexpnode_new_atom,
     METH_O,
     "new_atom(value) -> SExpNode\n\n"
     "Allocate a new unattached atom node in the owning tree."},
    {"new_list",
     (PyCFunction)sexpnode_new_list,
     METH_NOARGS,
     "new_list() -> SExpNode\n\n"
     "Allocate a new unattached list node in the owning tree."},
    {NULL}
};

static PyMappingMethods sexpnode_mapping = {
    .mp_length    = (lenfunc)sexpnode_length,
    .mp_subscript = (binaryfunc)sexpnode_subscript,
};

static PySequenceMethods sexpnode_as_sequence = {
    .sq_contains = (objobjproc)sexpnode_contains,
};

/* --- SExpNode iterator types --- */

PyTypeObject SExpNodeIterType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "sexp.SExpNodeIter",
    .tp_basicsize                          = sizeof(SExpIterObject),
    .tp_dealloc                            = (destructor)sexpiter_dealloc,
    .tp_iter                               = PyObject_SelfIter,
    .tp_iternext                           = (iternextfunc)sexpiter_next,
    .tp_flags                              = Py_TPFLAGS_DEFAULT,
};

PyTypeObject SExpNodeTailIterType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "sexp.SExpNodeTailIter",
    .tp_basicsize                          = sizeof(SExpIterObject),
    .tp_dealloc                            = (destructor)sexpiter_dealloc,
    .tp_iter                               = PyObject_SelfIter,
    .tp_iternext                           = (iternextfunc)sexpiter_next,
    .tp_flags                              = Py_TPFLAGS_DEFAULT,
};

PyTypeObject SExpNodeType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "sexp.SExpNode",
    .tp_basicsize                          = sizeof(SExpNodeObject),
    .tp_dealloc                            = (destructor)sexpnode_dealloc,
    .tp_repr                               = (reprfunc)sexpnode_repr,
    .tp_richcompare                        = (richcmpfunc)sexpnode_richcompare,
    .tp_hash                               = PyObject_HashNotImplemented,
    .tp_as_mapping                         = &sexpnode_mapping,
    .tp_as_sequence                        = &sexpnode_as_sequence,
    .tp_iter                               = (getiterfunc)sexpnode_iter,
    .tp_getset                             = sexpnode_getset,
    .tp_methods                            = sexpnode_methods,
    .tp_flags                              = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Non-owning view of a node within an S-expression tree.",
};
