#!/usr/bin/env bash
set -euo pipefail

mkdir -p /out/build /out/install
cd /out/build
export SOURCE_DATE_EPOCH=315532800

source_dir=/src
install_prefix=/out/install/MeshMC
updater_floor=
if [[ -f /out/updater-keys-floor.json ]]; then
    updater_floor=/out/updater-keys-floor.json
fi

configure=(cmake)
build=(make -j"$(nproc)")
test_command=(ctest --output-on-failure)
install=(make install)

case "$PLATFORM" in
    lin64|lin32)
        git config --global --add safe.directory /src
        export QT_QPA_PLATFORM=offscreen
        platform_args=(
            -DLauncher_LAYOUT=lin-nodeps
            -DCMAKE_C_COMPILER=/usr/bin/gcc-5
            -DCMAKE_CXX_COMPILER=/usr/bin/g++-5
            -DCMAKE_PREFIX_PATH=/opt/qt56
            -DQt5_DIR=/opt/qt56
        )
        ;;
    lin64-qt6)
        git config --global --add safe.directory /src
        export QT_QPA_PLATFORM=offscreen
        platform_args=(
            -DLauncher_LAYOUT=lin-nodeps
            -DLauncher_QT_MAJOR=6
            -DCMAKE_PREFIX_PATH=/opt/qt6
        )
        ;;
    win32|win32-5.15.2|win64-qt6)
        export WINEPREFIX=/out/prefix WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml="
        cp -R "$PREFIX_TEMPLATE" "$WINEPREFIX"
        rm -f "$WINEPREFIX/dosdevices/z:" "$WINEPREFIX"/dosdevices/*::
        ln -sfn "$TOOLS" "$WINEPREFIX/dosdevices/t:"
        ln -sfn /src "$WINEPREFIX/dosdevices/s:"
        ln -sfn /out "$WINEPREFIX/dosdevices/o:"
        export JAVA_HOME="${JDK:-T:\jdk1.8.0_112}"
        qt_major=5
        zlib='T:/GnuWin32'
        zlib_bin='T:\GnuWin32\bin'
        if [[ "$PLATFORM" == win64-qt6 ]]; then
            qt_major=6
            zlib='T:/Qt6/zlib-x64'
            zlib_bin='T:\Qt6\zlib-x64\bin'
            mingw='T:\Qt6\mingw810_64'
            qt='T:\Qt6\6.0.0\mingw81_64'
            pe_version='-Wl,--major-os-version,10,--minor-os-version,0,--major-subsystem-version,10,--minor-subsystem-version,0,--no-insert-timestamp'
            runtime_dlls=("$TOOLS/Qt6/openssl-x64/"{libcrypto-1_1-x64,libssl-1_1-x64}.dll)
        elif [[ "$PLATFORM" == win32-5.15.2 ]]; then
            mingw='T:\QtX\Tools\mingw810_32'
            qt='T:\QtX\5.15.2\mingw81_32'
            pe_version='-Wl,--major-os-version,6,--minor-os-version,1,--major-subsystem-version,6,--minor-subsystem-version,1,--no-insert-timestamp'
            runtime_dlls=(
                "$TOOLS/QtX/Tools/OpenSSL/Win_x86/bin/libcrypto-1_1.dll"
                "$TOOLS/QtX/Tools/OpenSSL/Win_x86/bin/libssl-1_1.dll"
                "$TOOLS/QtX/vcredist/x86/msvcr100.dll"
            )
        else
            mingw='T:\Qt\Tools\mingw492_32'
            qt='T:\Qt\5.6.3\mingw49_32'
            pe_version='-Wl,--major-os-version,5,--minor-os-version,1,--major-subsystem-version,5,--minor-subsystem-version,1'
            runtime_dlls=("$TOOLS/OpenSSL-Win32/"{libeay32,ssleay32}.dll)
        fi
        export WINEPATH="$mingw"'\bin;'"$qt"'\bin;T:\Tools;'"$zlib_bin"';'"$JAVA_HOME"'\bin'
        source_dir='S:/'
        install_prefix='O:/install/MeshMC'
        if [[ -n "$updater_floor" ]]; then
            updater_floor='O:/updater-keys-floor.json'
        fi
        configure=("$WINE" 'T:\CMake-3.20.1\bin\cmake.exe')
        build=("${configure[@]}" --build . -- -j"$(nproc)")
        test_command=("$WINE" 'T:\CMake-3.20.1\bin\ctest.exe' --output-on-failure)
        install=("${configure[@]}" --install .)
        platform_args=(
            -G 'MinGW Makefiles'
            -DLauncher_LAYOUT=win-bundle
            -DCMAKE_MAKE_PROGRAM=mingw32-make
            -DCMAKE_C_COMPILER=gcc
            -DCMAKE_CXX_COMPILER=g++
            "-DCMAKE_EXE_LINKER_FLAGS=$pe_version"
            "-DCMAKE_SHARED_LINKER_FLAGS=$pe_version"
            "-DCMAKE_PREFIX_PATH=$qt"
            "-DLauncher_QT_MAJOR=$qt_major"
            "-DQt${qt_major}_DIR=$qt"
            "-DZLIB_ROOT=$zlib"
            '-DCMAKE_GP_CMD_PATHS=T:/Tools'
            -DCMAKE_GP_TOOL=dumpbin
        )
        ;;
    osx64-5.15.2|osx-universal-qt6)
        git config --global --add safe.directory /src
        eval "$(/opt/osxcross/bin/osxcross-conf)"
        triple="x86_64-apple-$OSXCROSS_TARGET"
        export OSXCROSS_HOST="$triple" OSXCROSS_TARGET_DIR OSXCROSS_TARGET OSXCROSS_SDK
        export OSXCROSS_C_COMPILER=clang OSXCROSS_CXX_COMPILER=clang++
        mkdir -p /out/tools
        for tool in otool install_name_tool lipo codesign_allocate; do
            ln -sf "/opt/osxcross/bin/$triple-$tool" "/out/tools/$tool"
        done
        export PATH="/out/tools:$PATH"
        qt=/Users/peterix/Qt/5.15.2/clang_64
        install_prefix=/out/install
        platform_args=(
            -DLauncher_LAYOUT=mac-bundle
            -DCMAKE_TOOLCHAIN_FILE=/opt/toolchain-osx64.cmake
            -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13
            "-DCMAKE_PREFIX_PATH=$qt"
            "-DQt5_DIR=$qt/lib/cmake/Qt5"
            -DCMAKE_GP_TOOL=otool
        )
        if [[ "$PLATFORM" == osx-universal-qt6 ]]; then
            platform_args=(
                -DLauncher_LAYOUT=mac-bundle
                -DLauncher_QT_MAJOR=6
                -DCMAKE_TOOLCHAIN_FILE=/opt/toolchain-osx64.cmake
                "-DCMAKE_OSX_SYSROOT=$OSXCROSS_SDK"
                '-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64'
                -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14
                -DQT_HOST_PATH=/opt/qt6-host
                -DCMAKE_PREFIX_PATH=/opt/qt6-macos
                -DLauncher_QT_TARGET_PREFIX=/opt/qt6-macos
                -DCMAKE_GP_TOOL=otool
            )
        fi
        # CMake script mode otherwise uses the Linux host's platform.
        install=(cmake -DAPPLE=1 -DCMAKE_INSTALL_PREFIX=/out/install -P cmake_install.cmake)
        ;;
    *)
        echo "Unsupported toolchain target: $PLATFORM" >&2
        exit 1
        ;;
esac

run_logged() {
    local stage=$1
    shift
    printf '\n==> %s: %s\n' "$PLATFORM" "$stage"
    "$@" 2>&1 | tee "/out/$stage.log"
}

run_logged configure "${configure[@]}" "${platform_args[@]}" \
    "-DCMAKE_BUILD_TYPE=$BUILD_TYPE" \
    "-DCMAKE_INSTALL_PREFIX:PATH=$install_prefix" \
    "-DLauncher_VERSION_BUILD=$BUILD_NUMBER" \
    "-DLauncher_BUILD_PLATFORM=$PLATFORM" \
    "-DLauncher_UPDATER_ROOT_KEYS=$UPDATER_ROOT_KEYS" \
    "-DLauncher_UPDATER_KEYS_FLOOR=$updater_floor" \
    "-DLauncher_UPDATER_VARIANT=$UPDATER_VARIANT" \
    "-DLauncher_UPDATER_SITE=$UPDATER_SITE" \
    "$source_dir"
run_logged build "${build[@]}"
if [[ "$PLATFORM" != osx* ]]; then
    run_logged test "${test_command[@]}"
fi
run_logged install "${install[@]}"

if [[ "$PLATFORM" == win* ]]; then
    run_logged runtime cp "${runtime_dlls[@]}" /out/install/MeshMC/
fi

if [[ "$PLATFORM" == lin64-qt6 ]]; then
    run_logged abi bash -c 'python3 /src/tools/updater_v2/measure_linux_abi.py /out/install/MeshMC --details /out/abi-details.json > /out/abi.json'
fi
