import pytest

import sexp


# --- parse ---
def test_parse_str():
    t = sexp.parse("(a b)")
    assert repr(t) == "(a b)"


def test_parse_bytes():
    t = sexp.parse(b"(a b)")
    assert repr(t) == "(a b)"


def test_parse_bytearray():
    t = sexp.parse(bytearray(b"(a b)"))
    assert repr(t) == "(a b)"


def test_parse_invalid_raises():
    with pytest.raises(ValueError):
        sexp.parse("(unclosed")


def test_parse_stray_close_raises():
    with pytest.raises(ValueError):
        sexp.parse(")")


def test_parse_empty_string():
    t = sexp.parse("")
    assert len(t) == 0
    assert list(t) == []


# --- repr ---
def test_repr_atom():
    t = sexp.parse("atom")
    assert repr(t) == "atom"


def test_repr_flat_list():
    t = sexp.parse("(a b c)")
    assert repr(t) == "(a b c)"


def test_repr_nested_list():
    t = sexp.parse("(a (b c) d)")
    assert repr(t) == "(a (b c) d)"


# --- len ---
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


def test_node_len():
    t = sexp.parse("(a (b c d) e)")
    assert len(t[1]) == 3


# --- int indexing ---
def test_getitem_int():
    t = sexp.parse("(a b c)")
    assert t[0].value == "a"
    assert t[1].value == "b"
    assert t[2].value == "c"


def test_getitem_negative_index():
    t = sexp.parse("(a b c)")
    assert t[-1].value == "c"
    assert t[-2].value == "b"
    assert t[-3].value == "a"


def test_getitem_out_of_range():
    t = sexp.parse("(a b c)")
    with pytest.raises(IndexError):
        t[3]
    with pytest.raises(IndexError):
        t[-4]


def test_node_getitem_int():
    t = sexp.parse("(a (x y z) e)")
    inner = t[1]
    assert inner[0].value == "x"
    assert inner[2].value == "z"


def test_node_getitem_negative():
    t = sexp.parse("(a (x y z) e)")
    inner = t[1]
    assert inner[-1].value == "z"


def test_node_getitem_out_of_range():
    t = sexp.parse("(a (x y) e)")
    inner = t[1]
    with pytest.raises(IndexError):
        inner[5]


# --- str indexing ---
def test_getitem_str():
    t = sexp.parse("(player (pos 1 2) (vel 3 4))")
    node = t["pos"]
    assert repr(node) == "(pos 1 2)"


def test_getitem_str_not_found():
    t = sexp.parse("(player (pos 1 2))")
    with pytest.raises(KeyError):
        t["vel"]


def test_getitem_str_nested():
    t = sexp.parse("(player (pos 1 2) (vel 3 4))")
    node = t["vel"]
    assert repr(node[1]) == "3"


def test_getitem_str_multiple():
    t = sexp.parse("(player (pos 1 2) (vel 3 4) (pos 5 6))")
    node = t["pos"]
    assert repr(node) == "(pos 1 2)"


# --- iter ---
def test_iter_all_children():
    t = sexp.parse("(a b c)")
    values = [node.value for node in t]
    assert values == ["a", "b", "c"]


def test_iter_empty():
    t = sexp.parse("()")
    assert list(t) == []


def test_iter_nested():
    t = sexp.parse("(a (b c) d)")
    values = [repr(node) for node in t]
    assert values == ["a", "(b c)", "d"]


def test_node_iter():
    t = sexp.parse("(a (b c d) e)")
    inner = t[1]
    values = [n.value for n in inner]
    assert values == ["b", "c", "d"]


# --- head and tail ---
def test_head():
    t = sexp.parse("(a b c)")
    assert t.head.value == "a"


def test_head_empty_raises():
    t = sexp.parse("()")
    with pytest.raises(IndexError):
        _ = t.head


def test_tail():
    t = sexp.parse("(a b c)")
    values = [node.value for node in t.tail]
    assert values == ["b", "c"]


def test_tail_single_child():
    t = sexp.parse("(a)")
    assert list(t.tail) == []


def test_node_head():
    t = sexp.parse("(a (b c) d)")
    inner = t[1]
    assert inner.head.value == "b"


def test_node_head_empty_raises():
    t = sexp.parse("(a () d)")
    empty = t[1]
    with pytest.raises(IndexError):
        _ = empty.head


