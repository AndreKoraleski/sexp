use std::sync::{Arc, Mutex};

use pyo3::{PyErr, PyResult, exceptions::PyRuntimeError};

use crate::{
    core::{
        iter::ChildIter,
        node::{NodeId, NodeType},
        tree::Tree,
    },
    parse::ParseError as RustParseError,
};

use super::error::ParseError as PyParseError;

/// Reference-counted handle to a shared Tree.
pub(super) type SharedTree = Arc<Mutex<Tree>>;

pub(super) fn lock_error() -> PyErr {
    PyRuntimeError::new_err("internal tree lock poisoned")
}

pub(super) fn stale_error() -> PyErr {
    PyRuntimeError::new_err("stale node reference")
}

pub(super) fn rust_parse_error(error: RustParseError) -> PyErr {
    PyParseError::new_err(error.to_string())
}

/// Checks that `tree` and `child_tree` share the same allocation.
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
                let left_children: Vec<_> = ChildIter::new(left_tree, left).collect();
                let right_children: Vec<_> = ChildIter::new(right_tree, right).collect();
                if left_children.len() != right_children.len() {
                    return false;
                }
                for (l, r) in left_children.into_iter().zip(right_children) {
                    stack.push((l, r));
                }
            }
            _ => return false,
        }
    }
    true
}
