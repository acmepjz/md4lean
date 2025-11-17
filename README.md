# md4lean

a Lean wrapper for the [MD4C](https://github.com/mity/md4c) Markdown parser,
to be used in [doc-gen4](https://github.com/leanprover/doc-gen4),
replacing [CMark.lean](https://github.com/xubaiw/CMark.lean).

The `md4c` implementation (a custom fork, <https://github.com/acmepjz/md4c/tree/for_md4lean>)
has been copied into this repository, so there is no need for other installation process.

Compared to the upstream, a new feature `MD_HTML_FLAG_MATHJAX[_USE_DOLLAR]` is added.

## Compile under Windows

There is an extremely hacky solution for compiling under Windows, which is working but not widely tested.
It will call Lean's built-in clang compiler, which is shipped with full library files, but without headers.
To overcome missing header problem, the minimal ad-hoc C headers are provided.

## Versions

Users of Lean versions 4.21 and 4.22 should use commit 44da417da3705ede62b5c39382ddd2261ec3933e of this library. Users of later versions can track `main`.
