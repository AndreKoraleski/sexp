# How It Works

| Page | Contents |
|------|----------|
| [internals/node-array.md](internals/node-array.md) | Flat node array and left-child right-sibling representation |
| [internals/arena.md](internals/arena.md) | Bump-pointer arena allocator |
| [internals/intern-pool.md](internals/intern-pool.md) | Global string intern pool |
| [internals/parse.md](internals/parse.md) | Tokenizer and ParseStack |
| [internals/mutation.md](internals/mutation.md) | Insert, remove, clone, extract |

---

`sexp` is a thin Python wrapper around a C11 library that does all the real
work. Understanding the internals helps explain why the API looks the way it
does — and why it is fast.

---

## The node array

Every `SExp` object owns a flat, heap-allocated array of `Node` structs
(grown via `realloc`). A node is a small fixed-size record:

```
Node {
    type:           atom | list   (2 values, stored in a bit)
    atom_id:        index into the intern pool  (atom nodes only)
    first_child:    index into the node array   (list nodes only)
    next_sibling:   index into the node array
    parent:         index into the node array
}
```

All relationships between nodes are expressed as 32-bit integer indices into
that same array. There are no heap pointers between nodes — the entire tree
is one contiguous allocation.

## The bump-pointer arena

Intern pool string bytes live inside an `Arena`: a bump-pointer allocator
that hands out memory by advancing an offset cursor.

```
[base .............. position ................ capacity]
                         ^
                    next alloc here
```

When the current chunk is exhausted a new chunk of double the capacity is
allocated and linked to the old one via a `previous_chunk` pointer. All
chunks are freed together when `sexp_free` is called. There are no
individual `free()` calls per node.

All allocations are aligned to `ARENA_MAX_ALIGN` (16 bytes on Windows x64,
`alignof(max_align_t)` elsewhere).

## The intern pool

Atom strings are deduplicated at parse time by an intern pool: a global
open-addressing hash table protected by a mutex (`SRWLOCK` on Windows,
`pthread_mutex_t` elsewhere).

When the parser encounters a new string it:

1. Hashes the bytes with FNV-1a.
2. Looks up the hash in the table.
3. If found, returns the existing `AtomId` — no allocation.
4. If not found, copies the string bytes into the pool's own arena and stores
   the new `AtomId`.

Because equal strings always map to the same `AtomId`, equality checks on
atoms reduce to integer comparisons. The pool is reference-counted; each
`SExp` increments the ref count at parse time and decrements it at free time.

## Why `SExpNode` is just an index

A Python `SExpNode` object stores two things:

- a borrowed reference to its owning `SExp`
- a `uint32_t` node index

That is it. Getting `node.head`, `node.is_atom`, `node.value`, etc. all
resolve to a single array lookup:

```c
Node *n = &tree->nodes[index];
```

This is why mutations through any `SExpNode` view are instantly visible
through every other view of the same tree — they all index into the same
array — and why the library avoids per-node memory allocation entirely.

## Parse → tree construction

The parser is a hand-written tokenizer + recursive-descent builder. It reads
tokens left-to-right, maintaining a `ParseStack` of open list frames. When
`(` is encountered a new list node is pushed; when `)` is encountered the
frame is popped and the finished list is attached to its parent. Atom tokens
are interned and turned into atom nodes directly.

The `ParseStack` keeps the first 32 frames inline (no heap allocation for
the overwhelming majority of inputs) and spills to a heap buffer only for
deeper nesting.

## Serialisation

`repr()` walks the node array depth-first, emitting parentheses and
atom strings through a `SerializeFrame` stack that mirrors the parse stack
design — inline buffer first, heap spill only when needed.
