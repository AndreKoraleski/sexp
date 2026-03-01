#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <stdint.h>

#include "sexp.h"

static PyTypeObject SExpType;
static PyTypeObject SExpNodeType;
static PyTypeObject SExpIterType;
static PyTypeObject SExpTailIterType;
static PyTypeObject SExpNodeIterType;
static PyTypeObject SExpNodeTailIterType;

/* ════════════════════════════════════════════════════════════════
   Structs
   ════════════════════════════════════════════════════════════════ */

typedef struct {
    PyObject_HEAD
    SExp tree;
} SExpObject;

typedef struct {
    PyObject_HEAD
    SExpObject *owner;
    uint32_t    idx;
} SExpNodeObject;

typedef struct {
    PyObject_HEAD
    SExpObject *owner;
    uint32_t    next;
} SExpIterObject;

/* ════════════════════════════════════════════════════════════════
    Iterators
   ════════════════════════════════════════════════════════════════ */

static SExpNodeObject *
node_from_idx(SExpObject *owner, uint32_t idx)
{
    SExpNodeObject *obj = PyObject_New(SExpNodeObject, &SExpNodeType);
    if (!obj)
        return NULL;
    Py_INCREF(owner);
    obj->owner = owner;
    obj->idx   = idx;
    return obj;
}


static void
sexpiter_dealloc(SExpIterObject *self)
{
    Py_DECREF(self->owner);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
sexpiter_next(SExpIterObject *self)
{
    if (self->next == SEXP_NULL_INDEX)
        return NULL;

    uint32_t idx = self->next;
    self->next = sexp_next_sibling(&self->owner->tree, idx);
    return (PyObject *)node_from_idx(self->owner, idx);
}

static PyTypeObject SExpIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "sexp.SExpIter",
    .tp_basicsize = sizeof(SExpIterObject),
    .tp_dealloc   = (destructor)sexpiter_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT,
    .tp_doc       = "Iterator over all children of an S-expression node.",
    .tp_iter      = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)sexpiter_next,
};

static PyTypeObject SExpTailIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "sexp.SExpTailIter",
    .tp_basicsize = sizeof(SExpIterObject),
    .tp_dealloc   = (destructor)sexpiter_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT,
    .tp_doc       = "Iterator over children[1:] of an S-expression node.",
    .tp_iter      = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)sexpiter_next,
};

static PyObject *
make_iter(PyTypeObject *tp, SExpObject *owner, uint32_t start)
{
    SExpIterObject *it = PyObject_New(SExpIterObject, tp);
    if (!it)
        return NULL;
    Py_INCREF(owner);
    it->owner = owner;
    it->next  = start;
    return (PyObject *)it;
}

/* ════════════════════════════════════════════════════════════════
   Tree navigation helpers
   ════════════════════════════════════════════════════════════════ */

static Py_ssize_t
child_count(const SExp *tree, uint32_t root)
{
    Py_ssize_t n = 0;
    uint32_t c = sexp_first_child(tree, root);
    while (c != SEXP_NULL_INDEX) {
        n++;
        c = sexp_next_sibling(tree, c);
    }
    return n;
}

static uint32_t
child_at(const SExp *tree, uint32_t root, Py_ssize_t i)
{
    Py_ssize_t total = child_count(tree, root);
    if (i < 0)
        i += total;
    if (i < 0 || i >= total)
        return SEXP_NULL_INDEX;
    uint32_t c = sexp_first_child(tree, root);
    for (Py_ssize_t j = 0; j < i; j++)
        c = sexp_next_sibling(tree, c);
    return c;
}

static uint32_t
child_by_atom(const SExp *tree, uint32_t root, AtomId target)
{
    uint32_t c = sexp_first_child(tree, root);
    while (c != SEXP_NULL_INDEX) {
        if (sexp_kind(tree, c) == NODE_LIST) {
            uint32_t head = sexp_first_child(tree, c);
            if (head != SEXP_NULL_INDEX
                    && sexp_kind(tree, head) == NODE_ATOM
                    && sexp_atom(tree, head) == target)
                return c;
        }
        c = sexp_next_sibling(tree, c);
    }
    return SEXP_NULL_INDEX;
}

