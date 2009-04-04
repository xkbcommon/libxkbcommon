#!/bin/sh

srcdir=${srcdir-.}
builddir=${builddir-.}

log="$builddir/xkey.log"

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
    echo "${builddir}/xkey -s '$1'" >>"$log"
    val=`${builddir}/xkey -s "$1"` &&
        echo "$val" >>"$log" &&
        check_error "$1" "$2" "$val" >>"$log" 2>&1 ||
        exit $?
}

check_key()
{
    echo "${builddir}/xkey -k '$1'" >>"$log"
    val=`${builddir}/xkey -k "$1"` && \
        echo "$val" >>"$log" &&
        check_error "$1" "$2" "$val" >>"$log" 2>&1 || \
        exit $?
}

rm -f "$log"

check_string Undo 0xFF65
check_key 0x1008FF56 XF86Close
check_string ThisKeyShouldNotExist NoSymbol
check_key 0x0 NULL
check_string XF86_Switch_VT_5 0x1008FE05
check_key 0x1008FE20 XF86_Ungrab
