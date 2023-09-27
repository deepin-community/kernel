
FULL_LS_PATH=$(src)/..

FULL_BRIDGE_PATH = $(FULL_LS_PATH)/bridge

ccflags-y := -I$(FULL_LS_PATH)/include \
	-I$(FULL_BRIDGE_PATH)/ \
	-I$(FULL_LS_PATH)/include \

CFLAGS_gsgpu_fence.o += -O
gsgpu-y := gsgpu_drv.o

# add KMS driver
gsgpu-y += gsgpu_device.o gsgpu_kms.o \
	gsgpu_fence.o gsgpu_ttm.o gsgpu_object.o gsgpu_gart.o \
	gsgpu_display.o gsgpu_cp.o \
	gsgpu_fb.o gsgpu_gem.o gsgpu_ring.o \
	gsgpu_cs.o gsgpu_benchmark.o gsgpu_test.o \
	gsgpu_trace_points.o \
	gsgpu_sa.o gsgpu_zip_meta.o \
	gsgpu_prime.o gsgpu_vm.o gsgpu_vm_it.o gsgpu_ib.o \
	gsgpu_bo_list.o gsgpu_ctx.o gsgpu_sync.o \
	gsgpu_gtt_mgr.o gsgpu_vram_mgr.o \
	gsgpu_queue_mgr.o gsgpu_sched.o gsgpu_debugfs.o \
	gsgpu_ids.o gsgpu_hw_sema.o gsgpu_pm.o

gsgpu-y += gsgpu_common.o

# add MMU block
gsgpu-y +=  gsgpu_mmu.o

# add ZIP block
gsgpu-y += gsgpu_zip.o

# add IH block
gsgpu-y += \
	gsgpu_irq.o \
	gsgpu_ih.o

# add GFX block
gsgpu-y += gsgpu_gfx.o

# add XDMA block
gsgpu-y += gsgpu_xdma.o

# GPU scheduler
gsgpu-y += gsgpu_job.o

gsgpu-$(CONFIG_COMPAT) += gsgpu_ioc32.o
gsgpu-$(CONFIG_MMU_NOTIFIER) += gsgpu_hmm.o

# add DC block
gsgpu-y += gsgpu_dc_drv.o gsgpu_dc_crtc.o gsgpu_dc_hdmi.o \
	  gsgpu_dc_irq.o gsgpu_dc_plane.o gsgpu_dc_connector.o \
	  gsgpu_dc_encoder.o gsgpu_dc_i2c.o  gsgpu_dc_cursor.o \
	  gsgpu_dc_vbios.o gsgpu_backlight.o

RELATIVE_GSGPU_BRIDGE_PATH = ../bridge
include $(FULL_BRIDGE_PATH)/Makefile
gsgpu-y += $(GSGPU_BRIDGE_FILES)

obj-$(CONFIG_DRM_GSGPU)+= gsgpu.o
