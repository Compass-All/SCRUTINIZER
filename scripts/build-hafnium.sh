#!/bin/bash

SCRIPT_DIR="$(dirname $(readlink -f "$0"))"
source $SCRIPT_DIR/config.sh 

if [[ "$(uname -m)" != "aarch64" ]]; then
    export PATH=$THIRD_PARTY_DIR/$CROSS_COMPILE_DIR/bin:$THIRD_PARTY_DIR/$CROSS_COMPILE_CLANG_DIR/bin:${PATH}
else
    export PATH=$THIRD_PARTY_DIR/$CROSS_COMPILE_DIR_AARCH64/bin:$THIRD_PARTY_DIR/$CROSS_COMPILE_CLANG_DIR_AARCH64/bin:${PATH}
fi

pushd $THIRD_PARTY_DIR/$HF
make clean
# make PROJECT=reference
make PLATFORM="secure_aem_v8a_fvp"
popd
