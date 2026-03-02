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

static PyGetSetDef sexp_getset[] = {
    {"head", (getter)sexp_head, NULL, "First child node, or raises IndexError if empty.", NULL},
    {"tail", (getter)sexp_tail, NULL, "Iterator over children[1:].", NULL},
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
    {NULL}
};

static PyMappingMethods sexp_mapping = {
    .mp_length    = (lenfunc)sexp_length,
    .mp_subscript = (binaryfunc)sexp_subscript,
};

PyTypeObject SExpType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "sexp.SExp",
    .tp_basicsize                          = sizeof(SExpObject),
    .tp_dealloc                            = (destructor)sexp_dealloc,
    .tp_repr                               = (reprfunc)sexp_repr,
    .tp_as_mapping                         = &sexp_mapping,
    .tp_flags                              = Py_TPFLAGS_DEFAULT,
    .tp_doc                                = "Parsed S-expression tree (owns the backing memory).",
    .tp_iter                               = (getiterfunc)sexp_iter,
    .tp_getset                             = sexp_getset,
    .tp_methods                            = sexp_methods,
};
