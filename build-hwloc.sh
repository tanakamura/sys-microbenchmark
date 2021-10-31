#!/bin/sh

set -e -o pipefail

HWLOC_DIR=$1

(
    cd $HWLOC_DIR
    if ! test -x configure
    then
        ./autogen.sh
    fi
)

shift 1
mkdir -p hwloc-build || exit 1

(
    cd hwloc-build || exit 1
    $HWLOC_DIR/configure CFLAGS=-O2 --disable-libudev --disable-debug --enable-embedded-mode --disable-libxml2 --disable-io --enable-static --disable-shared --with-pic --prefix=$PWD/hwloc  $@ || exit 1
    make -j4
    make install
)

cp hwloc-build/hwloc/.libs/libhwloc_embedded.a .

