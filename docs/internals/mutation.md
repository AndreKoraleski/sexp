# Mutation: Insert, Remove, Clone, Extract

## Insert

`sexp_insert(tree, parent, after, child)` is the single primitive for
attaching a node to a parent. All of `append`, `prepend`, and
`insert_after` call it:

```
append(parent, child)         → insert(parent, last_child, child)
prepend(parent, child)        → insert(parent, NONE, child)
insert_after(parent, a, ch)   → insert(parent, a, child)
```

Steps:
1. If `child` already has a parent, unlink it first (one pointer patch).
2. Set `child.parent = parent`.
3. If `after == NONE`: set `child.next_sibling = parent.first_child`,
   then `parent.first_child = child` (prepend).
4. Otherwise: splice `child` between `after` and `after.next_sibling`.

Insert is **O(1)** when appending or prepending. `insert_after` with a
known sibling is also O(1). The only O(n) case is `append` when the parent
has n children and you don't already hold the last child's node.

---

## Remove

`sexp_remove(tree, index)` is the most complex operation because it must
maintain the invariant that the node array is compact (no gaps).

### Why compaction is necessary

The node array is indexed by `uint32_t` offsets. If nodes were simply
"tombstoned" in place, the array would develop holes over time. `count`
would diverge from the true number of alive nodes. Traversal would require
skip logic. Serialisation would break.

Instead, every `remove` leaves the array perfectly compact.

### The algorithm

```
1. Unlink the subtree root from its parent's child list.
   (One or two pointer patches — O(1).)

2. BFS-traverse the subtree, building:
     work[]   = node indices in BFS order
     removed[i] = 1 if node i is being removed

3. Build index remap:
     remap[i] = new position after compaction
              = SEXP_NULL_INDEX if node i is removed

   Example (removing indices 2 and 3 from a 5-node tree):
     old: 0  1  2  3  4
     new: 0  1  -  -  2
     remap = [0, 1, MAX, MAX, 2]

4. Compact in-place:
   For each surviving node i (in order):
     - Translate node.parent, first_child, next_sibling through remap
     - Move node to nodes[remap[i]]

5. tree.count -= removed_count
```

### What this means for Python code

**All `SExpNode` objects obtained before a `remove()` are invalidated.**

An `SExpNode` holds a `uint32_t index`. After compaction that index either:
- no longer exists (the node was removed), or
- points to a *different* node (everything above the removed range shifted
  down).

After calling `node.remove()`, **discard all `SExpNode` references to that
tree** and re-query from the root or a parent node you hold separately:

```python
tree = parse("(a (b c) d)")
sub  = tree[0]          # (a ...)
b    = sub[0]           # (b c)

b.remove()              # compacts the node array

# ✗ BAD — `sub` now holds a stale index
#   sub[0].value  →  undefined behaviour

# ✓ GOOD — re-query from the tree
sub2 = tree[0]          # fresh reference to (a d)
sub2[0].value           # 'd'
```

---

## Clone

`node.clone()` deep-copies the subtree into a new `SExp`:

1. Allocates a fresh `SExp` with a new node array.
2. BFS-traverses the source subtree, copying each `Node` struct and
   translating indices relative to the new array's base.
3. Interns are **shared** — the clone increments the global pool's
   `refcount` rather than copying string bytes. Atom strings are immutable
   once interned, so sharing is safe.

Clone is O(n) in the size of the subtree.

---

## Extract

`node.extract()` is a transactional `clone` + `remove`:

1. Try to clone the subtree into a new `SExp`.
2. If cloning fails (out of memory), return `NULL` — the source tree is
   unchanged.
3. If cloning succeeds, `remove` the original subtree.

The atomicity guarantee: either you get a new `SExp` and the subtree is
gone from the original, or the original is untouched and `None` is returned.

Same index-invalidation rules as `remove` apply after a successful
`extract`.
