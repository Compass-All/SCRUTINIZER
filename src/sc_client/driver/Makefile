PWD         := $(shell pwd) 
KVERSION    := $(shell uname -r)
CURRENT_DIR := $(CURDIR)
PROJ_DIR := $(abspath $(CURRENT_DIR)/../..)
KDIR   = $(PROJ_DIR)/linux
COMPILE_PATH := $(abspath $(CURRENT_DIR)/../../..)


MODULE_NAME  = sc_manager
obj-m       := $(MODULE_NAME).o
$(MODULE_NAME)-objs := sc_driver.o

ifeq ($(shell uname -m),aarch64)
    CROSS_COMPILE := $(COMPILE_PATH)/dev_workspace/gcc-arm-10.3-2021.07-aarch64-aarch64-none-elf/bin/aarch64-none-elf-
else
    CROSS_COMPILE := $(COMPILE_PATH)/dev_workspace/gcc-arm-10.3-2021.07-x86_64-aarch64-none-elf/bin/aarch64-none-elf-
endif

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=arm64 CROSS_COMPILE=$(CROSS_COMPILE) modules
	rm -rf *.mod* .*.cmd *.symvers *.order
clean:
	make -C $(KDIR) M=$(PWD) clean