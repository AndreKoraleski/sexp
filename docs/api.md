# API Reference

## `parse`

```python
from sexp import parse

def parse(source: str | bytes | bytearray) -> SExp: ...
```

Parse an S-expression and return the root node.

- `source` may be a `str`, `bytes`, or `bytearray`.
- Raises `ParseError` on malformed input: unclosed parenthesis, stray `)`,
  or **more than one top-level form**.
- An empty string returns an empty tree (`len == 0`).

One top-level form means one atom or one list:

```python
parse("hello")         # ok — single atom
parse("(a b c)")       # ok — single list
parse("(a) (b)")       # ParseError — two forms
```

To parse a sequence of multiple forms, wrap them in an outer list first:

```python
forms = parse(b"(" + source + b")")
for form in forms:
    ...
```

---

## `ParseError`

```python
class ParseError(ValueError): ...
```

Raised by `parse()` when the input is structurally malformed.

---

## `SExp`

A handle to one node inside a shared, heap-allocated tree.  Every `SExp`
returned by `parse()` or produced by navigation/allocation methods points
into the same underlying tree.  Mutations through any handle are immediately
visible through every other handle.

`SExp` objects are not hashable.

### Constructor

```python
SExp()
```

Creates an empty root list node in its own new tree.  Equivalent to
`parse("()")`.  Use this as a starting point when building a tree
programmatically without parsing anything.

```python
tree = SExp()
tree.append(tree.new_atom("hello"))
repr(tree)   # '(hello)'
```

### Staleness

Calls to `remove()` and `extract()` compact the tree's node array, which
invalidates the indices stored in every previously-obtained `SExp` handle for
that tree.  Accessing a stale handle raises `RuntimeError`.  Re-query from
the root or a known-valid ancestor immediately after any compaction.

Handles obtained from `append`, `prepend`, `insert_after`, `new_atom`, and
`new_list` are never invalidated by those operations—they only rewire
connectivity fields and do not compact the array.

---

### Indexing, slicing, and iteration

```python
len(node)             # number of direct children
node[0]               # child at index 0  (IndexError if out of range)
node[-1]              # last child
node[1:3]             # list[SExp] — slice (supports step and negative indices)
node["key"]           # first child matching "key" (see below)
for child in node:    # iterate direct children
    ...
```

#### String key lookup

`node["key"]` scans direct children left-to-right. A child matches if:

- it is a **list** whose first child is an atom with value `"key"`, **or**
- it is a **bare atom** with value `"key"`.

```python
tree = parse("(config (host localhost) (port 8080))")
tree["host"]             # returns the (host localhost) node
tree["host"][1].value    # 'localhost'

tree = parse("(flags -v debug)")
tree["debug"]            # returns the atom node 'debug'
```

Raises `KeyError` if no child matches.

### Membership

```python
"atom" in node    # True if any direct child is an atom with that value
child in node     # True if child is a direct child of this node
```

### Equality

```python
parse("(a b)") == parse("(a b)")   # True — structural, works across trees
parse("(a b)") != parse("(a x)")   # True
hash(node)                         # TypeError — SExp is unhashable
```

---

### Properties

```python
node.is_atom     # bool — True for leaf atoms, False for lists
node.parent      # SExp | None — parent node, None only at the root
node.head        # SExp — first child  (IndexError if empty)
node.tail        # Iterator[SExp] over children[1:]
```

### Atom value

```python
node.value          # str — atom string content (TypeError on list nodes)
node.value = "new"  # mutate atom string in-place  (TypeError on list nodes)
```

---

### Node allocation

Both methods allocate a node in the **same tree** as `self`.  The new node
has no parent until explicitly attached with `append`, `prepend`, or
`insert_after`.  Unattached nodes live in the tree's arena and are not
serialised.

```python
node.new_atom(value: str) -> SExp
node.new_list()           -> SExp
```

---

### Mutation

```python
node.append(child: SExp) -> None
```
Adds `child` as the last child.  `child` must belong to the same tree.

```python
node.prepend(child: SExp) -> None
```
Inserts `child` as the first child.

```python
node.insert_after(after: SExp | None, child: SExp) -> None
```
Inserts `child` immediately after `after`.  Pass `None` to prepend.
`after` must be a direct child of `node`.

If `child` is already attached elsewhere in the tree it is moved (detached
first).  All existing handles remain valid after these operations.

---

### Removal

```python
node.remove() -> None
```

Detaches this node and its entire subtree.  The array is compacted in-place:
every node after the removed range shifts down and all stored indices are
rewritten.  **All existing handles for this tree become stale.**
Re-query after calling `remove()`.

Raises `ValueError` if called on the root node.

---

### Copying

```python
node.clone() -> SExp
```

Deep-copies this subtree into a new, independent tree.  The clone owns its
own backing store.  Mutations to the clone do not affect the original and
vice versa.  Existing handles for the source tree are **not** invalidated.

```python
node.extract() -> SExp
```

Equivalent to `clone()` followed by `remove()`, performed atomically: if
cloning fails the source tree is left unchanged.  Returns a new `SExp` whose
root is the extracted subtree.  Like `remove()`, this **invalidates all
existing handles** for the source tree.

Raises `ValueError` if called on the root node.

---

### Serialisation

```python
repr(node)   # canonical S-expression string for this subtree
```

The root of an empty tree serialises to `''` (empty string).
