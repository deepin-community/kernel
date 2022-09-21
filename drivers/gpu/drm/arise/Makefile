CHIP?=E3k
DRIVER_NAME?=arise
PRO_DRIVER_NAME=$(DRIVER_NAME)
TARGET_ARCH?=x86_64
DEBUG?=0
VIDEO_ONLY_FPGA?=0
RUN_HW_NULL?=0
HW_NULL?=0
CONFIG-GFGPU=m

ccflags-y := -D__LINUX__ -DKERNEL_BUILD
ccflags-y += -Wno-undef -Wno-unused -Wno-missing-braces -Wno-missing-attributes -Wno-overflow
ifeq ($(DEBUG), 1)
	ccflags-y += -ggdb3 -O2 -D_DEBUG_ -DGF_TRACE_EVENT=1
else
	ccflags-y += -O2 -fno-strict-aliasing -DGF_TRACE_EVENT=1
endif

ifeq ($(VIDEO_ONLY_FPGA), 1)
ccflags-y += -DVIDEO_ONLY_FPGA
endif

ifeq ($(RUN_HW_NULL), 1)
ccflags-y += -DGF_HW_NULL
else
ccflags-y += -DGF_PCIE_BUS
endif

ccflags-y += -I$(src)

GFGPU_FULL_PATH=$(src)
include $(GFGPU_FULL_PATH)/core/Makefile
include $(GFGPU_FULL_PATH)/cbios/cbios.mk
include $(GFGPU_FULL_PATH)/linux/Makefile
obj-$(CONFIG-GFGPU) := $(PRO_DRIVER_NAME).o
