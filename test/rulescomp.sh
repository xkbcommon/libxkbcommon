#!/bin/sh

srcdir=${srcdir-.}
builddir=${builddir-.}

compile()
{
    ${builddir}/rulescomp "$1" "$2" "$3" "$4" "$5" || exit $?
}

compile base pc105 us "" ""
compile evdev pc105 us intl ""
compile evdev pc105 us intl grp:alts_toggle
