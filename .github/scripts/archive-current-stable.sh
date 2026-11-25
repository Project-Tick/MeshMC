#!/usr/bin/env bash
set -euo pipefail
: "${GH_TOKEN:?Set ARCHIVE_REPO_TOKEN in the Release environment}"

if ! stable="$(GH_TOKEN="$SOURCE_READ_TOKEN" gh release view stable --repo "$GITHUB_REPOSITORY" --json body 2>&1)"; then
    if [[ "$stable" == 'release not found' ]]; then
        exit 0
    fi
    echo "$stable" >&2
    exit 1
fi
body="$(jq -r '.body' <<< "$stable")"
old_build="$(sed -n '/^Updater build: [1-9][0-9]*$/ { s/^Updater build: //; p; q; }' <<< "$body")"
old_commit="$(sed -n '/^Source commit: [0-9a-f]*$/ { s/^Source commit: //; p; q; }' <<< "$body")"
if [[ -z "$old_build" || "$old_build" == "$BUILD_NUMBER" ]]; then
    exit 0
fi
[[ "$old_commit" =~ ^[0-9a-f]{40}$ ]]

repo=Project-Tick/MeshMC-Archive
if archive="$(gh release view "build-$old_build" --repo "$repo" --json isDraft 2>&1)"; then
    if jq -e '.isDraft == false' <<< "$archive" >/dev/null; then
        exit 0
    fi
elif [[ "$archive" != 'release not found' ]]; then
    echo "$archive" >&2
    exit 1
fi

download="$RUNNER_TEMP/previous-stable"
mkdir -p "$download"
GH_TOKEN="$SOURCE_READ_TOKEN" gh release download stable --repo "$GITHUB_REPOSITORY" \
    --dir "$download" --pattern 'MeshMC-stable-*' --pattern "MeshMC-build-$old_build-*"
BUILD_NUMBER="$old_build" STABLE_BUILD="$old_build" GITHUB_SHA="$old_commit" \
    ARCHIVE_SOURCE_URL="$GITHUB_SERVER_URL/$GITHUB_REPOSITORY/commit/$old_commit" \
    bash .github/scripts/publish-build-archive.sh "$download" "$old_build"
