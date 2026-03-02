#include "_sexp_types.h"

/* --- SExpType --- */

static void sexp_dealloc(SExpObject *self) {
    sexp_free(&self->tree);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *sexp_repr(SExpObject *self) {
    if (self->tree.count == 0) {
        return PyUnicode_FromString("");
    }
    size_t length     = 0;
    char  *serialized = sexp_serialize(&self->tree, &length);
    if (serialized == NULL) {
        return PyUnicode_FromString("");
    }
    PyObject *result = PyUnicode_FromStringAndSize(serialized, (Py_ssize_t)length);
    free(serialized);
    return result;
}

static Py_ssize_t sexp_length(SExpObject *self) {
    if (self->tree.count == 0) {
        return 0;
    }
    return child_count(&self->tree, 0);
}

static PyObject *sexp_subscript(SExpObject *self, PyObject *key) {
    if (self->tree.count == 0) {
        PyErr_SetString(PyExc_IndexError, "S-expression is empty");
        return NULL;
    }
    return subscript_at(self, &self->tree, 0, key);
}

static PyObject *sexp_iter(SExpObject *self) {
    uint32_t start = (self->tree.count > 0) ? sexp_first_child(&self->tree, 0) : SEXP_NULL_INDEX;
    return make_iter(&SExpIterType, self, start);
}

static PyObject *sexp_head(SExpObject *self, void *Py_UNUSED(ignored)) {
    if (self->tree.count == 0) {
        PyErr_SetString(PyExc_IndexError, "S-expression is empty");
        return NULL;
    }
    uint32_t first_child = sexp_first_child(&self->tree, 0);
    if (first_child == SEXP_NULL_INDEX) {
        PyErr_SetString(PyExc_IndexError, "S-expression has no children");
        return NULL;
    }
    return (PyObject *)node_from_index(self, first_child);
}

static PyObject *sexp_tail(SExpObject *self, void *Py_UNUSED(ignored)) {
    uint32_t first = (self->tree.count > 0) ? sexp_first_child(&self->tree, 0) : SEXP_NULL_INDEX;
    /* Skip the head, yielding children[1:]. */
    uint32_t start =
        (first != SEXP_NULL_INDEX) ? sexp_next_sibling(&self->tree, first) : SEXP_NULL_INDEX;
    return make_iter(&SExpTailIterType, self, start);
}

static PyObject *sexp_new_atom(SExpObject *self, PyObject *arg) {
    Py_ssize_t  length = 0;
    const char *string = PyUnicode_AsUTF8AndSize(arg, &length);
    if (!string) {
        return NULL;
    }
    uint32_t index = sexp_allocate_node(&self->tree, NODE_ATOM);
    if (index == SEXP_NULL_INDEX) {
        PyErr_NoMemory();
        return NULL;
    }
    sexp_set_atom(&self->tree, index, string, (size_t)length);
    return (PyObject *)node_from_index(self, index);
}

static PyObject *sexp_new_list(SExpObject *self, PyObject *Py_UNUSED(args)) {
    uint32_t index = sexp_allocate_node(&self->tree, NODE_LIST);
    if (index == SEXP_NULL_INDEX) {
        PyErr_NoMemory();
        return NULL;
    }
    return (PyObject *)node_from_index(self, index);
}

/* ---- Node-like interface on SExp ---- */

static PyObject *sexp_is_atom(SExpObject *Py_UNUSED(self), void *Py_UNUSED(closure)) {
    /* The root is always a list container. */
    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject *sexp_value_get(SExpObject *Py_UNUSED(self), void *Py_UNUSED(closure)) {
    PyErr_SetString(PyExc_TypeError, "SExp root is a list; only atom nodes have a value");
    return NULL;
}

static PyObject *sexp_parent_get(SExpObject *Py_UNUSED(self), void *Py_UNUSED(closure)) {
    /* The root has no parent. */
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *sexp_append(SExpObject *self, PyObject *arg) {
    if (!PyObject_TypeCheck(arg, &SExpNodeType)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a SExpNode");
        return NULL;
    }
    const SExpNodeObject *child = (const SExpNodeObject *)arg;
    if (child->owner != self) {
        PyErr_SetString(PyExc_ValueError, "node must belong to this tree");
        return NULL;
    }
    SExp    *tree  = &self->tree;
    uint32_t after = SEXP_NULL_INDEX;
    uint32_t cur   = sexp_first_child(tree, 0);
    while (cur != SEXP_NULL_INDEX) {
        after = cur;
        cur   = sexp_next_sibling(tree, cur);
    }
    sexp_insert(tree, 0, after, child->index);
    Py_RETURN_NONE;
}

static PyObject *sexp_prepend(SExpObject *self, PyObject *arg) {
    if (!PyObject_TypeCheck(arg, &SExpNodeType)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a SExpNode");
        return NULL;
    }
    const SExpNodeObject *child = (const SExpNodeObject *)arg;
    if (child->owner != self) {
        PyErr_SetString(PyExc_ValueError, "node must belong to this tree");
        return NULL;
    }
    sexp_insert(&self->tree, 0, SEXP_NULL_INDEX, child->index);
    Py_RETURN_NONE;
}

static PyObject *sexp_insert_after(SExpObject *self, PyObject *args) {
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
    if (child->owner != self) {
        PyErr_SetString(PyExc_ValueError, "node must belong to this tree");
        return NULL;
    }
    uint32_t after_index = SEXP_NULL_INDEX;
    if (after_object != Py_None) {
        if (!PyObject_TypeCheck(after_object, &SExpNodeType)) {
            PyErr_SetString(PyExc_TypeError, "after must be a SExpNode or None");
            return NULL;
        }
        const SExpNodeObject *after = (const SExpNodeObject *)after_object;
        if (after->owner != self) {
            PyErr_SetString(PyExc_ValueError, "node must belong to this tree");
            return NULL;
        }
        after_index = after->index;
    }
    sexp_insert(&self->tree, 0, after_index, child->index);
    Py_RETURN_NONE;
}

static PyObject *sexp_clone_method(SExpObject *self, PyObject *Py_UNUSED(args)) {
    if (self->tree.count == 0) {
        /* Clone of an empty tree is another empty tree. */
        SExpObject *object = PyObject_New(SExpObject, &SExpType);
        if (!object) {
            return NULL;
        }
        object->tree       = (SExp){0};
        object->generation = 0;
        return (PyObject *)object;
    }
    SExp cloned = sexp_clone_node(&self->tree, 0);
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

static PyObject *sexp_richcompare(SExpObject *self, PyObject *other, int op) {
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (!PyObject_TypeCheck(other, &SExpType)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    SExpObject *other_tree = (SExpObject *)other;
    int         equal;
    if (self->tree.count == 0 && other_tree->tree.count == 0) {
        equal = 1;
    } else if (self->tree.count == 0 || other_tree->tree.count == 0) {
        equal = 0;
    } else {
        equal = subtree_equal(&self->tree, 0, &other_tree->tree, 0);
    }
    return PyBool_FromLong(op == Py_EQ ? equal : !equal);
}

static int sexp_contains(SExpObject *self, PyObject *item) {
    if (self->tree.count == 0) {
        return 0;
    }
    return contains_at((const SExpObject *)self, &self->tree, 0, item);
}

/**
 * @brief tp_new for SExp: create an empty tree equivalent to parse("()").
 *
 * Initialises the intern pool, retains a reference, and allocates the root
 * list node so that new_atom() / new_list() / append() / prepend() all work
 * immediately on the returned object.
 */
static PyObject *
sexp_tp_new(PyTypeObject *type, PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwargs)) {
    if (intern_init() != 0) {
        PyErr_SetString(PyExc_RuntimeError, "sexp: intern_init() failed");
        return NULL;
    }
    intern_retain();

    SExpObject *self = (SExpObject *)type->tp_alloc(type, 0);
    if (!self) {
        intern_release();
        return NULL;
    }
    /* tp_alloc zero-initialises the struct, so tree and generation start at 0. */

    uint32_t root = sexp_allocate_node(&self->tree, NODE_LIST);
    if (root == SEXP_NULL_INDEX) {
        intern_release();
        Py_DECREF(self);
        PyErr_NoMemory();
        return NULL;
    }
    self->tree.valid = 1;
    return (PyObject *)self;
}

static PyGetSetDef sexp_getset[] = {
    {"head", (getter)sexp_head, NULL, "First child node, or raises IndexError if empty.", NULL},
    {"tail", (getter)sexp_tail, NULL, "Iterator over children[1:].", NULL},
    {"is_atom", (getter)sexp_is_atom, NULL, "Always False: the tree root is a list.", NULL},
    {"value",
     (getter)sexp_value_get,
     NULL,
     "Always raises TypeError: the root is not an atom.",
     NULL},
    {"parent", (getter)sexp_parent_get, NULL, "Always None: the root has no parent.", NULL},
    {NULL}
};

static PyMethodDef sexp_methods[] = {
    {"new_atom",
     (PyCFunction)sexp_new_atom,
     METH_O,
     "new_atom(value) -> SExpNode\n\nAllocate a new unattached atom node."},
    {"new_list",
     (PyCFunction)sexp_new_list,
     METH_NOARGS,
     "new_list() -> SExpNode\n\nAllocate a new unattached list node."},
    {"append",
     (PyCFunction)sexp_append,
     METH_O,
     "append(child) -> None\n\nAppend child as the last top-level node."},
    {"prepend",
     (PyCFunction)sexp_prepend,
     METH_O,
     "prepend(child) -> None\n\nInsert child as the first top-level node."},
    {"insert_after",
     (PyCFunction)sexp_insert_after,
     METH_VARARGS,
     "insert_after(after, child) -> None\n\n"
     "Insert child after the given top-level sibling. Pass None to prepend."},
    {"clone",
     (PyCFunction)sexp_clone_method,
     METH_NOARGS,
     "clone() -> SExp\n\nDeep-copy this tree into a new independent SExp."},
    {NULL}
};

static PyMappingMethods sexp_mapping = {
    .mp_length    = (lenfunc)sexp_length,
    .mp_subscript = (binaryfunc)sexp_subscript,
};

static PySequenceMethods sexp_as_sequence = {
    .sq_contains = (objobjproc)sexp_contains,
};

PyTypeObject SExpType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "sexp.SExp",
    .tp_basicsize                          = sizeof(SExpObject),
    .tp_new                                = sexp_tp_new,
    .tp_dealloc                            = (destructor)sexp_dealloc,
    .tp_repr                               = (reprfunc)sexp_repr,
    .tp_richcompare                        = (richcmpfunc)sexp_richcompare,
    .tp_hash                               = PyObject_HashNotImplemented,
    .tp_as_mapping                         = &sexp_mapping,
    .tp_as_sequence                        = &sexp_as_sequence,
    .tp_flags                              = Py_TPFLAGS_DEFAULT,
    .tp_doc                                = "Parsed S-expression tree (owns the backing memory).",
    .tp_iter                               = (getiterfunc)sexp_iter,
    .tp_getset                             = sexp_getset,
    .tp_methods                            = sexp_methods,
};
