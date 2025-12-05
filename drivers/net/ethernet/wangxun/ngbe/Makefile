# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2019 - 2022 Beijing WangXun Technology Co., Ltd.
#
# Makefile for the Wangxun(R) GbE PCI Express ethernet driver
#

obj-$(CONFIG_NGBE) += ngbe.o

ngbe-objs := ngbe_main.o \
	     ngbe_ethtool.o \
	     ngbe_lib.o \
	     ngbe_mbx.o \
	     ngbe_sriov.o \
	     ngbe_param.o \
	     ngbe_phy.o \
	     ngbe_hw.o \
	     ngbe_pcierr.o \
	     ngbe_xsk.o \
	     ngbe_ptp.o \
	     ngbe_sysfs.o

ngbe-${CONFIG_DEBUG_FS} += ngbe_debugfs.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -rf *.${MANSECTION}.gz *.ko
