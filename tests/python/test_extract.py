import sexp


def test_extract_returns_subtree_as_sexp():
    t = sexp.parse("(a (b c) d)")
    extracted = t[1].extract()
    assert repr(extracted) == "(b c)"


def test_extract_removes_from_original():
    t = sexp.parse("(a (b c) d)")
    t[1].extract()
    assert repr(t) == "(a d)"


def test_extract_roundtrip():
    t = sexp.parse("(a (b c) d)")
    extracted = t[1].extract()
    assert repr(t) == "(a d)"
    assert repr(extracted) == "(b c)"


def test_extract_atom():
    t = sexp.parse("(a b c)")
    extracted = t[1].extract()
    assert repr(extracted) == "b"
    assert repr(t) == "(a c)"


def test_extract_independence():
    t = sexp.parse("(a (b c) d)")
    extracted = t[1].extract()
    extracted[0].value = "X"
    assert repr(extracted) == "(X c)"
    assert repr(t) == "(a d)"


def test_extract_produces_sexp_type():
    t = sexp.parse("(a b)")
    extracted = t[0].extract()
    assert isinstance(extracted, sexp.SExp)


def test_extract_first_child():
    t = sexp.parse("(a b c)")
    extracted = t[0].extract()
    assert repr(extracted) == "a"
    assert repr(t) == "(b c)"
