ccflags-y += -I$(src)
ccflags-y += -I$(src)/smmu/kernel
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/table/include
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/comm/include
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/driver/include
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/sdk/include/dev/module/table/se
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/sdk/include/dev/module/se
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/sdk/include/api
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/sdk/include/dev/chip
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/sdk/include/dev/reg
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/sdk/include/dev/module/dma
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/sdk/include/dev/module/nppu
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/sdk/include/dev/module/ppu
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/sdk/include/dev/module/table
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/sdk/include/dev/module/tm
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/sdk/include/diag
subdir-ccflags-y += -I$(src)/../../../net/ethernet/dinghai/en_np/agent/include

obj-$(CONFIG_INFINIBAND_ZRDMA) += zrdma.o

zrdma-y := \
		main.o \
		manager.o \
		hw.o \
		cm.o \
		ctrl.o \
		hmc.o \
		pble.o \
		puda.o \
		uk.o \
		utils.o \
		verbs.o \
		uda.o \
		trace.o \
		icrdma_hw.o \
		vf.o \
		virtchnl.o \
		zxdh_ext_mem_config.o \
		zrdma_kcompat.o \
		tc_hmcdma.o \
		srq.o \
		restrack.o \
		private_verbs_cmd.o \
		dbgfs.o \
		smmu/kernel/adk_mmu600.o \
		smmu/kernel/cmdk_mmu600.o \
		smmu/kernel/cmdk_pagetable.o \
		slib.o
