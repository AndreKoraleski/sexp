use super::{node::NodeId, tree::Tree};

/// Appends `child` as the last child of `parent`.
///
/// If `child` is already attached elsewhere in the tree it is first detached from its current
/// parent and siblings.
///
/// # Panics
///
/// Panics if either id is stale.
pub fn append(tree: &mut Tree, parent: NodeId, child: NodeId) {
    if tree.nodes[child].parent.is_some() {
        unlink(tree, child);
    }
    let previous_last = tree.nodes[parent].last_child;

    tree.nodes[child].parent = Some(parent);
    tree.nodes[child].previous_sibling = previous_last;
    tree.nodes[child].next_sibling = None;

    match previous_last {
        Some(previous) => tree.nodes[previous].next_sibling = Some(child),
        None => tree.nodes[parent].first_child = Some(child),
    }
    tree.nodes[parent].last_child = Some(child);
    tree.nodes[parent].child_count += 1;
}

/// Inserts `child` as the first child of `parent`.
///
/// If `child` is already attached elsewhere in the tree it is first detached.
///
/// # Panics
///
/// Panics if either id is stale.
pub fn prepend(tree: &mut Tree, parent: NodeId, child: NodeId) {
    if tree.nodes[child].parent.is_some() {
        unlink(tree, child);
    }
    let previous_first = tree.nodes[parent].first_child;

    tree.nodes[child].parent = Some(parent);
    tree.nodes[child].previous_sibling = None;
    tree.nodes[child].next_sibling = previous_first;

    match previous_first {
        Some(previous) => tree.nodes[previous].previous_sibling = Some(child),
        None => tree.nodes[parent].last_child = Some(child),
    }
    tree.nodes[parent].first_child = Some(child);
    tree.nodes[parent].child_count += 1;
}

/// Inserts `child` immediately after `after` within the same parent.
///
/// Pass `None` for `after` to insert `child` as the first child of `parent`.
///
/// If `child` is already attached elsewhere in the tree it is first detached.
///
/// # Panics
///
/// Panics if any id is stale.
pub fn insert_after(tree: &mut Tree, parent: NodeId, after: Option<NodeId>, child: NodeId) {
    if tree.nodes[child].parent.is_some() {
        unlink(tree, child);
    }
    match after {
        None => prepend(tree, parent, child),
        Some(sibling) => {
            let next = tree.nodes[sibling].next_sibling;

            tree.nodes[child].parent = Some(parent);
            tree.nodes[child].previous_sibling = Some(sibling);
            tree.nodes[child].next_sibling = next;

            tree.nodes[sibling].next_sibling = Some(child);

            match next {
                Some(next_node) => tree.nodes[next_node].previous_sibling = Some(child),
                None => tree.nodes[parent].last_child = Some(child),
            }
            tree.nodes[parent].child_count += 1;
        }
    }
}

/// Removes `node` and its entire subtree from the tree.
///
/// All node ids within the subtree become stale after this call.
///
/// # Panics
///
/// Panics if `node` is stale.
pub fn remove(tree: &mut Tree, node: NodeId) {
    unlink(tree, node);
    drop_subtree(tree, node);
}

/// Detaches `node` from its parent and siblings without removing it from the slab.
fn unlink(tree: &mut Tree, node: NodeId) {
    let parent = tree.nodes[node].parent;
    let previous = tree.nodes[node].previous_sibling;
    let next = tree.nodes[node].next_sibling;

    if let Some(prev_node) = previous {
        tree.nodes[prev_node].next_sibling = next;
    } else if let Some(parent_node) = parent {
        tree.nodes[parent_node].first_child = next;
    }

    if let Some(next_node) = next {
        tree.nodes[next_node].previous_sibling = previous;
    } else if let Some(parent_node) = parent {
        tree.nodes[parent_node].last_child = previous;
    }

    if let Some(parent_node) = parent {
        tree.nodes[parent_node].child_count -= 1;
    }
    tree.nodes[node].parent = None;
    tree.nodes[node].previous_sibling = None;
    tree.nodes[node].next_sibling = None;
}

