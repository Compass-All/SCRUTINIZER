#!/bin/bash

#build dtb
# make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs

#build kernel and moudles
make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- -j$(nproc)

#build kernel image
#make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- -j$(nproc) Image



#build Module.symvers
#make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules_prepare

