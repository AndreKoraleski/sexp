"""Type stub for the sexp public package.

Re-exports every public name from the ``_sexp`` C extension so that type
checkers resolve ``import sexp`` and ``from sexp import SExp, SExpNode, parse``
correctly without needing to inspect the C extension directly.
"""

from _sexp import SExp as SExp
from _sexp import SExpNode as SExpNode
from _sexp import parse as parse

__all__ = ["SExp", "SExpNode", "parse"]
