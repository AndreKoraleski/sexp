from __future__ import annotations

from typing import Any

import pytest
from pytest_benchmark.fixture import BenchmarkFixture

import sexp
from benchmarks.inputs import LARGE, MEDIUM, SMALL

_ROUNDS = 200


def _tree_and_atom() -> tuple[tuple[sexp.SExp, sexp.SExp], dict[str, Any]]:
    tree = sexp.parse(MEDIUM)
    return (tree, tree.new_atom("x")), {}


def test_append(benchmark: BenchmarkFixture) -> None:
    benchmark.pedantic(
        lambda tree, node: tree.append(node),
        setup=_tree_and_atom,
        iterations=1,
        rounds=_ROUNDS,
    )


def test_prepend(benchmark: BenchmarkFixture) -> None:
    benchmark.pedantic(
        lambda tree, node: tree.prepend(node),
        setup=_tree_and_atom,
        iterations=1,
        rounds=_ROUNDS,
    )


def test_insert_after(benchmark: BenchmarkFixture) -> None:
    def _setup() -> tuple[tuple[sexp.SExp, sexp.SExp, sexp.SExp], dict[str, Any]]:
        tree = sexp.parse(MEDIUM)
        return (tree, tree[0], tree.new_atom("x")), {}

    benchmark.pedantic(
        lambda tree, ref, node: tree.insert_after(ref, node),
        setup=_setup,
        iterations=1,
        rounds=_ROUNDS,
    )


_SIZE_PARAMS = [
    pytest.param(SMALL, id="small"),
    pytest.param(MEDIUM, id="medium"),
    pytest.param(LARGE, id="large"),
]

_SUBTREE_PARAMS = [
    pytest.param(MEDIUM, id="medium"),
    pytest.param(LARGE, id="large"),
]


@pytest.mark.parametrize("data", _SIZE_PARAMS)
def test_remove_leaf(benchmark: BenchmarkFixture, data: bytes) -> None:
    def _setup() -> tuple[tuple[sexp.SExp], dict[str, Any]]:
        tree = sexp.parse(data)
        node = tree[len(tree) - 1]
        return (node,), {}

    benchmark.pedantic(
        lambda node: node.remove(),
        setup=_setup,
        iterations=1,
        rounds=_ROUNDS,
    )


@pytest.mark.parametrize("data", _SUBTREE_PARAMS)
def test_remove_subtree(benchmark: BenchmarkFixture, data: bytes) -> None:
    def _setup() -> tuple[tuple[sexp.SExp], dict[str, Any]] | None:
        tree = sexp.parse(data)
        for node in tree:
            if not node.is_atom:
                return (node,), {}
        pytest.skip("no list child in input")
        return None

    benchmark.pedantic(
        lambda node: node.remove(),
        setup=_setup,
        iterations=1,
        rounds=_ROUNDS,
    )


_CLONE_PARAMS = [
    pytest.param(SMALL, 500, id="small"),
    pytest.param(MEDIUM, 200, id="medium"),
    pytest.param(LARGE, 50, id="large"),
]


@pytest.mark.parametrize("data,iterations", _CLONE_PARAMS)
def test_clone(benchmark: BenchmarkFixture, data: bytes, iterations: int) -> None:
    tree = sexp.parse(data)
    root = tree.head.parent
    assert root is not None
    benchmark.pedantic(root.clone, iterations=iterations, rounds=100)


@pytest.mark.parametrize("data", _SUBTREE_PARAMS)
def test_extract(benchmark: BenchmarkFixture, data: bytes) -> None:
    def _setup() -> tuple[tuple[sexp.SExp], dict[str, Any]] | None:
        tree = sexp.parse(data)
        for node in tree:
            if not node.is_atom:
                return (node,), {}
        pytest.skip("no list child in input")
        return None

    benchmark.pedantic(
        lambda node: node.extract(),
        setup=_setup,
        iterations=1,
        rounds=_ROUNDS,
    )
