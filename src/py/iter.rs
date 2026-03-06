use std::sync::Arc;

use pyo3::prelude::*;

use crate::core::node::NodeId;

use super::{
    sexp::SExp,
    shared::{SharedTree, stale_error},
};

/// Lazy iterator over the direct children of an `SExp` node.
///
/// Holds a reference to the shared tree and the id of the next sibling to yield, so no upfront
/// allocation of child nodes is required.
#[pyclass]
pub struct SExpIter {
    pub(super) tree: SharedTree,
    pub(super) next: Option<NodeId>,
    pub(super) version: u64,
}

impl SExpIter {
    pub(super) fn new(tree: SharedTree, next: Option<NodeId>, version: u64) -> Self {
        SExpIter {
            tree,
            next,
            version,
        }
    }
}

#[pymethods]
impl SExpIter {
    fn __iter__(slf: PyRef<'_, Self>) -> PyRef<'_, Self> {
        slf
    }

    /// Return the next child, raising ``RuntimeError`` if the tree has been mutated.
    fn __next__(&mut self, py: Python<'_>) -> PyResult<Option<Py<SExp>>> {
        let current = match self.next {
            None => return Ok(None),
            Some(id) => id,
        };
        let tree = self.tree.get(py);
        if tree.version() != self.version {
            return Err(stale_error());
        }
        self.next = tree.get(current).next_sibling;
        Py::new(
            py,
            SExp::from_shared(Arc::clone(&self.tree), current, self.version),
        )
        .map(Some)
    }
}
