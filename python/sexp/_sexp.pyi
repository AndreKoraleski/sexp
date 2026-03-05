"""Type stubs for the ``sexp._sexp`` Rust extension module."""

from __future__ import annotations

from collections.abc import Iterator
from typing import overload

class ParseError(ValueError):
    """Raised when an S-expression string cannot be parsed."""

class SExpIter:
    """Iterator over the direct children of an :class:`SExp` node."""

    def __iter__(self) -> SExpIter: ...
    def __next__(self) -> SExp: ...

class SExp:
    """A single node in a shared S-expression tree.

    Every :class:`SExp` is a handle into a heap-allocated tree.  All handles that originate from the
    same :func:`parse` call (or the same empty constructor ``SExp()``) share the same underlying
    tree - mutations through one handle are immediately visible through any other.

    A node that has been removed via :meth:`remove` or :meth:`extract` becomes *stale* - any
    subsequent method call on it raises `RuntimeError`.
    """

    def __new__(cls) -> SExp:
        """Create an empty list node, equivalent to ``parse("()")``.

        Returns:
            SExp: An empty root list node in its own tree.

        """
        ...

    def __repr__(self) -> str: ...
    def __len__(self) -> int: ...
    def __eq__(self, other: object) -> bool: ...
    def __contains__(self, item: object) -> bool: ...

    # Integer index -> single child
    @overload
    def __getitem__(self, key: int) -> SExp: ...

    # String key -> child list whose first element matches
    @overload
    def __getitem__(self, key: str) -> SExp: ...

    # Slice -> list of children
    @overload
    def __getitem__(self, key: slice) -> list[SExp]: ...
    def __iter__(self) -> SExpIter: ...

    # --- Navigation ---

    @property
    def head(self) -> SExp:
        """Return the first child node.

        Raises:
            IndexError: If this node has no children.

        """
        ...

    @property
    def tail(self) -> SExpIter:
        """Return an iterator over every child after the first."""
        ...

    @property
    def parent(self) -> SExp | None:
        """Return the parent node, or ``None`` if this is the root."""
        ...

    # --- Inspection ---

    @property
    def is_atom(self) -> bool:
        """Return ``True`` if this node is an atom (leaf), ``False`` if it is a list."""
        ...

    @property
    def value(self) -> str:
        """Return the string content of this atom node.

        Raises:
            TypeError: If called on a list node.

        """
        ...

    @value.setter
    def value(self, v: str) -> None:
        """Set the string content of this atom node.

        Args:
            v (str): The new string value to assign.

        Raises:
            TypeError: If called on a list node.

        """
        ...

    # --- Node allocation ---

    def new_atom(self, value: str) -> SExp:
        """Allocate a new unattached atom node in the same tree.

        Args:
            value (str): The string content for the new atom.

        Returns:
            SExp: A new atom node belonging to this tree, not yet attached anywhere.

        """
        ...

    def new_list(self) -> SExp:
        """Allocate a new unattached empty list node in the same tree.

        Returns:
            SExp: A new empty list node belonging to this tree, not yet attached anywhere.

        """
        ...

    # --- Mutation ---

    def append(self, child: SExp) -> None:
        """Append *child* as the last child of this node.

        If *child* is already attached elsewhere in the tree it is moved
        (detached first).

        Args:
            child (SExp): The node to append. Must belong to the same tree as *self*.

        Raises:
            ValueError: If *child* belongs to a different tree.

        """
        ...

    def prepend(self, child: SExp) -> None:
        """Insert *child* as the first child of this node.

        If *child* is already attached elsewhere in the tree it is moved (detached first).

        Args:
            child (SExp): The node to prepend. Must belong to the same tree as *self*.

        Raises:
            ValueError: If *child* belongs to a different tree.

        """
        ...

    def insert_after(self, after: SExp | None, child: SExp) -> None:
        """Insert *child* immediately after *after*.

        Args:
            after (SExp | None): The existing child to insert after, or ``None`` to prepend. Must
                belong to the same tree as *self*.
            child (SExp): The node to insert. Must belong to the same tree as *self*.

        Raises:
            ValueError: If *after* or *child* belong to a different tree.

        """
        ...

    def remove(self) -> None:
        """Remove this node and its entire subtree from the tree.

        After this call the node is stale and must not be used.

        Raises:
            ValueError: If called on the root node.

        """
        ...

    # --- Copy / move ---

    def clone(self) -> SExp:
        """Deep-copy this subtree into a new, independent :class:`SExp`.

        Returns:
            SExp: A new :class:`SExp` whose root is a copy of this subtree, backed
                by its own tree.

        """
        ...

    def extract(self) -> SExp:
        """Remove this subtree and return it as a new, independent :class:`SExp`.

        After this call the original node is stale and must not be used.

        Returns:
            SExp: A new :class:`SExp` whose root is this subtree, backed by its own tree.

        Raises:
            ValueError: If called on the root node.

        """
        ...

def parse(source: str | bytes | bytearray) -> SExp:
    """Parse *source* into an :class:`SExp` tree.

    Args:
        source (str | bytes | bytearray): The S-expression to parse.

    Returns:
        SExp: The root node of the parsed tree.

    Raises:
        ParseError: If the input is structurally malformed.
        TypeError: If *source* is not `str`, `bytes`, or `bytearray`.

    """
    ...

# Iterator type alias exposed for use in annotations
SExpIterator = Iterator[SExp]
