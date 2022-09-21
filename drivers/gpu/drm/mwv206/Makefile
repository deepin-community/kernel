
ccflags-y := -Iinclude/drm

# CONFIG_SOUND = y,m
# CONFIG_SND_PCM = y,m
mwv206-config-$(CONFIG_SND_PCM) += -DSUPPORT_SND_PCM

SRCDIR=$(abspath $(dir $(word $(words $(MAKEFILE_LIST)), $(MAKEFILE_LIST))))
EXTRA_CFLAGS+=-I$(SRCDIR) -I$(SRCDIR)/include -I$(SRCDIR)/mwv206linuxdev -I$(SRCDIR)/mwv206hal $(mwv206-config-m) $(mwv206-config-y)

# Kernel modules
obj-$(CONFIG_DRM_MWV206) += mwv206.o

mwv206-objs := mwv206hal/gljos_kernel.o \
	mwv206hal/jmgpio.o \
	mwv206hal/jmgraphoutput.o \
	mwv206hal/jmiic.o \
	mwv206hal/jmirq.o \
	mwv206hal/jmpwm.o \
	mwv206hal/jmspi.o \
	mwv206hal/jmspi_flash.o \
	mwv206hal/mwv206ddr_chip.o \
	mwv206hal/mwv206dec.o \
	mwv206hal/mwv206dec_priv.o \
	mwv206hal/mwv206dev.o \
	mwv206hal/mwv206devconfigparser.o \
	mwv206hal/mwv206devconfigreader.o \
	mwv206hal/mwv206ioctl.o \
	mwv206hal/mwv206ioctl_cmd.o \
	mwv206hal/mwv206ioctl_config.o \
	mwv206hal/mwv206ioctl_dev.o \
	mwv206hal/mwv206ioctl_display.o \
	mwv206hal/mwv206ioctl_intr.o \
	mwv206hal/mwv206ioctl_mem.o \
	mwv206hal/mwv206kdma.o \
	mwv206hal/mwv206kg2d.o \
	mwv206hal/mwv206kg3d.o \
	mwv206hal/mwv206memmgr.o \
	mwv206hal/mwv206reg.o \
	mwv206hal/mwv206sdm.o \
	mwv206hal/mwv206ver_kernel.o \
	mwv206hal/oshal_linux_kernel.o \
	mwv206linuxdev/copy.o \
	mwv206linuxdev/fill.o \
	mwv206linuxdev/gljos.o \
	mwv206linuxdev/mwv206_backlight.o \
	mwv206linuxdev/mwv206_edid.o \
	mwv206linuxdev/mwv206_fb_interpret_edid.o \
	mwv206linuxdev/mwv206_fb.o \
	mwv206linuxdev/mwv206_hdmiaudio.o \
	mwv206linuxdev/mwv206_pci.o \
	mwv206linuxdev/mwv206_pm.o \
	mwv206linuxdev/mwv206_proc.o

