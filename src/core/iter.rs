use super::{node::NodeId, tree::Tree};

/// Iterator over the direct children of a node.
pub struct ChildIter<'a> {
    tree: &'a Tree,
    next: Option<NodeId>,
}

impl<'a> ChildIter<'a> {
    /// Creates an iterator over the direct children of `parent`.
    pub fn new(tree: &'a Tree, parent: NodeId) -> Self {
        Self {
            tree,
            next: tree.first_child(parent),
        }
    }
}

impl Iterator for ChildIter<'_> {
    type Item = NodeId;

    fn next(&mut self) -> Option<NodeId> {
        let current = self.next?;
        self.next = self.tree.next_sibling(current);
        Some(current)
    }
}

/// Iterator over all nodes in a subtree in depth-first, pre-order.
pub struct DepthFirstIter<'a> {
    tree: &'a Tree,
    stack: Vec<NodeId>,
}

impl<'a> DepthFirstIter<'a> {
    /// Creates an iterator over the subtree rooted at `start`, inclusive.
    pub fn new(tree: &'a Tree, start: NodeId) -> Self {
        Self {
            tree,
            stack: vec![start],
        }
    }
}

impl Iterator for DepthFirstIter<'_> {
    type Item = NodeId;

    fn next(&mut self) -> Option<NodeId> {
        let current = self.stack.pop()?;
        let mut cursor = self.tree.last_child(current);
        while let Some(child_id) = cursor {
            let prev = self.tree.prev_sibling(child_id);
            self.stack.push(child_id);
            cursor = prev;
        }
        Some(current)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::core::{mutation, tree::Tree};

    fn build_list_tree() -> Tree {
        let mut tree = Tree::new();
        let root = tree.root();
        let atom_a = tree.alloc_atom("a");
        let atom_b = tree.alloc_atom("b");
        let atom_c = tree.alloc_atom("c");
        mutation::append(&mut tree, root, atom_a);
        mutation::append(&mut tree, root, atom_b);
        mutation::append(&mut tree, root, atom_c);
        tree
    }

    #[test]
    fn child_iter_yields_direct_children_in_order() {
        let tree = build_list_tree();
        let children: Vec<NodeId> = ChildIter::new(&tree, tree.root()).collect();
        assert_eq!(children.len(), 3);
        assert_eq!(tree.get(children[0]).atom_value().unwrap().as_str(), "a");
        assert_eq!(tree.get(children[1]).atom_value().unwrap().as_str(), "b");
        assert_eq!(tree.get(children[2]).atom_value().unwrap().as_str(), "c");
    }

    #[test]
    fn depth_first_iter_visits_root_first() {
        let tree = build_list_tree();
        let mut iter = DepthFirstIter::new(&tree, tree.root());
        assert_eq!(iter.next(), Some(tree.root()));
    }

    #[test]
    fn depth_first_iter_on_empty_tree_yields_only_root() {
        let tree = Tree::new();
        let visited: Vec<NodeId> = DepthFirstIter::new(&tree, tree.root()).collect();
        assert_eq!(visited.len(), 1);
    }
}
