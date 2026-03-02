"""Stub for the _sexp C extension module."""

from __future__ import annotations

from collections.abc import Iterator
from typing import NoReturn, overload

class SExp:
    """Parsed S-expression tree (owns the backing memory).

    The tree is returned by ``parse()`` and represents the top-level form.
    All child nodes are accessed via :class:`SExpNode` views that hold a
    non-owning reference back to this object.
    """

    def __new__(cls) -> SExp:
        """Create an empty S-expression tree, equivalent to ``parse("()")``."""
        ...
    def __repr__(self) -> str: ...
    def __len__(self) -> int: ...
    def __eq__(self, other: object) -> bool: ...
    def __ne__(self, other: object) -> bool: ...
    __hash__: None  # type: ignore[assignment]
    def __contains__(self, item: object) -> bool: ...
    @overload
    def __getitem__(self, key: int) -> SExpNode: ...
    @overload
    def __getitem__(self, key: str) -> SExpNode: ...
    @overload
    def __getitem__(self, key: slice) -> list[SExpNode]: ...
    def __iter__(self) -> Iterator[SExpNode]: ...
    @property
    def head(self) -> SExpNode:
        """First child node.

        Raises:
            IndexError: If the expression is empty.

        """
        ...

    @property
    def tail(self) -> Iterator[SExpNode]:
        """Iterator over all children after the first (i.e. ``children[1:]``)."""
        ...

    def new_atom(self, value: str) -> SExpNode:
        """Allocate a new unattached atom node with the given string value."""
        ...

    def new_list(self) -> SExpNode:
        """Allocate a new unattached empty list node."""
        ...

    def append(self, child: SExpNode) -> None:
        """Append child as the last top-level node."""
        ...

    def prepend(self, child: SExpNode) -> None:
        """Insert child as the first top-level node."""
        ...

    def insert_after(self, after: SExpNode | None, child: SExpNode) -> None:
        """Insert child after the given top-level sibling. Pass None to prepend."""
        ...

    def clone(self) -> SExp:
        """Deep-copy this tree into a new independent SExp."""
        ...

    @property
    def is_atom(self) -> bool:
        """Always False: the tree root is a list."""
        ...

    @property
    def value(self) -> NoReturn:
        """Always raises TypeError: the root is not an atom."""
        ...

    @property
    def parent(self) -> None:
        """Always None: the root has no parent."""
        ...

class SExpNode:
    """Non-owning view of a single node within an :class:`SExp` tree.

    Instances are lightweight: they store a pointer to the owning :class:`SExp`
    and a 32-bit index.  Mutations through a node view are immediately visible
    through any other view of the same tree.
    """

    def __repr__(self) -> str: ...
    def __len__(self) -> int: ...
    def __eq__(self, other: object) -> bool: ...
    def __ne__(self, other: object) -> bool: ...
    __hash__: None  # type: ignore[assignment]
    def __contains__(self, item: object) -> bool: ...
    @overload
    def __getitem__(self, key: int) -> SExpNode: ...
    @overload
    def __getitem__(self, key: str) -> SExpNode: ...
    @overload
    def __getitem__(self, key: slice) -> list[SExpNode]: ...
    def __iter__(self) -> Iterator[SExpNode]: ...
    @property
    def head(self) -> SExpNode:
        """First child node.

        Raises:
            IndexError: If the node has no children.

        """
        ...

    @property
    def tail(self) -> Iterator[SExpNode]:
        """Iterator over all children after the first (i.e. ``children[1:]``)."""
        ...

    @property
    def value(self) -> str:
        """String content of an atom node.

        Raises:
            TypeError: If the node is a list node.

        """
        ...

    @value.setter
    def value(self, value: str) -> None:
        """Set the string content of an atom node.

        Raises:
            TypeError: If the node is a list node, or ``value`` is not a ``str``.

        """
        ...

    @property
    def parent(self) -> SExpNode | None:
        """Parent node, or ``None`` if this node is the root of its tree."""
        ...

    @property
    def is_atom(self) -> bool:
        """``True`` if this is an atom (leaf) node, ``False`` if it is a list."""
        ...

    def remove(self) -> None:
        """Remove this node and its entire subtree from the owning tree.

        All previously obtained indices into the tree are invalidated.
        """
        ...

    def clone(self) -> SExp:
        """Deep-copy this subtree into a new independent :class:`SExp`.

        The clone owns its own node array and shares the intern pool (reference
        counted).  Modifications to the clone do not affect the original tree.
        """
        ...

    def extract(self) -> SExp:
        """Remove this subtree from the tree and return it as a new :class:`SExp`.

        Equivalent to ``clone()`` followed by ``remove()``.  If cloning fails
        the source tree is left unchanged.
        """
        ...

    def append(self, child: SExpNode) -> None:
        """Append *child* as the last child of this list node."""
        ...

    def prepend(self, child: SExpNode) -> None:
        """Insert *child* as the first child of this list node."""
        ...

    def insert_after(self, after: SExpNode | None, child: SExpNode) -> None:
        """Insert *child* immediately after *after* in this node's child list."""
        ...

    def new_atom(self, value: str) -> SExpNode:
        """Allocate a new unattached atom node in the owning tree."""
        ...

    def new_list(self) -> SExpNode:
        """Allocate a new unattached list node in the owning tree."""
        ...

def parse(source: str | bytes | bytearray) -> SExp:
    """Parse an S-expression from *source* and return the tree.

    Raises:
        ParseError: If the input is malformed
                    (unclosed parenthesis, stray closing parenthesis, etc.).

    """
    ...

class ParseError(ValueError):
    """Raised when an S-expression string cannot be parsed.

    Subclass of ValueError for backwards compatibility.
    """

    ...
