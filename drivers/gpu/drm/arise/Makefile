CHIP?=E3k
DRIVER_NAME?=arise
TARGET_ARCH?=x86_64
DEBUG?=0
VIDEO_ONLY_FPGA?=0
RUN_HW_NULL?=0
HW_NULL?=0
CONFIG_DRM_ARISE?=m
ifeq ("$(M)", "")
CHECK_GCC_VERSION?=0
else
CHECK_GCC_VERSION?=1
endif

ccflags-y := -D__LINUX__ -DKERNEL_BUILD
ccflags-y += -Wno-undef -Wno-unused -Wno-missing-braces -Wno-overflow -Wno-missing-prototypes -Wno-missing-declarations
ccflags-y += $(call cc-option, -Wno-missing-attributes)

ifeq ($(CHECK_GCC_VERSION), 1)

KERNEL_UNAME?=$(shell uname -r)
KERNEL_MODLIB:=/lib/modules/$(KERNEL_UNAME)
KERNEL_SOURCES:=$(shell test -d $(KERNEL_MODLIB)/source && echo $(KERNEL_MODLIB)/source || echo $(KERNEL_MODLIB)/build)
KERNEL_COMPILE_H=$(KERNEL_SOURCES)/include/generated/compile.h
KERNEL_COMPILE_H_EXIT=$(shell if [ -f $(KERNEL_COMPILE_H) ]; then echo 1; else echo 0; fi)

ifeq ($(KERNEL_COMPILE_H_EXIT), 1)

KERNEL_BUILT_GCC_STRING=$(shell cat ${KERNEL_COMPILE_H} | grep LINUX_COMPILER | cut -f 2 -d '"')
KERNEL_BUILT_GCC_VERSION=$(shell echo "${KERNEL_BUILT_GCC_STRING}" | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' | head -n 1)
ifeq ("$(KERNEL_BUILT_GCC_VERSION)", "")
KERNEL_BUILT_GCC_VERSION=$(shell echo "${KERNEL_BUILT_GCC_STRING}" | grep -o '[0-9]\+\.[0-9]\+' | head -n 1)
endif

SYSTEM_GCC_VERSION=$(shell $(CC) -v 2>&1 | awk 'END{print}' | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' | head -n 1)
ifeq ("$(SYSTEM_GCC_VERSION)", "")
SYSTEM_GCC_VERSION=$(shell $(CC) -v 2>&1 | awk 'END{print}' | grep -o '[0-9]\+\.[0-9]\+' | head -n 1)
endif

ifneq ("$(KERNEL_BUILT_GCC_VERSION)", "$(SYSTEM_GCC_VERSION)")
$(warning "Kernel Built GCC Version ($(KERNEL_BUILT_GCC_VERSION)) Are Differ From System GCC Version($(SYSTEM_GCC_VERSION))!!")
$(warning "System GCC Version Must Match To Kernel Built GCC Version!!")
$(warning "Please Check GCC Version!!")
endif

else
$(warning "$(KERNEL_COMPILE_H) not exist,can not do gcc version check,skip")
endif

endif

ifeq ($(DEBUG), 1)
	ccflags-y += -ggdb3 -O2 -D_DEBUG_ -DGF_TRACE_EVENT=1
else
	ccflags-y += -O2 -fno-strict-aliasing -fno-stack-protector -DGF_TRACE_EVENT=1
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

ifeq ("$(M)", "")
ifeq ("$(O)", "")
GFGPU_FULL_PATH=$(src)
else
GFGPU_FULL_PATH=$(srctree)/$(src)
endif
else
GFGPU_FULL_PATH=$(src)
endif

include $(GFGPU_FULL_PATH)/core/Makefile
include $(GFGPU_FULL_PATH)/cbios/cbios.mk
include $(GFGPU_FULL_PATH)/linux/Makefile
obj-$(CONFIG_DRM_ARISE) := $(DRIVER_NAME).o