static PyObject *
subscript_at(SExpObject *owner, const SExp *tree, uint32_t root, PyObject *key)
{
    if (PyLong_Check(key)) {
        Py_ssize_t i = PyLong_AsSsize_t(key);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        uint32_t c = child_at(tree, root, i);
        if (c == SEXP_NULL_INDEX) {
            PyErr_SetString(PyExc_IndexError, "index out of range");
            return NULL;
        }
        return (PyObject *)node_from_idx(owner, c);
    }

    if (PyUnicode_Check(key)) {
        Py_ssize_t klen = 0;
        const char *kstr = PyUnicode_AsUTF8AndSize(key, &klen);
        if (!kstr)
            return NULL;
        AtomId target = intern_string(kstr, (size_t)klen);
        if (!target) {
            PyErr_NoMemory();
            return NULL;
        }
        uint32_t c = child_by_atom(tree, root, target);
        if (c == SEXP_NULL_INDEX) {
            PyErr_SetObject(PyExc_KeyError, key);
            return NULL;
        }
        return (PyObject *)node_from_idx(owner, c);
    }

    PyErr_SetString(PyExc_TypeError, "indices must be int or str");
    return NULL;
}

/* ════════════════════════════════════════════════════════════════
   __repr__ helper
   ════════════════════════════════════════════════════════════════ */

static int
repr_build(const SExp *tree, uint32_t idx, PyObject *list)
{
    if (sexp_kind(tree, idx) == NODE_ATOM) {
        size_t len  = 0;
        AtomId id   = sexp_atom(tree, idx);
        const char *s = intern_lookup(id, &len);
        PyObject *part = s
            ? PyUnicode_FromStringAndSize(s, (Py_ssize_t)len)
            : PyUnicode_FromString("?");
        if (!part)
            return -1;
        int rc = PyList_Append(list, part);
        Py_DECREF(part);
        return rc;
    }

    PyObject *lparen = PyUnicode_FromString("(");
    if (!lparen || PyList_Append(list, lparen) < 0) {
        Py_XDECREF(lparen);
        return -1;
    }
    Py_DECREF(lparen);

    int first_child = 1;
    uint32_t c = sexp_first_child(tree, idx);
    while (c != SEXP_NULL_INDEX) {
        if (!first_child) {
            PyObject *sp = PyUnicode_FromString(" ");
            if (!sp || PyList_Append(list, sp) < 0) {
                Py_XDECREF(sp);
                return -1;
            }
            Py_DECREF(sp);
        }
        first_child = 0;
        if (repr_build(tree, c, list) < 0)
            return -1;
        c = sexp_next_sibling(tree, c);
    }

    PyObject *rparen = PyUnicode_FromString(")");
    if (!rparen || PyList_Append(list, rparen) < 0) {
        Py_XDECREF(rparen);
        return -1;
    }
    Py_DECREF(rparen);
    return 0;
}

static PyObject *
repr_from_idx(const SExp *tree, uint32_t idx)
{
    PyObject *parts = PyList_New(0);
    if (!parts)
        return NULL;
    if (repr_build(tree, idx, parts) < 0) {
        Py_DECREF(parts);
        return NULL;
    }
    PyObject *empty  = PyUnicode_FromString("");
    PyObject *result = empty ? PyUnicode_Join(empty, parts) : NULL;
    Py_XDECREF(empty);
    Py_DECREF(parts);
    return result;
}

/* ════════════════════════════════════════════════════════════════
   SExpNodeType
   ════════════════════════════════════════════════════════════════ */

