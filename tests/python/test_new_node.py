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
