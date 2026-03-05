use pyo3::prelude::*;

pub mod core;
pub mod memory;
pub mod parse;
pub mod py;
pub mod serialize;

#[pymodule]
fn _sexp(module: &Bound<'_, PyModule>) -> PyResult<()> {
    py::register(module)
}
