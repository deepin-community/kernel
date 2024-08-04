#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 1999 - 2024 Intel Corporation

set -Eeuo pipefail

# This file generates HAVE_ and NEED_ defines for current kernel
# (or KSRC if provided).
#
# It does so by 'gen' function calls (see body of 'gen-devlink' for examples).
# 'gen' could look for various kinds of declarations in provided kernel headers,
# eg look for an enum in one of files specified and check if given enumeration
# (single value) is present. See 'Documentation' or comment above the 'gen' fun
# in the kcompat-lib.sh.

# Why using bash/awk instead of an old/legacy approach?
#
# The aim is to replicate all the defines provided by human developers
# in the past. Additional bonus is the fact, that we no longer need to care
# about backports done by OS vendors (RHEL, SLES, ORACLE, UBUNTU, more to come).
# We will even work (compile) with only part of backports provided.
#
# To enable smooth transition, especially in time of late fixes, "old" method
# of providing flags should still work as usual.

# End of intro.
# Find info about coding style/rules at the end of file.
# Most of the implementation is in kcompat-lib.sh, here are actual 'gen' calls.

export LC_ALL=C
SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"
ORIG_CWD="$(pwd)"
trap 'rc=$?; echo >&2 "$(realpath "$ORIG_CWD/${BASH_SOURCE[0]}"):$LINENO: failed with rc: $rc"' ERR

# shellcheck source=kcompat-lib.sh
source "$SCRIPT_DIR"/kcompat-lib.sh

ARCH=$(uname -m)
IS_ARM=
if [ "$ARCH" == aarch64 ]; then
	IS_ARM=1
fi

# DO NOT break gen calls below (via \), to make our compat code more grep-able,
# keep them also grouped, first by feature (like DEVLINK), then by .h filename
# finally, keep them sorted within a group (sort by flag name)

# handy line of DOC copy-pasted form kcompat-lib.sh:
#   gen DEFINE if (KIND [METHOD of]) NAME [(matches|lacks) PATTERN|absent] in <list-of-files>

function gen-aux() {
	ah='include/linux/auxiliary_bus.h'
	mh='include/linux/mod_devicetable.h'
	if config_has CONFIG_AUXILIARY_BUS; then
		gen HAVE_AUXILIARY_DRIVER_INT_REMOVE if method remove of auxiliary_driver matches 'int' in "$ah"
	fi

	# generate HAVE_AUXILIARY_DEVICE_ID only for cases when it's disabled in .config
	if ! config_has CONFIG_AUXILIARY_BUS; then
		gen HAVE_AUXILIARY_DEVICE_ID if struct auxiliary_device_id in "$mh"
	fi
}

function gen-bitfield() {
	bf='include/linux/bitfield.h'
	gen HAVE_INCLUDE_BITFIELD if macro FIELD_PREP in "$bf"
	gen NEED_BITFIELD_FIELD_FIT if macro FIELD_FIT absent in "$bf"
	gen NEED_BITFIELD_FIELD_MASK if fun field_mask absent in "$bf"
	gen NEED_BITFIELD_FIELD_MAX if macro FIELD_MAX absent in "$bf"
}

function gen-device() {
	dh='include/linux/device.h'
	dph='include/linux/dev_printk.h'
	gen NEED_BUS_FIND_DEVICE_CONST_DATA if fun bus_find_device lacks 'const void \\*data' in "$dh"
	gen NEED_DEV_LEVEL_ONCE if macro dev_level_once absent in "$dh" "$dph"
	gen NEED_DEVM_KASPRINTF if fun devm_kasprintf absent in "$dh"
	gen NEED_DEVM_KFREE if fun devm_kfree absent in "$dh"
	gen NEED_DEVM_KVASPRINTF if fun devm_kvasprintf absent in "$dh"
	gen NEED_DEVM_KZALLOC if fun devm_kzalloc absent in "$dh"
}

