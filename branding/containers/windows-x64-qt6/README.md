# Windows x86_64 with Qt 6.0.0

This image uses the existing MeshMC Wine toolchain as its base. It adds the
Qt 6.0.0 `mingw81_64` SDK with Qt SVG and Core5Compat, MinGW 8.1 x64,
x64 zlib headers and libraries, and the x64 OpenSSL 1.1 DLLs.

From this directory, provide the four installed inputs as BuildKit contexts.
The zlib context should use the standard `include`, `lib` and `bin` layout.
The OpenSSL context should contain `libssl-1_1-x64.dll` and
`libcrypto-1_1-x64.dll` at its root.

```sh
docker buildx build --platform linux/amd64 --load \
  --build-context qt=/path/to/6.0.0/mingw81_64 \
  --build-context mingw=/path/to/mingw810_64 \
  --build-context zlib=/path/to/zlib-x64 \
  --build-context openssl=/path/to/openssl-x64/bin \
  --tag ghcr.io/project-tick/meshmc/windows-x64-qt6:dev .
```

MeshMC CI uses the `win64-qt6` platform. The Windows 10 x64 package bundles
Qt and is selectable separately in the updater.
