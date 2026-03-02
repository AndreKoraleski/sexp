"""Walk and transform an S-expression tree."""

from sexp import SExpNode, parse


def rename(node: SExpNode, old: str, new: str) -> None:
    """Recursively rename all atoms matching `old` to `new`."""
    if node.is_atom:
        if node.value == old:
            node.value = new
    else:
        for child in node:
            rename(child, old, new)


src = parse("(define (square x) (* x x))")
for node in src:
    rename(node, "x", "n")
print(repr(src))  # (define (square n) (* n n))


def collect_atoms(node: SExpNode, result: set[str]) -> None:
    """Add all atom values in the subtree rooted at node to result."""
    if node.is_atom:
        result.add(node.value)
    else:
        for child in node:
            collect_atoms(child, result)


tree = parse("(let ((x 1) (y 2)) (+ x y))")
atoms: set[str] = set()
for node in tree:
    collect_atoms(node, atoms)
print(sorted(atoms))  # ['+', '1', '2', 'let', 'x', 'y']


# Inline (square 7) → (* 7 7).
# remove() compacts the node array, invalidating all existing SExpNode views;
# re-acquire them from the SExp object afterwards.
tree2 = parse("(print (square 7))")
arg = tree2[1][1].value
tree2[1].remove()

head = tree2.head
mul = tree2.new_list()
for sym in ("*", arg, arg):
    mul.append(tree2.new_atom(sym))
head.parent.insert_after(head, mul)

print(repr(tree2))  # (print (* 7 7))
