"""S-expression parser backed by a C extension."""

from _sexp import SExp, SExpNode, parse

__all__ = ["SExp", "SExpNode", "parse"]
