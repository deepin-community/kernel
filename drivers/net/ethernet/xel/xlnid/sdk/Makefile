# SPDX-License-Identifier: GPL-2.0-only
# Copyright(c) 2008 - 2024 Xel Technology.
#$(warning XLNID_Makefile:KERNELRELEASE=$(KERNELRELEASE),KBUILD_SRC=$(KBUILD_SRC),SRC= $(srctree)/$(src))
ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
#
# Makefile for the Xel-Tech(R) PCI Express Linux Network Driver
#

obj-$(CONFIG_XLNID) += xlnid.o
SOURCE = $(srctree)/$(src)
ccflags-y += -I$(SOURCE)/../include
#$(warning XLNID_Makefile:KBUILD_SRC=$(KBUILD_SRC),SOURCE= $(SOURCE))

define xlnid-y
	xlnid_main.o
	xlnid_lib.o
	xlnid_xsk.o
	xlnid_ethtool.o
	../common/xlnid_macsec.o
	../common/xlnid_westlake.o
	../common/xlnid_api.o
	../common/xlnid_common.o
	../common/xlnid_sriov.o
	../common/xlnid_param.o
	../common/xlnid_procfs.o
	../common/kcompat.o
endef
xlnid-y := $(strip ${xlnid-y})

xlnid-${CONFIG_DEBUG_FS} += ../common/xlnid_debugfs.o

xlnid-$(CONFIG_PTP_1588_CLOCK:m=y) += ../common/xlnid_ptp.o

xlnid-${CONFIG_SYSFS} += ../common/xlnid_sysfs.o

xlnid-y += ../common/kcompat.o

else	# ifneq($(KERNELRELEASE),)
# normal makefile

DRIVER := xlnid
CFLAGS_EXTRA := -I${CURDIR}/../include/

ifeq ($(XLNID_GEPHY_SUPPORT), y)
CFLAGS_EXTRA += -DXLNID_GEPHY_SUPPORT
else ifeq ($(XLNID_SGMII_MAC_SUPPORT), y)
CFLAGS_EXTRA += -DXLNID_SGMII_MAC_SUPPORT
else ifeq ($(XLNID_SGMII_PHY_SUPPORT), y)
CFLAGS_EXTRA += -DXLNID_SGMII_PHY_SUPPORT
else ifeq ($(XLNID_1000BASEX_SUPPORT), y)
CFLAGS_EXTRA += -DXLNID_1000BASEX_SUPPORT
else ifeq ($(XLNID_FORCE_1000BASEX_SUPPORT), y)
CFLAGS_EXTRA += -DXLNID_FORCE_1000BASEX_SUPPORT
else
CFLAGS_EXTRA += -DXLNID_AUTO_MEDIA_SUPPORT
endif

# associated with XLNID-191, default close.
#CFLAGS_EXTRA += -DXLNID_GEPHY_OPTIMIZE

# copper use cross-over
#CFLAGS_EXTRA += -DXLNID_COPPER_CROSSOVER_SUPPORT

ifeq ($(XLNID_SPI_FLASH), y)
CFLAGS_EXTRA += -DXLNID_SPI_FLASH
    ifneq ($(XLNID_SPI_CONFIG),)
    CFLAGS_EXTRA += -DXLNID_SPI_CONFIG=$(XLNID_SPI_CONFIG)
    $(info XLNID_SPI_CONFIG = $(XLNID_SPI_CONFIG), <memory[bit0-2], Earse subsector cycle time msec[bit4-15]>)
    endif
endif


ifeq (,$(wildcard common.mk))
  $(error Cannot find common.mk build rules)
else
  include common.mk
endif

# Check that kernel version is at least 2.6.0, since we don't support 2.4.x
# kernels with the xlnid driver. We can't use minimum_kver_check since SLES 10
# SP4's Make has a bug which causes $(eval) inside an ifeq conditional to error
# out. This was fixed in Make 3.81, but SLES 10 SP4 does not have a fix for
# this yet.
ifeq (0,$(shell [ ${KVER_CODE} -lt $(call get_kvercode,2,6,0) ]; echo "$?"))
  $(warning *** Aborting the build.)
  $(error This driver is not supported on kernel versions older than 2.6.0)
endif

######################
# Kernel Build Macro #
######################

# customized kernelbuild function
#
# ${1} is the kernel build target
# ${2} may contain extra rules to pass to kernelbuild macro
#
# We customize the kernelbuild target in order to provide our hack to disable
# CONFIG_PTP_1588_CLOCK support should -DNO_PTP_SUPPORT be defined in the extra
# cflags given on the command line.
devkernelbuild = $(call kernelbuild,$(if $(filter -DNO_PTP_SUPPORT,${EXTRA_CFLAGS}),CONFIG_PTP_1588_CLOCK=n) ${2},${1})

