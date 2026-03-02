import pytest

import sexp


def test_insert_after_sibling():
    t = sexp.parse("(a b)")
    n = t.new_atom("x")
    t[0].parent.insert_after(t[0], n)
    assert repr(t) == "(a x b)"


def test_insert_after_last_sibling():
    t = sexp.parse("(a b)")
    n = t.new_atom("x")
    t[0].parent.insert_after(t[1], n)
    assert repr(t) == "(a b x)"


def test_insert_after_none_is_prepend():
    t = sexp.parse("(a b)")
    n = t.new_atom("x")
    t[0].parent.insert_after(None, n)
    assert repr(t) == "(x a b)"


def test_insert_after_moves_existing_node():
    t = sexp.parse("(a b c)")
    c_node = t[2]
    t[0].parent.insert_after(t[0], c_node)
    assert repr(t) == "(a c b)"


def test_insert_after_wrong_tree_child_raises():
    t1 = sexp.parse("(a b)")
    t2 = sexp.parse("(c d)")
    with pytest.raises(ValueError):
        t1[0].parent.insert_after(None, t2[0])


def test_insert_after_wrong_tree_after_raises():
    t1 = sexp.parse("(a b)")
    t2 = sexp.parse("(c d)")
    with pytest.raises(ValueError):
        t1[0].parent.insert_after(t2[0], t1.new_atom("x"))


def test_insert_after_non_node_raises():
    t = sexp.parse("(a b)")
    with pytest.raises(TypeError):
        t[0].parent.insert_after("not a node", t.new_atom("x"))


def test_insert_after_preserves_order():
    t = sexp.parse("(a b c)")
    n = t.new_atom("x")
    t[0].parent.insert_after(t[1], n)
    assert repr(t) == "(a b x c)"


# --- SExp root ---


def test_sexp_insert_after_sibling_via_root():
    t = sexp.parse("(a b c)")
    n = t.new_atom("x")
    t.insert_after(t[0], n)
    assert repr(t) == "(a x b c)"


def test_sexp_insert_after_last_via_root():
    t = sexp.parse("(a b)")
    n = t.new_atom("z")
    t.insert_after(t[1], n)
    assert repr(t) == "(a b z)"


def test_sexp_insert_after_none_is_prepend_via_root():
    t = sexp.parse("(a b)")
    n = t.new_atom("x")
    t.insert_after(None, n)
    assert repr(t) == "(x a b)"


def test_sexp_insert_after_wrong_tree_raises():
    t1 = sexp.parse("(a b)")
    t2 = sexp.parse("(c d)")
    with pytest.raises(ValueError):
        t1.insert_after(None, t2[0])
