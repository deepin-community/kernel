# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2015 - 2025 Beijing WangXun Technology Co., Ltd.
#
# Makefile for the Wangxun(R) 10GbE PCI Express ethernet driver
#

obj-$(CONFIG_TXGBE) += txgbe.o

txgbe-objs := txgbe_main.o \
              txgbe_hw.o \
              txgbe_phy.o \
              txgbe_ethtool.o \
			  txgbe_bp.o \
			  txgbe_debugfs.o \
			  txgbe_mbx.o \
			  txgbe_mtd.o \
			  txgbe_e56.o \
			  txgbe_param.o \
			  txgbe_ptp.o \
			  txgbe_sriov.o \
			  txgbe_sysfs.o \
			  txgbe_xsk.o \
			  txgbe_lib.o \
			  txgbe_aml.o \
			  txgbe_aml40.o \
			  txgbe_pcierr.o \
			  txgbe_e56_bp.o

txgbe-$(CONFIG_DCB) += txgbe_dcb_nl.o txgbe_dcb.o
txgbe-$(CONFIG_FCOE:m=y) += txgbe_fcoe.o
txgbe-$(CONFIG_SYSFS) += txgbe_sysfs.o
txgbe-$(CONFIG_DEBUG_FS) += txgbe_debugfs.o
