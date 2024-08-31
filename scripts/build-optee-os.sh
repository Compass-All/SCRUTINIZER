#!/bin/bash

SCRIPT_DIR="$(dirname $(readlink -f "$0"))"
source $SCRIPT_DIR/config.sh 

OPTEE_OS_PATH="${SRC_DIR}/optee_os"
SC_SP_LAYOUT="${SRC_DIR}/tf-a/s-layout/"
OPTEE_COMPILER=$CROSS_COMPILE

    echo "Building OPTEE-OS"
 
BUILD_FLAGS="\
 	CROSS_COMPILE_core=${OPTEE_COMPILER} \
        CROSS_COMPILE_ta_arm64=${OPTEE_COMPILER} \
        PLATFORM=vexpress \
        PLATFORM_FLAVOR=fvp \
        CFG_ARM64_core=y \
        CFG_TEE_CORE_LOG_LEVEL=4 \
        CFG_TEE_CORE_DEBUG=y \
        CFG_TEE_BENCHMARK=n \
        CFG_WITH_STATS=y \
        CFG_ARM_GICV3=y \
        CFG_CORE_SEL2_SPMC=y \
        CFG_USER_TA_TARGETS=ta_arm64 \
        "
    pushd $OPTEE_OS_PATH
    make -j $PARALLELISM ${BUILD_FLAGS}
    # make -j $PARALLELISM ${BUILD_FLAGS} mem_usage

    cp out/arm-plat-vexpress/core/tee-pager_v2.bin $SC_SP_LAYOUT
    popd
