"""Tests for SExp.clone()."""

import sexp


class TestCloneBasics:
    def test_returns_sexp_type(self) -> None:
        assert isinstance(sexp.parse("(a b)")[0].clone(), sexp.SExp)

    def test_repr_matches_original(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        assert repr(tree[1].clone()) == "(b c)"

    def test_original_unchanged(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        _ = tree[1].clone()
        assert repr(tree) == "(a (b c) d)"

    def test_clone_atom_node(self) -> None:
        tree = sexp.parse("(a b c)")
        assert repr(tree[0].clone()) == "a"

    def test_clone_deep_structure(self) -> None:
        source = "(a (b (c d)) e)"
        tree = sexp.parse(source)
        assert repr(tree.clone()) == source

    def test_clone_empty_list(self) -> None:
        tree = sexp.parse("()")
        clone = tree.clone()
        assert isinstance(clone, sexp.SExp)
        assert len(clone) == 0


class TestCloneIndependence:
    def test_mutating_clone_does_not_affect_original(self) -> None:
        tree = sexp.parse("(a b c)")
        clone = tree.clone()
        clone[0].value = "Z"
        assert repr(tree) == "(a b c)"
        assert repr(clone) == "(Z b c)"

    def test_mutating_original_does_not_affect_clone(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        clone = tree[1].clone()
        tree[1][0].value = "X"
        assert repr(tree) == "(a (X c) d)"
        assert repr(clone) == "(b c)"

    def test_clone_of_subtree_is_new_root(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        clone = tree[1].clone()
        assert clone.parent is None
