import pytest

import sexp

# ── SExp structural equality ──────────────────────────────────────────────────


def test_sexp_eq_same_content():
    assert sexp.parse("(a b c)") == sexp.parse("(a b c)")


def test_sexp_eq_nested():
    assert sexp.parse("(a (b c) d)") == sexp.parse("(a (b c) d)")


def test_sexp_ne_different_atoms():
    assert sexp.parse("(a b c)") != sexp.parse("(a b x)")


def test_sexp_ne_different_length():
    assert sexp.parse("(a b)") != sexp.parse("(a b c)")


def test_sexp_ne_different_structure():
    assert sexp.parse("(a (b c))") != sexp.parse("((a b) c)")


def test_sexp_eq_empty():
    assert sexp.parse("") == sexp.parse("")


def test_sexp_ne_empty_vs_nonempty():
    assert sexp.parse("") != sexp.parse("(a)")


def test_sexp_eq_single_atom():
    assert sexp.parse("(a)") == sexp.parse("(a)")


def test_sexp_ne_returns_false_for_equal():
    t1 = sexp.parse("(a b)")
    t2 = sexp.parse("(a b)")
    assert (t1 != t2) is False


def test_sexp_eq_not_implemented_for_other_types():
    t = sexp.parse("(a b)")
    assert t != "(a b)"
    assert t != 42
    assert t != None  # noqa: E711


# ── SExp unhashable ───────────────────────────────────────────────────────────


def test_sexp_unhashable():
    t = sexp.parse("(a b)")
    with pytest.raises(TypeError, match="unhashable"):
        hash(t)


def test_sexp_not_usable_as_dict_key():
    t = sexp.parse("(a b)")
    with pytest.raises(TypeError):
        _ = {t: 1}


# ── SExpNode structural equality ──────────────────────────────────────────────


def test_node_eq_same_atom():
    t1 = sexp.parse("(a b)")
    t2 = sexp.parse("(a b)")
    assert t1[0] == t2[0]


def test_node_ne_different_atom():
    t = sexp.parse("(a b)")
    assert t[0] != t[1]


def test_node_eq_same_subtree():
    t1 = sexp.parse("((a b c))")
    t2 = sexp.parse("((a b c))")
    assert t1[0] == t2[0]


def test_node_ne_different_subtree():
    t1 = sexp.parse("((a b))")
    t2 = sexp.parse("((a x))")
    assert t1[0] != t2[0]


def test_node_ne_atom_vs_list():
    t = sexp.parse("(a (b))")
    assert t[0] != t[1]


def test_node_eq_not_implemented_for_other_types():
    t = sexp.parse("(a b)")
    assert t[0] != "a"
    assert t[0] != 0


# ── SExpNode unhashable ───────────────────────────────────────────────────────


def test_node_unhashable():
    t = sexp.parse("(a b)")
    with pytest.raises(TypeError, match="unhashable"):
        hash(t[0])


# ── Stale-node equality ───────────────────────────────────────────────────────


def test_stale_node_eq_raises_runtime_error():
    t = sexp.parse("(a b c)")
    node_a = t[0]
    t[2].remove()
    with pytest.raises(RuntimeError):
        _ = node_a == t[0]


def test_stale_node_ne_raises_runtime_error():
    t = sexp.parse("(a b c)")
    node_a = t[0]
    t[2].remove()
    with pytest.raises(RuntimeError):
        _ = node_a != t[0]