# Command to update initramfs or display a warning message
ifeq (${cmd_initrd},)
define cmd_initramfs
@echo "Unable to update initramfs. You may need to do this manually."
endef
else
define cmd_initramfs
@echo "Updating initramfs..."
-@$(call cmd_initrd)
endef
endif

###############
# Build rules #
###############

# Standard compilation, with regular output
default:
	@+$(call devkernelbuild,modules)

# Noisy output, for extra debugging
noisy:
	@+$(call devkernelbuild,modules,V=1)

# Silence any output generated
silent:
	@+$(call devkernelbuild,modules,>/dev/null)

# Enable higher warning level
checkwarnings: clean
	@+$(call devkernelbuild,modules,W=1)

# Run sparse static analyzer
sparse: clean
	@+$(call devkernelbuild,modules,C=2 CF="-D__CHECK_ENDIAN__ -Wbitwise -Wcontext")

# Run coccicheck static analyzer
ccc: clean
	@+$(call devkernelbuild,modules,coccicheck MODE=report))

# Clean the module subdirectories
clean:
	@+$(call devkernelbuild,clean)
	@-rm -rf ../common/*.o ../common/.*.o.cmd
	@-rm -rf *.${MANSECTION}.gz *.ko

# Install kernel module files. This target is called by the RPM specfile
# when generating binary RPMs, and is not expected to modify files outside
# of the build root. Thus, it must not update initramfs, or run depmod.
modules_install: default
	@echo "Installing modules..."
	@+$(call devkernelbuild,modules_install)

# After installing all the files, perform necessary work to ensure the
# system will use the new modules. This includes running depmod to update
# module dependencies and updating the initramfs image in case the module is
# loaded during early boot.
install: modules_install
	@echo "Running depmod..."
	$(call cmd_depmod)
	$(call cmd_initramfs)

mandocs_uninstall:
	if [ -e ${INSTALL_MOD_PATH}${MANDIR}/man${MANSECTION}/${DRIVER}.${MANSECTION}.gz ] ; then \
		rm -f ${INSTALL_MOD_PATH}${MANDIR}/man${MANSECTION}/${DRIVER}.${MANSECTION}.gz ; \
	fi;

# Remove installed module files. This target is called by the RPM specfile
# when generating binary RPMs, and is not expected to modify files outside
# of the build root. Thus, it must not update the initramfs image or run
# depmod.
modules_uninstall:
	rm -f ${INSTALL_MOD_PATH}/lib/modules/${KVER}/${INSTALL_MOD_DIR}/${DRIVER}.ko;

# After uninstalling all the files, perform necessary work to restore the
# system back to using the default kernel modules. This includes running
# depmod to update module dependencies and updating the initramfs image.
uninstall: modules_uninstall mandocs_uninstall
	$(call cmd_depmod)
	$(call cmd_initramfs)

########
# Help #
########
help:
	@echo 'Build targets:'
	@echo '  default             - Build module(s) with standard verbosity'
	@echo '  noisy               - Build module(s) with V=1 verbosity -- very noisy'
	@echo '  silent              - Build module(s), squelching all output'
	@echo ''
	@echo 'Static Analysis:'
	@echo '  checkwarnings       - Clean, then build module(s) with W=1 warnings enabled'
	@echo '  sparse              - Clean, then check module(s) using sparse'
	@echo '  ccc                 - Clean, then check module(s) using coccicheck'
	@echo ''
	@echo 'Cleaning targets:'
	@echo '  clean               - Clean files generated by kernel module build'
	@echo ''
	@echo 'Other targets:'
	@echo '  modules_install     - Install the module(s) only'
	@echo '  mandocs_install     - Install the manpage only'
	@echo '  install             - Build then install the module(s) and manpage, and update initramfs'
	@echo '  modules_uninstall   - Uninstall the module(s) only'
	@echo '  mandocs_uninstall   - Uninstall the manpage only'
	@echo '  uninstall           - Uninstall the module(s) and manpage, and update initramfs'
	@echo '  help                - Display this help message'
	@echo ''
	@echo 'Variables:'
	@echo '  LINUX_VERSION       - Debug tool to force kernel LINUX_VERSION_CODE. Use at your own risk.'
	@echo '  W=N                 - Kernel variable for setting warning levels'
	@echo '  V=N                 - Kernel variable for setting output verbosity'
	@echo '  INSTALL_MOD_PATH    - Add prefix for the module and manpage installation path'
	@echo '  INSTALL_MOD_DIR     - Use module directory other than updates/drivers/net/ethernet/xel/${DRIVER}'
	@echo ' Other variables may be available for tuning make process, see'
	@echo ' Kernel Kbuild documentation for more information'

.PHONY: default noisy clean manfile silent sparse ccc install uninstall help

endif	# ifneq($(KERNELRELEASE),)
