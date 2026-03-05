use pyo3::prelude::*;

pub mod core;
pub mod memory;

#[pymodule]
mod _sexp {}
