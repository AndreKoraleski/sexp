import sexp


def test_empty_constructor_returns_sexp():
    assert isinstance(sexp.SExp(), sexp.SExp)


def test_empty_constructor_repr():
    assert repr(sexp.SExp()) == "()"


def test_empty_constructor_len():
    assert len(sexp.SExp()) == 0


def test_empty_constructor_iter():
    assert list(sexp.SExp()) == []


def test_empty_constructor_equals_parsed_empty():
    assert sexp.SExp() == sexp.parse("()")


def test_empty_constructor_is_not_atom():
    assert sexp.SExp().is_atom is False


def test_empty_constructor_parent_is_none():
    assert sexp.SExp().parent is None


def test_empty_constructor_append_atom():
    t = sexp.SExp()
    t.append(t.new_atom("a"))
    assert repr(t) == "(a)"


def test_empty_constructor_prepend_atom():
    t = sexp.SExp()
    t.prepend(t.new_atom("z"))
    assert repr(t) == "(z)"


def test_empty_constructor_build_list():
    t = sexp.SExp()
    t.append(t.new_atom("x"))
    t.append(t.new_atom("y"))
    t.append(t.new_atom("z"))
    assert repr(t) == "(x y z)"


def test_empty_constructor_build_nested():
    t = sexp.SExp()
    inner = t.new_list()
    inner.append(t.new_atom("b"))
    inner.append(t.new_atom("c"))
    t.append(t.new_atom("a"))
    t.append(inner)
    assert repr(t) == "(a (b c))"


def test_empty_constructor_clone():
    t = sexp.SExp()
    t.append(t.new_atom("a"))
    c = t.clone()
    assert isinstance(c, sexp.SExp)
    assert repr(c) == "(a)"


def test_empty_constructor_clone_is_independent():
    t = sexp.SExp()
    t.append(t.new_atom("a"))
    c = t.clone()
    c[0].value = "Z"
    assert repr(t) == "(a)"
    assert repr(c) == "(Z)"


def test_empty_constructor_multiple_independent():
    t1 = sexp.SExp()
    t2 = sexp.SExp()
    t1.append(t1.new_atom("a"))
    assert len(t2) == 0