function gen-devlink() {
	dh='include/net/devlink.h'
	gen HAVE_DEVLINK_FLASH_UPDATE_BEGIN_END_NOTIFY if fun devlink_flash_update_begin_notify in "$dh"
	gen HAVE_DEVLINK_FLASH_UPDATE_PARAMS    if struct devlink_flash_update_params in "$dh"
	gen HAVE_DEVLINK_FLASH_UPDATE_PARAMS_FW if struct devlink_flash_update_params matches 'struct firmware \\*fw' in "$dh"
	gen HAVE_DEVLINK_HEALTH if enum devlink_health_reporter_state in "$dh"
	gen HAVE_DEVLINK_HEALTH_OPS_EXTACK if method dump of devlink_health_reporter_ops matches extack in "$dh"
	gen HAVE_DEVLINK_INFO_DRIVER_NAME_PUT if fun devlink_info_driver_name_put in "$dh"
	gen HAVE_DEVLINK_PARAMS if method validate of devlink_param matches extack in "$dh"
	gen HAVE_DEVLINK_PARAMS_PUBLISH if fun devlink_params_publish in "$dh"
	gen HAVE_DEVLINK_PORT_NEW if method port_new of devlink_ops in "$dh"
	gen HAVE_DEVLINK_PORT_OPS if struct devlink_port_ops in "$dh"
	gen HAVE_DEVLINK_PORT_SPLIT if method port_split of devlink_ops in "$dh"
	gen HAVE_DEVLINK_PORT_SPLIT if method port_split of devlink_port_ops in "$dh"
	gen HAVE_DEVLINK_PORT_SPLIT_EXTACK if method port_split of devlink_ops matches extack in "$dh"
	gen HAVE_DEVLINK_PORT_SPLIT_EXTACK if method port_split of devlink_port_ops matches extack in "$dh"
	gen HAVE_DEVLINK_PORT_SPLIT_IN_OPS if method port_split of devlink_ops in "$dh"
	gen HAVE_DEVLINK_PORT_SPLIT_IN_PORT_OPS if method port_split of devlink_port_ops in "$dh"
	gen HAVE_DEVLINK_PORT_SPLIT_PORT_STRUCT if method port_split of devlink_ops matches devlink_port in "$dh"
	gen HAVE_DEVLINK_PORT_SPLIT_PORT_STRUCT if method port_split of devlink_port_ops matches devlink_port in "$dh"
	gen HAVE_DEVLINK_PORT_TYPE_ETH_HAS_NETDEV if fun devlink_port_type_eth_set matches 'struct net_device' in "$dh"
	gen HAVE_DEVLINK_RATE_NODE_CREATE if fun devl_rate_node_create in "$dh"
	# keep devlink_region_ops body in variable, to not look 4 times for
	# exactly the same thing in big file
	# please consider it as an example of "how to speed up if needed"
	REGION_OPS="$(find-struct-decl devlink_region_ops "$dh")"
	gen HAVE_DEVLINK_REGIONS if struct devlink_region_ops in - <<< "$REGION_OPS"
	gen HAVE_DEVLINK_REGION_OPS_SNAPSHOT if fun snapshot in - <<< "$REGION_OPS"
	gen HAVE_DEVLINK_REGION_OPS_SNAPSHOT_OPS if fun snapshot matches devlink_region_ops in - <<< "$REGION_OPS"
	gen HAVE_DEVLINK_REGISTER_SETS_DEV if fun devlink_register matches 'struct device' in "$dh"
	gen HAVE_DEVLINK_RELOAD_ENABLE_DISABLE if fun devlink_reload_enable in "$dh"
	gen HAVE_DEVLINK_SET_FEATURES  if fun devlink_set_features in "$dh"
	gen HAVE_DEVL_HEALTH_REPORTER_DESTROY if fun devl_health_reporter_destroy in "$dh"
	gen HAVE_DEVL_PORT_REGISTER if fun devl_port_register in "$dh"
	gen NEED_DEVLINK_HEALTH_DEFAULT_AUTO_RECOVER if fun devlink_health_reporter_create matches auto_recover in "$dh"
	gen NEED_DEVLINK_RESOURCES_UNREGISTER_NO_RESOURCE if fun devlink_resources_unregister matches 'struct devlink_resource \\*' in "$dh"
	gen NEED_DEVLINK_TO_DEV  if fun devlink_to_dev absent in "$dh"
	gen NEED_DEVLINK_UNLOCKED_RESOURCE if fun devl_resource_size_get absent in "$dh"

	gen HAVE_DEVLINK_PORT_FLAVOUR_PCI_SF if enum devlink_port_flavour matches DEVLINK_PORT_FLAVOUR_PCI_SF in include/uapi/linux/devlink.h
	gen HAVE_DEVLINK_RELOAD_ACTION_AND_LIMIT if enum devlink_reload_action matches DEVLINK_RELOAD_ACTION_FW_ACTIVATE in include/uapi/linux/devlink.h
}

