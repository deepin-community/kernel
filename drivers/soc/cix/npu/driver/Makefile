# SPDX-License-Identifier: GPL-2.0
#
# Makefile for Cix Technology NPU (Zhouyi) Driver
#

obj-$(CONFIG_ARMCHINA_NPU) += cix-aipu.o

cix-aipu-y := armchina-npu/aipu.o armchina-npu/aipu_common.o armchina-npu/aipu_io.o armchina-npu/aipu_irq.o  \
                armchina-npu/aipu_job_manager.o armchina-npu/aipu_mm.o armchina-npu/aipu_dma_buf.o armchina-npu/aipu_priv.o \
                armchina-npu/aipu_tcb.o armchina-npu/zhouyi/zhouyi.o armchina-npu/zhouyi/v3.o armchina-npu/zhouyi/v3_priv.o armchina-npu/sky1/sky1.o

ccflags-y += -I$(src)/armchina-npu/include \
             -I$(src)/armchina-npu/zhouyi \
             -I$(src)/armchina-npu/sky1 \
             -I$(src) \
             -I$(src)/armchina-npu

