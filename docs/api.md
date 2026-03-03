# API Reference

## `parse`

```python
from sexp import parse

def parse(source: str | bytes | bytearray) -> SExp: ...
```

Parse an S-expression and return the tree.

- `source` may be a `str`, `bytes`, or `bytearray`.
- Raises `ValueError` on malformed input (unclosed parenthesis, stray `)`).
- An empty string returns an empty tree (`len == 0`).

---

## `SExp`

The tree object returned by `parse()`. Owns the backing arena and intern
pool. All `SExpNode` views hold a non-owning reference back to their `SExp`.

### Indexing, slicing, and iteration

```python
len(tree)            # number of top-level nodes
tree[0]              # SExpNode at index 0  (IndexError if out of range)
tree[-1]             # last top-level node
tree[1:3]            # list[SExpNode] — slice (supports step, negative indices)
tree["name"]         # first top-level node whose head atom equals "name"
for node in tree:    # iterate over top-level nodes
    ...
```

### Membership

```python
"atom" in tree       # True if any direct child is an atom with that value
node in tree         # True if node is a direct child of tree
```

### Equality

```python
parse("(a b)") == parse("(a b)")  # True — structural comparison
parse("(a b)") != parse("(a x)")  # True
hash(tree)                        # TypeError — SExp is unhashable (mutable)
```

### Properties

```python
tree.head            # first node  (IndexError if empty)
tree.tail            # Iterator[SExpNode] over nodes[1:]
```

### Building nodes

Both methods allocate a node in the tree's arena. The node has no parent
until explicitly attached with `append`, `prepend`, or `insert_after`.

```python
tree.new_atom(value: str) -> SExpNode
tree.new_list()           -> SExpNode
```

### Mutation

```python
tree.append(child: SExpNode) -> None
tree.prepend(child: SExpNode) -> None
tree.insert_after(after: SExpNode | None, child: SExpNode) -> None
```

Same semantics as the `SExpNode` equivalents — operate on top-level children
of the root.

### Serialisation

```python
repr(tree)  # canonical S-expression string, e.g. '(a (b c))'
```

---

## `SExpNode`

A lightweight, non-owning view of one node (atom or list) inside an `SExp`.
Mutations are immediately visible through all other views of the same tree.

### Inspection

```python
node.is_atom          # True for leaf atoms, False for lists
node.parent           # SExpNode | None  (None at tree root)
```

### Atom nodes

```python
node.value            # str — atom string content (TypeError on list nodes)
node.value = "new"    # mutate atom string in-place
```

### List nodes — indexing

```python
len(node)             # number of children
node[0]               # child by integer index
node[-1]              # last child
node[1:3]             # list[SExpNode] — slice (supports step, negative indices)
node["key"]           # first child whose head atom equals "key"
for child in node:    # iterate children
    ...
node.head             # first child (IndexError if empty)
node.tail             # Iterator[SExpNode] over children[1:]
```

### Membership

```python
"atom" in node        # True if any direct child is an atom with that value
child in node         # True if child is a direct child of this node
```

### Equality

```python
node_a == node_b      # structural comparison (works across trees)
node_a != node_b
hash(node)            # TypeError — SExpNode is unhashable (mutable)
```

### Building nodes

Allocate nodes in the owning tree's arena. The node has no parent until
explicitly attached.

```python
node.new_atom(value: str) -> SExpNode
node.new_list()           -> SExpNode
```

### Mutation

```python
node.append(child: SExpNode) -> None
```
Adds `child` as the last child. `child` must belong to the same tree.

```python
node.prepend(child: SExpNode) -> None
```
Inserts `child` as the first child.

```python
node.insert_after(after: SExpNode | None, child: SExpNode) -> None
```
Inserts `child` immediately after `after`. Pass `None` to prepend.

```python
node.remove() -> None
```
Detaches this node and its entire subtree from the owning tree. The node
array is compacted in-place: every node after the removed range shifts to
fill the gap, and all stored indices are rewritten via a remap table.

**All previously obtained `SExpNode` objects for that tree become stale.**
Accessing a stale node raises `RuntimeError`. Re-query from the tree or a
parent node after calling `remove()`.

See [docs/internals/mutation.md](internals/mutation.md) for the exact
compaction algorithm.

### Copying

```python
node.clone() -> SExp
```
Deep-copies this subtree into a new, independent `SExp`. The clone owns its
own node array but shares the intern pool (reference-counted). Mutations to
the clone do not affect the original. Does **not** invalidate views of the
source tree.

```python
node.extract() -> SExp
```
Equivalent to `clone()` followed by `remove()`, performed atomically: if
cloning fails, the source tree is left unchanged. Like `remove()`, it
compacts the source tree and **invalidates all existing views** of it.

### Serialisation

```python
repr(node)   # serialise this subtree only
```
---

## Recipes

### Querying

Use integer indices for positional access and string keys to look up a child
list by its head atom. String indexing is idiomatic for tagged-list formats
(think property lists or simple config files).

```python
tree = parse("(config (host localhost) (port 8080))")
tree["host"][1].value   # 'localhost'
tree["port"][1].value   # '8080'
```

Iterate when you need to inspect every child:

```python
for node in tree:
    if not node.is_atom:
        print(node.head.value, node[1].value)
# host localhost
# port 8080
```

### Transforming

Atom values can be updated in-place through `node.value = ...`. This is safe
to do while iterating, because only the string content changes — the tree
structure stays intact.

```python
tree = parse("(point 1.0 2.0)")
for child in tree:
    if child.is_atom and child.value.replace(".", "").isdigit():
        child.value = str(float(child.value) * 2)
repr(tree)  # '(point 2.0 4.0)'
```

`remove()` and `extract()` compact the node array and **invalidate all
existing `SExpNode` views** of that tree. `append`, `prepend`, and
`insert_after` only rewire connectivity fields (`first_child`, `next_sibling`,
`parent`) — as no node moves, all existing handles stay valid.

```python
tree = parse("(a b c)")
tree[1].remove()   # 'b' removed; all prior SExpNode views are now stale
repr(tree)         # '(a c)'
```

### Building

Allocate nodes with `tree.new_atom()` / `tree.new_list()`, then attach them
with `append`, `prepend`, or `insert_after`. Unattached nodes live in the
arena and are owned by the tree — they need not be attached immediately.

```python
tree = parse("(record)")
record = tree.head.parent

name = tree.new_atom("Alice")
age  = tree.new_atom("30")
record.append(name)
record.append(age)
repr(tree)   # '(record Alice 30)'
```

### Splitting

`extract()` atomically clones a subtree into a new, independent `SExp` and
removes the original node. Use it to break a single document into separate
trees without parsing each piece individually.

```python
doc = parse("(doc (a 1) (b 2) (c 3))")
parts = []
while len(doc) > 0:
    parts.append(doc[0].extract())   # re-query [0] each iteration — prior views are stale
# parts[0] is an SExp containing '(a 1)', etc.
```
