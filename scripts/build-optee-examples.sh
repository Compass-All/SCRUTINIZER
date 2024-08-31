#!/bin/bash


SCRIPT_DIR="$(dirname $(readlink -f "$0"))"
source $SCRIPT_DIR/config.sh 

if [[ "$(uname -m)" != "aarch64" ]]; then
    COMPILER_PATH=$THIRD_PARTY_DIR/$CROSS_COMPILE_DIR_LINUX_TA/bin/aarch64-none-linux-gnu-gcc
else
    COMPILER_PATH=gcc
fi

pushd $SRC_DIR/optee_examples

rm -rf build
mkdir build
pushd $SRC_DIR/optee_examples/build

cmake -DCMAKE_C_COMPILER=$COMPILER_PATH ..
make
popd
popd