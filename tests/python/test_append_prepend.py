import pytest

import sexp


def test_append_atom():
    t = sexp.parse("(a b)")
    n = t.new_atom("c")
    t[1].parent.append(n)
    assert repr(t) == "(a b c)"


def test_prepend_atom():
    t = sexp.parse("(a b)")
    n = t.new_atom("z")
    t[0].parent.prepend(n)
    assert repr(t) == "(z a b)"


def test_append_to_nested_list():
    t = sexp.parse("(x (a b) y)")
    inner = t[1]
    n = t.new_atom("c")
    inner.append(n)
    assert repr(t) == "(x (a b c) y)"


def test_prepend_to_nested_list():
    t = sexp.parse("(x (a b) y)")
    inner = t[1]
    n = t.new_atom("z")
    inner.prepend(n)
    assert repr(t) == "(x (z a b) y)"


def test_append_list_node():
    t = sexp.parse("(a b)")
    lst = t.new_list()
    child_x = t.new_atom("x")
    child_y = t.new_atom("y")
    lst.append(child_x)
    lst.append(child_y)
    t[1].parent.append(lst)
    assert repr(t) == "(a b (x y))"


def test_append_wrong_tree_raises():
    t1 = sexp.parse("(a b)")
    t2 = sexp.parse("(c d)")
    with pytest.raises(ValueError):
        t1[0].parent.append(t2[0])


def test_prepend_non_node_raises():
    t = sexp.parse("(a b)")
    with pytest.raises(TypeError):
        t[0].parent.prepend("not a node")


def test_append_moves_existing_node():
    t = sexp.parse("(a b c)")
    first = t[0]
    t[2].parent.append(first)
    assert repr(t) == "(b c a)"


# --- SExp root ---


def test_sexp_append_atom_via_root():
    t = sexp.parse("(a b)")
    n = t.new_atom("c")
    t.append(n)
    assert repr(t) == "(a b c)"


def test_sexp_prepend_atom_via_root():
    t = sexp.parse("(a b)")
    n = t.new_atom("z")
    t.prepend(n)
    assert repr(t) == "(z a b)"


def test_sexp_append_list_node_via_root():
    t = sexp.parse("(a b)")
    lst = t.new_list()
    lst.append(t.new_atom("x"))
    t.append(lst)
    assert repr(t) == "(a b (x))"


def test_sexp_prepend_list_node_via_root():
    t = sexp.parse("(a b)")
    lst = t.new_list()
    lst.append(t.new_atom("x"))
    t.prepend(lst)
    assert repr(t) == "((x) a b)"


def test_sexp_append_to_empty_tree():
    t = sexp.parse("()")
    n = t.new_atom("hello")
    t.append(n)
    assert repr(t) == "(hello)"


def test_sexp_append_wrong_tree_raises():
    t1 = sexp.parse("(a b)")
    t2 = sexp.parse("(c d)")
    with pytest.raises(ValueError):
        t1.append(t2[0])


def test_sexp_prepend_non_node_raises():
    t = sexp.parse("(a b)")
    with pytest.raises(TypeError):
        t.prepend("not a node")
