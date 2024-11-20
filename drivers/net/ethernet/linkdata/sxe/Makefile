# SPDX-License-Identifier: GPL-2.0
CONFIG_MODULE_SIG=n

#当前目录下的Makefile路径
MAKEPATH := $(abspath $(lastword $(MAKEFILE_LIST)))
#当前路径
CURDIR :=$(shell dirname $(MAKEPATH))

ifneq ($(KERNELRELEASE),)
#编译
CONFIG_SXE ?= m
obj-$(CONFIG_SXE) += sxe.o
sxe-objs += $(patsubst %.c, sxepf/%.o, $(notdir $(wildcard $(CURDIR)/sxepf/*.c)))
sxe-objs += $(patsubst %.c, base/trace/%.o, $(notdir $(wildcard $(CURDIR)/base/trace/*.c)))
sxe-objs += $(patsubst %.c, base/log/%.o, $(notdir $(wildcard $(CURDIR)/base/log/*.c)))

#添加编译选项和编译宏
ccflags-y += -Werror -frecord-gcc-switches
ccflags-y += $(call cc-option, -Wmaybe-uninitialized)
ccflags-y += -I$(CURDIR)/sxepf
ccflags-y += -I$(CURDIR)/include/sxe
ccflags-y += -I$(CURDIR)/include
ccflags-y += -I$(CURDIR)/base/compat
ccflags-y += -I$(CURDIR)/base/trace
ccflags-y += -I$(CURDIR)/base/log
ccflags-y += -DSXE_HOST_DRIVER
ccflags-y += -DSXE_DRIVER_RELEASE
ccflags-$(CONFIG_DCB) += -DSXE_DCB_CONFIGURE
ifneq ($(CONFIG_DCA), )
    ccflags-y += -DSXE_TPH_CONFIGURE
endif

# 生成 linux kernel version code
ifneq ($(wildcard $(CURDIR)/vercode_build.sh),)
  KER_DIR=$(srctree)
  SPECIFIC_LINUX=$(shell bash $(CURDIR)/vercode_build.sh $(KER_DIR))
  ifneq ($(SPECIFIC_LINUX),)
    ccflags-y += -DSPECIFIC_LINUX
    ccflags-y += -D$(SPECIFIC_LINUX)
  endif
endif

else # KERNELRELEASE
#内核树路径
KDIR := /lib/modules/$(shell uname -r)/build

all:
	@$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	@rm -rf *.o *.d *.ko Module.* modules.* *.mod* .*.d .*.cmd .tmp_versions *readme.txt
	@rm -rf ./sxepf/*.o ./sxepf/.*.cmd
	@rm -rf ./base/log/*.o ./base/trace/*.o

endif # KERNELRELEASE
