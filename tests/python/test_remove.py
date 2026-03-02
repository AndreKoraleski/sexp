import sexp


def test_remove_middle_child():
    t = sexp.parse("(a b c)")
    t[1].remove()
    assert repr(t) == "(a c)"


def test_remove_first_child():
    t = sexp.parse("(a b c)")
    t[0].remove()
    assert repr(t) == "(b c)"


def test_remove_last_child():
    t = sexp.parse("(a b c)")
    t[2].remove()
    assert repr(t) == "(a b)"


def test_remove_only_child():
    t = sexp.parse("(a)")
    t[0].remove()
    assert len(t) == 0


def test_remove_nested():
    t = sexp.parse("(a (b c d) e)")
    t[1][1].remove()
    assert repr(t) == "(a (b d) e)"


def test_remove_entire_inner_list():
    t = sexp.parse("(a (b c) d)")
    t[1].remove()
    assert repr(t) == "(a d)"


def test_remove_preserves_siblings():
    t = sexp.parse("(a b c d)")
    t[2].remove()
    assert repr(t) == "(a b d)"
    assert len(t) == 3


# --- SExp root ---


def test_sexp_root_not_removable():
    import pytest

    t = sexp.parse("(a b c)")
    with pytest.raises(AttributeError):
        t.remove()  # type: ignore[attr-defined]


def test_sexp_root_has_no_remove_method():
    t = sexp.parse("(a b c)")
    assert not hasattr(t, "remove")
    assert repr(t) == "(a b c)"
