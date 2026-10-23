# Vendored stb_vorbis

This directory contains a vendored copy of `stb_vorbis.c` v1.22 from
[stb](https://github.com/nothings/stb).

- Commit: `003501f6d7a8668ce8b6d875e34dd97ad0d5d5b5`
- Upstream file: `stb_vorbis.c`
- License: public domain / MIT (see the license block at the end of the file)

`stb_vorbis.h` is a small wrapper that exposes the declarations without
compiling the implementation in every translation unit.

To update, replace `stb_vorbis.c` with the desired upstream revision and
update the commit hash in this file.
