#!/bin/sh

srcdir=${srcdir-.}
builddir=${builddir-.}

name=context
prog="$builddir/$name$EXEEXT"
log="$builddir/$name.log"

rm -f "$log"
srcdir=${srcdir-.}
builddir=${builddir-.}
$builddir/$name$EXEEXT >> $log 2>&1
