use pyo3::{create_exception, exceptions::PyValueError};

create_exception!(
    _sexp,
    ParseError,
    PyValueError,
    "Raised when an S-expression string cannot be parsed."
);
