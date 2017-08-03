#!/usr/bin/env bash
set -e

# Check that all exported symbols are specified in the symbol version
# scripts.  If this fails, please update the appropriate .map file
# (adding new version nodes as needed).

# xkbcommon symbols
diff -a -u \
    <(grep -h '^\s\+xkb_' "$top_srcdir"/xkbcommon.map | sed -e 's/^\s\+\(.*\);/\1/' | sort) \
    <(grep -h 'XKB_EXPORT' -A1 "$top_srcdir"/src/{,xkbcomp,compose}/*.c | grep '^xkb_' | sed -e 's/(.*//' | sort)

# xkbcommon-x11 symbols
diff -a -u \
    <(grep -h '^\s\+xkb_.*' "$top_srcdir"/xkbcommon-x11.map | sed -e 's/^\s\+\(.*\);/\1/' | sort) \
    <(grep -h 'XKB_EXPORT' -A1 "$top_srcdir"/src/x11/*.c | grep '^xkb_' | sed -e 's/(.*//' | sort)
