pub mod error;
pub mod iter;
pub mod parse_fn;
pub mod sexp;
pub(super) mod shared;

use pyo3::prelude::*;

pub use error::ParseError;
pub use iter::SExpIter;
pub use parse_fn::parse;
pub use sexp::SExp;

pub fn register(module: &Bound<'_, PyModule>) -> PyResult<()> {
    module.add_class::<SExp>()?;
    module.add_class::<SExpIter>()?;
    module.add("ParseError", module.py().get_type::<ParseError>())?;
    module.add_function(wrap_pyfunction!(parse, module)?)?;
    Ok(())
}
