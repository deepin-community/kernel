# SPDX-License-Identifier: GPL-2.0
obj-y	+= haoc.o haoc-bitmap.o
obj-y	+= iee/
obj-$(CONFIG_CREDP) += credp/
obj-$(CONFIG_PTP) += ptp/
obj-$(CONFIG_KEYP) += keyp/
obj-$(CONFIG_VARP) += varp/

ccflags-$(CONFIG_IEE_SELINUX_P) += -I$(srctree)/security/selinux -I$(srctree)/security/selinux/include
obj-$(CONFIG_IEE_SELINUX_P) += selinuxp/
