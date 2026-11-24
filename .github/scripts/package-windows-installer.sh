#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update
sudo apt-get install -y nsis osslsigncode
mkdir -p "$GITHUB_WORKSPACE/dist"
version="$(git rev-parse --short HEAD)"
if [[ "$GITHUB_REF_TYPE" == tag ]]; then
    version="$GITHUB_REF_NAME"
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
sign() {
    local binary=$1
    if [[ -n "$WINDOWS_CODESIGN_CERT" ]]; then
        osslsigncode sign -pkcs12 "$work/certificate.pfx" -readpass "$work/password" \
            -h sha256 -ts http://timestamp.digicert.com \
            -in "$binary" -out "$work/signed.exe"
        mv "$work/signed.exe" "$binary"
    fi
}

cd "$BUILD_OUTPUT/install/MeshMC"
if [[ -n "$WINDOWS_CODESIGN_CERT" ]]; then
    umask 077
    printf '%s' "$WINDOWS_CODESIGN_CERT" | base64 --decode > "$work/certificate.pfx"
    printf '%s\n' "$WINDOWS_CODESIGN_PASSWORD" > "$work/password"
    for executable in MeshMC.exe meshmc-updater.exe meshmc-crashreporter.exe; do
        if [[ -f "$executable" ]]; then
            sign "$executable"
        fi
    done
fi

makensis -NOCD -DMESHMC_HOST_NSIS "-DMESHMC_ICON=$GITHUB_WORKSPACE/branding/MeshMC.ico" \
    "$BUILD_OUTPUT/build/win_install.nsi"
installer="$BUILD_OUTPUT/install/MeshMC-Setup.exe"
sign "$installer"
mv "$installer" "$GITHUB_WORKSPACE/dist/MeshMC-$version-$PLATFORM-Setup.exe"
