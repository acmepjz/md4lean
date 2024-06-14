# md4lean

a Lean wrapper for the [MD4C](https://github.com/mity/md4c) Markdown parser,
to be used in [doc-gen4](https://github.com/leanprover/doc-gen4),
replacing [CMark.lean](https://github.com/xubaiw/CMark.lean).

The `md4c` implementation (`release-0.5.2` version, <https://github.com/mity/md4c/releases/tag/release-0.5.2>)
has been copied into this repository, so there is no need for other installation process.

Compared to the upstream, a new feature `MD_HTML_FLAG_MATHJAX[_USE_DOLLAR]` is added.

## Compile under Windows using MSVC

There is an extremely hacky solution of compiling under Windows using MSVC.
You need to run `lake build` under Visual Studio Developer Command Prompt (or `git-bash` opened under it).
