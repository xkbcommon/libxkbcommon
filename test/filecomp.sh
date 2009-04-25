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
compile $srcdir/named.xkb
compile $srcdir/named.xkb de
compile $srcdir/named.xkb us
compile $srcdir/default.xkb

failcompile $srcdir/basic.xkb foo
failcompile $srcdir/named.xkb foo
failcompile $srcdir/bad.xkb
