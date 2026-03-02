#include "_sexp_types.h"

/* --- Shared helpers --- */

SExpNodeObject *node_from_index(SExpObject *owner, uint32_t index) {
    SExpNodeObject *object = PyObject_New(SExpNodeObject, &SExpNodeType);
    if (!object) {
        return NULL;
    }
    Py_INCREF(owner);
    object->owner = owner;
    object->index = index;
    return object;
}

void sexpiter_dealloc(SExpIterObject *self) {
    Py_DECREF(self->owner); /* Release the reference acquired in make_iter. */
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *sexpiter_next(SExpIterObject *self) {
    if (self->next == SEXP_NULL_INDEX) {
        return NULL;
    }

    uint32_t index = self->next;
    /* Advance before returning so the iterator is ready for the next call. */
    self->next = sexp_next_sibling(&self->owner->tree, index);
    return (PyObject *)node_from_index(self->owner, index);
}

PyObject *make_iter(PyTypeObject *type_object, SExpObject *owner, uint32_t start) {
    SExpIterObject *iterator = PyObject_New(SExpIterObject, type_object);
    if (!iterator) {
        return NULL;
    }
    Py_INCREF(owner);
    iterator->owner = owner;
    iterator->next  = start;
    return (PyObject *)iterator;
}

/* --- SExp iterator types --- */

PyTypeObject SExpIterType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "sexp.SExpIter",
    .tp_basicsize                          = sizeof(SExpIterObject),
    .tp_dealloc                            = (destructor)sexpiter_dealloc,
    .tp_flags                              = Py_TPFLAGS_DEFAULT,
    .tp_doc                                = "Iterator over all children of an S-expression node.",
    .tp_iter                               = PyObject_SelfIter,
    .tp_iternext                           = (iternextfunc)sexpiter_next,
};

PyTypeObject SExpTailIterType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "sexp.SExpTailIter",
    .tp_basicsize                          = sizeof(SExpIterObject),
    .tp_dealloc                            = (destructor)sexpiter_dealloc,
    .tp_flags                              = Py_TPFLAGS_DEFAULT,
    .tp_doc                                = "Iterator over children[1:] of an S-expression node.",
    .tp_iter                               = PyObject_SelfIter,
    .tp_iternext                           = (iternextfunc)sexpiter_next,
};
