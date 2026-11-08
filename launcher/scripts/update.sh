#!/bin/sh
#
# Regenerates po/meshmc.pot from the sources in this repository.
#
# MeshMC uses KI18n, so the catalog is a plain gettext one: i18n()/i18nc()/
# i18np()/i18ncp() and the lazy kli18n*() variants are collected from the C++
# sources with xgettext, and the strings of the Qt Designer .ui files (which are
# compiled with `uic -tr tr2i18n`) are fed in through extract-ui-strings.py.
# Crowdin syncs against po/meshmc.pot (see /crowdin.yml).
#
# Requires xgettext (gettext) and python3. Override the binaries with
# XGETTEXT_BIN / PYTHON_BIN if they are not on PATH.

set -e

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO=$(git -C "$ROOT" rev-parse --show-toplevel)

XGETTEXT_BIN=${XGETTEXT_BIN:-xgettext}
PYTHON_BIN=${PYTHON_BIN:-python3}

# Directories scanned for translatable strings, relative to the repository root.
# Override with e.g. SRC_DIRS="launcher/ui" ./update.sh
SRC_DIRS=${SRC_DIRS:-"launcher"}

# plugins/staging/ is gated behind the MeshMC_STAGING_PLUGINS option (off by
# default), so those strings are not shipped and must not reach translators.
# Tests never show anything to the user.
EXCLUDE_RE=${EXCLUDE_RE:-^launcher/plugin/plugins/staging/|_test\.cpp$}

TEMPLATE="$REPO/po/meshmc.pot"

WORK=$(mktemp -d)
cleanup() {
    rm -rf "$WORK"
}
trap cleanup EXIT

cd "$REPO"

echo "Collecting files..."
# Unquoted on purpose: SRC_DIRS is a whitespace separated list.
# shellcheck disable=SC2086
LC_ALL=C find $SRC_DIRS -type f \
    \( -name '*.h' -o -name '*.cpp' -o -name '*.mm' -o -name '*.ui' \) \
    | grep -Ev "$EXCLUDE_RE" | LC_ALL=C sort > "$WORK/all.lst"

grep -v '\.ui$' "$WORK/all.lst" > "$WORK/code.lst" || true
grep '\.ui$' "$WORK/all.lst" > "$WORK/ui.lst" || true
echo "    $(wc -l < "$WORK/code.lst") source files, $(wc -l < "$WORK/ui.lst") .ui files"

echo "Extracting strings from .ui files..."
# The stream is written as a .cpp file so xgettext treats it as C++. Its #line
# directives make the references in the .pot point at the .ui files.
# shellcheck disable=SC2046
"$PYTHON_BIN" "$ROOT/extract-ui-strings.py" "$REPO" $(cat "$WORK/ui.lst") \
    > "$WORK/ui-strings.cpp"
echo "$WORK/ui-strings.cpp" >> "$WORK/code.lst"

echo "Generating new template..."
mkdir -p "$REPO/po"
# --kde enables the KDE format flags and the i18n* keywords; the kli18n* ones
# (KLazyLocalizedString) are not part of it and are added by hand.
"$XGETTEXT_BIN" \
    --from-code=UTF-8 \
    --language=C++ \
    --kde \
    --add-comments=i18n \
    --sort-by-file \
    --package-name=MeshMC \
    --msgid-bugs-address=https://github.com/Project-Tick/MeshMC/issues \
    --copyright-holder="Project Tick" \
    -kkli18n:1 \
    -kkli18nc:1c,2 \
    -kkli18np:1,2 \
    -kkli18ncp:1c,2,3 \
    --files-from="$WORK/code.lst" \
    -o "$WORK/meshmc.pot"

# xgettext leaves the charset as a placeholder; every catalog here is UTF-8.
# The creation date changes on every run and would make every regeneration a
# diff, so it is dropped.
sed -e 's/charset=CHARSET/charset=UTF-8/' \
    -e '/^"POT-Creation-Date:/d' \
    "$WORK/meshmc.pot" > "$TEMPLATE"

echo "    $(grep -c '^msgid ' "$TEMPLATE") messages (incl. header) in po/meshmc.pot"
echo "All done!"
