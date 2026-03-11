# sexp

[![Release](https://github.com/AndreKoraleski/sexp/actions/workflows/publish.yml/badge.svg)](https://github.com/AndreKoraleski/sexp/actions/workflows/publish.yml)
[![Test](https://github.com/AndreKoraleski/sexp/actions/workflows/test.yml/badge.svg)](https://github.com/AndreKoraleski/sexp/actions/workflows/test.yml)
[![PyPI](https://img.shields.io/pypi/v/sexp)](https://pypi.org/project/sexp/)
[![Python](https://img.shields.io/pypi/pyversions/sexp)](https://pypi.org/project/sexp/)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A fast S-expression library for Python.

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
| [CHANGELOG.md](CHANGELOG.md) | Release history |
| [docs/api.md](docs/api.md) | Full API reference |
| [docs/guide.md](docs/guide.md) | User guide: navigation, mutation, patterns |

## Development

### Prerequisites

- Python 3.10+
- [Rust stable toolchain](https://rustup.rs) (for building from source)

### Setup

```bash
python -m venv .venv && source .venv/bin/activate  # Windows: .venv\Scripts\activate
pip install maturin
make dev
pip install -e '.[dev]'
```

### Common tasks

```
make test        Run Python tests
make test-rs     Run Rust unit tests
make test-all    Run all tests
make lint        Lint (ruff + clippy)
make format      Auto-format (ruff + rustfmt)
make typecheck   mypy
make bench       Run benchmarks
make help        Show all available targets
```

## License

[MIT](LICENSE)
