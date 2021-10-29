#!/bin/bash

set -xe -o pipefail

#rm -rf build_test
mkdir -p build_test

(
    cd build_test

    mkdir -p build_self
    (
        cd build_self
        cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ../..
        ninja
    )

    mkdir -p build_aarch64
    (
        cd build_self
        cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../aarch64-linux-gnu-toolchain.cmake  ../..
        ninja
    )

    mkdir -p build_wasi
    (
        cd build_wasi
        export WASI_SDK_PATH=/opt/wasi-sdk/wasi-sdk-12.0
        cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../wasi-toolchain.cmake -DWASI_SDK_PATH=${WASI_SDK_PATH}  ../..
        ninja
    )

    mkdir -p build_em
    (
        cd build_em
        emcmake cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ../..
        ninja
    )

#    mkdir -p build_mingw
#    (
#        cd build_mingw
#        cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../mingw-w64-toolchain.cmake ../..
#        ninja
#    )

)
