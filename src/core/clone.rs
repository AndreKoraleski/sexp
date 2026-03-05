use super::{
    iter::ChildIter,
    mutation::append,
    node::{NodeId, NodeType},
    tree::Tree,
};

/// Deep-copies the subtree rooted at `node` into a new independent [`Tree`].
///
/// The destination root corresponds to `node`. Its children are cloned recursively.
pub fn clone_subtree(source: &Tree, node: NodeId) -> Tree {
    let mut destination = Tree::new();
    let destination_root = destination.root();
    let children: Vec<NodeId> = ChildIter::new(source, node).collect();
    for child in children {
        clone_into(source, child, &mut destination, destination_root);
    }
    destination
}

/// Removes the subtree rooted at `node` from `tree` and returns it as a new [`Tree`].
///
/// If `node` is the root, returns a clone and leaves the source tree unchanged.
pub fn extract_subtree(tree: &mut Tree, node: NodeId) -> Tree {
    let extracted = clone_subtree(tree, node);
    if node != tree.root() {
        super::mutation::remove(tree, node);
    }
    extracted
}

/// Clones `source_node` from `source` into `destination` as a child of `destination_parent`.
/// Uses an explicit stack to avoid call-stack overflow on deeply nested input.
fn clone_into(
    source: &Tree,
    source_node: NodeId,
    destination: &mut Tree,
    destination_parent: NodeId,
) {
    let mut stack = vec![(source_node, destination_parent)];
    while let Some((src, dst_parent)) = stack.pop() {
        let new_node = match &source.get(src).kind {
            NodeType::Atom(atom) => destination.alloc_atom(atom.clone()),
            NodeType::List => destination.alloc_list(),
        };
        append(destination, dst_parent, new_node);
        let children: Vec<NodeId> = ChildIter::new(source, src).collect();
        for child in children.into_iter().rev() {
            stack.push((child, new_node));
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::core::{iter::ChildIter, mutation, tree::Tree};

    fn build_nested_tree() -> Tree {
        let mut tree = Tree::new();
        let root = tree.root();
        let atom_a = tree.alloc_atom("a");
        let list = tree.alloc_list();
        let atom_b = tree.alloc_atom("b");
        let atom_c = tree.alloc_atom("c");
        mutation::append(&mut tree, root, atom_a);
        mutation::append(&mut tree, root, list);
        mutation::append(&mut tree, list, atom_b);
        mutation::append(&mut tree, list, atom_c);
        tree
    }

    #[test]
    fn cloned_tree_is_independent() {
        let source = build_nested_tree();
        let mut cloned = clone_subtree(&source, source.root());
        assert_eq!(cloned.len(), source.len());
        let cloned_root = cloned.root();
        let extra = cloned.alloc_atom("extra");
        mutation::append(&mut cloned, cloned_root, extra);
        assert_ne!(cloned.len(), source.len());
    }

    #[test]
    fn clone_preserves_atom_values() {
        let source = build_nested_tree();
        let cloned = clone_subtree(&source, source.root());
        let first_child = cloned.first_child(cloned.root()).unwrap();
        assert_eq!(cloned.get(first_child).atom_value().unwrap().as_str(), "a");
    }

    #[test]
    fn extract_removes_node_from_source() {
        let mut tree = build_nested_tree();
        let root = tree.root();
        let node_count_before = tree.len();
        let inner_list = ChildIter::new(&tree, root).nth(1).unwrap();
        let extracted = extract_subtree(&mut tree, inner_list);
        assert!(tree.nodes.get(inner_list).is_none());
        assert!(tree.len() < node_count_before);
        assert_eq!(extracted.len(), 3);
    }
}
