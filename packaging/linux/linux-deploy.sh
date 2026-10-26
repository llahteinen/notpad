#!/usr/bin/env bash

# deploy.sh -- usage: deploy.sh <deploy_dir> <exe> <qt_prefix> <patchelf>
# qt_prefix == path to Qt install dir
# patchelf == path to patchelf utility

set -euo pipefail
shopt -s nullglob

DEPLOY="$1"; EXE="$2"; QT_PREFIX="$3"; PATCHELF="$4"
MANIFEST="$(dirname "$(readlink -f "$0")")/deploy-manifest-linux.txt"

[ -d "$QT_PREFIX/plugins/platforms" ] || { echo "ERROR: '$QT_PREFIX' is not a Qt prefix" >&2; exit 1; }

mkdir -p "$DEPLOY/lib"
cp "$EXE" "$DEPLOY/"
"$PATCHELF" --set-rpath '$ORIGIN/lib' "$DEPLOY/$(basename "$EXE")"

# libs: copy real content under the SONAME name (fixes the .6.12.0-only problem)
grep -E '^lib.+\.so' "$MANIFEST" | while read -r soname; do
    [ -e "$QT_PREFIX/lib/$soname" ] || { echo "ERROR: $QT_PREFIX/lib/$soname missing" >&2; exit 1; }
    cp -L "$QT_PREFIX/lib/$soname" "$DEPLOY/lib/$soname"
done

# plugins: exact files, nothing else (no .debug etc)
grep -E '^[a-z0-9-]+/.+\.so' "$MANIFEST" | while read -r rel; do
    [ -e "$QT_PREFIX/plugins/$rel" ] || { echo "ERROR: $QT_PREFIX/plugins/$rel missing" >&2; exit 1; }
    mkdir -p "$DEPLOY/$(dirname "$rel")"
    cp "$QT_PREFIX/plugins/$rel" "$DEPLOY/$rel"
done

# set rpath by depth
for elf in "$DEPLOY"/lib/*.so*; do "$PATCHELF" --set-rpath '$ORIGIN' "$elf";      done
for elf in "$DEPLOY"/*/*.so;    do "$PATCHELF" --set-rpath '$ORIGIN/../lib' "$elf"; done

# strip
find "$DEPLOY" \( -name '*.so*' -o -name "$(basename "$EXE")" \) -exec strip --strip-unneeded {} +

echo "Deployed $(find "$DEPLOY" -type f | wc -l) files to $DEPLOY"
