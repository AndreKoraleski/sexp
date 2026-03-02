import sexp


def test_parent_of_child_is_not_none():
    t = sexp.parse("(a b c)")
    assert t[0].parent is not None


def test_parent_chain():
    t = sexp.parse("(a (b c))")
    inner = t[1]
    b = inner[0]
    assert repr(b.parent) == "(b c)"
    assert repr(b.parent.parent) == "(a (b c))"


def test_parent_of_root_child_round_trips_to_top_level():
    t = sexp.parse("(a)")
    a = t[0]
    assert a.parent is not None
    assert a.parent.parent is None


def test_parent_of_direct_child_matches_repr():
    t = sexp.parse("(x y z)")
    assert repr(t[2].parent) == "(x y z)"


def test_parent_is_none_for_root_nodes():
    t = sexp.parse("(a b)")
    assert t[0].parent.parent is None
