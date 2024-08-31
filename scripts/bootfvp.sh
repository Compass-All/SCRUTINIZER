#!/bin/bash

SCRIPT_DIR="$(dirname $(readlink -f "$0"))"
source $SCRIPT_DIR/config.sh 

#DEBUG = 1: enable DS-5 debug
DEBUG=""
ETEPLUGIN="1"
TRACE=""
TRACE_LOG=""
CYPTOPLUGIN="1"

log() {
  printf "${GREEN}$1${NC}\n"
}    

FS_IMG_PATH=$PROJ_DIR/$ROOTFS
ATF_OUT_PATH="${SRC_DIR}/tf-a/build/fvp/debug"
LINUX_IMAGE=$SRC_DIR/linux/arch/arm64/boot/Image@0x84000000

# command
cmd="$FVP \
	-C bp.refcounter.non_arch_start_at_default=1 -C bp.refcounter.use_real_time=0 -C bp.ve_sysregs.exit_on_shutdown=1 \
	-C cache_state_modelled=0 \
	-C bp.dram_metadata.is_enabled=1 -C bp.dram_size=4 -C bp.secure_memory=0 -C cluster0.has_trbe=1 \
	-C cluster0.has_tme=1 -C cluster0.tme_implementation-type=2 \
	-C cluster0.NUM_CORES=1 -C cluster0.PA_SIZE=48 -C cluster0.ecv_support_level=2 -C cluster0.gicv3.cpuintf-mmap-access-level=2 \
	-C cluster0.gicv3.without-DS-support=1 -C cluster0.gicv4.mask-virtual-interrupt=1 -C cluster0.has_arm_v8-6=1 -C cluster0.has_amu=1 \
	-C cluster0.has_branch_target_exception=1 -C cluster0.rme_support_level=2 -C cluster0.has_rndr=1 -C cluster0.has_v8_7_pmu_extension=2 \
	-C cluster0.max_32bit_el=0 -C cluster0.stage12_tlb_size=1024 -C cluster0.check_memory_attributes=0 \
	-C cluster0.memory_tagging_support_level=2 -C cluster0.restriction_on_speculative_execution=2 -C cluster0.restriction_on_speculative_execution_aarch32=2 \
	-C cluster1.has_tme=1 -C cluster1.tme_implementation-type=2 -C cluster1.has_trbe=1 -C cluster1.NUM_CORES=0 -C cluster1.PA_SIZE=48 -C cluster1.ecv_support_level=2 -C cluster1.gicv3.cpuintf-mmap-access-level=2 -C cluster1.gicv3.without-DS-support=1 -C cluster1.gicv4.mask-virtual-interrupt=1 -C cluster1.has_arm_v8-6=1 -C cluster1.has_amu=1 -C cluster1.has_branch_target_exception=1 -C cluster1.rme_support_level=2 -C cluster1.has_rndr=1 -C cluster1.has_v8_7_pmu_extension=2 -C cluster1.max_32bit_el=0 -C cluster1.stage12_tlb_size=1024 -C cluster1.check_memory_attributes=0 -C cluster1.memory_tagging_support_level=2 -C cluster1.restriction_on_speculative_execution=2 -C cluster1.restriction_on_speculative_execution_aarch32=2 \
	-C pci.pci_smmuv3.mmu.SMMU_AIDR=2 -C pci.pci_smmuv3.mmu.SMMU_IDR0=0x0046123B -C pci.pci_smmuv3.mmu.SMMU_IDR1=0x00600002 -C pci.pci_smmuv3.mmu.SMMU_IDR3=0x1714 -C pci.pci_smmuv3.mmu.SMMU_IDR5=0xFFFF0475 -C pci.pci_smmuv3.mmu.SMMU_S_IDR1=0xA0000002 -C pci.pci_smmuv3.mmu.SMMU_S_IDR2=0 -C pci.pci_smmuv3.mmu.SMMU_S_IDR3=0 -C pci.pci_smmuv3.mmu.SMMU_ROOT_IDR0=3 -C pci.pci_smmuv3.mmu.SMMU_ROOT_IIDR=0x43B -C pci.pci_smmuv3.mmu.root_register_page_offset=0x20000 \
	-C pctl.startup=0.0.0.0 -C bp.smsc_91c111.enabled=1 -C bp.smsc_91c111.mac_address=00:02:F7:C1:9F:81 -C bp.has_rme=1 \
	-C bp.hostbridge.userNetworking=0 -C bp.hostbridge.interfaceName=tap0 -C bp.pl011_uart0.uart_enable=0 -C bp.pl011_uart1.uart_enable=0 \
	-C bp.pl011_uart2.uart_enable=0 -C bp.pl011_uart3.uart_enable=0 \
	-C bp.pl011_uart0.out_file=$LOG_DIR/uart0.log \
	-C bp.pl011_uart1.out_file=$LOG_DIR/uart1.log \
	-C bp.pl011_uart2.out_file=$LOG_DIR/uart2.log \
	-C bp.pl011_uart3.out_file=$LOG_DIR/uart3.log \
	-C bp.pl011_uart0.unbuffered_output=1 -C bp.pl011_uart1.unbuffered_output=1 -C bp.pl011_uart2.unbuffered_output=1 -C bp.pl011_uart3.unbuffered_output=1 \
	-C bp.secureflashloader.fname=$ATF_OUT_PATH/bl1.bin \
	-C bp.flashloader0.fname=$ATF_OUT_PATH/fip.bin \
	--stat \
	-C bp.vis.disable_visualisation=false \
	-C bp.virtioblockdevice.image_path=$FS_IMG_PATH \
	--data cluster0.cpu0=$LINUX_IMAGE" \

	if [[ ! -z "$DEBUG" ]]; then
		cmd="$cmd --cadi-server --print-port-number"
	fi

	if [[ ! -z "$TRACE" ]]; then
		cmd="$cmd --plugin $FVP_PLUGIN_DIR/TarmacTrace.so"
	fi

	if [[ ! -z "$TRACE_LOG" ]]; then
		cmd="$cmd -C TRACE.TarmacTrace.trace-file=$LOG_DIR/trace.log"
	fi

	if [[ ! -z "$ETEPLUGIN" ]]; then
		cmd="$cmd --plugin $FVP_PLUGIN_DIR/libete-plugin.so"
	fi

	if [[ ! -z "$CYPTOPLUGIN" ]]; then
		cmd="$cmd --plugin $FVP_PLUGIN_DIR/Crypto.so"
	fi
	
	ifconfig tap0 > /dev/null 2>&1
    if [ $? != 0 ]; then
        log "Creating network interface tap0..."
        sudo ip tuntap add dev tap0 mode tap user $(whoami)
        sudo ifconfig tap0 0.0.0.0 promisc up
        sudo brctl addif virbr0 tap0
    fi
	echo $cmd
	$cmd
