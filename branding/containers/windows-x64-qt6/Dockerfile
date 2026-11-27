# syntax=docker/dockerfile:1
ARG BASE_IMAGE=ghcr.io/project-tick/meshmc/windows-win32:dev
FROM ${BASE_IMAGE}

# Use the x86_64 MinGW 8.1 Qt SDK, toolchain, zlib, and OpenSSL.
COPY --from=qt . /opt/win32/Tools/Qt6/6.0.0/mingw81_64
COPY --from=mingw . /opt/win32/Tools/Qt6/mingw810_64
COPY --from=zlib . /opt/win32/Tools/Qt6/zlib-x64
COPY --from=openssl . /opt/win32/Tools/Qt6/openssl-x64
RUN test -f /opt/win32/Tools/Qt6/6.0.0/mingw81_64/lib/cmake/Qt6Core5Compat/Qt6Core5CompatConfig.cmake \
    && test -f /opt/win32/Tools/Qt6/mingw810_64/bin/g++.exe \
    && test -f /opt/win32/Tools/Qt6/openssl-x64/libssl-1_1-x64.dll \
    && test -f /opt/win32/Tools/Qt6/openssl-x64/libcrypto-1_1-x64.dll
LABEL org.opencontainers.image.title="MeshMC Windows x86_64 Qt 6.0.0 toolchain"