function gen-dma() {
	dma='include/linux/dma-mapping.h'
	gen NEED_DMA_ATTRS_PTR if struct dma_attrs in include/linux/dma-attrs.h
	gen NEED_DMA_ATTRS if fun dma_map_page_attrs absent in "$dma"
}

function gen-dpll() {
	dh='include/linux/dpll.h'
	gen HAVE_DPLL_LOCK_STATUS_ERROR if method lock_status_get of dpll_device_ops matches status_error in "$dh"
	gen HAVE_DPLL_PHASE_OFFSET if method phase_offset_get of dpll_pin_ops in "$dh"
	gen NEED_DPLL_NETDEV_PIN_SET if fun dpll_netdev_pin_set absent in "$dh"
}

function gen-ethtool() {
	eth='include/linux/ethtool.h'
	ueth='include/uapi/linux/ethtool.h'
	gen HAVE_ETHTOOL_COALESCE_EXTACK if method get_coalesce of ethtool_ops matches 'struct kernel_ethtool_coalesce \\*' in "$eth"
	gen HAVE_ETHTOOL_EXTENDED_RINGPARAMS if method get_ringparam of ethtool_ops matches 'struct kernel_ethtool_ringparam \\*' in "$eth"
	gen HAVE_ETHTOOL_KEEE if struct ethtool_keee in "$eth"
	gen HAVE_ETHTOOL_RXFH_PARAM if struct ethtool_rxfh_param in "$eth"
	gen NEED_ETHTOOL_SPRINTF if fun ethtool_sprintf absent in "$eth"
	gen HAVE_ETHTOOL_FLOW_RSS if macro FLOW_RSS in "$ueth"
}

function gen-filter() {
	fh='include/linux/filter.h'
	gen NEED_NO_NETDEV_PROG_XDP_WARN_ACTION if fun bpf_warn_invalid_xdp_action lacks 'struct net_device \\*' in "$fh"
	gen NEED_XDP_DO_FLUSH if fun xdp_do_flush absent in "$fh"
}

function gen-flow-dissector() {
	gen HAVE_FLOW_DISSECTOR_KEY_PPPOE if enum flow_dissector_key_id matches FLOW_DISSECTOR_KEY_PPPOE in include/net/flow_dissector.h include/net/flow_keys.h
	# following HAVE ... CVLAN flag is mistakenly named after an enum key,
	# but guards code around function call that was introduced later
	gen HAVE_FLOW_DISSECTOR_KEY_CVLAN if fun flow_rule_match_cvlan in include/net/flow_offload.h
}

function gen-gnss() {
	cdh='include/linux/cdev.h'
	clh='include/linux/device/class.h'
	dh='include/linux/device.h'
	gh='include/linux/gnss.h'
	th='include/uapi/linux/types.h'
	fh='include/linux/fs.h'

	gen HAVE_CDEV_DEVICE if fun cdev_device_add in "$cdh"
	gen HAVE_DEV_UEVENT_CONST if method dev_uevent of class matches '(const|RH_KABI_CONST) struct device' in "$clh" "$dh"
	gen HAVE_STREAM_OPEN if fun stream_open in "$fh"
	# There can be either macro class_create or a function
	gen NEED_CLASS_CREATE_WITH_MODULE_PARAM if fun class_create matches 'owner' in "$clh" "$dh"
	gen NEED_CLASS_CREATE_WITH_MODULE_PARAM if macro class_create in "$clh" "$dh"

	if ! config_has CONFIG_SUSE_KERNEL; then
		gen HAVE_GNSS_MODULE if struct gnss_device in "$gh"
	fi

	gen HAVE_POLL_T if typedef __poll_t in "$th"
}

