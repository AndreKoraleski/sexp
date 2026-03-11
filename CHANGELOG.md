# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.1] - 2026-03-11

### Added

- `__repr__` caching: the string representation of a node is computed once and
  stored internally; subsequent calls return the cached value without
  re-serialising the tree. The cache is invalidated automatically on mutation.
- Cycle guard on `__repr__` prevents infinite recursion if a tree is somehow
  constructed with a reference cycle.

### Fixed

- Mutation operations no longer invalidate sibling handles. Version tracking
  is now scoped correctly so that only truly stale nodes (those detached or
  superseded by a structural change) raise `StaleNodeError`.

### Changed

- CI: dropped automatic benchmark regression gate (the 15 % median threshold
  check on every PR).
- CI: switched to PyPI Trusted Publishing (OIDC) for wheel releases; `sccache`
  removed from Docker-based Linux wheel builds.

---

## [2.0.0] - 2026-03-05

Complete rewrite of the extension module in Rust (PyO3 + maturin), replacing
the C11 implementation. Performance is at parity with or exceeds the C
version across all measured workloads.

### Breaking changes

- **`SExpNode` removed.** The separate `SExpNode` type no longer exists.
  `parse()` now returns an `SExp` that is itself the root node, and every
  handle obtained by indexing, iterating, or navigation is the same `SExp`
  type. Code that imported or annotated `SExpNode` must be updated.
- **`parent` on a top-level node** now returns the root `SExp`, not `None`.
  `None` is only returned by `node.parent` when `node` is the root itself.

### Added

- String key lookup (`node["key"]`) now also matches **bare atom children**
  whose value equals the key, in addition to list children whose head matches.
- Benchmark regression CI (`.github/workflows/benchmark.yml`) — master pushes
  save a baseline; PRs fail if any benchmark regresses by more than 15% median.

### Changed

- Build backend switched from `scikit-build-core` / CMake to `maturin`.
  Building from source now requires a Rust stable toolchain instead of CMake
  and a C compiler.
- `SExpIter` is now an implementation detail, the public iterator type is
  `Iterator[SExp]` in all annotations.

### Removed

- `SExpNode` type.
- C source tree, CMake build files, and Unity test framework.
- `docs/how-it-works.md` and `docs/internals/` — replaced by
  `docs/guide.md`.

---

## [1.1.2] - 2026-03-04

### Fixed

- `bindings/sexp/__init__.pyi` now contains inline class and function
  definitions instead of re-exporting from `_sexp`, so IDEs (Pylance, pyright)
  can resolve method signatures and docstrings for `SExp`, `SExpNode` and
  `parse` without needing to locate the C extension stub separately.

### Changed

- Applied `clang-format` to C sources; extracted `has_multiple_roots` helper.

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

[2.0.0]: https://github.com/AndreKoraleski/sexp/releases/tag/v2.0.0
[1.1.2]: https://github.com/AndreKoraleski/sexp/releases/tag/v1.1.2
[1.1.1]: https://github.com/AndreKoraleski/sexp/releases/tag/v1.1.1
[1.1.0]: https://github.com/AndreKoraleski/sexp/releases/tag/v1.1.0
[1.0.0]: https://github.com/AndreKoraleski/sexp/releases/tag/v1.0.0
