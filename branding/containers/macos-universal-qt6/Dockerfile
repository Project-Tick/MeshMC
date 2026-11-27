# syntax=docker/dockerfile:1
ARG BASE_IMAGE=ghcr.io/project-tick/meshmc/macos-osx64:dev
FROM ${BASE_IMAGE}

# Use an osxcross SDK and wrappers already present in the Qt 5 toolchain image.
# Qt tools run on the Linux host while the universal frameworks target macOS.
RUN apt-get update \
    && glib_package=libglib2.0-0 \
    && if apt-cache show libglib2.0-0t64 2>/dev/null | grep -q '^Package: libglib2.0-0t64$'; then \
        glib_package=libglib2.0-0t64; \
    fi \
    && apt-get install -y --no-install-recommends "$glib_package" libatomic1 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=qthost . /opt/qt6-host
COPY --from=qt . /opt/qt6-macos
RUN qt_version=$(/opt/qt6-host/bin/qmake -query QT_VERSION) \
    && test "$qt_version" = 6.2.0 \
    && /opt/qt6-host/libexec/moc -v \
    && /opt/qt6-host/libexec/rcc -v \
    && /opt/qt6-host/libexec/uic -v \
    && test -f /opt/qt6-macos/lib/cmake/Qt6Core5Compat/Qt6Core5CompatConfig.cmake \
    && file -L /opt/qt6-macos/lib/QtCore.framework/QtCore | grep 'universal binary'
LABEL org.opencontainers.image.title="MeshMC macOS universal Qt 6.2.0 toolchain"
