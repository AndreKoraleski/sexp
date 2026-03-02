import sexp


def test_len_flat_list():
    t = sexp.parse("(a b c)")
    assert len(t) == 3


def test_len_nested_list():
    t = sexp.parse("(a (b c) d)")
    assert len(t) == 3


def test_len_empty_list():
    t = sexp.parse("()")
    assert len(t) == 0


def test_len_atom():
    t = sexp.parse("atom")
    assert len(t) == 0


def test_len_empty_parse():
    t = sexp.parse("")
    assert len(t) == 0


def test_node_len():
    t = sexp.parse("(a (b c d) e)")
    assert len(t[1]) == 3
