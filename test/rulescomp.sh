#!/bin/sh

srcdir=${srcdir-.}
builddir=${builddir-.}

name=rulescomp
prog="$builddir/$name$EXEEXT"
log="$builddir/$name.log"

compile()
{
    echo "$prog '$1' '$2' '$3' '$4' '$5'" >>"$log"
    $prog "$1" "$2" "$3" "$4" "$5" >>"$log" 2>&1 || exit $?
}

failcompile()
{
    echo "$prog '$1' '$2' '$3' '$4' '$5'" >>"$log"
    if $prog "$1" "$2" "$3" "$4" "$5" >>"$log" 2>&1; then
        exit 1
    fi
}

rm -f "$log"

compile base pc105 us "" ""
compile base "" us "" ""
compile evdev pc105 us intl ""
compile evdev pc105 us intl grp:alts_toggle

failcompile "" "" "" "" "" ""
failcompile base "" "" "" "" ""
failcompile base pc105 "" "" "" ""
failcompile badrules "" us "" "" ""
