BIN_TYPE ?= $(shell uname -m |sed -e s/i.86/i386/)
KERNEL_VERSION := $(shell uname -r)
LINUXDIR ?= /lib/modules/$(KERNEL_VERSION)/build
DRM_VER=$(shell sed -n 's/^RHEL_DRM_VERSION = \(.*\)/\1/p' $(LINUXDIR)/Makefile)
YHQILINOS=$(shell sed -n 's/^export KBUILD_BUILD_USER=//p' $(LINUXDIR)/Makefile)
DRM_SUBVERSION=$(shell sed -n 's/^EXTRAVERSION = \(-\)//p' $(LINUXDIR)/Makefile)
YKLINOS_VERSION=$(shell sed -n 's/^KYLIN_BUILD_VERSION =//p' $(LINUXDIR)/Makefile)

ifneq (,$(DRM_VER))
DRM_PATCH=$(shell sed -n 's/^RHEL_DRM_PATCHLEVEL = \(.*\)/\1/p' $(LINUXDIR)/Makefile)
DRM_SUBLEVEL=$(shell sed -n 's/^RHEL_DRM_SUBLEVEL = \(.*\)/\1/p' $(LINUXDIR)/Makefile)
DRM_CODE=$(shell expr $(DRM_VER) \* 65536 + 0$(DRM_PATCH) \* 256 + 0$(DRM_SUBLEVEL))
ccflags-y += -DDRM_VERSION_CODE=$(DRM_CODE)
endif

ifneq ($(YKLINOS_VERSION), )
  ccflags-y += -DKYLIN
endif

ifeq ($(BIN_TYPE)_$(YHQILINOS), aarch64_YHKYLIN-OS)
    DISTRIB_ID=$(shell sed -n 's/^DISTRIB_ID=//p' /etc/lsb-release)
    DISTRIB_RELEASE=$(shell sed -n 's/^DISTRIB_RELEASE=//p' /etc/lsb-release)
    KYLIN_RELEASE=$(shell sed -n 's/^DISTRIB_KYLIN_RELEASE=//p' /etc/lsb-release)
    ifeq ($(DISTRIB_RELEASE), V10)
        DISTRIB_ID_RELEASE=$(DISTRIB_ID)_$(DISTRIB_RELEASE)
    else
        DISTRIB_ID_RELEASE=$(DISTRIB_ID)_$(KYLIN_RELEASE)
    endif
    ifneq (,$(filter $(DISTRIB_ID_RELEASE)_$(BIN_TYPE), Kylin_V10_aarch64 Kylin_4.0.2-SP4_aarch64))
         ccflags-y += -DPHYTIUM_2000
    else
         DRM_VER=4
         DRM_PATCH=9
         DRM_SUBLEVEL=0
         DRM_CODE=$(shell expr $(DRM_VER) \* 65536 + 0$(DRM_PATCH) \* 256 + 0$(DRM_SUBLEVEL))
         ccflags-y += -DDRM_VERSION_CODE=$(DRM_CODE) -DYHQILIN
    endif
endif

ifneq ($(BIN_TYPE)_$(YHQILINOS), aarch64_YHKYLIN-OS)
    ifeq ($(YHQILINOS), YHKYLIN-OS)
        ccflags-y += -DYHQILIN
        KERNEL_VERSION_NUM := $(shell echo $(KERNEL_VERSION) | cut -d - -f1)
        ifeq (,$(DRM_VER))
            ifeq ($(KERNEL_VERSION_NUM), 4.4.131)
                DRM_VER=4
                DRM_PATCH=9
                DRM_SUBLEVEL=0
                DRM_CODE=$(shell expr $(DRM_VER) \* 65536 + 0$(DRM_PATCH) \* 256 + 0$(DRM_SUBLEVEL))
                ccflags-y += -DDRM_VERSION_CODE=$(DRM_CODE) -DDRM_SUBVERSION_CODE=$(DRM_SUBVERSION)
            endif
        endif
    endif
endif

ifeq ($(BIN_TYPE), mips64)
	ccflags-y += -mips64r2 -D__mips64__ -mlong-calls
endif

ifeq ($(BIN_TYPE), aarch64)
	ccflags-y += -mstrict-align
endif

ccflgs-y += -fms-extensions


ccflags-y += \
	-I$(GFGPU_FULL_PATH)/shared \
	-I$(GFGPU_FULL_PATH)/core/include \
	-I$(GFGPU_FULL_PATH)/cbios \
	-I$(GFGPU_FULL_PATH)/linux


linux-objs:= \
	os_interface.o  \
	gf.o \
	gf_driver.o \
	../shared/os_shared.o \
	gf_debugfs.o \
	gf_ioctl.o \
	gf_gem.o \
	gf_fence.o \
	gf_trace_events.o \
	gf_atomic.o \
	gf_connector.o \
	gf_crtc.o \
	gf_disp.o \
	gf_drmfb.o \
	gf_encoder.o \
	gf_irq.o \
	gf_plane.o \
	gf_fbdev.o \
	gf_cbios.o \
	gf_params.o \
	gf_capture_drv.o \
	gf_vip.o \
	gf_wb.o \
	e3k/gf_irq_e3k.o

#linux-objs += $(vip-objs)

ifeq ($(HW_NULL), 1)
	linux-objs += gf_hw_null.o
else
	linux-objs += gf_pcie.o
endif

$(PRO_DRIVER_NAME)-objs += $(addprefix linux/, $(linux-objs))