/// Removes `node` and all its descendants from the slab using an explicit stack.
fn drop_subtree(tree: &mut Tree, node: NodeId) {
    let mut stack = vec![node];
    while let Some(current) = stack.pop() {
        let mut cursor = tree.last_child(current);
        while let Some(child_id) = cursor {
            let prev = tree.prev_sibling(child_id);
            stack.push(child_id);
            cursor = prev;
        }
        tree.nodes.remove(current);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::core::{iter::ChildIter, tree::Tree};

    #[test]
    fn appended_child_becomes_last_child() {
        let mut tree = Tree::new();
        let root = tree.root();
        let atom = tree.alloc_atom("x");
        append(&mut tree, root, atom);
        assert_eq!(tree.last_child(root), Some(atom));
        assert_eq!(tree.first_child(root), Some(atom));
    }

    #[test]
    fn prepended_child_becomes_first_child() {
        let mut tree = Tree::new();
        let root = tree.root();
        let first = tree.alloc_atom("first");
        let second = tree.alloc_atom("second");
        append(&mut tree, root, second);
        prepend(&mut tree, root, first);
        assert_eq!(tree.first_child(root), Some(first));
        assert_eq!(tree.last_child(root), Some(second));
    }

    #[test]
    fn insert_after_none_prepends() {
        let mut tree = Tree::new();
        let root = tree.root();
        let existing = tree.alloc_atom("existing");
        let inserted = tree.alloc_atom("inserted");
        append(&mut tree, root, existing);
        insert_after(&mut tree, root, None, inserted);
        assert_eq!(tree.first_child(root), Some(inserted));
    }

    #[test]
    fn insert_after_sibling_links_correctly() {
        let mut tree = Tree::new();
        let root = tree.root();
        let first = tree.alloc_atom("first");
        let third = tree.alloc_atom("third");
        let second = tree.alloc_atom("second");
        append(&mut tree, root, first);
        append(&mut tree, root, third);
        insert_after(&mut tree, root, Some(first), second);
        assert_eq!(tree.next_sibling(first), Some(second));
        assert_eq!(tree.next_sibling(second), Some(third));
        assert_eq!(tree.prev_sibling(third), Some(second));
    }

    #[test]
    fn removed_node_becomes_stale() {
        let mut tree = Tree::new();
        let root = tree.root();
        let atom = tree.alloc_atom("gone");
        append(&mut tree, root, atom);
        remove(&mut tree, atom);
        assert!(tree.nodes.get(atom).is_none());
        assert_eq!(tree.first_child(root), None);
    }

    #[test]
    fn remove_drops_entire_subtree() {
        let mut tree = Tree::new();
        let root = tree.root();
        let list = tree.alloc_list();
        let child = tree.alloc_atom("child");
        append(&mut tree, root, list);
        append(&mut tree, list, child);
        remove(&mut tree, list);
        assert!(tree.nodes.get(list).is_none());
        assert!(tree.nodes.get(child).is_none());
        assert!(tree.is_empty());
    }

    #[test]
    fn append_moves_already_attached_node() {
        let mut tree = Tree::new();
        let root = tree.root();
        let a = tree.alloc_atom("a");
        let b = tree.alloc_atom("b");
        let c = tree.alloc_atom("c");
        append(&mut tree, root, a);
        append(&mut tree, root, b);
        append(&mut tree, root, c);
        append(&mut tree, root, a);
        let children: Vec<_> = ChildIter::new(&tree, root).collect();
        assert_eq!(children.len(), 3);
        assert_eq!(tree.get(children[0]).atom_value().unwrap().as_str(), "b");
        assert_eq!(tree.get(children[1]).atom_value().unwrap().as_str(), "c");
        assert_eq!(tree.get(children[2]).atom_value().unwrap().as_str(), "a");
    }

    #[test]
    fn prepend_moves_already_attached_node() {
        let mut tree = Tree::new();
        let root = tree.root();
        let a = tree.alloc_atom("a");
        let b = tree.alloc_atom("b");
        let c = tree.alloc_atom("c");
        append(&mut tree, root, a);
        append(&mut tree, root, b);
        append(&mut tree, root, c);
        prepend(&mut tree, root, c);
        let children: Vec<_> = ChildIter::new(&tree, root).collect();
        assert_eq!(children.len(), 3);
        assert_eq!(tree.get(children[0]).atom_value().unwrap().as_str(), "c");
        assert_eq!(tree.get(children[1]).atom_value().unwrap().as_str(), "a");
        assert_eq!(tree.get(children[2]).atom_value().unwrap().as_str(), "b");
    }

    #[test]
    fn insert_after_moves_already_attached_node() {
        let mut tree = Tree::new();
        let root = tree.root();
        let a = tree.alloc_atom("a");
        let b = tree.alloc_atom("b");
        let c = tree.alloc_atom("c");
        append(&mut tree, root, a);
        append(&mut tree, root, b);
        append(&mut tree, root, c);
        insert_after(&mut tree, root, Some(a), c);
        let children: Vec<_> = ChildIter::new(&tree, root).collect();
        assert_eq!(children.len(), 3);
        assert_eq!(tree.get(children[0]).atom_value().unwrap().as_str(), "a");
        assert_eq!(tree.get(children[1]).atom_value().unwrap().as_str(), "c");
        assert_eq!(tree.get(children[2]).atom_value().unwrap().as_str(), "b");
    }
}
