# vcpkg-tool-meson (overlay)

A copy of the upstream vcpkg port of the same name, with **one** addition:
`universal-osx.patch`. Everything else in this directory is upstream's, taken
from `cmake/vcpkg/ports/vcpkg-tool-meson` at meson 1.9.0, port version 10.

Carrying the whole port is not a choice — vcpkg overlays replace a port wholesale,
so a single extra patch means vendoring the directory.

## Why the patch exists

`tomlplusplus` builds with meson, which is the only reason meson is in this
dependency set at all. On the `universal-osx` triplet vcpkg passes
`-arch arm64 -arch x86_64` in `CC`/`CXX`, and meson's compiler detection runs
the compiler with `-E -dM -` to read out its preprocessor defines. clang refuses
to preprocess for more than one architecture at once, so the probe exits
non-zero, `_get_clang_compiler_defines()` raises, and `meson setup` fails before
it builds anything:

```
CMake Error at scripts/cmake/vcpkg_execute_required_process.cmake:127 (message):
    Command failed: .../meson.py setup ... (Error code: 1)
```

The patch drops `-arch <name>` pairs from the command line used for that one
probe. Nothing else is affected: those defines identify the compiler, they do
not describe the target, and the real compilation still receives both flags —
the binaries stay fat.

Upstream meson bugs: [#5290](https://github.com/mesonbuild/meson/issues/5290),
[#8206](https://github.com/mesonbuild/meson/issues/8206).

## Keeping it in sync

When the vcpkg baseline in `vcpkg-configuration.json` moves and this port
changes upstream, re-copy the directory and re-apply the one addition:

```bash
cp cmake/vcpkg/ports/vcpkg-tool-meson/* cmake/vcpkg-ports/vcpkg-tool-meson/
# then restore universal-osx.patch, this README, and the two edits in
# portfile.cmake (the header comment and the patches list entry)
```

Check whether the patch still applies against the meson version the port
downloads:

```bash
curl -sLO https://github.com/mesonbuild/meson/archive/<version>.tar.gz
tar xzf <version>.tar.gz && cd meson-<version> && git init -q .
git apply --check ../cmake/vcpkg-ports/vcpkg-tool-meson/universal-osx.patch
```

Drop this overlay entirely once meson fixes the upstream bug.