def test_node_tail():
    t = sexp.parse("(a (x y z) d)")
    inner = t[1]
    values = [n.value for n in inner.tail]
    assert values == ["y", "z"]


def test_node_tail_single_child():
    t = sexp.parse("(a (x) d)")
    inner = t[1]
    assert list(inner.tail) == []


# --- node value ---
def test_node_value_atom():
    t = sexp.parse("(a b c)")
    assert t[0].value == "a"


def test_node_value_list_raises():
    t = sexp.parse("(a (b c) d)")
    with pytest.raises(TypeError):
        _ = t[1].value


def test_node_value_set():
    t = sexp.parse("(a b c)")
    t[0].value = "x"
    assert repr(t) == "(x b c)"


def test_node_value_set_nested():
    t = sexp.parse("(a (b c) d)")
    t[1][0].value = "X"
    assert repr(t) == "(a (X c) d)"


def test_node_value_set_non_string_raises():
    t = sexp.parse("(a b c)")
    with pytest.raises(TypeError):
        t[0].value = 42


# --- round trip ---
def test_round_trip():
    src = "(player (pos 1 2) (vel 3 4))"
    t = sexp.parse(src)
    assert repr(t) == src


def test_round_trip_nested():
    src = "(a (b (c d)) e)"
    t = sexp.parse(src)
    assert repr(t) == src


def test_round_trip_deeply_nested():
    src = "(a (b (c (d e))) f)"
    t = sexp.parse(src)
    assert repr(t) == src


# --- is_atom ---
def test_is_atom_true():
    t = sexp.parse("(a b)")
    assert t[0].is_atom is True


def test_is_atom_false_on_list():
    t = sexp.parse("(a (b c))")
    assert t[1].is_atom is False


# --- parent ---
def test_parent_of_top_level_child_is_none():
    t = sexp.parse("(a b c)")
    assert t[0].parent is not None


def test_parent_chain():
    t = sexp.parse("(a (b c))")
    inner = t[1]
    b = inner[0]
    assert repr(b.parent) == "(b c)"
    assert repr(b.parent.parent) == "(a (b c))"


def test_parent_of_root_node_is_none():
    t = sexp.parse("(a)")
    a = t[0]
    assert a.parent is not None
    assert a.parent.parent is None


# --- remove ---
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


# --- append / prepend ---
def test_append_atom():
    t = sexp.parse("(a b)")
    n = t.new_atom("c")
    t[1].parent.append(n)
    assert repr(t) == "(a b c)"


def test_prepend_atom():
    t = sexp.parse("(a b)")
    n = t.new_atom("z")
    t[0].parent.prepend(n)
    assert repr(t) == "(z a b)"


def test_append_to_nested_list():
    t = sexp.parse("(x (a b) y)")
    inner = t[1]
    n = t.new_atom("c")
    inner.append(n)
    assert repr(t) == "(x (a b c) y)"


def test_prepend_to_nested_list():
    t = sexp.parse("(x (a b) y)")
    inner = t[1]
    n = t.new_atom("z")
    inner.prepend(n)
    assert repr(t) == "(x (z a b) y)"


def test_append_list_node():
    t = sexp.parse("(a b)")
    lst = t.new_list()
    c1 = t.new_atom("x")
    c2 = t.new_atom("y")
    lst.append(c1)
    lst.append(c2)
    t[1].parent.append(lst)
    assert repr(t) == "(a b (x y))"


def test_append_wrong_tree_raises():
    t1 = sexp.parse("(a b)")
    t2 = sexp.parse("(c d)")
    with pytest.raises(ValueError):
        t1[0].parent.append(t2[0])


def test_prepend_non_node_raises():
    t = sexp.parse("(a b)")
    with pytest.raises(TypeError):
        t[0].parent.prepend("not a node")


# --- new_atom / new_list ---
def test_new_atom_creates_unattached():
    t = sexp.parse("(a b)")
    n = t.new_atom("x")
    assert n.is_atom
    assert n.value == "x"
    assert len(t) == 2


def test_new_list_creates_unattached():
    t = sexp.parse("(a b)")
    lst = t.new_list()
    assert not lst.is_atom
    assert len(lst) == 0
    assert len(t) == 2


def test_new_atom_then_append_round_trips():
    t = sexp.parse("(a)")
    n = t.new_atom("b")
    t[0].parent.append(n)
    assert repr(t) == "(a b)"
