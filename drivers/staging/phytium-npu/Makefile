# SPDX-License-Identifier: GPL-2.0
ccflags-y += -I$(src)/include/

ifdef CONFIG_NPU_PLATFORM
	ifeq ($(CONFIG_NPU_PLATFORM),m)
		EXTRA_CFLAGS += -DPHYTIUM_NPU_PLATFORM
	endif

	ifeq ($(CONFIG_NPU_PLATFORM),y)
		EXTRA_CFLAGS += -DPHYTIUM_NPU_PLATFORM
	endif
endif

obj-$(CONFIG_PHYTIUM_NPU) += phytium_npu.o
phytium_npu-y := phytium_npu_common.o phytium_npu_dev.o \
phytium_npu_dev_mmu.o  phytium_npu_mm_mmu.o phytium_npu_workqueue.o \
phytium_npu_session.o phytium_npu_uapi.o  phytium_npu_debug.o

obj-$(CONFIG_NPU_PLATFORM) += phytium_npu_platform.o
obj-$(CONFIG_NPU_PCI) += phytium_npu_pci.o
