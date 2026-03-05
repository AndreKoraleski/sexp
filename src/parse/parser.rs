use crate::{
    core::{
        node::{Node, NodeId},
        tree::Tree,
    },
    memory::{atom::Atom, slab::Slab},
};

use super::{
    error::ParseError,
    tokenizer::{Token, Tokenizer},
};

/// Parses `input` into a [`Tree`].
///
/// `input` must be valid UTF-8.  Use [`parse_bytes`] when the source may be raw bytes.
///
/// # Errors
///
/// Returns a [`ParseError`] if the input is structurally malformed: an unclosed parenthesis, a
/// stray closing parenthesis, or more than one top-level form.
pub fn parse(input: &str) -> Result<Tree, ParseError> {
    let mut nodes: Slab<Node> = Slab::new();
    let mut open_lists: Vec<NodeId> = Vec::new();
    let mut top_level: Vec<NodeId> = Vec::new();

    for token in Tokenizer::new(input) {
        match token {
            Token::LeftParen => {
                let list_node = nodes.insert(Node::new_list());

                if let Some(&parent) = open_lists.last() {
                    link_child(&mut nodes, parent, list_node);
                } else {
                    top_level.push(list_node);
                }

                open_lists.push(list_node);
            }
            Token::RightParen => {
                if open_lists.pop().is_none() {
                    return Err(ParseError::StrayClosingParenthesis);
                }
            }
            Token::Atom(value) => {
                let atom_node = nodes.insert(Node::new_atom(Atom::new(value)));

                if let Some(&parent) = open_lists.last() {
                    link_child(&mut nodes, parent, atom_node);
                } else {
                    top_level.push(atom_node);
                }
            }
        }
    }

    if !open_lists.is_empty() {
        return Err(ParseError::UnclosedParenthesis);
    }

    match top_level.len() {
        0 => Ok(Tree::new_bare()),
        1 => Ok(Tree::from_raw(nodes, top_level[0])),
        _ => Err(ParseError::MultipleTopLevelForms),
    }
}

/// Parses `input` bytes as UTF-8 and returns a [`Tree`].
///
/// # Errors
///
/// Returns [`ParseError::InvalidUtf8`] if `input` is not valid UTF-8, or any other [`ParseError`]
/// variant if the S-expression is malformed.
pub fn parse_bytes(input: &[u8]) -> Result<Tree, ParseError> {
    let source = std::str::from_utf8(input).map_err(|_| ParseError::InvalidUtf8)?;
    parse(source)
}

/// Links `child` as the last child of `parent` in the node slab.
fn link_child(nodes: &mut Slab<Node>, parent: NodeId, child: NodeId) {
    nodes[child].parent = Some(parent);

    match nodes[parent].last_child {
        Some(previous_last) => {
            nodes[previous_last].next_sibling = Some(child);
            nodes[child].previous_sibling = Some(previous_last);
        }
        None => {
            nodes[parent].first_child = Some(child);
        }
    }

    nodes[parent].last_child = Some(child);
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::core::iter::ChildIter;

    #[test]
    fn empty_input_returns_empty_list() {
        let tree = parse("").unwrap();
        assert!(tree.is_empty());
        assert!(!tree.get(tree.root()).is_atom());
    }

    #[test]
    fn bare_atom_becomes_root() {
        let tree = parse("hello").unwrap();
        assert!(tree.get(tree.root()).is_atom());
        assert_eq!(
            tree.get(tree.root()).atom_value().unwrap().as_str(),
            "hello"
        );
    }

    #[test]
    fn simple_list_parses_correctly() {
        let tree = parse("(a b c)").unwrap();
        let root = tree.root();
        assert!(!tree.get(root).is_atom());
        let children: Vec<_> = ChildIter::new(&tree, root).collect();
        assert_eq!(children.len(), 3);
        assert_eq!(tree.get(children[0]).atom_value().unwrap().as_str(), "a");
        assert_eq!(tree.get(children[2]).atom_value().unwrap().as_str(), "c");
    }

    #[test]
    fn nested_list_preserves_structure() {
        let tree = parse("(a (b c) d)").unwrap();
        let root = tree.root();
        let children: Vec<_> = ChildIter::new(&tree, root).collect();
        assert_eq!(children.len(), 3);
        let inner_children: Vec<_> = ChildIter::new(&tree, children[1]).collect();
        assert_eq!(inner_children.len(), 2);
    }

    #[test]
    fn empty_list_parses_correctly() {
        let tree = parse("()").unwrap();
        assert!(!tree.get(tree.root()).is_atom());
        assert!(tree.is_empty());
    }

    #[test]
    fn unclosed_parenthesis_returns_error() {
        assert!(matches!(
            parse("(unclosed"),
            Err(ParseError::UnclosedParenthesis)
        ));
    }

    #[test]
    fn stray_closing_parenthesis_returns_error() {
        assert!(matches!(
            parse(")"),
            Err(ParseError::StrayClosingParenthesis)
        ));
    }

    #[test]
    fn multiple_top_level_lists_return_error() {
        assert!(matches!(
            parse("(a)(b)"),
            Err(ParseError::MultipleTopLevelForms)
        ));
    }

    #[test]
    fn multiple_top_level_atoms_return_error() {
        assert!(matches!(
            parse("a b"),
            Err(ParseError::MultipleTopLevelForms)
        ));
    }

    #[test]
    fn empty_input_returns_bare_tree() {
        let tree = parse("").unwrap();
        assert!(tree.is_bare());
    }

    #[test]
    fn non_empty_input_is_not_bare() {
        assert!(!parse("(a b)").unwrap().is_bare());
        assert!(!parse("hello").unwrap().is_bare());
        assert!(!parse("()").unwrap().is_bare());
    }
}
