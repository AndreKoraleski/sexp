# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.1] - 2026-03-04

### Changed

- `ParseError` messages are now specific to the failure reason:
  - Unclosed parenthesis → `"unclosed parenthesis"`
  - Stray closing parenthesis → `"stray closing parenthesis"`
  - Multiple top-level forms → `"multiple top-level forms: wrap input in '(...)' to parse as a sequence"`
  - Other/internal → `"failed to parse S-expression"` (unchanged)
- Added `parse_error` field (`SExpParseError` enum) to the C `SExp` struct to carry the reason code.

## [1.1.0] - 2026-03-04

### Changed

- `parse()` now raises `ParseError` when the input contains more than one
  top-level form (e.g. `"(a 1)(b 2)"`). Previously, extra forms were silently
  unreachable. An S-expression is a single form by definition; callers that
  need to iterate multiple forms should wrap the input: `parse(b"(" + src + b")")`.

## [1.0.0] - 2026-03-02

First stable release.

### Added

#### Core library (C)
- Arena bump-pointer allocator with alignment support and thread-safe intern pool
- Flat node-array tree: `SExpTree` and `SExpNode` backed by a single allocation
- Full mutation API: append, prepend, insert-after, remove, extract, clone, value-set
- Recursive structural equality comparison
- Stale-node detection: mutating operations invalidate handles obtained before the change
- Serializer producing canonical S-expression strings
- Parser with an explicit parse-stack (O(n) — replaces earlier O(n²) tail-walk)

#### Python bindings
- `SExp` (tree) and `SExpNode` (handle) types exposed via a C extension module
- `parse(src)` entry point; `SExp()` empty-tree constructor
- `ParseError` exception with line/column information
- Zero-copy node handles: each `SExpNode` is an 8-byte (owner + index) struct
- Full Python API: `__len__`, `__iter__`, `__getitem__` (int and str), `__eq__`, `__repr__`
- `head`, `tail`, `parent`, `value`, `is_atom` properties
- `append`, `prepend`, `insert_after`, `remove`, `extract`, `clone` mutation methods

#### Packaging & build
- `scikit-build-core` build backend with CMake; editable installs supported
- Wheels for Linux (`x86_64`, `aarch64`), macOS (`x86_64`, `arm64`), Windows (`AMD64`)
- Portable build: MSVC 2019+ and clang-cl supported alongside gcc/clang

#### CI
- Build, test, and code-quality workflows (GitHub Actions)
- C tests via Unity framework; Python tests via pytest
- Benchmark suite (`pytest-benchmark`) covering throughput, memory, and mutation

#### Documentation & examples
- `docs/api.md` — full Python API reference
- `docs/how-it-works.md` — architecture overview
- `docs/internals/` — deep-dives on the arena, intern pool, node array, parser, and mutation
- Five annotated examples: query, transform, build, split-document, mini Lisp evaluator

[1.1.1]: https://github.com/AndreKoraleski/sexp/releases/tag/v1.1.1
[1.1.0]: https://github.com/AndreKoraleski/sexp/releases/tag/v1.1.0
[1.0.0]: https://github.com/AndreKoraleski/sexp/releases/tag/v1.0.0
