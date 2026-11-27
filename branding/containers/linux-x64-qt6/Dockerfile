# syntax=docker/dockerfile:1
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive LANG=C.UTF-8 JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential binutils cmake git file openjdk-8-jdk-headless python3 zlib1g-dev \
    libgl1-mesa-dev libegl1-mesa-dev libfontconfig1 libdbus-1-3 libxkbcommon0 \
    libx11-6 libx11-xcb1 libxcb1 libxcb-render-util0 libxcb-image0 libxcb-keysyms1 \
    libxcb-icccm4 libxcb-xinerama0 libxcb-xkb1 libxkbcommon-x11-0 \
    libxcb-randr0 libxcb-shape0 libxcb-shm0 libxcb-xfixes0 libxcb-sync1 \
    libxcb-render0 libxcb-xinput0 libxcb-util1 libsm6 libice6 libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

# Named contexts: the Qt 6.0.0 gcc_64 SDK.
COPY --from=qt . /opt/qt6
RUN test "$(/opt/qt6/bin/qmake -query QT_VERSION)" = 6.0.0 \
    && test -f /opt/qt6/lib/cmake/Qt6Core5Compat/Qt6Core5CompatConfig.cmake
LABEL org.opencontainers.image.title="MeshMC Linux x86_64 Qt 6.0.0 toolchain"
