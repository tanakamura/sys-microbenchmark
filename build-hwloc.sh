#!/bin/sh

if ! test -r hwloc-2.4.0/README
then
    wget https://download.open-mpi.org/release/hwloc/v2.4/hwloc-2.4.0.tar.bz2 || exit 1
    tar -xf hwloc-2.4.0.tar.bz2
fi

mkdir -p hwloc-build || exit 1
cd hwloc-build || exit 1
../hwloc-2.4.0/configure CFLAGS=-O2 --disable-libudev --disable-debug --enable-embedded-mode --disable-libxml2 --disable-io --enable-static --disable-shared --with-pic --prefix=$PWD/hwloc  $@ || exit 1
make -j2
make install

