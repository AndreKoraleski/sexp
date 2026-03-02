/**
 * @file _sexp.c
 * @brief Module initialisation for the _sexp Python C extension.
 *
 * Wires together the types defined in _sexp_iter.c, _sexp_node.c, and _sexp_tree.c into a
 * Python module and exposes the top-level parse() function.
 */

#include "_sexp_types.h"

/* --- parse() --- */

static PyObject *sexp_parse_func(PyObject *Py_UNUSED(module), PyObject *args) {
    Py_buffer view;
    if (!PyArg_ParseTuple(args, "s*", &view)) {
        return NULL;
    }

    SExpObject *tree_object = PyObject_New(SExpObject, &SExpType);
    if (!tree_object) {
        PyBuffer_Release(&view);
        return NULL;
    }

    tree_object->tree = sexp_parse((const char *)view.buf, (size_t)view.len);
    PyBuffer_Release(&view);

    if (!tree_object->tree.valid) {
        Py_DECREF(tree_object);
        PyErr_SetString(PyExc_ValueError, "failed to parse S-expression");
        return NULL;
    }
    return (PyObject *)tree_object;
}

static PyMethodDef module_methods[] = {
    {"parse",
     sexp_parse_func,
     METH_VARARGS,
     "parse(source) -> SExp\n"
     "\n"
     "Parse an S-expression from a str, bytes, or bytearray."},
    {NULL}
};

static PyModuleDef python_module_definition = {
    PyModuleDef_HEAD_INIT,
    .m_name    = "_sexp",
    .m_doc     = "C extension backing the sexp package.",
    .m_size    = -1,
    .m_methods = module_methods,
};

PyMODINIT_FUNC PyInit__sexp(void) {
    if (intern_init() != 0) {
        PyErr_SetString(PyExc_RuntimeError, "sexp: intern_init() failed");
        return NULL;
    }

    /* Register all types before creating the module so type hierarchies are resolved. */
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
        if (PyType_Ready(types[i]) < 0) {
            return NULL;
        }
    }

    PyObject *python_module = PyModule_Create(&python_module_definition);
    if (!python_module) {
        return NULL;
    }

    if (PyModule_AddObjectRef(python_module, "SExp", (PyObject *)&SExpType) < 0 ||
        PyModule_AddObjectRef(python_module, "SExpNode", (PyObject *)&SExpNodeType) < 0) {
        Py_DECREF(python_module);
        return NULL;
    }

    return python_module;
}
