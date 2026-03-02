import pytest

import sexp

# ── SExp slicing ─────────────────────────────────────────────────────────────


def test_slice_basic():
    t = sexp.parse("(a b c d e)")
    result = t[1:3]
    assert [n.value for n in result] == ["b", "c"]


def test_slice_from_start():
    t = sexp.parse("(a b c)")
    assert [n.value for n in t[:2]] == ["a", "b"]


def test_slice_to_end():
    t = sexp.parse("(a b c)")
    assert [n.value for n in t[1:]] == ["b", "c"]


def test_slice_step():
    t = sexp.parse("(a b c d e)")
    assert [n.value for n in t[::2]] == ["a", "c", "e"]


def test_slice_reverse():
    t = sexp.parse("(a b c)")
    assert [n.value for n in t[::-1]] == ["c", "b", "a"]


def test_slice_negative_indices():
    t = sexp.parse("(a b c d)")
    assert [n.value for n in t[-2:]] == ["c", "d"]
    assert [n.value for n in t[:-1]] == ["a", "b", "c"]


def test_slice_empty_result():
    t = sexp.parse("(a b c)")
    assert t[5:10] == []


def test_slice_full():
    t = sexp.parse("(a b c)")
    result = t[:]
    assert [n.value for n in result] == ["a", "b", "c"]


def test_slice_returns_list():
    t = sexp.parse("(a b c)")
    assert isinstance(t[:], list)


def test_slice_nodes_are_live():
    t = sexp.parse("(a b c)")
    nodes = t[0:2]
    nodes[0].value = "x"
    assert t[0].value == "x"


# ── SExpNode slicing ──────────────────────────────────────────────────────────


def test_node_slice_basic():
    t = sexp.parse("((a b c d))")
    inner = t[0]
    assert [n.value for n in inner[1:3]] == ["b", "c"]


def test_node_slice_step():
    t = sexp.parse("((a b c d e))")
    inner = t[0]
    assert [n.value for n in inner[::2]] == ["a", "c", "e"]


def test_node_slice_reverse():
    t = sexp.parse("((a b c))")
    inner = t[0]
    assert [n.value for n in inner[::-1]] == ["c", "b", "a"]


def test_node_slice_empty():
    t = sexp.parse("((a b c))")
    inner = t[0]
    assert inner[10:20] == []


def test_slice_type_error():
    t = sexp.parse("(a b c)")
    with pytest.raises(TypeError):
        t[1.5]  # type: ignore[index]


def test_atom_node_slice_returns_empty():
    t = sexp.parse("(a b c)")
    atom = t[0]
    assert atom[0:2] == []
    assert atom[:] == []
    assert atom[::-1] == []
