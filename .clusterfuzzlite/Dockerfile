FROM gcr.io/oss-fuzz-base/base-builder:v1
RUN apt-get update && apt-get install -y make autoconf automake libtool meson bison libxml2-dev pkg-config valgrind
COPY . $SRC/libxkbcommon
WORKDIR $SRC/libxkbcommon
COPY .clusterfuzzlite/build.sh $SRC/