static void
sexpnode_dealloc(SExpNodeObject *self)
{
    Py_DECREF(self->owner);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
sexpnode_repr(SExpNodeObject *self)
{
    SExp *tree = &self->owner->tree;
    if (sexp_kind(tree, self->idx) == NODE_ATOM) {
        size_t len = 0;
        const char *str = intern_lookup(sexp_atom(tree, self->idx), &len);
        if (str == NULL)
            return PyUnicode_FromString("");
        return PyUnicode_FromStringAndSize(str, (Py_ssize_t)len);
    }
    return repr_from_idx(tree, self->idx);
}

static Py_ssize_t
sexpnode_length(SExpNodeObject *self)
{
    return child_count(&self->owner->tree, self->idx);
}

static PyObject *
sexpnode_subscript(SExpNodeObject *self, PyObject *key)
{
    return subscript_at(self->owner, &self->owner->tree, self->idx, key);
}

static PyObject *
sexpnode_iter(SExpNodeObject *self)
{
    uint32_t start = sexp_first_child(&self->owner->tree, self->idx);
    return make_iter(&SExpNodeIterType, self->owner, start);
}

static PyObject *
sexpnode_head(SExpNodeObject *self, void *Py_UNUSED(closure))
{
    uint32_t child = sexp_first_child(&self->owner->tree, self->idx);
    if (child == SEXP_NULL_INDEX) {
        PyErr_SetString(PyExc_IndexError, "node has no children");
        return NULL;
    }
    return (PyObject *)node_from_idx(self->owner, child);
}

static PyObject *
sexpnode_tail(SExpNodeObject *self, void *Py_UNUSED(closure))
{
    const SExp *tree  = &self->owner->tree;
    uint32_t    first = sexp_first_child(tree, self->idx);
    uint32_t    start = (first != SEXP_NULL_INDEX)
        ? sexp_next_sibling(tree, first)
        : SEXP_NULL_INDEX;
    return make_iter(&SExpNodeTailIterType, self->owner, start);
}

static PyObject *
sexpnode_value_get(SExpNodeObject *self, void *Py_UNUSED(closure))
{
    SExp *tree = &self->owner->tree;
    if (sexp_kind(tree, self->idx) != NODE_ATOM) {
        PyErr_SetString(PyExc_TypeError, "node is not an atom");
        return NULL;
    }
    size_t len = 0;
    const char *str = intern_lookup(sexp_atom(tree, self->idx), &len);
    if (str == NULL)
        return PyUnicode_FromString("");
    return PyUnicode_FromStringAndSize(str, (Py_ssize_t)len);
}

static int
sexpnode_value_set(SExpNodeObject *self, PyObject *val,
                   void *Py_UNUSED(closure))
{
    if (!PyUnicode_Check(val)) {
        PyErr_SetString(PyExc_TypeError, "value must be a str");
        return -1;
    }
    Py_ssize_t len = 0;
    const char *str = PyUnicode_AsUTF8AndSize(val, &len);
    if (str == NULL)
        return -1;
    sexp_set_atom(&self->owner->tree, self->idx, str, (size_t)len);
    return 0;
}

static PyGetSetDef sexpnode_getset[] = {
    { "head",  (getter)sexpnode_head,  NULL,
      "First child node.", NULL },
    { "tail",  (getter)sexpnode_tail,  NULL,
      "Iterator over children after the first.", NULL },
    { "value", (getter)sexpnode_value_get, (setter)sexpnode_value_set,
      "String value of an atom node.", NULL },
    { NULL }
};

static PyMappingMethods sexpnode_mapping = {
    .mp_length    = (lenfunc)sexpnode_length,
    .mp_subscript = (binaryfunc)sexpnode_subscript,
};

static PyTypeObject SExpNodeIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "sexp.SExpNodeIter",
    .tp_basicsize = sizeof(SExpIterObject),
    .tp_dealloc   = (destructor)sexpiter_dealloc,
    .tp_iter      = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)sexpiter_next,
    .tp_flags     = Py_TPFLAGS_DEFAULT,
};

static PyTypeObject SExpNodeTailIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "sexp.SExpNodeTailIter",
    .tp_basicsize = sizeof(SExpIterObject),
    .tp_dealloc   = (destructor)sexpiter_dealloc,
    .tp_iter      = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc)sexpiter_next,
    .tp_flags     = Py_TPFLAGS_DEFAULT,
};

static PyTypeObject SExpNodeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name       = "sexp.SExpNode",
    .tp_basicsize  = sizeof(SExpNodeObject),
    .tp_dealloc    = (destructor)sexpnode_dealloc,
    .tp_repr       = (reprfunc)sexpnode_repr,
    .tp_as_mapping = &sexpnode_mapping,
    .tp_iter       = (getiterfunc)sexpnode_iter,
    .tp_getset     = sexpnode_getset,
    .tp_flags      = Py_TPFLAGS_DEFAULT,
    .tp_doc        = "Non-owning view of a node within an S-expression tree.",
};

/* ════════════════════════════════════════════════════════════════
   SExpType
   ════════════════════════════════════════════════════════════════ */

