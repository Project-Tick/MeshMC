#!/usr/bin/env bash
set -euo pipefail

packages=$1
destination=$2
prefix=$3
stable_build=${4:-}
mkdir -p "$destination"
shopt -s nullglob

collect() {
    local artifact=$1 suffix=$2
    local files
    if [[ -n "$stable_build" ]]; then
        if [[ -s "$packages/MeshMC-build-$stable_build-$suffix" ]]; then
            files=("$packages/MeshMC-build-$stable_build-$suffix")
        else
            files=("$packages/MeshMC-stable-$suffix")
        fi
    else
        files=("$packages/$artifact"/MeshMC-*-"$suffix")
    fi
    if [[ "${#files[@]}" -ne 1 || ! -s "${files[0]}" ]]; then
        echo "::error::Expected one non-empty $suffix in $artifact."
        exit 1
    fi
    cp -- "${files[0]}" "$destination/$prefix-$suffix"
}

collect package-lin64 lin64.tar.gz
collect package-lin32 lin32.tar.gz
collect package-win32 win32.zip
collect package-win32-setup win32-Setup.exe
collect package-win32-5.15.2 win32-5.15.2.zip
collect package-win32-5.15.2-setup win32-5.15.2-Setup.exe
collect package-osx64-5.15.2 osx64-5.15.2.tar.gz