function gen-mdev() {
	mdevh='include/linux/mdev.h'

	gen HAVE_DEV_IN_MDEV_API if method probe of mdev_driver matches 'struct device \\*' in "$mdevh"
	gen HAVE_KOBJ_IN_MDEV_PARENT_OPS_CREATE if method create of mdev_parent_ops matches 'struct kobject \\*' in "$mdevh"
}

function gen-netdevice() {
	ndh='include/linux/netdevice.h'
	gen HAVE_NDO_ETH_IOCTL if fun ndo_eth_ioctl in "$ndh"
	gen HAVE_NDO_EXTENDED_SET_TX_MAXRATE if method ndo_set_tx_maxrate of net_device_ops_extended in "$ndh"
	gen HAVE_NDO_FDB_ADD_VID    if method ndo_fdb_del of net_device_ops matches 'u16 vid' in "$ndh"
	gen HAVE_NDO_FDB_DEL_EXTACK if method ndo_fdb_del of net_device_ops matches extack in "$ndh"
	gen HAVE_NDO_GET_DEVLINK_PORT if method ndo_get_devlink_port of net_device_ops in "$ndh"
	gen HAVE_NDO_UDP_TUNNEL_CALLBACK if method ndo_udp_tunnel_add of net_device_ops in "$ndh"
	gen HAVE_NETDEV_EXTENDED_MIN_MAX_MTU if struct net_device_extended matches min_mtu in "$ndh"
	gen HAVE_NETDEV_MIN_MAX_MTU if struct net_device matches min_mtu in "$ndh"
	gen HAVE_NETIF_SET_TSO_MAX if fun netif_set_tso_max_size in "$ndh"
	gen HAVE_SET_NETDEV_DEVLINK_PORT if macro SET_NETDEV_DEVLINK_PORT in "$ndh"
	gen NEED_NETIF_NAPI_ADD_NO_WEIGHT if fun netif_napi_add matches 'int weight' in "$ndh"
	gen NEED_NET_PREFETCH if fun net_prefetch absent in "$ndh"
	gen NEED_XDP_FEATURES if enum netdev_xdp_act absent in include/uapi/linux/netdev.h
}

function gen-pci() {
	pcih='include/linux/pci.h'
	gen HAVE_PCI_MSIX_ALLOC_IRQ_AT if fun pci_msix_alloc_irq_at in "$pcih"
	gen HAVE_PCI_MSIX_CAN_ALLOC_DYN if fun pci_msix_can_alloc_dyn in "$pcih"
	gen HAVE_PCI_MSIX_FREE_IRQ if fun pci_msix_free_irq in "$pcih"
	gen HAVE_PER_VF_MSIX_SYSFS if method sriov_set_msix_vec_count of pci_driver in "$pcih"
	gen HAVE_STRUCT_PCI_DEV_PTM_ENABLED if struct pci_dev matches ptm_enabled in "$pcih"
	gen NEED_PCIE_FLR if fun pcie_flr absent in "$pcih"
	gen NEED_PCIE_FLR_RETVAL if fun pcie_flr lacks 'int pcie_flr' in "$pcih"
	gen NEED_PCIE_PTM_ENABLED if fun pcie_ptm_enabled absent in "$pcih"
	gen NEED_PCI_ENABLE_PTM if fun pci_enable_ptm absent in "$pcih"
}

function gen-stddef() {
	stddef='include/linux/stddef.h'
	ustddef='include/uapi/linux/stddef.h'
	gen HAVE_STDDEF_OFFSETTOEND if macro offsetofend in "$stddef"
	gen NEED_DECLARE_FLEX_ARRAY if macro DECLARE_FLEX_ARRAY absent in "$stddef"
	gen NEED_STRUCT_GROUP if macro struct_group absent in "$stddef"
	gen NEED___STRUCT_GROUP if macro __struct_group absent in "$ustddef"
}

