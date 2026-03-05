use super::node::{Node, NodeId};
use crate::memory::{atom::Atom, slab::Slab};

/// Owns the node slab for a single S-expression tree.
pub struct Tree {
    pub(super) nodes: Slab<Node>,
    pub(super) root: NodeId,
}

impl Tree {
    /// Creates a tree containing only an empty root list node.
    pub fn new() -> Self {
        let mut nodes = Slab::new();
        let root = nodes.insert(Node::new_list());
        Self { nodes, root }
    }

    /// Returns the id of the root node.
    pub fn root(&self) -> NodeId {
        self.root
    }

    /// Allocates an unattached atom node and returns its id.
    pub fn alloc_atom(&mut self, value: impl Into<Atom>) -> NodeId {
        self.nodes.insert(Node::new_atom(value.into()))
    }

    /// Allocates an unattached list node and returns its id.
    pub fn alloc_list(&mut self) -> NodeId {
        self.nodes.insert(Node::new_list())
    }

    /// Returns a reference to the node at `id`.
    ///
    /// # Panics
    ///
    /// Panics if `id` is stale.
    pub fn get(&self, id: NodeId) -> &Node {
        &self.nodes[id]
    }

    /// Returns a mutable reference to the node at `id`.
    ///
    /// # Panics
    ///
    /// Panics if `id` is stale.
    pub fn get_mut(&mut self, id: NodeId) -> &mut Node {
        &mut self.nodes[id]
    }

    /// Returns the number of nodes currently in the tree, including the root.
    pub fn len(&self) -> usize {
        self.nodes.len()
    }

    /// Returns `true` if the tree contains only the root node.
    pub fn is_empty(&self) -> bool {
        self.nodes.len() == 1
    }

    /// Returns the id of the first child of `parent`, or `None`.
    pub fn first_child(&self, parent: NodeId) -> Option<NodeId> {
        self.nodes[parent].first_child
    }

    /// Returns the id of the last child of `parent`, or `None`.
    pub fn last_child(&self, parent: NodeId) -> Option<NodeId> {
        self.nodes[parent].last_child
    }

    /// Returns the id of the next sibling of `node`, or `None`.
    pub fn next_sibling(&self, node: NodeId) -> Option<NodeId> {
        self.nodes[node].next_sibling
    }

    /// Returns the id of the previous sibling of `node`, or `None`.
    pub fn prev_sibling(&self, node: NodeId) -> Option<NodeId> {
        self.nodes[node].previous_sibling
    }

    /// Returns the id of the parent of `node`, or `None` if `node` is the root.
    pub fn parent(&self, node: NodeId) -> Option<NodeId> {
        self.nodes[node].parent
    }

    /// Assembles a `Tree` from a pre-built slab and a root node id.
    ///
    /// Used by the parser, which builds the slab directly rather than going through the public
    /// mutation API.
    pub(crate) fn from_raw(nodes: Slab<Node>, root: NodeId) -> Self {
        Self { nodes, root }
    }
}

impl Default for Tree {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn new_tree_contains_only_root() {
        let tree = Tree::new();
        assert_eq!(tree.len(), 1);
        assert!(tree.is_empty());
    }

    #[test]
    fn root_is_a_list_node() {
        let tree = Tree::new();
        assert!(!tree.get(tree.root()).is_atom());
    }

    #[test]
    fn alloc_atom_increases_node_count() {
        let mut tree = Tree::new();
        tree.alloc_atom("hello");
        assert_eq!(tree.len(), 2);
    }
}
