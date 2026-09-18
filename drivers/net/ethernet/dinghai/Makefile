subdir-ccflags-y += -I$(src)
subdir-ccflags-y += -I$(srctree)/include/linux/dinghai/
ccflags-y += -Werror


# 追加额外的编译选项（如 CCFLAGS_ADD）
ifneq ($(CCFLAGS_ADD),)
ccflags-y += $(CCFLAGS_ADD)
endif

ifeq ($(CONFIG_ZXDH_MSGQ),m)
EXTRA_CFLAGS += -DZXDH_MSGQ
endif

ifeq ($(CONFIG_DINGHAI_SEC),m)
EXTRA_CFLAGS += -DZXDH_SEC
endif

ifeq ($(CONFIG_DINGHAI_PTP),m)
EXTRA_CFLAGS += -DPTP_DRIVER_INTERFACE_EN
endif

ifeq ($(CONFIG_DRIVER_VERSION),)
EXTRA_CFLAGS += -DDRIVER_VERSION_VAL=\"1.0-1\"
$(info CONFIG_PF_MPF_DRIVER_VERSION is null, EXTRA_CFLAGS=$(EXTRA_CFLAGS))
else
EXTRA_CFLAGS += -DDRIVER_VERSION_VAL=\"$(CONFIG_DRIVER_VERSION)\"
$(info CONFIG_PF_MPF_DRIVER_VERSION is not null, EXTRA_CFLAGS=$(EXTRA_CFLAGS))
endif


obj-$(CONFIG_DINGHAI_DH_CMD) += dinghai10e_cmd.o
dinghai10e_cmd-y := dh_cmd.o  cmd/msg_main.o cmd/msg_chan_lock.o cmd/msg_chan_test.o log.o

obj-$(CONFIG_DINGHAI_PF) += dinghai10e.o

dinghai10e-y := events.o en_pf.o eq.o pci_irq.o en_pf/en_pf_irq.o en_pf/en_pf_eq.o devlink.o en_pf/en_pf_devlink.o en_pf/en_pf_events.o \
    en_pf/msg_func.o dh_procfs.o lag/lag.o lag/lag_procfs.o plcr.o sriov_sysfs.o xarray.o health.o log.o slib.o zxdh_tools/zxdh_tools_mmap_chrdev.o \
    pcie_common.o kcompat.o irq_affinity.o

dinghai10e-y += en_sf.o en_sf/en_sf_eq.o en_sf/en_sf_irq.o en_sf/en_sf_devlink.o

dinghai10e-y += en_aux.o en_aux/queue.o en_aux/en_aux_cmd.o en_aux/en_aux_eq.o \
                 en_aux/en_aux_events.o en_ethtool/ethtool.o en_aux/en_aux_ioctl.o \
                 en_aux/dcbnl/en_dcbnl.o en_aux/dcbnl/en_dcbnl_api.o \
                 zxdh_tools/zxdh_tools_ioctl.o zxdh_tools/zxdh_tools_netlink.o zxdh_tools/zxdh_tools_table.o zxdh_tools/zxdh_tools_csig_tag.o\
                 en_aux/en_1588_pkt_proc.o en_aux/en_1588_pkt_proc_func.o en_aux/selq.o en_tc/en_tc.o en_tc/en_fs.o
dinghai10e-y += en_aux/drs_sec_dtb.o
dinghai10e-y += bonding/zxdh_lag.o bonding/rdma_ops.o bonding/dual_tor.o

obj-$(CONFIG_DINGHAI_PTP)  += dinghai10e_ptp.o
dinghai10e_ptp-y :=en_ptp/tod_driver.o en_ptp/tod_driver_stub.o en_ptp/zxdh_ptp.o log.o slib.o
obj-$(CONFIG_DINGHAI_TSN)  += dinghai10e_tsn.o
dinghai10e_tsn-y :=en_tsn/zxdh_tsn.o en_tsn/zxdh_tsn_reg.o en_tsn/zxdh_tsn_ioctl.o log.o

include $(src)/en_np/Makefile

