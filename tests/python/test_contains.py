import sexp

# ── SExp.__contains__ ─────────────────────────────────────────────────────────


def test_contains_atom_str_present():
    t = sexp.parse("(a b c)")
    assert "a" in t
    assert "b" in t
    assert "c" in t


def test_contains_atom_str_absent():
    t = sexp.parse("(a b c)")
    assert "x" not in t


def test_contains_str_matches_atom_not_list_head():
    t = sexp.parse("((a b) c)")
    assert "c" in t
    assert "a" not in t


def test_contains_node_present():
    t = sexp.parse("(a b c)")
    node = t[1]
    assert node in t


def test_contains_node_absent_different_tree():
    t1 = sexp.parse("(a b c)")
    t2 = sexp.parse("(a b c)")
    assert t2[0] not in t1


def test_contains_stale_node_is_absent():
    t = sexp.parse("(a b c)")
    node = t[0]
    t[2].remove()
    assert node not in t


def test_contains_empty_tree():
    t = sexp.parse("")
    assert "a" not in t


def test_contains_empty_list():
    t = sexp.parse("()")
    assert "a" not in t


# ── SExpNode.__contains__ ─────────────────────────────────────────────────────


def test_node_contains_atom_str_present():
    t = sexp.parse("((a b c))")
    inner = t[0]
    assert "a" in inner
    assert "c" in inner


def test_node_contains_atom_str_absent():
    t = sexp.parse("((a b c))")
    inner = t[0]
    assert "x" not in inner


def test_node_contains_child_node():
    t = sexp.parse("((a b c))")
    inner = t[0]
    child = inner[1]
    assert child in inner


def test_node_contains_non_child_node():
    t = sexp.parse("((a b) (c d))")
    first = t[0]
    node_in_second = t[1][0]
    assert node_in_second not in first


def test_atom_node_contains_str_is_false():
    t = sexp.parse("(a)")
    atom = t[0]
    assert "a" not in atom
    assert "x" not in atom


def test_atom_node_contains_node_is_false():
    t = sexp.parse("(a b)")
    atom = t[0]
    other = t[1]
    assert other not in atom
