phytium-dc-drm-y := phytium_display_drv.o \
		    phytium_plane.o \
		    phytium_crtc.o \
		    phytium_dp.o \
		    phytium_fb.o \
		    phytium_gem.o \
		    phytium_fbdev.o \
		    phytium_debugfs.o \
		    x100_dp.o \
		    phytium_panel.o \

obj-$(CONFIG_DRM_PHYTIUM) += phytium-dc-drm.o
CFLAGS_REMOVE_phytium_crtc.o += -mgeneral-regs-only
