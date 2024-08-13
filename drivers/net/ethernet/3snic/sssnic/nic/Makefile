# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2023 3SNIC
#

SYS_TIME=$(shell date +%Y-%m-%d_%H:%M:%S)
ccflags-y += -D __TIME_STR__=\"$(SYS_TIME)\"

ccflags-y += -I$(srctree)/drivers/net/ethernet/3snic/sssnic/include
ccflags-y += -I$(srctree)/drivers/net/ethernet/3snic/sssnic/include/hw
ccflags-y += -I$(srctree)/drivers/net/ethernet/3snic/sssnic/include/kernel
ccflags-y += -I$(srctree)/drivers/net/ethernet/3snic/sssnic/nic
ccflags-y += -I$(srctree)/drivers/net/ethernet/3snic/sssnic/nic/tool
ccflags-y += -I$(srctree)/drivers/net/ethernet/3snic/sssnic/nic/include
ccflags-y += -I$(srctree)/drivers/net/ethernet/3snic/sssnic/hw
ccflags-y += -I$(srctree)/drivers/net/ethernet/3snic/sssnic/hw/tool
ccflags-y += -I$(srctree)/drivers/net/ethernet/3snic/sssnic/include
ccflags-y += -I$(srctree)/drivers/net/ethernet/3snic/sssnic/include/hw
ccflags-y += -I$(srctree)/drivers/net/ethernet/3snic/sssnic/include/kernel
ccflags-y += -I$(srctree)/drivers/net/ethernet/3snic/sssnic/hw/include

ccflags-y += -Werror

obj-$(CONFIG_SSSNIC) += sssnic.o
sssnic-y := sss_nic_main.o \
			sss_nic_tx.o \
			sss_nic_tx_init.o \
			sss_nic_rx.o \
			sss_nic_rx_init.o \
			sss_nic_rx_reset.o \
			sss_nic_rss.o \
			sss_nic_ntuple.o \
			sss_nic_dcb.o \
			sss_nic_ethtool.o \
			sss_nic_ethtool_api.o \
			sss_nic_ethtool_stats.o \
			sss_nic_ethtool_stats_api.o \
			sss_nic_irq.o \
			sss_nic_filter.o \
			sss_nic_netdev_ops.o \
			sss_nic_cfg.o \
			sss_nic_mag_cfg.o \
			sss_nic_vf_cfg.o \
			sss_nic_rss_cfg.o \
			sss_nic_event.o \
			sss_nic_io.o \
			sss_nic_netdev_ops_api.o \
			./tool/sss_tool_nic_func.o \
			./tool/sss_tool_nic_dcb.o \
			./tool/sss_tool_nic_phy_attr.o \
			./tool/sss_tool_nic_qp_info.o \
			./tool/sss_tool_nic_stats.o \
			../hw/sss_hw_main.o \
			../hw/sss_pci.o \
			../hw/sss_pci_probe.o \
			../hw/sss_pci_remove.o \
			../hw/sss_pci_shutdown.o \
			../hw/sss_pci_error.o \
			../hw/sss_pci_sriov.o \
			../hw/sss_pci_global.o \
			../hw/sss_hwdev_api.o \
			../hw/sss_hwdev_cap.o \
			../hw/sss_hwdev_export.o \
			../hw/sss_hwdev_link.o \
			../hw/sss_hwdev_init.o \
			../hw/sss_hwdev_mgmt_info.o \
			../hw/sss_hwdev_mgmt_channel.o \
			../hw/sss_hwdev_io_flush.o \
			../hw/sss_hwif_ctrlq.o \
			../hw/sss_hwif_ctrlq_init.o \
			../hw/sss_hwif_ctrlq_export.o \
			../hw/sss_hwif_mbx.o \
			../hw/sss_hwif_mbx_init.o \
			../hw/sss_hwif_mbx_export.o \
			../hw/sss_hwif_adm.o \
			../hw/sss_hwif_adm_init.o \
			../hw/sss_hwif_init.o \
			../hw/sss_hwif_api.o \
			../hw/sss_hwif_export.o \
			../hw/sss_hwif_eq.o \
			../hw/sss_hwif_mgmt_init.o \
			../hw/sss_hwif_irq.o \
			../hw/sss_hwif_aeq.o \
			../hw/sss_common.o \
			../hw/sss_wq.o \
			../hw/sss_hwif_ceq.o \
			../hw/sss_adapter_mgmt.o \
			../hw/tool/sss_tool_main.o \
			../hw/tool/sss_tool_chip.o \
			../hw/tool/sss_tool_sdk.o \
			../hw/tool/sss_tool_sm.o
