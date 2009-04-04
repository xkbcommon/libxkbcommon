#!/bin/sh

srcdir=${srcdir-.}
builddir=${builddir-.}

log="$builddir/rulescomp.log"

compile()
{
    echo "${builddir}/rulescomp '$1' '$2' '$3' '$4' '$5'" >>"$log"
    ${builddir}/rulescomp "$1" "$2" "$3" "$4" "$5" >>"$log" 2>&1 || exit $?
}

failcompile()
{
    echo "${builddir}/rulescomp '$1' '$2' '$3' '$4' '$5'" >>"$log"
    if ${builddir}/rulescomp "$1" "$2" "$3" "$4" "$5" >>"$log" 2>&1; then
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
