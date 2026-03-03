# Arena Allocator

## What the arena is for

The `Arena` is a bump-pointer allocator used by the intern pool to store
string bytes. It is **not** used for the node array itself (which is a
plain `realloc`-grown heap buffer).

## Structure

```c
typedef struct Arena {
    uint8_t      *base;            // start of the current chunk
    size_t        capacity;        // size of the current chunk in bytes
    size_t        position;        // next free offset within the chunk
    struct Arena *previous_chunk;  // linked list of older chunks
} Arena;
```

## How allocation works

```
 base                   position              capacity
  │                        │                     │
  ▼                        ▼                     ▼
  ┌────────────────────────┬─────────────────────┐
  │   allocated objects    │   free space        │
  └────────────────────────┴─────────────────────┘
                            ↑
                     next alloc starts here
```

`arena_alloc(arena, size)`:

1. Rounds `size` up to `ARENA_MAX_ALIGN` (16 bytes on Windows x64,
   `alignof(max_align_t)` elsewhere), ensuring every pointer returned is
   suitably aligned for any fundamental type.
2. If `position + aligned_size <= capacity`, bumps `position` and returns
   the pointer. **O(1), no system call.**
3. Otherwise, allocates a new chunk of `max(2 × capacity, aligned_size)`
   bytes, links the old chunk at `new_chunk->previous_chunk`, and retries.

## Growth chain

```
chunk 3 (newest, active)
  previous_chunk → chunk 2
                     previous_chunk → chunk 1
                                        previous_chunk → NULL
```

When the arena is freed, `arena_free` walks this chain and frees every
chunk in reverse. There are no per-string `free()` calls.

## Why strings go here but nodes don't

Intern pool strings are immutable and never deleted individually — they live
as long as the pool. An arena is perfect for this: allocate fast, free all
at once.

Node arrays, on the other hand, need to grow and compact dynamically as the
tree is mutated. A `realloc`-grown flat buffer is a better fit there.