function gen-vfio() {
	# PASID_SUPPORT depends on multiple different functions existing
	PASID_FUNC1="$(find-fun-decl mdev_set_iommu_device include/linux/mdev.h)"
	PASID_FUNC2="$(find-fun-decl vfio_group_iommu_domain include/linux/vfio.h)"

	gen HAVE_PASID_SUPPORT if string "${PASID_FUNC1:+1}${PASID_FUNC2:+1}" equals 11
	gen HAVE_VFIO_FREE_DEV if fun vfio_free_device in include/linux/vfio.h
	gen HAVE_LMV1_SUPPORT if macro VFIO_REGION_TYPE_MIGRATION in include/uapi/linux/vfio.h
}

function gen-other() {
	pciaerh='include/linux/aer.h'
	ush='include/linux/u64_stats_sync.h'
	gen HAVE_X86_STEPPING if struct cpuinfo_x86 matches x86_stepping in arch/x86/include/asm/processor.h
	gen HAVE_PCI_ENABLE_PCIE_ERROR_REPORTING if fun pci_enable_pcie_error_reporting in "$pciaerh"
	gen NEED_PCI_AER_CLEAR_NONFATAL_STATUS if fun pci_aer_clear_nonfatal_status absent in "$pciaerh"
	gen NEED_BITMAP_COPY_CLEAR_TAIL if fun bitmap_copy_clear_tail absent in include/linux/bitmap.h
	gen NEED_BITMAP_FROM_ARR32 if fun bitmap_from_arr32 absent in include/linux/bitmap.h
	gen NEED_BITMAP_TO_ARR32 if fun bitmap_to_arr32 absent in include/linux/bitmap.h
	gen NEED_ASSIGN_BIT if fun assign_bit absent in include/linux/bitops.h
	gen NEED_STATIC_ASSERT if macro static_assert absent in include/linux/build_bug.h
	gen NEED_CLEANUP_API if macro __free absent in include/linux/cleanup.h
	gen NEED___STRUCT_SIZE if macro __struct_size absent in include/linux/compiler_types.h include/linux/fortify-string.h
	gen HAVE_COMPLETION_RAW_SPINLOCK if struct completion matches 'struct swait_queue_head' in include/linux/completion.h
	gen NEED_IS_CONSTEXPR if macro __is_constexpr absent in include/linux/const.h include/linux/minmax.h include/linux/kernel.h
	gen NEED_DEBUGFS_LOOKUP if fun debugfs_lookup absent in include/linux/debugfs.h
	gen NEED_DEBUGFS_LOOKUP_AND_REMOVE if fun debugfs_lookup_and_remove absent in include/linux/debugfs.h
	gen NEED_ETH_HW_ADDR_SET if fun eth_hw_addr_set absent in include/linux/etherdevice.h
	gen NEED_FIND_NEXT_BIT_WRAP if fun find_next_bit_wrap absent in include/linux/find.h
	gen HAVE_FILE_IN_SEQ_FILE if struct seq_file matches 'struct file' in include/linux/fs.h
	gen NEED_FS_FILE_DENTRY if fun file_dentry absent in include/linux/fs.h
	gen HAVE_HWMON_DEVICE_REGISTER_WITH_INFO if fun hwmon_device_register_with_info in include/linux/hwmon.h
	gen NEED_HWMON_CHANNEL_INFO if macro HWMON_CHANNEL_INFO absent in include/linux/hwmon.h
	gen NEED_ETH_TYPE_VLAN if fun eth_type_vlan absent in include/linux/if_vlan.h
	gen HAVE_IOMMU_DEV_FEAT_AUX if enum iommu_dev_features matches IOMMU_DEV_FEAT_AUX in include/linux/iommu.h
	gen NEED_READ_POLL_TIMEOUT if macro read_poll_timeout absent in include/linux/iopoll.h
	gen NEED_DEFINE_STATIC_KEY_FALSE if macro DEFINE_STATIC_KEY_FALSE absent in include/linux/jump_label.h
	gen NEED_STATIC_BRANCH_LIKELY if macro static_branch_likely absent in include/linux/jump_label.h
	gen HAVE_STRUCT_STATIC_KEY_FALSE if struct static_key_false in include/linux/jump_label.h include/linux/jump_label_type.h
	gen NEED_DECLARE_STATIC_KEY_FALSE if macro DECLARE_STATIC_KEY_FALSE absent in include/linux/jump_label.h include/linux/jump_label_type.h
	gen NEED_LOWER_16_BITS if macro lower_16_bits absent in include/linux/kernel.h
	gen NEED_UPPER_16_BITS if macro upper_16_bits absent in include/linux/kernel.h
	gen NEED_LIST_COUNT_NODES if fun list_count_nodes absent in include/linux/list.h

	# On aarch64 RHEL systems, mul_u64_u64_div_u64 appears to be declared
	# in math64 header, but is not provided by kernel
	# so on these systems, set it to need anyway.
	if [ "$IS_ARM" ]; then
		NEED_MUL_STR=1
	else
		MUL_U64_U64_DIV_U64_FUNC="$(find-fun-decl mul_u64_u64_div_u64 include/linux/math64.h)"
		NEED_MUL_STR="${MUL_U64_U64_DIV_U64_FUNC:-1}"
	fi
	gen NEED_MUL_U64_U64_DIV_U64 if string "${NEED_MUL_STR}" equals 1

	gen HAVE_MDEV_GET_DRVDATA if fun mdev_get_drvdata in include/linux/mdev.h
	gen HAVE_MDEV_REGISTER_PARENT if fun mdev_register_parent in include/linux/mdev.h
	gen HAVE_VM_FLAGS_API if fun vm_flags_init in include/linux/mm.h
	gen HAVE_NL_SET_ERR_MSG_FMT if macro NL_SET_ERR_MSG_FMT in include/linux/netlink.h
	gen NEED_DEV_PM_DOMAIN_ATTACH if fun dev_pm_domain_attach absent in include/linux/pm_domain.h include/linux/pm.h
	gen NEED_DEV_PM_DOMAIN_DETACH if fun dev_pm_domain_detach absent in include/linux/pm_domain.h include/linux/pm.h
	gen NEED_PTP_CLASSIFY_RAW if fun ptp_classify_raw absent in include/linux/ptp_classify.h
	gen NEED_PTP_PARSE_HEADER if fun ptp_parse_header absent in include/linux/ptp_classify.h
	gen HAVE_PTP_CLOCK_INFO_ADJFINE if method adjfine of ptp_clock_info in include/linux/ptp_clock_kernel.h
	gen NEED_DIFF_BY_SCALED_PPM if fun diff_by_scaled_ppm absent in include/linux/ptp_clock_kernel.h
	gen NEED_PTP_SYSTEM_TIMESTAMP if fun ptp_read_system_prets absent in include/linux/ptp_clock_kernel.h
	gen NEED_RADIX_TREE_EMPTY if fun radix_tree_empty absent in include/linux/radix-tree.h
	gen NEED_SCHED_PARAM if struct sched_param absent in include/linux/sched.h
	gen NEED_SET_SCHED_FIFO if fun sched_set_fifo absent in include/linux/sched.h
	gen NEED_RT_H if macro MAX_RT_PRIO absent in include/linux/sched/prio.h
	gen NEED_DEV_PAGE_IS_REUSABLE if fun dev_page_is_reusable absent in include/linux/skbuff.h
	gen NEED_NAPI_BUILD_SKB if fun napi_build_skb absent in include/linux/skbuff.h
	gen NEED_KREALLOC_ARRAY if fun krealloc_array absent in include/linux/slab.h
	gen NEED_SYSFS_MATCH_STRING if macro sysfs_match_string absent in include/linux/string.h
	gen NEED_SYSFS_EMIT if fun sysfs_emit absent in include/linux/sysfs.h
	gen HAVE_TRACE_ENABLED_SUPPORT if implementation of macro __DECLARE_TRACE matches 'trace_##name##_enabled' in include/linux/tracepoint.h
	gen HAVE_TTY_OP_WRITE_SIZE_T if method write of tty_operations matches size_t in include/linux/tty_driver.h
	gen HAVE_U64_STATS_FETCH_BEGIN_IRQ if fun u64_stats_fetch_begin_irq in "$ush"
	gen HAVE_U64_STATS_FETCH_RETRY_IRQ if fun u64_stats_fetch_retry_irq in "$ush"
	gen NEED_U64_STATS_READ if fun u64_stats_read absent in "$ush"
	gen NEED_U64_STATS_SET if fun u64_stats_set absent in "$ush"
	gen HAVE_NET_RPS_H if macro RPS_NO_FILTER in include/net/rps.h
}