static void
sexp_dealloc(SExpObject *self)
{
    sexp_free(&self->tree);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
sexp_repr(SExpObject *self)
{
    if (self->tree.count == 0)
        return PyUnicode_FromString("()");
    return repr_from_idx(&self->tree, 0);
}

static Py_ssize_t
sexp_length(SExpObject *self)
{
    if (self->tree.count == 0)
        return 0;
    return child_count(&self->tree, 0);
}

static PyObject *
sexp_subscript(SExpObject *self, PyObject *key)
{
    if (self->tree.count == 0) {
        PyErr_SetString(PyExc_IndexError, "S-expression is empty");
        return NULL;
    }
    return subscript_at(self, &self->tree, 0, key);
}

static PyObject *
sexp_iter(SExpObject *self)
{
    uint32_t start = (self->tree.count > 0)
        ? sexp_first_child(&self->tree, 0)
        : SEXP_NULL_INDEX;
    return make_iter(&SExpIterType, self, start);
}

static PyObject *
sexp_head(SExpObject *self, void *Py_UNUSED(ignored))
{
    if (self->tree.count == 0) {
        PyErr_SetString(PyExc_IndexError, "S-expression is empty");
        return NULL;
    }
    uint32_t c = sexp_first_child(&self->tree, 0);
    if (c == SEXP_NULL_INDEX) {
        PyErr_SetString(PyExc_IndexError, "S-expression has no children");
        return NULL;
    }
    return (PyObject *)node_from_idx(self, c);
}

static PyObject *
sexp_tail(SExpObject *self, void *Py_UNUSED(ignored))
{
    uint32_t first = (self->tree.count > 0)
        ? sexp_first_child(&self->tree, 0)
        : SEXP_NULL_INDEX;
    uint32_t start = (first != SEXP_NULL_INDEX)
        ? sexp_next_sibling(&self->tree, first)
        : SEXP_NULL_INDEX;
    return make_iter(&SExpTailIterType, self, start);
}

static PyGetSetDef sexp_getset[] = {
    { "head", (getter)sexp_head, NULL,
      "First child node, or raises IndexError if empty.", NULL },
    { "tail", (getter)sexp_tail, NULL,
      "Iterator over children[1:].", NULL },
    { NULL }
};

static PyMappingMethods sexp_mapping = {
    .mp_length    = (lenfunc)sexp_length,
    .mp_subscript = (binaryfunc)sexp_subscript,
};

static PyTypeObject SExpType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name       = "sexp.SExp",
    .tp_basicsize  = sizeof(SExpObject),
    .tp_dealloc    = (destructor)sexp_dealloc,
    .tp_repr       = (reprfunc)sexp_repr,
    .tp_as_mapping = &sexp_mapping,
    .tp_flags      = Py_TPFLAGS_DEFAULT,
    .tp_doc        = "Parsed S-expression tree (owns the backing memory).",
    .tp_iter       = (getiterfunc)sexp_iter,
    .tp_getset     = sexp_getset,
};

/* ════════════════════════════════════════════════════════════════
   parse()
   ════════════════════════════════════════════════════════════════ */

static PyObject *
sexp_parse_func(PyObject *Py_UNUSED(module), PyObject *args)
{
    Py_buffer view;
    if (!PyArg_ParseTuple(args, "s*", &view))
        return NULL;

    SExpObject *obj = PyObject_New(SExpObject, &SExpType);
    if (!obj) {
        PyBuffer_Release(&view);
        return NULL;
    }

    obj->tree = sexp_parse((const char *)view.buf, (size_t)view.len);
    PyBuffer_Release(&view);

    if (obj->tree.arena.base == NULL) {
        Py_DECREF(obj);
        PyErr_SetString(PyExc_ValueError, "failed to parse S-expression");
        return NULL;
    }
    return (PyObject *)obj;
}

static PyMethodDef module_methods[] = {
    {
        "parse", sexp_parse_func, METH_VARARGS,
        "parse(src) -> SExp\n"
        "\n"
        "Parse an S-expression from a str, bytes, or bytearray."
    },
    { NULL }
};

static PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    .m_name    = "_sexp",
    .m_doc     = "C extension backing the sexp package.",
    .m_size    = -1,
    .m_methods = module_methods,
};

PyMODINIT_FUNC
PyInit__sexp(void)
{
    if (intern_init() != 0) {
        PyErr_SetString(PyExc_RuntimeError, "sexp: intern_init() failed");
        return NULL;
    }

    static PyTypeObject *types[] = {
        &SExpIterType,
        &SExpTailIterType,
        &SExpNodeIterType,
        &SExpNodeTailIterType,
        &SExpNodeType,
        &SExpType,
        NULL,
    };
    for (int i = 0; types[i]; i++) {
        if (PyType_Ready(types[i]) < 0)
            return NULL;
    }

    PyObject *m = PyModule_Create(&module_def);
    if (!m)
        return NULL;

    if (PyModule_AddObjectRef(m, "SExp",     (PyObject *)&SExpType)     < 0
     || PyModule_AddObjectRef(m, "SExpNode", (PyObject *)&SExpNodeType) < 0) {
        Py_DECREF(m);
        return NULL;
    }

    return m;
}