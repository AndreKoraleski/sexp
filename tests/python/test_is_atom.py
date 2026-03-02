import sexp


def test_is_atom_true():
    t = sexp.parse("(a b)")
    assert t[0].is_atom is True


def test_is_atom_false_on_list():
    t = sexp.parse("(a (b c))")
    assert t[1].is_atom is False


def test_is_atom_on_nested_atom():
    t = sexp.parse("(a (b c))")
    assert t[1][0].is_atom is True


def test_is_atom_returned_as_bool():
    t = sexp.parse("(a b)")
    assert type(t[0].is_atom) is bool


# --- SExp root ---


def test_sexp_is_atom_always_false():
    t = sexp.parse("(a b c)")
    assert t.is_atom is False


def test_sexp_is_atom_false_on_empty_tree():
    t = sexp.parse("()")
    assert t.is_atom is False


def test_sexp_is_atom_returns_bool():
    t = sexp.parse("(a)")
    assert type(t.is_atom) is bool
