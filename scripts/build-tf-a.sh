#!/bin/bash

SCRIPT_DIR="$(dirname $(readlink -f "$0"))"
source $SCRIPT_DIR/config.sh 


SC_SP_LAYOUT="${SRC_DIR}/tf-a/s-layout/sp_layout.json"

# *** RMM Options *** #
# RMM_PATH="${SRC_DIR}/rmm"
RMM_PATH=$THIRD_PARTY_DIR/$RMM
RMM_BUILD_DIR="${RMM_PATH}/build"
RMM_BUILD_TYPE=Debug
RMM_LOG_LEVEL=40
RMM_OUT_BIN="${RMM_BUILD_DIR}/${RMM_BUILD_TYPE}/rmm.img"
# *** End RMM *** #


# *** Hafnium Options *** #
HAFNIUM_PATH=$THIRD_PARTY_DIR/$HF
HAFNIUM_OUT_BIN="${HAFNIUM_PATH}/out/reference/secure_aem_v8a_fvp_clang/hafnium.bin"
# *** End Hafnium *** #



# *** TF-A Options *** #
TF_A_RME_BUILD_ENABLED=1
TF_A_PATH="${SRC_DIR}/tf-a"
TF_A_ARCH=aarch64
TF_A_PLATS=fvp
TF_A_BUILD_TYPE="debug" # debug=1, release=0

# TF-A 4-World 	
# config OPTEE with hafnium using ARM_SPMC_MANIFEST_DTS=plat/arm/board/fvp/fdts/fvp_spmc_optee_sp_manifest.dts  \
TF_A_4W_FLAGS="\
	ENABLE_RME=1 \
	RMM=${RMM_OUT_BIN} \
	ENABLE_SVE_FOR_NS=1 \
	ENABLE_SVE_FOR_SWD=1 \
	ARM_DISABLE_TRUSTED_WDOG=1 \
	FVP_HW_CONFIG_DTS=fdts/fvp-base-gicv3-psci-1t.dts \
	ARM_ARCH_MINOR=5 \
	BRANCH_PROTECTION=1 \
	CTX_INCLUDE_PAUTH_REGS=1 \
	CTX_INCLUDE_EL2_REGS=1 \
	CTX_INCLUDE_MTE_REGS=1 \
	LOG_LEVEL=40 \
	SPD=spmd \
	SPMD_SPM_AT_SEL2=1 \
	SP_LAYOUT_FILE=${SC_SP_LAYOUT} \
	BL32=${HAFNIUM_OUT_BIN} \
	ARM_SPMC_MANIFEST_DTS=plat/arm/board/fvp/fdts/fvp_spmc_optee_sp_manifest.dts
	"

TF_A_PRELOADED_KERNEL="\
	ARM_LINUX_KERNEL_AS_BL33=1 \
	PRELOADED_BL33_BASE=0x84000000
	"
# *** End TF-A *** #


if [[ "$(uname -m)" != "aarch64" ]]; then
    export PATH=$THIRD_PARTY_DIR/$CROSS_COMPILE_DIR/bin:${PATH}
else
    export PATH=$THIRD_PARTY_DIR/$CROSS_COMPILE_DIR_AARCH64/bin:${PATH}
fi


		pushd $TF_A_PATH
        make realclean
		BUILD_FLAGS="${TF_A_4W_FLAGS} ${TF_A_PRELOADED_KERNEL}"
		make -j$PARALLELISM \
			PLAT=$TF_A_PLATS \
			ARCH=$TF_A_ARCH \
			DEBUG=1 \
			${BUILD_FLAGS} \
			all fip
		popd