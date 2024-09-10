# SPDX-License-Identifier: GPL-2.0
# Copyright(c) 2022 - 2024 Mucse Corporation
#
# Makefile for the Mucse(R) 1GbE PCI Express ethernet driver
#
#

obj-$(CONFIG_MGBE) += rnpgbe.o
rnpgbe-objs :=   \
		rnpgbe_main.o \
		rnpgbe_common.o \
		rnpgbe_ethtool.o \
		rnpgbe_lib.o \
		rnpgbe_mbx.o \
		rnpgbe_chip.o \
		rnpgbe_mbx_fw.o\
		rnpgbe_sriov.o \
		rnpgbe_param.o \
		rnpgbe_sysfs.o \
		rnpgbe_sfc.o \
		rnpgbe_ptp.o

rnpgbe-$(CONFIG_DEBUG_FS) += rnpgbe_debugfs.o
