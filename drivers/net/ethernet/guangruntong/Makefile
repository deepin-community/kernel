# SPDX-License-Identifier: GPL-2.0
# Copyright(c) 1999 - 2021 Intel Corporation.

ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
#
# Makefile for the Intel(R) 10GbE PCI Express Linux Network Driver
#

obj-$(CONFIG_GRTNIC_XGB) += grtnic_xgb.o

define grtnic_xgb-y
	grtnic_main.o
	grtnic_netdev.o
	grtnic_macphy.o
	grtnic_param.o
	grtnic_nvm.o
	grtnic_ethtool.o
	grtnic_proc.o
endef
grtnic_xgb-y := $(strip ${grtnic_xgb-y})
grtnic_xgb-y += kcompat.o

else	# ifneq($(KERNELRELEASE),)
# normal makefile

DRIVER := grtnic_xgb

ifeq (,$(wildcard common.mk))
  $(error Cannot find common.mk build rules)
else
  include common.mk
endif

# Check that kernel version is at least 2.6.0, since we don't support 2.4.x
# kernels with the grtnic driver. We can't use minimum_kver_check since SLES 10
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

# Build manfiles
manfile:
	@gzip -c ../${DRIVER}.${MANSECTION} > ${DRIVER}.${MANSECTION}.gz

# Clean the module subdirectories
clean:
	@+$(call devkernelbuild,clean)
	@-rm -rf *.${MANSECTION}.gz *.ko

kylin: EXTRA_CFLAGS += -DKYLIN_KERNEL
kylin:
	@echo "Compile for Kylin kernel..."
	@+$(call devkernelbuild,modules)

kylin44: EXTRA_CFLAGS += -DKYLIN_KERNEL44
kylin44:
	@echo "Compile for Kylin4.4 kernel..."
	@+$(call devkernelbuild,modules)

uos: EXTRA_CFLAGS += -DUOS_KERNEL
uos:
	@echo "Compile for Uos kernel..."
	@+$(call devkernelbuild,modules)

euler: EXTRA_CFLAGS += -DEULER_KERNEL
euler:
	@echo "Compile for OpenEuler kernel..."
	@+$(call devkernelbuild,modules)


# Install the modules and manpage
mandocs_install: manfile
	@echo "Copying manpages..."
	@install -D -m 644 ${DRIVER}.${MANSECTION}.gz ${INSTALL_MOD_PATH}${MANDIR}/man${MANSECTION}/${DRIVER}.${MANSECTION}.gz

# Install kernel module files. This target is called by the RPM specfile
# when generating binary RPMs, and is not expected to modify files outside
# of the build root. Thus, it must not update initramfs, or run depmod.
modules_install: default
	@echo "Installing modules..."
	@+$(call devkernelbuild,modules_install)

kylin_modules_install: kylin
	@echo "Installing modules..."
	@+$(call devkernelbuild,modules_install)

kylin44_modules_install: kylin44
	@echo "Installing modules..."
	@+$(call devkernelbuild,modules_install)

uos_modules_install: uos
	@echo "Installing modules..."
	@+$(call devkernelbuild,modules_install)

euler_modules_install: euler
	@echo "Installing modules..."
	@+$(call devkernelbuild,modules_install)

# After installing all the files, perform necessary work to ensure the
# system will use the new modules. This includes running depmod to update
# module dependencies and updating the initramfs image in case the module is
# loaded during early boot.
install: modules_install #mandocs_install
	@echo "Running depmod..."
	$(call cmd_depmod)
	$(call cmd_initramfs)

kylin_install: kylin_modules_install #mandocs_install
	@echo "Running depmod..."
	$(call cmd_depmod)
	$(call cmd_initramfs)

kylin44_install: kylin44_modules_install #mandocs_install
	@echo "Running depmod..."
	$(call cmd_depmod)
	$(call cmd_initramfs)

uos_install: uos_modules_install #mandocs_install
	@echo "Running depmod..."
	$(call cmd_depmod)
	$(call cmd_initramfs)

euler_install: euler_modules_install #mandocs_install
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
uninstall: modules_uninstall
	$(call cmd_depmod)
	$(call cmd_initramfs)

########
# Help #
########
help:
	@echo 'Build targets:'
	@echo '  default             - Build module(s) with standard verbosity'
	@echo '  kylin           		 - Build module(s) for kylin Kernel'
	@echo '  kylin44         		 - Build module(s) for kylin 4.4 Kernel'
	@echo '  uos           		 	 - Build module(s) for uos Kernel'
	@echo '  euler           		 - Build module(s) for euler Kernel'
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
	@echo '  manfile             - Generate a gzipped manpage'
	@echo '  modules_install     - Install the module(s) only'
	@echo '  mandocs_install     - Install the manpage only'
	@echo '  install             - Build then install the module(s) and manpage, and update initramfs'
	@echo '  kylin_install   	   - Build then install kylin module(s) and update initramfs'
	@echo '  kylin44_install 	   - Build then install kylin 4.4 module(s) and update initramfs'
	@echo '  uos_install   	     - Build then install uos module(s) and update initramfs'
	@echo '  euler_install   	   - Build then install OpenEuler module(s) and update initramfs'
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
	@echo '  INSTALL_MOD_DIR     - Use module directory other than updates/drivers/net/ethernet/intel/${DRIVER}'
	@echo ' Other variables may be available for tuning make process, see'
	@echo ' Kernel Kbuild documentation for more information'

.PHONY: default noisy clean manfile silent sparse ccc install uninstall help

endif	# ifneq($(KERNELRELEASE),)
