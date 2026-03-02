"""Build configuration for the sexp C extension."""

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext as _build_ext

_GCC_CLANG_FLAGS = ["-std=c11", "-Wall", "-Wextra", "-O2"]
_MSVC_FLAGS = ["/std:c11", "/W3", "/O2"]


class build_ext(_build_ext):
    """Inject the correct compiler flags before building."""

    def build_extensions(self) -> None:
        """Select GCC/Clang or MSVC flags and attach them to every extension."""
        is_msvc = self.compiler.compiler_type == "msvc"
        flags = _MSVC_FLAGS if is_msvc else _GCC_CLANG_FLAGS
        for ext in self.extensions:
            ext.extra_compile_args = flags
        super().build_extensions()


sexp_ext = Extension(
    "_sexp",
    sources=[
        "python/_sexp.c",
        "python/_sexp_iter.c",
        "python/_sexp_node.c",
        "python/_sexp_tree.c",
        "src/memory/arena.c",
        "src/memory/intern.c",
        "src/core/node.c",
        "src/core/tree.c",
        "src/parse/tokenizer.c",
        "src/parse/parse_stack.c",
        "src/parse/parser.c",
        "src/serialize/serializer.c",
        "src/internal/mutate.c",
        "src/internal/clone.c",
    ],
    include_dirs=["src"],
)

setup(
    ext_modules=[sexp_ext],
    packages=["sexp"],
    package_dir={"sexp": "python/sexp"},
    package_data={"sexp": ["py.typed", "*.pyi"]},
    cmdclass={"build_ext": build_ext},
)
