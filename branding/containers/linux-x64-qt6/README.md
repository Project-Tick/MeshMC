# Linux x86_64 with Qt 6.0.0

This image is based on Ubuntu 20.04. It builds MeshMC against the Qt 6.0.0
Linux `gcc_64` SDK, including Qt SVG and Core5Compat. The SDK directory must
contain `bin/qmake` and `lib/cmake/Qt6`.

From this directory, supply the installed SDK as the `qt` BuildKit context.

```sh
docker buildx build --platform linux/amd64 --load \
  --build-context qt=/path/to/6.0.0/gcc_64 \
  --tag ghcr.io/project-tick/meshmc/linux-x64-qt6:dev .
```

MeshMC CI uses the `lin64-qt6` platform. GNU readelf is a build dependency
used by MeshMC to measure the packaged binaries. The resulting launcher
uses the distribution's Qt 6 and checks its ELF dependencies with MeshMC's
updater. The package should be smoke-tested on a host with the appropriate
Qt 6 modules and a working Qt platform plugin.
