#!/bin/bash

SCRIPT_DIR="$(dirname $(readlink -f "$0"))"
source $SCRIPT_DIR/config.sh 

pushd $SRC_DIR/optee_examples/hello_world/ta

if [[ "$(uname -m)" != "aarch64" ]]; then
    COMPILER_PATH=$THIRD_PARTY_DIR/$CROSS_COMPILE_DIR_LINUX/bin/
    make CROSS_COMPILE=$COMPILER_PATH/aarch64-none-linux-gnu- PLATFORM=vexpress PLATFORM_FLAVOR=fvp \
    TA_DEV_KIT_DIR=$SRC_DIR/optee_os/out/arm-plat-vexpress/export-ta_arm64
else
        make PLATFORM=vexpress PLATFORM_FLAVOR=fvp \
    TA_DEV_KIT_DIR=$SRC_DIR/optee_os/out/arm-plat-vexpress/export-ta_arm64
fi

popd