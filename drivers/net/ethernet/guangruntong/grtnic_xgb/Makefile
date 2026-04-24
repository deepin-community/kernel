# SPDX-License-Identifier: GPL-2.0
# Copyright(c) 1999 - 2026 Beijing GuangRunTong(GRT) Corporation.
#
# Makefile for the GRT(R) 1-10GbE PCI Express ethernet driver
#
DRV_NAME      := grtnic_xgb

obj-$(CONFIG_GRTNIC_XGB) += $(DRV_NAME).o

$(DRV_NAME)-objs := grtnic_main.o grtnic_netdev.o grtnic_macphy.o grtnic_param.o grtnic_nvm.o \
                    grtnic_ethtool.o grtnic_proc.o kcompat.o