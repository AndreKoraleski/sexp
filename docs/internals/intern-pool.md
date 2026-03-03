# String Intern Pool

## Purpose

All atom strings are deduplicated at parse time. Two atoms with the same
text will always carry the same `AtomId` integer — an equality check on
atoms becomes an integer comparison, with no string hashing or `strcmp`
at lookup time.

## Data structures

```c
typedef uint32_t AtomId;

typedef struct InternHashTable {
    uint64_t *hashes;   // full 64-bit FNV-1a hashes (0 = empty slot)
    AtomId   *atom_ids; // AtomId for each occupied slot
    uint32_t  count;    // number of occupied slots
    uint32_t  capacity; // total slot count, always a power of two
} InternHashTable;

typedef struct InternPool {
    Arena           arena;            // bump allocator for string bytes
    InternHashTable table;            // hash table for content-to-id lookup
    char          **strings;          // strings[id-1] → interned string bytes
    size_t         *string_lengths;   // string_lengths[id-1] → byte length
    uint32_t        strings_capacity; // allocated capacity of strings array
    uint32_t        reference_count;  // number of active SExp references
} InternPool;
```

There is one **global** `InternPool`, shared across all `SExp` objects.
Each `SExp` increments `reference_count` at creation and decrements it when freed;
the pool itself is released when the count reaches zero.

## Hash function — FNV-1a

```c
static const uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
static const uint64_t FNV_PRIME        = 1099511628211ULL;

uint64_t hash = FNV_OFFSET_BASIS;
for (size_t i = 0; i < length; i++) {
    hash ^= (uint8_t)data[i];
    hash *= FNV_PRIME;
}
// ensure result is never zero (zero is used as "empty slot" sentinel)
if (hash == 0) hash = 1;
```

FNV-1a: simple, fast, and good enough for typical atom strings (keywords,
identifiers, numbers). No cryptographic properties needed here.

## Lookup and insertion

```
intern_string(string, length):
  1. hash = fnv1a(string, length)         (non-zero; 0 remapped to 1)
  2. slot = hash & (capacity - 1)         (capacity is always a power of two)
  3. while table.hashes[slot] != 0:
       if hashes[slot] == hash and bytes match: return existing AtomId
       slot = (slot + 1) & (capacity - 1)
  4. copy string bytes into pool arena
  5. store hash and atom_id at slot; store pointer and length in strings[]
  6. if count >= capacity / 2: grow table (double capacity, rehash)
  7. return new AtomId
```

## Thread safety

The global pool is protected by a single lock:

| Platform | Lock type |
|----------|-----------|
| Windows  | `SRWLOCK` (exclusive acquire/release) |
| POSIX    | `pthread_mutex_t` (statically initialised with `PTHREAD_MUTEX_INITIALIZER`) |

The lock is held only for the duration of `intern_string`. Tree traversal
and serialisation do not touch the lock because `AtomId` → string lookups
(`intern_lookup`) read from immutable, already-interned data.
