use std::{cell::UnsafeCell, sync::Arc};

use pyo3::{PyErr, PyResult, Python, exceptions::PyRuntimeError};

use crate::{
    core::{
        node::{NodeId, NodeType},
        tree::Tree,
    },
    parse::ParseError as RustParseError,
};

use super::error::ParseError as PyParseError;

/// `Tree` wrapped in `UnsafeCell` for lock-free shared ownership under the GIL.
pub(super) struct GilTree(UnsafeCell<Tree>);

unsafe impl Send for GilTree {}
unsafe impl Sync for GilTree {}

impl GilTree {
    pub(super) fn new(tree: Tree) -> Self {
        Self(UnsafeCell::new(tree))
    }

    /// Shared reference to the inner tree. Caller must hold the GIL.
    #[inline]
    pub(super) fn get<'a>(&'a self, _py: Python<'_>) -> &'a Tree {
        unsafe { &*self.0.get() }
    }

    /// Exclusive reference to the inner tree. Caller must hold the GIL and must not hold any live
    /// `&Tree` borrow on the same `GilTree`.
    #[inline]
    #[allow(clippy::mut_from_ref)]
    pub(super) fn get_mut<'a>(&'a self, _py: Python<'_>) -> &'a mut Tree {
        unsafe { &mut *self.0.get() }
    }
}

/// Reference-counted, GIL-protected handle to a shared `Tree`.
pub(super) type SharedTree = Arc<GilTree>;

pub(super) fn stale_error() -> PyErr {
    PyRuntimeError::new_err("stale node reference")
}

pub(super) fn rust_parse_error(error: RustParseError) -> PyErr {
    PyParseError::new_err(error.to_string())
}

/// Checks that `tree` and `other` share the same allocation.
pub(super) fn assert_same_tree(tree: &SharedTree, other: &SharedTree) -> PyResult<()> {
    if !Arc::ptr_eq(tree, other) {
        Err(pyo3::exceptions::PyValueError::new_err(
            "node belongs to a different tree",
        ))
    } else {
        Ok(())
    }
}

/// Structural deep equality between two subtrees, possibly in different trees.
pub(super) fn subtrees_equal(
    left_tree: &Tree,
    left_node: NodeId,
    right_tree: &Tree,
    right_node: NodeId,
) -> bool {
    let mut stack = vec![(left_node, right_node)];
    while let Some((left, right)) = stack.pop() {
        match (&left_tree.get(left).kind, &right_tree.get(right).kind) {
            (NodeType::Atom(la), NodeType::Atom(ra)) => {
                if la.as_str() != ra.as_str() {
                    return false;
                }
            }
            (NodeType::List, NodeType::List) => {
                if left_tree.get(left).child_count != right_tree.get(right).child_count {
                    return false;
                }
                let mut lc = left_tree.first_child(left);
                let mut rc = right_tree.first_child(right);
                while let (Some(l_id), Some(r_id)) = (lc, rc) {
                    stack.push((l_id, r_id));
                    lc = left_tree.next_sibling(l_id);
                    rc = right_tree.next_sibling(r_id);
                }
            }
            _ => return false,
        }
    }
    true
}
