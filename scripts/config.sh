#!/bin/bash

# packages & sources
export CROSS_COMPILE_SRC="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-a/10.3-2021.07/binrel/gcc-arm-10.3-2021.07-x86_64-aarch64-none-elf.tar.xz"
export CROSS_COMPILE_SRC_AARCH64="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-a/10.3-2021.07/binrel/gcc-arm-10.3-2021.07-aarch64-aarch64-none-elf.tar.xz"
export CROSS_COMPILE_CLANG_SRC="https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar.xz"
export CROSS_COMPILE_CLANG_SRC_AARCH64="https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/clang+llvm-15.0.6-aarch64-linux-gnu.tar.xz"
export CROSS_COMPILE_SRC_LINUX="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-a/10.3-2021.07/binrel/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu.tar.xz"
export CROSS_COMPILE_SRC_LINUX_TA="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-linux-gnu.tar.xz"

export FVP_SRC="https://developer.arm.com/-/cdn-downloads/permalink/Fixed-Virtual-Platforms/FM-11.26/FVP_Base_RevC-2xAEMvA_11.26_11_Linux64.tgz"
export FVP_SRC_AARCH64="https://developer.arm.com/-/cdn-downloads/permalink/Fixed-Virtual-Platforms/FM-11.26/FVP_Base_RevC-2xAEMvA_11.26_11_Linux64_armv8l.tgz"

export OELAMP_IMG_SRC="https://releases.linaro.org/openembedded/juno-lsk/15.09/lt-vexpress64-openembedded_lamp-armv8-gcc-4.9_20150912-729.img.gz"
export ROOTFS_SRC="host-fs.7z"
# SH
export SH="/bin/bash"


# project structure directories
export SCRIPT_DIR="$(dirname $(readlink -f "$0"))"
export PROJ_DIR="$(dirname $SCRIPT_DIR)"
export SRC_DIR=$PROJ_DIR/src
export PROJ_CONF_DIR=$PROJ_DIR/configs
export THIRD_PARTY_DIR=$PROJ_DIR/dev_workspace
export DBG_DIR=$PROJ_DIR/debug 
export LOG_DIR=$PROJ_DIR/debug/logs
export ETE_DECODER=$PROJ_DIR/scripts/trace

if [[ "$(uname -m)" != "aarch64" ]]; then
    export FVP_CRYPTO_LIB=$PROJ_CONF_DIR/Crypto.so
else
    export FVP_CRYPTO_LIB=$PROJ_CONF_DIR/Crypto_arm.so
fi

# package dsts
export CROSS_COMPILE_DIR="gcc-arm-10.3-2021.07-x86_64-aarch64-none-elf"
export CROSS_COMPILE_DIR_AARCH64="gcc-arm-10.3-2021.07-aarch64-aarch64-none-elf"
export CROSS_COMPILE_CLANG_DIR="clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04"
export CROSS_COMPILE_CLANG_DIR_AARCH64="clang+llvm-15.0.6-aarch64-linux-gnu"
export CROSS_COMPILE_DIR_LINUX="gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu"
export CROSS_COMPILE_DIR_LINUX_TA="arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-linux-gnu"

export FVP_DIR="Base_RevC_AEMvA_pkg"
# export OELAMP_IMG="lt-vexpress64-openembedded_lamp-armv8.img"
export ROOTFS="host-fs.ext4"
export HF="hafnium"
export RMM="tf-rmm"



# bin locations
if [[ "$(uname -m)" != "aarch64" ]]; then
    export CROSS_COMPILE=$THIRD_PARTY_DIR/$CROSS_COMPILE_DIR/bin/aarch64-none-elf-
    export CROSS_COMPILE_LINARO=$CROSS_COMPILE
else
    export CROSS_COMPILE=$THIRD_PARTY_DIR/$CROSS_COMPILE_DIR_AARCH64/bin/aarch64-none-elf-
    export CROSS_COMPILE_LINARO=$CROSS_COMPILE
fi

if [[ "$(uname -m)" == "aarch64" ]]; then
    export FVP=$THIRD_PARTY_DIR/$FVP_DIR/models/Linux64_armv8l_GCC-9.3/FVP_Base_RevC-2xAEMvA
else
    export FVP=$THIRD_PARTY_DIR/$FVP_DIR/models/Linux64_GCC-9.3/FVP_Base_RevC-2xAEMvA
fi

if [[ "$(uname -m)" != "aarch64" ]]; then
    FVP_PLUGIN_DIR=$THIRD_PARTY_DIR/$FVP_DIR/plugins/Linux64_GCC-9.3
else
    FVP_PLUGIN_DIR=$THIRD_PARTY_DIR/$FVP_DIR/plugins/Linux64_armv8l_GCC-9.3
fi


export MODEL=${FVP}
# color
export GREEN='\033[0;32m'
export NC='\033[0m' # No Color
export RED='\033[0;31m'
# repo command
# export PATH=${THIRD_PARTY_DIR}/.bin:${PATH}
# export PATH=${THIRD_PARTY_DIR}/.bin:${PATH}

#
# Functions
#

print_config() {
    echo "=== Configurations ==="
    echo "=> THIRD_PARTY_DIR=$PROJ_DIR/toolchains"
    echo "=> CROSS_COMPILE=$CROSS_COMPILE"
}

log() {
    printf "${GREEN}$1${NC}\n"
}

log_error() {
    printf "${RED}$1${NC}\n"
}