#!/bin/bash

SCRIPT_DIR="$(dirname $(readlink -f "$0"))"
source $SCRIPT_DIR/config.sh 

if [[ "$(uname -m)" != "aarch64" ]]; then
    export PATH=$THIRD_PARTY_DIR/$CROSS_COMPILE_DIR/bin:${PATH}
else
    export PATH=$THIRD_PARTY_DIR/$CROSS_COMPILE_DIR_AARCH64/bin:${PATH}
fi

export PATH=/snap/cmake/current/bin:$PATH

# pushd $SRC_DIR/rmm
pushd $THIRD_PARTY_DIR/$RMM
cmake -DRMM_CONFIG=fvp_defcfg \
        -DCMAKE_BUILD_TYPE=Debug \
        -DLOG_LEVEL=40 \
          -S . \
          -B build 
cmake --build build
popd
