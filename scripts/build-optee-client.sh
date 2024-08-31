#!/bin/bash

SCRIPT_DIR="$(dirname $(readlink -f "$0"))"
source $SCRIPT_DIR/config.sh 

if [[ "$(uname -m)" != "aarch64" ]]; then
    COMPILER_PATH=$THIRD_PARTY_DIR/$CROSS_COMPILE_DIR_LINUX/bin/aarch64-none-linux-gnu-gcc
else
    COMPILER_PATH=gcc
fi

pushd $SRC_DIR/optee_client

rm -rf build
rm -rf tmp_output
mkdir build
mkdir tmp_output
pushd $SRC_DIR/optee_client/build

cmake -DCMAKE_C_COMPILER=$COMPILER_PATH -DCMAKE_INSTALL_PREFIX=$SRC_DIR/optee_client/tmp_output ..

make
make install

popd
popd
