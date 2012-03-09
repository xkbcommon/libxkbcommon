#!/bin/sh -x

srcdir=${srcdir-.}
builddir=${builddir-.}

name=canonicalise
prog="$builddir/$name$EXEEXT"
log="$builddir/$name.log"

log_kccgst()
{
    echo "  keycodes: $1" >>"$log"
    echo "  compat: $2" >>"$log"
    echo "  symbols: $3" >>"$log"
    echo "  types: $4" >>"$log"
}

rm -f "$log"

test() {
    ret=`$prog $2 $3`
    echo "Input (new):" >>"$log"
    log_kccgst $2
    echo >>"$log"
    echo "Input (old):" >>"$log"
    log_kccgst $3
    echo >>"$log"
    echo "Expecting:" >>"$log"
    log_kccgst $1
    echo >>"$log"
    echo "Received:" >>"$log"
    log_kccgst $ret
    echo >>"$log"

    ret=`echo "$ret" | sed -e 's/[  ]*/ /g;'`
    exp=`echo "$1" | sed -e 's/[  ]*/ /g;'`

    if ! [ "$ret" = "$exp" ]; then
        echo "Error: Return and expectations different" >>"$log"
        exit 1
    fi
}

# This is a bit of a horror, but I can't really remember how to properly
# handle arrays in shell, and I'm offline.
twopart_new="+inet(pc104)        %+complete     pc(pc104)+%+ctrl(nocaps)          |complete"
twopart_old="xfree86             basic          us(dvorak)                        xfree86"
twopart_exp="xfree86+inet(pc104) basic+complete pc(pc104)+us(dvorak)+ctrl(nocaps) xfree86|complete"

onepart_new="evdev               complete       pc(pc104)+us+compose(ralt)        complete"
onepart_exp="evdev               complete       pc(pc104)+us+compose(ralt)        complete"

test "$twopart_exp" "$twopart_new" "$twopart_old"
echo >>"$log"
echo >>"$log"
test "$onepart_exp" "$onepart_new"
