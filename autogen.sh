#! /bin/sh

srcdir=`dirname "$0"`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd "$srcdir"

autoreconf --verbose --install --symlink --warnings=all || exit 1
cd "$ORIGDIR" || exit $?

exec "$srcdir/configure" --enable-maintainer-mode "$@"