# all the generations, extracted from main() to keep normal code and various
# prep separated
function gen-all() {
	if config_has CONFIG_NET_DEVLINK; then
		gen-devlink
	fi
	gen-netdevice
	# code above is covered by unit_tests/test_gold.sh
	if [ -n "${JUST_UNIT_TESTING-}" ]; then
		return
	fi
	gen-aux
	gen-bitfield
	gen-device
	gen-dma
	gen-dpll
	gen-ethtool
	gen-filter
	gen-flow-dissector
	gen-gnss
	gen-mdev
	gen-pci
	gen-stddef
	gen-vfio
	gen-other
}

function main() {
	if ! [ -d "${KSRC-}" ]; then
		echo >&2 "env KSRC=${KSRC-} does not exist or is not a directory"
		exit 11
	fi

	# we need some flags from .config or (autoconf.h), try to find it
	if [ -z ${CONFIG_FILE-} ]; then
		find_config_file

		if [ -z ${CONFIG_FILE-} ]; then
			echo >&2 "unable to locate a config file at KSRC=${KSRC}. please set CONFIG_FILE to the kernel configuration file."
			exit 10
		fi
	fi

	if [ ! -f "${CONFIG_FILE-}" ]; then
		echo >&2 ".config passed in by env CONFIG_FILE=${CONFIG_FILE} does not exist or is not a file"
		exit 9
	fi
	CONFIG_FILE=$(realpath "${CONFIG_FILE-}")

	# check if caller (like our makefile) wants to redirect output to file
	if [ -n "${OUT-}" ]; then

		# in case OUT exists, we don't want to overwrite it, instead
		# write to a temporary copy.
		if [ -s "${OUT}" ]; then
			TMP_OUT="$(mktemp "${OUT}.XXX")"
			trap "rm -f '${TMP_OUT}'" EXIT

			REAL_OUT="${OUT}"
			OUT="${TMP_OUT}"
		fi

		exec > "$OUT"
		# all stdout goes to OUT since now
		echo "/* Autogenerated for KSRC=${KSRC-} via $(basename "$0") */"
	fi

	cd "${KSRC}"

	# check if KSRC was ok/if we are in proper place to look for headers
	if [ -z "$(filter-out-bad-files include/linux/kernel.h)" ]; then
		echo >&2 "seems that there are no kernel includes placed in KSRC=${KSRC}
			pwd=$(pwd); ls -l:"
		ls -l >&2
		exit 8
	fi

	if [ -z ${UNIFDEF_MODE-} ]; then
		echo "#ifndef _KCOMPAT_GENERATED_DEFS_H_"
		echo "#define _KCOMPAT_GENERATED_DEFS_H_"
	fi

	gen-all

	if [ -z ${UNIFDEF_MODE-} ]; then
		echo "#endif /* _KCOMPAT_GENERATED_DEFS_H_ */"
	fi

	if [ -n "${OUT-}" ]; then
		cd "$ORIG_CWD"

		# Compare and see if anything changed. This avoids updating
		# mtime of the file.
		if [ -n "${REAL_OUT-}" ]; then
			if cmp --silent "${REAL_OUT}" "${TMP_OUT}"; then
				# exit now, skipping print of the output since
				# there were no changes. the trap should
				# cleanup TMP_OUT
				exit 0
			fi

			mv -f "${TMP_OUT}" "${REAL_OUT}"
			OUT="${REAL_OUT}"
		fi

		# dump output, will be visible in CI
		if [ -n "${JUST_UNIT_TESTING-}${QUIET_COMPAT-}" ]; then
			return
		fi
		cat -n "$OUT" >&2
	fi
}

main

# Coding style:
# - rely on `set -e` handling as much as possible, so:
#  - do not use <(bash process substitution) - it breaks error handling;
#  - do not put substantial logic in `if`-like statement - it disables error
#    handling inside of the conditional (`if big-fun call; then` is substantial)
# - make shellcheck happy - https://www.shellcheck.net
#
# That enables us to move processing out of `if` or `... && ...` statements,
# what finally means that bash error handling (`set -e`) would break on errors.
