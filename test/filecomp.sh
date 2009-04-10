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

compile basic.xkb
compile named.xkb
compile named.xkb de
compile named.xkb us
compile default.xkb

failcompile basic.xkb foo
failcompile named.xkb foo
failcompile bad.xkb
