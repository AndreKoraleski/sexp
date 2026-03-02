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


# --- SExp root clone ---


def test_sexp_clone_returns_sexp():
    t = sexp.parse("(a b c)")
    c = t.clone()
    assert isinstance(c, sexp.SExp)


def test_sexp_clone_repr_matches():
    t = sexp.parse("(a (b c) d)")
    c = t.clone()
    assert repr(c) == repr(t)


def test_sexp_clone_is_independent():
    t = sexp.parse("(a b c)")
    c = t.clone()
    c[0].value = "Z"
    assert repr(t) == "(a b c)"
    assert repr(c) == "(Z b c)"


def test_sexp_clone_does_not_modify_original():
    t = sexp.parse("(a (b c) d)")
    _ = t.clone()
    assert repr(t) == "(a (b c) d)"


def test_sexp_clone_empty_tree():
    t = sexp.parse("()")
    c = t.clone()
    assert isinstance(c, sexp.SExp)
    assert len(c) == 0


def test_sexp_clone_deep_structure():
    src = "(a (b (c d)) e)"
    t = sexp.parse(src)
    c = t.clone()
    assert repr(c) == src
