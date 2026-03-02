import sexp


def test_round_trip():
    source = "(player (pos 1 2) (vel 3 4))"
    t = sexp.parse(source)
    assert repr(t) == source


def test_round_trip_nested():
    source = "(a (b (c d)) e)"
    t = sexp.parse(source)
    assert repr(t) == source


def test_round_trip_deeply_nested():
    source = "(a (b (c (d e))) f)"
    t = sexp.parse(source)
    assert repr(t) == source


def test_round_trip_atom():
    t = sexp.parse("hello")
    assert repr(t) == "hello"


def test_round_trip_empty_list():
    t = sexp.parse("()")
    assert repr(t) == "()"
