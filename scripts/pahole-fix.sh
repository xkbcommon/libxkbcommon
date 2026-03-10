#!/bin/bash

binary="$1"

pahole --anon_include --nested_anon_include --packable "${@:2}" "$binary" \
    | sort -k4 -nr \
    | awk '{print $1}' \
    | while read name; do
    echo "=== $name (savings: $(pahole --anon_include --nested_anon_include --packable "${@:2}" "$binary" \
        | awk -v n="$name" '$1==n {print $4}') bytes) ==="
    diff -u -U 9999 \
        <(pahole --class_name "$name" --anon_include --nested_anon_include --show_decl_info "${@:2}" "$binary") \
        <(pahole --class_name "$name" --anon_include --nested_anon_include --show_decl_info --reorganize --fixup_silly_bitfields "${@:2}" "$binary")
done
