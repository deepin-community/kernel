# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_SCSI_PS3STOR) += ps3stor.o

ps3stor-objs := \
                ps3_cmd_channel.o \
                ps3_device_update.o \
                ps3_ioc_manager.o \
                ps3_mgr_channel.o \
                ps3_platform_utils.o \
                ps3_scsi_cmd_err.o \
                ps3_cmd_complete.o \
                ps3_ioc_state.o \
                ps3_mgr_cmd.o \
                ps3_qos.o \
                ps3_scsih.o \
                ps3_cmd_statistics.o \
                ps3_event.o \
                ps3_ioctl.o \
                ps3_mgr_cmd_err.o \
                ps3_r1x_write_lock.o \
                ps3_scsih_cmd_parse.o \
                ps3_debug.o \
                ps3_io_trace.o \
                ps3_module_para.o \
                ps3_rb_tree.o \
                ps3_scsih_raid_engine.o \
                ps3_device_manager.o \
                ps3_instance_manager.o \
                ps3_irq.o \
                ps3_nvme_resp_to_scsi.o \
                ps3_recovery.o \
                ps3_trace_id_alloc.o \
                ps3_device_manager_sas.o \
                ps3_ioc_adp.o \
                ps3_load.o \
                ps3_pci.o \
                ps3_sas_transport.o \
                ps3_watchdog.o \
                ./linux/ps3_base.o \
                ./linux/ps3_cli.o \
                ./linux/ps3_cli_debug.o \
                ./linux/ps3_driver_log.o \
                ./linux/ps3_dump.o \
                ./linux/ps3_pcie_err_handle.o

ccflags-y += -DPS3_CFG_RELEASE
ccflags-y += -DPS3_HARDWARE_ASIC
ccflags-y += -DPS3_MODEL_V200
ccflags-y += -Werror
ccflags-y += $(call cc-option, -Wmaybe-uninitialized)
ccflags-y += \
        -I$(srctree)/drivers/scsi/linkdata/ps3stor \
        -I$(srctree)/drivers/scsi/linkdata/ps3stor/linux \
        -I$(srctree)/drivers/scsi/linkdata/ps3stor/include \
        -I$(srctree)/drivers/scsi/linkdata/ps3stor/include/htp_v200