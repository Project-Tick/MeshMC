#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-License-Identifier: GPL-3.0-or-later
#
# build-deps.sh — Build and install all MeshMC monorepo dependencies, then
#                  configure MeshMC itself.  (Linux / macOS / MSYS2-MinGW)
#
# For Windows MSVC, use build-deps.ps1 instead.
#
# Usage:
#   ./scripts/build-deps.sh [--configure] [--build] [--jobs N]
#
# Flags:
#   --configure   Also configure MeshMC after installing dependencies
#   --build       Also build MeshMC (implies --configure)
#   --jobs N      Parallel build jobs (default: nproc)
#   --clean       Remove existing build directories before building
#
# Environment variables:
#   MONOREPO_ROOT     Override monorepo root (default: auto-detected)
#   INSTALL_PREFIX    Override install prefix (default: platform-dependent)
#   BUILD_TYPE        Override build type (default: Release)
#   MINGW_PREFIX      MinGW prefix (required for MinGW builds)
#   CMAKE_GENERATOR   Override generator (default: Ninja)

set -euo pipefail

##############################################################################
# Helpers
##############################################################################

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log()   { echo -e "${GREEN}[build-deps]${NC} $*"; }
warn()  { echo -e "${YELLOW}[build-deps]${NC} $*" >&2; }
error() { echo -e "${RED}[build-deps]${NC} $*" >&2; exit 1; }

##############################################################################
# Defaults
##############################################################################

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MONOREPO_ROOT="${MONOREPO_ROOT:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
MESHMC_DIR="$MONOREPO_ROOT/meshmc"
BUILD_TYPE="${BUILD_TYPE:-Release}"
GENERATOR="${CMAKE_GENERATOR:-Ninja}"
JOBS="${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

DO_CONFIGURE=false
DO_BUILD=false
DO_CLEAN=false

##############################################################################
# Parse arguments
##############################################################################

while [[ $# -gt 0 ]]; do
    case "$1" in
        --configure) DO_CONFIGURE=true ;;
        --build)     DO_BUILD=true; DO_CONFIGURE=true ;;
        --clean)     DO_CLEAN=true ;;
        --jobs)      shift; JOBS="$1" ;;
        -h|--help)
            sed -n '2,/^$/s/^# \?//p' "$0"
            exit 0
            ;;
        *) error "Unknown option: $1" ;;
    esac
    shift
done

##############################################################################
# Platform detection
##############################################################################

detect_platform() {
    local uname_s
    uname_s="$(uname -s)"

    case "$uname_s" in
        Linux)
            PLATFORM="linux"
            ;;
        Darwin)
            PLATFORM="macos"
            ;;
        MINGW*|MSYS*|CLANG*)
            if [[ -n "${MINGW_PREFIX:-}" ]]; then
                PLATFORM="windows-mingw"
            else
                error "MINGW_PREFIX is not set. Are you running inside an MSYS2 shell?"
            fi
            ;;
        *)
            if [[ "${OS:-}" == "Windows_NT" ]]; then
                error "For Windows MSVC, use build-deps.ps1 instead."
            else
                error "Unsupported platform: $uname_s"
            fi
            ;;
    esac

    log "Detected platform: ${BLUE}$PLATFORM${NC}"
}

##############################################################################
# Platform-specific CMake flags
##############################################################################

setup_cmake_flags() {
    case "$PLATFORM" in
        linux)
            INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
            CMAKE_COMMON=(
                "-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
                "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
            )
            NEED_SUDO=true
            ;;
        macos)
            INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
            CMAKE_COMMON=(
                "-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
                "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
                "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"
            )
            NEED_SUDO=true
            ;;
        windows-mingw)
            INSTALL_PREFIX="${INSTALL_PREFIX:-$MINGW_PREFIX}"
            CMAKE_COMMON=(
                "-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
                "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
            )
            NEED_SUDO=false
            ;;
    esac

    log "Install prefix: ${BLUE}$INSTALL_PREFIX${NC}"
    log "Build type:     ${BLUE}$BUILD_TYPE${NC}"
    log "Generator:      ${BLUE}$GENERATOR${NC}"
    log "Jobs:           ${BLUE}$JOBS${NC}"
}

##############################################################################
# Build a single library
##############################################################################

BUILT_COUNT=0
TOTAL_LIBS=15

install_lib() {
    local dir="$1"; shift
    local src="$MONOREPO_ROOT/$dir"
    local bld="$src/build"

    BUILT_COUNT=$((BUILT_COUNT + 1))

    if [[ ! -d "$src" ]]; then
        error "Directory not found: $src"
    fi

    log "[$BUILT_COUNT/$TOTAL_LIBS] Building ${BLUE}$dir${NC} ..."

    if [[ "$DO_CLEAN" == true ]] && [[ -d "$bld" ]]; then
        rm -rf "$bld"
    fi

    cmake -S "$src" -B "$bld" \
        "${CMAKE_COMMON[@]}" "$@" \
        -G "$GENERATOR"

    cmake --build "$bld" --parallel "$JOBS"

    if [[ "$NEED_SUDO" == true ]]; then
        sudo cmake --install "$bld"
    else
        cmake --install "$bld"
    fi
}

##############################################################################
# Build all dependencies (in correct order)
##############################################################################

build_deps() {
    log "Building all MeshMC dependencies..."
    echo

    # Everything that used to be built and installed here is now part of the
    # MeshMC tree and configured by the main build:
    #
    #   libraries/zlib-ng, libraries/libarchive    git submodules
    #   libraries/* (nbt++, systeminfo, ...)       git subtrees
    #   cmark, toml++                              FetchContent (top-level CMake)
    #
    # zlib-ng replaces neozip specifically so libarchive can link against a
    # standard zlib API and keep its DEFLATE support. Building libarchive into
    # a separate prefix like this used to do is exactly how it ended up
    # compiled without zlib -- unable to read a single .jar -- so please do not
    # reintroduce it here.
    log "Remaining dependencies are in-tree; nothing else to install."
    log "Ensure the submodules are checked out:"
    log "    git submodule update --init --recursive"

    # Refresh linker cache on Linux
    if [[ "$PLATFORM" == "linux" ]]; then
        sudo ldconfig
    fi

    echo
    log "${GREEN}All dependencies built and installed successfully!${NC}"
}

##############################################################################
# Configure MeshMC
##############################################################################

configure_meshmc() {
    log "Configuring MeshMC..."

    local preset
    case "$PLATFORM" in
        linux)           preset="linux" ;;
        macos)           preset="macos_universal" ;;
        windows-mingw)   preset="windows_mingw" ;;
    esac

    cmake --preset "$preset" -S "$MESHMC_DIR"

    log "MeshMC configured with preset: ${BLUE}$preset${NC}"
}

##############################################################################
# Build MeshMC
##############################################################################

build_meshmc() {
    log "Building MeshMC..."

    local preset
    case "$PLATFORM" in
        linux)           preset="linux" ;;
        macos)           preset="macos_universal" ;;
        windows-mingw)   preset="windows_mingw" ;;
    esac

    cmake --build --preset "$preset" --config "$BUILD_TYPE" --parallel "$JOBS"

    log "${GREEN}MeshMC built successfully!${NC}"
}

##############################################################################
# Main
##############################################################################

main() {
    detect_platform
    setup_cmake_flags

    echo
    build_deps

    if [[ "$DO_CONFIGURE" == true ]]; then
        echo
        configure_meshmc
    fi

    if [[ "$DO_BUILD" == true ]]; then
        echo
        build_meshmc
    fi

    echo
    log "${GREEN}Done!${NC}"
}

main
