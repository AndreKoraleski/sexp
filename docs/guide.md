# User Guide

## Quick start

```python
from sexp import parse, SExp

tree = parse("(config (host localhost) (port 8080))")

tree["host"][1].value   # 'localhost'
tree["port"][1].value   # '8080'
```

`parse()` returns an `SExp` that is itself the root node.  Every handle
obtained from it — by indexing, iterating, or navigation — is also an `SExp`
pointing into the same shared tree.

---

## Navigating a tree

### By position

```python
node[0]     # first child
node[-1]    # last child
node[1:4]   # list[SExp] — any slice syntax works
node.head   # first child, raises IndexError if empty
node.tail   # iterator over children[1:]
```

### By key

String indexing finds the first direct child that matches:

```python
node["key"]
```

A child matches if it is a **list whose first child is an atom** equal to
`"key"`, or if it is a **bare atom** equal to `"key"`.  This covers both
tagged-list formats and plain symbol lookup:

```python
tree = parse("(point x 1 y 2)")
tree["x"]   # the atom 'x'
tree["y"]   # the atom 'y'

tree = parse("(define (square x) (* x x))")
tree["square"]   # the sub-list (square x)
```

Raises `KeyError` if no match is found.

### Iteration

```python
for child in node:
    print(child.is_atom, repr(child))
```

Iterators capture the current tree version.  Mutating the **structure** of
the tree while iterating (see [Staleness](#staleness)) adds no cost but will
cause a `RuntimeError` the next time the iterator tries to advance if the
tree was compacted.  Mutating atom *values* is safe during iteration.

### Inspecting a node

```python
node.is_atom        # True → leaf atom, False → list
node.value          # str   (TypeError on a list)
len(node)           # number of direct children (always 0 for atoms)
node.parent         # SExp | None  (None only for the root node)
```

---

## Staleness

Two operations — `remove()` and `extract()` — compact the tree's node array.
Every `SExp` handle stores an integer index into that array.  After either
compaction call **all previously-obtained handles for that tree are stale**.
Accessing a stale handle raises `RuntimeError`.

The rule: **re-query from the root after any `remove()` or `extract()`**.

```python
tree = parse("(a b c)")
b = tree[0]          # handle to 'a'
b.remove()           # compacts the array — every handle is now stale
repr(tree)           # OK — tree itself is always revalidated at the root
b.is_atom            # RuntimeError: stale node handle
tree[0].is_atom      # OK — re-queried after remove
```

The other mutation operations — `append`, `prepend`, `insert_after`,
`new_atom`, `new_list`, and `value = ...` — only rewire connectivity fields
or update string content.  They never compact the array, so all existing
handles remain valid.

---

## Mutating a tree

### Changing atom values

Atom values can be updated in-place.  This is safe to do while iterating
because the tree structure does not change.

```python
tree = parse("(point 1.0 2.0)")
for child in tree:
    if child.is_atom:
        child.value = str(float(child.value) * 2)
repr(tree)   # '(point 2.0 4.0)'
```

### Moving nodes

`append`, `prepend`, and `insert_after` accept any node that belongs to the
**same tree**, whether attached or not.  If the node is already attached
somewhere it is detached first, then reattached at the new position.

```python
tree = parse("(a b c)")
tree.insert_after(tree[-1], tree[0])   # moves 'a' after 'c'
repr(tree)   # '(b c a)'
```

`child` must belong to the same tree as `self`.  Passing a node from a
different tree raises `ValueError`.

### Removing nodes

```python
node.remove()
```

Detaches `node` and its entire subtree.  All existing handles become stale.
Re-query everything you need from the root.

```python
tree = parse("(record name Alice age 30)")
# remove 'age' — break immediately; re-query for the next pass
for child in tree:
    if child.is_atom and child.value == "age":
        child.remove()    # all handles stale after this
        break
# now remove '30' — fresh iteration after the prior compaction
for child in tree:
    if child.is_atom and child.value == "30":
        child.remove()
        break
repr(tree)   # '(record name Alice)'
```

---

## Building a tree

Allocate nodes with `new_atom` / `new_list`, then attach them.

```python
tree = SExp()                      # empty root list
name = tree.new_atom("Alice")
age  = tree.new_atom("30")
tag  = tree.new_list()
tag.append(tree.new_atom("record"))
tag.append(name)
tag.append(age)
tree.append(tag)
repr(tree)   # '((record Alice 30))'
```

Unattached nodes live in the tree's arena and are not serialised.  You can
hold onto them and attach them later.

---

## Splitting and merging trees

### `clone()` — non-destructive copy

`clone()` deep-copies a subtree into a new independent tree.  Existing
handles for the source tree are **not** invalidated.

```python
doc = parse("(record (name Alice) (age 30))")
name_copy = doc["name"].clone()   # new SExp rooted at '(name Alice)'
repr(name_copy)   # '(name Alice)'
repr(doc)         # unchanged: '(record (name Alice) (age 30))'
```

### `extract()` — atomic move

`extract()` is `clone()` followed by `remove()` in a single atomic step.
Use it to split a document into independent trees.

```python
doc = parse("(doc (a 1) (b 2) (c 3))")
parts = []
while len(doc) > 0:
    parts.append(doc[0].extract())   # re-query [0] after each extract
repr(parts[0])   # '(a 1)'
repr(parts[1])   # '(b 2)'
repr(parts[2])   # '(c 3)'
```

Because `extract()` compacts the source tree, **re-query `[0]` on each
iteration** rather than caching the handle.

---

## Common patterns

### Tagged-list config

```python
tree = parse("""
(settings
  (config
    (host localhost)
    (port 8080)
    (debug true)))
""")

host  = tree["config"]["host"][1].value
port  = int(tree["config"]["port"][1].value)
debug = tree["config"]["debug"][1].value == "true"
```

### Updating values without rebuilding

```python
tree["config"]["host"][1].value = "example.com"
tree["config"]["port"][1].value = "443"
repr(tree["config"])   # '(config (host example.com) (port 443) (debug true))'
```

### Collecting all atoms at any depth

```python
def atoms(node):
    if node.is_atom:
        yield node.value
    else:
        for child in node:
            yield from atoms(child)

list(atoms(parse("(a (b c) (d (e f)))")))
# ['a', 'b', 'c', 'd', 'e', 'f']
```

### Safe iteration with structural mutation

`remove()` invalidates **every** handle for the tree, including any you already
have in a local list.  The only safe pattern is to restart iteration from
scratch after each removal:

```python
tree = parse("(a 1 b 2 c 3)")
found = True
while found:
    found = False
    for child in tree:
        if child.is_atom and child.value.isdigit():
            child.remove()   # all handles stale — break and restart
            found = True
            break
repr(tree)   # '(a b c)'
```

If you only need to remove *one* node, a single pass is enough:

```python
for child in tree:
    if child.is_atom and child.value == "1":
        child.remove()
        break   # must break, continuing the loop would RuntimeError
```
