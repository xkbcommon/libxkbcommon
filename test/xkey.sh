#!/bin/sh

srcdir=${srcdir-.}
builddir=${builddir-.}

check_error()
{
    if [ "$2" != "$3" ]; then
        echo "error checking $1" >&2
        echo "  expected: $2" >&2
        echo "  received: $3" >&2
        return 1
    fi
}

check_string()
{
    val=`${builddir}/xkey -s "$1"` &&
        check_error "$1" "$2" "$val" ||
        exit $?
}

check_key()
{
    val=`${builddir}/xkey -k "$1"` && \
        check_error "$1" "$2" "$val" || \
        exit $?
}

check_string Undo 0xff65
check_key 0x1008FF56 XF86Close
check_string ThisKeyShouldNotExist NoSymbol
check_key 0x0 NULL
