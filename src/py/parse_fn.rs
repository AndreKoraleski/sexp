use pyo3::{exceptions::PyTypeError, prelude::*, types::PyAny};

use crate::parse::parser::{parse as parse_tree, parse_bytes as parse_bytes_tree};

use super::{sexp::SExp, shared::rust_parse_error};

/// Parse an S-expression from a ``str``, ``bytes``, or ``bytearray``.
///
/// Raises:
///     ParseError: If the input is structurally malformed.
///     TypeError: If the argument is not ``str``, ``bytes``, or ``bytearray``.
#[pyfunction]
pub fn parse(py: Python<'_>, source: &Bound<'_, PyAny>) -> PyResult<Py<SExp>> {
    let tree = if let Ok(s) = source.extract::<&str>() {
        parse_tree(s).map_err(rust_parse_error)?
    } else if let Ok(b) = source.extract::<&[u8]>() {
        parse_bytes_tree(b).map_err(rust_parse_error)?
    } else if let Ok(b) = source.extract::<Vec<u8>>() {
        parse_bytes_tree(&b).map_err(rust_parse_error)?
    } else {
        return Err(PyTypeError::new_err("expected str, bytes, or bytearray"));
    };
    Py::new(py, SExp::from_tree(tree))
}
