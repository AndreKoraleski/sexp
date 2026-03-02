"""Split a document into independent sub-trees with extract().

extract() atomically clones and removes a subtree, returning a new SExp.
It invalidates all SExpNode views into the source tree; re-query from the
SExp object after each call.
"""

from sexp import parse

source = """
(doc
  (section  "Introduction"
    (p "S-expressions are a simple, flexible notation.")
    (p "They were popularised by Lisp."))
  (section  "Usage"
    (p "Install with pip.")
    (p "Call parse() to get a tree."))
  (section  "License"
    (p "MIT.")))
"""

doc = parse(source)

sections = []
while len(doc) > 1:
    sections.append(doc[1].extract())  # doc[1]: first child after the head atom

for sec in sections:
    print(f"  [{sec[1].value}]  {repr(sec)[:60]}...")


# Partition atoms and lists
mixed = parse("(a 1 (nested x) b 2 (nested y) c)")
atoms, lists = [], []

while len(mixed) > 0:
    child = mixed[0].extract()
    (atoms if len(child) == 0 else lists).append(repr(child))

print("Atoms:", atoms)  # ['a', '1', 'b', '2', 'c']
print("Lists:", lists)  # ['(nested x)', '(nested y)']
