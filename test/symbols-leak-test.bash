#!/usr/bin/env bash
set -o pipefail -o errexit -o nounset

tempdir=$(mktemp -d "$top_builddir"/symbols-leak-test.XXXXXXXXXX)
trap 'rm -rf "$tempdir"' EXIT

# Check that all exported symbols are specified in the symbol version
# scripts.  If this fails, please update the appropriate .map file
# (adding new version nodes as needed).

# xkbcommon symbols
grep -h '^\s\+xkb_' "$top_srcdir"/xkbcommon.map | sed -e 's/^\s\+\(.*\);/\1/' | sort > "$tempdir"/symbols
grep -h 'XKB_EXPORT' -A1 "$top_srcdir"/src/{,xkbcomp,compose}/*.c | grep '^xkb_' | sed -e 's/(.*//' | sort > "$tempdir"/exported
diff -a -u "$tempdir"/symbols "$tempdir"/exported

# xkbcommon-x11 symbols
grep -h '^\s\+xkb_.*' "$top_srcdir"/xkbcommon-x11.map | sed -e 's/^\s\+\(.*\);/\1/' | sort > "$tempdir"/symbols
grep -h 'XKB_EXPORT' -A1 "$top_srcdir"/src/x11/*.c | grep '^xkb_' | sed -e 's/(.*//' | sort > "$tempdir"/exported
diff -a -u "$tempdir"/symbols "$tempdir"/exported
