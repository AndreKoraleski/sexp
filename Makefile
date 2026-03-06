.PHONY: help dev build test lint format format-check typecheck bench bench-save bench-ci pre-commit pre-commit-install clean

# --- Help ---
help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  help               Show this help message"
	@echo "  dev                Install the extension for development"
	@echo "  build              Build a release wheel"
	@echo "  test               Run Python tests"
	@echo "  test-rs            Run Rust unit tests"
	@echo "  test-all           Run all tests"
	@echo "  bench              Run benchmarks"
	@echo "  bench-save         Run benchmarks and save as baseline"
	@echo "  bench-ci           Compare against baseline (fail on >15% median regression)"
	@echo "  lint               Lint Python (ruff) and Rust (clippy)"
	@echo "  format             Auto-format Python and Rust"
	@echo "  format-check       Check formatting without modifying files"
	@echo "  typecheck          Run mypy type-checker over the Python package"
	@echo "  pre-commit         Run all pre-commit hooks over every file"
	@echo "  pre-commit-install Install pre-commit hooks into .git/hooks"
	@echo "  clean              Remove build artefacts"

# --- Development and Build ---

## Install the extension
dev:
	maturin develop

## Build a release wheel
build:
	maturin build --release

# --- Testing ---

## Run Python tests
test:
	pytest tests/python -x -q

## Run Rust unit tests
test-rs:
	cargo test

## Run all tests
test-all: test test-rs

_BENCH_FLAGS := --benchmark-only --benchmark-disable-gc --benchmark-storage=benchmarks/.results

## Run benchmarks
bench:
	pytest benchmarks/ -q $(_BENCH_FLAGS)

## Save current results as a named baseline
bench-save:
	pytest benchmarks/ -q $(_BENCH_FLAGS) --benchmark-autosave

## Compare against latest saved baseline (fail on >15% mean regression)
bench-ci:
	@if [ -z "$$(ls benchmarks/.results/ 2>/dev/null)" ]; then \
		echo "No baseline found. Run 'make bench-save' first."; exit 1; \
	fi
	pytest benchmarks/ -q $(_BENCH_FLAGS) --benchmark-compare --benchmark-compare-fail=median:15%

# --- Lint ---

## Lint Python (ruff) and Rust (clippy)
lint:
	ruff check .
	cargo clippy -- -D warnings

# --- Format ---

## Auto-format Python and Rust
format:
	ruff format .
	cargo fmt

## Check formatting without modifying files
format-check:
	ruff format --check .
	cargo fmt -- --check

# --- Types ---


## Run mypy type-checker over the Python package
typecheck:
	mypy python/sexp

# --- Pre-commit ---

## Run all pre-commit hooks over every file
pre-commit:
	pre-commit run --all-files

## Install pre-commit hooks into .git/hooks
pre-commit-install:
	pre-commit install

# --- Clean ---

## Remove build artefacts
clean:
	cargo clean
	find . -type d -name __pycache__ -exec rm -rf {} +
	find . -type d -name .pytest_cache -exec rm -rf {} +
	find . -name "*.so" -delete
	rm -rf dist/ htmlcov/ .coverage
