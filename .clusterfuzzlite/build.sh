#!/bin/bash -eu
# https://google.github.io/clusterfuzzlite/build-integration/

: ${LD:="${CXX}"}
: ${LDFLAGS:="${CXXFLAGS}"}

rm -rf build
meson setup build -Denable-x11=false -Denable-wayland=false -Denable-docs=false
ninja -C build

# "even if your project is written in pure C you must use $CXX to link your fuzz target binaries"
$CXX $CXXFLAGS -std=c++11 -Ibuild/ $SRC/fuzz/compose/target.c -o $OUT/compose_fuzzer $LIB_FUZZING_ENGINE build/libxkbcommon.so
