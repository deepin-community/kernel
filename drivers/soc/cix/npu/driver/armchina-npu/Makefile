# SPDX-License-Identifier: GPL-2.0

subdir-ccflags-y += -I$(src)
subdir-ccflags-y += -I$(src)/include
subdir-ccflags-y += -I$(src)/zhouyi
subdir-ccflags-y += -I$(src)/sky1

# obj-$(CONFIG_ARMCHINA_NPU) += armchina_npu.o
aipu-objs := aipu.o aipu_common.o aipu_io.o aipu_irq.o  \
		aipu_job_manager.o aipu_mm.o aipu_dma_buf.o aipu_priv.o \
		aipu_tcb.o zhouyi/zhouyi.o zhouyi/v3.o zhouyi/v3_priv.o sky1/sky1.o

include $(src)/zhouyi/Makefile
include $(src)/default/Makefile
include $(src)/r329/Makefile
include $(src)/sky1/Makefile
