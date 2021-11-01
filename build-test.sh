#!/bin/bash

set -xe -o pipefail

#rm -rf build_test
mkdir -p build_test

#meson setup --buildtype release build_test/build_self
#(
#    cd build_test/build_self
#    ninja
#)

#meson setup --cross-file aarch64-linux-gnu-cross.ini --buildtype release build_test/build_aarch64
#(
#    cd build_test/build_aarch64
#    ninja
#)


export PATH=$PATH:$PWD/tools
meson setup --cross-file wasi-cross.ini --buildtype release build_test/build_wasi
(
    cd build_test/build_wasi
    ninja
)

#    mkdir -p build_wasi
#    (
#        cd build_wasi
#        export WASI_SDK_PATH=/opt/wasi-sdk/wasi-sdk-12.0
#        cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../wasi-toolchain.cmake -DWASI_SDK_PATH=${WASI_SDK_PATH}  ../..
#        ninja
#    )

#meson setup --cross-file emsdk-cross.ini --buildtype release build_test/build_em
#(
#    cd build_test/build_em
#    ninja
#)

#meson setup --cross-file mingw-cross.ini --buildtype release build_test/build_mingw
#(
#    cd build_test/build_mingw
#    ninja
#)

