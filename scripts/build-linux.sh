#!/bin/bash

SCRIPT_DIR="$(dirname $(readlink -f "$0"))"
source $SCRIPT_DIR/config.sh 

pushd $SRC_DIR/linux

make ARCH=arm64 -j$(nproc)
popd