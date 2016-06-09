#!/usr/bin/env bash
set -e

# Check that all exported symbols are specified in the symbol
# version scripts.  If this fails, please update the appropriate
# (adding new version nodes when needed).

# xkbcommon symbols
diff -a -u \
    <(cat "$top_srcdir"/xkbcommon.map | \
        grep '^\s\+xkb_.*' | \
        sed -e 's/^\s\+\(.*\);/\1/' | sort) \
    <(cat "$top_srcdir"/src/{,xkbcomp,compose}/*.c | \
        grep XKB_EXPORT -A 1 | grep '^xkb_.*' | \
        sed -e 's/(.*//' | sort)

# xkbcommon-x11 symbols
diff -a -u \
    <(cat "$top_srcdir"/xkbcommon-x11.map | \
        grep '^\s\+xkb_.*' | \
        sed -e 's/^\s\+\(.*\);/\1/' | sort) \
    <(cat "$top_srcdir"/src/x11/*.c | \
        grep XKB_EXPORT -A 1 | grep '^xkb_.*' | \
        sed -e 's/(.*//' | sort)
