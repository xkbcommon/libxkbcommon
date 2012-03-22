#!/bin/sh

srcdir=${srcdir-.}
builddir=${builddir-.}

name=filecomp
prog="$builddir/$name$EXEEXT"
log="$builddir/$name.log"

compile()
{
    echo "$prog '$1' ${2+'$2'}" >>"$log"
    $prog "$1" ${2+"$2"} >>"$log" 2>&1 || exit $?
}

failcompile()
{
    echo "$prog '$1' ${2+'$2'}" >>"$log"
    if $prog "$1" ${2+"$2"} >>"$log" 2>&1; then
        exit 1
    fi
}

rm -f "$log"

compile $srcdir/basic.xkb
# XXX check we actually get qwertz here ...
compile $srcdir/default.xkb
compile $srcdir/comprehensive-plus-geom.xkb
failcompile $srcdir/bad.xkb
