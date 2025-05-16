# SPDX-License-Identifier: GPL-2.0-only
ccflags-y += -I$(srctree)/drivers/net/ethernet/huawei/hinic3/
ccflags-y += -I$(srctree)/drivers/net/ethernet/huawei/hinic3/hw/
ccflags-y += -I$(srctree)/drivers/net/ethernet/huawei/hinic3/cqm/
ccflags-y += -I$(srctree)/drivers/net/ethernet/huawei/hinic3/adapter/hw_cmdq/
ccflags-y += -I$(srctree)/drivers/net/ethernet/huawei/hinic3/adapter/sw_cmdq/
ccflags-y += -I$(srctree)/drivers/net/ethernet/huawei/hinic3/include/
ccflags-y += -I$(srctree)/drivers/net/ethernet/huawei/hinic3/include/cqm/
ccflags-y += -I$(srctree)/drivers/net/ethernet/huawei/hinic3/include/public/
ccflags-y += -I$(srctree)/drivers/net/ethernet/huawei/hinic3/include/cfg_mgmt/
ccflags-y += -I$(srctree)/drivers/net/ethernet/huawei/hinic3/include/mpu/
ccflags-y += -I$(srctree)/drivers/net/ethernet/huawei/hinic3/include/bond/
ccflags-y += -I$(srctree)/drivers/net/ethernet/huawei/hinic3/include/vmsec/
ccflags-y += -I$(srctree)/drivers/net/ethernet/huawei/hinic3/bond/

obj-$(CONFIG_HINIC3) += hinic3.o
hinic3-objs := hw/hinic3_hwdev.o \
			hw/hinic3_hw_cfg.o \
			hw/hinic3_hw_comm.o \
			hw/hinic3_prof_adap.o \
			hw/hinic3_sriov.o \
			hw/hinic3_lld.o \
			hw/hinic3_dev_mgmt.o \
			hw/hinic3_common.o \
			hw/hinic3_hwif.o \
			hw/hinic3_wq.o \
			hw/hinic3_cmdq.o \
			hw/hinic3_eqs.o \
			hw/hinic3_mbox.o \
			hw/hinic3_mgmt.o \
			hw/hinic3_api_cmd.o \
			hw/hinic3_hw_api.o \
			hw/hinic3_sml_lt.o \
			hw/hinic3_hw_mt.o \
			hw/hinic3_nictool.o \
			hw/hinic3_devlink.o \
			hw/hinic3_cmdq_enhance.o \
			hw/ossl_knl_linux.o \
			hw/hinic3_multi_host_mgmt.o \
			adapter/hw_cmdq/hw_cmdq_ops.o \
			adapter/sw_cmdq/sw_cmdq_ops.o \
			bond/hinic3_bond.o \
			hinic3_main.o \
			hinic3_cmdq_adapter.o \
			hinic3_tx.o \
			hinic3_rx.o \
			hinic3_rss.o \
			hinic3_ntuple.o \
			hinic3_dcb.o \
			hinic3_ethtool.o \
			hinic3_ethtool_stats.o \
			hinic3_dbg.o \
			hinic3_irq.o \
			hinic3_filter.o \
			hinic3_netdev_ops.o \
			hinic3_nic_prof.o \
			hinic3_nic_cfg.o \
			hinic3_mag_cfg.o \
			hinic3_nic_cfg_vf.o \
			hinic3_rss_cfg.o \
			hinic3_nic_event.o \
			hinic3_nic_io.o \
			hinic3_nic_dbg.o \
			cqm/cqm_bat_cla.o \
			cqm/cqm_bitmap_table.o \
			cqm/cqm_object_intern.o \
			cqm/cqm_bloomfilter.o \
			cqm/cqm_cmd.o \
			cqm/cqm_db.o \
			cqm/cqm_object.o \
			cqm/cqm_main.o \
			cqm/cqm_memsec.o
