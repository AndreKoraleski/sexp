use crate::core::{
    node::{NodeId, NodeType},
    tree::Tree,
};

/// Serializes the subtree rooted at `node` into a `String`.
///
/// Atoms are written as-is. Lists are written as `(child1 child2 ...)` with a single space between
/// siblings. An empty list serializes as `()`.
pub fn serialize_node(tree: &Tree, node: NodeId) -> String {
    // Heuristic: each node contributes ~5 chars on average (atom text + space,
    // or two parens + spaces for lists).  Pre-allocating avoids reallocs.
    let mut output = String::with_capacity(tree.len() * 5);
    write_node(tree, node, &mut output, false);
    output
}

/// Serializes the entire tree from its root into a `String`.
pub fn serialize(tree: &Tree) -> String {
    serialize_node(tree, tree.root())
}

/// Frame on the explicit serialization stack.
enum Frame {
    /// Emit the node, preceded by a space if `needs_space` is true.
    Node { id: NodeId, needs_space: bool },

    /// Emit a closing `)`.
    Close,
}

/// Writes `node` and its entire subtree into `output` using an explicit stack.
fn write_node(tree: &Tree, node: NodeId, output: &mut String, needs_space: bool) {
    let mut stack = vec![Frame::Node {
        id: node,
        needs_space,
    }];

    while let Some(frame) = stack.pop() {
        match frame {
            Frame::Close => output.push(')'),
            Frame::Node { id, needs_space } => match &tree.get(id).kind {
                NodeType::Atom(atom) => {
                    if needs_space {
                        output.push(' ');
                    }
                    output.push_str(atom.as_str());
                }

                NodeType::List => {
                    if needs_space {
                        output.push(' ');
                    }
                    output.push('(');

                    stack.push(Frame::Close);

                    let mut cursor = tree.last_child(id);
                    while let Some(child_id) = cursor {
                        let prev = tree.prev_sibling(child_id);
                        stack.push(Frame::Node {
                            id: child_id,
                            needs_space: prev.is_some(),
                        });
                        cursor = prev;
                    }
                }
            },
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        core::{mutation, tree::Tree},
        parse::parser::parse,
    };

    #[test]
    fn empty_tree_serializes_as_empty_list() {
        let tree = Tree::new();
        assert_eq!(serialize(&tree), "()");
    }

    #[test]
    fn single_atom_round_trips() {
        let tree = parse("hello").unwrap();
        assert_eq!(serialize(&tree), "hello");
    }

    #[test]
    fn simple_list_round_trips() {
        let tree = parse("(a b c)").unwrap();
        assert_eq!(serialize(&tree), "(a b c)");
    }

    #[test]
    fn nested_list_round_trips() {
        let tree = parse("(player (pos 1 2) (vel 3 4))").unwrap();
        assert_eq!(serialize(&tree), "(player (pos 1 2) (vel 3 4))");
    }

    #[test]
    fn deeply_nested_list_round_trips() {
        let tree = parse("(a (b (c (d e))) f)").unwrap();
        assert_eq!(serialize(&tree), "(a (b (c (d e))) f)");
    }

    #[test]
    fn empty_list_round_trips() {
        let tree = parse("()").unwrap();
        assert_eq!(serialize(&tree), "()");
    }

    #[test]
    fn serialize_node_produces_subtree_only() {
        let mut tree = Tree::new();
        let root = tree.root();
        let inner = tree.alloc_list();
        let atom_a = tree.alloc_atom("a");
        let atom_b = tree.alloc_atom("b");
        mutation::append(&mut tree, root, inner);
        mutation::append(&mut tree, inner, atom_a);
        mutation::append(&mut tree, inner, atom_b);
        assert_eq!(serialize_node(&tree, inner), "(a b)");
    }
}
