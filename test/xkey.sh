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

val=`${builddir}/xkey -s Undo` && \
    check_error Undo 0xff65 $val || \
    exit $?

val=`${builddir}/xkey -k 0x1008ff56` && \
    check_error 0x1008FF56 XF86Close $val || \
    exit $?

val=`${builddir}/xkey -s ThisKeyShouldNotExist` && \
    check_error ThisKeyShouldNotExist NoSymbol $val || \
    exit $?

val=`${builddir}/xkey -k 0x0` && \
    check_error 0x0 NULL $val || \
    exit $?
