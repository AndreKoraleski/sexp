use std::sync::Arc;

use parking_lot::RwLock;

use pyo3::{
    exceptions::{PyIndexError, PyTypeError, PyValueError},
    prelude::*,
    types::{PyAny, PyList},
};

use crate::{
    core::{
        clone::{clone_subtree, extract_subtree},
        iter::ChildIter,
        mutation,
        node::{NodeId, NodeType},
        tree::Tree,
    },
    memory::atom::Atom,
};

use super::{
    iter::SExpIter,
    shared::{SharedTree, assert_same_tree, stale_error, subtrees_equal},
};

/// A single node in an S-expression tree.
///
/// Every ``SExp`` is a handle into a shared, heap-allocated tree.  All handles that originate from
/// the same `parse` call (or the same empty constructor) share the same tree. Mutations through one
/// handle are immediately visible through any other.
///
/// Nodes that have been removed via `remove` or `extract` become *stale*.  Any method call on a
/// stale node raises `RuntimeError`.
#[pyclass]
pub struct SExp {
    pub(super) tree: SharedTree,
    pub(super) node: NodeId,
    pub(super) version: u64,
}

impl SExp {
    pub(super) fn from_tree(tree: Tree) -> Self {
        let node = tree.root();
        let version = tree.version();
        SExp {
            node,
            version,
            tree: Arc::new(RwLock::new(tree)),
        }
    }

    pub(super) fn from_shared(tree: SharedTree, node: NodeId, version: u64) -> Self {
        SExp {
            tree,
            node,
            version,
        }
    }

    /// Returns true if this handle is stale (must not be used). The root node is always considered
    /// valid.
    pub(super) fn is_stale(&self, tree: &Tree) -> bool {
        self.node != tree.root() && self.version != tree.version()
    }

    fn with_tree<F, T>(&self, f: F) -> PyResult<T>
    where
        F: FnOnce(&Tree) -> PyResult<T>,
    {
        let guard = self.tree.read();
        if self.is_stale(&guard) {
            return Err(stale_error());
        }
        f(&guard)
    }

    fn with_tree_mut<F, T>(&self, f: F) -> PyResult<T>
    where
        F: FnOnce(&mut Tree) -> PyResult<T>,
    {
        let mut guard = self.tree.write();
        if self.is_stale(&guard) {
            return Err(stale_error());
        }
        f(&mut guard)
    }

    fn collect_children(&self, py: Python<'_>) -> PyResult<Vec<Py<SExp>>> {
        self.with_tree(|tree| {
            let v = tree.version();
            ChildIter::new(tree, self.node)
                .map(|id| Py::new(py, SExp::from_shared(self.tree.clone(), id, v)))
                .collect::<PyResult<Vec<_>>>()
        })
    }
}

#[pymethods]
impl SExp {
    /// Create an empty S-expression list, equivalent to ``parse("()")``.
    #[new]
    pub fn new_empty() -> Self {
        SExp::from_tree(Tree::new())
    }

    fn __repr__(&self) -> PyResult<String> {
        self.with_tree(|tree| {
            if tree.is_bare() && self.node == tree.root() && tree.first_child(self.node).is_none() {
                return Ok(String::new());
            }
            Ok(crate::serialize::serialize_node(tree, self.node))
        })
    }

    fn __len__(&self) -> PyResult<usize> {
        self.with_tree(|tree| Ok(tree.get(self.node).child_count as usize))
    }

