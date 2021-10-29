#!/bin/bash

set -xe -o pipefail

#rm -rf build_test
mkdir -p build_test

(
    cd build_test
    mkdir -p build_wasi


    (
        cd build_wasi
        export WASI_SDK_PATH=/opt/wasi-sdk/wasi-sdk-12.0
        cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=../../wasi-toolchain.cmake -DWASI_SDK_PATH=${WASI_SDK_PATH}  ../..
        ninja
    )
)
