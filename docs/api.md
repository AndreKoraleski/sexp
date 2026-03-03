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
Detaches this node and its entire subtree from the owning tree.

**All previously obtained `SExpNode` objects for that tree become stale.**
Accessing a stale node raises `RuntimeError`. Re-query nodes from the tree
or a parent node after any mutation.

See [docs/internals/mutation.md](internals/mutation.md) for the exact
compaction algorithm.

### Copying

```python
node.clone() -> SExp
```
Deep-copies this subtree into a new, independent `SExp`. The clone owns its
own node array but shares the intern pool (reference-counted). Mutations to
the clone do not affect the original.

```python
node.extract() -> SExp
```
Equivalent to `clone()` followed by `remove()`, performed atomically: if
cloning fails, the source tree is left unchanged.

### Serialisation

```python
repr(node)   # serialise this subtree only
```
