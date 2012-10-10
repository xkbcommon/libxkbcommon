#!/bin/sh -e

srcdir=`dirname "$0"`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd "$srcdir"

autoreconf --verbose --install --symlink --warnings=all
cd "$ORIGDIR"

if test -z "$NOCONFIGURE"; then
    exec "$srcdir/configure" "$@"
fi
