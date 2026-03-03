# sexp

[![Build](https://github.com/AndreKoraleski/sexp/actions/workflows/build.yml/badge.svg)](https://github.com/AndreKoraleski/sexp/actions/workflows/build.yml)
[![Test](https://github.com/AndreKoraleski/sexp/actions/workflows/test.yml/badge.svg)](https://github.com/AndreKoraleski/sexp/actions/workflows/test.yml)
[![PyPI](https://img.shields.io/pypi/v/sexp)](https://pypi.org/project/sexp/)
[![Python](https://img.shields.io/pypi/pyversions/sexp)](https://pypi.org/project/sexp/)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A minimal-allocation S-expression library for Python, backed by a C extension.

Parsing and tree manipulation are handled entirely in C with a bump-pointer
arena allocator and an interned string pool. Python gets lightweight node
views — no copying, no per-node heap allocation.

## Installation

```bash
pip install sexp
```

Requires **Python 3.10+**. Wheels are provided for **Linux** (`x86_64` + `aarch64`),
**macOS** (`x86_64` + `arm64`), and **Windows** (`AMD64`).

## Quick start

```python
from sexp import parse

tree = parse("(define (square x) (* x x))")

tree[0].value         # 'define'
tree[1][0].value      # 'square'
tree.head.value       # 'define'

for node in tree:
    print(node)

src = "(a (b (c d)) e)"
assert repr(parse(src)) == src
```

## Further reading

| Path | Contents |
|------|----------|
| [examples/01_query.py](examples/01_query.py) | Navigate and read a structured document |
| [examples/02_transform.py](examples/02_transform.py) | Walk and mutate a tree |
| [examples/03_build.py](examples/03_build.py) | Construct trees programmatically |
| [examples/04_split_document.py](examples/04_split_document.py) | Split a document with `extract()` |
| [examples/05_evaluator.py](examples/05_evaluator.py) | Mini Lisp evaluator using `sexp` as an AST |
| [docs/api.md](docs/api.md) | Full API reference |
| [docs/how-it-works.md](docs/how-it-works.md) | Architecture overview: node array, arena, intern pool, parser |
| [docs/internals/](docs/internals/README.md) | Internals wiki: node array, arena, intern pool, parser, mutation |

## Development

### Prerequisites

| Audience | Requirements |
|----------|--------------|
| **Installing from PyPI** | Python 3.10+ — wheels are provided for Linux (x86\_64, aarch64), macOS (x86\_64, arm64), and Windows (AMD64). No compiler needed. |
| **Building from source** | Python 3.10+, CMake 3.15+, and a C11 compiler (gcc or clang on Linux/macOS; MSVC 2019+ or clang-cl on Windows). |

### Setup

```bash
python -m venv .venv && source .venv/bin/activate  # Windows: .venv\Scripts\activate
pip install -e '.[dev]'
```

### Running tests

```bash
# Python tests
pytest tests/python

# C tests
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure

# Benchmarks (local only)
pytest benchmarks/ --benchmark-only
```

## License

[MIT](LICENSE)
