# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux sw_64-specific parts of the memory manager.
#

#ccflags-y := -Werror

obj-y	:= init.o fault.o  physaddr.o mmap.o extable.o

obj-$(CONFIG_NUMA)	+= numa.o
ifeq ($(CONFIG_SUBARCH_C4),y)
obj-$(CONFIG_HUGETLB_PAGE)	+= hugetlbpage_c4.o
else
obj-$(CONFIG_HUGETLB_PAGE)      += hugetlbpage.o
endif
obj-$(CONFIG_TRANSPARENT_HUGEPAGE)      += thp.o
