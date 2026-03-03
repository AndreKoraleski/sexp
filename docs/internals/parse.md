# Parser and Serialiser

## Parsing overview

```
source string
      │
      ▼
 Tokenizer          (hand-written, operates on raw bytes)
      │  Token stream: LPAREN, RPAREN, ATOM, END
      ▼
 Parser             (drives tree construction)
      │  uses ParseStack to track open list frames
      ▼
 SExp node array    (flat, fully linked)
```

## Tokenizer

`src/parse/tokenizer.c` scans the source byte-by-byte with no allocations.
It recognises:

| Token | Bytes |
|-------|-------|
| `LPAREN` | `(` |
| `RPAREN` | `)` |
| `ATOM` | any run of non-whitespace, non-paren bytes |
| `END` | end of input |
| `ERROR` | unexpected character or truncated input |

Atoms are returned as a `(pointer, length)` slice into the original source
buffer — no copy is made at tokenise time.

## ParseStack

The parser maintains a stack of open list frames. When `(` is seen, a new
list node is created and pushed. When `)` is seen, the frame is popped and
the finished list is linked into its parent.

`ParseStack` keeps the first 32 frames inline inside the struct — no heap
allocation for the overwhelming majority of inputs. It spills to a
`malloc`-grown buffer only when nesting exceeds 32 levels:

```c
typedef struct ParseStack {
    ParseFrame  inline_buffer[PARSE_STACK_INLINE_CAPACITY]; // 32 frames inline
    ParseFrame *data;      // points to inline_buffer, or heap allocation on spill
    uint32_t    top;       // number of frames currently on the stack
    uint32_t    capacity;  // current capacity (starts at INLINE_CAPACITY)
    int         heap;      // non-zero if data is a heap allocation
} ParseStack;
```

On spill, the inline buffer is `memcpy`-d into a freshly `malloc`-d heap
buffer. Subsequent overflows use `realloc` (doubling each time).

## Atom interning during parse

When an `ATOM` token is produced, the parser immediately interns the string:

```
token slice → intern_lookup_or_insert() → AtomId
```

The `AtomId` is stored in the node; the source string slice is not retained.

## Serialiser

`repr()` walks the node array depth-first using a `SerializeFrame` stack
that mirrors the `ParseStack` design: 32 frames inline, heap spill on
deeper trees. Output is written into a `PyUnicodeWriter` (Python ≥ 3.12) or
a comparable buffer on older versions.

Atom strings are retrieved in O(1) from the intern pool via `intern_lookup(AtomId)`.
