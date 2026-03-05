use crate::memory::{atom::Atom, slab::Key};

/// A typed index into a [`crate::core::tree::Tree`]'s node slab.
pub type NodeId = Key<Node>;

/// The content of a node: either a leaf atom or an interior list.
pub enum NodeType {
    Atom(Atom),
    List,
}

/// A single node stored inside a [`crate::core::tree::Tree`].
pub struct Node {
    pub(super) kind: NodeType,
    pub(super) parent: Option<NodeId>,
    pub(super) first_child: Option<NodeId>,
    pub(super) last_child: Option<NodeId>,
    pub(super) previous_sibling: Option<NodeId>,
    pub(super) next_sibling: Option<NodeId>,
}

impl Node {
    /// Creates a new unlinked atom node.
    pub(super) fn new_atom(value: Atom) -> Self {
        Self {
            kind: NodeType::Atom(value),
            parent: None,
            first_child: None,
            last_child: None,
            previous_sibling: None,
            next_sibling: None,
        }
    }

    /// Creates a new unlinked list node.
    pub(super) fn new_list() -> Self {
        Self {
            kind: NodeType::List,
            parent: None,
            first_child: None,
            last_child: None,
            previous_sibling: None,
            next_sibling: None,
        }
    }

    /// Returns `true` if this is an atom node.
    pub fn is_atom(&self) -> bool {
        matches!(self.kind, NodeType::Atom(_))
    }

    /// Returns the atom value, or `None` if this is a list node.
    pub fn atom_value(&self) -> Option<&Atom> {
        match &self.kind {
            NodeType::Atom(atom) => Some(atom),
            NodeType::List => None,
        }
    }
}
