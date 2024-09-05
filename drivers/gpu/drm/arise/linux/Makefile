
ifeq ($(CONFIG_ARM64), y)
    ccflags-y += -mstrict-align
ifeq ($(CONFIG_KYLINOS_DESKTOP), y)
    ccflags-y += -DKYLIN
else ifeq ($(CONFIG_KYLINOS_SERVER), y)
    ccflags-y += -DKYLIN
endif
else ifeq ($(CONFIG_MIPS), y)
    ccflags-y += -mips64r2 -D__mips64__ -mlong-calls
endif

ccflags-y += -fms-extensions

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
	gf_sysfs.o \
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
	gf_i2c.o \
	gf_sink.o \
	gf_splice.o \
	e3k/gf_irq_e3k.o

ifeq ($(HW_NULL), 1)
	linux-objs += gf_hw_null.o
else
	linux-objs += gf_pcie.o
endif

$(DRIVER_NAME)-objs += $(addprefix linux/, $(linux-objs))
