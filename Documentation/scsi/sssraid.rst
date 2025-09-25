.. SPDX-License-Identifier: GPL-2.0

==============================================
SSSRAID - 3SNIC SCSI RAID Controller driver
==============================================

This file describes the SSSRAID SCSI driver for 3SNIC
(http://www.3snic.com) RAID controllers. The SSSRAID
driver is the first generation RAID driver for 3SNIC Corp.

For 3SNIC SSSRAID controller support, enable the SSSRAID driver
when configuring the kernel.

SSSRAID specific entries in /sys
=================================

SSSRAID host attributes
------------------------
  - /sys/class/scsi_host/host*/csts_pp
  - /sys/class/scsi_host/host*/csts_shst
  - /sys/class/scsi_host/host*/csts_cfs
  - /sys/class/scsi_host/host*/csts_rdy
  - /sys/class/scsi_host/host*/fw_version

  The host csts_pp attribute is a read only attribute. This attribute
  indicates whether the controller is processing commands. If this attribute
  is set to '1', then the controller is processing commands normally. If
  this attribute is cleared to '0', then the controller has temporarily stopped
  processing commands in order to handle an event (e.g., firmware activation).

  The host csts_shst attribute is a read only attribute. This attribute
  indicates status of shutdown processing.The shutdown status values are defined
  as:
        ======     ==============================
        Value      Definition
        ======     ==============================
        00b        Normal operation
        01b        Shutdown processing occurring
        10b        Shutdown processing complete
        11b        Reserved
        ======     ==============================
  The host csts_cfs attribute is a read only attribute. This attribute is set to
  '1' when a fatal controller error occurred that could not be communicated in the
  appropriate Completion Queue. This bit is cleared to '0' when a fatal controller
  error has not occurred.

  The host csts_rdy attribute is a read only attribute. This attribute is set to
  '1' when the controller is ready to process submission queue entries.

  The fw_version attribute is read-only and will return the driver version and the
  controller firmware version.

SSSRAID scsi device attributes
------------------------------
  - /sys/class/scsi_device/X\:X\:X\:X/device/raid_level
  - /sys/class/scsi_device/X\:X\:X\:X/device/raid_state
  - /sys/class/scsi_device/X\:X\:X\:X/device/raid_resync

  The device raid_level attribute is a read only attribute. This attribute indicates
  RAID level of scsi device(will dispaly "NA" if scsi device is not virtual disk type).

  The device raid_state attribute is read-only and indicates RAID status of scsi
  device(will dispaly "NA" if scsi device is not virtual disk type).

  The device raid_resync attribute is read-only and indicates RAID rebuild processing
  of scsi device(will dispaly "NA" if scsi device is not virtual disk type).

Supported devices
=================

        ===================     ======= =======================================
        PCI ID (pci.ids)        OEM     Product
        ===================     ======= =======================================
        1F3F:2100               3SNIC 	3S510(HBA:8Ports,1G DDR)
        1F3F:2100               3SNIC 	3S520(HBA:16Ports,1G DDR)
        1F3F:2100               3SNIC 	3S530(HBA:32Ports,1G DDR)
        1F3F:2100               3SNIC 	3S540(HBA:40Ports,1G DDR)
        1F3F:2200               3SNIC 	3S580(RAID:16Ports,2G cache)
        1F3F:2200               3SNIC 	3S585(RAID:16Ports,4G cache)
        1F3F:2200               3SNIC 	3S590(RAID:32Ports,4G cache)
        1F3F:2200               3SNIC 	3S5A0(RAID:40Ports,2G cache)
        1F3F:2200               3SNIC 	3S5A5(RAID:40Ports,4G cache)
        ===================     ======= =======================================