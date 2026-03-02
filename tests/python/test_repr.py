import sexp


def test_repr_atom():
    t = sexp.parse("atom")
    assert repr(t) == "atom"


def test_repr_flat_list():
    t = sexp.parse("(a b c)")
    assert repr(t) == "(a b c)"


def test_repr_nested_list():
    t = sexp.parse("(a (b c) d)")
    assert repr(t) == "(a (b c) d)"


def test_repr_empty():
    t = sexp.parse("")
    assert repr(t) == ""
