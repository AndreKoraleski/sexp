import sexp


def test_clone_returns_independent_sexp():
    t = sexp.parse("(a (b c) d)")
    cloned = t[1].clone()
    assert repr(cloned) == "(b c)"


def test_clone_does_not_affect_original():
    t = sexp.parse("(a (b c) d)")
    cloned = t[1].clone()
    cloned[0].value = "X"
    assert repr(t) == "(a (b c) d)"
    assert repr(cloned) == "(X c)"


def test_clone_original_not_removed():
    t = sexp.parse("(a (b c) d)")
    t[1].clone()
    assert repr(t) == "(a (b c) d)"


def test_clone_atom_node():
    t = sexp.parse("(a)")
    cloned = t[0].clone()
    assert repr(cloned) == "a"


def test_clone_deeply_nested():
    source = "(a (b (c d)) e)"
    t = sexp.parse(source)
    cloned = t[0].parent.clone()
    assert repr(cloned) == source


def test_clone_produces_sexp_type():
    t = sexp.parse("(a b)")
    cloned = t[0].clone()
    assert isinstance(cloned, sexp.SExp)


def test_clone_is_separately_modifiable():
    t = sexp.parse("(a b c)")
    cloned = t[0].parent.clone()
    cloned[0].value = "Z"
    assert repr(t) == "(a b c)"
    assert repr(cloned) == "(Z b c)"
