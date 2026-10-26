#!/usr/bin/env bash

DEPLOY="$1"; EXE="$2"; QT_PREFIX="$3"
MANIFEST="$(dirname "$(readlink -f "$0")")/deploy-manifest-linux.txt"

check_bundle() {
    local exe_name; exe_name="$(basename "$EXE")"
    find "$DEPLOY" -name '*.so*' -o -name "$exe_name" | while read -r f; do
        LD_LIBRARY_PATH="$DEPLOY/lib:$QT_PREFIX/lib" ldd "$f" 2>/dev/null \
          | awk -v f="$f" -v prefix="$QT_PREFIX" '/=>/ && $3 ~ "^"prefix {
                print "WARN: manifest missing " $1 " (needed by " f ")"
            }'
    done
}
check_bundle || true
