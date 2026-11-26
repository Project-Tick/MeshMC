#!/usr/bin/env bash
set -euo pipefail

bundle=$1
test -f "$bundle/Contents/MacOS/MeshMC"

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
version=0.29.0
archive="apple-codesign-$version-x86_64-unknown-linux-musl.tar.gz"
curl --fail --silent --show-error --location --retry 3 \
    "https://github.com/indygreg/apple-platform-rs/releases/download/apple-codesign%2F$version/$archive" \
    --output "$work/$archive"
tar -xzf "$work/$archive" -C "$work"
signer=$(find "$work" -type f -name rcodesign -print -quit)
test -x "$signer"

# Walk the deployed files so framework binaries and plugins are signed even
# when a framework's public name is a symlink into its Versions directory.
find "$bundle/Contents" -type f -print0 > "$work/files"
while IFS= read -r -d '' binary; do
    kind=$(file -b -- "$binary")
    if [[ "$kind" == Mach-O* ]]; then
        "$signer" sign "$binary"
    fi
done < "$work/files"

# Seal the application after signing its contents.
"$signer" sign "$bundle"
test -s "$bundle/Contents/_CodeSignature/CodeResources"
