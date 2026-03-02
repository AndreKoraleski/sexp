"""Stub for the _sexp C extension module."""

from __future__ import annotations

from collections.abc import Iterator
from typing import overload

class SExp:
    """Parsed S-expression tree (owns the backing memory).

    The tree is returned by ``parse()`` and represents the top-level form.
    All child nodes are accessed via :class:`SExpNode` views that hold a
    non-owning reference back to this object.
    """

    def __repr__(self) -> str: ...
    def __len__(self) -> int: ...
    @overload
    def __getitem__(self, key: int) -> SExpNode: ...
    @overload
    def __getitem__(self, key: str) -> SExpNode: ...
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
        """Allocate a new unattached atom node with the given string value.

        The node exists in the tree's node array but has no parent, so it does
        not affect ``len(self)`` or ``repr(self)`` until it is attached via
        ``append``, ``prepend``, or ``insert_after``.
        """
        ...

    def new_list(self) -> SExpNode:
        """Allocate a new unattached empty list node.

        The node exists in the tree's node array but has no parent, so it does
        not affect ``len(self)`` or ``repr(self)`` until it is attached.
        """
        ...

class SExpNode:
    """Non-owning view of a single node within an :class:`SExp` tree.

    Instances are lightweight: they store a pointer to the owning :class:`SExp`
    and a 32-bit index.  Mutations through a node view are immediately visible
    through any other view of the same tree.
    """

    def __repr__(self) -> str: ...
    def __len__(self) -> int: ...
    @overload
    def __getitem__(self, key: int) -> SExpNode: ...
    @overload
    def __getitem__(self, key: str) -> SExpNode: ...
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
        """Append *child* as the last child of this list node.

        Raises:
            TypeError:  If *child* is not an :class:`SExpNode`.
            ValueError: If *child* belongs to a different tree.

        """
        ...

    def prepend(self, child: SExpNode) -> None:
        """Insert *child* as the first child of this list node.

        Raises:
            TypeError:  If *child* is not an :class:`SExpNode`.
            ValueError: If *child* belongs to a different tree.

        """
        ...

    def insert_after(self, after: SExpNode | None, child: SExpNode) -> None:
        """Insert *child* immediately after *after* in this node's child list.

        If *after* is ``None`` the node is inserted as the first child
        (equivalent to ``prepend``).  *child* is automatically detached from
        its current parent first.

        Raises:
            TypeError:  If *after* is not an :class:`SExpNode` or ``None``,
                        or if *child* is not an :class:`SExpNode`.
            ValueError: If *child* or *after* belong to a different tree.

        """
        ...

def parse(source: str | bytes | bytearray) -> SExp:
    """Parse an S-expression from *source* and return the tree.

    Raises:
        ValueError: If the input is malformed
                    (unclosed parenthesis, stray closing parenthesis, etc.).

    """
    ...
