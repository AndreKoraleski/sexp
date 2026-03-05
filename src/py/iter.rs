use pyo3::prelude::*;

use super::{
    sexp::SExp,
    shared::{lock_error, stale_error},
};

/// Iterator over the direct children of an `SExp` node.
#[pyclass]
pub struct SExpIter {
    pub(super) items: Vec<Py<SExp>>,
    pub(super) index: usize,
}

impl SExpIter {
    pub(super) fn new(items: Vec<Py<SExp>>) -> Self {
        SExpIter { items, index: 0 }
    }
}

#[pymethods]
impl SExpIter {
    fn __iter__(slf: PyRef<'_, Self>) -> PyRef<'_, Self> {
        slf
    }

    /// Return the next item, raising ``RuntimeError`` if the item has become stale.
    fn __next__(&mut self, py: Python<'_>) -> PyResult<Option<Py<SExp>>> {
        if self.index >= self.items.len() {
            return Ok(None);
        }
        let item = self.items[self.index].clone_ref(py);
        self.index += 1;
        {
            let sexp = item.borrow(py);
            let guard = sexp.tree.lock().map_err(|_| lock_error())?;
            if sexp.is_stale(&guard) || guard.try_get(sexp.node).is_none() {
                return Err(stale_error());
            }
        }
        Ok(Some(item))
    }
}
