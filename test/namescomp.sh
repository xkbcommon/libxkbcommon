#!/bin/sh

srcdir=${srcdir-.}
builddir=${builddir-.}

name=namescomp
prog="$builddir/$name$EXEEXT"
log="$builddir/$name.log"

compile()
{
    echo "$prog '$1' '$2' '$3' '$4'" >>"$log"
    $prog "$1" "$2" "$3" "$4" >>"$log" 2>&1 || exit $?
}

failcompile()
{
    echo "$prog '$1' '$2' '$3' '$4'" >>"$log"
    if $prog "$1" "$2" "$3" "$4" >>"$log" 2>&1; then
        exit 1
    fi
}

rm -f "$log"

compile "xfree86+aliases(qwertz)" "complete" "complete" "pc+de"
compile "xfree86+aliases(qwerty)" "complete" "complete" "pc+us"
compile "xfree86+aliases(qwertz)" "complete" "complete" \
    "pc+de+level3(ralt_switch_for_alts_toggle)+group(alts_toggle)"

failcompile "" "" "" ""
failcompile "xfree86+aliases(qwerty)" "" "" ""
failcompile "xfree86+aliases(qwertz)" "" "" "pc+de"
failcompile "xfree86+aliases(qwertz)" "complete" "" "pc+de"
failcompile "xfree86+aliases(qwertz)" "" "complete" "pc+de"
failcompile "xfree86+aliases(qwertz)" "complete" "complete" ""
failcompile "badnames" "complete" "pc+us" "pc(pc101)"