    fn __eq__(&self, other: &Bound<'_, PyAny>) -> PyResult<bool> {
        let Ok(other_sexp) = other.extract::<PyRef<'_, SExp>>() else {
            return Ok(false);
        };
        if Arc::ptr_eq(&self.tree, &other_sexp.tree) {
            let guard = self.tree.read();
            if self.is_stale(&guard) || other_sexp.is_stale(&guard) {
                return Err(stale_error());
            }
            Ok(subtrees_equal(&guard, self.node, &guard, other_sexp.node))
        } else {
            let self_guard = self.tree.read();
            if self.is_stale(&self_guard) {
                return Err(stale_error());
            }
            let other_guard = other_sexp.tree.read();
            if other_sexp.is_stale(&other_guard) {
                return Err(stale_error());
            }
            Ok(subtrees_equal(
                &self_guard,
                self.node,
                &other_guard,
                other_sexp.node,
            ))
        }
    }

    fn __contains__(&self, item: &Bound<'_, PyAny>) -> PyResult<bool> {
        let guard = self.tree.read();
        if self.is_stale(&guard) {
            return Err(stale_error());
        }
        if let Ok(other_sexp) = item.extract::<PyRef<'_, SExp>>() {
            if !Arc::ptr_eq(&self.tree, &other_sexp.tree) {
                return Ok(false);
            }

            if other_sexp.is_stale(&guard) {
                return Ok(false);
            }
            return Ok(ChildIter::new(&guard, self.node).any(|id| id == other_sexp.node));
        }
        if let Ok(s) = item.extract::<&str>() {
            for child_id in ChildIter::new(&guard, self.node) {
                if let Some(atom) = guard.get(child_id).atom_value()
                    && atom.as_str() == s
                {
                    return Ok(true);
                }
            }
            return Ok(false);
        }
        Ok(false)
    }

    fn __getitem__(&self, py: Python<'_>, key: &Bound<'_, PyAny>) -> PyResult<Py<PyAny>> {
        if let Ok(index) = key.extract::<isize>() {
            let (child_id, version) = self.with_tree(|tree| {
                let children: Vec<NodeId> = ChildIter::new(tree, self.node).collect();
                let len = children.len() as isize;
                let real = if index < 0 { len + index } else { index };
                if real < 0 || real >= len {
                    Err(PyIndexError::new_err("index out of range"))
                } else {
                    Ok((children[real as usize], tree.version()))
                }
            })?;
            return Py::new(py, SExp::from_shared(self.tree.clone(), child_id, version))
                .map(|p| p.into_bound(py).into_any().unbind());
        }

        if let Ok(s) = key.extract::<String>() {
            let (child_id, version) = self.with_tree(|tree| {
                let v = tree.version();
                ChildIter::new(tree, self.node)
                    .find(|&id| {
                        let node = tree.get(id);
                        if let Some(first_child) = node.first_child {
                            tree.get(first_child)
                                .atom_value()
                                .map(|a| a.as_str() == s.as_str())
                                .unwrap_or(false)
                        } else {
                            node.atom_value()
                                .map(|a| a.as_str() == s.as_str())
                                .unwrap_or(false)
                        }
                    })
                    .map(|id| (id, v))
                    .ok_or_else(|| pyo3::exceptions::PyKeyError::new_err(format!("no child '{s}'")))
            })?;
            return Py::new(py, SExp::from_shared(self.tree.clone(), child_id, version))
                .map(|p| p.into_bound(py).into_any().unbind());
        }

        let children = self.collect_children(py)?;
        let list = PyList::new(py, &children)?;
        Ok(list.into_any().get_item(key)?.unbind())
    }

    fn __iter__(&self, py: Python<'_>) -> PyResult<Py<SExpIter>> {
        let (first, version) =
            self.with_tree(|tree| Ok((tree.first_child(self.node), tree.version())))?;
        Py::new(py, SExpIter::new(self.tree.clone(), first, version))
    }

    /// First child node.  Raises `IndexError` if empty.
    #[getter]
    fn head(&self, py: Python<'_>) -> PyResult<Py<SExp>> {
        let (child_id, version) = self.with_tree(|tree| {
            tree.first_child(self.node)
                .map(|id| (id, tree.version()))
                .ok_or_else(|| PyIndexError::new_err("expression is empty"))
        })?;
        Py::new(py, SExp::from_shared(self.tree.clone(), child_id, version))
    }

    /// Iterator over all children after the first.
    #[getter]
    fn tail(&self, py: Python<'_>) -> PyResult<Py<SExpIter>> {
        let (second, version) = self.with_tree(|tree| {
            let second = tree
                .first_child(self.node)
                .and_then(|fc| tree.next_sibling(fc));
            Ok((second, tree.version()))
        })?;
        Py::new(py, SExpIter::new(self.tree.clone(), second, version))
    }

    /// ``True`` if this is an atom (leaf) node.
    #[getter]
    fn is_atom(&self) -> PyResult<bool> {
        self.with_tree(|tree| Ok(tree.get(self.node).is_atom()))
    }

    /// String content of this atom node. Raises `TypeError` if this is a list.
    #[getter]
    fn value(&self) -> PyResult<String> {
        self.with_tree(|tree| match &tree.get(self.node).kind {
            NodeType::Atom(atom) => Ok(atom.as_str().to_owned()),
            NodeType::List => Err(PyTypeError::new_err("list node has no value")),
        })
    }

    /// Set the string content. Raises `TypeError` if this is a list.
    #[setter]
    fn set_value(&self, value: &str) -> PyResult<()> {
        self.with_tree_mut(|tree| {
            let node = tree.get_mut(self.node);
            if !node.is_atom() {
                return Err(PyTypeError::new_err("cannot set value on a list node"));
            }
            node.kind = NodeType::Atom(Atom::new(value));
            Ok(())
        })
    }

    /// Parent node, or ``None`` if this is the root.
    #[getter]
    fn parent(&self, py: Python<'_>) -> PyResult<Option<Py<SExp>>> {
        self.with_tree(|tree| match tree.parent(self.node) {
            None => Ok(None),
            Some(parent_id) => Ok(Some(Py::new(
                py,
                SExp::from_shared(self.tree.clone(), parent_id, tree.version()),
            )?)),
        })
    }

    /// Allocate a new unattached atom node in this tree.
    fn new_atom(&self, py: Python<'_>, value: &str) -> PyResult<Py<SExp>> {
        let (id, version) = self.with_tree_mut(|tree| {
            let id = tree.alloc_atom(value);
            Ok((id, tree.version()))
        })?;
        Py::new(py, SExp::from_shared(self.tree.clone(), id, version))
    }

    /// Allocate a new unattached empty list node in this tree.
    fn new_list(&self, py: Python<'_>) -> PyResult<Py<SExp>> {
        let (id, version) = self.with_tree_mut(|tree| {
            let id = tree.alloc_list();
            Ok((id, tree.version()))
        })?;
        Py::new(py, SExp::from_shared(self.tree.clone(), id, version))
    }

    /// Append ``child`` as the last child of this node.
    fn append(&self, child: &SExp) -> PyResult<()> {
        assert_same_tree(&self.tree, &child.tree)?;
        self.with_tree_mut(|tree| {
            mutation::append(tree, self.node, child.node);
            Ok(())
        })
    }

    /// Insert ``child`` as the first child of this node.
    fn prepend(&self, child: &SExp) -> PyResult<()> {
        assert_same_tree(&self.tree, &child.tree)?;
        self.with_tree_mut(|tree| {
            mutation::prepend(tree, self.node, child.node);
            Ok(())
        })
    }

    /// Insert ``child`` immediately after ``after``. Pass ``None`` to prepend.
    fn insert_after(&self, after: Option<&SExp>, child: &SExp) -> PyResult<()> {
        assert_same_tree(&self.tree, &child.tree)?;
        if let Some(after) = after {
            assert_same_tree(&self.tree, &after.tree)?;
        }
        let after_id = after.map(|a| a.node);
        self.with_tree_mut(|tree| {
            mutation::insert_after(tree, self.node, after_id, child.node);
            Ok(())
        })
    }

    /// Remove this node and its entire subtree. Raises `ValueError` on the root.
    fn remove(&self) -> PyResult<()> {
        self.with_tree_mut(|tree| {
            if self.node == tree.root() {
                return Err(PyValueError::new_err("cannot remove the root node"));
            }
            mutation::remove(tree, self.node);
            tree.bump_version();
            Ok(())
        })
    }

    /// Deep-copy this subtree into a new independent `SExp`.
    #[pyo3(name = "clone")]
    fn clone_node(&self, py: Python<'_>) -> PyResult<Py<SExp>> {
        let new_tree = self.with_tree(|tree| Ok(clone_subtree(tree, self.node)))?;
        Py::new(py, SExp::from_tree(new_tree))
    }

    /// Remove this subtree and return it as a new `SExp`. Raises `ValueError` on the root.
    fn extract(&self, py: Python<'_>) -> PyResult<Py<SExp>> {
        let new_tree = self.with_tree_mut(|tree| {
            if self.node == tree.root() {
                return Err(PyValueError::new_err("cannot extract the root node"));
            }
            let extracted = extract_subtree(tree, self.node);
            tree.bump_version();
            Ok(extracted)
        })?;
        Py::new(py, SExp::from_tree(new_tree))
    }
}
