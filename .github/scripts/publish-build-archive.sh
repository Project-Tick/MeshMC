#!/usr/bin/env bash
set -euo pipefail

repo=Project-Tick/MeshMC-Archive
[[ "$BUILD_NUMBER" =~ ^[1-9][0-9]{0,4}$ && "$GITHUB_SHA" =~ ^[0-9a-f]{40}$ ]]
: "${GH_TOKEN:?Set ARCHIVE_REPO_TOKEN in the Release environment}"

tag="build-$BUILD_NUMBER"
dist="$RUNNER_TEMP/meshmc-archive-$tag"
bash .github/scripts/collect-release-packages.sh "$1" "$dist" "MeshMC-$tag" "${2:-}"

if ! release="$(gh release view "$tag" --repo "$repo" --json apiUrl,body,isDraft 2>&1)"; then
    if [[ "$release" != 'release not found' ]]; then
        echo "$release" >&2
        exit 1
    fi
    branch="$(gh api "repos/$repo" --jq .default_branch)"
    if ! gh api "repos/$repo/branches/$branch" >/dev/null; then
        echo "::error::$repo needs an existing default branch before build releases can be published."
        exit 1
    fi
    channel="$GITHUB_REF_NAME"
    if [[ "$STABLE_BUILD" == "$BUILD_NUMBER" ]]; then
        channel=stable
    fi
    source_url="${ARCHIVE_SOURCE_URL:-$GITHUB_SERVER_URL/$GITHUB_REPOSITORY/actions/runs/$GITHUB_RUN_ID}"
    notes="$(printf 'Source commit: %s\nChannel: %s\n\n%s\n' "$GITHUB_SHA" "$channel" "$source_url")"
    gh release create "$tag" --repo "$repo" --target "$branch" \
        --title "MeshMC $tag" --notes "$notes" --draft --latest=false
    release="$(gh release view "$tag" --repo "$repo" --json apiUrl,body,isDraft)"
fi

source_commit="$(jq -r '.body' <<< "$release" | sed -n '/^Source commit: [0-9a-f]*$/ { s/^Source commit: //; p; q; }')"
if [[ "$source_commit" != "$GITHUB_SHA" ]]; then
    echo "::error::$tag is already associated with a different source commit."
    exit 1
fi
release_id="$(jq -er '.apiUrl | split("/") | last | tonumber' <<< "$release")"
endpoint="repos/$repo/releases/$release_id"
assets="$(gh api "$endpoint/assets?per_page=100")"
pending=()
draft="$(jq -er '.isDraft | tostring' <<< "$release")"
for file in "$dist"/*; do
    name="${file##*/}"
    digest="sha256:$(sha256sum "$file" | cut -d ' ' -f1)"
    if jq -e --arg name "$name" --arg digest "$digest" \
        'any(.[]; .name == $name and .state == "uploaded" and .digest == $digest)' <<< "$assets" >/dev/null; then
        continue
    fi
    if [[ "$draft" != true ]]; then
        echo "::error::Published archive $tag is missing the expected contents of $name."
        exit 1
    fi
    pending+=("$file")
done
if (( ${#pending[@]} )); then
    gh release upload "$tag" "${pending[@]}" --clobber --repo "$repo"
fi
if [[ "$draft" == true ]]; then
    gh api --method PATCH "$endpoint" -F draft=false -f make_latest=false >/dev/null
fi
