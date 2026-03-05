.PHONY: help dev build test lint format format-check typecheck pre-commit pre-commit-install clean

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
