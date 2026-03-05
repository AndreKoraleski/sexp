"""Tests for SExp.extract()."""

import pytest

import sexp


class TestExtractBasics:
    def test_returns_sexp_type(self) -> None:
        tree = sexp.parse("(a b)")
        assert isinstance(tree[0].extract(), sexp.SExp)

    def test_repr_of_extracted(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        assert repr(tree[1].extract()) == "(b c)"

    def test_removes_from_original(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        tree[1].extract()
        assert repr(tree) == "(a d)"

    def test_extract_atom(self) -> None:
        tree = sexp.parse("(a b c)")
        extracted = tree[1].extract()
        assert repr(extracted) == "b"
        assert repr(tree) == "(a c)"

    def test_extract_first_child(self) -> None:
        tree = sexp.parse("(a b c)")
        extracted = tree[0].extract()
        assert repr(extracted) == "a"
        assert repr(tree) == "(b c)"

    def test_extracted_root_is_parent_free(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        extracted = tree[1].extract()
        assert extracted.parent is None


class TestExtractIndependence:
    def test_mutating_extracted_does_not_affect_original(self) -> None:
        tree = sexp.parse("(a (b c) d)")
        extracted = tree[1].extract()
        extracted[0].value = "X"
        assert repr(extracted) == "(X c)"
        assert repr(tree) == "(a d)"


class TestExtractStaleness:
    def test_original_handle_becomes_stale(self) -> None:
        tree = sexp.parse("(a b c)")
        node = tree[1]
        node.extract()
        with pytest.raises(RuntimeError):
            repr(node)

    def test_root_extract_raises_value_error(self) -> None:
        with pytest.raises(ValueError):
            sexp.parse("(a b c)").extract()

    def test_empty_root_extract_raises_value_error(self) -> None:
        with pytest.raises(ValueError):
            sexp.parse("()").extract()
