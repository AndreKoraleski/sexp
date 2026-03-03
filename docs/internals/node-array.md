# The Node Array

## What a node is

Every node in a `sexp` tree is a fixed-size C struct:

```c
typedef struct Node {
    NodeType type;         // NODE_ATOM, NODE_LIST, or NODE_INVALID
    AtomId   atom_id;      // interned string id  (atom nodes only)
    uint32_t first_child;  // index of first child (list nodes only)
    uint32_t next_sibling; // index of next sibling
    uint32_t parent;       // index of parent
} Node;
```

All link fields use `uint32_t` indices rather than pointers. The sentinel
value `SEXP_NULL_INDEX = UINT32_MAX` means "absent".

## The flat array

Every `SExp` owns one flat, heap-allocated `Node *nodes` array. All nodes
for that tree live contiguously in that array, indexed from 0:

```
 index:   0        1        2        3        4
       ┌────────┬────────┬────────┬────────┬────────┐
nodes: │ (root) │   a    │  (sub) │   b    │   c    │
       └────────┴────────┴────────┴────────┴────────┘
```

No heap pointers connect nodes to each other — the entire tree is a single
allocation. This means:

- Traversal is cache-friendly (sequential array access).
- There are no individual `malloc`/`free` calls per node.
- The whole tree is freed with one `free(tree->nodes)`.

## Left-child right-sibling representation

`sexp` uses the
[left-child right-sibling](https://en.wikipedia.org/wiki/Left-child_right-sibling_binary_tree)
(LCRS) encoding to represent arbitrary-arity trees with a fixed node size.
Each list node stores:

- `first_child` → the index of its **first** child
- Each child's `next_sibling` → the index of the **next** child

For the expression `(define (square x) (* x x))`:

```
 0: LIST  first_child=1  parent=NONE
 1: ATOM  "define"       next_sibling=2   parent=0
 2: LIST  first_child=3  next_sibling=4   parent=0    → (square x)
 3: ATOM  "square"       next_sibling=5   parent=2
 4: LIST  first_child=6  parent=0                     → (* x x)
 5: ATOM  "x"            next_sibling=NONE parent=2
 6: ATOM  "*"            next_sibling=7   parent=4
 7: ATOM  "x"            next_sibling=8   parent=4
 8: ATOM  "x"            next_sibling=NONE parent=4
```

## What `SExpNode` is in Python

A Python `SExpNode` stores exactly two things:

```c
SExpNodeObject {
    PyObject_HEAD
    SExpObject *owner;  // borrowed ref to the SExp
    uint32_t    index;  // index into owner->tree.nodes[]
}
```

Every property access (`node.is_atom`, `node.value`, `node.head`, …) is a
single array lookup:

```c
Node *n = &owner->tree.nodes[index];
```

No Python object wraps node data — the node struct is read directly in C
and the result returned to Python. This is why mutations through any
`SExpNode` view are instantly visible through every other view of the same
tree: they all write into the same `nodes[]` array.

## Index stability

Indices are stable as long as the tree is not structurally modified. Any
operation that **removes** nodes compacts the array and reassigns all
indices — see [mutation.md](mutation.md).
