"""Build configuration for the sexp C extension."""

from setuptools import Extension, setup

sexp_ext = Extension(
    "_sexp",
    sources=[
        "python/_sexp.c",
        "src/arena.c",
        "src/intern.c",
        "src/tree.c",
    ],
    include_dirs=["src/include"],
    extra_compile_args=["-std=c11", "-Wall", "-Wextra", "-O2"],
)

setup(
    ext_modules=[sexp_ext],
    packages=["sexp"],
    package_dir={"sexp": "python/sexp"},
)
