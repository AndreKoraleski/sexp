import pytest

import sexp


def test_new_atom_creates_unattached():
    t = sexp.parse("(a b)")
    n = t.new_atom("x")
    assert n.is_atom
    assert n.value == "x"
    assert len(t) == 2


def test_new_list_creates_unattached():
    t = sexp.parse("(a b)")
    lst = t.new_list()
    assert not lst.is_atom
    assert len(lst) == 0
    assert len(t) == 2


def test_new_atom_then_append_round_trips():
    t = sexp.parse("(a)")
    n = t.new_atom("b")
    t[0].parent.append(n)
    assert repr(t) == "(a b)"


def test_new_list_then_populate_and_attach():
    t = sexp.parse("(a)")
    lst = t.new_list()
    child = t.new_atom("x")
    lst.append(child)
    t[0].parent.append(lst)
    assert repr(t) == "(a (x))"


def test_new_atom_does_not_change_tree_len():
    t = sexp.parse("(a b c)")
    _ = t.new_atom("z")
    assert len(t) == 3


# --- SExpNode.new_atom / new_list ---


def test_node_new_atom_creates_unattached_atom():
    t = sexp.parse("(a b)")
    node = t[0].new_atom("x")
    assert node.is_atom
    assert node.value == "x"
    assert len(t) == 2  # tree unchanged


def test_node_new_list_creates_unattached_list():
    t = sexp.parse("(a b)")
    lst = t[0].new_list()
    assert not lst.is_atom
    assert len(lst) == 0
    assert len(t) == 2


def test_node_new_atom_then_attach():
    t = sexp.parse("(a b)")
    inner = t[1]  # 'b'
    # allocate via inner node, attach at root
    n = inner.new_atom("c")
    t.append(n)
    assert repr(t) == "(a b c)"


def test_node_new_list_then_populate_and_attach():
    t = sexp.parse("(a)")
    lst = t[0].new_list()
    child = t[0].new_atom("x")
    lst.append(child)
    t.append(lst)
    assert repr(t) == "(a (x))"


def test_node_new_atom_belongs_to_same_tree():
    """A node allocated via SExpNode.new_atom can be appended to the same tree."""
    t = sexp.parse("(a b c)")
    n = t[2].new_atom("d")
    t.append(n)
    assert repr(t) == "(a b c d)"


def test_node_new_list_cross_tree_raises():
    """Nodes from different trees cannot be mixed."""
    t1 = sexp.parse("(a b)")
    t2 = sexp.parse("(c d)")
    foreign = t2[0].new_atom("x")
    with pytest.raises(ValueError):
        t1.append(foreign)
