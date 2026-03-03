# Internals

How `sexp` works under the hood.

| Page | Contents |
|------|----------|
| [node-array.md](node-array.md) | Flat node array and left-child right-sibling tree representation |
| [arena.md](arena.md) | Bump-pointer arena allocator — O(1) allocation, bulk free |
| [intern-pool.md](intern-pool.md) | Global string intern pool — deduplication, FNV-1a hash, thread safety |
| [parse.md](parse.md) | Tokenizer, ParseStack, tree construction |
| [mutation.md](mutation.md) | Insert, remove (array compaction), clone, extract |

Reading order: node-array → arena → intern-pool → parse → mutation.
