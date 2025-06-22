// SPDX-License-Identifier: GPL-2.0
/*
 * Scsi Host Layer for MPT (Message Passing Technology) based controllers
 *
 * Copyright (C) 2013-2021  LSI Corporation
 * Copyright (C) 2013-2021  Avago Technologies
 * Copyright (C) 2013-2021  Broadcom Inc.
 *  (mailto:MPT-FusionLinux.pdl@broadcom.com)
 *
 * Copyright (C) 2024 LeapIO Tech Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * NO WARRANTY
 * THE PROGRAM IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT
 * LIMITATION, ANY WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Each Recipient is
 * solely responsible for determining the appropriateness of using and
 * distributing the Program and assumes all risks associated with its
 * exercise of rights under this Agreement, including but not limited to
 * the risks and costs of program errors, damage to or loss of data,
 * programs or equipment, and unavailability or interruption of operations.

 * DISCLAIMER OF LIABILITY
 * NEITHER RECIPIENT NOR ANY CONTRIBUTORS SHALL HAVE ANY LIABILITY FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING WITHOUT LIMITATION LOST PROFITS), HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OR DISTRIBUTION OF THE PROGRAM OR THE EXERCISE OF ANY RIGHTS GRANTED
 * HEREUNDER, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/blkdev.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <asm/unaligned.h>
#include <linux/aer.h>
#include <linux/raid_class.h>
#include "leapioraid_func.h"
#include <linux/blk-mq-pci.h>

#define RAID_CHANNEL 1

static void leapioraid_scsihost_expander_node_remove(
			struct LEAPIORAID_ADAPTER *ioc,
			struct leapioraid_raid_sas_node *sas_expander);
static void leapioraid_firmware_event_work(struct work_struct *work);
static void leapioraid_firmware_event_work_delayed(struct work_struct *work);
static enum device_responsive_state
leapioraid_scsihost_inquiry_vpd_sn(struct LEAPIORAID_ADAPTER *ioc, u16 handle,
			 u8 **serial_number);
static enum device_responsive_state
leapioraid_scsihost_inquiry_vpd_supported_pages(struct LEAPIORAID_ADAPTER *ioc,
				      u16 handle, u32 lun, void *data,
				      u32 data_length);
static enum device_responsive_state leapioraid_scsihost_ata_pass_thru_idd(
			struct LEAPIORAID_ADAPTER *ioc,
			u16 handle,
			u8 *is_ssd_device,
			u8 tr_timeout,
			u8 tr_method);
static enum device_responsive_state
leapioraid_scsihost_wait_for_target_to_become_ready(
			struct LEAPIORAID_ADAPTER *ioc,
			u16 handle, u8 retry_count, u8 is_pd,
			u8 tr_timeout, u8 tr_method);
static enum device_responsive_state
leapioraid_scsihost_wait_for_device_to_become_ready(
			struct LEAPIORAID_ADAPTER *ioc,
			u16 handle, u8 retry_count, u8 is_pd,
			int lun, u8 tr_timeout, u8 tr_method);
static void leapioraid_scsihost_remove_device(
			struct LEAPIORAID_ADAPTER *ioc,
			struct leapioraid_sas_device *sas_device);
static int leapioraid_scsihost_add_device(
			struct LEAPIORAID_ADAPTER *ioc, u16 handle,
			u8 retry_count, u8 is_pd);
static u8 leapioraid_scsihost_check_for_pending_tm(
			struct LEAPIORAID_ADAPTER *ioc, u16 smid);
static void leapioraid_scsihost_send_event_to_turn_on_pfa_led(
			struct LEAPIORAID_ADAPTER *ioc, u16 handle);
static void leapioraid_scsihost_complete_devices_scanning(
	struct LEAPIORAID_ADAPTER *ioc);

LIST_HEAD(leapioraid_ioc_list);
DEFINE_SPINLOCK(leapioraid_gioc_lock);

MODULE_AUTHOR(LEAPIORAID_AUTHOR);
MODULE_DESCRIPTION(LEAPIORAID_DESCRIPTION);
MODULE_LICENSE("GPL");
MODULE_VERSION(LEAPIORAID_DRIVER_VERSION);

static u8 scsi_io_cb_idx = -1;
static u8 tm_cb_idx = -1;
static u8 ctl_cb_idx = -1;
static u8 ctl_tm_cb_idx = -1;
static u8 base_cb_idx = -1;
static u8 port_enable_cb_idx = -1;
static u8 transport_cb_idx = -1;
static u8 scsih_cb_idx = -1;
static u8 config_cb_idx = -1;
static int leapioraid_ids;
static u8 tm_tr_cb_idx = -1;
static u8 tm_tr_volume_cb_idx = -1;
static u8 tm_tr_internal_cb_idx = -1;
static u8 tm_sas_control_cb_idx = -1;
static u32 logging_level;

MODULE_PARM_DESC(logging_level,
		 " bits for enabling additional logging info (default=0)");

static int open_pcie_trace;
module_param(open_pcie_trace, int, 0444);
MODULE_PARM_DESC(open_pcie_trace, "open_pcie_trace: open=1/default=0(close)");

static int disable_discovery = -1;
module_param(disable_discovery, int, 0444);
MODULE_PARM_DESC(disable_discovery, "disable discovery");

static struct raid_template *leapioraid_raid_template;

enum device_responsive_state {
	DEVICE_READY,
	DEVICE_RETRY,
	DEVICE_RETRY_UA,
	DEVICE_START_UNIT,
	DEVICE_STOP_UNIT,
	DEVICE_ERROR,
};

struct sense_info {
	u8 skey;
	u8 asc;
	u8 ascq;
};

#define LEAPIORAID_TURN_ON_PFA_LED (0xFFFC)
#define LEAPIORAID_PORT_ENABLE_COMPLETE (0xFFFD)
#define LEAPIORAID_REMOVE_UNRESPONDING_DEVICES (0xFFFF)

struct leapioraid_fw_event_work {
	struct list_head list;
	struct work_struct work;
	u8 cancel_pending_work;
	struct delayed_work delayed_work;
	u8 delayed_work_active;
	struct LEAPIORAID_ADAPTER *ioc;
	u16 device_handle;
	u8 VF_ID;
	u8 VP_ID;
	u8 ignore;
	u16 event;
	struct kref refcount;
	void *event_data;
	u8 *retries;
};

static void
leapioraid_fw_event_work_free(struct kref *r)
{
	struct leapioraid_fw_event_work *fw_work;

	fw_work = container_of(
		r, struct leapioraid_fw_event_work, refcount);
	kfree(fw_work->event_data);
	kfree(fw_work->retries);
	kfree(fw_work);
}

static void
leapioraid_fw_event_work_get(
	struct leapioraid_fw_event_work *fw_work)
{
	kref_get(&fw_work->refcount);
}

static void
leapioraid_fw_event_work_put(struct leapioraid_fw_event_work *fw_work)
{
	kref_put(&fw_work->refcount, leapioraid_fw_event_work_free);
}

static
struct leapioraid_fw_event_work *leapioraid_alloc_fw_event_work(int len)
{
	struct leapioraid_fw_event_work *fw_event;

	fw_event = kzalloc(sizeof(*fw_event) + len, GFP_ATOMIC);
	if (!fw_event)
		return NULL;
	kref_init(&fw_event->refcount);
	return fw_event;
}

static int
leapioraid_scsihost_set_debug_level(
	const char *val, const struct kernel_param *kp)
{
	int ret = param_set_int(val, kp);
	struct LEAPIORAID_ADAPTER *ioc;

	if (ret)
		return ret;
	pr_info("setting logging_level(0x%08x)\n", logging_level);
	spin_lock(&leapioraid_gioc_lock);
	list_for_each_entry(ioc, &leapioraid_ioc_list, list)
		ioc->logging_level = logging_level;
	spin_unlock(&leapioraid_gioc_lock);
	return 0;
}

module_param_call(logging_level,
		leapioraid_scsihost_set_debug_level, param_get_int,
		&logging_level, 0644);

static inline int
leapioraid_scsihost_srch_boot_sas_address(u64 sas_address,
				struct LEAPIORAID_BOOT_DEVICE_SAS_WWID *boot_device)
{
	return (sas_address == le64_to_cpu(boot_device->SASAddress)) ? 1 : 0;
}

static inline int
leapioraid_scsihost_srch_boot_device_name(u64 device_name,
				struct LEAPIORAID_BOOT_DEVICE_DEVICE_NAME *boot_device)
{
	return (device_name == le64_to_cpu(boot_device->DeviceName)) ? 1 : 0;
}

static inline int
leapioraid_scsihost_srch_boot_encl_slot(u64 enclosure_logical_id, u16 slot_number,
			      struct LEAPIORAID_BOOT_DEVICE_ENCLOSURE_SLOT *boot_device)
{
	return (enclosure_logical_id ==
		le64_to_cpu(boot_device->EnclosureLogicalID)
		&& slot_number == le16_to_cpu(boot_device->SlotNumber)) ? 1 : 0;
}

static void
leapioraid_scsihost_display_enclosure_chassis_info(
					struct LEAPIORAID_ADAPTER *ioc,
					struct leapioraid_sas_device *sas_device,
					struct scsi_device *sdev,
					struct scsi_target *starget)
{
	if (sdev) {
		if (sas_device->enclosure_handle != 0)
			sdev_printk(KERN_INFO, sdev,
				"enclosure logical id(0x%016llx), slot(%d)\n",
				(unsigned long long)sas_device->enclosure_logical_id,
				sas_device->slot);
		if (sas_device->connector_name[0] != '\0')
			sdev_printk(KERN_INFO, sdev,
				"enclosure level(0x%04x), connector name( %s)\n",
				sas_device->enclosure_level,
				sas_device->connector_name);
		if (sas_device->is_chassis_slot_valid)
			sdev_printk(KERN_INFO, sdev, "chassis slot(0x%04x)\n",
				    sas_device->chassis_slot);
	} else if (starget) {
		if (sas_device->enclosure_handle != 0)
			starget_printk(KERN_INFO, starget,
				"enclosure logical id(0x%016llx), slot(%d)\n",
				(unsigned long long)sas_device->enclosure_logical_id,
				sas_device->slot);
		if (sas_device->connector_name[0] != '\0')
			starget_printk(KERN_INFO, starget,
				"enclosure level(0x%04x), connector name( %s)\n",
				sas_device->enclosure_level,
				sas_device->connector_name);
		if (sas_device->is_chassis_slot_valid)
			starget_printk(KERN_INFO, starget,
				"chassis slot(0x%04x)\n", sas_device->chassis_slot);
	} else {
		if (sas_device->enclosure_handle != 0)
			pr_info("%s enclosure logical id(0x%016llx), slot(%d)\n",
			       ioc->name,
			       (unsigned long long)sas_device->enclosure_logical_id,
			       sas_device->slot);
		if (sas_device->connector_name[0] != '\0')
			pr_info("%s enclosure level(0x%04x),connector name( %s)\n",
			       ioc->name,
			       sas_device->enclosure_level,
			       sas_device->connector_name);
		if (sas_device->is_chassis_slot_valid)
			pr_info("%s chassis slot(0x%04x)\n",
			       ioc->name, sas_device->chassis_slot);
	}
}

struct leapioraid_hba_port *leapioraid_get_port_by_id(
	struct LEAPIORAID_ADAPTER *ioc,
	u8 port_id, u8 skip_dirty_flag)
{
	struct leapioraid_hba_port *port, *port_next;

	if (!ioc->multipath_on_hba)
		port_id = LEAPIORAID_MULTIPATH_DISABLED_PORT_ID;
	list_for_each_entry_safe(port, port_next, &ioc->port_table_list, list) {
		if (port->port_id != port_id)
			continue;
		if (port->flags & LEAPIORAID_HBA_PORT_FLAG_DIRTY_PORT)
			continue;
		return port;
	}
	if (skip_dirty_flag) {
		port = port_next = NULL;
		list_for_each_entry_safe(port, port_next,
					 &ioc->port_table_list, list) {
			if (port->port_id != port_id)
				continue;
			return port;
		}
	}
	if (unlikely(!ioc->multipath_on_hba)) {
		port = kzalloc(sizeof(struct leapioraid_hba_port), GFP_ATOMIC);
		if (!port)
			return NULL;

		port->port_id = LEAPIORAID_MULTIPATH_DISABLED_PORT_ID;
		pr_err(
			"%s hba_port entry: %p, port: %d is added to hba_port list\n",
			ioc->name, port, port->port_id);
		list_add_tail(&port->list, &ioc->port_table_list);
		return port;
	}
	return NULL;
}

struct leapioraid_virtual_phy *leapioraid_get_vphy_by_phy(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_hba_port *port, u32 phy)
{
	struct leapioraid_virtual_phy *vphy, *vphy_next;

	if (!port->vphys_mask)
		return NULL;
	list_for_each_entry_safe(vphy, vphy_next, &port->vphys_list, list) {
		if (vphy->phy_mask & (1 << phy))
			return vphy;
	}
	return NULL;
}

static int
leapioraid_scsihost_is_boot_device(u64 sas_address, u64 device_name,
			 u64 enclosure_logical_id, u16 slot, u8 form,
			 union LEAPIORAID_BIOSPAGE2_BOOT_DEVICE *boot_device)
{
	int rc = 0;

	switch (form) {
	case LEAPIORAID_BIOSPAGE2_FORM_SAS_WWID:
		if (!sas_address)
			break;
		rc = leapioraid_scsihost_srch_boot_sas_address(sas_address,
						     &boot_device->SasWwid);
		break;
	case LEAPIORAID_BIOSPAGE2_FORM_ENCLOSURE_SLOT:
		if (!enclosure_logical_id)
			break;
		rc = leapioraid_scsihost_srch_boot_encl_slot(
			enclosure_logical_id,
			slot,
			&boot_device->EnclosureSlot);
		break;
	case LEAPIORAID_BIOSPAGE2_FORM_DEVICE_NAME:
		if (!device_name)
			break;
		rc = leapioraid_scsihost_srch_boot_device_name(device_name,
						     &boot_device->DeviceName);
		break;
	case LEAPIORAID_BIOSPAGE2_FORM_NO_DEVICE_SPECIFIED:
		break;
	}
	return rc;
}

static int
leapioraid_scsihost_get_sas_address(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle,
	u64 *sas_address)
{
	struct LeapioraidSasDevP0_t sas_device_pg0;
	struct LeapioraidCfgRep_t mpi_reply;
	u32 ioc_status;

	*sas_address = 0;
	if ((leapioraid_config_get_sas_device_pg0
	     (ioc, &mpi_reply, &sas_device_pg0,
	      LEAPIORAID_SAS_DEVICE_PGAD_FORM_HANDLE, handle))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return -ENXIO;
	}
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status == LEAPIORAID_IOCSTATUS_SUCCESS) {
		if ((handle <= ioc->sas_hba.num_phys) &&
		    (!(le32_to_cpu(sas_device_pg0.DeviceInfo) &
		       LEAPIORAID_SAS_DEVICE_INFO_SEP)))
			*sas_address = ioc->sas_hba.sas_address;
		else
			*sas_address = le64_to_cpu(sas_device_pg0.SASAddress);
		return 0;
	}
	if (ioc_status == LEAPIORAID_IOCSTATUS_CONFIG_INVALID_PAGE)
		return -ENXIO;
	pr_err("%s handle(0x%04x), ioc_status(0x%04x), failure at %s:%d/%s()!\n",
	       ioc->name, handle, ioc_status,
	       __FILE__, __LINE__, __func__);
	return -EIO;
}

static void
leapioraid_scsihost_determine_boot_device(
	struct LEAPIORAID_ADAPTER *ioc, void *device,
	u32 channel)
{
	struct leapioraid_sas_device *sas_device;
	struct leapioraid_raid_device *raid_device;
	u64 sas_address;
	u64 device_name;
	u64 enclosure_logical_id;
	u16 slot;

	if (!ioc->is_driver_loading)
		return;
	if (!ioc->bios_pg3.BiosVersion)
		return;
	if (channel == RAID_CHANNEL) {
		raid_device = device;
		sas_address = raid_device->wwid;
		device_name = 0;
		enclosure_logical_id = 0;
		slot = 0;
	} else {
		sas_device = device;
		sas_address = sas_device->sas_address;
		device_name = sas_device->device_name;
		enclosure_logical_id = sas_device->enclosure_logical_id;
		slot = sas_device->slot;
	}
	if (!ioc->req_boot_device.device) {
		if (leapioraid_scsihost_is_boot_device(sas_address, device_name,
					     enclosure_logical_id, slot,
					     (ioc->bios_pg2.ReqBootDeviceForm &
					      LEAPIORAID_BIOSPAGE2_FORM_MASK),
					     &ioc->bios_pg2.RequestedBootDevice)) {
			dinitprintk(ioc,
				    pr_err(
					   "%s %s: req_boot_device(0x%016llx)\n",
					   ioc->name, __func__,
					   (unsigned long long)sas_address));
			ioc->req_boot_device.device = device;
			ioc->req_boot_device.channel = channel;
		}
	}
	if (!ioc->req_alt_boot_device.device) {
		if (leapioraid_scsihost_is_boot_device(sas_address, device_name,
					     enclosure_logical_id, slot,
					     (ioc->bios_pg2.ReqAltBootDeviceForm &
					      LEAPIORAID_BIOSPAGE2_FORM_MASK),
					     &ioc->bios_pg2.RequestedAltBootDevice)) {
			dinitprintk(ioc,
				    pr_err(
					   "%s %s: req_alt_boot_device(0x%016llx)\n",
					   ioc->name, __func__,
					   (unsigned long long)sas_address));
			ioc->req_alt_boot_device.device = device;
			ioc->req_alt_boot_device.channel = channel;
		}
	}
	if (!ioc->current_boot_device.device) {
		if (leapioraid_scsihost_is_boot_device(sas_address, device_name,
					     enclosure_logical_id, slot,
					     (ioc->bios_pg2.CurrentBootDeviceForm &
					      LEAPIORAID_BIOSPAGE2_FORM_MASK),
					     &ioc->bios_pg2.CurrentBootDevice)) {
			dinitprintk(ioc,
				    pr_err(
					   "%s %s: current_boot_device(0x%016llx)\n",
					   ioc->name, __func__,
					   (unsigned long long)sas_address));
			ioc->current_boot_device.device = device;
			ioc->current_boot_device.channel = channel;
		}
	}
}

static
struct leapioraid_sas_device *__leapioraid_get_sdev_from_target(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LEAPIORAID_TARGET *tgt_priv)
{
	struct leapioraid_sas_device *ret;

	assert_spin_locked(&ioc->sas_device_lock);
	ret = tgt_priv->sas_dev;
	if (ret)
		leapioraid_sas_device_get(ret);
	return ret;
}

static
struct leapioraid_sas_device *leapioraid_get_sdev_from_target(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LEAPIORAID_TARGET *tgt_priv)
{
	struct leapioraid_sas_device *ret;
	unsigned long flags;

	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	ret = __leapioraid_get_sdev_from_target(ioc, tgt_priv);
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	return ret;
}

static
struct leapioraid_sas_device *__leapioraid_get_sdev_by_addr(
	struct LEAPIORAID_ADAPTER *ioc,
	u64 sas_address, struct leapioraid_hba_port *port)
{
	struct leapioraid_sas_device *sas_device;

	if (!port)
		return NULL;
	assert_spin_locked(&ioc->sas_device_lock);
	list_for_each_entry(sas_device, &ioc->sas_device_list, list)
		if (sas_device->sas_address == sas_address &&
		sas_device->port == port)
			goto found_device;
	list_for_each_entry(sas_device, &ioc->sas_device_init_list, list)
		if (sas_device->sas_address == sas_address &&
		sas_device->port == port)
			goto found_device;
	return NULL;
found_device:
	leapioraid_sas_device_get(sas_device);
	return sas_device;
}

struct leapioraid_sas_device *__leapioraid_get_sdev_by_addr_and_rphy(
							   struct LEAPIORAID_ADAPTER *ioc,
							   u64 sas_address,
							   struct sas_rphy *rphy)
{
	struct leapioraid_sas_device *sas_device;

	assert_spin_locked(&ioc->sas_device_lock);
	list_for_each_entry(sas_device, &ioc->sas_device_list, list)
		if (sas_device->sas_address == sas_address &&
		(sas_device->rphy == rphy))
			goto found_device;
	list_for_each_entry(sas_device, &ioc->sas_device_init_list, list)
		if (sas_device->sas_address == sas_address &&
		(sas_device->rphy == rphy))
			goto found_device;
	return NULL;
found_device:
	leapioraid_sas_device_get(sas_device);
	return sas_device;
}

struct leapioraid_sas_device *leapioraid_get_sdev_by_addr(
	struct LEAPIORAID_ADAPTER *ioc,
	u64 sas_address,
	struct leapioraid_hba_port *port)
{
	struct leapioraid_sas_device *sas_device = NULL;
	unsigned long flags;

	if (!port)
		return sas_device;
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_device = __leapioraid_get_sdev_by_addr(ioc, sas_address, port);
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	return sas_device;
}

static struct leapioraid_sas_device *__leapioraid_get_sdev_by_handle(
		struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct leapioraid_sas_device *sas_device;

	assert_spin_locked(&ioc->sas_device_lock);
	list_for_each_entry(sas_device, &ioc->sas_device_list, list)
		if (sas_device->handle == handle)
			goto found_device;
	list_for_each_entry(sas_device, &ioc->sas_device_init_list, list)
		if (sas_device->handle == handle)
			goto found_device;
	return NULL;
found_device:
	leapioraid_sas_device_get(sas_device);
	return sas_device;
}

struct leapioraid_sas_device *leapioraid_get_sdev_by_handle(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct leapioraid_sas_device *sas_device;
	unsigned long flags;

	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_device = __leapioraid_get_sdev_by_handle(ioc, handle);
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	return sas_device;
}

void
leapioraid_scsihost_sas_device_remove(struct LEAPIORAID_ADAPTER *ioc,
			    struct leapioraid_sas_device *sas_device)
{
	unsigned long flags;
	int was_on_sas_device_list = 0;

	if (!sas_device)
		return;
	pr_info("%s %s: removing handle(0x%04x), sas_addr(0x%016llx)\n",
	       ioc->name, __func__, sas_device->handle,
	       (unsigned long long)sas_device->sas_address);
	leapioraid_scsihost_display_enclosure_chassis_info(
		ioc, sas_device, NULL, NULL);
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	if (!list_empty(&sas_device->list)) {
		list_del_init(&sas_device->list);
		was_on_sas_device_list = 1;
	}
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	if (was_on_sas_device_list) {
		kfree(sas_device->serial_number);
		leapioraid_sas_device_put(sas_device);
	}
}

static void
leapioraid_scsihost_device_remove_by_handle(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct leapioraid_sas_device *sas_device;
	unsigned long flags;
	int was_on_sas_device_list = 0;

	if (ioc->shost_recovery)
		return;
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_device = __leapioraid_get_sdev_by_handle(ioc, handle);
	if (sas_device) {
		if (!list_empty(&sas_device->list)) {
			list_del_init(&sas_device->list);
			was_on_sas_device_list = 1;
			leapioraid_sas_device_put(sas_device);
		}
	}
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	if (was_on_sas_device_list) {
		leapioraid_scsihost_remove_device(ioc, sas_device);
		leapioraid_sas_device_put(sas_device);
	}
}

void
leapioraid_device_remove_by_sas_address(
	struct LEAPIORAID_ADAPTER *ioc,
	u64 sas_address, struct leapioraid_hba_port *port)
{
	struct leapioraid_sas_device *sas_device;
	unsigned long flags;
	int was_on_sas_device_list = 0;

	if (ioc->shost_recovery)
		return;
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_device = __leapioraid_get_sdev_by_addr(ioc, sas_address, port);
	if (sas_device) {
		if (!list_empty(&sas_device->list)) {
			list_del_init(&sas_device->list);
			was_on_sas_device_list = 1;
			leapioraid_sas_device_put(sas_device);
		}
	}
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	if (was_on_sas_device_list) {
		leapioraid_scsihost_remove_device(ioc, sas_device);
		leapioraid_sas_device_put(sas_device);
	}
}

static void
leapioraid_scsihost_sas_device_add(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_sas_device *sas_device)
{
	unsigned long flags;

	dewtprintk(ioc, pr_info("%s %s: handle(0x%04x), sas_addr(0x%016llx)\n",
			       ioc->name,
			       __func__, sas_device->handle,
			       (unsigned long long)sas_device->sas_address));
	dewtprintk(ioc,
		   leapioraid_scsihost_display_enclosure_chassis_info(ioc, sas_device,
							    NULL, NULL));
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	leapioraid_sas_device_get(sas_device);
	list_add_tail(&sas_device->list, &ioc->sas_device_list);
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	if (ioc->hide_drives) {
		clear_bit(sas_device->handle, ioc->pend_os_device_add);
		return;
	}
	if (!leapioraid_transport_port_add(ioc, sas_device->handle,
					   sas_device->sas_address_parent,
					   sas_device->port)) {
		leapioraid_scsihost_sas_device_remove(ioc, sas_device);
	} else if (!sas_device->starget) {
		if (!ioc->is_driver_loading) {
			leapioraid_transport_port_remove(ioc,
							 sas_device->sas_address,
							 sas_device->sas_address_parent,
							 sas_device->port);
			leapioraid_scsihost_sas_device_remove(ioc, sas_device);
		}
	} else
		clear_bit(sas_device->handle, ioc->pend_os_device_add);
}

static void
leapioraid_scsihost_sas_device_init_add(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_sas_device *sas_device)
{
	unsigned long flags;

	dewtprintk(ioc, pr_info("%s %s: handle(0x%04x), sas_addr(0x%016llx)\n",
			       ioc->name,
			       __func__, sas_device->handle,
			       (unsigned long long)sas_device->sas_address));
	dewtprintk(ioc,
		   leapioraid_scsihost_display_enclosure_chassis_info(ioc, sas_device,
							    NULL, NULL));
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	leapioraid_sas_device_get(sas_device);
	list_add_tail(&sas_device->list, &ioc->sas_device_init_list);
	leapioraid_scsihost_determine_boot_device(ioc, sas_device, 0);
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
}

static
struct leapioraid_raid_device *leapioraid_scsihost_raid_device_find_by_id(
	struct LEAPIORAID_ADAPTER *ioc, int id, int channel)
{
	struct leapioraid_raid_device *raid_device, *r;

	r = NULL;
	list_for_each_entry(raid_device, &ioc->raid_device_list, list) {
		if (raid_device->id == id && raid_device->channel == channel) {
			r = raid_device;
			goto out;
		}
	}
out:
	return r;
}

struct leapioraid_raid_device *leapioraid_raid_device_find_by_handle(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct leapioraid_raid_device *raid_device, *r;

	r = NULL;
	list_for_each_entry(raid_device, &ioc->raid_device_list, list) {
		if (raid_device->handle != handle)
			continue;
		r = raid_device;
		goto out;
	}
out:
	return r;
}

static
struct leapioraid_raid_device *leapioraid_scsihost_raid_device_find_by_wwid(
		struct LEAPIORAID_ADAPTER *ioc, u64 wwid)
{
	struct leapioraid_raid_device *raid_device, *r;

	r = NULL;
	list_for_each_entry(raid_device, &ioc->raid_device_list, list) {
		if (raid_device->wwid != wwid)
			continue;
		r = raid_device;
		goto out;
	}
out:
	return r;
}

static void
leapioraid_scsihost_raid_device_add(struct LEAPIORAID_ADAPTER *ioc,
			  struct leapioraid_raid_device *raid_device)
{
	unsigned long flags;
	u8 protection_mask;

	dewtprintk(ioc, pr_info("%s %s: handle(0x%04x), wwid(0x%016llx)\n",
			       ioc->name,
			       __func__, raid_device->handle,
			       (unsigned long long)raid_device->wwid));
	spin_lock_irqsave(&ioc->raid_device_lock, flags);
	list_add_tail(&raid_device->list, &ioc->raid_device_list);
	if (!ioc->disable_eedp_support) {
		protection_mask = scsi_host_get_prot(ioc->shost);
		if (protection_mask & SHOST_DIX_TYPE0_PROTECTION) {
			scsi_host_set_prot(ioc->shost, protection_mask & 0x77);
			pr_err(
				"%s: Disabling DIX0 because of unsupport!\n",
					ioc->name);
		}
	}
	spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
}

static void
leapioraid_scsihost_raid_device_remove(struct LEAPIORAID_ADAPTER *ioc,
			     struct leapioraid_raid_device *raid_device)
{
	unsigned long flags;

	spin_lock_irqsave(&ioc->raid_device_lock, flags);
	list_del(&raid_device->list);
	kfree(raid_device);
	spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
}

struct leapioraid_raid_sas_node *leapioraid_scsihost_expander_find_by_handle(
		struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct leapioraid_raid_sas_node *sas_expander, *r;

	r = NULL;
	list_for_each_entry(sas_expander, &ioc->sas_expander_list, list) {
		if (sas_expander->handle != handle)
			continue;
		r = sas_expander;
		goto out;
	}
out:
	return r;
}

static
struct leapioraid_enclosure_node *leapioraid_scsihost_enclosure_find_by_handle(
								     struct LEAPIORAID_ADAPTER *ioc,
								     u16 handle)
{
	struct leapioraid_enclosure_node *enclosure_dev, *r;

	r = NULL;
	list_for_each_entry(enclosure_dev, &ioc->enclosure_list, list) {
		if (le16_to_cpu(enclosure_dev->pg0.EnclosureHandle) != handle)
			continue;
		r = enclosure_dev;
		goto out;
	}
out:
	return r;
}

struct leapioraid_raid_sas_node *leapioraid_scsihost_expander_find_by_sas_address(
								   struct LEAPIORAID_ADAPTER *ioc,
								   u64 sas_address,
								   struct leapioraid_hba_port *port)
{
	struct leapioraid_raid_sas_node *sas_expander, *r;

	r = NULL;
	if (!port)
		return r;
	list_for_each_entry(sas_expander, &ioc->sas_expander_list, list) {
		if (sas_expander->sas_address != sas_address ||
		    sas_expander->port != port)
			continue;
		r = sas_expander;
		goto out;
	}
out:
	return r;
}

static void
leapioraid_scsihost_expander_node_add(struct LEAPIORAID_ADAPTER *ioc,
			    struct leapioraid_raid_sas_node *sas_expander)
{
	unsigned long flags;

	spin_lock_irqsave(&ioc->sas_node_lock, flags);
	list_add_tail(&sas_expander->list, &ioc->sas_expander_list);
	spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
}

static int
leapioraid_scsihost_is_sas_end_device(u32 device_info)
{
	if (device_info & LEAPIORAID_SAS_DEVICE_INFO_END_DEVICE &&
		((device_info & LEAPIORAID_SAS_DEVICE_INFO_SSP_TARGET) |
		(device_info & LEAPIORAID_SAS_DEVICE_INFO_STP_TARGET) |
		(device_info & LEAPIORAID_SAS_DEVICE_INFO_SATA_DEVICE)))
		return 1;
	else
		return 0;
}

static u8
leapioraid_scsihost_scsi_lookup_find_by_target(
	struct LEAPIORAID_ADAPTER *ioc, int id,
	int channel)
{
	int smid;
	struct scsi_cmnd *scmd;

	for (smid = 1; smid <= ioc->shost->can_queue; smid++) {
		scmd = leapioraid_scsihost_scsi_lookup_get(ioc, smid);
		if (!scmd)
			continue;
		if (scmd->device->id == id && scmd->device->channel == channel)
			return 1;
	}
	return 0;
}

static u8
leapioraid_scsihost_scsi_lookup_find_by_lun(
	struct LEAPIORAID_ADAPTER *ioc, int id,
	unsigned int lun, int channel)
{
	int smid;
	struct scsi_cmnd *scmd;

	for (smid = 1; smid <= ioc->shost->can_queue; smid++) {
		scmd = leapioraid_scsihost_scsi_lookup_get(ioc, smid);
		if (!scmd)
			continue;
		if (scmd->device->id == id &&
		    scmd->device->channel == channel &&
		    scmd->device->lun == lun)
			return 1;
	}
	return 0;
}

struct scsi_cmnd *leapioraid_scsihost_scsi_lookup_get(
							  struct LEAPIORAID_ADAPTER *ioc, u16 smid)
{
	struct scsi_cmnd *scmd = NULL;
	struct leapioraid_scsiio_tracker *st;
	struct LeapioraidSCSIIOReq_t *mpi_request;
	u32 unique_tag = smid - 1;

	if (smid > 0 && smid <= ioc->shost->can_queue) {
		unique_tag =
		    ioc->io_queue_num[smid -
				      1] << BLK_MQ_UNIQUE_TAG_BITS | (smid - 1);
		mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
		if (!mpi_request->DevHandle)
			return scmd;
		scmd = scsi_host_find_tag(ioc->shost, unique_tag);
		if (scmd) {
			st = leapioraid_base_scsi_cmd_priv(scmd);
			if ((!st) || (st->cb_idx == 0xFF) || (st->smid == 0))
				scmd = NULL;
		}
	}
	return scmd;
}

static void
leapioraid_scsihost_display_sdev_qd(struct scsi_device *sdev)
{
	if (sdev->inquiry_len <= 7)
		return;
	sdev_printk(KERN_INFO, sdev,
		    "qdepth(%d), tagged(%d), scsi_level(%d), cmd_que(%d)\n",
		    sdev->queue_depth, sdev->tagged_supported,
		    sdev->scsi_level, ((sdev->inquiry[7] & 2) >> 1));
}

static int
leapioraid_scsihost_change_queue_depth(
	struct scsi_device *sdev, int qdepth)
{
	struct Scsi_Host *shost = sdev->host;
	int max_depth;
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	struct leapioraid_sas_device *sas_device;
	unsigned long flags;

	max_depth = shost->can_queue;

	goto not_sata;

	sas_device_priv_data = sdev->hostdata;
	if (!sas_device_priv_data)
		goto not_sata;
	sas_target_priv_data = sas_device_priv_data->sas_target;
	if (!sas_target_priv_data)
		goto not_sata;
	if ((sas_target_priv_data->flags & LEAPIORAID_TARGET_FLAGS_VOLUME))
		goto not_sata;
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_device =
	    __leapioraid_get_sdev_from_target(ioc, sas_target_priv_data);
	if (sas_device) {
		if (sas_device->device_info & LEAPIORAID_SAS_DEVICE_INFO_SATA_DEVICE)
			max_depth = LEAPIORAID_SATA_QUEUE_DEPTH;
		leapioraid_sas_device_put(sas_device);
	}
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
not_sata:
	if (!sdev->tagged_supported)
		max_depth = 1;
	if (qdepth > max_depth)
		qdepth = max_depth;
	scsi_change_queue_depth(sdev, qdepth);
	leapioraid_scsihost_display_sdev_qd(sdev);
	return sdev->queue_depth;
}

void
leapioraid__scsihost_change_queue_depth(
	struct scsi_device *sdev, int qdepth)
{
	struct Scsi_Host *shost = sdev->host;
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	if (ioc->enable_sdev_max_qd)
		qdepth = shost->can_queue;
	leapioraid_scsihost_change_queue_depth(sdev, qdepth);
}

static int
leapioraid_scsihost_target_alloc(struct scsi_target *starget)
{
	struct Scsi_Host *shost = dev_to_shost(&starget->dev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	struct leapioraid_sas_device *sas_device;
	struct leapioraid_raid_device *raid_device;
	unsigned long flags;
	struct sas_rphy *rphy;

	sas_target_priv_data =
	    kzalloc(sizeof(struct LEAPIORAID_TARGET), GFP_KERNEL);
	if (!sas_target_priv_data)
		return -ENOMEM;
	starget->hostdata = sas_target_priv_data;
	sas_target_priv_data->starget = starget;
	sas_target_priv_data->handle = LEAPIORAID_INVALID_DEVICE_HANDLE;
	if (starget->channel == RAID_CHANNEL) {
		spin_lock_irqsave(&ioc->raid_device_lock, flags);
		raid_device = leapioraid_scsihost_raid_device_find_by_id(
			ioc, starget->id, starget->channel);
		if (raid_device) {
			sas_target_priv_data->handle = raid_device->handle;
			sas_target_priv_data->sas_address = raid_device->wwid;
			sas_target_priv_data->flags |=
			    LEAPIORAID_TARGET_FLAGS_VOLUME;
			raid_device->starget = starget;
		}
		spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
		return 0;
	}
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	rphy = dev_to_rphy(starget->dev.parent);
	sas_device = __leapioraid_get_sdev_by_addr_and_rphy(ioc,
							    rphy->identify.sas_address, rphy);
	if (sas_device) {
		sas_target_priv_data->handle = sas_device->handle;
		sas_target_priv_data->sas_address = sas_device->sas_address;
		sas_target_priv_data->port = sas_device->port;
		sas_target_priv_data->sas_dev = sas_device;
		sas_device->starget = starget;
		sas_device->id = starget->id;
		sas_device->channel = starget->channel;
		if (test_bit(sas_device->handle, ioc->pd_handles))
			sas_target_priv_data->flags |=
			    LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT;
		if (sas_device->fast_path)
			sas_target_priv_data->flags |=
			    LEAPIORAID_TARGET_FASTPATH_IO;
	}
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	return 0;
}

static void
leapioraid_scsihost_target_destroy(struct scsi_target *starget)
{
	struct Scsi_Host *shost = dev_to_shost(&starget->dev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	struct leapioraid_sas_device *sas_device;
	struct leapioraid_raid_device *raid_device;
	unsigned long flags;

	sas_target_priv_data = starget->hostdata;
	if (!sas_target_priv_data)
		return;
	if (starget->channel == RAID_CHANNEL) {
		spin_lock_irqsave(&ioc->raid_device_lock, flags);
		raid_device = leapioraid_scsihost_raid_device_find_by_id(
			ioc, starget->id, starget->channel);
		if (raid_device) {
			raid_device->starget = NULL;
			raid_device->sdev = NULL;
		}
		spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
		goto out;
	}
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_device =
	    __leapioraid_get_sdev_from_target(ioc, sas_target_priv_data);
	if (sas_device && (sas_device->starget == starget)
	    && (sas_device->id == starget->id)
	    && (sas_device->channel == starget->channel))
		sas_device->starget = NULL;
	if (sas_device) {
		sas_target_priv_data->sas_dev = NULL;
		leapioraid_sas_device_put(sas_device);
		leapioraid_sas_device_put(sas_device);
	}
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
out:
	kfree(sas_target_priv_data);
	starget->hostdata = NULL;
}

static int
leapioraid_scsihost_slave_alloc(struct scsi_device *sdev)
{
	struct Scsi_Host *shost;
	struct LEAPIORAID_ADAPTER *ioc;
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct scsi_target *starget;
	struct leapioraid_raid_device *raid_device;
	struct leapioraid_sas_device *sas_device;
	unsigned long flags;

	sas_device_priv_data =
	    kzalloc(sizeof(*sas_device_priv_data), GFP_KERNEL);
	if (!sas_device_priv_data)
		return -ENOMEM;
	sas_device_priv_data->lun = sdev->lun;
	sas_device_priv_data->flags = LEAPIORAID_DEVICE_FLAGS_INIT;
	starget = scsi_target(sdev);
	sas_target_priv_data = starget->hostdata;
	sas_target_priv_data->num_luns++;
	sas_device_priv_data->sas_target = sas_target_priv_data;
	sdev->hostdata = sas_device_priv_data;
	if ((sas_target_priv_data->flags & LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT))
		sdev->no_uld_attach = 1;
	shost = dev_to_shost(&starget->dev);
	ioc = leapioraid_shost_private(shost);
	if (starget->channel == RAID_CHANNEL) {
		spin_lock_irqsave(&ioc->raid_device_lock, flags);
		raid_device = leapioraid_scsihost_raid_device_find_by_id(ioc,
							       starget->id,
							       starget->channel);
		if (raid_device)
			raid_device->sdev = sdev;
		spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
	}
	if (!(sas_target_priv_data->flags & LEAPIORAID_TARGET_FLAGS_VOLUME)) {
		spin_lock_irqsave(&ioc->sas_device_lock, flags);
		sas_device = __leapioraid_get_sdev_by_addr(ioc,
							   sas_target_priv_data->sas_address,
							   sas_target_priv_data->port);
		if (sas_device && (sas_device->starget == NULL)) {
			sdev_printk(KERN_INFO, sdev,
				    "%s : sas_device->starget set to starget @ %d\n",
				    __func__, __LINE__);
			sas_device->starget = starget;
		}
		if (sas_device)
			leapioraid_sas_device_put(sas_device);
		spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	}
	return 0;
}

static void
leapioraid_scsihost_slave_destroy(struct scsi_device *sdev)
{
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	struct scsi_target *starget;
	struct Scsi_Host *shost;
	struct LEAPIORAID_ADAPTER *ioc;
	struct leapioraid_sas_device *sas_device;
	unsigned long flags;

	if (!sdev->hostdata)
		return;
	starget = scsi_target(sdev);
	sas_target_priv_data = starget->hostdata;
	sas_target_priv_data->num_luns--;
	shost = dev_to_shost(&starget->dev);
	ioc = leapioraid_shost_private(shost);
	if (!(sas_target_priv_data->flags & LEAPIORAID_TARGET_FLAGS_VOLUME)) {
		spin_lock_irqsave(&ioc->sas_device_lock, flags);
		sas_device = __leapioraid_get_sdev_from_target(ioc,
							       sas_target_priv_data);
		if (sas_device && !sas_target_priv_data->num_luns)
			sas_device->starget = NULL;
		if (sas_device)
			leapioraid_sas_device_put(sas_device);
		spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	}
	kfree(sdev->hostdata);
	sdev->hostdata = NULL;
}

static void
leapioraid_scsihost_display_sata_capabilities(
	struct LEAPIORAID_ADAPTER *ioc,
	u16 handle, struct scsi_device *sdev)
{
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidSasDevP0_t sas_device_pg0;
	u32 ioc_status;
	u16 flags;
	u32 device_info;

	if ((leapioraid_config_get_sas_device_pg0
	     (ioc, &mpi_reply, &sas_device_pg0,
	      LEAPIORAID_SAS_DEVICE_PGAD_FORM_HANDLE, handle))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return;
	}
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return;
	}
	flags = le16_to_cpu(sas_device_pg0.Flags);
	device_info = le32_to_cpu(sas_device_pg0.DeviceInfo);
	sdev_printk(KERN_INFO, sdev,
		"atapi(%s), ncq(%s), asyn_notify(%s),\n\t\t"
		"smart(%s), fua(%s), sw_preserve(%s)\n",
		(device_info & LEAPIORAID_SAS_DEVICE_INFO_ATAPI_DEVICE) ? "y" :
		"n",
		(flags & LEAPIORAID_SAS_DEVICE0_FLAGS_SATA_NCQ_SUPPORTED) ? "y"
		: "n",
		(flags & LEAPIORAID_SAS_DEVICE0_FLAGS_SATA_ASYNCHRONOUS_NOTIFY)
		? "y" : "n",
		(flags & LEAPIORAID_SAS_DEVICE0_FLAGS_SATA_SMART_SUPPORTED) ?
		"y" : "n",
		(flags & LEAPIORAID_SAS_DEVICE0_FLAGS_SATA_FUA_SUPPORTED) ? "y"
		: "n",
		(flags & LEAPIORAID_SAS_DEVICE0_FLAGS_SATA_SW_PRESERVE) ? "y" :
		"n");
}

static int
leapioraid_scsihost_is_raid(struct device *dev)
{
	struct scsi_device *sdev = to_scsi_device(dev);

	return (sdev->channel == RAID_CHANNEL) ? 1 : 0;
}

static void
leapioraid_scsihost_get_resync(struct device *dev)
{
	struct scsi_device *sdev = to_scsi_device(dev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(sdev->host);
	static struct leapioraid_raid_device *raid_device;
	unsigned long flags;
	struct LeapioraidRaidVolP0_t vol_pg0;
	struct LeapioraidCfgRep_t mpi_reply;
	u32 volume_status_flags;
	u8 percent_complete;
	u16 handle;

	percent_complete = 0;
	handle = 0;
	spin_lock_irqsave(&ioc->raid_device_lock, flags);
	raid_device = leapioraid_scsihost_raid_device_find_by_id(
							ioc, sdev->id, sdev->channel);
	if (raid_device) {
		handle = raid_device->handle;
		percent_complete = raid_device->percent_complete;
	}
	spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
	if (!handle)
		goto out;
	if (leapioraid_config_get_raid_volume_pg0(ioc, &mpi_reply, &vol_pg0,
						  LEAPIORAID_RAID_VOLUME_PGAD_FORM_HANDLE,
						  handle,
						  sizeof
						  (struct LeapioraidRaidVolP0_t))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		percent_complete = 0;
		goto out;
	}
	volume_status_flags = le32_to_cpu(vol_pg0.VolumeStatusFlags);
	if (!(volume_status_flags &
	      LEAPIORAID_RAIDVOL0_STATUS_FLAG_RESYNC_IN_PROGRESS))
		percent_complete = 0;
out:
	raid_set_resync(leapioraid_raid_template, dev, percent_complete);
}

static void
leapioraid_scsihost_get_state(struct device *dev)
{
	struct scsi_device *sdev = to_scsi_device(dev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(sdev->host);
	static struct leapioraid_raid_device *raid_device;
	unsigned long flags;
	struct LeapioraidRaidVolP0_t vol_pg0;
	struct LeapioraidCfgRep_t mpi_reply;
	u32 volstate;
	enum raid_state state = RAID_STATE_UNKNOWN;
	u16 handle = 0;

	spin_lock_irqsave(&ioc->raid_device_lock, flags);
	raid_device = leapioraid_scsihost_raid_device_find_by_id(
							ioc, sdev->id, sdev->channel);
	if (raid_device)
		handle = raid_device->handle;
	spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
	if (!raid_device)
		goto out;
	if (leapioraid_config_get_raid_volume_pg0(ioc, &mpi_reply, &vol_pg0,
						  LEAPIORAID_RAID_VOLUME_PGAD_FORM_HANDLE,
						  handle,
						  sizeof
						  (struct LeapioraidRaidVolP0_t))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out;
	}
	volstate = le32_to_cpu(vol_pg0.VolumeStatusFlags);
	if (volstate & LEAPIORAID_RAIDVOL0_STATUS_FLAG_RESYNC_IN_PROGRESS) {
		state = RAID_STATE_RESYNCING;
		goto out;
	}
	switch (vol_pg0.VolumeState) {
	case LEAPIORAID_RAID_VOL_STATE_OPTIMAL:
	case LEAPIORAID_RAID_VOL_STATE_ONLINE:
		state = RAID_STATE_ACTIVE;
		break;
	case LEAPIORAID_RAID_VOL_STATE_DEGRADED:
		state = RAID_STATE_DEGRADED;
		break;
	case LEAPIORAID_RAID_VOL_STATE_FAILED:
	case LEAPIORAID_RAID_VOL_STATE_MISSING:
		state = RAID_STATE_OFFLINE;
		break;
	}
out:
	raid_set_state(leapioraid_raid_template, dev, state);
}

static void
leapioraid_scsihost_set_level(struct LEAPIORAID_ADAPTER *ioc,
		    struct scsi_device *sdev, u8 volume_type)
{
	enum raid_level level = RAID_LEVEL_UNKNOWN;

	switch (volume_type) {
	case LEAPIORAID_RAID_VOL_TYPE_RAID0:
		level = RAID_LEVEL_0;
		break;
	case LEAPIORAID_RAID_VOL_TYPE_RAID10:
	case LEAPIORAID_RAID_VOL_TYPE_RAID1E:
		level = RAID_LEVEL_10;
		break;
	case LEAPIORAID_RAID_VOL_TYPE_RAID1:
		level = RAID_LEVEL_1;
		break;
	}
	raid_set_level(leapioraid_raid_template, &sdev->sdev_gendev, level);
}

static int
leapioraid_scsihost_get_volume_capabilities(
				struct LEAPIORAID_ADAPTER *ioc,
				struct leapioraid_raid_device *raid_device)
{
	struct LeapioraidRaidVolP0_t *vol_pg0;
	struct LeapioraidRaidPDP0_t pd_pg0;
	struct LeapioraidSasDevP0_t sas_device_pg0;
	struct LeapioraidCfgRep_t mpi_reply;
	u16 sz;
	u8 num_pds;

	if ((leapioraid_config_get_number_pds(ioc, raid_device->handle,
					      &num_pds)) || !num_pds) {
		dfailprintk(ioc, pr_warn(
					"%s failure at %s:%d/%s()!\n", ioc->name,
					__FILE__, __LINE__, __func__));
		return 1;
	}
	raid_device->num_pds = num_pds;
	sz = offsetof(struct LeapioraidRaidVolP0_t, PhysDisk) + (num_pds *
							  sizeof
							  (struct LEAPIORAID_RAIDVOL0_PHYS_DISK));
	vol_pg0 = kzalloc(sz, GFP_KERNEL);
	if (!vol_pg0) {
		dfailprintk(ioc, pr_warn(
					"%s failure at %s:%d/%s()!\n", ioc->name,
					__FILE__, __LINE__, __func__));
		return 1;
	}
	if ((leapioraid_config_get_raid_volume_pg0(ioc, &mpi_reply, vol_pg0,
						   LEAPIORAID_RAID_VOLUME_PGAD_FORM_HANDLE,
						   raid_device->handle, sz))) {
		dfailprintk(ioc,
			    pr_warn(
				   "%s failure at %s:%d/%s()!\n", ioc->name,
				   __FILE__, __LINE__, __func__));
		kfree(vol_pg0);
		return 1;
	}
	raid_device->volume_type = vol_pg0->VolumeType;
	if (!(leapioraid_config_get_phys_disk_pg0(ioc, &mpi_reply,
						  &pd_pg0,
						  LEAPIORAID_PHYSDISK_PGAD_FORM_PHYSDISKNUM,
						  vol_pg0->PhysDisk[0].PhysDiskNum))) {
		if (!
		    (leapioraid_config_get_sas_device_pg0
		     (ioc, &mpi_reply, &sas_device_pg0,
		      LEAPIORAID_SAS_DEVICE_PGAD_FORM_HANDLE,
		      le16_to_cpu(pd_pg0.DevHandle)))) {
			raid_device->device_info =
			    le32_to_cpu(sas_device_pg0.DeviceInfo);
		}
	}
	kfree(vol_pg0);
	return 0;
}

static void
leapioraid_scsihost_enable_tlr(
	struct LEAPIORAID_ADAPTER *ioc, struct scsi_device *sdev)
{
	u8 data[30];
	u8 page_len, ii;
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	struct leapioraid_sas_device *sas_device;

	if (sdev->type != TYPE_TAPE)
		return;
	if (!(ioc->facts.IOCCapabilities & LEAPIORAID_IOCFACTS_CAPABILITY_TLR))
		return;
	sas_device_priv_data = sdev->hostdata;
	if (!sas_device_priv_data)
		return;
	sas_target_priv_data = sas_device_priv_data->sas_target;
	if (!sas_target_priv_data)
		return;
	if (leapioraid_scsihost_inquiry_vpd_supported_pages(ioc,
						  sas_target_priv_data->handle,
						  sdev->lun, data,
						  sizeof(data)) !=
	    DEVICE_READY) {
		sas_device =
		    leapioraid_get_sdev_by_addr(ioc,
						sas_target_priv_data->sas_address,
						sas_target_priv_data->port);
		if (sas_device) {
			sdev_printk(KERN_INFO, sdev,
				"%s: DEVICE NOT READY: handle(0x%04x),\n\t\t"
				"sas_addr(0x%016llx), phy(%d), device_name(0x%016llx)\n",
					__func__,
					sas_device->handle,
					(unsigned long long)sas_device->sas_address,
					sas_device->phy,
					(unsigned long long)sas_device->device_name);
			leapioraid_scsihost_display_enclosure_chassis_info(NULL,
								 sas_device,
								 sdev, NULL);
			leapioraid_sas_device_put(sas_device);
		}
		return;
	}
	page_len = data[3];
	for (ii = 4; ii < page_len + 4; ii++) {
		if (data[ii] == 0x90) {
			sas_device_priv_data->flags |= LEAPIORAID_DEVICE_TLR_ON;
			return;
		}
	}
}

static void
leapioraid_scsihost_enable_ssu_on_sata(
	struct leapioraid_sas_device *sas_device,
	struct scsi_device *sdev)
{
	if (!(sas_device->device_info & LEAPIORAID_SAS_DEVICE_INFO_SATA_DEVICE))
		return;
	if (sas_device->ssd_device) {
		sdev->manage_system_start_stop = 1;
		sdev->manage_runtime_start_stop = 1;
	}
}

static int
leapioraid_scsihost_slave_configure(struct scsi_device *sdev)
{
	struct Scsi_Host *shost = sdev->host;
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	struct leapioraid_sas_device *sas_device;
	struct leapioraid_raid_device *raid_device;
	unsigned long flags;
	int qdepth;
	u8 ssp_target = 0;
	char *ds = "";
	char *r_level = "";
	u16 handle, volume_handle = 0;
	u64 volume_wwid = 0;
	u8 *serial_number = NULL;
	enum device_responsive_state retval;
	u8 count = 0;

	qdepth = 1;
	sas_device_priv_data = sdev->hostdata;
	sas_device_priv_data->configured_lun = 1;
	sas_device_priv_data->flags &= ~LEAPIORAID_DEVICE_FLAGS_INIT;
	sas_target_priv_data = sas_device_priv_data->sas_target;
	handle = sas_target_priv_data->handle;
	if (sas_target_priv_data->flags & LEAPIORAID_TARGET_FLAGS_VOLUME) {
		spin_lock_irqsave(&ioc->raid_device_lock, flags);
		raid_device =
		    leapioraid_raid_device_find_by_handle(ioc, handle);
		spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
		if (!raid_device) {
			dfailprintk(ioc, pr_warn(
						"%s failure at %s:%d/%s()!\n",
						ioc->name, __FILE__, __LINE__,
						__func__));
			return 1;
		}
		if (leapioraid_scsihost_get_volume_capabilities(ioc, raid_device)) {
			dfailprintk(ioc, pr_warn(
						"%s failure at %s:%d/%s()!\n",
						ioc->name, __FILE__, __LINE__,
						__func__));
			return 1;
		}
		if (raid_device->device_info &
		    LEAPIORAID_SAS_DEVICE_INFO_SSP_TARGET) {
			qdepth = LEAPIORAID_SAS_QUEUE_DEPTH;
			ds = "SSP";
		} else {
			qdepth = LEAPIORAID_SATA_QUEUE_DEPTH;
			if (raid_device->device_info &
			    LEAPIORAID_SAS_DEVICE_INFO_SATA_DEVICE)
				ds = "SATA";
			else
				ds = "STP";
		}
		switch (raid_device->volume_type) {
		case LEAPIORAID_RAID_VOL_TYPE_RAID0:
			r_level = "RAID0";
			break;
		case LEAPIORAID_RAID_VOL_TYPE_RAID1E:
			qdepth = LEAPIORAID_RAID_QUEUE_DEPTH;
			if (ioc->manu_pg10.OEMIdentifier &&
			    (le32_to_cpu(ioc->manu_pg10.GenericFlags0) &
			     0x00000004) &&
			    !(raid_device->num_pds % 2))
				r_level = "RAID10";
			else
				r_level = "RAID1E";
			break;
		case LEAPIORAID_RAID_VOL_TYPE_RAID1:
			qdepth = LEAPIORAID_RAID_QUEUE_DEPTH;
			r_level = "RAID1";
			break;
		case LEAPIORAID_RAID_VOL_TYPE_RAID10:
			qdepth = LEAPIORAID_RAID_QUEUE_DEPTH;
			r_level = "RAID10";
			break;
		case LEAPIORAID_RAID_VOL_TYPE_UNKNOWN:
		default:
			qdepth = LEAPIORAID_RAID_QUEUE_DEPTH;
			r_level = "RAIDX";
			break;
		}
		if (!ioc->warpdrive_msg)
			sdev_printk(
				KERN_INFO, sdev,
				"%s: handle(0x%04x), wwid(0x%016llx), pd_count(%d), type(%s)\n",
				    r_level, raid_device->handle,
				    (unsigned long long)raid_device->wwid,
				    raid_device->num_pds, ds);
		if (shost->max_sectors > LEAPIORAID_RAID_MAX_SECTORS) {
			blk_queue_max_hw_sectors(sdev->request_queue,
						 LEAPIORAID_RAID_MAX_SECTORS);
			sdev_printk(KERN_INFO, sdev,
				    "Set queue's max_sector to: %u\n",
				    LEAPIORAID_RAID_MAX_SECTORS);
		}
		leapioraid__scsihost_change_queue_depth(sdev, qdepth);
		leapioraid_scsihost_set_level(ioc, sdev, raid_device->volume_type);
		return 0;
	}
	if (sas_target_priv_data->flags & LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT) {
		if (leapioraid_config_get_volume_handle(ioc, handle,
							&volume_handle)) {
			dfailprintk(ioc, pr_warn(
						"%s failure at %s:%d/%s()!\n",
						ioc->name, __FILE__, __LINE__,
						__func__));
			return 1;
		}
		if (volume_handle && leapioraid_config_get_volume_wwid(ioc,
								       volume_handle,
								       &volume_wwid)) {
			dfailprintk(ioc,
				    pr_warn(
					   "%s failure at %s:%d/%s()!\n",
					   ioc->name, __FILE__, __LINE__,
					   __func__));
			return 1;
		}
	}
	leapioraid_scsihost_inquiry_vpd_sn(ioc, handle, &serial_number);
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_device = __leapioraid_get_sdev_by_addr(ioc,
						   sas_device_priv_data->sas_target->sas_address,
						   sas_device_priv_data->sas_target->port);
	if (!sas_device) {
		spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
		dfailprintk(ioc, pr_warn(
					"%s failure at %s:%d/%s()!\n", ioc->name,
					__FILE__, __LINE__, __func__));
		kfree(serial_number);
		return 1;
	}
	sas_device->volume_handle = volume_handle;
	sas_device->volume_wwid = volume_wwid;
	sas_device->serial_number = serial_number;
	if (sas_device->device_info & LEAPIORAID_SAS_DEVICE_INFO_SSP_TARGET) {
		qdepth = (sas_device->port_type > 1) ?
		    ioc->max_wideport_qd : ioc->max_narrowport_qd;
		ssp_target = 1;
		if (sas_device->device_info & LEAPIORAID_SAS_DEVICE_INFO_SEP) {
			sdev_printk(KERN_WARNING, sdev,
				    "set ignore_delay_remove for handle(0x%04x)\n",
				    sas_device_priv_data->sas_target->handle);
			sas_device_priv_data->ignore_delay_remove = 1;
			ds = "SES";
		} else
			ds = "SSP";
	} else {
		qdepth = ioc->max_sata_qd;
		if (sas_device->device_info & LEAPIORAID_SAS_DEVICE_INFO_STP_TARGET)
			ds = "STP";
		else if (sas_device->device_info &
			 LEAPIORAID_SAS_DEVICE_INFO_SATA_DEVICE)
			ds = "SATA";
	}
	sdev_printk(
		KERN_INFO, sdev,
		"%s: handle(0x%04x), sas_addr(0x%016llx), phy(%d), device_name(0x%016llx)\n",
		    ds, handle, (unsigned long long)sas_device->sas_address,
		    sas_device->phy,
		    (unsigned long long)sas_device->device_name);
	leapioraid_scsihost_display_enclosure_chassis_info(
		NULL, sas_device, sdev, NULL);
	leapioraid_sas_device_put(sas_device);
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	if (!ssp_target) {
		leapioraid_scsihost_display_sata_capabilities(ioc, handle, sdev);
		do {
			retval = leapioraid_scsihost_ata_pass_thru_idd(ioc, handle,
							     &sas_device->ssd_device, 30, 0);
		} while ((retval == DEVICE_RETRY || retval == DEVICE_RETRY_UA)
			 && count++ < 3);
	}
	leapioraid_scsihost_enable_ssu_on_sata(sas_device, sdev);
	if (serial_number)
		sdev_printk(KERN_INFO, sdev, "serial_number(%s)\n",
			    serial_number);
	leapioraid__scsihost_change_queue_depth(sdev, qdepth);
	if (ssp_target) {
		sas_read_port_mode_page(sdev);
		leapioraid_scsihost_enable_tlr(ioc, sdev);
	}

	return 0;
}

static int
leapioraid_scsihost_bios_param(
	struct scsi_device *sdev, struct block_device *bdev,
	sector_t capacity, int params[])
{
	int heads;
	int sectors;
	sector_t cylinders;
	ulong dummy;

	heads = 64;
	sectors = 32;
	dummy = heads * sectors;
	cylinders = capacity;
	sector_div(cylinders, dummy);
	if ((ulong) capacity >= 0x200000) {
		heads = 255;
		sectors = 63;
		dummy = heads * sectors;
		cylinders = capacity;
		sector_div(cylinders, dummy);
	}
	params[0] = heads;
	params[1] = sectors;
	params[2] = cylinders;
	return 0;
}

static void
leapioraid_scsihost_response_code(
	struct LEAPIORAID_ADAPTER *ioc, u8 response_code)
{
	char *desc;

	switch (response_code) {
	case LEAPIORAID_SCSITASKMGMT_RSP_TM_COMPLETE:
		desc = "task management request completed";
		break;
	case LEAPIORAID_SCSITASKMGMT_RSP_INVALID_FRAME:
		desc = "invalid frame";
		break;
	case LEAPIORAID_SCSITASKMGMT_RSP_TM_NOT_SUPPORTED:
		desc = "task management request not supported";
		break;
	case LEAPIORAID_SCSITASKMGMT_RSP_TM_FAILED:
		desc = "task management request failed";
		break;
	case LEAPIORAID_SCSITASKMGMT_RSP_TM_SUCCEEDED:
		desc = "task management request succeeded";
		break;
	case LEAPIORAID_SCSITASKMGMT_RSP_TM_INVALID_LUN:
		desc = "invalid lun";
		break;
	case 0xA:
		desc = "overlapped tag attempted";
		break;
	case LEAPIORAID_SCSITASKMGMT_RSP_IO_QUEUED_ON_IOC:
		desc = "task queued, however not sent to target";
		break;
	default:
		desc = "unknown";
		break;
	}
	pr_warn("%s response_code(0x%01x): %s\n",
	       ioc->name, response_code, desc);
}

static u8
leapioraid_scsihost_tm_done(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid, u8 msix_index,
	u32 reply)
{
	struct LeapioraidDefaultRep_t *mpi_reply;

	if (ioc->tm_cmds.status == LEAPIORAID_CMD_NOT_USED)
		return 1;
	if (ioc->tm_cmds.smid != smid)
		return 1;
	ioc->tm_cmds.status |= LEAPIORAID_CMD_COMPLETE;
	mpi_reply = leapioraid_base_get_reply_virt_addr(ioc, reply);
	if (mpi_reply) {
		memcpy(ioc->tm_cmds.reply, mpi_reply, mpi_reply->MsgLength * 4);
		ioc->tm_cmds.status |= LEAPIORAID_CMD_REPLY_VALID;
	}
	ioc->tm_cmds.status &= ~LEAPIORAID_CMD_PENDING;
	complete(&ioc->tm_cmds.done);
	return 1;
}

void
leapioraid_scsihost_set_tm_flag(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct scsi_device *sdev;
	u8 skip = 0;

	shost_for_each_device(sdev, ioc->shost) {
		if (skip)
			continue;
		sas_device_priv_data = sdev->hostdata;
		if (!sas_device_priv_data)
			continue;
		if (sas_device_priv_data->sas_target->handle == handle) {
			sas_device_priv_data->sas_target->tm_busy = 1;
			skip = 1;
			ioc->ignore_loginfos = 1;
		}
	}
}

void
leapioraid_scsihost_clear_tm_flag(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct scsi_device *sdev;
	u8 skip = 0;

	shost_for_each_device(sdev, ioc->shost) {
		if (skip)
			continue;
		sas_device_priv_data = sdev->hostdata;
		if (!sas_device_priv_data)
			continue;
		if (sas_device_priv_data->sas_target->handle == handle) {
			sas_device_priv_data->sas_target->tm_busy = 0;
			skip = 1;
			ioc->ignore_loginfos = 0;
		}
	}
}

static int
leapioraid_scsihost_tm_cmd_map_status(
	struct LEAPIORAID_ADAPTER *ioc, uint channel,
	uint id, uint lun, u8 type, u16 smid_task)
{
	if (smid_task <= ioc->shost->can_queue) {
		switch (type) {
		case LEAPIORAID_SCSITASKMGMT_TASKTYPE_TARGET_RESET:
			if (!
			    (leapioraid_scsihost_scsi_lookup_find_by_target
			     (ioc, id, channel)))
				return SUCCESS;
			break;
		case LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABRT_TASK_SET:
		case LEAPIORAID_SCSITASKMGMT_TASKTYPE_LOGICAL_UNIT_RESET:
			if (!
			    (leapioraid_scsihost_scsi_lookup_find_by_lun
			     (ioc, id, lun, channel)))
				return SUCCESS;
			break;
		default:
			return SUCCESS;
		}
	} else if (smid_task == ioc->scsih_cmds.smid) {
		if ((ioc->scsih_cmds.status & LEAPIORAID_CMD_COMPLETE) ||
		    (ioc->scsih_cmds.status & LEAPIORAID_CMD_NOT_USED))
			return SUCCESS;
	} else if (smid_task == ioc->ctl_cmds.smid) {
		if ((ioc->ctl_cmds.status & LEAPIORAID_CMD_COMPLETE) ||
		    (ioc->ctl_cmds.status & LEAPIORAID_CMD_NOT_USED))
			return SUCCESS;
	}
	return FAILED;
}

static int
leapioraid_scsihost_tm_post_processing(struct LEAPIORAID_ADAPTER *ioc, u16 handle,
			 uint channel, uint id, uint lun, u8 type,
			 u16 smid_task)
{
	int rc;

	rc = leapioraid_scsihost_tm_cmd_map_status(ioc, channel, id, lun, type, smid_task);
	if (rc == SUCCESS)
		return rc;
	pr_err(
		"%s Poll finish of smid(%d),task_type(0x%02x),handle(0x%04x)\n",
			ioc->name,
			smid_task,
			type,
			handle);
	leapioraid_base_mask_interrupts(ioc);
	leapioraid_base_sync_reply_irqs(ioc, 1);
	leapioraid_base_unmask_interrupts(ioc);
	return leapioraid_scsihost_tm_cmd_map_status(
		ioc, channel, id, lun, type, smid_task);
}

int
leapioraid_scsihost_issue_tm(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle,
	uint channel, uint id, uint lun, u8 type,
	u16 smid_task, u8 timeout, u8 tr_method)
{
	struct LeapioraidSCSITmgReq_t *mpi_request;
	struct LeapioraidSCSITmgRep_t *mpi_reply;
	struct LeapioraidSCSIIOReq_t *request;
	u16 smid = 0;
	u32 ioc_state;
	struct leapioraid_scsiio_tracker *scsi_lookup = NULL;
	int rc;
	u16 msix_task = 0;
	u8 issue_reset = 0;

	lockdep_assert_held(&ioc->tm_cmds.mutex);
	if (ioc->tm_cmds.status != LEAPIORAID_CMD_NOT_USED) {
		pr_info("%s %s: tm_cmd busy!!!\n",
		       __func__, ioc->name);
		return FAILED;
	}
	if (ioc->shost_recovery || ioc->remove_host || ioc->pci_error_recovery) {
		pr_info("%s %s: host reset in progress!\n",
		       __func__, ioc->name);
		return FAILED;
	}
	ioc_state = leapioraid_base_get_iocstate(ioc, 0);
	if (ioc_state & LEAPIORAID_DOORBELL_USED) {
		pr_info("%s unexpected doorbell active!\n",
		       ioc->name);
		rc = leapioraid_base_hard_reset_handler(ioc, FORCE_BIG_HAMMER);
		return (!rc) ? SUCCESS : FAILED;
	}
	if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_FAULT) {
		leapioraid_print_fault_code(ioc, ioc_state &
					    LEAPIORAID_DOORBELL_DATA_MASK);
		rc = leapioraid_base_hard_reset_handler(ioc, FORCE_BIG_HAMMER);
		return (!rc) ? SUCCESS : FAILED;
	} else if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) ==
		   LEAPIORAID_IOC_STATE_COREDUMP) {
		leapioraid_base_coredump_info(ioc,
					      ioc_state &
					      LEAPIORAID_DOORBELL_DATA_MASK);
		rc = leapioraid_base_hard_reset_handler(ioc, FORCE_BIG_HAMMER);
		return (!rc) ? SUCCESS : FAILED;
	}
	smid = leapioraid_base_get_smid_hpr(ioc, ioc->tm_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		return FAILED;
	}
	if (type == LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABORT_TASK)
		scsi_lookup = leapioraid_get_st_from_smid(ioc, smid_task);
	dtmprintk(ioc, pr_info(
		"%s sending tm: handle(0x%04x),\n\t\t"
		"task_type(0x%02x), timeout(%d) tr_method(0x%x) smid(%d)\n",
			ioc->name,
			handle,
			type,
			timeout,
			tr_method,
			smid_task));
	ioc->tm_cmds.status = LEAPIORAID_CMD_PENDING;
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->tm_cmds.smid = smid;
	memset(mpi_request, 0, sizeof(struct LeapioraidSCSITmgReq_t));
	memset(ioc->tm_cmds.reply, 0, sizeof(struct LeapioraidSCSITmgRep_t));
	mpi_request->Function = LEAPIORAID_FUNC_SCSI_TASK_MGMT;
	mpi_request->DevHandle = cpu_to_le16(handle);
	mpi_request->TaskType = type;
	mpi_request->MsgFlags = tr_method;
	if (type == LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABORT_TASK ||
	    type == LEAPIORAID_SCSITASKMGMT_TASKTYPE_QUERY_TASK)
		mpi_request->TaskMID = cpu_to_le16(smid_task);
	int_to_scsilun(lun, (struct scsi_lun *)mpi_request->LUN);
	leapioraid_scsihost_set_tm_flag(ioc, handle);
	init_completion(&ioc->tm_cmds.done);
	if ((type == LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABORT_TASK) &&
	    (scsi_lookup && (scsi_lookup->msix_io < ioc->reply_queue_count)))
		msix_task = scsi_lookup->msix_io;
	else
		msix_task = 0;
	ioc->put_smid_hi_priority(ioc, smid, msix_task);
	wait_for_completion_timeout(&ioc->tm_cmds.done, timeout * HZ);
	if (!(ioc->tm_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		leapioraid_check_cmd_timeout(ioc,
					     ioc->tm_cmds.status, mpi_request,
					     sizeof
					     (struct LeapioraidSCSITmgReq_t)
					     / 4, issue_reset);
		if (issue_reset) {
			rc = leapioraid_base_hard_reset_handler(ioc,
								FORCE_BIG_HAMMER);
			rc = (!rc) ? SUCCESS : FAILED;
			goto out;
		}
	}
	leapioraid_base_sync_reply_irqs(ioc, 0);
	if (ioc->tm_cmds.status & LEAPIORAID_CMD_REPLY_VALID) {
		mpi_reply = ioc->tm_cmds.reply;
		dtmprintk(ioc, pr_info(
			"%s complete tm: ioc_status(0x%04x),\n\t\t"
			"loginfo(0x%08x), term_count(0x%08x)\n",
				ioc->name,
				le16_to_cpu(mpi_reply->IOCStatus),
				le32_to_cpu(mpi_reply->IOCLogInfo),
				le32_to_cpu(mpi_reply->TerminationCount)));
		if (ioc->logging_level & LEAPIORAID_DEBUG_TM) {
			leapioraid_scsihost_response_code(
				ioc, mpi_reply->ResponseCode);
			if (mpi_reply->IOCStatus)
				leapioraid_debug_dump_mf(
					mpi_request,
					sizeof(struct LeapioraidSCSITmgReq_t) / 4);
		}
	}
	switch (type) {
	case LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABORT_TASK:
		rc = SUCCESS;
		request = leapioraid_base_get_msg_frame(ioc, smid_task);
		if (le16_to_cpu(request->DevHandle) != handle)
			break;
		pr_err(
			"%s Task abort tm failed:\n\t\t"
			"handle(0x%04x), timeout(%d),\n\t\t"
			"tr_method(0x%x), smid(%d), msix_index(%d)\n",
				ioc->name,
				handle,
				timeout,
				tr_method,
				smid_task,
				msix_task);
		rc = FAILED;
		break;
	case LEAPIORAID_SCSITASKMGMT_TASKTYPE_TARGET_RESET:
	case LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABRT_TASK_SET:
	case LEAPIORAID_SCSITASKMGMT_TASKTYPE_LOGICAL_UNIT_RESET:
		rc = leapioraid_scsihost_tm_post_processing(
			ioc, handle, channel, id, lun, type, smid_task);
		break;
	case LEAPIORAID_SCSITASKMGMT_TASKTYPE_QUERY_TASK:
		rc = SUCCESS;
		break;
	default:
		rc = FAILED;
		break;
	}
out:
	leapioraid_scsihost_clear_tm_flag(ioc, handle);
	ioc->tm_cmds.status = LEAPIORAID_CMD_NOT_USED;
	return rc;
}

int
leapioraid_scsihost_issue_locked_tm(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle,
	uint channel, uint id, uint lun, u8 type,
	u16 smid_task, u8 timeout, u8 tr_method)
{
	int ret;

	mutex_lock(&ioc->tm_cmds.mutex);
	ret = leapioraid_scsihost_issue_tm(
		ioc, handle, channel, id, lun, type,
		smid_task, timeout, tr_method);
	mutex_unlock(&ioc->tm_cmds.mutex);
	return ret;
}

static void
leapioraid_scsihost_tm_display_info(
	struct LEAPIORAID_ADAPTER *ioc,
	struct scsi_cmnd *scmd)
{
	struct scsi_target *starget = scmd->device->sdev_target;
	struct LEAPIORAID_TARGET *priv_target = starget->hostdata;
	struct leapioraid_sas_device *sas_device = NULL;
	unsigned long flags;
	char *device_str = NULL;

	if (!priv_target)
		return;
	if (ioc->warpdrive_msg)
		device_str = "WarpDrive";
	else
		device_str = "volume";
	scsi_print_command(scmd);
	if (priv_target->flags & LEAPIORAID_TARGET_FLAGS_VOLUME) {
		starget_printk(
			KERN_INFO, starget, "%s handle(0x%04x), %s wwid(0x%016llx)\n",
			device_str,
			priv_target->handle, device_str,
			(unsigned long long)priv_target->sas_address);
	} else {
		spin_lock_irqsave(&ioc->sas_device_lock, flags);
		sas_device =
		    __leapioraid_get_sdev_from_target(ioc, priv_target);
		if (sas_device) {
			if (priv_target->flags &
			    LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT) {
				starget_printk(KERN_INFO, starget,
					"volume handle(0x%04x), volume wwid(0x%016llx)\n",
					sas_device->volume_handle,
					(unsigned long long)sas_device->volume_wwid);
			}
			starget_printk(KERN_INFO, starget,
				"%s: handle(0x%04x), sas_address(0x%016llx), phy(%d)\n",
				__func__, sas_device->handle,
				(unsigned long long)sas_device->sas_address, sas_device->phy);
			leapioraid_scsihost_display_enclosure_chassis_info(NULL,
								 sas_device,
								 NULL, starget);
			leapioraid_sas_device_put(sas_device);
		}
		spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	}
}

static int
leapioraid_scsihost_abort(struct scsi_cmnd *scmd)
{
	struct LEAPIORAID_ADAPTER *ioc
		= leapioraid_shost_private(scmd->device->host);
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	u16 handle;
	int r;
	struct leapioraid_scsiio_tracker *st
		= leapioraid_base_scsi_cmd_priv(scmd);
	u8 timeout = 30;

	sdev_printk(
		KERN_INFO, scmd->device,
		"attempting task abort! scmd(0x%p), outstanding for %u ms & timeout %u ms\n",
		scmd, jiffies_to_msecs(jiffies - scmd->jiffies_at_alloc),
		(scsi_cmd_to_rq(scmd)->timeout / HZ) * 1000);
	leapioraid_scsihost_tm_display_info(ioc, scmd);
	if (leapioraid_base_pci_device_is_unplugged(ioc) || ioc->remove_host) {
		sdev_printk(KERN_INFO, scmd->device, "%s scmd(0x%p)\n",
			((ioc->remove_host) ? ("shost is getting removed!")
				: ("pci device been removed!")), scmd);
		if (st && st->smid)
			leapioraid_base_free_smid(ioc, st->smid);
		scmd->result = DID_NO_CONNECT << 16;
		r = FAILED;
		goto out;
	}
	sas_device_priv_data = scmd->device->hostdata;
	if (!sas_device_priv_data || !sas_device_priv_data->sas_target) {
		sdev_printk(KERN_INFO, scmd->device,
			"device been deleted! scmd(0x%p)\n", scmd);
		scmd->result = DID_NO_CONNECT << 16;
		scsi_done(scmd);
		r = SUCCESS;
		goto out;
	}
	if (st == NULL || st->cb_idx == 0xFF) {
		sdev_printk(KERN_INFO, scmd->device,
			"No ref at driver, assuming scmd(0x%p) might have completed\n",
				scmd);
		scmd->result = DID_RESET << 16;
		r = SUCCESS;
		goto out;
	}
	if (sas_device_priv_data->sas_target->flags &
	    LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT ||
	    sas_device_priv_data->sas_target->flags & LEAPIORAID_TARGET_FLAGS_VOLUME) {
		scmd->result = DID_RESET << 16;
		r = FAILED;
		goto out;
	}
	leapioraid_halt_firmware(ioc, 0);
	handle = sas_device_priv_data->sas_target->handle;
	r = leapioraid_scsihost_issue_locked_tm(
			ioc, handle,
			scmd->device->channel,
			scmd->device->id,
			scmd->device->lun,
			LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABORT_TASK,
			st->smid, timeout, 0);
out:
	sdev_printk(
		KERN_INFO, scmd->device,
		"task abort: %s scmd(0x%p)\n",
		((r == SUCCESS) ? "SUCCESS" : "FAILED"), scmd);
	return r;
}

static int
leapioraid_scsihost_dev_reset(struct scsi_cmnd *scmd)
{
	struct LEAPIORAID_ADAPTER *ioc
		= leapioraid_shost_private(scmd->device->host);
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct leapioraid_sas_device *sas_device = NULL;
	u16 handle;
	u8 tr_method = 0;
	u8 tr_timeout = 30;
	int r;
	struct scsi_target *starget = scmd->device->sdev_target;
	struct LEAPIORAID_TARGET *target_priv_data = starget->hostdata;

	sdev_printk(KERN_INFO, scmd->device,
		"attempting device reset! scmd(0x%p)\n", scmd);
	leapioraid_scsihost_tm_display_info(ioc, scmd);
	if (leapioraid_base_pci_device_is_unplugged(ioc) || ioc->remove_host) {
		sdev_printk(KERN_INFO, scmd->device, "%s scmd(0x%p)\n",
			((ioc->remove_host) ? ("shost is getting removed!")
				: ("pci device been removed!")), scmd);
		scmd->result = DID_NO_CONNECT << 16;
		r = FAILED;
		goto out;
	}
	sas_device_priv_data = scmd->device->hostdata;
	if (!sas_device_priv_data || !sas_device_priv_data->sas_target) {
		sdev_printk(KERN_INFO, scmd->device,
			"device been deleted! scmd(0x%p)\n", scmd);
		scmd->result = DID_NO_CONNECT << 16;
		scsi_done(scmd);
		r = SUCCESS;
		goto out;
	}
	handle = 0;
	if (sas_device_priv_data->sas_target->flags &
	    LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT) {
		sas_device = leapioraid_get_sdev_from_target(ioc,
							     target_priv_data);
		if (sas_device)
			handle = sas_device->volume_handle;
	} else
		handle = sas_device_priv_data->sas_target->handle;
	if (!handle) {
		scmd->result = DID_RESET << 16;
		r = FAILED;
		goto out;
	}
	tr_method = LEAPIORAID_SCSITASKMGMT_MSGFLAGS_LINK_RESET;
	r = leapioraid_scsihost_issue_locked_tm(ioc, handle,
			scmd->device->channel,
			scmd->device->id,
			scmd->device->lun,
			LEAPIORAID_SCSITASKMGMT_TASKTYPE_LOGICAL_UNIT_RESET,
			0, tr_timeout, tr_method);
out:
	sdev_printk(KERN_INFO, scmd->device,
		"device reset: %s scmd(0x%p)\n",
		((r == SUCCESS) ? "SUCCESS" : "FAILED"), scmd);
	if (sas_device)
		leapioraid_sas_device_put(sas_device);
	return r;
}

static int
leapioraid_scsihost_target_reset(struct scsi_cmnd *scmd)
{
	struct LEAPIORAID_ADAPTER *ioc
		= leapioraid_shost_private(scmd->device->host);
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct leapioraid_sas_device *sas_device = NULL;
	u16 handle;
	u8 tr_method = 0;
	u8 tr_timeout = 30;
	int r;
	struct scsi_target *starget = scmd->device->sdev_target;
	struct LEAPIORAID_TARGET *target_priv_data = starget->hostdata;

	starget_printk(KERN_INFO, starget,
		"attempting target reset! scmd(0x%p)\n", scmd);
	leapioraid_scsihost_tm_display_info(ioc, scmd);
	if (leapioraid_base_pci_device_is_unplugged(ioc) || ioc->remove_host) {
		sdev_printk(KERN_INFO, scmd->device, "%s scmd(0x%p)\n",
			((ioc->remove_host) ? ("shost is getting removed!")
				: ("pci device been removed!")), scmd);
		scmd->result = DID_NO_CONNECT << 16;
		r = FAILED;
		goto out;
	}
	sas_device_priv_data = scmd->device->hostdata;
	if (!sas_device_priv_data || !sas_device_priv_data->sas_target) {
		starget_printk(KERN_INFO, starget,
			"target been deleted! scmd(0x%p)\n", scmd);
		scmd->result = DID_NO_CONNECT << 16;
		scsi_done(scmd);
		r = SUCCESS;
		goto out;
	}
	handle = 0;
	if (sas_device_priv_data->sas_target->flags &
	    LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT) {
		sas_device = leapioraid_get_sdev_from_target(ioc,
							     target_priv_data);
		if (sas_device)
			handle = sas_device->volume_handle;
	} else
		handle = sas_device_priv_data->sas_target->handle;
	if (!handle) {
		scmd->result = DID_RESET << 16;
		r = FAILED;
		goto out;
	}
	tr_method = LEAPIORAID_SCSITASKMGMT_MSGFLAGS_LINK_RESET;
	r = leapioraid_scsihost_issue_locked_tm(ioc, handle,
			scmd->device->channel,
			scmd->device->id, 0,
			LEAPIORAID_SCSITASKMGMT_TASKTYPE_TARGET_RESET,
			0, tr_timeout, tr_method);
out:
	starget_printk(KERN_INFO, starget,
		"target reset: %s scmd(0x%p)\n",
		((r == SUCCESS) ? "SUCCESS" : "FAILED"), scmd);
	if (sas_device)
		leapioraid_sas_device_put(sas_device);
	return r;
}

static int
leapioraid_scsihost_host_reset(struct scsi_cmnd *scmd)
{
	struct LEAPIORAID_ADAPTER *ioc
		= leapioraid_shost_private(scmd->device->host);
	int r, retval;

	pr_info("%s attempting host reset! scmd(0x%p)\n",
	       ioc->name, scmd);
	scsi_print_command(scmd);
	if (ioc->is_driver_loading || ioc->remove_host) {
		pr_info("%s Blocking the host reset\n",
		       ioc->name);
		r = FAILED;
		goto out;
	}
	retval = leapioraid_base_hard_reset_handler(
		ioc, FORCE_BIG_HAMMER);
	r = (retval < 0) ? FAILED : SUCCESS;
out:
	pr_info("%s host reset: %s scmd(0x%p)\n",
	       ioc->name, ((r == SUCCESS) ? "SUCCESS" : "FAILED"),
		   scmd);
	return r;
}

static void
leapioraid_scsihost_fw_event_add(struct LEAPIORAID_ADAPTER *ioc,
		       struct leapioraid_fw_event_work *fw_event)
{
	unsigned long flags;

	if (ioc->firmware_event_thread == NULL)
		return;
	spin_lock_irqsave(&ioc->fw_event_lock, flags);
	leapioraid_fw_event_work_get(fw_event);
	INIT_LIST_HEAD(&fw_event->list);
	list_add_tail(&fw_event->list, &ioc->fw_event_list);
	INIT_WORK(&fw_event->work, leapioraid_firmware_event_work);
	leapioraid_fw_event_work_get(fw_event);
	queue_work(ioc->firmware_event_thread, &fw_event->work);
	spin_unlock_irqrestore(&ioc->fw_event_lock, flags);
}

static void
leapioraid_scsihost_fw_event_del_from_list(
			struct LEAPIORAID_ADAPTER *ioc,
			struct leapioraid_fw_event_work *fw_event)
{
	unsigned long flags;

	spin_lock_irqsave(&ioc->fw_event_lock, flags);
	if (!list_empty(&fw_event->list)) {
		list_del_init(&fw_event->list);
		leapioraid_fw_event_work_put(fw_event);
	}
	spin_unlock_irqrestore(&ioc->fw_event_lock, flags);
}

static void
leapioraid_scsihost_fw_event_requeue(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_fw_event_work *fw_event, unsigned long delay)
{
	unsigned long flags;

	if (ioc->firmware_event_thread == NULL)
		return;
	spin_lock_irqsave(&ioc->fw_event_lock, flags);
	leapioraid_fw_event_work_get(fw_event);
	list_add_tail(&fw_event->list, &ioc->fw_event_list);
	if (!fw_event->delayed_work_active) {
		fw_event->delayed_work_active = 1;
		INIT_DELAYED_WORK(&fw_event->delayed_work,
				  leapioraid_firmware_event_work_delayed);
	}
	queue_delayed_work(ioc->firmware_event_thread, &fw_event->delayed_work,
			   msecs_to_jiffies(delay));
	spin_unlock_irqrestore(&ioc->fw_event_lock, flags);
}

static void
leapioraid_scsihost_error_recovery_delete_devices(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_fw_event_work *fw_event;

	fw_event = leapioraid_alloc_fw_event_work(0);
	if (!fw_event)
		return;
	fw_event->event = LEAPIORAID_REMOVE_UNRESPONDING_DEVICES;
	fw_event->ioc = ioc;
	leapioraid_scsihost_fw_event_add(ioc, fw_event);
	leapioraid_fw_event_work_put(fw_event);
}

void
leapioraid_port_enable_complete(struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_fw_event_work *fw_event;

	fw_event = leapioraid_alloc_fw_event_work(0);
	if (!fw_event)
		return;
	fw_event->event = LEAPIORAID_PORT_ENABLE_COMPLETE;
	fw_event->ioc = ioc;
	leapioraid_scsihost_fw_event_add(ioc, fw_event);
	leapioraid_fw_event_work_put(fw_event);
}

static struct leapioraid_fw_event_work *dequeue_next_fw_event(
						   struct LEAPIORAID_ADAPTER *ioc)
{
	unsigned long flags;
	struct leapioraid_fw_event_work *fw_event = NULL;

	spin_lock_irqsave(&ioc->fw_event_lock, flags);
	if (!list_empty(&ioc->fw_event_list)) {
		fw_event = list_first_entry(&ioc->fw_event_list,
					    struct leapioraid_fw_event_work, list);
		list_del_init(&fw_event->list);
		leapioraid_fw_event_work_put(fw_event);
	}
	spin_unlock_irqrestore(&ioc->fw_event_lock, flags);
	return fw_event;
}

static void
leapioraid_scsihost_fw_event_cleanup_queue(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_fw_event_work *fw_event;
	bool rc = false;

	if ((list_empty(&ioc->fw_event_list) && !ioc->current_event) ||
	    !ioc->firmware_event_thread || in_interrupt())
		return;

	ioc->fw_events_cleanup = 1;
	if (ioc->shost_recovery && ioc->current_event)
		ioc->current_event->ignore = 1;
	while ((fw_event = dequeue_next_fw_event(ioc)) ||
	       (fw_event = ioc->current_event)) {
		if (fw_event == ioc->current_event &&
		    ioc->current_event->event !=
		    LEAPIORAID_REMOVE_UNRESPONDING_DEVICES) {
			ioc->current_event = NULL;
			continue;
		}
		if (fw_event->event == LEAPIORAID_PORT_ENABLE_COMPLETE) {
			ioc->port_enable_cmds.status |= LEAPIORAID_CMD_RESET;
			ioc->start_scan = 0;
		}
		if (fw_event->delayed_work_active)
			rc = cancel_delayed_work_sync(&fw_event->delayed_work);
		else
			rc = cancel_work_sync(&fw_event->work);
		if (rc)
			leapioraid_fw_event_work_put(fw_event);
	}
	ioc->fw_events_cleanup = 0;
}

static void
leapioraid_scsihost_internal_device_block(
	struct scsi_device *sdev,
	struct LEAPIORAID_DEVICE
	*sas_device_priv_data)
{
	int r = 0;

	sdev_printk(KERN_INFO, sdev, "device_block, handle(0x%04x)\n",
		    sas_device_priv_data->sas_target->handle);
	sas_device_priv_data->block = 1;

	r = scsi_internal_device_block_nowait(sdev);
	if (r == -EINVAL)
		sdev_printk(KERN_WARNING, sdev,
			"device_block failed with return(%d) for handle(0x%04x)\n",
			r, sas_device_priv_data->sas_target->handle);
}

static void
leapioraid_scsihost_internal_device_unblock(struct scsi_device *sdev,
				  struct LEAPIORAID_DEVICE
				  *sas_device_priv_data)
{
	int r = 0;

	sdev_printk(KERN_WARNING, sdev,
		"device_unblock and setting to running, handle(0x%04x)\n",
		sas_device_priv_data->sas_target->handle);
	sas_device_priv_data->block = 0;

	r = scsi_internal_device_unblock_nowait(sdev, SDEV_RUNNING);
	if (r == -EINVAL) {
		sdev_printk(KERN_WARNING, sdev,
			"device_unblock failed with return(%d)\n\t\t"
			"for handle(0x%04x) performing a block followed by an unblock\n",
				r,
				sas_device_priv_data->sas_target->handle);
		sas_device_priv_data->block = 1;
		r = scsi_internal_device_block_nowait(sdev);
		if (r)
			sdev_printk(KERN_WARNING, sdev,
				"retried device_block failed with return(%d)\n\t\t"
				"for handle(0x%04x)\n",
					r,
					sas_device_priv_data->sas_target->handle);
		sas_device_priv_data->block = 0;

		r = scsi_internal_device_unblock_nowait(sdev, SDEV_RUNNING);
		if (r)
			sdev_printk(KERN_WARNING, sdev,
				"retried device_unblock failed\n\t\t"
				"with return(%d) for handle(0x%04x)\n",
					r,
					sas_device_priv_data->sas_target->handle);
	}
}

static void
leapioraid_scsihost_ublock_io_all_device(
	struct LEAPIORAID_ADAPTER *ioc, u8 no_turs)
{
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct LEAPIORAID_TARGET *sas_target;
	enum device_responsive_state rc;
	struct scsi_device *sdev;
	struct leapioraid_sas_device *sas_device = NULL;
	int count;
	u8 tr_timeout = 30;
	u8 tr_method = 0;

	shost_for_each_device(sdev, ioc->shost) {
		sas_device_priv_data = sdev->hostdata;
		if (!sas_device_priv_data)
			continue;
		sas_target = sas_device_priv_data->sas_target;
		if (!sas_target || sas_target->deleted)
			continue;
		if (!sas_device_priv_data->block)
			continue;
		count = 0;
		if (no_turs) {
			sdev_printk(KERN_WARNING, sdev,
				"device_unblocked, handle(0x%04x)\n",
				sas_device_priv_data->sas_target->handle);
			leapioraid_scsihost_internal_device_unblock(sdev,
							  sas_device_priv_data);
			continue;
		}
		do {
			rc = leapioraid_scsihost_wait_for_device_to_become_ready(
				ioc,
				sas_target->handle,
				0,
				(sas_target->flags
					& LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT),
				sdev->lun,
				tr_timeout,
				tr_method);
			if (rc == DEVICE_RETRY || rc == DEVICE_START_UNIT
			    || rc == DEVICE_STOP_UNIT || rc == DEVICE_RETRY_UA)
				ssleep(1);
		} while ((rc == DEVICE_RETRY || rc == DEVICE_START_UNIT ||
			  rc == DEVICE_STOP_UNIT || rc == DEVICE_RETRY_UA)
			 && count++ < 144);
		sas_device_priv_data->block = 0;
		if (rc != DEVICE_READY)
			sas_device_priv_data->deleted = 1;
		leapioraid_scsihost_internal_device_unblock(
			sdev, sas_device_priv_data);
		if (rc != DEVICE_READY) {
			sdev_printk(KERN_WARNING, sdev,
				"%s: device_offlined, handle(0x%04x)\n",
				__func__,
				sas_device_priv_data->sas_target->handle);
			scsi_device_set_state(sdev, SDEV_OFFLINE);
			sas_device = leapioraid_get_sdev_by_addr(ioc,
						sas_device_priv_data->sas_target->sas_address,
						sas_device_priv_data->sas_target->port);
			if (sas_device) {
				leapioraid_scsihost_display_enclosure_chassis_info(
						NULL,
						sas_device,
						sdev,
						NULL);
				leapioraid_sas_device_put(sas_device);
			}
		} else
			sdev_printk(KERN_WARNING, sdev,
				"device_unblocked, handle(0x%04x)\n",
				sas_device_priv_data->sas_target->handle);
	}
}

static void
leapioraid_scsihost_ublock_io_device_wait(
	struct LEAPIORAID_ADAPTER *ioc, u64 sas_address,
	struct leapioraid_hba_port *port)
{
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct LEAPIORAID_TARGET *sas_target;
	enum device_responsive_state rc;
	struct scsi_device *sdev;
	int count, host_reset_completion_count;
	struct leapioraid_sas_device *sas_device;
	u8 tr_timeout = 30;
	u8 tr_method = 0;

	shost_for_each_device(sdev, ioc->shost) {
		sas_device_priv_data = sdev->hostdata;
		if (!sas_device_priv_data)
			continue;
		sas_target = sas_device_priv_data->sas_target;
		if (!sas_target)
			continue;
		if (sas_target->sas_address != sas_address ||
		    sas_target->port != port)
			continue;
		if (sdev->sdev_state == SDEV_OFFLINE) {
			sas_device_priv_data->block = 1;
			sas_device_priv_data->deleted = 0;
			scsi_device_set_state(sdev, SDEV_RUNNING);
			scsi_internal_device_block_nowait(sdev);
		}
	}
	shost_for_each_device(sdev, ioc->shost) {
		sas_device_priv_data = sdev->hostdata;
		if (!sas_device_priv_data)
			continue;
		sas_target = sas_device_priv_data->sas_target;
		if (!sas_target)
			continue;
		if (sas_target->sas_address != sas_address ||
		    sas_target->port != port)
			continue;
		if (!sas_device_priv_data->block)
			continue;
		count = 0;
		do {
			host_reset_completion_count = 0;
			rc = leapioraid_scsihost_wait_for_device_to_become_ready(
				ioc,
				sas_target->handle,
				0,
				(sas_target->flags & LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT),
				sdev->lun,
				tr_timeout,
				tr_method);
			if (rc == DEVICE_RETRY || rc == DEVICE_START_UNIT
			    || rc == DEVICE_STOP_UNIT
			    || rc == DEVICE_RETRY_UA) {
				do {
					msleep(500);
					host_reset_completion_count++;
				} while (rc == DEVICE_RETRY &&
					 ioc->shost_recovery);
				if (host_reset_completion_count > 1) {
					rc = leapioraid_scsihost_wait_for_device_to_become_ready(
						ioc, sas_target->handle, 0,
						(sas_target->flags
						& LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT),
						sdev->lun, tr_timeout, tr_method);
					if (rc == DEVICE_RETRY
					    || rc == DEVICE_START_UNIT
					    || rc == DEVICE_STOP_UNIT
					    || rc == DEVICE_RETRY_UA)
						msleep(500);
				}
				continue;
			}
		} while ((rc == DEVICE_RETRY || rc == DEVICE_START_UNIT ||
			  rc == DEVICE_STOP_UNIT || rc == DEVICE_RETRY_UA)
			 && count++ <= 144);
		sas_device_priv_data->block = 0;
		if (rc != DEVICE_READY)
			sas_device_priv_data->deleted = 1;

		scsi_internal_device_unblock_nowait(sdev, SDEV_RUNNING);

		if (rc != DEVICE_READY) {
			sdev_printk(KERN_WARNING, sdev,
				    "%s: device_offlined, handle(0x%04x)\n",
				    __func__,
				    sas_device_priv_data->sas_target->handle);
			sas_device =
			    leapioraid_get_sdev_by_handle(ioc,
							  sas_device_priv_data->sas_target->handle);
			if (sas_device) {
				leapioraid_scsihost_display_enclosure_chassis_info(NULL,
									 sas_device,
									 sdev,
									 NULL);
				leapioraid_sas_device_put(sas_device);
			}
			scsi_device_set_state(sdev, SDEV_OFFLINE);
		} else {
			sdev_printk(KERN_WARNING, sdev,
				    "device_unblocked, handle(0x%04x)\n",
				    sas_device_priv_data->sas_target->handle);
		}
	}
}

static void
leapioraid_scsihost_ublock_io_device(
		struct LEAPIORAID_ADAPTER *ioc, u64 sas_address,
		struct leapioraid_hba_port *port)
{
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct scsi_device *sdev;

	shost_for_each_device(sdev, ioc->shost) {
		sas_device_priv_data = sdev->hostdata;
		if (!sas_device_priv_data || !sas_device_priv_data->sas_target)
			continue;
		if (sas_device_priv_data->sas_target->sas_address
		    != sas_address ||
		    sas_device_priv_data->sas_target->port != port)
			continue;
		if (sas_device_priv_data->block) {
			leapioraid_scsihost_internal_device_unblock(sdev,
							  sas_device_priv_data);
		}
		scsi_device_set_state(sdev, SDEV_OFFLINE);
	}
}

static void leapioraid_scsihost_block_io_all_device(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct scsi_device *sdev;

	shost_for_each_device(sdev, ioc->shost) {
		sas_device_priv_data = sdev->hostdata;
		if (!sas_device_priv_data)
			continue;
		if (sas_device_priv_data->block)
			continue;
		if (sas_device_priv_data->ignore_delay_remove) {
			sdev_printk(KERN_INFO, sdev,
				    "%s skip device_block for SES handle(0x%04x)\n",
				    __func__,
				    sas_device_priv_data->sas_target->handle);
			continue;
		}
		leapioraid_scsihost_internal_device_block(
			sdev, sas_device_priv_data);
	}
}

static void
leapioraid_scsihost_block_io_device(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct scsi_device *sdev;
	struct leapioraid_sas_device *sas_device;

	sas_device = leapioraid_get_sdev_by_handle(ioc, handle);
	shost_for_each_device(sdev, ioc->shost) {
		sas_device_priv_data = sdev->hostdata;
		if (!sas_device_priv_data)
			continue;
		if (sas_device_priv_data->sas_target->handle != handle)
			continue;
		if (sas_device_priv_data->block)
			continue;
		if (sas_device && sas_device->pend_sas_rphy_add)
			continue;
		if (sas_device_priv_data->ignore_delay_remove) {
			sdev_printk(KERN_INFO, sdev,
				    "%s skip device_block for SES handle(0x%04x)\n",
				    __func__,
				    sas_device_priv_data->sas_target->handle);
			continue;
		}
		leapioraid_scsihost_internal_device_block(
			sdev, sas_device_priv_data);
	}
	if (sas_device)
		leapioraid_sas_device_put(sas_device);
}

static void
leapioraid_scsihost_block_io_to_children_attached_to_ex(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_raid_sas_node *sas_expander)
{
	struct leapioraid_sas_port *leapioraid_port;
	struct leapioraid_sas_device *sas_device;
	struct leapioraid_raid_sas_node *expander_sibling;
	unsigned long flags;

	if (!sas_expander)
		return;
	list_for_each_entry(leapioraid_port,
			    &sas_expander->sas_port_list, port_list) {
		if (leapioraid_port->remote_identify.device_type ==
		    SAS_END_DEVICE) {
			spin_lock_irqsave(&ioc->sas_device_lock, flags);
			sas_device = __leapioraid_get_sdev_by_addr(ioc,
					leapioraid_port->remote_identify.sas_address,
					leapioraid_port->hba_port);
			if (sas_device) {
				set_bit(sas_device->handle,
					ioc->blocking_handles);
				leapioraid_sas_device_put(sas_device);
			}
			spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
		}
	}
	list_for_each_entry(leapioraid_port,
			    &sas_expander->sas_port_list, port_list) {
		if (leapioraid_port->remote_identify.device_type ==
		    SAS_EDGE_EXPANDER_DEVICE ||
		    leapioraid_port->remote_identify.device_type ==
		    SAS_FANOUT_EXPANDER_DEVICE) {
			expander_sibling =
			    leapioraid_scsihost_expander_find_by_sas_address
			    (ioc, leapioraid_port->remote_identify.sas_address,
			     leapioraid_port->hba_port);
			leapioraid_scsihost_block_io_to_children_attached_to_ex(
				ioc, expander_sibling);
		}
	}
}

static void
leapioraid_scsihost_block_io_to_children_attached_directly(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidEventDataSasTopoChangeList_t *event_data)
{
	int i;
	u16 handle;
	u16 reason_code;

	for (i = 0; i < event_data->NumEntries; i++) {
		handle = le16_to_cpu(event_data->PHY[i].AttachedDevHandle);
		if (!handle)
			continue;
		reason_code = event_data->PHY[i].PhyStatus &
		    LEAPIORAID_EVENT_SAS_TOPO_RC_MASK;
		if (reason_code ==
		    LEAPIORAID_EVENT_SAS_TOPO_RC_DELAY_NOT_RESPONDING)
			leapioraid_scsihost_block_io_device(ioc, handle);
	}
}

static void
leapioraid_scsihost_tm_tr_send(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct LeapioraidSCSITmgReq_t *mpi_request;
	u16 smid;
	struct leapioraid_sas_device *sas_device = NULL;
	struct LEAPIORAID_TARGET *sas_target_priv_data = NULL;
	u64 sas_address = 0;
	unsigned long flags;
	struct leapioraid_tr_list *delayed_tr;
	u32 ioc_state;
	struct leapioraid_hba_port *port = NULL;
	u8 tr_method = 0;

	if (ioc->pci_error_recovery) {
		dewtprintk(ioc, pr_info(
			"%s %s: host in pci error recovery: handle(0x%04x)\n",
			__func__, ioc->name, handle));
		return;
	}
	ioc_state = leapioraid_base_get_iocstate(ioc, 1);
	if (ioc_state != LEAPIORAID_IOC_STATE_OPERATIONAL) {
		dewtprintk(ioc, pr_info(
			"%s %s: host is not operational: handle(0x%04x)\n",
			__func__, ioc->name, handle));
		return;
	}
	if (test_bit(handle, ioc->pd_handles))
		return;
	clear_bit(handle, ioc->pend_os_device_add);
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_device = __leapioraid_get_sdev_by_handle(ioc, handle);
	if (sas_device && sas_device->starget && sas_device->starget->hostdata) {
		sas_target_priv_data = sas_device->starget->hostdata;
		sas_target_priv_data->deleted = 1;
		sas_address = sas_device->sas_address;
		port = sas_device->port;
	}
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	if (!sas_device)
		tr_method = LEAPIORAID_SCSITASKMGMT_MSGFLAGS_LINK_RESET;

	if (sas_target_priv_data) {
		dewtprintk(ioc, pr_err(
			"%s %s: setting delete flag: handle(0x%04x), sas_addr(0x%016llx)\n",
			ioc->name, __func__, handle,
			(unsigned long long)sas_address));
		if (sas_device) {
			dewtprintk(ioc,
				   leapioraid_scsihost_display_enclosure_chassis_info(
							ioc,
							sas_device,
							NULL,
							NULL));
		}
		leapioraid_scsihost_ublock_io_device(ioc, sas_address, port);
		sas_target_priv_data->handle =
		    LEAPIORAID_INVALID_DEVICE_HANDLE;
	}
	smid = leapioraid_base_get_smid_hpr(ioc, ioc->tm_tr_cb_idx);
	if (!smid) {
		delayed_tr = kzalloc(sizeof(*delayed_tr), GFP_ATOMIC);
		if (!delayed_tr)
			goto out;
		INIT_LIST_HEAD(&delayed_tr->list);
		delayed_tr->handle = handle;
		list_add_tail(&delayed_tr->list, &ioc->delayed_tr_list);
		dewtprintk(ioc, pr_err(
				       "%s DELAYED:tr:handle(0x%04x), (open)\n",
				       ioc->name, handle));
		goto out;
	}
	dewtprintk(ioc, pr_info(
		"%s tr_send:handle(0x%04x), (open), smid(%d), cb(%d)\n",
		ioc->name, handle,
		smid, ioc->tm_tr_cb_idx));
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	memset(mpi_request, 0, sizeof(struct LeapioraidSCSITmgReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_SCSI_TASK_MGMT;
	mpi_request->DevHandle = cpu_to_le16(handle);
	mpi_request->TaskType = LEAPIORAID_SCSITASKMGMT_TASKTYPE_TARGET_RESET;
	mpi_request->MsgFlags = tr_method;
	set_bit(handle, ioc->device_remove_in_progress);
	ioc->put_smid_hi_priority(ioc, smid, 0);
out:
	if (sas_device)
		leapioraid_sas_device_put(sas_device);
}

static u8
leapioraid_scsihost_tm_tr_complete(
		struct LEAPIORAID_ADAPTER *ioc, u16 smid,
		u8 msix_index, u32 reply)
{
	u16 handle;
	struct LeapioraidSCSITmgReq_t *mpi_request_tm;
	struct LeapioraidSCSITmgRep_t *mpi_reply =
	    leapioraid_base_get_reply_virt_addr(ioc, reply);
	struct LeapioraidSasIoUnitControlReq_t *mpi_request;
	u16 smid_sas_ctrl;
	u32 ioc_state;
	struct leapioraid_sc_list *delayed_sc;

	if (ioc->pci_error_recovery) {
		dewtprintk(ioc, pr_info(
			"%s %s: host in pci error recovery\n", __func__,
			ioc->name));
		return 1;
	}
	ioc_state = leapioraid_base_get_iocstate(ioc, 1);
	if (ioc_state != LEAPIORAID_IOC_STATE_OPERATIONAL) {
		dewtprintk(ioc, pr_info(
			"%s %s: host is not operational\n", __func__, ioc->name));
		return 1;
	}
	if (unlikely(!mpi_reply)) {
		pr_err(
		       "%s mpi_reply not valid at %s:%d/%s()!\n", ioc->name,
		       __FILE__, __LINE__, __func__);
		return 1;
	}
	mpi_request_tm = leapioraid_base_get_msg_frame(ioc, smid);
	handle = le16_to_cpu(mpi_request_tm->DevHandle);
	if (handle != le16_to_cpu(mpi_reply->DevHandle)) {
		dewtprintk(ioc, pr_err(
			"%s spurious interrupt: handle(0x%04x:0x%04x), smid(%d)!!!\n",
			ioc->name, handle,
			le16_to_cpu(mpi_reply->DevHandle), smid));
		return 0;
	}
	dewtprintk(ioc, pr_err(
		"%s tr_complete: handle(0x%04x), (open) smid(%d),\n\t\t"
			"ioc_status(0x%04x), loginfo(0x%08x), completed(%d)\n",
				ioc->name,
				handle,
				smid,
				le16_to_cpu(mpi_reply->IOCStatus),
				le32_to_cpu(mpi_reply->IOCLogInfo),
				le32_to_cpu(mpi_reply->TerminationCount)));
	smid_sas_ctrl =
	    leapioraid_base_get_smid(ioc, ioc->tm_sas_control_cb_idx);
	if (!smid_sas_ctrl) {
		delayed_sc = kzalloc(sizeof(*delayed_sc), GFP_ATOMIC);
		if (!delayed_sc)
			return leapioraid_scsihost_check_for_pending_tm(ioc, smid);
		INIT_LIST_HEAD(&delayed_sc->list);
		delayed_sc->handle = le16_to_cpu(mpi_request_tm->DevHandle);
		list_add_tail(&delayed_sc->list, &ioc->delayed_sc_list);
		dewtprintk(ioc, pr_err(
			"%s DELAYED:sc:handle(0x%04x), (open)\n",
			ioc->name, handle));
		return leapioraid_scsihost_check_for_pending_tm(ioc, smid);
	}
	dewtprintk(ioc, pr_info(
		"%s sc_send:handle(0x%04x), (open), smid(%d), cb(%d)\n",
		ioc->name, handle,
		smid_sas_ctrl, ioc->tm_sas_control_cb_idx));
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid_sas_ctrl);
	memset(mpi_request, 0, sizeof(struct LeapioraidIoUnitControlReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_IO_UNIT_CONTROL;
	mpi_request->Operation = LEAPIORAID_CTRL_OP_REMOVE_DEVICE;
	mpi_request->DevHandle = mpi_request_tm->DevHandle;
	ioc->put_smid_default(ioc, smid_sas_ctrl);
	return leapioraid_scsihost_check_for_pending_tm(ioc, smid);
}

inline bool
leapioraid_scsihost_allow_scmd_to_device(
	struct LEAPIORAID_ADAPTER *ioc,
	struct scsi_cmnd *scmd)
{
	if (ioc->pci_error_recovery)
		return false;
	if (ioc->adapter_over_temp)
		return false;
	if (ioc->remove_host) {
		if (leapioraid_base_pci_device_is_unplugged(ioc))
			return false;
		switch (scmd->cmnd[0]) {
		case SYNCHRONIZE_CACHE:
		case START_STOP:
			return true;
		default:
			return false;
		}
	}
	return true;
}

static u8
leapioraid_scsihost_sas_control_complete(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid,
	u8 msix_index, u32 reply)
{
	struct LeapioraidDefaultRep_t *mpi_reply =
	    leapioraid_base_get_reply_virt_addr(ioc, reply);
	u16 dev_handle;

	if (likely(mpi_reply)) {
		dev_handle
			= ((struct LeapioraidIoUnitControlRep_t *)mpi_reply)->DevHandle;
		dewtprintk(ioc, pr_err(
			"%s sc_complete:handle(0x%04x), (open) smid(%d),\n\t\t"
				"ioc_status(0x%04x), loginfo(0x%08x)\n",
					ioc->name,
					le16_to_cpu(dev_handle),
					smid,
					le16_to_cpu(mpi_reply->IOCStatus),
					le32_to_cpu(mpi_reply->IOCLogInfo)));
		if (le16_to_cpu(mpi_reply->IOCStatus) ==
		    LEAPIORAID_IOCSTATUS_SUCCESS) {
			clear_bit(le16_to_cpu(dev_handle),
				  ioc->device_remove_in_progress);
			ioc->tm_tr_retry[le16_to_cpu(dev_handle)] = 0;
		} else if (ioc->tm_tr_retry[le16_to_cpu(dev_handle)] < 3) {
			dewtprintk(ioc, pr_err(
				"%s re-initiating tm_tr_send:handle(0x%04x)\n",
				ioc->name,
				le16_to_cpu(dev_handle)));
			ioc->tm_tr_retry[le16_to_cpu(dev_handle)]++;
			leapioraid_scsihost_tm_tr_send(ioc, le16_to_cpu(dev_handle));
		} else {
			dewtprintk(ioc, pr_err(
				"%s Exiting out of tm_tr_send retries:handle(0x%04x)\n",
				ioc->name,
				le16_to_cpu(dev_handle)));
			ioc->tm_tr_retry[le16_to_cpu(dev_handle)] = 0;
			clear_bit(le16_to_cpu(dev_handle),
				  ioc->device_remove_in_progress);
		}
	} else {
		pr_err(
		       "%s mpi_reply not valid at %s:%d/%s()!\n", ioc->name,
		       __FILE__, __LINE__, __func__);
	}
	return leapioraid_check_for_pending_internal_cmds(ioc, smid);
}

static void
leapioraid_scsihost_tm_tr_volume_send(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct LeapioraidSCSITmgReq_t *mpi_request;
	u16 smid;
	struct leapioraid_tr_list *delayed_tr;

	if (ioc->pci_error_recovery) {
		dewtprintk(ioc, pr_info(
			"%s %s: host reset in progress!\n", __func__, ioc->name));
		return;
	}
	smid = leapioraid_base_get_smid_hpr(ioc, ioc->tm_tr_volume_cb_idx);
	if (!smid) {
		delayed_tr = kzalloc(sizeof(*delayed_tr), GFP_ATOMIC);
		if (!delayed_tr)
			return;
		INIT_LIST_HEAD(&delayed_tr->list);
		delayed_tr->handle = handle;
		list_add_tail(&delayed_tr->list, &ioc->delayed_tr_volume_list);
		dewtprintk(ioc, pr_err(
				"%s DELAYED:tr:handle(0x%04x), (open)\n",
				ioc->name, handle));
		return;
	}
	dewtprintk(ioc, pr_info(
		"%s tr_send:handle(0x%04x), (open), smid(%d), cb(%d)\n",
		ioc->name, handle,
		smid, ioc->tm_tr_volume_cb_idx));
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	memset(mpi_request, 0, sizeof(struct LeapioraidSCSITmgReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_SCSI_TASK_MGMT;
	mpi_request->DevHandle = cpu_to_le16(handle);
	mpi_request->TaskType = LEAPIORAID_SCSITASKMGMT_TASKTYPE_TARGET_RESET;
	ioc->put_smid_hi_priority(ioc, smid, 0);
}

static u8
leapioraid_scsihost_tm_volume_tr_complete(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid,
	u8 msix_index, u32 reply)
{
	u16 handle;
	struct LeapioraidSCSITmgReq_t *mpi_request_tm;
	struct LeapioraidSCSITmgRep_t *mpi_reply =
	    leapioraid_base_get_reply_virt_addr(ioc, reply);

	if (ioc->shost_recovery || ioc->pci_error_recovery) {
		dewtprintk(ioc, pr_info(
			"%s %s: host reset in progress!\n", __func__, ioc->name));
		return 1;
	}
	if (unlikely(!mpi_reply)) {
		pr_err(
			"%s mpi_reply not valid at %s:%d/%s()!\n", ioc->name,
			__FILE__, __LINE__, __func__);
		return 1;
	}
	mpi_request_tm = leapioraid_base_get_msg_frame(ioc, smid);
	handle = le16_to_cpu(mpi_request_tm->DevHandle);
	if (handle != le16_to_cpu(mpi_reply->DevHandle)) {
		dewtprintk(ioc, pr_err(
			"%s spurious interrupt: handle(0x%04x:0x%04x), smid(%d)!!!\n",
			ioc->name, handle,
			le16_to_cpu(mpi_reply->DevHandle), smid));
		return 0;
	}
	dewtprintk(ioc, pr_err(
		"%s tr_complete:handle(0x%04x), (open) smid(%d),\n\t\t"
			"ioc_status(0x%04x), loginfo(0x%08x), completed(%d)\n",
				ioc->name,
				handle,
				smid,
				le16_to_cpu(mpi_reply->IOCStatus),
				le32_to_cpu(mpi_reply->IOCLogInfo),
				le32_to_cpu(mpi_reply->TerminationCount)));
	return leapioraid_scsihost_check_for_pending_tm(ioc, smid);
}

static void
leapioraid_scsihost_tm_internal_tr_send(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct leapioraid_tr_list *delayed_tr;
	struct LeapioraidSCSITmgReq_t *mpi_request;
	u16 smid;
	u8 tr_method = LEAPIORAID_SCSITASKMGMT_MSGFLAGS_LINK_RESET;

	smid = leapioraid_base_get_smid_hpr(ioc, ioc->tm_tr_internal_cb_idx);
	if (!smid) {
		delayed_tr = kzalloc(sizeof(*delayed_tr), GFP_ATOMIC);
		if (!delayed_tr)
			return;
		INIT_LIST_HEAD(&delayed_tr->list);
		delayed_tr->handle = handle;
		list_add_tail(&delayed_tr->list,
			      &ioc->delayed_internal_tm_list);
		dewtprintk(ioc,
			   pr_err(
				  "%s DELAYED:tr:handle(0x%04x), (open)\n",
				  ioc->name, handle));
		return;
	}
	dewtprintk(ioc, pr_info(
		"%s tr_send:handle(0x%04x), (open), smid(%d), cb(%d)\n",
		ioc->name, handle,
		smid, ioc->tm_tr_internal_cb_idx));
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	memset(mpi_request, 0, sizeof(struct LeapioraidSCSITmgReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_SCSI_TASK_MGMT;
	mpi_request->DevHandle = cpu_to_le16(handle);
	mpi_request->TaskType = LEAPIORAID_SCSITASKMGMT_TASKTYPE_TARGET_RESET;
	mpi_request->MsgFlags = tr_method;
	ioc->put_smid_hi_priority(ioc, smid, 0);
}

static u8
leapioraid_scsihost_tm_internal_tr_complete(
		struct LEAPIORAID_ADAPTER *ioc, u16 smid,
		u8 msix_index, u32 reply)
{
	struct LeapioraidSCSITmgRep_t *mpi_reply =
	    leapioraid_base_get_reply_virt_addr(ioc, reply);

	if (likely(mpi_reply)) {
		dewtprintk(ioc, pr_err(
			"%s tr_complete:handle(0x%04x),\n\t\t"
				"(open) smid(%d), ioc_status(0x%04x), loginfo(0x%08x)\n",
				ioc->name,
				le16_to_cpu(mpi_reply->DevHandle),
				smid,
				le16_to_cpu(mpi_reply->IOCStatus),
				le32_to_cpu(mpi_reply->IOCLogInfo)));
	} else {
		pr_err("%s mpi_reply not valid at %s:%d/%s()!\n", ioc->name,
		       __FILE__, __LINE__, __func__);
		return 1;
	}
	return leapioraid_scsihost_check_for_pending_tm(ioc, smid);
}

static void
leapioraid_scsihost_issue_delayed_event_ack(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid,
	U16 event, U32 event_context)
{
	struct LeapioraidEventAckReq_t *ack_request;
	int i = smid - ioc->internal_smid;
	unsigned long flags;

	spin_lock_irqsave(&ioc->scsi_lookup_lock, flags);
	ioc->internal_lookup[i].cb_idx = ioc->base_cb_idx;
	spin_unlock_irqrestore(&ioc->scsi_lookup_lock, flags);
	dewtprintk(ioc, pr_info(
		"%s EVENT ACK: event(0x%04x), smid(%d), cb(%d)\n",
		ioc->name, le16_to_cpu(event),
		smid, ioc->base_cb_idx));
	ack_request = leapioraid_base_get_msg_frame(ioc, smid);
	memset(ack_request, 0, sizeof(struct LeapioraidEventAckReq_t));
	ack_request->Function = LEAPIORAID_FUNC_EVENT_ACK;
	ack_request->Event = event;
	ack_request->EventContext = event_context;
	ack_request->VF_ID = 0;
	ack_request->VP_ID = 0;
	ioc->put_smid_default(ioc, smid);
}

static void
leapioraid_scsihost_issue_delayed_sas_io_unit_ctrl(
	struct LEAPIORAID_ADAPTER *ioc,
	u16 smid, u16 handle)
{
	struct LeapioraidSasIoUnitControlReq_t *mpi_request;
	u32 ioc_state;
	int i = smid - ioc->internal_smid;
	unsigned long flags;

	if (ioc->remove_host) {
		dewtprintk(ioc, pr_info(
			"%s %s: host has been removed\n", __func__, ioc->name));
		return;
	} else if (ioc->pci_error_recovery) {
		dewtprintk(ioc, pr_info(
			"%s %s: host in pci error recovery\n", __func__,
			ioc->name));
		return;
	}
	ioc_state = leapioraid_base_get_iocstate(ioc, 1);
	if (ioc_state != LEAPIORAID_IOC_STATE_OPERATIONAL) {
		dewtprintk(ioc, pr_info(
			"%s %s: host is not operational\n", __func__, ioc->name));
		return;
	}
	spin_lock_irqsave(&ioc->scsi_lookup_lock, flags);
	ioc->internal_lookup[i].cb_idx = ioc->tm_sas_control_cb_idx;
	spin_unlock_irqrestore(&ioc->scsi_lookup_lock, flags);
	dewtprintk(ioc, pr_info(
		"%s sc_send:handle(0x%04x), (open), smid(%d), cb(%d)\n",
		ioc->name, handle,
		smid, ioc->tm_sas_control_cb_idx));
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	memset(mpi_request, 0, sizeof(struct LeapioraidIoUnitControlReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_IO_UNIT_CONTROL;
	mpi_request->Operation = 0x0D;
	mpi_request->DevHandle = cpu_to_le16(handle);
	ioc->put_smid_default(ioc, smid);
}

u8
leapioraid_check_for_pending_internal_cmds(struct LEAPIORAID_ADAPTER *ioc,
					   u16 smid)
{
	struct leapioraid_sc_list *delayed_sc;
	struct leapioraid_event_ack_list *delayed_event_ack;

	if (!list_empty(&ioc->delayed_event_ack_list)) {
		delayed_event_ack = list_entry(ioc->delayed_event_ack_list.next,
					       struct leapioraid_event_ack_list, list);
		leapioraid_scsihost_issue_delayed_event_ack(ioc, smid,
						  delayed_event_ack->Event,
						  delayed_event_ack->EventContext);
		list_del(&delayed_event_ack->list);
		kfree(delayed_event_ack);
		return 0;
	}
	if (!list_empty(&ioc->delayed_sc_list)) {
		delayed_sc = list_entry(ioc->delayed_sc_list.next,
					struct leapioraid_sc_list, list);
		leapioraid_scsihost_issue_delayed_sas_io_unit_ctrl(ioc, smid,
							 delayed_sc->handle);
		list_del(&delayed_sc->list);
		kfree(delayed_sc);
		return 0;
	}
	return 1;
}

static u8
leapioraid_scsihost_check_for_pending_tm(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid)
{
	struct leapioraid_tr_list *delayed_tr;

	if (!list_empty(&ioc->delayed_tr_volume_list)) {
		delayed_tr = list_entry(ioc->delayed_tr_volume_list.next,
					struct leapioraid_tr_list, list);
		leapioraid_base_free_smid(ioc, smid);
		leapioraid_scsihost_tm_tr_volume_send(ioc, delayed_tr->handle);
		list_del(&delayed_tr->list);
		kfree(delayed_tr);
		return 0;
	}
	if (!list_empty(&ioc->delayed_tr_list)) {
		delayed_tr = list_entry(ioc->delayed_tr_list.next,
					struct leapioraid_tr_list, list);
		leapioraid_base_free_smid(ioc, smid);
		leapioraid_scsihost_tm_tr_send(ioc, delayed_tr->handle);
		list_del(&delayed_tr->list);
		kfree(delayed_tr);
		return 0;
	}
	if (!list_empty(&ioc->delayed_internal_tm_list)) {
		delayed_tr = list_entry(ioc->delayed_internal_tm_list.next,
					struct leapioraid_tr_list, list);
		leapioraid_base_free_smid(ioc, smid);
		leapioraid_scsihost_tm_internal_tr_send(
			ioc, delayed_tr->handle);
		list_del(&delayed_tr->list);
		kfree(delayed_tr);
		return 0;
	}
	return 1;
}

static void
leapioraid_scsihost_check_topo_delete_events(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidEventDataSasTopoChangeList_t *event_data)
{
	struct leapioraid_fw_event_work *fw_event;
	struct LeapioraidEventDataSasTopoChangeList_t *local_event_data;
	u16 expander_handle;
	struct leapioraid_raid_sas_node *sas_expander;
	unsigned long flags;
	int i, reason_code;
	u16 handle;

	for (i = 0; i < event_data->NumEntries; i++) {
		handle = le16_to_cpu(event_data->PHY[i].AttachedDevHandle);
		if (!handle)
			continue;
		reason_code = event_data->PHY[i].PhyStatus &
		    LEAPIORAID_EVENT_SAS_TOPO_RC_MASK;
		if (reason_code ==
		    LEAPIORAID_EVENT_SAS_TOPO_RC_TARG_NOT_RESPONDING)
			leapioraid_scsihost_tm_tr_send(ioc, handle);
	}
	expander_handle = le16_to_cpu(event_data->ExpanderDevHandle);
	if (expander_handle < ioc->sas_hba.num_phys) {
		leapioraid_scsihost_block_io_to_children_attached_directly(
			ioc, event_data);
		return;
	}
	if (event_data->ExpStatus ==
	    LEAPIORAID_EVENT_SAS_TOPO_ES_DELAY_NOT_RESPONDING) {
		spin_lock_irqsave(&ioc->sas_node_lock, flags);
		sas_expander = leapioraid_scsihost_expander_find_by_handle(
			ioc, expander_handle);
		leapioraid_scsihost_block_io_to_children_attached_to_ex(
			ioc, sas_expander);
		spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
		do {
			handle = find_first_bit(ioc->blocking_handles,
						ioc->facts.MaxDevHandle);
			if (handle < ioc->facts.MaxDevHandle)
				leapioraid_scsihost_block_io_device(ioc, handle);
		} while (test_and_clear_bit(handle, ioc->blocking_handles));
	} else if (event_data->ExpStatus ==
		   LEAPIORAID_EVENT_SAS_TOPO_ES_RESPONDING)
		leapioraid_scsihost_block_io_to_children_attached_directly(
			ioc, event_data);
	if (event_data->ExpStatus != LEAPIORAID_EVENT_SAS_TOPO_ES_NOT_RESPONDING)
		return;
	spin_lock_irqsave(&ioc->fw_event_lock, flags);
	list_for_each_entry(fw_event, &ioc->fw_event_list, list) {
		if (fw_event->event != LEAPIORAID_EVENT_SAS_TOPOLOGY_CHANGE_LIST ||
		    fw_event->ignore)
			continue;
		local_event_data = fw_event->event_data;
		if (local_event_data->ExpStatus ==
		    LEAPIORAID_EVENT_SAS_TOPO_ES_ADDED ||
		    local_event_data->ExpStatus ==
		    LEAPIORAID_EVENT_SAS_TOPO_ES_RESPONDING) {
			if (le16_to_cpu(local_event_data->ExpanderDevHandle) ==
			    expander_handle) {
				dewtprintk(ioc, pr_err(
					"%s setting ignoring flag\n",
					ioc->name));
				fw_event->ignore = 1;
			}
		}
	}
	spin_unlock_irqrestore(&ioc->fw_event_lock, flags);
}

static void
leapioraid_scsihost_set_volume_delete_flag(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct leapioraid_raid_device *raid_device;
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	unsigned long flags;

	spin_lock_irqsave(&ioc->raid_device_lock, flags);
	raid_device = leapioraid_raid_device_find_by_handle(
		ioc, handle);
	if (raid_device && raid_device->starget &&
	    raid_device->starget->hostdata) {
		sas_target_priv_data = raid_device->starget->hostdata;
		sas_target_priv_data->deleted = 1;
		dewtprintk(ioc, pr_err(
			"%s setting delete flag: handle(0x%04x), wwid(0x%016llx)\n",
			ioc->name, handle,
			(unsigned long long)raid_device->wwid));
	}
	spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
}

static void
leapioraid_scsihost_set_volume_handle_for_tr(
	u16 handle, u16 *a, u16 *b)
{
	if (!handle || handle == *a || handle == *b)
		return;
	if (!*a)
		*a = handle;
	else if (!*b)
		*b = handle;
}

static void
leapioraid_scsihost_check_ir_config_unhide_events(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidEventDataIrCfgChangeList_t *event_data)
{
	struct LeapioraidEventIrCfgEle_t *element;
	int i;
	u16 handle, volume_handle, a, b;
	struct leapioraid_tr_list *delayed_tr;

	a = 0;
	b = 0;
	element =
	    (struct LeapioraidEventIrCfgEle_t *) &event_data->ConfigElement[0];
	for (i = 0; i < event_data->NumElements; i++, element++) {
		if (le32_to_cpu(event_data->Flags) &
		    LEAPIORAID_EVENT_IR_CHANGE_FLAGS_FOREIGN_CONFIG)
			continue;
		if (element->ReasonCode ==
		    LEAPIORAID_EVENT_IR_CHANGE_RC_VOLUME_DELETED ||
		    element->ReasonCode == LEAPIORAID_EVENT_IR_CHANGE_RC_REMOVED) {
			volume_handle = le16_to_cpu(element->VolDevHandle);
			leapioraid_scsihost_set_volume_delete_flag(ioc, volume_handle);
			leapioraid_scsihost_set_volume_handle_for_tr(
				volume_handle, &a, &b);
		}
	}
	element =
	    (struct LeapioraidEventIrCfgEle_t *) &event_data->ConfigElement[0];
	for (i = 0; i < event_data->NumElements; i++, element++) {
		if (le32_to_cpu(event_data->Flags) &
		    LEAPIORAID_EVENT_IR_CHANGE_FLAGS_FOREIGN_CONFIG)
			continue;
		if (element->ReasonCode == LEAPIORAID_EVENT_IR_CHANGE_RC_UNHIDE) {
			volume_handle = le16_to_cpu(element->VolDevHandle);
			leapioraid_scsihost_set_volume_handle_for_tr(
				volume_handle, &a, &b);
		}
	}
	if (a)
		leapioraid_scsihost_tm_tr_volume_send(ioc, a);
	if (b)
		leapioraid_scsihost_tm_tr_volume_send(ioc, b);
	element =
	    (struct LeapioraidEventIrCfgEle_t *) &event_data->ConfigElement[0];
	for (i = 0; i < event_data->NumElements; i++, element++) {
		if (element->ReasonCode != LEAPIORAID_EVENT_IR_CHANGE_RC_UNHIDE)
			continue;
		handle = le16_to_cpu(element->PhysDiskDevHandle);
		volume_handle = le16_to_cpu(element->VolDevHandle);
		clear_bit(handle, ioc->pd_handles);
		if (!volume_handle)
			leapioraid_scsihost_tm_tr_send(ioc, handle);
		else if (volume_handle == a || volume_handle == b) {
			delayed_tr = kzalloc(sizeof(*delayed_tr), GFP_ATOMIC);
			BUG_ON(!delayed_tr);
			INIT_LIST_HEAD(&delayed_tr->list);
			delayed_tr->handle = handle;
			list_add_tail(&delayed_tr->list, &ioc->delayed_tr_list);
			dewtprintk(ioc, pr_err(
				"%s DELAYED:tr:handle(0x%04x), (open)\n",
				ioc->name, handle));
		} else
			leapioraid_scsihost_tm_tr_send(ioc, handle);
	}
}

static void
leapioraid_scsihost_check_volume_delete_events(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidEventDataIrVol_t *event_data)
{
	u32 state;

	if (event_data->ReasonCode != LEAPIORAID_EVENT_IR_VOLUME_RC_STATE_CHANGED)
		return;
	state = le32_to_cpu(event_data->NewValue);
	if (state == LEAPIORAID_RAID_VOL_STATE_MISSING || state ==
	    LEAPIORAID_RAID_VOL_STATE_FAILED)
		leapioraid_scsihost_set_volume_delete_flag(
			ioc, le16_to_cpu(event_data->VolDevHandle));
}

static int
leapioraid_scsihost_set_satl_pending(
	struct scsi_cmnd *scmd, bool pending)
{
	struct LEAPIORAID_DEVICE *priv = scmd->device->hostdata;

	if (scmd->cmnd[0] != ATA_12 && scmd->cmnd[0] != ATA_16)
		return 0;
	if (pending)
		return test_and_set_bit(LEAPIORAID_CMND_PENDING_BIT,
					&priv->ata_command_pending);
	clear_bit(LEAPIORAID_CMND_PENDING_BIT, &priv->ata_command_pending);
	return 0;
}

void
leapioraid_scsihost_flush_running_cmds(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct scsi_cmnd *scmd;
	struct leapioraid_scsiio_tracker *st;
	u16 smid;
	u16 count = 0;

	for (smid = 1; smid <= ioc->shost->can_queue; smid++) {
		scmd = leapioraid_scsihost_scsi_lookup_get(ioc, smid);
		if (!scmd)
			continue;
		count++;
		st = leapioraid_base_scsi_cmd_priv(scmd);
		if (st && st->smid == 0)
			continue;
		leapioraid_scsihost_set_satl_pending(scmd, false);
		leapioraid_base_get_msg_frame(ioc, smid);
		scsi_dma_unmap(scmd);

		leapioraid_base_clear_st(ioc, st);
		if ((!leapioraid_base_pci_device_is_available(ioc)) ||
		    (ioc->ioc_reset_status != 0)
		    || ioc->adapter_over_temp || ioc->remove_host)
			scmd->result = DID_NO_CONNECT << 16;
		else
			scmd->result = DID_RESET << 16;
		scsi_done(scmd);
	}
	dtmprintk(ioc, pr_info("%s completing %d cmds\n",
			      ioc->name, count));
}

static inline u8 scsih_is_io_belongs_to_RT_class(
	struct scsi_cmnd *scmd)
{
	struct request *rq = scsi_cmd_to_rq(scmd);

	return (IOPRIO_PRIO_CLASS(req_get_ioprio(rq)) == IOPRIO_CLASS_RT);
}

static int
leapioraid_scsihost_qcmd(
	struct Scsi_Host *shost, struct scsi_cmnd *scmd)
{
	struct LEAPIORAID_ADAPTER *ioc
		= leapioraid_shost_private(scmd->device->host);
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	struct LeapioraidSCSIIOReq_t *mpi_request;
	u32 mpi_control;
	u16 smid;
	u16 handle;
	int rc = 0;

	if (ioc->logging_level & LEAPIORAID_DEBUG_SCSI)
		scsi_print_command(scmd);
	sas_device_priv_data = scmd->device->hostdata;
	if (!sas_device_priv_data || !sas_device_priv_data->sas_target) {
		scmd->result = DID_NO_CONNECT << 16;
		scsi_done(scmd);
		goto out;
	}
	if (!(leapioraid_scsihost_allow_scmd_to_device(ioc, scmd))) {
		scmd->result = DID_NO_CONNECT << 16;
		scsi_done(scmd);
		goto out;
	}
	sas_target_priv_data = sas_device_priv_data->sas_target;
	handle = sas_target_priv_data->handle;
	if (handle == LEAPIORAID_INVALID_DEVICE_HANDLE) {
		scmd->result = DID_NO_CONNECT << 16;
		scsi_done(scmd);
		goto out;
	}
	if (sas_device_priv_data->block &&
	    scmd->device->host->shost_state == SHOST_RECOVERY &&
	    scmd->cmnd[0] == TEST_UNIT_READY) {
		scsi_build_sense(scmd, 0, UNIT_ATTENTION,
						0x29, 0x07);
		scsi_done(scmd);
		goto out;
	}
	if (ioc->shost_recovery || ioc->ioc_link_reset_in_progress) {
		rc = SCSI_MLQUEUE_HOST_BUSY;
		goto out;
	} else if (sas_target_priv_data->deleted ||
		   sas_device_priv_data->deleted) {
		scmd->result = DID_NO_CONNECT << 16;
		scsi_done(scmd);
		goto out;
	} else if (sas_target_priv_data->tm_busy || sas_device_priv_data->block) {
		rc = SCSI_MLQUEUE_DEVICE_BUSY;
		goto out;
	}
	do {
		if (test_bit(LEAPIORAID_CMND_PENDING_BIT,
			     &sas_device_priv_data->ata_command_pending)) {
			rc = SCSI_MLQUEUE_DEVICE_BUSY;
			goto out;
		}
	} while (leapioraid_scsihost_set_satl_pending(scmd, true));
	if (scmd->sc_data_direction == DMA_FROM_DEVICE)
		mpi_control = LEAPIORAID_SCSIIO_CONTROL_READ;
	else if (scmd->sc_data_direction == DMA_TO_DEVICE)
		mpi_control = LEAPIORAID_SCSIIO_CONTROL_WRITE;
	else
		mpi_control = LEAPIORAID_SCSIIO_CONTROL_NODATATRANSFER;
	mpi_control |= LEAPIORAID_SCSIIO_CONTROL_SIMPLEQ;
	if (sas_device_priv_data->ncq_prio_enable) {
		if (scsih_is_io_belongs_to_RT_class(scmd))
			mpi_control |= 1 << LEAPIORAID_SCSIIO_CONTROL_CMDPRI_SHIFT;
	}
	if ((sas_device_priv_data->flags & LEAPIORAID_DEVICE_TLR_ON) &&
	    scmd->cmd_len != 32)
		mpi_control |= LEAPIORAID_SCSIIO_CONTROL_TLR_ON;
	smid = leapioraid_base_get_smid_scsiio(
		ioc, ioc->scsi_io_cb_idx, scmd);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		rc = SCSI_MLQUEUE_HOST_BUSY;
		leapioraid_scsihost_set_satl_pending(scmd, false);
		goto out;
	}
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	if (scmd->cmd_len == 32)
		mpi_control |= 4 << LEAPIORAID_SCSIIO_CONTROL_ADDCDBLEN_SHIFT;
	mpi_request->Function = LEAPIORAID_FUNC_SCSI_IO_REQUEST;
	if (sas_device_priv_data->sas_target->flags &
	    LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT)
		mpi_request->Function =
		    LEAPIORAID_FUNC_RAID_SCSI_IO_PASSTHROUGH;
	else
		mpi_request->Function = LEAPIORAID_FUNC_SCSI_IO_REQUEST;
	mpi_request->DevHandle = cpu_to_le16(handle);
	mpi_request->DataLength = cpu_to_le32(scsi_bufflen(scmd));
	mpi_request->Control = cpu_to_le32(mpi_control);
	mpi_request->IoFlags = cpu_to_le16(scmd->cmd_len);
	mpi_request->MsgFlags = LEAPIORAID_SCSIIO_MSGFLAGS_SYSTEM_SENSE_ADDR;
	mpi_request->SenseBufferLength = SCSI_SENSE_BUFFERSIZE;
	mpi_request->SenseBufferLowAddress =
	    leapioraid_base_get_sense_buffer_dma(ioc, smid);
	mpi_request->SGLOffset0 = offsetof(struct LeapioraidSCSIIOReq_t, SGL) / 4;
	int_to_scsilun(sas_device_priv_data->lun, (struct scsi_lun *)
		       mpi_request->LUN);
	memcpy(mpi_request->CDB.CDB32, scmd->cmnd, scmd->cmd_len);
	if (mpi_request->DataLength) {
		if (ioc->build_sg_scmd(ioc, scmd, smid)) {
			leapioraid_base_free_smid(ioc, smid);
			rc = SCSI_MLQUEUE_HOST_BUSY;
			leapioraid_scsihost_set_satl_pending(scmd, false);
			goto out;
		}
	} else
		ioc->build_zero_len_sge(ioc, &mpi_request->SGL);
	if (likely(mpi_request->Function == LEAPIORAID_FUNC_SCSI_IO_REQUEST)) {
		if (sas_target_priv_data->flags & LEAPIORAID_TARGET_FASTPATH_IO) {
			mpi_request->IoFlags = cpu_to_le16(scmd->cmd_len | 0x4000);
			ioc->put_smid_fast_path(ioc, smid, handle);
		} else
			ioc->put_smid_scsi_io(ioc, smid,
					      le16_to_cpu(mpi_request->DevHandle));
	} else
		ioc->put_smid_default(ioc, smid);
out:
	return rc;
}

static void
leapioraid_scsihost_normalize_sense(
	char *sense_buffer, struct sense_info *data)
{
	if ((sense_buffer[0] & 0x7F) >= 0x72) {
		data->skey = sense_buffer[1] & 0x0F;
		data->asc = sense_buffer[2];
		data->ascq = sense_buffer[3];
	} else {
		data->skey = sense_buffer[2] & 0x0F;
		data->asc = sense_buffer[12];
		data->ascq = sense_buffer[13];
	}
}

static void
leapioraid_scsihost_scsi_ioc_info(
	struct LEAPIORAID_ADAPTER *ioc, struct scsi_cmnd *scmd,
	struct LeapioraidSCSIIORep_t *mpi_reply, u16 smid,
	u8 scsi_status, u16 error_response_count)
{
	u32 response_info;
	u8 *response_bytes;
	u16 ioc_status = le16_to_cpu(mpi_reply->IOCStatus) &
	    LEAPIORAID_IOCSTATUS_MASK;
	u8 scsi_state = mpi_reply->SCSIState;
	char *desc_ioc_state = NULL;
	char *desc_scsi_status = NULL;
	char *desc_scsi_state = ioc->tmp_string;
	u32 log_info = le32_to_cpu(mpi_reply->IOCLogInfo);
	struct leapioraid_sas_device *sas_device = NULL;
	struct scsi_target *starget = scmd->device->sdev_target;
	struct LEAPIORAID_TARGET *priv_target = starget->hostdata;
	char *device_str = NULL;

	if (!priv_target)
		return;
	if (ioc->warpdrive_msg)
		device_str = "WarpDrive";
	else
		device_str = "volume";
	if (log_info == 0x31170000)
		return;
	switch (ioc_status) {
	case LEAPIORAID_IOCSTATUS_SUCCESS:
		desc_ioc_state = "success";
		break;
	case LEAPIORAID_IOCSTATUS_INVALID_FUNCTION:
		desc_ioc_state = "invalid function";
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_RECOVERED_ERROR:
		desc_ioc_state = "scsi recovered error";
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_INVALID_DEVHANDLE:
		desc_ioc_state = "scsi invalid dev handle";
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_DEVICE_NOT_THERE:
		desc_ioc_state = "scsi device not there";
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_DATA_OVERRUN:
		desc_ioc_state = "scsi data overrun";
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_DATA_UNDERRUN:
		desc_ioc_state = "scsi data underrun";
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_IO_DATA_ERROR:
		desc_ioc_state = "scsi io data error";
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_PROTOCOL_ERROR:
		desc_ioc_state = "scsi protocol error";
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_TASK_TERMINATED:
		desc_ioc_state = "scsi task terminated";
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_RESIDUAL_MISMATCH:
		desc_ioc_state = "scsi residual mismatch";
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_TASK_MGMT_FAILED:
		desc_ioc_state = "scsi task mgmt failed";
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_IOC_TERMINATED:
		desc_ioc_state = "scsi ioc terminated";
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_EXT_TERMINATED:
		desc_ioc_state = "scsi ext terminated";
		break;
	case LEAPIORAID_IOCSTATUS_EEDP_GUARD_ERROR:
		if (!ioc->disable_eedp_support) {
			desc_ioc_state = "eedp guard error";
			break;
		}
		fallthrough;
	case LEAPIORAID_IOCSTATUS_EEDP_REF_TAG_ERROR:
		if (!ioc->disable_eedp_support) {
			desc_ioc_state = "eedp ref tag error";
			break;
		}
		fallthrough;
	case LEAPIORAID_IOCSTATUS_EEDP_APP_TAG_ERROR:
		if (!ioc->disable_eedp_support) {
			desc_ioc_state = "eedp app tag error";
			break;
		}
		fallthrough;
	case LEAPIORAID_IOCSTATUS_INSUFFICIENT_POWER:
		desc_ioc_state = "insufficient power";
		break;
	default:
		desc_ioc_state = "unknown";
		break;
	}
	switch (scsi_status) {
	case LEAPIORAID_SCSI_STATUS_GOOD:
		desc_scsi_status = "good";
		break;
	case LEAPIORAID_SCSI_STATUS_CHECK_CONDITION:
		desc_scsi_status = "check condition";
		break;
	case LEAPIORAID_SCSI_STATUS_CONDITION_MET:
		desc_scsi_status = "condition met";
		break;
	case LEAPIORAID_SCSI_STATUS_BUSY:
		desc_scsi_status = "busy";
		break;
	case LEAPIORAID_SCSI_STATUS_INTERMEDIATE:
		desc_scsi_status = "intermediate";
		break;
	case LEAPIORAID_SCSI_STATUS_INTERMEDIATE_CONDMET:
		desc_scsi_status = "intermediate condmet";
		break;
	case LEAPIORAID_SCSI_STATUS_RESERVATION_CONFLICT:
		desc_scsi_status = "reservation conflict";
		break;
	case LEAPIORAID_SCSI_STATUS_COMMAND_TERMINATED:
		desc_scsi_status = "command terminated";
		break;
	case LEAPIORAID_SCSI_STATUS_TASK_SET_FULL:
		desc_scsi_status = "task set full";
		break;
	case LEAPIORAID_SCSI_STATUS_ACA_ACTIVE:
		desc_scsi_status = "aca active";
		break;
	case LEAPIORAID_SCSI_STATUS_TASK_ABORTED:
		desc_scsi_status = "task aborted";
		break;
	default:
		desc_scsi_status = "unknown";
		break;
	}
	desc_scsi_state[0] = '\0';
	if (!scsi_state)
		desc_scsi_state = " ";
	if (scsi_state & LEAPIORAID_SCSI_STATE_RESPONSE_INFO_VALID)
		strcat(desc_scsi_state, "response info ");
	if (scsi_state & LEAPIORAID_SCSI_STATE_TERMINATED)
		strcat(desc_scsi_state, "state terminated ");
	if (scsi_state & LEAPIORAID_SCSI_STATE_NO_SCSI_STATUS)
		strcat(desc_scsi_state, "no status ");
	if (scsi_state & LEAPIORAID_SCSI_STATE_AUTOSENSE_FAILED)
		strcat(desc_scsi_state, "autosense failed ");
	if (scsi_state & LEAPIORAID_SCSI_STATE_AUTOSENSE_VALID)
		strcat(desc_scsi_state, "autosense valid ");
	scsi_print_command(scmd);
	if (priv_target->flags & LEAPIORAID_TARGET_FLAGS_VOLUME) {
		pr_warn("%s \t%s wwid(0x%016llx)\n",
		       ioc->name, device_str,
		       (unsigned long long)priv_target->sas_address);
	} else {
		sas_device = leapioraid_get_sdev_from_target(ioc, priv_target);
		if (sas_device) {
			pr_warn(
			       "%s \t%s: sas_address(0x%016llx), phy(%d)\n",
			       ioc->name, __func__, (unsigned long long)
			       sas_device->sas_address, sas_device->phy);
			leapioraid_scsihost_display_enclosure_chassis_info(ioc,
								 sas_device,
								 NULL, NULL);
			leapioraid_sas_device_put(sas_device);
		}
	}
	pr_warn(
		"%s \thandle(0x%04x), ioc_status(%s)(0x%04x), smid(%d)\n",
		ioc->name, le16_to_cpu(mpi_reply->DevHandle), desc_ioc_state,
		ioc_status, smid);
	pr_warn("%s \trequest_len(%d), underflow(%d), resid(%d)\n",
		ioc->name, scsi_bufflen(scmd), scmd->underflow,
	    scsi_get_resid(scmd));
	pr_warn("%s \ttag(%d), transfer_count(%d), sc->result(0x%08x)\n",
		ioc->name,
		le16_to_cpu(mpi_reply->TaskTag),
		le32_to_cpu(mpi_reply->TransferCount), scmd->result);
	pr_warn("%s \tscsi_status(%s)(0x%02x), scsi_state(%s)(0x%02x)\n",
		ioc->name, desc_scsi_status,
	    scsi_status, desc_scsi_state, scsi_state);
	if (scsi_state & LEAPIORAID_SCSI_STATE_AUTOSENSE_VALID) {
		struct sense_info data;

		leapioraid_scsihost_normalize_sense(scmd->sense_buffer, &data);
		pr_warn(
			"%s \t[sense_key,asc,ascq]: [0x%02x,0x%02x,0x%02x], count(%d)\n",
			ioc->name,
		    data.skey, data.asc, data.ascq,
		    le32_to_cpu(mpi_reply->SenseCount));
	}
	if (scsi_state & LEAPIORAID_SCSI_STATE_RESPONSE_INFO_VALID) {
		response_info = le32_to_cpu(mpi_reply->ResponseInfo);
		response_bytes = (u8 *) &response_info;
		leapioraid_scsihost_response_code(ioc, response_bytes[0]);
	}
}

static void
leapioraid_scsihost_turn_on_pfa_led(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct LeapioraidSepRep_t mpi_reply;
	struct LeapioraidSepReq_t mpi_request;
	struct leapioraid_sas_device *sas_device;

	sas_device = leapioraid_get_sdev_by_handle(ioc, handle);
	if (!sas_device)
		return;
	memset(&mpi_request, 0, sizeof(struct LeapioraidSepReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_SCSI_ENCLOSURE_PROCESSOR;
	mpi_request.Action = LEAPIORAID_SEP_REQ_ACTION_WRITE_STATUS;
	mpi_request.SlotStatus =
	    cpu_to_le32(LEAPIORAID_SEP_REQ_SLOTSTATUS_PREDICTED_FAULT);
	mpi_request.DevHandle = cpu_to_le16(handle);
	mpi_request.Flags = LEAPIORAID_SEP_REQ_FLAGS_DEVHANDLE_ADDRESS;
	if ((leapioraid_base_scsi_enclosure_processor(ioc, &mpi_reply,
						      &mpi_request)) != 0) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out;
	}
	sas_device->pfa_led_on = 1;
	if (mpi_reply.IOCStatus || mpi_reply.IOCLogInfo) {
		dewtprintk(ioc, pr_info(
			"%s enclosure_processor: ioc_status (0x%04x), loginfo(0x%08x)\n",
			ioc->name, le16_to_cpu(mpi_reply.IOCStatus),
			le32_to_cpu(mpi_reply.IOCLogInfo)));
		goto out;
	}
out:
	leapioraid_sas_device_put(sas_device);
}

static void
leapioraid_scsihost_turn_off_pfa_led(struct LEAPIORAID_ADAPTER *ioc,
			   struct leapioraid_sas_device *sas_device)
{
	struct LeapioraidSepRep_t mpi_reply;
	struct LeapioraidSepReq_t mpi_request;

	memset(&mpi_request, 0, sizeof(struct LeapioraidSepReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_SCSI_ENCLOSURE_PROCESSOR;
	mpi_request.Action = LEAPIORAID_SEP_REQ_ACTION_WRITE_STATUS;
	mpi_request.SlotStatus = 0;
	mpi_request.Slot = cpu_to_le16(sas_device->slot);
	mpi_request.DevHandle = 0;
	mpi_request.EnclosureHandle = cpu_to_le16(sas_device->enclosure_handle);
	mpi_request.Flags = LEAPIORAID_SEP_REQ_FLAGS_ENCLOSURE_SLOT_ADDRESS;
	if ((leapioraid_base_scsi_enclosure_processor(ioc, &mpi_reply,
						      &mpi_request)) != 0) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return;
	}
	if (mpi_reply.IOCStatus || mpi_reply.IOCLogInfo) {
		dewtprintk(ioc, pr_info(
			"%s enclosure_processor: ioc_status (0x%04x), loginfo(0x%08x)\n",
			ioc->name, le16_to_cpu(mpi_reply.IOCStatus),
			le32_to_cpu(mpi_reply.IOCLogInfo)));
		return;
	}
}

static void
leapioraid_scsihost_send_event_to_turn_on_pfa_led(
	struct LEAPIORAID_ADAPTER *ioc,
	u16 handle)
{
	struct leapioraid_fw_event_work *fw_event;

	fw_event = leapioraid_alloc_fw_event_work(0);
	if (!fw_event)
		return;
	fw_event->event = LEAPIORAID_TURN_ON_PFA_LED;
	fw_event->device_handle = handle;
	fw_event->ioc = ioc;
	leapioraid_scsihost_fw_event_add(ioc, fw_event);
	leapioraid_fw_event_work_put(fw_event);
}

static void
leapioraid_scsihost_smart_predicted_fault(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle,
	u8 from_sata_smart_polling)
{
	struct scsi_target *starget;
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	struct LeapioraidEventNotificationRep_t *event_reply;
	struct LeapioraidEventDataSasDeviceStatusChange_t *event_data;
	struct leapioraid_sas_device *sas_device;
	ssize_t sz;
	unsigned long flags;

	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_device = __leapioraid_get_sdev_by_handle(ioc, handle);
	if (!sas_device)
		goto out_unlock;

	starget = sas_device->starget;
	sas_target_priv_data = starget->hostdata;
	if ((sas_target_priv_data->flags & LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT)
	    || ((sas_target_priv_data->flags & LEAPIORAID_TARGET_FLAGS_VOLUME)))
		goto out_unlock;
	leapioraid_scsihost_display_enclosure_chassis_info(NULL, sas_device, NULL,
						 starget);
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	if (from_sata_smart_polling)
		leapioraid_scsihost_send_event_to_turn_on_pfa_led(ioc, handle);
	sz = offsetof(struct LeapioraidEventNotificationRep_t, EventData) +
	    sizeof(struct LeapioraidEventDataSasDeviceStatusChange_t);
	event_reply = kzalloc(sz, GFP_ATOMIC);
	if (!event_reply) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out;
	}
	event_reply->Function = LEAPIORAID_FUNC_EVENT_NOTIFICATION;
	event_reply->Event =
	    cpu_to_le16(LEAPIORAID_EVENT_SAS_DEVICE_STATUS_CHANGE);
	event_reply->MsgLength = sz / 4;
	event_reply->EventDataLength =
	    cpu_to_le16(sizeof(struct LeapioraidEventDataSasDeviceStatusChange_t) / 4);
	event_data = (struct LeapioraidEventDataSasDeviceStatusChange_t *)
	    event_reply->EventData;
	event_data->ReasonCode = LEAPIORAID_EVENT_SAS_DEV_STAT_RC_SMART_DATA;
	event_data->ASC = 0x5D;
	event_data->DevHandle = cpu_to_le16(handle);
	event_data->SASAddress = cpu_to_le64(sas_target_priv_data->sas_address);
	leapioraid_ctl_add_to_event_log(ioc, event_reply);
	kfree(event_reply);
out:
	if (sas_device)
		leapioraid_sas_device_put(sas_device);
	return;
out_unlock:
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	goto out;
}

static u8
leapioraid_scsihost_io_done(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid, u8 msix_index,
	u32 reply)
{
	struct LeapioraidSCSIIOReq_t *mpi_request;
	struct LeapioraidSCSIIORep_t *mpi_reply;
	struct scsi_cmnd *scmd;
	u16 ioc_status, error_response_count = 0;
	u32 xfer_cnt;
	u8 scsi_state;
	u8 scsi_status;
	u32 log_info;
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	u32 response_code = 0;
	struct leapioraid_scsiio_tracker *st;

	scmd = leapioraid_scsihost_scsi_lookup_get(ioc, smid);
	if (scmd == NULL)
		return 1;
	leapioraid_scsihost_set_satl_pending(scmd, false);
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	mpi_reply = leapioraid_base_get_reply_virt_addr(ioc, reply);
	if (mpi_reply == NULL) {
		scmd->result = DID_OK << 16;
		goto out;
	}
	sas_device_priv_data = scmd->device->hostdata;
	if (!sas_device_priv_data || !sas_device_priv_data->sas_target ||
	    sas_device_priv_data->sas_target->deleted) {
		scmd->result = DID_NO_CONNECT << 16;
		goto out;
	}
	ioc_status = le16_to_cpu(mpi_reply->IOCStatus);
	st = leapioraid_base_scsi_cmd_priv(scmd);
	if (st->direct_io && ((ioc_status & LEAPIORAID_IOCSTATUS_MASK)
			      != LEAPIORAID_IOCSTATUS_SCSI_TASK_TERMINATED)) {
		st->scmd = scmd;
		st->direct_io = 0;
		memcpy(mpi_request->CDB.CDB32, scmd->cmnd, scmd->cmd_len);
		mpi_request->DevHandle =
		    cpu_to_le16(sas_device_priv_data->sas_target->handle);
		ioc->put_smid_scsi_io(ioc, smid,
				      sas_device_priv_data->sas_target->handle);
		return 0;
	}
	scsi_state = mpi_reply->SCSIState;
	if (scsi_state & LEAPIORAID_SCSI_STATE_RESPONSE_INFO_VALID)
		response_code = le32_to_cpu(mpi_reply->ResponseInfo) & 0xFF;
	if (!sas_device_priv_data->tlr_snoop_check) {
		sas_device_priv_data->tlr_snoop_check++;
		if ((sas_device_priv_data->flags & LEAPIORAID_DEVICE_TLR_ON) &&
		    response_code == LEAPIORAID_SCSITASKMGMT_RSP_INVALID_FRAME)
			sas_device_priv_data->flags &= ~LEAPIORAID_DEVICE_TLR_ON;
	}
	if (ioc_status & LEAPIORAID_IOCSTATUS_FLAG_LOG_INFO_AVAILABLE)
		log_info = le32_to_cpu(mpi_reply->IOCLogInfo);
	else
		log_info = 0;
	ioc_status &= LEAPIORAID_IOCSTATUS_MASK;
	scsi_status = mpi_reply->SCSIStatus;
	xfer_cnt = le32_to_cpu(mpi_reply->TransferCount);
	scsi_set_resid(scmd, scsi_bufflen(scmd) - xfer_cnt);
	if (ioc_status == LEAPIORAID_IOCSTATUS_SCSI_DATA_UNDERRUN
		&& xfer_cnt == 0
	    && (scsi_status == LEAPIORAID_SCSI_STATUS_BUSY
		|| scsi_status == LEAPIORAID_SCSI_STATUS_RESERVATION_CONFLICT
		|| scsi_status == LEAPIORAID_SCSI_STATUS_TASK_SET_FULL)) {
		ioc_status = LEAPIORAID_IOCSTATUS_SUCCESS;
	}
	if (scsi_state & LEAPIORAID_SCSI_STATE_AUTOSENSE_VALID) {
		struct sense_info data;
		const void *sense_data = leapioraid_base_get_sense_buffer(ioc,
									  smid);
		u32 sz = min_t(u32, SCSI_SENSE_BUFFERSIZE,
			       le32_to_cpu(mpi_reply->SenseCount));
		memcpy(scmd->sense_buffer, sense_data, sz);
		leapioraid_scsihost_normalize_sense(scmd->sense_buffer, &data);
		if (data.asc == 0x5D)
			leapioraid_scsihost_smart_predicted_fault(ioc,
							le16_to_cpu(mpi_reply->DevHandle),
							0);
	}
	switch (ioc_status) {
	case LEAPIORAID_IOCSTATUS_BUSY:
	case LEAPIORAID_IOCSTATUS_INSUFFICIENT_RESOURCES:
		scmd->result = SAM_STAT_BUSY;
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_DEVICE_NOT_THERE:
		scmd->result = DID_NO_CONNECT << 16;
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_IOC_TERMINATED:
		if (sas_device_priv_data->block) {
			scmd->result = DID_TRANSPORT_DISRUPTED << 16;
			goto out;
		}
		if (log_info == 0x31110630) {
			if (scmd->retries > 2) {
				scmd->result = DID_NO_CONNECT << 16;
				scsi_device_set_state(scmd->device,
						      SDEV_OFFLINE);
			} else {
				scmd->result = DID_SOFT_ERROR << 16;
				scmd->device->expecting_cc_ua = 1;
			}
			break;
		} else if (log_info == 0x32010081) {
			scmd->result = DID_RESET << 16;
			break;
		} else if ((scmd->device->channel == RAID_CHANNEL) &&
			   (scsi_state == (LEAPIORAID_SCSI_STATE_TERMINATED |
					   LEAPIORAID_SCSI_STATE_NO_SCSI_STATUS))) {
			scmd->result = DID_RESET << 16;
			break;
		}
		scmd->result = DID_SOFT_ERROR << 16;
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_TASK_TERMINATED:
	case LEAPIORAID_IOCSTATUS_SCSI_EXT_TERMINATED:
		scmd->result = DID_RESET << 16;
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_RESIDUAL_MISMATCH:
		if ((xfer_cnt == 0) || (scmd->underflow > xfer_cnt))
			scmd->result = DID_SOFT_ERROR << 16;
		else
			scmd->result = (DID_OK << 16) | scsi_status;
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_DATA_UNDERRUN:
		scmd->result = (DID_OK << 16) | scsi_status;
		if ((scsi_state & LEAPIORAID_SCSI_STATE_AUTOSENSE_VALID))
			break;
		if (xfer_cnt < scmd->underflow) {
			if (scsi_status == SAM_STAT_BUSY)
				scmd->result = SAM_STAT_BUSY;
			else
				scmd->result = DID_SOFT_ERROR << 16;
		} else if (scsi_state & (LEAPIORAID_SCSI_STATE_AUTOSENSE_FAILED |
					 LEAPIORAID_SCSI_STATE_NO_SCSI_STATUS))
			scmd->result = DID_SOFT_ERROR << 16;
		else if (scsi_state & LEAPIORAID_SCSI_STATE_TERMINATED)
			scmd->result = DID_RESET << 16;
		else if (!xfer_cnt && scmd->cmnd[0] == REPORT_LUNS) {
			mpi_reply->SCSIState =
			    LEAPIORAID_SCSI_STATE_AUTOSENSE_VALID;
			mpi_reply->SCSIStatus = SAM_STAT_CHECK_CONDITION;
			scsi_build_sense(scmd, 0,
							ILLEGAL_REQUEST, 0x20,
							0);
		}
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_DATA_OVERRUN:
		scsi_set_resid(scmd, 0);
		fallthrough;
	case LEAPIORAID_IOCSTATUS_SCSI_RECOVERED_ERROR:
	case LEAPIORAID_IOCSTATUS_SUCCESS:
		scmd->result = (DID_OK << 16) | scsi_status;
		if (response_code ==
		    LEAPIORAID_SCSITASKMGMT_RSP_INVALID_FRAME ||
		    (scsi_state & (LEAPIORAID_SCSI_STATE_AUTOSENSE_FAILED |
				   LEAPIORAID_SCSI_STATE_NO_SCSI_STATUS)))
			scmd->result = DID_SOFT_ERROR << 16;
		else if (scsi_state & LEAPIORAID_SCSI_STATE_TERMINATED)
			scmd->result = DID_RESET << 16;
		break;
	case LEAPIORAID_IOCSTATUS_EEDP_GUARD_ERROR:
	case LEAPIORAID_IOCSTATUS_EEDP_REF_TAG_ERROR:
		fallthrough;
	case LEAPIORAID_IOCSTATUS_EEDP_APP_TAG_ERROR:
		fallthrough;
	case LEAPIORAID_IOCSTATUS_SCSI_PROTOCOL_ERROR:
	case LEAPIORAID_IOCSTATUS_INVALID_FUNCTION:
	case LEAPIORAID_IOCSTATUS_INVALID_SGL:
	case LEAPIORAID_IOCSTATUS_INTERNAL_ERROR:
	case LEAPIORAID_IOCSTATUS_INVALID_FIELD:
	case LEAPIORAID_IOCSTATUS_INVALID_STATE:
	case LEAPIORAID_IOCSTATUS_SCSI_IO_DATA_ERROR:
	case LEAPIORAID_IOCSTATUS_SCSI_TASK_MGMT_FAILED:
	case LEAPIORAID_IOCSTATUS_INSUFFICIENT_POWER:
	default:
		scmd->result = DID_SOFT_ERROR << 16;
		break;
	}
	if (scmd->result && (ioc->logging_level & LEAPIORAID_DEBUG_REPLY))
		leapioraid_scsihost_scsi_ioc_info(
			ioc, scmd, mpi_reply, smid, scsi_status,
			error_response_count);
out:
	scsi_dma_unmap(scmd);
	leapioraid_base_free_smid(ioc, smid);
	scsi_done(scmd);
	return 0;
}

static void
leapioraid_scsihost_update_vphys_after_reset(
	struct LEAPIORAID_ADAPTER *ioc)
{
	u16 sz, ioc_status;
	int i;
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidSasIOUnitP0_t *sas_iounit_pg0 = NULL;
	u16 attached_handle;
	u64 attached_sas_addr;
	u8 found = 0, port_id;
	struct LeapioraidSasPhyP0_t phy_pg0;
	struct leapioraid_hba_port *port, *port_next, *mport;
	struct leapioraid_virtual_phy *vphy, *vphy_next;
	struct leapioraid_sas_device *sas_device;

	list_for_each_entry_safe(port, port_next, &ioc->port_table_list, list) {
		if (!port->vphys_mask)
			continue;
		list_for_each_entry_safe(vphy, vphy_next, &port->vphys_list,
					 list) {
			vphy->flags |= LEAPIORAID_VPHY_FLAG_DIRTY_PHY;
		}
	}
	sz = offsetof(struct LeapioraidSasIOUnitP0_t, PhyData)
		+ (ioc->sas_hba.num_phys
			* sizeof(struct LEAPIORAID_SAS_IO_UNIT0_PHY_DATA));
	sas_iounit_pg0 = kzalloc(sz, GFP_KERNEL);
	if (!sas_iounit_pg0) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return;
	}
	if ((leapioraid_config_get_sas_iounit_pg0(ioc, &mpi_reply,
						  sas_iounit_pg0, sz)) != 0)
		goto out;
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS)
		goto out;
	for (i = 0; i < ioc->sas_hba.num_phys; i++) {
		if ((sas_iounit_pg0->PhyData[i].NegotiatedLinkRate >> 4) <
		    LEAPIORAID_SAS_NEG_LINK_RATE_1_5)
			continue;
		if (!(le32_to_cpu(sas_iounit_pg0->PhyData[i].ControllerPhyDeviceInfo)
			& LEAPIORAID_SAS_DEVICE_INFO_SEP))
			continue;
		if ((leapioraid_config_get_phy_pg0(ioc, &mpi_reply, &phy_pg0,
						   i))) {
			pr_err("%s failure at %s:%d/%s()!\n",
			       ioc->name, __FILE__, __LINE__, __func__);
			continue;
		}
		if (!
		    (le32_to_cpu(phy_pg0.PhyInfo) &
		     LEAPIORAID_SAS_PHYINFO_VIRTUAL_PHY))
			continue;
		attached_handle =
		    le16_to_cpu(sas_iounit_pg0->PhyData[i].AttachedDevHandle);
		if (leapioraid_scsihost_get_sas_address
		    (ioc, attached_handle, &attached_sas_addr)
		    != 0) {
			pr_err("%s failure at %s:%d/%s()!\n",
			       ioc->name, __FILE__, __LINE__, __func__);
			continue;
		}
		found = 0;
		port = port_next = NULL;
		list_for_each_entry_safe(port, port_next, &ioc->port_table_list,
					 list) {
			if (!port->vphys_mask)
				continue;
			list_for_each_entry_safe(vphy, vphy_next,
						 &port->vphys_list, list) {
				if (!
				    (vphy->flags & LEAPIORAID_VPHY_FLAG_DIRTY_PHY))
					continue;
				if (vphy->sas_address != attached_sas_addr)
					continue;
				if (!(vphy->phy_mask & (1 << i)))
					vphy->phy_mask = (1 << i);
				port_id = sas_iounit_pg0->PhyData[i].Port;
				mport =
				    leapioraid_get_port_by_id(ioc, port_id, 1);
				if (!mport) {
					mport =
					    kzalloc(sizeof(struct leapioraid_hba_port),
						    GFP_KERNEL);
					if (!mport) {
						pr_err(
						       "%s failure at %s:%d/%s()!\n",
						       ioc->name, __FILE__,
						       __LINE__, __func__);
						break;
					}
					mport->port_id = port_id;
					pr_err(
						"%s %s: hba_port entry: %p, port: %d is added to hba_port list\n",
					    ioc->name, __func__, mport,
					    mport->port_id);
					list_add_tail(&mport->list,
						      &ioc->port_table_list);
				}
				if (port != mport) {
					if (!mport->vphys_mask)
						INIT_LIST_HEAD(&mport->vphys_list);
					mport->vphys_mask |= (1 << i);
					port->vphys_mask &= ~(1 << i);
					list_move(&vphy->list,
						  &mport->vphys_list);
					sas_device =
					    leapioraid_get_sdev_by_addr(ioc,
									attached_sas_addr,
									port);
					if (sas_device)
						sas_device->port = mport;
				}
				if (mport->flags & LEAPIORAID_HBA_PORT_FLAG_DIRTY_PORT) {
					mport->sas_address = 0;
					mport->phy_mask = 0;
					mport->flags &=
					    ~LEAPIORAID_HBA_PORT_FLAG_DIRTY_PORT;
				}
				vphy->flags &= ~LEAPIORAID_VPHY_FLAG_DIRTY_PHY;
				found = 1;
				break;
			}
			if (found)
				break;
		}
	}
out:
	kfree(sas_iounit_pg0);
}

static u8
leapioraid_scsihost_get_port_table_after_reset(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_hba_port *port_table)
{
	u16 sz, ioc_status;
	int i, j;
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidSasIOUnitP0_t *sas_iounit_pg0 = NULL;
	u16 attached_handle;
	u64 attached_sas_addr;
	u8 found = 0, port_count = 0, port_id;

	sz = offsetof(struct LeapioraidSasIOUnitP0_t, PhyData)
		+ (ioc->sas_hba.num_phys
		* sizeof(struct LEAPIORAID_SAS_IO_UNIT0_PHY_DATA));
	sas_iounit_pg0 = kzalloc(sz, GFP_KERNEL);
	if (!sas_iounit_pg0) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return port_count;
	}
	if ((leapioraid_config_get_sas_iounit_pg0(ioc, &mpi_reply,
						  sas_iounit_pg0, sz)) != 0)
		goto out;
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS)
		goto out;
	for (i = 0; i < ioc->sas_hba.num_phys; i++) {
		found = 0;
		if ((sas_iounit_pg0->PhyData[i].NegotiatedLinkRate >> 4) <
		    LEAPIORAID_SAS_NEG_LINK_RATE_1_5)
			continue;
		attached_handle =
		    le16_to_cpu(sas_iounit_pg0->PhyData[i].AttachedDevHandle);
		if (leapioraid_scsihost_get_sas_address
		    (ioc, attached_handle, &attached_sas_addr)
		    != 0) {
			pr_err("%s failure at %s:%d/%s()!\n",
			       ioc->name, __FILE__, __LINE__, __func__);
			continue;
		}
		for (j = 0; j < port_count; j++) {
			port_id = sas_iounit_pg0->PhyData[i].Port;
			if ((port_table[j].port_id == port_id) &&
			    (port_table[j].sas_address == attached_sas_addr)) {
				port_table[j].phy_mask |= (1 << i);
				found = 1;
				break;
			}
		}
		if (found)
			continue;
		port_id = sas_iounit_pg0->PhyData[i].Port;
		port_table[port_count].port_id = port_id;
		port_table[port_count].phy_mask = (1 << i);
		port_table[port_count].sas_address = attached_sas_addr;
		port_count++;
	}
out:
	kfree(sas_iounit_pg0);
	return port_count;
}

enum hba_port_matched_codes {
	NOT_MATCHED = 0,
	MATCHED_WITH_ADDR_AND_PHYMASK,
	MATCHED_WITH_ADDR_SUBPHYMASK_AND_PORT,
	MATCHED_WITH_ADDR_AND_SUBPHYMASK,
	MATCHED_WITH_ADDR,
};
static int
leapioraid_scsihost_look_and_get_matched_port_entry(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_hba_port *port_entry,
	struct leapioraid_hba_port **matched_port_entry,
	int *count)
{
	struct leapioraid_hba_port *port_table_entry, *matched_port = NULL;
	enum hba_port_matched_codes matched_code = NOT_MATCHED;
	int lcount = 0;

	*matched_port_entry = NULL;
	list_for_each_entry(port_table_entry, &ioc->port_table_list, list) {
		if (!(port_table_entry->flags & LEAPIORAID_HBA_PORT_FLAG_DIRTY_PORT))
			continue;
		if ((port_table_entry->sas_address == port_entry->sas_address)
		    && (port_table_entry->phy_mask == port_entry->phy_mask)) {
			matched_code = MATCHED_WITH_ADDR_AND_PHYMASK;
			matched_port = port_table_entry;
			break;
		}
		if ((port_table_entry->sas_address == port_entry->sas_address)
		    && (port_table_entry->phy_mask & port_entry->phy_mask)
		    && (port_table_entry->port_id == port_entry->port_id)) {
			matched_code = MATCHED_WITH_ADDR_SUBPHYMASK_AND_PORT;
			matched_port = port_table_entry;
			continue;
		}
		if ((port_table_entry->sas_address == port_entry->sas_address)
		    && (port_table_entry->phy_mask & port_entry->phy_mask)) {
			if (matched_code ==
			    MATCHED_WITH_ADDR_SUBPHYMASK_AND_PORT)
				continue;
			matched_code = MATCHED_WITH_ADDR_AND_SUBPHYMASK;
			matched_port = port_table_entry;
			continue;
		}
		if (port_table_entry->sas_address == port_entry->sas_address) {
			if (matched_code ==
			    MATCHED_WITH_ADDR_SUBPHYMASK_AND_PORT)
				continue;
			if (matched_code == MATCHED_WITH_ADDR_AND_SUBPHYMASK)
				continue;
			matched_code = MATCHED_WITH_ADDR;
			matched_port = port_table_entry;
			lcount++;
		}
	}
	*matched_port_entry = matched_port;
	if (matched_code == MATCHED_WITH_ADDR)
		*count = lcount;
	return matched_code;
}

static void
leapioraid_scsihost_del_phy_part_of_anther_port(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_hba_port *port_table,
	int index, u8 port_count, int offset)
{
	struct leapioraid_raid_sas_node *sas_node = &ioc->sas_hba;
	u32 i, found = 0;

	for (i = 0; i < port_count; i++) {
		if (i == index)
			continue;
		if (port_table[i].phy_mask & (1 << offset)) {
			leapioraid_transport_del_phy_from_an_existing_port(
				ioc,
				sas_node,
				&sas_node->phy
				[offset]);
			found = 1;
			break;
		}
	}
	if (!found)
		port_table[index].phy_mask |= (1 << offset);
}

static void
leapioraid_scsihost_add_or_del_phys_from_existing_port(
		struct LEAPIORAID_ADAPTER *ioc,
		struct leapioraid_hba_port *hba_port_entry,
		struct leapioraid_hba_port *port_table,
		int index, u8 port_count)
{
	u32 phy_mask, offset = 0;
	struct leapioraid_raid_sas_node *sas_node = &ioc->sas_hba;

	phy_mask = hba_port_entry->phy_mask ^ port_table[index].phy_mask;
	for (offset = 0; offset < ioc->sas_hba.num_phys; offset++) {
		if (phy_mask & (1 << offset)) {
			if (!(port_table[index].phy_mask & (1 << offset))) {
				leapioraid_scsihost_del_phy_part_of_anther_port(
					ioc,
					port_table,
					index,
					port_count,
					offset);
			} else {
#if defined(LEAPIORAID_WIDE_PORT_API)
				if (sas_node->phy[offset].phy_belongs_to_port)
					leapioraid_transport_del_phy_from_an_existing_port
					    (ioc, sas_node,
					     &sas_node->phy[offset]);
				leapioraid_transport_add_phy_to_an_existing_port
				    (ioc, sas_node, &sas_node->phy[offset],
				     hba_port_entry->sas_address,
				     hba_port_entry);
#endif
			}
		}
	}
}

static void
leapioraid_scsihost_del_dirty_vphy(struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_hba_port *port, *port_next;
	struct leapioraid_virtual_phy *vphy, *vphy_next;

	list_for_each_entry_safe(port, port_next, &ioc->port_table_list, list) {
		if (!port->vphys_mask)
			continue;
		list_for_each_entry_safe(vphy, vphy_next, &port->vphys_list,
					 list) {
			if (vphy->flags & LEAPIORAID_VPHY_FLAG_DIRTY_PHY) {
				drsprintk(ioc, pr_err(
					"%s Deleting vphy %p entry from port id: %d\t, Phy_mask 0x%08x\n",
					ioc->name, vphy,
					port->port_id,
					vphy->phy_mask));
				port->vphys_mask &= ~vphy->phy_mask;
				list_del(&vphy->list);
				kfree(vphy);
			}
		}
		if (!port->vphys_mask && !port->sas_address)
			port->flags |= LEAPIORAID_HBA_PORT_FLAG_DIRTY_PORT;
	}
}

static void
leapioraid_scsihost_del_dirty_port_entries(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_hba_port *port, *port_next;

	list_for_each_entry_safe(port, port_next, &ioc->port_table_list, list) {
		if (!(port->flags & LEAPIORAID_HBA_PORT_FLAG_DIRTY_PORT) ||
		    port->flags & LEAPIORAID_HBA_PORT_FLAG_NEW_PORT)
			continue;
		drsprintk(ioc, pr_err(
			"%s Deleting port table entry %p having Port id: %d\t, Phy_mask 0x%08x\n",
			ioc->name, port, port->port_id,
			port->phy_mask));
		list_del(&port->list);
		kfree(port);
	}
}

static void
leapioraid_scsihost_sas_port_refresh(struct LEAPIORAID_ADAPTER *ioc)
{
	u8 port_count = 0;
	struct leapioraid_hba_port *port_table;
	struct leapioraid_hba_port *port_table_entry;
	struct leapioraid_hba_port *port_entry = NULL;
	int i, j, ret, count = 0, lcount = 0;
	u64 sas_addr;
	u8 num_phys;

	drsprintk(ioc, pr_err(
		"%s updating ports for sas_host(0x%016llx)\n",
		ioc->name,
		(unsigned long long)ioc->sas_hba.sas_address));
	leapioraid_config_get_number_hba_phys(ioc, &num_phys);
	if (!num_phys) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return;
	}
	if (num_phys > ioc->sas_hba.nr_phys_allocated) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return;
	}
	ioc->sas_hba.num_phys = num_phys;
	port_table = kcalloc(ioc->sas_hba.num_phys,
			     sizeof(struct leapioraid_hba_port), GFP_KERNEL);
	if (!port_table)
		return;
	port_count = leapioraid_scsihost_get_port_table_after_reset(
		ioc, port_table);
	if (!port_count)
		return;
	drsprintk(ioc,
		  pr_info("%s New Port table\n", ioc->name));
	for (j = 0; j < port_count; j++)
		drsprintk(ioc, pr_err(
			"%s Port: %d\t Phy_mask 0x%08x\t sas_addr(0x%016llx)\n",
			ioc->name, port_table[j].port_id,
			port_table[j].phy_mask,
			port_table[j].sas_address));
	list_for_each_entry(port_table_entry, &ioc->port_table_list, list) {
		port_table_entry->flags |= LEAPIORAID_HBA_PORT_FLAG_DIRTY_PORT;
	}
	drsprintk(ioc,
		  pr_info("%s Old Port table\n", ioc->name));
	port_table_entry = NULL;
	list_for_each_entry(port_table_entry, &ioc->port_table_list, list) {
		drsprintk(ioc, pr_err(
			"%s Port: %d\t Phy_mask 0x%08x\t sas_addr(0x%016llx)\n",
			ioc->name, port_table_entry->port_id,
			port_table_entry->phy_mask,
			port_table_entry->sas_address));
	}
	for (j = 0; j < port_count; j++) {
		ret = leapioraid_scsihost_look_and_get_matched_port_entry(ioc,
			&port_table[j],
			&port_entry,
			&count);
		if (!port_entry) {
			drsprintk(ioc, pr_err(
				"%s No Matched entry for sas_addr(0x%16llx), Port:%d\n",
				ioc->name,
				port_table[j].sas_address,
				port_table[j].port_id));
			continue;
		}
		switch (ret) {
		case MATCHED_WITH_ADDR_SUBPHYMASK_AND_PORT:
		case MATCHED_WITH_ADDR_AND_SUBPHYMASK:
			leapioraid_scsihost_add_or_del_phys_from_existing_port(ioc,
				port_entry,
				port_table,
				j,
				port_count);
			break;
		case MATCHED_WITH_ADDR:
			sas_addr = port_table[j].sas_address;
			for (i = 0; i < port_count; i++) {
				if (port_table[i].sas_address == sas_addr)
					lcount++;
			}
			if ((count > 1) || (lcount > 1))
				port_entry = NULL;
			else
				leapioraid_scsihost_add_or_del_phys_from_existing_port
				    (ioc, port_entry, port_table, j,
				     port_count);
		}
		if (!port_entry)
			continue;
		if (port_entry->port_id != port_table[j].port_id)
			port_entry->port_id = port_table[j].port_id;
		port_entry->flags &= ~LEAPIORAID_HBA_PORT_FLAG_DIRTY_PORT;
		port_entry->phy_mask = port_table[j].phy_mask;
	}
	port_table_entry = NULL;
}

static
struct leapioraid_virtual_phy *leapioraid_scsihost_alloc_vphy(
	struct LEAPIORAID_ADAPTER *ioc,
						u8 port_id, u8 phy_num)
{
	struct leapioraid_virtual_phy *vphy;
	struct leapioraid_hba_port *port;

	port = leapioraid_get_port_by_id(ioc, port_id, 0);
	if (!port)
		return NULL;
	vphy = leapioraid_get_vphy_by_phy(ioc, port, phy_num);
	if (!vphy) {
		vphy = kzalloc(sizeof(struct leapioraid_virtual_phy), GFP_KERNEL);
		if (!vphy)
			return NULL;
		if (!port->vphys_mask)
			INIT_LIST_HEAD(&port->vphys_list);
		port->vphys_mask |= (1 << phy_num);
		vphy->phy_mask |= (1 << phy_num);
		list_add_tail(&vphy->list, &port->vphys_list);
		pr_info(
			"%s vphy entry: %p, port id: %d, phy:%d is added to port's vphys_list\n",
		    ioc->name, vphy, port->port_id, phy_num);
	}
	return vphy;
}

static void
leapioraid_scsihost_sas_host_refresh(struct LEAPIORAID_ADAPTER *ioc)
{
	u16 sz;
	u16 ioc_status;
	int i;
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidSasIOUnitP0_t *sas_iounit_pg0 = NULL;
	u16 attached_handle;
	u8 link_rate, port_id;
	struct leapioraid_hba_port *port;
	struct LeapioraidSasPhyP0_t phy_pg0;

	dtmprintk(ioc, pr_err(
		"%s updating handles for sas_host(0x%016llx)\n",
		ioc->name,
		(unsigned long long)ioc->sas_hba.sas_address));
	sz = offsetof(struct LeapioraidSasIOUnitP0_t,
		      PhyData) +
	    (ioc->sas_hba.num_phys * sizeof(struct LEAPIORAID_SAS_IO_UNIT0_PHY_DATA));
	sas_iounit_pg0 = kzalloc(sz, GFP_KERNEL);
	if (!sas_iounit_pg0) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return;
	}
	if ((leapioraid_config_get_sas_iounit_pg0(ioc, &mpi_reply,
						  sas_iounit_pg0, sz)) != 0)
		goto out;
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS)
		goto out;
	for (i = 0; i < ioc->sas_hba.num_phys; i++) {
		link_rate = sas_iounit_pg0->PhyData[i].NegotiatedLinkRate >> 4;
		if (i == 0)
			ioc->sas_hba.handle =
			    le16_to_cpu(sas_iounit_pg0->PhyData[0].ControllerDevHandle);
		port_id = sas_iounit_pg0->PhyData[i].Port;
		if (!(leapioraid_get_port_by_id(ioc, port_id, 0))) {
			port = kzalloc(sizeof(struct leapioraid_hba_port), GFP_KERNEL);
			if (!port)
				goto out;

			port->port_id = port_id;
			pr_info(
				"%s hba_port entry: %p, port: %d is added to hba_port list\n",
			    ioc->name, port, port->port_id);
			if (ioc->shost_recovery)
				port->flags = LEAPIORAID_HBA_PORT_FLAG_NEW_PORT;
			list_add_tail(&port->list, &ioc->port_table_list);
		}
		if (le32_to_cpu
		    (sas_iounit_pg0->PhyData[i].ControllerPhyDeviceInfo)
			& LEAPIORAID_SAS_DEVICE_INFO_SEP
		    && (link_rate >= LEAPIORAID_SAS_NEG_LINK_RATE_1_5)) {
			if ((leapioraid_config_get_phy_pg0
			     (ioc, &mpi_reply, &phy_pg0, i))) {
				pr_err(
					"%s failure at %s:%d/%s()!\n", ioc->name,
					__FILE__, __LINE__, __func__);
				continue;
			}
			if (!
			    (le32_to_cpu(phy_pg0.PhyInfo) &
			     LEAPIORAID_SAS_PHYINFO_VIRTUAL_PHY))
				continue;
			if (!leapioraid_scsihost_alloc_vphy(ioc, port_id, i))
				goto out;
			ioc->sas_hba.phy[i].hba_vphy = 1;
		}
		ioc->sas_hba.phy[i].handle = ioc->sas_hba.handle;
		attached_handle =
		    le16_to_cpu(sas_iounit_pg0->PhyData[i].AttachedDevHandle);
		if (attached_handle
		    && link_rate < LEAPIORAID_SAS_NEG_LINK_RATE_1_5)
			link_rate = LEAPIORAID_SAS_NEG_LINK_RATE_1_5;
		ioc->sas_hba.phy[i].port =
		    leapioraid_get_port_by_id(ioc, port_id, 0);
		if (!ioc->sas_hba.phy[i].phy) {
			if ((leapioraid_config_get_phy_pg0
			     (ioc, &mpi_reply, &phy_pg0, i))) {
				pr_err(
				       "%s failure at %s:%d/%s()!\n", ioc->name,
				       __FILE__, __LINE__, __func__);
				continue;
			}
			ioc_status = le16_to_cpu(mpi_reply.IOCStatus) &
			    LEAPIORAID_IOCSTATUS_MASK;
			if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
				pr_err(
				       "%s failure at %s:%d/%s()!\n", ioc->name,
				       __FILE__, __LINE__, __func__);
				continue;
			}
			ioc->sas_hba.phy[i].phy_id = i;
			leapioraid_transport_add_host_phy(ioc,
							  &ioc->sas_hba.phy[i],
							  phy_pg0,
							  ioc->sas_hba.parent_dev);
			continue;
		}
		leapioraid_transport_update_links(ioc, ioc->sas_hba.sas_address,
						  attached_handle, i, link_rate,
						  ioc->sas_hba.phy[i].port);
	}
out:
	kfree(sas_iounit_pg0);
}

static void
leapioraid_scsihost_sas_host_add(struct LEAPIORAID_ADAPTER *ioc)
{
	int i;
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidSasIOUnitP0_t *sas_iounit_pg0 = NULL;
	struct LeapioraidSasIOUnitP1_t *sas_iounit_pg1 = NULL;
	struct LeapioraidSasPhyP0_t phy_pg0;
	struct LeapioraidSasDevP0_t sas_device_pg0;
	struct LeapioraidSasEncP0_t enclosure_pg0;
	u16 ioc_status;
	u16 sz;
	u8 device_missing_delay;
	u8 num_phys, port_id;
	struct leapioraid_hba_port *port;

	leapioraid_config_get_number_hba_phys(ioc, &num_phys);
	if (!num_phys) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return;
	}
	ioc->sas_hba.nr_phys_allocated =
	    max_t(u8, LEAPIORAID_MAX_HBA_NUM_PHYS, num_phys);
	ioc->sas_hba.phy =
	    kcalloc(ioc->sas_hba.nr_phys_allocated,
			sizeof(struct leapioraid_sas_phy),
		    GFP_KERNEL);
	if (!ioc->sas_hba.phy) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return;
	}
	ioc->sas_hba.num_phys = num_phys;
	sz = offsetof(struct LeapioraidSasIOUnitP0_t,
		      PhyData) +
	    (ioc->sas_hba.num_phys
			* sizeof(struct LEAPIORAID_SAS_IO_UNIT0_PHY_DATA));
	sas_iounit_pg0 = kzalloc(sz, GFP_KERNEL);
	if (!sas_iounit_pg0) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return;
	}
	if ((leapioraid_config_get_sas_iounit_pg0(ioc, &mpi_reply,
						  sas_iounit_pg0, sz))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out;
	}
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus)
		& LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out;
	}
	sz = offsetof(struct LeapioraidSasIOUnitP1_t,
		      PhyData) +
	    (ioc->sas_hba.num_phys
			* sizeof(struct LEAPIORAID_SAS_IO_UNIT1_PHY_DATA));
	sas_iounit_pg1 = kzalloc(sz, GFP_KERNEL);
	if (!sas_iounit_pg1) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out;
	}
	if ((leapioraid_config_get_sas_iounit_pg1(ioc, &mpi_reply,
						  sas_iounit_pg1, sz))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out;
	}
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out;
	}
	ioc->io_missing_delay = sas_iounit_pg1->IODeviceMissingDelay;
	device_missing_delay = sas_iounit_pg1->ReportDeviceMissingDelay;
	if (device_missing_delay & LEAPIORAID_SASIOUNIT1_REPORT_MISSING_UNIT_16)
		ioc->device_missing_delay = (device_missing_delay &
					     LEAPIORAID_SASIOUNIT1_REPORT_MISSING_TIMEOUT_MASK)
		    * 16;
	else
		ioc->device_missing_delay = device_missing_delay &
		    LEAPIORAID_SASIOUNIT1_REPORT_MISSING_TIMEOUT_MASK;
	ioc->sas_hba.parent_dev = &ioc->shost->shost_gendev;
	for (i = 0; i < ioc->sas_hba.num_phys; i++) {
		if ((leapioraid_config_get_phy_pg0(ioc, &mpi_reply, &phy_pg0,
						   i))) {
			pr_err("%s failure at %s:%d/%s()!\n",
			       ioc->name, __FILE__, __LINE__, __func__);
			goto out;
		}
		ioc_status = le16_to_cpu(mpi_reply.IOCStatus) &
		    LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			pr_err("%s failure at %s:%d/%s()!\n",
			       ioc->name, __FILE__, __LINE__, __func__);
			goto out;
		}
		if (i == 0)
			ioc->sas_hba.handle =
			    le16_to_cpu(sas_iounit_pg0->PhyData[0].ControllerDevHandle);
		port_id = sas_iounit_pg0->PhyData[i].Port;
		if (!(leapioraid_get_port_by_id(ioc, port_id, 0))) {
			port = kzalloc(sizeof(struct leapioraid_hba_port), GFP_KERNEL);
			if (!port)
				goto out;

			port->port_id = port_id;
			pr_info(
				"%s hba_port entry: %p, port: %d is added to hba_port list\n",
			    ioc->name, port, port->port_id);
			list_add_tail(&port->list, &ioc->port_table_list);
		}
		if ((le32_to_cpu(phy_pg0.PhyInfo) &
		     LEAPIORAID_SAS_PHYINFO_VIRTUAL_PHY)
		    && (phy_pg0.NegotiatedLinkRate >> 4) >=
		    LEAPIORAID_SAS_NEG_LINK_RATE_1_5) {
			if (!leapioraid_scsihost_alloc_vphy(ioc, port_id, i))
				goto out;
			ioc->sas_hba.phy[i].hba_vphy = 1;
		}
		ioc->sas_hba.phy[i].handle = ioc->sas_hba.handle;
		ioc->sas_hba.phy[i].phy_id = i;
		ioc->sas_hba.phy[i].port =
		    leapioraid_get_port_by_id(ioc, port_id, 0);
		leapioraid_transport_add_host_phy(ioc, &ioc->sas_hba.phy[i],
						  phy_pg0,
						  ioc->sas_hba.parent_dev);
	}
	if ((leapioraid_config_get_sas_device_pg0
	     (ioc, &mpi_reply, &sas_device_pg0,
	      LEAPIORAID_SAS_DEVICE_PGAD_FORM_HANDLE, ioc->sas_hba.handle))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out;
	}
	ioc->sas_hba.enclosure_handle =
	    le16_to_cpu(sas_device_pg0.EnclosureHandle);
	ioc->sas_hba.sas_address = le64_to_cpu(sas_device_pg0.SASAddress);
	pr_info(
		"%s host_add: handle(0x%04x), sas_addr(0x%016llx), phys(%d)\n",
		ioc->name,
		ioc->sas_hba.handle,
		(unsigned long long)ioc->sas_hba.sas_address,
		ioc->sas_hba.num_phys);
	if (ioc->sas_hba.enclosure_handle) {
		if (!(leapioraid_config_get_enclosure_pg0(ioc, &mpi_reply,
							  &enclosure_pg0,
							  LEAPIORAID_SAS_ENCLOS_PGAD_FORM_HANDLE,
							  ioc->sas_hba.enclosure_handle)))
			ioc->sas_hba.enclosure_logical_id =
			    le64_to_cpu(enclosure_pg0.EnclosureLogicalID);
	}
out:
	kfree(sas_iounit_pg1);
	kfree(sas_iounit_pg0);
}

static int
leapioraid_scsihost_expander_add(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct leapioraid_raid_sas_node *sas_expander;
	struct leapioraid_enclosure_node *enclosure_dev;
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidExpanderP0_t expander_pg0;
	struct LeapioraidExpanderP1_t expander_pg1;
	u32 ioc_status;
	u16 parent_handle;
	u64 sas_address, sas_address_parent = 0;
	int i;
	unsigned long flags;
	u8 port_id;
	struct leapioraid_sas_port *leapioraid_port = NULL;
	int rc = 0;

	if (!handle)
		return -1;
	if (ioc->shost_recovery || ioc->pci_error_recovery)
		return -1;
	if ((leapioraid_config_get_expander_pg0(
			ioc, &mpi_reply, &expander_pg0,
			LEAPIORAID_SAS_EXPAND_PGAD_FORM_HNDL,
			handle))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return -1;
	}
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus)
		& LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return -1;
	}
	parent_handle = le16_to_cpu(expander_pg0.ParentDevHandle);
	if (leapioraid_scsihost_get_sas_address(
		ioc, parent_handle, &sas_address_parent)
	    != 0) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return -1;
	}
	port_id = expander_pg0.PhysicalPort;
	if (sas_address_parent != ioc->sas_hba.sas_address) {
		spin_lock_irqsave(&ioc->sas_node_lock, flags);
		sas_expander =
		    leapioraid_scsihost_expander_find_by_sas_address(
				ioc,
				sas_address_parent,
				leapioraid_get_port_by_id(ioc, port_id, 0));
		spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
		if (!sas_expander) {
			rc = leapioraid_scsihost_expander_add(ioc, parent_handle);
			if (rc != 0)
				return rc;
		}
	}
	spin_lock_irqsave(&ioc->sas_node_lock, flags);
	sas_address = le64_to_cpu(expander_pg0.SASAddress);
	sas_expander = leapioraid_scsihost_expander_find_by_sas_address(
		ioc,
		sas_address,
		leapioraid_get_port_by_id(ioc, port_id, 0));
	spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
	if (sas_expander)
		return 0;
	sas_expander = kzalloc(sizeof(struct leapioraid_raid_sas_node),
		GFP_KERNEL);
	if (!sas_expander)
		return -1;

	sas_expander->handle = handle;
	sas_expander->num_phys = expander_pg0.NumPhys;
	sas_expander->sas_address_parent = sas_address_parent;
	sas_expander->sas_address = sas_address;
	sas_expander->port = leapioraid_get_port_by_id(ioc, port_id, 0);
	if (!sas_expander->port) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = -1;
		goto out_fail;
	}
	pr_info(
		"%s expander_add: handle(0x%04x), parent(0x%04x), sas_addr(0x%016llx), phys(%d)\n",
		ioc->name,
	    handle, parent_handle,
		(unsigned long long)sas_expander->sas_address,
		sas_expander->num_phys);
	if (!sas_expander->num_phys) {
		rc = -1;
		goto out_fail;
	}
	sas_expander->phy = kcalloc(sas_expander->num_phys,
				    sizeof(struct leapioraid_sas_phy), GFP_KERNEL);
	if (!sas_expander->phy) {
		rc = -1;
		goto out_fail;
	}
	INIT_LIST_HEAD(&sas_expander->sas_port_list);
	leapioraid_port = leapioraid_transport_port_add(
		ioc, handle,
		sas_address_parent,
		sas_expander->port);
	if (!leapioraid_port) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = -1;
		goto out_fail;
	}
	sas_expander->parent_dev = &leapioraid_port->rphy->dev;
	sas_expander->rphy = leapioraid_port->rphy;
	for (i = 0; i < sas_expander->num_phys; i++) {
		if ((leapioraid_config_get_expander_pg1(
				ioc, &mpi_reply,
				&expander_pg1, i,
				handle))) {
			pr_err("%s failure at %s:%d/%s()!\n",
			       ioc->name, __FILE__, __LINE__, __func__);
			rc = -1;
			goto out_fail;
		}
		sas_expander->phy[i].handle = handle;
		sas_expander->phy[i].phy_id = i;
		sas_expander->phy[i].port =
		    leapioraid_get_port_by_id(ioc, port_id, 0);
		if ((leapioraid_transport_add_expander_phy
		     (ioc, &sas_expander->phy[i], expander_pg1,
		      sas_expander->parent_dev))) {
			pr_err("%s failure at %s:%d/%s()!\n",
			       ioc->name, __FILE__, __LINE__, __func__);
			rc = -1;
			goto out_fail;
		}
	}
	if (sas_expander->enclosure_handle) {
		enclosure_dev =
		    leapioraid_scsihost_enclosure_find_by_handle(
				ioc,
				sas_expander->enclosure_handle);
		if (enclosure_dev)
			sas_expander->enclosure_logical_id =
			    le64_to_cpu(enclosure_dev->pg0.EnclosureLogicalID);
	}
	leapioraid_scsihost_expander_node_add(ioc, sas_expander);
	return 0;
out_fail:
	if (leapioraid_port)
		leapioraid_transport_port_remove(ioc,
			sas_expander->sas_address,
			sas_address_parent,
			sas_expander->port);
	kfree(sas_expander);
	return rc;
}

void
leapioraid_expander_remove(
	struct LEAPIORAID_ADAPTER *ioc,
	u64 sas_address, struct leapioraid_hba_port *port)
{
	struct leapioraid_raid_sas_node *sas_expander;
	unsigned long flags;

	if (ioc->shost_recovery)
		return;
	if (!port)
		return;
	spin_lock_irqsave(&ioc->sas_node_lock, flags);
	sas_expander = leapioraid_scsihost_expander_find_by_sas_address(
		ioc, sas_address, port);
	spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
	if (sas_expander)
		leapioraid_scsihost_expander_node_remove(
			ioc, sas_expander);
}

static u8
leapioraid_scsihost_done(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid, u8 msix_index,
	u32 reply)
{
	struct LeapioraidDefaultRep_t *mpi_reply;

	mpi_reply = leapioraid_base_get_reply_virt_addr(ioc, reply);
	if (ioc->scsih_cmds.status == LEAPIORAID_CMD_NOT_USED)
		return 1;
	if (ioc->scsih_cmds.smid != smid)
		return 1;
	ioc->scsih_cmds.status |= LEAPIORAID_CMD_COMPLETE;
	if (mpi_reply) {
		memcpy(ioc->scsih_cmds.reply, mpi_reply,
		       mpi_reply->MsgLength * 4);
		ioc->scsih_cmds.status |= LEAPIORAID_CMD_REPLY_VALID;
	}
	ioc->scsih_cmds.status &= ~LEAPIORAID_CMD_PENDING;
	complete(&ioc->scsih_cmds.done);
	return 1;
}

static int
leapioraid_scsi_send_scsi_io(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_scsi_io_transfer *transfer_packet,
	u8 tr_timeout, u8 tr_method)
{
	struct LeapioraidSCSIIORep_t *mpi_reply;
	struct LeapioSCSIIOReq_t *mpi_request;
	u16 smid;
	u8 issue_reset = 0;
	int rc;
	void *priv_sense;
	u32 mpi_control;
	void *psge;
	dma_addr_t data_out_dma = 0;
	dma_addr_t data_in_dma = 0;
	size_t data_in_sz = 0;
	size_t data_out_sz = 0;
	u16 handle;
	u8 retry_count = 0, host_reset_count = 0;
	int tm_return_code;

	if (ioc->pci_error_recovery) {
		pr_err("%s %s: pci error recovery in progress!\n",
			ioc->name, __func__);
		return -EFAULT;
	}
	if (ioc->shost_recovery) {
		pr_info("%s %s: host recovery in progress!\n",
		       ioc->name, __func__);
		return -EAGAIN;
	}
	handle = transfer_packet->handle;
	if (handle == LEAPIORAID_INVALID_DEVICE_HANDLE) {
		pr_info("%s %s: no device!\n",
		       __func__, ioc->name);
		return -EFAULT;
	}
	mutex_lock(&ioc->scsih_cmds.mutex);
	if (ioc->scsih_cmds.status != LEAPIORAID_CMD_NOT_USED) {
		pr_err("%s %s: scsih_cmd in use\n",
		       ioc->name, __func__);
		rc = -EAGAIN;
		goto out;
	}
retry_loop:
	if (test_bit(handle, ioc->device_remove_in_progress)) {
		pr_info("%s %s: device removal in progress\n",
		       ioc->name, __func__);
		rc = -EFAULT;
		goto out;
	}
	ioc->scsih_cmds.status = LEAPIORAID_CMD_PENDING;
	rc = leapioraid_wait_for_ioc_to_operational(ioc, 10);
	if (rc)
		goto out;
	smid = ioc->shost->can_queue
		+ LEAPIORAID_INTERNAL_SCSIIO_FOR_DISCOVERY;
	rc = 0;
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->scsih_cmds.smid = smid;
	memset(mpi_request, 0, sizeof(struct LeapioSCSIIOReq_t));
	if (transfer_packet->is_raid)
		mpi_request->Function =
		    LEAPIORAID_FUNC_RAID_SCSI_IO_PASSTHROUGH;
	else
		mpi_request->Function = LEAPIORAID_FUNC_SCSI_IO_REQUEST;
	mpi_request->DevHandle = cpu_to_le16(handle);
	switch (transfer_packet->dir) {
	case DMA_TO_DEVICE:
		mpi_control = LEAPIORAID_SCSIIO_CONTROL_WRITE;
		data_out_dma = transfer_packet->data_dma;
		data_out_sz = transfer_packet->data_length;
		break;
	case DMA_FROM_DEVICE:
		mpi_control = LEAPIORAID_SCSIIO_CONTROL_READ;
		data_in_dma = transfer_packet->data_dma;
		data_in_sz = transfer_packet->data_length;
		break;
	case DMA_BIDIRECTIONAL:
		mpi_control = LEAPIORAID_SCSIIO_CONTROL_BIDIRECTIONAL;
		BUG();
		break;
	default:
	case DMA_NONE:
		mpi_control = LEAPIORAID_SCSIIO_CONTROL_NODATATRANSFER;
		break;
	}
	psge = &mpi_request->SGL;
	ioc->build_sg(
		ioc, psge, data_out_dma,
		data_out_sz, data_in_dma,
		data_in_sz);
	mpi_request->Control = cpu_to_le32(mpi_control |
					   LEAPIORAID_SCSIIO_CONTROL_SIMPLEQ);
	mpi_request->DataLength = cpu_to_le32(transfer_packet->data_length);
	mpi_request->MsgFlags = LEAPIORAID_SCSIIO_MSGFLAGS_SYSTEM_SENSE_ADDR;
	mpi_request->SenseBufferLength = SCSI_SENSE_BUFFERSIZE;
	mpi_request->SenseBufferLowAddress =
	    leapioraid_base_get_sense_buffer_dma(ioc, smid);
	priv_sense = leapioraid_base_get_sense_buffer(ioc, smid);
	mpi_request->SGLOffset0 = offsetof(struct LeapioSCSIIOReq_t, SGL) / 4;
	mpi_request->IoFlags = cpu_to_le16(transfer_packet->cdb_length);
	int_to_scsilun(transfer_packet->lun, (struct scsi_lun *)
		       mpi_request->LUN);
	memcpy(mpi_request->CDB.CDB32, transfer_packet->cdb,
	       transfer_packet->cdb_length);
	init_completion(&ioc->scsih_cmds.done);
	if (likely(mpi_request->Function == LEAPIORAID_FUNC_SCSI_IO_REQUEST))
		ioc->put_smid_scsi_io(ioc, smid, handle);
	else
		ioc->put_smid_default(ioc, smid);
	wait_for_completion_timeout(&ioc->scsih_cmds.done,
					       transfer_packet->timeout * HZ);
	if (!(ioc->scsih_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		leapioraid_check_cmd_timeout(ioc,
					     ioc->scsih_cmds.status,
					     mpi_request,
					     sizeof(struct LeapioSCSIIOReq_t) / 4,
					     issue_reset);
		goto issue_target_reset;
	}
	if (ioc->scsih_cmds.status & LEAPIORAID_CMD_REPLY_VALID) {
		transfer_packet->valid_reply = 1;
		mpi_reply = ioc->scsih_cmds.reply;
		transfer_packet->sense_length =
		    le32_to_cpu(mpi_reply->SenseCount);
		if (transfer_packet->sense_length)
			memcpy(transfer_packet->sense, priv_sense,
			       transfer_packet->sense_length);
		transfer_packet->transfer_length =
		    le32_to_cpu(mpi_reply->TransferCount);
		transfer_packet->ioc_status =
		    le16_to_cpu(mpi_reply->IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
		transfer_packet->scsi_state = mpi_reply->SCSIState;
		transfer_packet->scsi_status = mpi_reply->SCSIStatus;
		transfer_packet->log_info = le32_to_cpu(mpi_reply->IOCLogInfo);
	}
	goto out;
issue_target_reset:
	if (issue_reset) {
		pr_info("%s issue target reset: handle(0x%04x)\n", ioc->name, handle);
		tm_return_code =
		    leapioraid_scsihost_issue_locked_tm(ioc, handle,
				0xFFFFFFFF, 0xFFFFFFFF,
				0,
				LEAPIORAID_SCSITASKMGMT_TASKTYPE_TARGET_RESET,
				smid, tr_timeout,
				tr_method);
		if (tm_return_code == SUCCESS) {
			pr_err(
			       "%s target reset completed: handle (0x%04x)\n",
			       ioc->name, handle);
			if (((ioc->scsih_cmds.status & LEAPIORAID_CMD_COMPLETE)
			     && retry_count++ < 3)
			    || ((ioc->scsih_cmds.status & LEAPIORAID_CMD_RESET)
				&& host_reset_count++ == 0)) {
				pr_info("%s issue retry: handle (0x%04x)\n",
					ioc->name, handle);
				goto retry_loop;
			}
		} else
			pr_err("%s target reset didn't complete: handle(0x%04x)\n",
				ioc->name, handle);
		rc = -EFAULT;
	} else
		rc = -EAGAIN;
out:
	ioc->scsih_cmds.status = LEAPIORAID_CMD_NOT_USED;
	mutex_unlock(&ioc->scsih_cmds.mutex);
	return rc;
}

static enum device_responsive_state
leapioraid_scsihost_determine_disposition(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_scsi_io_transfer *transfer_packet)
{
	static enum device_responsive_state rc;
	struct sense_info sense_info = { 0, 0, 0 };
	u8 check_sense = 0;
	char *desc = NULL;

	if (!transfer_packet->valid_reply)
		return DEVICE_READY;
	switch (transfer_packet->ioc_status) {
	case LEAPIORAID_IOCSTATUS_BUSY:
	case LEAPIORAID_IOCSTATUS_INSUFFICIENT_RESOURCES:
	case LEAPIORAID_IOCSTATUS_SCSI_TASK_TERMINATED:
	case LEAPIORAID_IOCSTATUS_SCSI_IO_DATA_ERROR:
	case LEAPIORAID_IOCSTATUS_SCSI_EXT_TERMINATED:
		rc = DEVICE_RETRY;
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_IOC_TERMINATED:
		if (transfer_packet->log_info == 0x31170000) {
			rc = DEVICE_RETRY;
			break;
		}
		if (transfer_packet->cdb[0] == REPORT_LUNS)
			rc = DEVICE_READY;
		else
			rc = DEVICE_RETRY;
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_DATA_UNDERRUN:
	case LEAPIORAID_IOCSTATUS_SCSI_RECOVERED_ERROR:
	case LEAPIORAID_IOCSTATUS_SUCCESS:
		if (!transfer_packet->scsi_state &&
		    !transfer_packet->scsi_status) {
			rc = DEVICE_READY;
			break;
		}
		if (transfer_packet->scsi_state &
		    LEAPIORAID_SCSI_STATE_AUTOSENSE_VALID) {
			rc = DEVICE_ERROR;
			check_sense = 1;
			break;
		}
		if (transfer_packet->scsi_state &
		    (LEAPIORAID_SCSI_STATE_AUTOSENSE_FAILED |
		     LEAPIORAID_SCSI_STATE_NO_SCSI_STATUS |
		     LEAPIORAID_SCSI_STATE_TERMINATED)) {
			rc = DEVICE_RETRY;
			break;
		}
		if (transfer_packet->scsi_status >= LEAPIORAID_SCSI_STATUS_BUSY) {
			rc = DEVICE_RETRY;
			break;
		}
		rc = DEVICE_READY;
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_PROTOCOL_ERROR:
		if (transfer_packet->scsi_state & LEAPIORAID_SCSI_STATE_TERMINATED)
			rc = DEVICE_RETRY;
		else
			rc = DEVICE_ERROR;
		break;
	case LEAPIORAID_IOCSTATUS_INSUFFICIENT_POWER:
	default:
		rc = DEVICE_ERROR;
		break;
	}
	if (check_sense) {
		leapioraid_scsihost_normalize_sense(
			transfer_packet->sense, &sense_info);
		if (sense_info.skey == UNIT_ATTENTION)
			rc = DEVICE_RETRY_UA;
		else if (sense_info.skey == NOT_READY) {
			if (sense_info.asc == 0x3a)
				rc = DEVICE_READY;
			else if (sense_info.asc == 0x04) {
				if (sense_info.ascq == 0x03 ||
				    sense_info.ascq == 0x0b ||
				    sense_info.ascq == 0x0c) {
					rc = DEVICE_ERROR;
				} else
					rc = DEVICE_START_UNIT;
			} else if (sense_info.asc == 0x3e && !sense_info.ascq)
				rc = DEVICE_START_UNIT;
		} else if (sense_info.skey == ILLEGAL_REQUEST &&
			   transfer_packet->cdb[0] == REPORT_LUNS) {
			rc = DEVICE_READY;
		} else if (sense_info.skey == MEDIUM_ERROR) {
			if (sense_info.asc == 0x31)
				rc = DEVICE_READY;
		} else if (sense_info.skey == HARDWARE_ERROR) {
			if (sense_info.asc == 0x19)
				rc = DEVICE_READY;
		}
	}
	if (ioc->logging_level & LEAPIORAID_DEBUG_EVENT_WORK_TASK) {
		switch (rc) {
		case DEVICE_READY:
			desc = "ready";
			break;
		case DEVICE_RETRY:
			desc = "retry";
			break;
		case DEVICE_RETRY_UA:
			desc = "retry_ua";
			break;
		case DEVICE_START_UNIT:
			desc = "start_unit";
			break;
		case DEVICE_STOP_UNIT:
			desc = "stop_unit";
			break;
		case DEVICE_ERROR:
			desc = "error";
			break;
		}
		pr_info(
			"%s \tioc_status(0x%04x), loginfo(0x%08x),\n\t\t"
				"scsi_status(0x%02x), scsi_state(0x%02x), rc(%s)\n",
				ioc->name,
				transfer_packet->ioc_status,
				transfer_packet->log_info,
				transfer_packet->scsi_status,
				transfer_packet->scsi_state,
				desc);
		if (check_sense)
			pr_info("%s \t[sense_key,asc,ascq]: [0x%02x,0x%02x,0x%02x]\n",
				ioc->name,
				sense_info.skey, sense_info.asc,
				sense_info.ascq);
	}
	return rc;
}

static enum device_responsive_state
leapioraid_scsihost_inquiry_vpd_sn(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle,
	u8 **serial_number)
{
	struct leapioraid_scsi_io_transfer *transfer_packet;
	enum device_responsive_state rc;
	u8 *inq_data;
	int return_code;
	u32 data_length;
	u8 len;
	u8 tr_timeout = 30;
	u8 tr_method = 0;

	inq_data = NULL;
	transfer_packet
		= kzalloc(sizeof(struct leapioraid_scsi_io_transfer), GFP_KERNEL);
	if (!transfer_packet) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = DEVICE_RETRY;
		goto out;
	}
	data_length = 252;
	inq_data = dma_alloc_coherent(&ioc->pdev->dev, data_length,
				      &transfer_packet->data_dma, GFP_ATOMIC);
	if (!inq_data) {
		rc = DEVICE_RETRY;
		goto out;
	}

	rc = DEVICE_READY;
	memset(inq_data, 0, data_length);
	transfer_packet->handle = handle;
	transfer_packet->dir = DMA_FROM_DEVICE;
	transfer_packet->data_length = data_length;
	transfer_packet->cdb_length = 6;
	transfer_packet->cdb[0] = INQUIRY;
	transfer_packet->cdb[1] = 1;
	transfer_packet->cdb[2] = 0x80;
	transfer_packet->cdb[4] = data_length;
	transfer_packet->timeout = 30;
	tr_method = LEAPIORAID_SCSITASKMGMT_MSGFLAGS_LINK_RESET;
	return_code =
	    leapioraid_scsi_send_scsi_io(
			ioc, transfer_packet, tr_timeout, tr_method);
	switch (return_code) {
	case 0:
		rc = leapioraid_scsihost_determine_disposition(
			ioc, transfer_packet);
		if (rc == DEVICE_READY) {
			len = strlen(&inq_data[4]) + 1;
			*serial_number = kmalloc(len, GFP_KERNEL);
			if (*serial_number)
				strscpy(*serial_number, &inq_data[4], sizeof(*serial_number));
		}
		break;
	case -EAGAIN:
		rc = DEVICE_RETRY;
		break;
	case -EFAULT:
	default:
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = DEVICE_ERROR;
		break;
	}
out:
	if (inq_data)
		dma_free_coherent(&ioc->pdev->dev, data_length, inq_data,
				  transfer_packet->data_dma);
	kfree(transfer_packet);
	return rc;
}

static enum device_responsive_state
leapioraid_scsihost_inquiry_vpd_supported_pages(
			struct LEAPIORAID_ADAPTER *ioc,
			u16 handle, u32 lun, void *data,
			u32 data_length)
{
	struct leapioraid_scsi_io_transfer *transfer_packet;
	enum device_responsive_state rc;
	void *inq_data;
	int return_code;

	inq_data = NULL;
	transfer_packet = kzalloc(sizeof(struct leapioraid_scsi_io_transfer),
		GFP_KERNEL);
	if (!transfer_packet) {
		rc = DEVICE_RETRY;
		goto out;
	}
	inq_data = dma_alloc_coherent(&ioc->pdev->dev, data_length,
				      &transfer_packet->data_dma, GFP_ATOMIC);
	if (!inq_data) {
		rc = DEVICE_RETRY;
		goto out;
	}
	rc = DEVICE_READY;
	memset(inq_data, 0, data_length);
	transfer_packet->handle = handle;
	transfer_packet->dir = DMA_FROM_DEVICE;
	transfer_packet->data_length = data_length;
	transfer_packet->cdb_length = 6;
	transfer_packet->lun = lun;
	transfer_packet->cdb[0] = INQUIRY;
	transfer_packet->cdb[1] = 1;
	transfer_packet->cdb[4] = data_length;
	transfer_packet->timeout = 30;
	return_code = leapioraid_scsi_send_scsi_io(
		ioc, transfer_packet, 30, 0);
	switch (return_code) {
	case 0:
		rc = leapioraid_scsihost_determine_disposition(
			ioc, transfer_packet);
		if (rc == DEVICE_READY)
			memcpy(data, inq_data, data_length);
		break;
	case -EAGAIN:
		rc = DEVICE_RETRY;
		break;
	case -EFAULT:
	default:
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = DEVICE_ERROR;
		break;
	}
out:
	if (inq_data)
		dma_free_coherent(&ioc->pdev->dev, data_length, inq_data,
				  transfer_packet->data_dma);
	kfree(transfer_packet);
	return rc;
}

static enum device_responsive_state
leapioraid_scsihost_report_luns(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle, void *data,
	u32 data_length, u8 retry_count, u8 is_pd, u8 tr_timeout,
	u8 tr_method)
{
	struct leapioraid_scsi_io_transfer *transfer_packet;
	enum device_responsive_state rc;
	void *lun_data;
	int return_code;
	int retries;

	lun_data = NULL;
	transfer_packet = kzalloc(sizeof(struct leapioraid_scsi_io_transfer),
		GFP_KERNEL);
	if (!transfer_packet) {
		rc = DEVICE_RETRY;
		goto out;
	}
	lun_data = dma_alloc_coherent(&ioc->pdev->dev, data_length,
				      &transfer_packet->data_dma, GFP_ATOMIC);
	if (!lun_data) {
		rc = DEVICE_RETRY;
		goto out;
	}
	for (retries = 0; retries < 4; retries++) {
		rc = DEVICE_ERROR;
		pr_info("%s REPORT_LUNS: handle(0x%04x), retries(%d)\n",
			ioc->name, handle, retries);
		memset(lun_data, 0, data_length);
		transfer_packet->handle = handle;
		transfer_packet->dir = DMA_FROM_DEVICE;
		transfer_packet->data_length = data_length;
		transfer_packet->cdb_length = 12;
		transfer_packet->cdb[0] = REPORT_LUNS;
		transfer_packet->cdb[6] = (data_length >> 24) & 0xFF;
		transfer_packet->cdb[7] = (data_length >> 16) & 0xFF;
		transfer_packet->cdb[8] = (data_length >> 8) & 0xFF;
		transfer_packet->cdb[9] = data_length & 0xFF;
		transfer_packet->timeout = 30;
		transfer_packet->is_raid = is_pd;
		return_code =
		    leapioraid_scsi_send_scsi_io(ioc, transfer_packet, tr_timeout,
				       tr_method);
		switch (return_code) {
		case 0:
			rc = leapioraid_scsihost_determine_disposition(ioc,
							     transfer_packet);
			if (rc == DEVICE_READY) {
				memcpy(data, lun_data, data_length);
				goto out;
			} else if (rc == DEVICE_ERROR)
				goto out;
			break;
		case -EAGAIN:
			rc = DEVICE_RETRY;
			break;
		case -EFAULT:
		default:
			pr_err("%s failure at %s:%d/%s()!\n",
			       ioc->name, __FILE__, __LINE__, __func__);
			goto out;
		}
	}
out:
	if (lun_data)
		dma_free_coherent(&ioc->pdev->dev, data_length, lun_data,
				  transfer_packet->data_dma);
	kfree(transfer_packet);
	if ((rc == DEVICE_RETRY || rc == DEVICE_START_UNIT ||
	     rc == DEVICE_RETRY_UA) && retry_count >= 144)
		rc = DEVICE_ERROR;
	return rc;
}

static enum device_responsive_state
leapioraid_scsihost_start_unit(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle, u32 lun,
	u8 is_pd, u8 tr_timeout, u8 tr_method)
{
	struct leapioraid_scsi_io_transfer *transfer_packet;
	enum device_responsive_state rc;
	int return_code;

	transfer_packet = kzalloc(sizeof(struct leapioraid_scsi_io_transfer),
		GFP_KERNEL);
	if (!transfer_packet) {
		rc = DEVICE_RETRY;
		goto out;
	}

	rc = DEVICE_READY;
	transfer_packet->handle = handle;
	transfer_packet->dir = DMA_NONE;
	transfer_packet->lun = lun;
	transfer_packet->cdb_length = 6;
	transfer_packet->cdb[0] = START_STOP;
	transfer_packet->cdb[1] = 1;
	transfer_packet->cdb[4] = 1;
	transfer_packet->timeout = 30;
	transfer_packet->is_raid = is_pd;
	pr_info("%s START_UNIT: handle(0x%04x), lun(%d)\n",
		ioc->name, handle, lun);
	return_code =
	    leapioraid_scsi_send_scsi_io(
			ioc, transfer_packet, tr_timeout, tr_method);
	switch (return_code) {
	case 0:
		rc = leapioraid_scsihost_determine_disposition(
			ioc, transfer_packet);
		break;
	case -EAGAIN:
		rc = DEVICE_RETRY;
		break;
	case -EFAULT:
	default:
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = DEVICE_ERROR;
		break;
	}
out:
	kfree(transfer_packet);
	return rc;
}

static enum device_responsive_state
leapioraid_scsihost_test_unit_ready(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle, u32 lun,
	u8 is_pd, u8 tr_timeout, u8 tr_method)
{
	struct leapioraid_scsi_io_transfer *transfer_packet;
	enum device_responsive_state rc;
	int return_code;
	int sata_init_failure = 0;

	transfer_packet = kzalloc(sizeof(struct leapioraid_scsi_io_transfer),
		GFP_KERNEL);
	if (!transfer_packet) {
		rc = DEVICE_RETRY;
		goto out;
	}
	rc = DEVICE_READY;
	transfer_packet->handle = handle;
	transfer_packet->dir = DMA_NONE;
	transfer_packet->lun = lun;
	transfer_packet->cdb_length = 6;
	transfer_packet->cdb[0] = TEST_UNIT_READY;
	transfer_packet->timeout = 30;
	transfer_packet->is_raid = is_pd;
sata_init_retry:
	pr_info("%s TEST_UNIT_READY: handle(0x%04x), lun(%d)\n",
		ioc->name, handle, lun);
	return_code =
	    leapioraid_scsi_send_scsi_io(
			ioc, transfer_packet, tr_timeout, tr_method);
	switch (return_code) {
	case 0:
		rc = leapioraid_scsihost_determine_disposition(
			ioc, transfer_packet);
		if (rc == DEVICE_RETRY &&
		    transfer_packet->log_info == 0x31111000) {
			if (!sata_init_failure++) {
				pr_err(
					"%s SATA Initialization Timeout,sending a retry\n",
					ioc->name);
				rc = DEVICE_READY;
				goto sata_init_retry;
			} else {
				pr_err(
					"%s SATA Initialization Failed\n",
					ioc->name);
				rc = DEVICE_ERROR;
			}
		}
		break;
	case -EAGAIN:
		rc = DEVICE_RETRY;
		break;
	case -EFAULT:
	default:
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = DEVICE_ERROR;
		break;
	}
out:
	kfree(transfer_packet);
	return rc;
}

static enum device_responsive_state
leapioraid_scsihost_ata_pass_thru_idd(
		struct LEAPIORAID_ADAPTER *ioc, u16 handle,
		u8 *is_ssd_device, u8 tr_timeout, u8 tr_method)
{
	struct leapioraid_scsi_io_transfer *transfer_packet;
	enum device_responsive_state rc;
	u16 *idd_data;
	int return_code;
	u32 data_length;

	idd_data = NULL;
	transfer_packet = kzalloc(sizeof(struct leapioraid_scsi_io_transfer),
		GFP_KERNEL);
	if (!transfer_packet) {
		rc = DEVICE_RETRY;
		goto out;
	}
	data_length = 512;
	idd_data = dma_alloc_coherent(&ioc->pdev->dev, data_length,
				      &transfer_packet->data_dma, GFP_ATOMIC);
	if (!idd_data) {
		rc = DEVICE_RETRY;
		goto out;
	}
	rc = DEVICE_READY;
	memset(idd_data, 0, data_length);
	transfer_packet->handle = handle;
	transfer_packet->dir = DMA_FROM_DEVICE;
	transfer_packet->data_length = data_length;
	transfer_packet->cdb_length = 12;
	transfer_packet->cdb[0] = ATA_12;
	transfer_packet->cdb[1] = 0x8;
	transfer_packet->cdb[2] = 0xd;
	transfer_packet->cdb[3] = 0x1;
	transfer_packet->cdb[9] = 0xec;
	transfer_packet->timeout = 30;
	return_code = leapioraid_scsi_send_scsi_io(
		ioc, transfer_packet, 30, 0);
	switch (return_code) {
	case 0:
		rc = leapioraid_scsihost_determine_disposition(
			ioc, transfer_packet);
		if (rc == DEVICE_READY) {
			if (le16_to_cpu(idd_data[217]) == 1)
				*is_ssd_device = 1;
		}
		break;
	case -EAGAIN:
		rc = DEVICE_RETRY;
		break;
	case -EFAULT:
	default:
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = DEVICE_ERROR;
		break;
	}
out:
	if (idd_data) {
		dma_free_coherent(&ioc->pdev->dev, data_length, idd_data,
				  transfer_packet->data_dma);
	}
	kfree(transfer_packet);
	return rc;
}

static enum device_responsive_state
leapioraid_scsihost_wait_for_device_to_become_ready(
	struct LEAPIORAID_ADAPTER *ioc,
	u16 handle, u8 retry_count, u8 is_pd,
	int lun, u8 tr_timeout, u8 tr_method)
{
	enum device_responsive_state rc;

	if (ioc->pci_error_recovery)
		return DEVICE_ERROR;
	if (ioc->shost_recovery)
		return DEVICE_RETRY;
	rc = leapioraid_scsihost_test_unit_ready(
		ioc, handle, lun, is_pd, tr_timeout,
		tr_method);
	if (rc == DEVICE_READY || rc == DEVICE_ERROR)
		return rc;
	else if (rc == DEVICE_START_UNIT) {
		rc = leapioraid_scsihost_start_unit(
			ioc, handle, lun, is_pd, tr_timeout,
			tr_method);
		if (rc == DEVICE_ERROR)
			return rc;
		rc = leapioraid_scsihost_test_unit_ready(
			ioc, handle, lun, is_pd,
			tr_timeout, tr_method);
	}
	if ((rc == DEVICE_RETRY || rc == DEVICE_START_UNIT ||
	     rc == DEVICE_RETRY_UA) && retry_count >= 144)
		rc = DEVICE_ERROR;
	return rc;
}

static enum device_responsive_state
leapioraid_scsihost_wait_for_target_to_become_ready(
	struct LEAPIORAID_ADAPTER *ioc,
	u16 handle, u8 retry_count, u8 is_pd,
	u8 tr_timeout, u8 tr_method)
{
	enum device_responsive_state rc;
	struct scsi_lun *lun_data;
	u32 length, num_luns;
	u8 *data;
	int lun;
	struct scsi_lun *lunp;

	lun_data =
	    kcalloc(255, sizeof(struct scsi_lun), GFP_KERNEL);
	if (!lun_data) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return DEVICE_RETRY;
	}
	rc = leapioraid_scsihost_report_luns(ioc, handle, lun_data,
				   255 * sizeof(struct scsi_lun),
				   retry_count, is_pd, tr_timeout, tr_method);
	if (rc != DEVICE_READY)
		goto out;
	data = (u8 *) lun_data;
	length = ((data[0] << 24) | (data[1] << 16) |
		  (data[2] << 8) | (data[3] << 0));
	num_luns = (length / sizeof(struct scsi_lun));
	lunp = &lun_data[1];
	lun = (num_luns) ? scsilun_to_int(&lun_data[1]) : 0;
	rc = leapioraid_scsihost_wait_for_device_to_become_ready(
		ioc, handle, retry_count,
		is_pd, lun, tr_timeout,
		tr_method);
	if (rc == DEVICE_ERROR) {
		struct scsi_lun *lunq;

		for (lunq = lunp++; lunq <= &lun_data[num_luns]; lunq++) {
			rc = leapioraid_scsihost_wait_for_device_to_become_ready(ioc,
								       handle,
								       retry_count,
								       is_pd,
								       scsilun_to_int
								       (lunq),
								       tr_timeout,
								       tr_method);
			if (rc != DEVICE_ERROR)
				goto out;
		}
	}
out:
	kfree(lun_data);
	return rc;
}

static u8
leapioraid_scsihost_check_access_status(
	struct LEAPIORAID_ADAPTER *ioc, u64 sas_address,
	u16 handle, u8 access_status)
{
	u8 rc = 1;
	char *desc = NULL;

	switch (access_status) {
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_NO_ERRORS:
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SATA_NEEDS_INITIALIZATION:
		rc = 0;
		break;
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SATA_CAPABILITY_FAILED:
		desc = "sata capability failed";
		break;
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SATA_AFFILIATION_CONFLICT:
		desc = "sata affiliation conflict";
		break;
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_ROUTE_NOT_ADDRESSABLE:
		desc = "route not addressable";
		break;
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SMP_ERROR_NOT_ADDRESSABLE:
		desc = "smp error not addressable";
		break;
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_DEVICE_BLOCKED:
		desc = "device blocked";
		break;
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SATA_INIT_FAILED:
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SIF_UNKNOWN:
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SIF_AFFILIATION_CONFLICT:
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SIF_DIAG:
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SIF_IDENTIFICATION:
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SIF_CHECK_POWER:
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SIF_PIO_SN:
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SIF_MDMA_SN:
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SIF_UDMA_SN:
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SIF_ZONING_VIOLATION:
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SIF_NOT_ADDRESSABLE:
	case LEAPIORAID_SAS_DEVICE0_ASTATUS_SIF_MAX:
		desc = "sata initialization failed";
		break;
	default:
		desc = "unknown";
		break;
	}
	if (!rc)
		return 0;
	pr_err(
		"%s discovery errors(%s): sas_address(0x%016llx),\n\t\t"
		"handle(0x%04x)\n",
			ioc->name,
			desc,
			(unsigned long long)sas_address,
			handle);
	return rc;
}

static void
leapioraid_scsihost_check_device(struct LEAPIORAID_ADAPTER *ioc,
		       u64 parent_sas_address, u16 handle, u8 phy_number,
		       u8 link_rate)
{
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidSasDevP0_t sas_device_pg0;
	struct leapioraid_sas_device *sas_device = NULL;
	struct leapioraid_enclosure_node *enclosure_dev = NULL;
	u32 ioc_status;
	unsigned long flags;
	u64 sas_address;
	struct scsi_target *starget;
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	u32 device_info;
	u8 *serial_number = NULL;
	u8 *original_serial_number = NULL;
	int rc;
	struct leapioraid_hba_port *port;

	if ((leapioraid_config_get_sas_device_pg0
	     (ioc, &mpi_reply, &sas_device_pg0,
	      LEAPIORAID_SAS_DEVICE_PGAD_FORM_HANDLE, handle)))
		return;
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus)
		& LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS)
		return;
	if (phy_number != sas_device_pg0.PhyNum)
		return;
	device_info = le32_to_cpu(sas_device_pg0.DeviceInfo);
	if (!(leapioraid_scsihost_is_sas_end_device(device_info)))
		return;
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_address = le64_to_cpu(sas_device_pg0.SASAddress);
	port = leapioraid_get_port_by_id(ioc, sas_device_pg0.PhysicalPort, 0);
	if (!port)
		goto out_unlock;
	sas_device = __leapioraid_get_sdev_by_addr(ioc, sas_address, port);
	if (!sas_device)
		goto out_unlock;
	if (unlikely(sas_device->handle != handle)) {
		starget = sas_device->starget;
		sas_target_priv_data = starget->hostdata;
		starget_printk(KERN_INFO, starget,
			"handle changed from(0x%04x) to (0x%04x)!!!\n",
			sas_device->handle, handle);
		sas_target_priv_data->handle = handle;
		sas_device->handle = handle;
		if (le16_to_cpu(sas_device_pg0.Flags) &
		    LEAPIORAID_SAS_DEVICE0_FLAGS_ENCL_LEVEL_VALID) {
			sas_device->enclosure_level =
			    sas_device_pg0.EnclosureLevel;
			memcpy(sas_device->connector_name,
			       sas_device_pg0.ConnectorName, 4);
			sas_device->connector_name[4] = '\0';
		} else {
			sas_device->enclosure_level = 0;
			sas_device->connector_name[0] = '\0';
		}
		sas_device->enclosure_handle =
		    le16_to_cpu(sas_device_pg0.EnclosureHandle);
		sas_device->is_chassis_slot_valid = 0;
		enclosure_dev =
		    leapioraid_scsihost_enclosure_find_by_handle(ioc,
								 sas_device->enclosure_handle);
		if (enclosure_dev) {
			sas_device->enclosure_logical_id =
			    le64_to_cpu(enclosure_dev->pg0.EnclosureLogicalID);
			if (le16_to_cpu(enclosure_dev->pg0.Flags) &
			    LEAPIORAID_SAS_ENCLS0_FLAGS_CHASSIS_SLOT_VALID) {
				sas_device->is_chassis_slot_valid = 1;
				sas_device->chassis_slot =
				    enclosure_dev->pg0.ChassisSlot;
			}
		}
	}
	if (!(le16_to_cpu(sas_device_pg0.Flags) &
	      LEAPIORAID_SAS_DEVICE0_FLAGS_DEVICE_PRESENT)) {
		pr_err("%s device is not present handle(0x%04x), flags!!!\n",
			ioc->name, handle);
		goto out_unlock;
	}
	if (leapioraid_scsihost_check_access_status(ioc, sas_address, handle,
					  sas_device_pg0.AccessStatus))
		goto out_unlock;
	original_serial_number = sas_device->serial_number;
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	leapioraid_scsihost_ublock_io_device_wait(ioc, sas_address, port);
	if (!original_serial_number)
		goto out;
	if (leapioraid_scsihost_inquiry_vpd_sn(ioc, handle, &serial_number) ==
	    DEVICE_READY && serial_number) {
		rc = strcmp(original_serial_number, serial_number);
		kfree(serial_number);
		if (!rc)
			goto out;
		leapioraid_device_remove_by_sas_address(ioc, sas_address, port);
		leapioraid_transport_update_links(ioc, parent_sas_address,
						  handle, phy_number, link_rate,
						  port);
		leapioraid_scsihost_add_device(ioc, handle, 0, 0);
	}
	goto out;
out_unlock:
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
out:
	if (sas_device)
		leapioraid_sas_device_put(sas_device);
}

static int
leapioraid_scsihost_add_device(
		struct LEAPIORAID_ADAPTER *ioc, u16 handle, u8 retry_count,
		u8 is_pd)
{
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidSasDevP0_t sas_device_pg0;
	struct leapioraid_sas_device *sas_device;
	struct leapioraid_enclosure_node *enclosure_dev = NULL;
	u32 ioc_status;
	u64 sas_address;
	u32 device_info;
	enum device_responsive_state rc;
	u8 connector_name[5], port_id;

	if ((leapioraid_config_get_sas_device_pg0
	     (ioc, &mpi_reply, &sas_device_pg0,
	      LEAPIORAID_SAS_DEVICE_PGAD_FORM_HANDLE, handle))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return 0;
	}
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus)
		& LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return 0;
	}
	device_info = le32_to_cpu(sas_device_pg0.DeviceInfo);
	if (!(leapioraid_scsihost_is_sas_end_device(device_info)))
		return 0;
	set_bit(handle, ioc->pend_os_device_add);
	sas_address = le64_to_cpu(sas_device_pg0.SASAddress);
	if (!(le16_to_cpu(sas_device_pg0.Flags) &
	      LEAPIORAID_SAS_DEVICE0_FLAGS_DEVICE_PRESENT)) {
		pr_err("%s device is not present handle(0x04%x)!!!\n",
			ioc->name, handle);
		return 0;
	}
	if (leapioraid_scsihost_check_access_status(
		ioc, sas_address, handle,
		sas_device_pg0.AccessStatus))
		return 0;
	port_id = sas_device_pg0.PhysicalPort;
	sas_device = leapioraid_get_sdev_by_addr(ioc,
		sas_address,
		leapioraid_get_port_by_id(ioc, port_id, 0));
	if (sas_device) {
		clear_bit(handle, ioc->pend_os_device_add);
		leapioraid_sas_device_put(sas_device);
		return 0;
	}
	if (le16_to_cpu(sas_device_pg0.EnclosureHandle)) {
		enclosure_dev =
		    leapioraid_scsihost_enclosure_find_by_handle(ioc,
								 le16_to_cpu
								 (sas_device_pg0.EnclosureHandle));
		if (enclosure_dev == NULL)
			pr_info(
				"%s Enclosure handle(0x%04x)doesn't\n\t\t"
				"match with enclosure device!\n",
					ioc->name,
					le16_to_cpu(sas_device_pg0.EnclosureHandle));
	}
	if (!ioc->wait_for_discovery_to_complete) {
		pr_info(
			"%s detecting: handle(0x%04x), sas_address(0x%016llx), phy(%d)\n",
			ioc->name, handle,
			(unsigned long long)sas_address,
			sas_device_pg0.PhyNum);
		rc = leapioraid_scsihost_wait_for_target_to_become_ready(
			ioc, handle,
			retry_count,
			is_pd, 30, 0);
		if (rc != DEVICE_READY) {
			if (le16_to_cpu(sas_device_pg0.EnclosureHandle) != 0)
				dewtprintk(ioc,
					pr_info("%s %s: device not ready: slot(%d)\n",
						ioc->name, __func__,
						le16_to_cpu(sas_device_pg0.Slot)));
			if (le16_to_cpu(sas_device_pg0.Flags) &
			    LEAPIORAID_SAS_DEVICE0_FLAGS_ENCL_LEVEL_VALID) {
				memcpy(connector_name,
				       sas_device_pg0.ConnectorName, 4);
				connector_name[4] = '\0';
				dewtprintk(ioc,
					pr_info(
						"%s %s: device not ready: enclosure level(0x%04x), connector name( %s)\n",
						ioc->name, __func__,
						sas_device_pg0.EnclosureLevel,
						connector_name));
			}
			if ((enclosure_dev)
			    && (le16_to_cpu(enclosure_dev->pg0.Flags) &
				LEAPIORAID_SAS_ENCLS0_FLAGS_CHASSIS_SLOT_VALID))
				pr_err(
					"%s chassis slot(0x%04x)\n", ioc->name,
					enclosure_dev->pg0.ChassisSlot);
			if (rc == DEVICE_RETRY || rc == DEVICE_START_UNIT
			    || rc == DEVICE_STOP_UNIT || rc == DEVICE_RETRY_UA)
				return 1;
			else if (rc == DEVICE_ERROR)
				return 0;
		}
	}
	sas_device = kzalloc(sizeof(struct leapioraid_sas_device),
		GFP_KERNEL);
	if (!sas_device)
		return 0;

	kref_init(&sas_device->refcount);
	sas_device->handle = handle;
	if (leapioraid_scsihost_get_sas_address(ioc,
			le16_to_cpu(sas_device_pg0.ParentDevHandle),
			&sas_device->sas_address_parent) != 0)
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
	sas_device->enclosure_handle =
	    le16_to_cpu(sas_device_pg0.EnclosureHandle);
	if (sas_device->enclosure_handle != 0)
		sas_device->slot = le16_to_cpu(sas_device_pg0.Slot);
	sas_device->device_info = device_info;
	sas_device->sas_address = sas_address;
	sas_device->port = leapioraid_get_port_by_id(ioc, port_id, 0);
	if (!sas_device->port) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out;
	}
	sas_device->phy = sas_device_pg0.PhyNum;
	sas_device->fast_path = (le16_to_cpu(sas_device_pg0.Flags) &
				 LEAPIORAID_SAS_DEVICE0_FLAGS_FAST_PATH_CAPABLE) ?
	    1 : 0;
	sas_device->supports_sata_smart =
	    (le16_to_cpu(sas_device_pg0.Flags) &
	     LEAPIORAID_SAS_DEVICE0_FLAGS_SATA_SMART_SUPPORTED);
	if (le16_to_cpu(sas_device_pg0.Flags) &
	    LEAPIORAID_SAS_DEVICE0_FLAGS_ENCL_LEVEL_VALID) {
		sas_device->enclosure_level = sas_device_pg0.EnclosureLevel;
		memcpy(sas_device->connector_name,
		       sas_device_pg0.ConnectorName, 4);
		sas_device->connector_name[4] = '\0';
	} else {
		sas_device->enclosure_level = 0;
		sas_device->connector_name[0] = '\0';
	}
	sas_device->is_chassis_slot_valid = 0;
	if (enclosure_dev) {
		sas_device->enclosure_logical_id =
		    le64_to_cpu(enclosure_dev->pg0.EnclosureLogicalID);
		if (le16_to_cpu(enclosure_dev->pg0.Flags) &
		    LEAPIORAID_SAS_ENCLS0_FLAGS_CHASSIS_SLOT_VALID) {
			sas_device->is_chassis_slot_valid = 1;
			sas_device->chassis_slot =
			    enclosure_dev->pg0.ChassisSlot;
		}
	}
	sas_device->device_name = le64_to_cpu(sas_device_pg0.DeviceName);
	sas_device->port_type = sas_device_pg0.MaxPortConnections;
	pr_err(
	       "%s handle(0x%0x) sas_address(0x%016llx) port_type(0x%0x)\n",
	       ioc->name, handle, sas_device->sas_address,
	       sas_device->port_type);
	if (ioc->wait_for_discovery_to_complete)
		leapioraid_scsihost_sas_device_init_add(ioc, sas_device);
	else
		leapioraid_scsihost_sas_device_add(ioc, sas_device);
out:
	leapioraid_sas_device_put(sas_device);
	return 0;
}

static void
leapioraid_scsihost_remove_device(struct LEAPIORAID_ADAPTER *ioc,
			struct leapioraid_sas_device *sas_device)
{
	struct LEAPIORAID_TARGET *sas_target_priv_data;

	if (sas_device->pfa_led_on) {
		leapioraid_scsihost_turn_off_pfa_led(ioc, sas_device);
		sas_device->pfa_led_on = 0;
	}
	dewtprintk(ioc, pr_info(
		"%s %s: enter: handle(0x%04x), sas_addr(0x%016llx)\n",
		ioc->name, __func__, sas_device->handle,
		(unsigned long long)sas_device->sas_address));
	dewtprintk(ioc,
		   leapioraid_scsihost_display_enclosure_chassis_info(
				ioc, sas_device, NULL, NULL));
	if (sas_device->starget && sas_device->starget->hostdata) {
		sas_target_priv_data = sas_device->starget->hostdata;
		sas_target_priv_data->deleted = 1;
		leapioraid_scsihost_ublock_io_device(
			ioc, sas_device->sas_address,
			sas_device->port);
		sas_target_priv_data->handle =
		    LEAPIORAID_INVALID_DEVICE_HANDLE;
	}
	if (!ioc->hide_drives)
		leapioraid_transport_port_remove(ioc,
			sas_device->sas_address,
			sas_device->sas_address_parent,
			sas_device->port);
	pr_info("%s removing handle(0x%04x), sas_addr(0x%016llx)\n",
		ioc->name, sas_device->handle,
	    (unsigned long long)sas_device->sas_address);
	leapioraid_scsihost_display_enclosure_chassis_info(ioc, sas_device, NULL, NULL);
	dewtprintk(ioc, pr_info(
		"%s %s: exit: handle(0x%04x), sas_addr(0x%016llx)\n",
		ioc->name, __func__, sas_device->handle,
		(unsigned long long)
		sas_device->sas_address));
	dewtprintk(ioc,
		   leapioraid_scsihost_display_enclosure_chassis_info(
				ioc, sas_device, NULL, NULL));
	kfree(sas_device->serial_number);
}

static void
leapioraid_scsihost_sas_topology_change_event_debug(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidEventDataSasTopoChangeList_t *event_data)
{
	int i;
	u16 handle;
	u16 reason_code;
	u8 phy_number;
	char *status_str = NULL;
	u8 link_rate, prev_link_rate;

	switch (event_data->ExpStatus) {
	case LEAPIORAID_EVENT_SAS_TOPO_ES_ADDED:
		status_str = "add";
		break;
	case LEAPIORAID_EVENT_SAS_TOPO_ES_NOT_RESPONDING:
		status_str = "remove";
		break;
	case LEAPIORAID_EVENT_SAS_TOPO_ES_RESPONDING:
	case 0:
		status_str = "responding";
		break;
	case LEAPIORAID_EVENT_SAS_TOPO_ES_DELAY_NOT_RESPONDING:
		status_str = "remove delay";
		break;
	default:
		status_str = "unknown status";
		break;
	}
	pr_info("%s sas topology change: (%s)\n",
	       ioc->name, status_str);
	pr_info(
		"\thandle(0x%04x), enclosure_handle(0x%04x)\n\t\t"
		"start_phy(%02d), count(%d)\n",
			le16_to_cpu(event_data->ExpanderDevHandle),
			le16_to_cpu(event_data->EnclosureHandle),
			event_data->StartPhyNum,
			event_data->NumEntries);
	for (i = 0; i < event_data->NumEntries; i++) {
		handle = le16_to_cpu(event_data->PHY[i].AttachedDevHandle);
		if (!handle)
			continue;
		phy_number = event_data->StartPhyNum + i;
		reason_code = event_data->PHY[i].PhyStatus &
		    LEAPIORAID_EVENT_SAS_TOPO_RC_MASK;
		switch (reason_code) {
		case LEAPIORAID_EVENT_SAS_TOPO_RC_TARG_ADDED:
			status_str = "target add";
			break;
		case LEAPIORAID_EVENT_SAS_TOPO_RC_TARG_NOT_RESPONDING:
			status_str = "target remove";
			break;
		case LEAPIORAID_EVENT_SAS_TOPO_RC_DELAY_NOT_RESPONDING:
			status_str = "delay target remove";
			break;
		case LEAPIORAID_EVENT_SAS_TOPO_RC_PHY_CHANGED:
			status_str = "link rate change";
			break;
		case LEAPIORAID_EVENT_SAS_TOPO_RC_NO_CHANGE:
			status_str = "target responding";
			break;
		default:
			status_str = "unknown";
			break;
		}
		link_rate = event_data->PHY[i].LinkRate >> 4;
		prev_link_rate = event_data->PHY[i].LinkRate & 0xF;
		pr_info(
			"\tphy(%02d), attached_handle(0x%04x): %s:\n\t\t"
				"link rate: new(0x%02x), old(0x%02x)\n",
					phy_number,
					handle,
					status_str,
					link_rate,
					prev_link_rate);
	}
}

static int
leapioraid_scsihost_sas_topology_change_event(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_fw_event_work *fw_event)
{
	int i;
	u16 parent_handle, handle;
	u16 reason_code;
	u8 phy_number, max_phys;
	struct leapioraid_raid_sas_node *sas_expander;
	struct leapioraid_sas_device *sas_device;
	u64 sas_address;
	unsigned long flags;
	u8 link_rate, prev_link_rate;
	int rc;
	int requeue_event;
	struct leapioraid_hba_port *port;
	struct LeapioraidEventDataSasTopoChangeList_t *event_data =
	    fw_event->event_data;

	if (ioc->logging_level & LEAPIORAID_DEBUG_EVENT_WORK_TASK)
		leapioraid_scsihost_sas_topology_change_event_debug(
			ioc, event_data);
	if (ioc->shost_recovery || ioc->remove_host || ioc->pci_error_recovery)
		return 0;
	if (!ioc->sas_hba.num_phys)
		leapioraid_scsihost_sas_host_add(ioc);
	else
		leapioraid_scsihost_sas_host_refresh(ioc);
	if (fw_event->ignore) {
		dewtprintk(ioc,
			pr_info("%s ignoring expander event\n",
				ioc->name));
		return 0;
	}
	parent_handle = le16_to_cpu(event_data->ExpanderDevHandle);
	port = leapioraid_get_port_by_id(ioc, event_data->PhysicalPort, 0);
	if (event_data->ExpStatus == LEAPIORAID_EVENT_SAS_TOPO_ES_ADDED)
		if (leapioraid_scsihost_expander_add(ioc, parent_handle) != 0)
			return 0;
	spin_lock_irqsave(&ioc->sas_node_lock, flags);
	sas_expander = leapioraid_scsihost_expander_find_by_handle(
		ioc, parent_handle);
	if (sas_expander) {
		sas_address = sas_expander->sas_address;
		max_phys = sas_expander->num_phys;
		port = sas_expander->port;
	} else if (parent_handle < ioc->sas_hba.num_phys) {
		sas_address = ioc->sas_hba.sas_address;
		max_phys = ioc->sas_hba.num_phys;
	} else {
		spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
		return 0;
	}
	spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
	for (i = 0, requeue_event = 0; i < event_data->NumEntries; i++) {
		if (fw_event->ignore) {
			dewtprintk(ioc, pr_info(
				"%s ignoring expander event\n",
				ioc->name));
			return 0;
		}
		if (ioc->remove_host || ioc->pci_error_recovery)
			return 0;
		phy_number = event_data->StartPhyNum + i;
		if (phy_number >= max_phys)
			continue;
		reason_code = event_data->PHY[i].PhyStatus &
		    LEAPIORAID_EVENT_SAS_TOPO_RC_MASK;
		if ((event_data->PHY[i].PhyStatus &
		     LEAPIORAID_EVENT_SAS_TOPO_PHYSTATUS_VACANT) && (reason_code !=
				LEAPIORAID_EVENT_SAS_TOPO_RC_TARG_NOT_RESPONDING))
			continue;
		if (fw_event->delayed_work_active && (reason_code ==
			LEAPIORAID_EVENT_SAS_TOPO_RC_TARG_NOT_RESPONDING)) {
			dewtprintk(ioc,
				pr_info(
					"%s ignoring Targ not responding\n\t\t"
					"event phy in re-queued event processing\n",
					ioc->name));
			continue;
		}
		handle = le16_to_cpu(event_data->PHY[i].AttachedDevHandle);
		if (!handle)
			continue;
		link_rate = event_data->PHY[i].LinkRate >> 4;
		prev_link_rate = event_data->PHY[i].LinkRate & 0xF;
		switch (reason_code) {
		case LEAPIORAID_EVENT_SAS_TOPO_RC_PHY_CHANGED:
			if (ioc->shost_recovery)
				break;
			if (link_rate == prev_link_rate)
				break;
			leapioraid_transport_update_links(ioc, sas_address,
							  handle, phy_number,
							  link_rate, port);
			if (link_rate < LEAPIORAID_SAS_NEG_LINK_RATE_1_5)
				break;
			leapioraid_scsihost_check_device(ioc, sas_address, handle,
					       phy_number, link_rate);
			spin_lock_irqsave(&ioc->sas_device_lock, flags);
			sas_device = __leapioraid_get_sdev_by_handle(ioc,
								     handle);
			spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
			if (sas_device) {
				leapioraid_sas_device_put(sas_device);
				break;
			}
			if (!test_bit(handle, ioc->pend_os_device_add))
				break;
			dewtprintk(ioc, pr_err(
				"%s handle(0x%04x) device not found:\n\t\t"
					"convert event to a device add\n",
						ioc->name,  handle));
			event_data->PHY[i].PhyStatus &= 0xF0;
			event_data->PHY[i].PhyStatus |=
			    LEAPIORAID_EVENT_SAS_TOPO_RC_TARG_ADDED;
			fallthrough;
		case LEAPIORAID_EVENT_SAS_TOPO_RC_TARG_ADDED:
			if (ioc->shost_recovery)
				break;
			leapioraid_transport_update_links(ioc, sas_address,
							  handle, phy_number,
							  link_rate, port);
			if (link_rate < LEAPIORAID_SAS_NEG_LINK_RATE_1_5)
				break;
			rc = leapioraid_scsihost_add_device(ioc, handle,
						  fw_event->retries[i], 0);
			if (rc) {
				fw_event->retries[i]++;
				requeue_event = 1;
			} else {
				event_data->PHY[i].PhyStatus |=
				    LEAPIORAID_EVENT_SAS_TOPO_PHYSTATUS_VACANT;
			}
			break;
		case LEAPIORAID_EVENT_SAS_TOPO_RC_TARG_NOT_RESPONDING:
			leapioraid_scsihost_device_remove_by_handle(ioc, handle);
			break;
		}
	}
	if (event_data->ExpStatus == LEAPIORAID_EVENT_SAS_TOPO_ES_NOT_RESPONDING
		&& sas_expander)
		leapioraid_expander_remove(ioc, sas_address, port);
	return requeue_event;
}

static void
leapioraid_scsihost_sas_device_status_change_event_debug(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidEventDataSasDeviceStatusChange_t *event_data)
{
	char *reason_str = NULL;

	switch (event_data->ReasonCode) {
	case LEAPIORAID_EVENT_SAS_DEV_STAT_RC_SMART_DATA:
		reason_str = "smart data";
		break;
	case LEAPIORAID_EVENT_SAS_DEV_STAT_RC_UNSUPPORTED:
		reason_str = "unsupported device discovered";
		break;
	case LEAPIORAID_EVENT_SAS_DEV_STAT_RC_INTERNAL_DEVICE_RESET:
		reason_str = "internal device reset";
		break;
	case LEAPIORAID_EVENT_SAS_DEV_STAT_RC_TASK_ABORT_INTERNAL:
		reason_str = "internal task abort";
		break;
	case LEAPIORAID_EVENT_SAS_DEV_STAT_RC_ABORT_TASK_SET_INTERNAL:
		reason_str = "internal task abort set";
		break;
	case LEAPIORAID_EVENT_SAS_DEV_STAT_RC_CLEAR_TASK_SET_INTERNAL:
		reason_str = "internal clear task set";
		break;
	case LEAPIORAID_EVENT_SAS_DEV_STAT_RC_QUERY_TASK_INTERNAL:
		reason_str = "internal query task";
		break;
	case LEAPIORAID_EVENT_SAS_DEV_STAT_RC_SATA_INIT_FAILURE:
		reason_str = "sata init failure";
		break;
	case LEAPIORAID_EVENT_SAS_DEV_STAT_RC_CMP_INTERNAL_DEV_RESET:
		reason_str = "internal device reset complete";
		break;
	case LEAPIORAID_EVENT_SAS_DEV_STAT_RC_CMP_TASK_ABORT_INTERNAL:
		reason_str = "internal task abort complete";
		break;
	case LEAPIORAID_EVENT_SAS_DEV_STAT_RC_ASYNC_NOTIFICATION:
		reason_str = "internal async notification";
		break;
	case LEAPIORAID_EVENT_SAS_DEV_STAT_RC_EXPANDER_REDUCED_FUNCTIONALITY:
		reason_str = "expander reduced functionality";
		break;
	case LEAPIORAID_EVENT_SAS_DEV_STAT_RC_CMP_EXPANDER_REDUCED_FUNCTIONALITY:
		reason_str = "expander reduced functionality complete";
		break;
	default:
		reason_str = "unknown reason";
		break;
	}
	pr_info("%s device status change: (%s)\n"
	       "\thandle(0x%04x), sas address(0x%016llx), tag(%d)",
	       ioc->name, reason_str, le16_to_cpu(event_data->DevHandle),
	       (unsigned long long)le64_to_cpu(event_data->SASAddress),
	       le16_to_cpu(event_data->TaskTag));
	if (event_data->ReasonCode == LEAPIORAID_EVENT_SAS_DEV_STAT_RC_SMART_DATA)
		pr_info("%s , ASC(0x%x), ASCQ(0x%x)\n",
		       ioc->name, event_data->ASC, event_data->ASCQ);
	pr_info("\n");
}

static void
leapioraid_scsihost_sas_device_status_change_event(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidEventDataSasDeviceStatusChange_t *event_data)
{
	struct LEAPIORAID_TARGET *target_priv_data;
	struct leapioraid_sas_device *sas_device;
	u64 sas_address;
	unsigned long flags;

	if ((ioc->facts.HeaderVersion >> 8) < 0xC)
		return;
	if (event_data->ReasonCode !=
	    LEAPIORAID_EVENT_SAS_DEV_STAT_RC_INTERNAL_DEVICE_RESET &&
	    event_data->ReasonCode !=
	    LEAPIORAID_EVENT_SAS_DEV_STAT_RC_CMP_INTERNAL_DEV_RESET)
		return;
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_address = le64_to_cpu(event_data->SASAddress);
	sas_device = __leapioraid_get_sdev_by_addr(
		ioc, sas_address,
		leapioraid_get_port_by_id(ioc, event_data->PhysicalPort, 0));
	if (!sas_device || !sas_device->starget)
		goto out;
	target_priv_data = sas_device->starget->hostdata;
	if (!target_priv_data)
		goto out;
	if (event_data->ReasonCode ==
	    LEAPIORAID_EVENT_SAS_DEV_STAT_RC_INTERNAL_DEVICE_RESET)
		target_priv_data->tm_busy = 1;
	else
		target_priv_data->tm_busy = 0;
	if (ioc->logging_level & LEAPIORAID_DEBUG_EVENT_WORK_TASK)
		pr_err(
			"%s %s tm_busy flag for handle(0x%04x)\n", ioc->name,
			(target_priv_data->tm_busy == 1) ? "Enable" : "Disable",
			target_priv_data->handle);
out:
	if (sas_device)
		leapioraid_sas_device_put(sas_device);
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
}

static void
leapioraid_scsihost_sas_enclosure_dev_status_change_event_debug(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidEventDataSasEnclDevStatusChange_t *event_data)
{
	char *reason_str = NULL;

	switch (event_data->ReasonCode) {
	case LEAPIORAID_EVENT_SAS_ENCL_RC_ADDED:
		reason_str = "enclosure add";
		break;
	case LEAPIORAID_EVENT_SAS_ENCL_RC_NOT_RESPONDING:
		reason_str = "enclosure remove";
		break;
	default:
		reason_str = "unknown reason";
		break;
	}
	pr_info(
		"%s enclosure status change: (%s)\n\thandle(0x%04x),\n\t\t"
		"enclosure logical id(0x%016llx) number slots(%d)\n",
			ioc->name,
			reason_str,
			le16_to_cpu(event_data->EnclosureHandle),
			(unsigned long long)le64_to_cpu(event_data->EnclosureLogicalID),
			le16_to_cpu(event_data->StartSlot));
}

static void
leapioraid_scsihost_sas_enclosure_dev_status_change_event(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_fw_event_work *fw_event)
{
	struct LeapioraidCfgRep_t mpi_reply;
	struct leapioraid_enclosure_node *enclosure_dev = NULL;
	struct LeapioraidEventDataSasEnclDevStatusChange_t *event_data =
	    fw_event->event_data;
	int rc;

	if (ioc->logging_level & LEAPIORAID_DEBUG_EVENT_WORK_TASK)
		leapioraid_scsihost_sas_enclosure_dev_status_change_event_debug(
			ioc, fw_event->event_data);
	if (ioc->shost_recovery)
		return;
	event_data->EnclosureHandle = le16_to_cpu(event_data->EnclosureHandle);
	if (event_data->EnclosureHandle)
		enclosure_dev =
		    leapioraid_scsihost_enclosure_find_by_handle(ioc,
								 event_data->EnclosureHandle);
	switch (event_data->ReasonCode) {
	case LEAPIORAID_EVENT_SAS_ENCL_RC_ADDED:
		if (!enclosure_dev) {
			enclosure_dev =
			    kzalloc(sizeof(struct leapioraid_enclosure_node), GFP_KERNEL);
			if (!enclosure_dev) {
				pr_err("%s failure at %s:%d/%s()!\n", ioc->name,
				       __FILE__, __LINE__, __func__);
				return;
			}
			rc = leapioraid_config_get_enclosure_pg0(ioc,
						&mpi_reply,
						&enclosure_dev->pg0,
						LEAPIORAID_SAS_ENCLOS_PGAD_FORM_HANDLE,
						event_data->EnclosureHandle);
			if (rc
			    || (le16_to_cpu(mpi_reply.IOCStatus) &
				LEAPIORAID_IOCSTATUS_MASK)) {
				kfree(enclosure_dev);
				return;
			}
			list_add_tail(&enclosure_dev->list,
				      &ioc->enclosure_list);
		}
		break;
	case LEAPIORAID_EVENT_SAS_ENCL_RC_NOT_RESPONDING:
		if (enclosure_dev) {
			list_del(&enclosure_dev->list);
			kfree(enclosure_dev);
		}
		break;
	default:
		break;
	}
}

static void
leapioraid_scsihost_sas_broadcast_primitive_event(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_fw_event_work *fw_event)
{
	struct scsi_cmnd *scmd;
	struct scsi_device *sdev;
	u16 smid, handle;
	u32 lun;
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	u32 termination_count;
	u32 query_count;
	struct LeapioraidSCSITmgRep_t *mpi_reply;
	struct LeapioraidEventDataSasBroadcastPrimitive_t *event_data =
	    fw_event->event_data;
	u16 ioc_status;
	unsigned long flags;
	int r;
	u8 max_retries = 0;
	u8 task_abort_retries;
	struct leapioraid_scsiio_tracker *st;

	mutex_lock(&ioc->tm_cmds.mutex);
	dewtprintk(ioc,
		pr_info(
			"%s %s: enter: phy number(%d), width(%d)\n",
			ioc->name, __func__,
			event_data->PhyNum, event_data->PortWidth));
	leapioraid_scsihost_block_io_all_device(ioc);
	spin_lock_irqsave(&ioc->scsi_lookup_lock, flags);
	mpi_reply = ioc->tm_cmds.reply;
broadcast_aen_retry:
	if (max_retries++ == 5) {
		dewtprintk(ioc, pr_info("%s %s: giving up\n",
				       ioc->name, __func__));
		goto out;
	} else if (max_retries > 1)
		dewtprintk(ioc, pr_info("%s %s: %d retry\n",
				       ioc->name, __func__, max_retries - 1));
	termination_count = 0;
	query_count = 0;
	for (smid = 1; smid <= ioc->shost->can_queue; smid++) {
		if (ioc->shost_recovery)
			goto out;
		scmd = leapioraid_scsihost_scsi_lookup_get(ioc, smid);
		if (!scmd)
			continue;
		st = leapioraid_base_scsi_cmd_priv(scmd);
		if (!st || st->smid == 0)
			continue;
		sdev = scmd->device;
		sas_device_priv_data = sdev->hostdata;
		if (!sas_device_priv_data || !sas_device_priv_data->sas_target)
			continue;
		if (sas_device_priv_data->sas_target->flags &
		    LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT)
			continue;
		if (sas_device_priv_data->sas_target->flags &
		    LEAPIORAID_TARGET_FLAGS_VOLUME)
			continue;
		handle = sas_device_priv_data->sas_target->handle;
		lun = sas_device_priv_data->lun;
		query_count++;
		if (ioc->shost_recovery)
			goto out;
		spin_unlock_irqrestore(&ioc->scsi_lookup_lock, flags);
		r = leapioraid_scsihost_issue_tm(ioc, handle, 0, 0, lun,
						 LEAPIORAID_SCSITASKMGMT_TASKTYPE_QUERY_TASK,
						 st->smid, 30, 0);
		if (r == FAILED) {
			sdev_printk(KERN_WARNING, sdev,
				"leapioraid_scsihost_issue_tm:\n\t\t"
				"FAILED when sending QUERY_TASK: scmd(%p)\n",
				scmd);
			spin_lock_irqsave(&ioc->scsi_lookup_lock, flags);
			goto broadcast_aen_retry;
		}
		ioc_status = le16_to_cpu(mpi_reply->IOCStatus)
		    & LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			sdev_printk(KERN_WARNING, sdev,
				"query task: FAILED with IOCSTATUS(0x%04x), scmd(%p)\n",
				ioc_status, scmd);
			spin_lock_irqsave(&ioc->scsi_lookup_lock, flags);
			goto broadcast_aen_retry;
		}
		if (mpi_reply->ResponseCode ==
		    LEAPIORAID_SCSITASKMGMT_RSP_TM_SUCCEEDED ||
		    mpi_reply->ResponseCode ==
		    LEAPIORAID_SCSITASKMGMT_RSP_IO_QUEUED_ON_IOC) {
			spin_lock_irqsave(&ioc->scsi_lookup_lock, flags);
			continue;
		}
		task_abort_retries = 0;
tm_retry:
		if (task_abort_retries++ == 60) {
			dewtprintk(ioc, pr_err(
					       "%s %s: ABORT_TASK: giving up\n",
					       ioc->name, __func__));
			spin_lock_irqsave(&ioc->scsi_lookup_lock, flags);
			goto broadcast_aen_retry;
		}
		if (ioc->shost_recovery)
			goto out_no_lock;
		r = leapioraid_scsihost_issue_tm(ioc, handle, sdev->channel,
						 sdev->id, sdev->lun,
						 LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABORT_TASK,
						 st->smid, 30, 0);
		if (r == FAILED) {
			sdev_printk(KERN_WARNING, sdev,
				"ABORT_TASK: FAILED : scmd(%p)\n", scmd);
			goto tm_retry;
		}
		if (task_abort_retries > 1)
			sdev_printk(KERN_WARNING, sdev,
				"leapioraid_scsihost_issue_tm:\n\t\t"
					"ABORT_TASK: RETRIES (%d): scmd(%p)\n",
						task_abort_retries - 1,
						scmd);
		termination_count += le32_to_cpu(mpi_reply->TerminationCount);
		spin_lock_irqsave(&ioc->scsi_lookup_lock, flags);
	}
	if (ioc->broadcast_aen_pending) {
		dewtprintk(ioc,
			pr_info("%s %s: loop back due to pending AEN\n",
				ioc->name, __func__));
		ioc->broadcast_aen_pending = 0;
		goto broadcast_aen_retry;
	}
out:
	spin_unlock_irqrestore(&ioc->scsi_lookup_lock, flags);
out_no_lock:
	dewtprintk(ioc, pr_err(
		"%s %s - exit, query_count = %d termination_count = %d\n",
		ioc->name, __func__, query_count,
		termination_count));
	ioc->broadcast_aen_busy = 0;
	if (!ioc->shost_recovery)
		leapioraid_scsihost_ublock_io_all_device(ioc, 1);
	mutex_unlock(&ioc->tm_cmds.mutex);
}

static void
leapioraid_scsihost_sas_discovery_event(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_fw_event_work *fw_event)
{
	struct LeapioraidEventDataSasDiscovery_t *event_data
		= fw_event->event_data;

	if (ioc->logging_level & LEAPIORAID_DEBUG_EVENT_WORK_TASK) {
		pr_info("%s sas discovery event: (%s)",
		       ioc->name,
		       (event_data->ReasonCode ==
			LEAPIORAID_EVENT_SAS_DISC_RC_STARTED) ? "start" : "stop");
		if (event_data->DiscoveryStatus)
			pr_info("discovery_status(0x%08x)",
			       le32_to_cpu(event_data->DiscoveryStatus));
		pr_info("\n");
	}
	if (event_data->ReasonCode == LEAPIORAID_EVENT_SAS_DISC_RC_STARTED &&
	    !ioc->sas_hba.num_phys) {
		if (disable_discovery > 0 && ioc->shost_recovery) {
			while (ioc->shost_recovery)
				ssleep(1);
		}
		leapioraid_scsihost_sas_host_add(ioc);
	}
}

static void
leapioraid_scsihost_sas_device_discovery_error_event(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_fw_event_work *fw_event)
{
	struct LeapioraidEventDataSasDeviceDiscoveryError_t *event_data =
	    fw_event->event_data;

	switch (event_data->ReasonCode) {
	case LEAPIORAID_EVENT_SAS_DISC_ERR_SMP_FAILED:
		pr_warn(
			"%s SMP command sent to the expander(handle:0x%04x,\n\t\t"
			"sas_address:0x%016llx,physical_port:0x%02x) has failed\n",
				ioc->name,
				le16_to_cpu(event_data->DevHandle),
				(unsigned long long)le64_to_cpu(event_data->SASAddress),
				event_data->PhysicalPort);
		break;
	case LEAPIORAID_EVENT_SAS_DISC_ERR_SMP_TIMEOUT:
		pr_warn(
			"%s SMP command sent to the expander(handle:0x%04x,\n\t\t"
			"sas_address:0x%016llx,physical_port:0x%02x) has timed out\n",
				ioc->name,
				le16_to_cpu(event_data->DevHandle),
				(unsigned long long)le64_to_cpu(event_data->SASAddress),
				event_data->PhysicalPort);
		break;
	default:
		break;
	}
}

static int
leapioraid_scsihost_ir_fastpath(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle,
	u8 phys_disk_num)
{
	struct LeapioraidRaidActionReq_t *mpi_request;
	struct LeapioraidRaidActionRep_t *mpi_reply;
	u16 smid;
	u8 issue_reset = 0;
	int rc = 0;
	u16 ioc_status;
	u32 log_info;

	mutex_lock(&ioc->scsih_cmds.mutex);
	if (ioc->scsih_cmds.status != LEAPIORAID_CMD_NOT_USED) {
		pr_err("%s %s: scsih_cmd in use\n",
		       ioc->name, __func__);
		rc = -EAGAIN;
		goto out;
	}
	ioc->scsih_cmds.status = LEAPIORAID_CMD_PENDING;
	smid = leapioraid_base_get_smid(ioc, ioc->scsih_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		ioc->scsih_cmds.status = LEAPIORAID_CMD_NOT_USED;
		rc = -EAGAIN;
		goto out;
	}
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->scsih_cmds.smid = smid;
	memset(mpi_request, 0, sizeof(struct LeapioraidRaidActionReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_RAID_ACTION;
	mpi_request->Action = 0x24;
	mpi_request->PhysDiskNum = phys_disk_num;
	dewtprintk(ioc, pr_info(
		"%s IR RAID_ACTION: turning fast path on for handle(0x%04x), phys_disk_num (0x%02x)\n",
		ioc->name, handle, phys_disk_num));
	init_completion(&ioc->scsih_cmds.done);
	ioc->put_smid_default(ioc, smid);
	wait_for_completion_timeout(&ioc->scsih_cmds.done, 10 * HZ);
	if (!(ioc->scsih_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		leapioraid_check_cmd_timeout(ioc,
			ioc->scsih_cmds.status,
			mpi_request,
			sizeof(struct LeapioraidRaidActionReq_t)
			/ 4, issue_reset);
		rc = -EFAULT;
		goto out;
	}
	if (ioc->scsih_cmds.status & LEAPIORAID_CMD_REPLY_VALID) {
		mpi_reply = ioc->scsih_cmds.reply;
		ioc_status = le16_to_cpu(mpi_reply->IOCStatus);
		if (ioc_status & LEAPIORAID_IOCSTATUS_FLAG_LOG_INFO_AVAILABLE)
			log_info = le32_to_cpu(mpi_reply->IOCLogInfo);
		else
			log_info = 0;
		ioc_status &= LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			dewtprintk(ioc, pr_err(
				"%s IR RAID_ACTION: failed: ioc_status(0x%04x), loginfo(0x%08x)!!!\n",
				ioc->name, ioc_status,
				log_info));
			rc = -EFAULT;
		} else
			dewtprintk(ioc, pr_err(
				"%s IR RAID_ACTION: completed successfully\n",
				ioc->name));
	}
out:
	ioc->scsih_cmds.status = LEAPIORAID_CMD_NOT_USED;
	mutex_unlock(&ioc->scsih_cmds.mutex);
	if (issue_reset)
		leapioraid_base_hard_reset_handler(ioc, FORCE_BIG_HAMMER);
	return rc;
}

static void
leapioraid_scsihost_reprobe_lun(
	struct scsi_device *sdev, void *no_uld_attach)
{
	int rc;

	sdev->no_uld_attach = no_uld_attach ? 1 : 0;
	sdev_printk(KERN_INFO, sdev, "%s raid component\n",
		    sdev->no_uld_attach ? "hiding" : "exposing");
	rc = scsi_device_reprobe(sdev);
	pr_info("%s rc=%d\n", __func__, rc);
}

static void
leapioraid_scsihost_sas_volume_add(struct LEAPIORAID_ADAPTER *ioc,
			 struct LeapioraidEventIrCfgEle_t *element)
{
	struct leapioraid_raid_device *raid_device;
	unsigned long flags;
	u64 wwid;
	u16 handle = le16_to_cpu(element->VolDevHandle);
	int rc;

	leapioraid_config_get_volume_wwid(ioc, handle, &wwid);
	if (!wwid) {
		pr_err("%s failure at %s:%d/%s()!\n", ioc->name,
		       __FILE__, __LINE__, __func__);
		return;
	}
	spin_lock_irqsave(&ioc->raid_device_lock, flags);
	raid_device = leapioraid_scsihost_raid_device_find_by_wwid(
		ioc, wwid);
	spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
	if (raid_device)
		return;
	raid_device = kzalloc(sizeof(struct leapioraid_raid_device),
		GFP_KERNEL);
	if (!raid_device)
		return;

	raid_device->id = ioc->sas_id++;
	raid_device->channel = RAID_CHANNEL;
	raid_device->handle = handle;
	raid_device->wwid = wwid;
	leapioraid_scsihost_raid_device_add(ioc, raid_device);
	if (!ioc->wait_for_discovery_to_complete) {
		rc = scsi_add_device(ioc->shost, RAID_CHANNEL,
				     raid_device->id, 0);
		if (rc)
			leapioraid_scsihost_raid_device_remove(ioc, raid_device);
	} else {
		spin_lock_irqsave(&ioc->raid_device_lock, flags);
		leapioraid_scsihost_determine_boot_device(
			ioc, raid_device, RAID_CHANNEL);
		spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
	}
}

static void
leapioraid_scsihost_sas_volume_delete(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle)
{
	struct leapioraid_raid_device *raid_device;
	unsigned long flags;
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	struct scsi_target *starget = NULL;

	spin_lock_irqsave(&ioc->raid_device_lock, flags);
	raid_device = leapioraid_raid_device_find_by_handle(ioc, handle);
	if (raid_device) {
		if (raid_device->starget) {
			starget = raid_device->starget;
			sas_target_priv_data = starget->hostdata;
			sas_target_priv_data->deleted = 1;
		}
		pr_info("%s removing handle(0x%04x), wwid(0x%016llx)\n",
		       ioc->name, raid_device->handle,
		       (unsigned long long)raid_device->wwid);
		list_del(&raid_device->list);
		kfree(raid_device);
	}
	spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
	if (starget)
		scsi_remove_target(&starget->dev);
}

static void
leapioraid_scsihost_sas_pd_expose(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidEventIrCfgEle_t *element)
{
	struct leapioraid_sas_device *sas_device;
	struct scsi_target *starget = NULL;
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	unsigned long flags;
	u16 handle = le16_to_cpu(element->PhysDiskDevHandle);

	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_device = __leapioraid_get_sdev_by_handle(ioc, handle);
	if (sas_device) {
		sas_device->volume_handle = 0;
		sas_device->volume_wwid = 0;
		clear_bit(handle, ioc->pd_handles);
		if (sas_device->starget && sas_device->starget->hostdata) {
			starget = sas_device->starget;
			sas_target_priv_data = starget->hostdata;
			sas_target_priv_data->flags &=
			    ~LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT;
			sas_device->pfa_led_on = 0;
			leapioraid_sas_device_put(sas_device);
		}
	}
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	if (!sas_device)
		return;
	if (starget)
		starget_for_each_device(starget, NULL, leapioraid_scsihost_reprobe_lun);
}

static void
leapioraid_scsihost_sas_pd_hide(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidEventIrCfgEle_t *element)
{
	struct leapioraid_sas_device *sas_device;
	struct scsi_target *starget = NULL;
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	unsigned long flags;
	u16 handle = le16_to_cpu(element->PhysDiskDevHandle);
	u16 volume_handle = 0;
	u64 volume_wwid = 0;

	leapioraid_config_get_volume_handle(ioc, handle, &volume_handle);
	if (volume_handle)
		leapioraid_config_get_volume_wwid(ioc, volume_handle,
						  &volume_wwid);
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_device = __leapioraid_get_sdev_by_handle(ioc, handle);
	if (sas_device) {
		set_bit(handle, ioc->pd_handles);
		if (sas_device->starget && sas_device->starget->hostdata) {
			starget = sas_device->starget;
			sas_target_priv_data = starget->hostdata;
			sas_target_priv_data->flags |=
			    LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT;
			sas_device->volume_handle = volume_handle;
			sas_device->volume_wwid = volume_wwid;
			leapioraid_sas_device_put(sas_device);
		}
	}
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	if (!sas_device)
		return;
	leapioraid_scsihost_ir_fastpath(ioc, handle, element->PhysDiskNum);
	if (starget)
		starget_for_each_device(starget, (void *)1,
					leapioraid_scsihost_reprobe_lun);
}

static void
leapioraid_scsihost_sas_pd_delete(struct LEAPIORAID_ADAPTER *ioc,
			struct LeapioraidEventIrCfgEle_t *element)
{
	u16 handle = le16_to_cpu(element->PhysDiskDevHandle);

	leapioraid_scsihost_device_remove_by_handle(ioc, handle);
}

static void
leapioraid_scsihost_sas_pd_add(struct LEAPIORAID_ADAPTER *ioc,
		     struct LeapioraidEventIrCfgEle_t *element)
{
	struct leapioraid_sas_device *sas_device;
	u16 handle = le16_to_cpu(element->PhysDiskDevHandle);
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidSasDevP0_t sas_device_pg0;
	u32 ioc_status;
	u64 sas_address;
	u16 parent_handle;

	set_bit(handle, ioc->pd_handles);
	sas_device = leapioraid_get_sdev_by_handle(ioc, handle);
	if (sas_device) {
		leapioraid_scsihost_ir_fastpath(ioc, handle, element->PhysDiskNum);
		leapioraid_sas_device_put(sas_device);
		return;
	}
	if ((leapioraid_config_get_sas_device_pg0
	     (ioc, &mpi_reply, &sas_device_pg0,
	      LEAPIORAID_SAS_DEVICE_PGAD_FORM_HANDLE, handle))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return;
	}
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return;
	}
	parent_handle = le16_to_cpu(sas_device_pg0.ParentDevHandle);
	if (!leapioraid_scsihost_get_sas_address(ioc, parent_handle, &sas_address))
		leapioraid_transport_update_links(ioc, sas_address, handle,
						  sas_device_pg0.PhyNum,
						  LEAPIORAID_SAS_NEG_LINK_RATE_1_5,
						  leapioraid_get_port_by_id(ioc,
							sas_device_pg0.PhysicalPort,
							0));
	leapioraid_scsihost_ir_fastpath(ioc, handle, element->PhysDiskNum);
	leapioraid_scsihost_add_device(ioc, handle, 0, 1);
}

static void
leapioraid_scsihost_sas_ir_config_change_event_debug(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidEventDataIrCfgChangeList_t *event_data)
{
	struct LeapioraidEventIrCfgEle_t *element;
	u8 element_type;
	int i;
	char *reason_str = NULL, *element_str = NULL;

	element =
	    (struct LeapioraidEventIrCfgEle_t *) &event_data->ConfigElement[0];
	pr_info("%s raid config change: (%s), elements(%d)\n",
	       ioc->name,
	       (le32_to_cpu(event_data->Flags) &
		LEAPIORAID_EVENT_IR_CHANGE_FLAGS_FOREIGN_CONFIG) ? "foreign" :
	       "native", event_data->NumElements);
	for (i = 0; i < event_data->NumElements; i++, element++) {
		switch (element->ReasonCode) {
		case LEAPIORAID_EVENT_IR_CHANGE_RC_ADDED:
			reason_str = "add";
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_RC_REMOVED:
			reason_str = "remove";
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_RC_NO_CHANGE:
			reason_str = "no change";
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_RC_HIDE:
			reason_str = "hide";
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_RC_UNHIDE:
			reason_str = "unhide";
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_RC_VOLUME_CREATED:
			reason_str = "volume_created";
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_RC_VOLUME_DELETED:
			reason_str = "volume_deleted";
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_RC_PD_CREATED:
			reason_str = "pd_created";
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_RC_PD_DELETED:
			reason_str = "pd_deleted";
			break;
		default:
			reason_str = "unknown reason";
			break;
		}
		element_type = le16_to_cpu(element->ElementFlags) &
		    LEAPIORAID_EVENT_IR_CHANGE_EFLAGS_ELEMENT_TYPE_MASK;
		switch (element_type) {
		case LEAPIORAID_EVENT_IR_CHANGE_EFLAGS_VOLUME_ELEMENT:
			element_str = "volume";
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_EFLAGS_VOLPHYSDISK_ELEMENT:
			element_str = "phys disk";
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_EFLAGS_HOTSPARE_ELEMENT:
			element_str = "hot spare";
			break;
		default:
			element_str = "unknown element";
			break;
		}
		pr_info(
			"\t(%s:%s), vol handle(0x%04x), pd handle(0x%04x), pd num(0x%02x)\n",
			element_str,
			reason_str, le16_to_cpu(element->VolDevHandle),
			le16_to_cpu(element->PhysDiskDevHandle),
			element->PhysDiskNum);
	}
}

static void
leapioraid_scsihost_sas_ir_config_change_event(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_fw_event_work *fw_event)
{
	struct LeapioraidEventIrCfgEle_t *element;
	int i;
	u8 foreign_config;
	struct LeapioraidEventDataIrCfgChangeList_t *event_data
		= fw_event->event_data;

	if ((ioc->logging_level & LEAPIORAID_DEBUG_EVENT_WORK_TASK)
	    && !ioc->warpdrive_msg)
		leapioraid_scsihost_sas_ir_config_change_event_debug(ioc, event_data);
	foreign_config = (le32_to_cpu(event_data->Flags) &
			  LEAPIORAID_EVENT_IR_CHANGE_FLAGS_FOREIGN_CONFIG) ? 1 : 0;
	element =
	    (struct LeapioraidEventIrCfgEle_t *) &event_data->ConfigElement[0];
	if (ioc->shost_recovery) {
		for (i = 0; i < event_data->NumElements; i++, element++) {
			if (element->ReasonCode ==
			    LEAPIORAID_EVENT_IR_CHANGE_RC_HIDE)
				leapioraid_scsihost_ir_fastpath(ioc,
						      le16_to_cpu(element->PhysDiskDevHandle),
						      element->PhysDiskNum);
		}
		return;
	}
	for (i = 0; i < event_data->NumElements; i++, element++) {
		switch (element->ReasonCode) {
		case LEAPIORAID_EVENT_IR_CHANGE_RC_VOLUME_CREATED:
		case LEAPIORAID_EVENT_IR_CHANGE_RC_ADDED:
			if (!foreign_config)
				leapioraid_scsihost_sas_volume_add(ioc, element);
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_RC_VOLUME_DELETED:
		case LEAPIORAID_EVENT_IR_CHANGE_RC_REMOVED:
			if (!foreign_config)
				leapioraid_scsihost_sas_volume_delete(ioc,
							    le16_to_cpu
							    (element->VolDevHandle));
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_RC_PD_CREATED:
			leapioraid_scsihost_sas_pd_hide(ioc, element);
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_RC_PD_DELETED:
			leapioraid_scsihost_sas_pd_expose(ioc, element);
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_RC_HIDE:
			leapioraid_scsihost_sas_pd_add(ioc, element);
			break;
		case LEAPIORAID_EVENT_IR_CHANGE_RC_UNHIDE:
			leapioraid_scsihost_sas_pd_delete(ioc, element);
			break;
		}
	}
}

static void
leapioraid_scsihost_sas_ir_volume_event(
		struct LEAPIORAID_ADAPTER *ioc,
		struct leapioraid_fw_event_work *fw_event)
{
	u64 wwid;
	unsigned long flags;
	struct leapioraid_raid_device *raid_device;
	u16 handle;
	u32 state;
	int rc;
	struct LeapioraidEventDataIrVol_t *event_data
		= fw_event->event_data;

	if (ioc->shost_recovery)
		return;
	if (event_data->ReasonCode != LEAPIORAID_EVENT_IR_VOLUME_RC_STATE_CHANGED)
		return;
	handle = le16_to_cpu(event_data->VolDevHandle);
	state = le32_to_cpu(event_data->NewValue);
	if (!ioc->warpdrive_msg)
		dewtprintk(ioc,
			pr_info("%s %s: handle(0x%04x), old(0x%08x), new(0x%08x)\n",
				ioc->name,
				__func__, handle,
				le32_to_cpu(event_data->PreviousValue),
				state));
	switch (state) {
	case LEAPIORAID_RAID_VOL_STATE_MISSING:
	case LEAPIORAID_RAID_VOL_STATE_FAILED:
		leapioraid_scsihost_sas_volume_delete(ioc, handle);
		break;
	case LEAPIORAID_RAID_VOL_STATE_ONLINE:
	case LEAPIORAID_RAID_VOL_STATE_DEGRADED:
	case LEAPIORAID_RAID_VOL_STATE_OPTIMAL:
		spin_lock_irqsave(&ioc->raid_device_lock, flags);
		raid_device =
		    leapioraid_raid_device_find_by_handle(ioc, handle);
		spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
		if (raid_device)
			break;
		leapioraid_config_get_volume_wwid(ioc, handle, &wwid);
		if (!wwid) {
			pr_err(
			       "%s failure at %s:%d/%s()!\n", ioc->name,
			       __FILE__, __LINE__, __func__);
			break;
		}
		raid_device = kzalloc(sizeof(struct leapioraid_raid_device),
			GFP_KERNEL);
		if (!raid_device)
			break;

		raid_device->id = ioc->sas_id++;
		raid_device->channel = RAID_CHANNEL;
		raid_device->handle = handle;
		raid_device->wwid = wwid;
		leapioraid_scsihost_raid_device_add(ioc, raid_device);
		rc = scsi_add_device(ioc->shost, RAID_CHANNEL,
				     raid_device->id, 0);
		if (rc)
			leapioraid_scsihost_raid_device_remove(ioc, raid_device);
		break;
	case LEAPIORAID_RAID_VOL_STATE_INITIALIZING:
	default:
		break;
	}
}

static void
leapioraid_scsihost_sas_ir_physical_disk_event(
		struct LEAPIORAID_ADAPTER *ioc,
		struct leapioraid_fw_event_work *fw_event)
{
	u16 handle, parent_handle;
	u32 state;
	struct leapioraid_sas_device *sas_device;
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidSasDevP0_t sas_device_pg0;
	u32 ioc_status;
	struct LeapioraidEventDataIrPhyDisk_t *event_data
		= fw_event->event_data;
	u64 sas_address;

	if (ioc->shost_recovery)
		return;
	if (event_data->ReasonCode !=
	    LEAPIORAID_EVENT_IR_PHYSDISK_RC_STATE_CHANGED)
		return;
	handle = le16_to_cpu(event_data->PhysDiskDevHandle);
	state = le32_to_cpu(event_data->NewValue);
	if (!ioc->warpdrive_msg)
		dewtprintk(ioc,
			pr_info("%s %s: handle(0x%04x), old(0x%08x), new(0x%08x)\n",
				ioc->name,
				__func__, handle,
				le32_to_cpu(event_data->PreviousValue),
				state));
	switch (state) {
	case LEAPIORAID_RAID_PD_STATE_ONLINE:
	case LEAPIORAID_RAID_PD_STATE_DEGRADED:
	case LEAPIORAID_RAID_PD_STATE_REBUILDING:
	case LEAPIORAID_RAID_PD_STATE_OPTIMAL:
	case LEAPIORAID_RAID_PD_STATE_HOT_SPARE:
		set_bit(handle, ioc->pd_handles);
		sas_device = leapioraid_get_sdev_by_handle(ioc, handle);
		if (sas_device) {
			leapioraid_sas_device_put(sas_device);
			return;
		}
		if ((leapioraid_config_get_sas_device_pg0(
				ioc, &mpi_reply,
				&sas_device_pg0,
				LEAPIORAID_SAS_DEVICE_PGAD_FORM_HANDLE,
				handle))) {
			pr_err("%s failure at %s:%d/%s()!\n",
			       ioc->name, __FILE__, __LINE__, __func__);
			return;
		}
		ioc_status = le16_to_cpu(mpi_reply.IOCStatus) &
		    LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			pr_err("%s failure at %s:%d/%s()!\n",
			       ioc->name, __FILE__, __LINE__, __func__);
			return;
		}
		parent_handle = le16_to_cpu(sas_device_pg0.ParentDevHandle);
		if (!leapioraid_scsihost_get_sas_address
		    (ioc, parent_handle, &sas_address))
			leapioraid_transport_update_links(ioc, sas_address,
					handle,
					sas_device_pg0.PhyNum,
					LEAPIORAID_SAS_NEG_LINK_RATE_1_5,
					leapioraid_get_port_by_id
					(ioc,
					sas_device_pg0.PhysicalPort, 0));
		leapioraid_scsihost_add_device(ioc, handle, 0, 1);
		break;
	case LEAPIORAID_RAID_PD_STATE_OFFLINE:
	case LEAPIORAID_RAID_PD_STATE_NOT_CONFIGURED:
	case LEAPIORAID_RAID_PD_STATE_NOT_COMPATIBLE:
	default:
		break;
	}
}

static void
leapioraid_scsihost_sas_ir_operation_status_event_debug(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidEventDataIrOpStatus_t *event_data)
{
	char *reason_str = NULL;

	switch (event_data->RAIDOperation) {
	case LEAPIORAID_EVENT_IR_RAIDOP_RESYNC:
		reason_str = "resync";
		break;
	case LEAPIORAID_EVENT_IR_RAIDOP_ONLINE_CAP_EXPANSION:
		reason_str = "online capacity expansion";
		break;
	case LEAPIORAID_EVENT_IR_RAIDOP_CONSISTENCY_CHECK:
		reason_str = "consistency check";
		break;
	case LEAPIORAID_EVENT_IR_RAIDOP_BACKGROUND_INIT:
		reason_str = "background init";
		break;
	case LEAPIORAID_EVENT_IR_RAIDOP_MAKE_DATA_CONSISTENT:
		reason_str = "make data consistent";
		break;
	}
	if (!reason_str)
		return;
	pr_info(
		"%s raid operational status: (%s)\thandle(0x%04x), percent complete(%d)\n",
		ioc->name, reason_str,
		le16_to_cpu(event_data->VolDevHandle),
		event_data->PercentComplete);
}

static void
leapioraid_scsihost_sas_ir_operation_status_event(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_fw_event_work *fw_event)
{
	struct LeapioraidEventDataIrOpStatus_t *event_data
		= fw_event->event_data;
	static struct leapioraid_raid_device *raid_device;
	unsigned long flags;
	u16 handle;

	if ((ioc->logging_level & LEAPIORAID_DEBUG_EVENT_WORK_TASK)
	    && !ioc->warpdrive_msg)
		leapioraid_scsihost_sas_ir_operation_status_event_debug(
			ioc, event_data);
	if (event_data->RAIDOperation == LEAPIORAID_EVENT_IR_RAIDOP_RESYNC) {
		spin_lock_irqsave(&ioc->raid_device_lock, flags);
		handle = le16_to_cpu(event_data->VolDevHandle);
		raid_device =
		    leapioraid_raid_device_find_by_handle(ioc, handle);
		if (raid_device)
			raid_device->percent_complete =
			    event_data->PercentComplete;
		spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
	}
}

static void
leapioraid_scsihost_prep_device_scan(struct LEAPIORAID_ADAPTER *ioc)
{
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct scsi_device *sdev;

	shost_for_each_device(sdev, ioc->shost) {
		sas_device_priv_data = sdev->hostdata;
		if (sas_device_priv_data && sas_device_priv_data->sas_target)
			sas_device_priv_data->sas_target->deleted = 1;
	}
}

static void
leapioraid_scsihost_update_device_qdepth(struct LEAPIORAID_ADAPTER *ioc)
{
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct leapioraid_sas_device *sas_device;
	struct scsi_device *sdev;
	u16 qdepth;

	pr_info("%s Update Devices with FW Reported QD\n",
	       ioc->name);
	shost_for_each_device(sdev, ioc->shost) {
		sas_device_priv_data = sdev->hostdata;
		if (sas_device_priv_data && sas_device_priv_data->sas_target) {
			sas_device = sas_device_priv_data->sas_target->sas_dev;
			if (sas_device &&
			    sas_device->device_info & LEAPIORAID_SAS_DEVICE_INFO_SSP_TARGET)
				qdepth =
				    (sas_device->port_type >
				     1) ? ioc->max_wideport_qd : ioc->max_narrowport_qd;
			else if (sas_device
				 && sas_device->device_info &
				 LEAPIORAID_SAS_DEVICE_INFO_SATA_DEVICE)
				qdepth = ioc->max_sata_qd;
			else
				continue;
			leapioraid__scsihost_change_queue_depth(sdev, qdepth);
		}
	}
}

static void
leapioraid_scsihost_mark_responding_sas_device(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidSasDevP0_t *sas_device_pg0)
{
	struct LEAPIORAID_TARGET *sas_target_priv_data = NULL;
	struct scsi_target *starget;
	struct leapioraid_sas_device *sas_device;
	struct leapioraid_enclosure_node *enclosure_dev = NULL;
	unsigned long flags;
	struct leapioraid_hba_port *port;

	port = leapioraid_get_port_by_id(ioc, sas_device_pg0->PhysicalPort, 0);
	if (sas_device_pg0->EnclosureHandle) {
		enclosure_dev =
		    leapioraid_scsihost_enclosure_find_by_handle(ioc,
								 le16_to_cpu
								 (sas_device_pg0->EnclosureHandle));
		if (enclosure_dev == NULL)
			pr_info(
				"%s Enclosure handle(0x%04x)doesn't match with enclosure device!\n",
			    ioc->name, sas_device_pg0->EnclosureHandle);
	}
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	list_for_each_entry(sas_device, &ioc->sas_device_list, list) {
		if ((sas_device->sas_address ==
		     le64_to_cpu(sas_device_pg0->SASAddress))
		    && (sas_device->slot == le16_to_cpu(sas_device_pg0->Slot))
		    && (sas_device->port == port)) {
			sas_device->responding = 1;
			starget = sas_device->starget;
			if (starget && starget->hostdata) {
				sas_target_priv_data = starget->hostdata;
				sas_target_priv_data->tm_busy = 0;
				sas_target_priv_data->deleted = 0;
			} else
				sas_target_priv_data = NULL;
			if (starget) {
				starget_printk(KERN_INFO, starget,
					"handle(0x%04x), sas_address(0x%016llx), port: %d\n",
					sas_device->handle,
					(unsigned long long)sas_device->sas_address,
					sas_device->port->port_id);
				if (sas_device->enclosure_handle != 0)
					starget_printk(KERN_INFO, starget,
						"enclosure logical id(0x%016llx), slot(%d)\n",
						(unsigned long long)
						sas_device->enclosure_logical_id,
						sas_device->slot);
			}
			if (le16_to_cpu(sas_device_pg0->Flags) &
			    LEAPIORAID_SAS_DEVICE0_FLAGS_ENCL_LEVEL_VALID) {
				sas_device->enclosure_level =
				    sas_device_pg0->EnclosureLevel;
				memcpy(sas_device->connector_name,
				       sas_device_pg0->ConnectorName, 4);
				sas_device->connector_name[4] = '\0';
			} else {
				sas_device->enclosure_level = 0;
				sas_device->connector_name[0] = '\0';
			}
			sas_device->enclosure_handle =
			    le16_to_cpu(sas_device_pg0->EnclosureHandle);
			sas_device->is_chassis_slot_valid = 0;
			if (enclosure_dev) {
				sas_device->enclosure_logical_id =
				    le64_to_cpu(enclosure_dev->pg0.EnclosureLogicalID);
				if (le16_to_cpu(enclosure_dev->pg0.Flags) &
				    LEAPIORAID_SAS_ENCLS0_FLAGS_CHASSIS_SLOT_VALID) {
					sas_device->is_chassis_slot_valid = 1;
					sas_device->chassis_slot =
					    enclosure_dev->pg0.ChassisSlot;
				}
			}
			if (sas_device->handle ==
			    le16_to_cpu(sas_device_pg0->DevHandle))
				goto out;
			pr_info("\thandle changed from(0x%04x)!!!\n",
			       sas_device->handle);
			sas_device->handle =
			    le16_to_cpu(sas_device_pg0->DevHandle);
			if (sas_target_priv_data)
				sas_target_priv_data->handle =
				    le16_to_cpu(sas_device_pg0->DevHandle);
			goto out;
		}
	}
out:
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
}

static void
leapioraid_scsihost_create_enclosure_list_after_reset(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_enclosure_node *enclosure_dev;
	struct LeapioraidCfgRep_t mpi_reply;
	u16 enclosure_handle;
	int rc;

	leapioraid_free_enclosure_list(ioc);
	enclosure_handle = 0xFFFF;
	do {
		enclosure_dev =
		    kzalloc(sizeof(struct leapioraid_enclosure_node), GFP_KERNEL);
		if (!enclosure_dev) {
			pr_err("%s failure at %s:%d/%s()!\n", ioc->name,
			       __FILE__, __LINE__, __func__);
			return;
		}
		rc = leapioraid_config_get_enclosure_pg0(ioc, &mpi_reply,
				&enclosure_dev->pg0,
				LEAPIORAID_SAS_ENCLOS_PGAD_FORM_GET_NEXT_HANDLE,
				enclosure_handle);
		if (rc || (le16_to_cpu(mpi_reply.IOCStatus) &
			   LEAPIORAID_IOCSTATUS_MASK)) {
			kfree(enclosure_dev);
			return;
		}
		list_add_tail(&enclosure_dev->list, &ioc->enclosure_list);
		enclosure_handle =
		    le16_to_cpu(enclosure_dev->pg0.EnclosureHandle);
	} while (1);
}

static void
leapioraid_scsihost_search_responding_sas_devices(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidSasDevP0_t sas_device_pg0;
	struct LeapioraidCfgRep_t mpi_reply;
	u16 ioc_status;
	u16 handle;
	u32 device_info;

	pr_info("%s search for end-devices: start\n",
	       ioc->name);
	if (list_empty(&ioc->sas_device_list))
		goto out;
	handle = 0xFFFF;
	while (!(leapioraid_config_get_sas_device_pg0(ioc, &mpi_reply,
				&sas_device_pg0,
				LEAPIORAID_SAS_DEVICE_PGAD_FORM_GET_NEXT_HANDLE,
				handle))) {
		ioc_status =
		    le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			pr_info(
				"%s \tbreak from %s: ioc_status(0x%04x), loginfo(0x%08x)\n",
			    ioc->name, __func__, ioc_status,
			    le32_to_cpu(mpi_reply.IOCLogInfo));
			break;
		}
		handle = le16_to_cpu(sas_device_pg0.DevHandle);
		device_info = le32_to_cpu(sas_device_pg0.DeviceInfo);
		if (!(leapioraid_scsihost_is_sas_end_device(device_info)))
			continue;
		leapioraid_scsihost_mark_responding_sas_device(
			ioc, &sas_device_pg0);
	}
out:
	pr_info("%s search for end-devices: complete\n",
	       ioc->name);
}

static void
leapioraid_scsihost_mark_responding_raid_device(
	struct LEAPIORAID_ADAPTER *ioc, u64 wwid, u16 handle)
{
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	struct scsi_target *starget;
	struct leapioraid_raid_device *raid_device;
	unsigned long flags;

	spin_lock_irqsave(&ioc->raid_device_lock, flags);
	list_for_each_entry(raid_device, &ioc->raid_device_list, list) {
		if (raid_device->wwid == wwid && raid_device->starget) {
			starget = raid_device->starget;
			if (starget && starget->hostdata) {
				sas_target_priv_data = starget->hostdata;
				sas_target_priv_data->deleted = 0;
			} else
				sas_target_priv_data = NULL;
			raid_device->responding = 1;
			spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
			starget_printk(KERN_INFO, raid_device->starget,
				       "handle(0x%04x), wwid(0x%016llx)\n",
				       handle,
				       (unsigned long long)raid_device->wwid);
			spin_lock_irqsave(&ioc->raid_device_lock, flags);
			if (raid_device->handle == handle) {
				spin_unlock_irqrestore(&ioc->raid_device_lock,
						       flags);
				return;
			}
			pr_info("\thandle changed from(0x%04x)!!!\n",
			       raid_device->handle);
			raid_device->handle = handle;
			if (sas_target_priv_data)
				sas_target_priv_data->handle = handle;
			spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
			return;
		}
	}
	spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
}

static void
leapioraid_scsihost_search_responding_raid_devices(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidRaidVolP1_t volume_pg1;
	struct LeapioraidRaidVolP0_t volume_pg0;
	struct LeapioraidRaidPDP0_t pd_pg0;
	struct LeapioraidCfgRep_t mpi_reply;
	u16 ioc_status;
	u16 handle;
	u8 phys_disk_num;

	if (!ioc->ir_firmware)
		return;
	pr_info("%s search for raid volumes: start\n",
	       ioc->name);
	if (list_empty(&ioc->raid_device_list))
		goto out;
	handle = 0xFFFF;
	while (!(leapioraid_config_get_raid_volume_pg1(ioc, &mpi_reply,
			&volume_pg1,
			LEAPIORAID_RAID_VOLUME_PGAD_FORM_GET_NEXT_HANDLE,
			handle))) {
		ioc_status =
		    le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			pr_info("%s \tbreak from %s: ioc_status(0x%04x), loginfo(0x%08x)\n",
			       ioc->name, __func__, ioc_status,
			       le32_to_cpu(mpi_reply.IOCLogInfo));
			break;
		}
		handle = le16_to_cpu(volume_pg1.DevHandle);
		if (leapioraid_config_get_raid_volume_pg0(ioc, &mpi_reply,
				&volume_pg0,
				LEAPIORAID_RAID_VOLUME_PGAD_FORM_HANDLE,
				handle,
				sizeof
				(struct LeapioraidRaidVolP0_t)))
			continue;
		if (volume_pg0.VolumeState == LEAPIORAID_RAID_VOL_STATE_OPTIMAL ||
		    volume_pg0.VolumeState == LEAPIORAID_RAID_VOL_STATE_ONLINE ||
		    volume_pg0.VolumeState == LEAPIORAID_RAID_VOL_STATE_DEGRADED)
			leapioraid_scsihost_mark_responding_raid_device(ioc,
							      le64_to_cpu
							      (volume_pg1.WWID),
							      handle);
	}
	phys_disk_num = 0xFF;
	memset(ioc->pd_handles, 0, ioc->pd_handles_sz);
	while (!(leapioraid_config_get_phys_disk_pg0(ioc, &mpi_reply,
			&pd_pg0,
			LEAPIORAID_PHYSDISK_PGAD_FORM_GET_NEXT_PHYSDISKNUM,
			phys_disk_num))) {
		ioc_status =
		    le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			pr_info("%s \tbreak from %s: ioc_status(0x%04x), loginfo(0x%08x)\n",
			       ioc->name, __func__, ioc_status,
			       le32_to_cpu(mpi_reply.IOCLogInfo));
			break;
		}
		phys_disk_num = pd_pg0.PhysDiskNum;
		handle = le16_to_cpu(pd_pg0.DevHandle);
		set_bit(handle, ioc->pd_handles);
	}
out:
	pr_info("%s search for responding raid volumes: complete\n",
		ioc->name);
}

static void
leapioraid_scsihost_mark_responding_expander(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidExpanderP0_t *expander_pg0)
{
	struct leapioraid_raid_sas_node *sas_expander;
	unsigned long flags;
	int i;
	u8 port_id = expander_pg0->PhysicalPort;
	struct leapioraid_hba_port *port = leapioraid_get_port_by_id(
		ioc, port_id, 0);
	struct leapioraid_enclosure_node *enclosure_dev = NULL;
	u16 handle = le16_to_cpu(expander_pg0->DevHandle);
	u16 enclosure_handle = le16_to_cpu(expander_pg0->EnclosureHandle);
	u64 sas_address = le64_to_cpu(expander_pg0->SASAddress);

	if (enclosure_handle)
		enclosure_dev =
		    leapioraid_scsihost_enclosure_find_by_handle(ioc,
								 enclosure_handle);
	spin_lock_irqsave(&ioc->sas_node_lock, flags);
	list_for_each_entry(sas_expander, &ioc->sas_expander_list, list) {
		if (sas_expander->sas_address != sas_address ||
		    (sas_expander->port != port))
			continue;
		sas_expander->responding = 1;
		if (enclosure_dev) {
			sas_expander->enclosure_logical_id =
			    le64_to_cpu(enclosure_dev->pg0.EnclosureLogicalID);
			sas_expander->enclosure_handle =
			    le16_to_cpu(expander_pg0->EnclosureHandle);
		}
		if (sas_expander->handle == handle)
			goto out;
		pr_info(
			"\texpander(0x%016llx): handle changed from(0x%04x) to (0x%04x)!!!\n",
			(unsigned long long)sas_expander->sas_address,
			sas_expander->handle, handle);
		sas_expander->handle = handle;
		for (i = 0; i < sas_expander->num_phys; i++)
			sas_expander->phy[i].handle = handle;
		goto out;
	}
out:
	spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
}

static void
leapioraid_scsihost_search_responding_expanders(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidExpanderP0_t expander_pg0;
	struct LeapioraidCfgRep_t mpi_reply;
	u16 ioc_status;
	u64 sas_address;
	u16 handle;
	u8 port;

	pr_info("%s search for expanders: start\n",
	       ioc->name);
	if (list_empty(&ioc->sas_expander_list))
		goto out;
	handle = 0xFFFF;
	while (!
	       (leapioraid_config_get_expander_pg0
		(ioc, &mpi_reply, &expander_pg0,
		 LEAPIORAID_SAS_EXPAND_PGAD_FORM_GET_NEXT_HNDL, handle))) {
		ioc_status =
		    le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			pr_info(
				"%s \tbreak from %s: ioc_status(0x%04x), loginfo(0x%08x)\n",
			    ioc->name, __func__, ioc_status,
			    le32_to_cpu(mpi_reply.IOCLogInfo));
			break;
		}
		handle = le16_to_cpu(expander_pg0.DevHandle);
		sas_address = le64_to_cpu(expander_pg0.SASAddress);
		port = expander_pg0.PhysicalPort;
		pr_info(
			"\texpander present: handle(0x%04x), sas_addr(0x%016llx), port:%d\n",
			handle,
		    (unsigned long long)sas_address,
		    ((ioc->multipath_on_hba) ?
			(port) : (LEAPIORAID_MULTIPATH_DISABLED_PORT_ID)));
		leapioraid_scsihost_mark_responding_expander(
			ioc, &expander_pg0);
	}
out:
	pr_info("%s search for expanders: complete\n",
	       ioc->name);
}

static void
leapioraid_scsihost_remove_unresponding_devices(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_sas_device *sas_device, *sas_device_next;
	struct leapioraid_raid_sas_node *sas_expander, *sas_expander_next;
	struct leapioraid_raid_device *raid_device, *raid_device_next;
	struct list_head tmp_list;
	unsigned long flags;
	LIST_HEAD(head);

	pr_info("%s removing unresponding devices: start\n",
	       ioc->name);
	pr_err("%s removing unresponding devices: sas end-devices\n",
		ioc->name);
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	list_for_each_entry_safe(sas_device, sas_device_next,
				 &ioc->sas_device_init_list, list) {
		list_del_init(&sas_device->list);
		leapioraid_sas_device_put(sas_device);
	}
	list_for_each_entry_safe(sas_device, sas_device_next,
				 &ioc->sas_device_list, list) {
		if (!sas_device->responding)
			list_move_tail(&sas_device->list, &head);
		else
			sas_device->responding = 0;
	}
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	list_for_each_entry_safe(sas_device, sas_device_next, &head, list) {
		leapioraid_scsihost_remove_device(ioc, sas_device);
		list_del_init(&sas_device->list);
		leapioraid_sas_device_put(sas_device);
	}
	if (ioc->ir_firmware) {
		pr_info("%s removing unresponding devices: volumes\n",
			ioc->name);
		list_for_each_entry_safe(raid_device, raid_device_next,
					 &ioc->raid_device_list, list) {
			if (!raid_device->responding)
				leapioraid_scsihost_sas_volume_delete(ioc,
							    raid_device->handle);
			else
				raid_device->responding = 0;
		}
	}
	pr_err("%s removing unresponding devices: expanders\n",
		ioc->name);
	spin_lock_irqsave(&ioc->sas_node_lock, flags);
	INIT_LIST_HEAD(&tmp_list);
	list_for_each_entry_safe(sas_expander, sas_expander_next,
				 &ioc->sas_expander_list, list) {
		if (!sas_expander->responding)
			list_move_tail(&sas_expander->list, &tmp_list);
		else
			sas_expander->responding = 0;
	}
	spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
	list_for_each_entry_safe(
		sas_expander, sas_expander_next, &tmp_list, list) {
		leapioraid_scsihost_expander_node_remove(ioc, sas_expander);
	}
	pr_err("%s removing unresponding devices: complete\n", ioc->name);
	leapioraid_scsihost_ublock_io_all_device(ioc, 0);
}

static void
leapioraid_scsihost_refresh_expander_links(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_raid_sas_node *sas_expander, u16 handle)
{
	struct LeapioraidExpanderP1_t expander_pg1;
	struct LeapioraidCfgRep_t mpi_reply;
	int i;

	for (i = 0; i < sas_expander->num_phys; i++) {
		if ((leapioraid_config_get_expander_pg1(ioc, &mpi_reply,
							&expander_pg1, i,
							handle))) {
			pr_err("%s failure at %s:%d/%s()!\n",
			       ioc->name, __FILE__, __LINE__, __func__);
			return;
		}
		leapioraid_transport_update_links(ioc,
			sas_expander->sas_address,
			le16_to_cpu(expander_pg1.AttachedDevHandle),
			i,
			expander_pg1.NegotiatedLinkRate >> 4,
			sas_expander->port);
	}
}

static void
leapioraid_scsihost_scan_for_devices_after_reset(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidExpanderP0_t expander_pg0;
	struct LeapioraidSasDevP0_t sas_device_pg0;
	struct LeapioraidRaidVolP1_t *volume_pg1;
	struct LeapioraidRaidVolP0_t *volume_pg0;
	struct LeapioraidRaidPDP0_t pd_pg0;
	struct LeapioraidEventIrCfgEle_t element;
	struct LeapioraidCfgRep_t mpi_reply;
	u8 phys_disk_num, port_id;
	u16 ioc_status;
	u16 handle, parent_handle;
	u64 sas_address;
	struct leapioraid_sas_device *sas_device;
	struct leapioraid_raid_sas_node *expander_device;
	static struct leapioraid_raid_device *raid_device;
	u8 retry_count;
	unsigned long flags;

	volume_pg0 = kzalloc(sizeof(*volume_pg0), GFP_KERNEL);
	if (!volume_pg0)
		return;

	volume_pg1 = kzalloc(sizeof(*volume_pg1), GFP_KERNEL);
	if (!volume_pg1) {
		kfree(volume_pg0);
		return;
	}
	pr_info("%s scan devices: start\n", ioc->name);
	leapioraid_scsihost_sas_host_refresh(ioc);
	pr_info("%s \tscan devices: expanders start\n",
	       ioc->name);
	handle = 0xFFFF;
	while (!
	       (leapioraid_config_get_expander_pg0
		(ioc, &mpi_reply, &expander_pg0,
		 LEAPIORAID_SAS_EXPAND_PGAD_FORM_GET_NEXT_HNDL, handle))) {
		ioc_status =
		    le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			pr_err(
				"%s \tbreak from expander scan: ioc_status(0x%04x), loginfo(0x%08x)\n",
			    ioc->name, ioc_status,
			    le32_to_cpu(mpi_reply.IOCLogInfo));
			break;
		}
		handle = le16_to_cpu(expander_pg0.DevHandle);
		spin_lock_irqsave(&ioc->sas_node_lock, flags);
		port_id = expander_pg0.PhysicalPort;
		expander_device =
		    leapioraid_scsihost_expander_find_by_sas_address(
				ioc,
				le64_to_cpu
				(expander_pg0.SASAddress),
				leapioraid_get_port_by_id
				(ioc,
				port_id,
				0));
		spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
		if (expander_device)
			leapioraid_scsihost_refresh_expander_links(
				ioc, expander_device, handle);
		else {
			pr_err(
				"%s \tBEFORE adding expander:\n\t\t"
				"handle (0x%04x), sas_addr(0x%016llx)\n",
				ioc->name, handle, (unsigned long long)
				le64_to_cpu(expander_pg0.SASAddress));
			leapioraid_scsihost_expander_add(ioc, handle);
			pr_info(
				"%s \tAFTER adding expander:\n\t\t"
				"handle (0x%04x), sas_addr(0x%016llx)\n",
				ioc->name, handle, (unsigned long long)
				le64_to_cpu(expander_pg0.SASAddress));
		}
	}
	pr_info("%s \tscan devices: expanders complete\n",
	       ioc->name);
	if (!ioc->ir_firmware)
		goto skip_to_sas;
	pr_info("%s \tscan devices: phys disk start\n",
	       ioc->name);
	phys_disk_num = 0xFF;
	while (!(leapioraid_config_get_phys_disk_pg0(ioc, &mpi_reply,
			&pd_pg0,
			LEAPIORAID_PHYSDISK_PGAD_FORM_GET_NEXT_PHYSDISKNUM,
			phys_disk_num))) {
		ioc_status =
		    le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			pr_err(
				"%s \tbreak from phys disk scan:\n\t\t"
				"ioc_status(0x%04x), loginfo(0x%08x)\n",
					ioc->name,
					ioc_status,
					le32_to_cpu(mpi_reply.IOCLogInfo));
			break;
		}
		phys_disk_num = pd_pg0.PhysDiskNum;
		handle = le16_to_cpu(pd_pg0.DevHandle);
		sas_device = leapioraid_get_sdev_by_handle(ioc, handle);
		if (sas_device) {
			leapioraid_sas_device_put(sas_device);
			continue;
		}
		if (leapioraid_config_get_sas_device_pg0(ioc, &mpi_reply,
							 &sas_device_pg0,
							 LEAPIORAID_SAS_DEVICE_PGAD_FORM_HANDLE,
							 handle) != 0)
			continue;
		ioc_status = le16_to_cpu(mpi_reply.IOCStatus) &
		    LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			pr_err(
				"%s \tbreak from phys disk scan ioc_status(0x%04x), loginfo(0x%08x)\n",
			    ioc->name, ioc_status,
			    le32_to_cpu(mpi_reply.IOCLogInfo));
			break;
		}
		parent_handle = le16_to_cpu(sas_device_pg0.ParentDevHandle);
		if (!leapioraid_scsihost_get_sas_address(ioc, parent_handle,
					       &sas_address)) {
			pr_err(
				"%s \tBEFORE adding phys disk:\n\t\t"
				"handle (0x%04x), sas_addr(0x%016llx)\n",
				ioc->name, handle, (unsigned long long)
				le64_to_cpu(sas_device_pg0.SASAddress));
			port_id = sas_device_pg0.PhysicalPort;
			leapioraid_transport_update_links(ioc, sas_address,
							  handle,
							  sas_device_pg0.PhyNum,
							  LEAPIORAID_SAS_NEG_LINK_RATE_1_5,
							  leapioraid_get_port_by_id
							  (ioc, port_id, 0));
			set_bit(handle, ioc->pd_handles);
			retry_count = 0;
			while (leapioraid_scsihost_add_device
			       (ioc, handle, retry_count++, 1)) {
				ssleep(1);
			}
			pr_err(
				"%s \tAFTER adding phys disk:\n\t\t"
				"handle (0x%04x), sas_addr(0x%016llx)\n",
				ioc->name, handle, (unsigned long long)
				le64_to_cpu(sas_device_pg0.SASAddress));
		}
	}
	pr_info("%s \tscan devices: phys disk complete\n",
		ioc->name);
	pr_info("%s \tscan devices: volumes start\n",
		ioc->name);
	handle = 0xFFFF;
	while (!(leapioraid_config_get_raid_volume_pg1(ioc, &mpi_reply,
			volume_pg1,
			LEAPIORAID_RAID_VOLUME_PGAD_FORM_GET_NEXT_HANDLE,
			handle))) {
		ioc_status =
		    le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			pr_err(
				"%s \tbreak from volume scan: ioc_status(0x%04x), loginfo(0x%08x)\n",
			    ioc->name, ioc_status,
			    le32_to_cpu(mpi_reply.IOCLogInfo));
			break;
		}
		handle = le16_to_cpu(volume_pg1->DevHandle);
		spin_lock_irqsave(&ioc->raid_device_lock, flags);
		raid_device = leapioraid_scsihost_raid_device_find_by_wwid(
			ioc, le64_to_cpu(volume_pg1->WWID));
		spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
		if (raid_device)
			continue;
		if (leapioraid_config_get_raid_volume_pg0(ioc, &mpi_reply,
							  volume_pg0,
							  LEAPIORAID_RAID_VOLUME_PGAD_FORM_HANDLE,
							  handle,
							  sizeof
							  (struct LeapioraidRaidVolP0_t)))
			continue;
		ioc_status = le16_to_cpu(mpi_reply.IOCStatus) &
		    LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			pr_err(
				"%s \tbreak from volume scan: ioc_status(0x%04x), loginfo(0x%08x)\n",
			    ioc->name, ioc_status,
			    le32_to_cpu(mpi_reply.IOCLogInfo));
			break;
		}
		if (volume_pg0->VolumeState == LEAPIORAID_RAID_VOL_STATE_OPTIMAL ||
		    volume_pg0->VolumeState == LEAPIORAID_RAID_VOL_STATE_ONLINE ||
		    volume_pg0->VolumeState ==
		    LEAPIORAID_RAID_VOL_STATE_DEGRADED) {
			memset(&element, 0,
			       sizeof(struct LeapioraidEventIrCfgEle_t));
			element.ReasonCode = LEAPIORAID_EVENT_IR_CHANGE_RC_ADDED;
			element.VolDevHandle = volume_pg1->DevHandle;
			pr_info("%s \tBEFORE adding volume:  handle (0x%04x)\n",
				ioc->name, volume_pg1->DevHandle);
			leapioraid_scsihost_sas_volume_add(ioc, &element);
			pr_info("%s \tAFTER adding volume:  handle (0x%04x)\n",
				ioc->name, volume_pg1->DevHandle);
		}
	}
	pr_info("%s \tscan devices: volumes complete\n",
	       ioc->name);
skip_to_sas:
	pr_info("%s \tscan devices: sas end devices start\n",
	       ioc->name);
	handle = 0xFFFF;
	while (!(leapioraid_config_get_sas_device_pg0(ioc, &mpi_reply,
			&sas_device_pg0,
			LEAPIORAID_SAS_DEVICE_PGAD_FORM_GET_NEXT_HANDLE,
			handle))) {
		ioc_status =
		    le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
			pr_err(
				"%s \tbreak from sas end device scan: ioc_status(0x%04x), loginfo(0x%08x)\n",
			    ioc->name, ioc_status,
			    le32_to_cpu(mpi_reply.IOCLogInfo));
			break;
		}
		handle = le16_to_cpu(sas_device_pg0.DevHandle);
		if (!
		    (leapioraid_scsihost_is_sas_end_device
		     (le32_to_cpu(sas_device_pg0.DeviceInfo))))
			continue;
		port_id = sas_device_pg0.PhysicalPort;
		sas_device = leapioraid_get_sdev_by_addr(ioc,
							 le64_to_cpu
							 (sas_device_pg0.SASAddress),
							 leapioraid_get_port_by_id
							 (ioc, port_id, 0));
		if (sas_device) {
			leapioraid_sas_device_put(sas_device);
			continue;
		}
		parent_handle = le16_to_cpu(sas_device_pg0.ParentDevHandle);
		if (!leapioraid_scsihost_get_sas_address
		    (ioc, parent_handle, &sas_address)) {
			pr_err(
				"%s \tBEFORE adding sas end device:\n\t\t"
				"handle (0x%04x), sas_addr(0x%016llx)\n",
				ioc->name, handle, (unsigned long long)
				le64_to_cpu(sas_device_pg0.SASAddress));
			leapioraid_transport_update_links(ioc, sas_address,
							handle,
							sas_device_pg0.PhyNum,
							LEAPIORAID_SAS_NEG_LINK_RATE_1_5,
							leapioraid_get_port_by_id
							(ioc, port_id, 0));
			retry_count = 0;
			while (leapioraid_scsihost_add_device
			       (ioc, handle, retry_count++, 0)) {
				ssleep(1);
			}
			pr_err(
				"%s \tAFTER adding sas end device:\n\t\t"
				"handle (0x%04x), sas_addr(0x%016llx)\n",
				ioc->name, handle, (unsigned long long)
				le64_to_cpu(sas_device_pg0.SASAddress));
		}
	}
	pr_err("%s \tscan devices: sas end devices complete\n", ioc->name);
	kfree(volume_pg0);
	kfree(volume_pg1);
	pr_info("%s scan devices: complete\n", ioc->name);
}

void
leapioraid_scsihost_clear_outstanding_scsi_tm_commands(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_internal_qcmd *scsih_qcmd, *scsih_qcmd_next;
	unsigned long flags;

	if (ioc->scsih_cmds.status & LEAPIORAID_CMD_PENDING) {
		ioc->scsih_cmds.status |= LEAPIORAID_CMD_RESET;
		leapioraid_base_free_smid(ioc, ioc->scsih_cmds.smid);
		complete(&ioc->scsih_cmds.done);
	}
	if (ioc->tm_cmds.status & LEAPIORAID_CMD_PENDING) {
		ioc->tm_cmds.status |= LEAPIORAID_CMD_RESET;
		leapioraid_base_free_smid(ioc, ioc->tm_cmds.smid);
		complete(&ioc->tm_cmds.done);
	}
	spin_lock_irqsave(&ioc->scsih_q_internal_lock, flags);
	list_for_each_entry_safe(scsih_qcmd, scsih_qcmd_next,
				 &ioc->scsih_q_intenal_cmds, list) {
		scsih_qcmd->status |= LEAPIORAID_CMD_RESET;
		leapioraid_base_free_smid(ioc, scsih_qcmd->smid);
	}
	spin_unlock_irqrestore(&ioc->scsih_q_internal_lock, flags);
	memset(ioc->pend_os_device_add, 0, ioc->pend_os_device_add_sz);
	memset(ioc->device_remove_in_progress, 0,
	       ioc->device_remove_in_progress_sz);
	memset(ioc->tm_tr_retry, 0, ioc->tm_tr_retry_sz);
	leapioraid_scsihost_fw_event_cleanup_queue(ioc);
	leapioraid_scsihost_flush_running_cmds(ioc);
}

void
leapioraid_scsihost_reset_handler(struct LEAPIORAID_ADAPTER *ioc,
				  int reset_phase)
{
	switch (reset_phase) {
	case LEAPIORAID_IOC_PRE_RESET_PHASE:
		dtmprintk(ioc, pr_info(
			"%s %s: LEAPIORAID_IOC_PRE_RESET_PHASE\n",
				ioc->name, __func__));
		break;
	case LEAPIORAID_IOC_AFTER_RESET_PHASE:
		dtmprintk(ioc, pr_info(
			"%s %s: LEAPIORAID_IOC_AFTER_RESET_PHASE\n",
				ioc->name, __func__));
		leapioraid_scsihost_clear_outstanding_scsi_tm_commands(ioc);
		break;
	case LEAPIORAID_IOC_DONE_RESET_PHASE:
		dtmprintk(ioc, pr_info(
			"%s %s: LEAPIORAID_IOC_DONE_RESET_PHASE\n",
				ioc->name, __func__));
		if (!(disable_discovery > 0 && !ioc->sas_hba.num_phys)) {
			if (ioc->multipath_on_hba) {
				leapioraid_scsihost_sas_port_refresh(ioc);
				leapioraid_scsihost_update_vphys_after_reset(ioc);
			}
			leapioraid_scsihost_prep_device_scan(ioc);
			leapioraid_scsihost_create_enclosure_list_after_reset(ioc);
			leapioraid_scsihost_search_responding_sas_devices(ioc);
			leapioraid_scsihost_search_responding_raid_devices(ioc);
			leapioraid_scsihost_search_responding_expanders(ioc);
			leapioraid_scsihost_error_recovery_delete_devices(ioc);
		}
		break;
	}
}

static void
leapioraid_fw_work(struct LEAPIORAID_ADAPTER *ioc,
		    struct leapioraid_fw_event_work *fw_event)
{
	ioc->current_event = fw_event;
	leapioraid_scsihost_fw_event_del_from_list(ioc, fw_event);
	if (ioc->remove_host || ioc->pci_error_recovery) {
		leapioraid_fw_event_work_put(fw_event);
		ioc->current_event = NULL;
		return;
	}
	switch (fw_event->event) {
	case LEAPIORAID_REMOVE_UNRESPONDING_DEVICES:
		while (scsi_host_in_recovery(ioc->shost) || ioc->shost_recovery) {
			if (ioc->remove_host || ioc->fw_events_cleanup)
				goto out;
			ssleep(1);
		}
		leapioraid_scsihost_remove_unresponding_devices(ioc);
		leapioraid_scsihost_del_dirty_vphy(ioc);
		leapioraid_scsihost_del_dirty_port_entries(ioc);
		leapioraid_scsihost_update_device_qdepth(ioc);
		leapioraid_scsihost_scan_for_devices_after_reset(ioc);
		if (ioc->is_driver_loading)
			leapioraid_scsihost_complete_devices_scanning(ioc);
		break;
	case LEAPIORAID_PORT_ENABLE_COMPLETE:
		ioc->start_scan = 0;
		dewtprintk(ioc, pr_info(
			"%s port enable: complete from worker thread\n",
			ioc->name));
		break;
	case LEAPIORAID_TURN_ON_PFA_LED:
		leapioraid_scsihost_turn_on_pfa_led(ioc, fw_event->device_handle);
		break;
	case LEAPIORAID_EVENT_SAS_TOPOLOGY_CHANGE_LIST:
		if (leapioraid_scsihost_sas_topology_change_event(ioc, fw_event)) {
			leapioraid_scsihost_fw_event_requeue(ioc, fw_event, 1000);
			ioc->current_event = NULL;
			return;
		}
		break;
	case LEAPIORAID_EVENT_SAS_DEVICE_STATUS_CHANGE:
		if (ioc->logging_level & LEAPIORAID_DEBUG_EVENT_WORK_TASK)
			leapioraid_scsihost_sas_device_status_change_event_debug(
				ioc,
				(struct LeapioraidEventDataSasDeviceStatusChange_t *)
					fw_event->event_data);
		break;
	case LEAPIORAID_EVENT_SAS_DISCOVERY:
		leapioraid_scsihost_sas_discovery_event(
			ioc, fw_event);
		break;
	case LEAPIORAID_EVENT_SAS_DEVICE_DISCOVERY_ERROR:
		leapioraid_scsihost_sas_device_discovery_error_event(
			ioc, fw_event);
		break;
	case LEAPIORAID_EVENT_SAS_BROADCAST_PRIMITIVE:
		leapioraid_scsihost_sas_broadcast_primitive_event(
			ioc, fw_event);
		break;
	case LEAPIORAID_EVENT_SAS_ENCL_DEVICE_STATUS_CHANGE:
		leapioraid_scsihost_sas_enclosure_dev_status_change_event(
			ioc, fw_event);
		break;
	case LEAPIORAID_EVENT_IR_CONFIGURATION_CHANGE_LIST:
		leapioraid_scsihost_sas_ir_config_change_event(
			ioc, fw_event);
		break;
	case LEAPIORAID_EVENT_IR_VOLUME:
		leapioraid_scsihost_sas_ir_volume_event(
			ioc, fw_event);
		break;
	case LEAPIORAID_EVENT_IR_PHYSICAL_DISK:
		leapioraid_scsihost_sas_ir_physical_disk_event(
			ioc, fw_event);
		break;
	case LEAPIORAID_EVENT_IR_OPERATION_STATUS:
		leapioraid_scsihost_sas_ir_operation_status_event(
			ioc, fw_event);
		break;
	default:
		break;
	}
out:
	leapioraid_fw_event_work_put(fw_event);
	ioc->current_event = NULL;
}

static void
leapioraid_firmware_event_work(struct work_struct *work)
{
	struct leapioraid_fw_event_work *fw_event = container_of(work,
						      struct leapioraid_fw_event_work,
						      work);

	leapioraid_fw_work(fw_event->ioc, fw_event);
}

static void
leapioraid_firmware_event_work_delayed(struct work_struct *work)
{
	struct leapioraid_fw_event_work *fw_event = container_of(work,
						      struct leapioraid_fw_event_work,
						      delayed_work.work);

	leapioraid_fw_work(fw_event->ioc, fw_event);
}

u8
leapioraid_scsihost_event_callback(struct LEAPIORAID_ADAPTER *ioc,
				   u8 msix_index, u32 reply)
{
	struct leapioraid_fw_event_work *fw_event;
	struct LeapioraidEventNotificationRep_t *mpi_reply;
	u16 event;
	u16 sz;

	if (ioc->pci_error_recovery)
		return 1;

	mpi_reply = leapioraid_base_get_reply_virt_addr(ioc, reply);
	if (unlikely(!mpi_reply)) {
		pr_err("%s mpi_reply not valid at %s:%d/%s()!\n", ioc->name,
		       __FILE__, __LINE__, __func__);
		return 1;
	}
	event = le16_to_cpu(mpi_reply->Event);
	switch (event) {
	case LEAPIORAID_EVENT_SAS_BROADCAST_PRIMITIVE:
	{
		struct LeapioraidEventDataSasBroadcastPrimitive_t *baen_data =
			(struct LeapioraidEventDataSasBroadcastPrimitive_t *)
			mpi_reply->EventData;
		if (baen_data->Primitive !=
			LEAPIORAID_EVENT_PRIMITIVE_ASYNCHRONOUS_EVENT)
			return 1;
		if (ioc->broadcast_aen_busy) {
			ioc->broadcast_aen_pending++;
			return 1;
		}
		ioc->broadcast_aen_busy = 1;
		break;
	}
	case LEAPIORAID_EVENT_SAS_TOPOLOGY_CHANGE_LIST:
		leapioraid_scsihost_check_topo_delete_events(
			ioc,
			(struct LeapioraidEventDataSasTopoChangeList_t *)
			mpi_reply->EventData);
		if (ioc->shost_recovery)
			return 1;
		break;
	case LEAPIORAID_EVENT_IR_CONFIGURATION_CHANGE_LIST:
		leapioraid_scsihost_check_ir_config_unhide_events(
			ioc,
			(struct LeapioraidEventDataIrCfgChangeList_t *)
			mpi_reply->EventData);
		break;
	case LEAPIORAID_EVENT_IR_VOLUME:
		leapioraid_scsihost_check_volume_delete_events(
			ioc,
			(struct LeapioraidEventDataIrVol_t *)
			mpi_reply->EventData);
		break;
	case LEAPIORAID_EVENT_LOG_ENTRY_ADDED:
		fallthrough;
	case LEAPIORAID_EVENT_SAS_DEVICE_STATUS_CHANGE:
		leapioraid_scsihost_sas_device_status_change_event(
			ioc,
			(struct LeapioraidEventDataSasDeviceStatusChange_t *)
			mpi_reply->EventData);
		break;
	case LEAPIORAID_EVENT_IR_OPERATION_STATUS:
	case LEAPIORAID_EVENT_SAS_DISCOVERY:
	case LEAPIORAID_EVENT_SAS_DEVICE_DISCOVERY_ERROR:
	case LEAPIORAID_EVENT_SAS_ENCL_DEVICE_STATUS_CHANGE:
	case LEAPIORAID_EVENT_IR_PHYSICAL_DISK:
		break;
	default:
		return 1;
	}
	fw_event = leapioraid_alloc_fw_event_work(0);
	if (!fw_event) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return 1;
	}
	sz = le16_to_cpu(mpi_reply->EventDataLength) * 4;
	fw_event->event_data = kzalloc(sz, GFP_ATOMIC);
	if (!fw_event->event_data) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		leapioraid_fw_event_work_put(fw_event);
		return 1;
	}
	if (event == LEAPIORAID_EVENT_SAS_TOPOLOGY_CHANGE_LIST) {
		struct LeapioraidEventDataSasTopoChangeList_t *topo_event_data =
		    (struct LeapioraidEventDataSasTopoChangeList_t *)
		    mpi_reply->EventData;
		fw_event->retries = kzalloc(topo_event_data->NumEntries,
					    GFP_ATOMIC);
		if (!fw_event->retries) {
			kfree(fw_event->event_data);
			leapioraid_fw_event_work_put(fw_event);
			return 1;
		}
	}
	memcpy(fw_event->event_data, mpi_reply->EventData, sz);
	fw_event->ioc = ioc;
	fw_event->VF_ID = mpi_reply->VF_ID;
	fw_event->VP_ID = mpi_reply->VP_ID;
	fw_event->event = event;
	leapioraid_scsihost_fw_event_add(ioc, fw_event);
	leapioraid_fw_event_work_put(fw_event);
	return 1;
}

static void
leapioraid_scsihost_expander_node_remove(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_raid_sas_node *sas_expander)
{
	struct leapioraid_sas_port *leapioraid_port, *next;
	unsigned long flags;
	int port_id;

	list_for_each_entry_safe(leapioraid_port, next,
				 &sas_expander->sas_port_list, port_list) {
		if (ioc->shost_recovery)
			return;
		if (leapioraid_port->remote_identify.device_type ==
		    SAS_END_DEVICE)
			leapioraid_device_remove_by_sas_address(ioc,
				leapioraid_port->remote_identify.sas_address,
				leapioraid_port->hba_port);
		else if (leapioraid_port->remote_identify.device_type ==
			 SAS_EDGE_EXPANDER_DEVICE
			 || leapioraid_port->remote_identify.device_type ==
			 SAS_FANOUT_EXPANDER_DEVICE)
			leapioraid_expander_remove(ioc,
				leapioraid_port->remote_identify.sas_address,
				leapioraid_port->hba_port);
	}
	port_id = sas_expander->port->port_id;
	leapioraid_transport_port_remove(ioc, sas_expander->sas_address,
					 sas_expander->sas_address_parent,
					 sas_expander->port);
	pr_info(
		"%s expander_remove: handle(0x%04x), sas_addr(0x%016llx), port:%d\n",
		ioc->name,
		sas_expander->handle,
		(unsigned long long)sas_expander->sas_address,
		port_id);
	spin_lock_irqsave(&ioc->sas_node_lock, flags);
	list_del(&sas_expander->list);
	spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
	kfree(sas_expander->phy);
	kfree(sas_expander);
}

static void
leapioraid_scsihost_ir_shutdown(struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidRaidActionReq_t *mpi_request;
	struct LeapioraidRaidActionRep_t *mpi_reply;
	u16 smid;

	if (!ioc->ir_firmware)
		return;

	if (list_empty(&ioc->raid_device_list))
		return;
	if (leapioraid_base_pci_device_is_unplugged(ioc))
		return;
	mutex_lock(&ioc->scsih_cmds.mutex);
	if (ioc->scsih_cmds.status != LEAPIORAID_CMD_NOT_USED) {
		pr_err("%s %s: scsih_cmd in use\n",
		       ioc->name, __func__);
		goto out;
	}
	ioc->scsih_cmds.status = LEAPIORAID_CMD_PENDING;
	smid = leapioraid_base_get_smid(ioc, ioc->scsih_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		ioc->scsih_cmds.status = LEAPIORAID_CMD_NOT_USED;
		goto out;
	}
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->scsih_cmds.smid = smid;
	memset(mpi_request, 0, sizeof(struct LeapioraidRaidActionReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_RAID_ACTION;
	mpi_request->Action = 0x20;
	if (!ioc->warpdrive_msg)
		pr_info("%s IR shutdown (sending)\n",
		       ioc->name);
	init_completion(&ioc->scsih_cmds.done);
	ioc->put_smid_default(ioc, smid);
	wait_for_completion_timeout(&ioc->scsih_cmds.done, 10 * HZ);
	if (!(ioc->scsih_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		pr_err("%s %s: timeout\n",
		       ioc->name, __func__);
		goto out;
	}
	if (ioc->scsih_cmds.status & LEAPIORAID_CMD_REPLY_VALID) {
		mpi_reply = ioc->scsih_cmds.reply;
		if (!ioc->warpdrive_msg)
			pr_info(
				"%s IR shutdown (complete): ioc_status(0x%04x), loginfo(0x%08x)\n",
			    ioc->name, le16_to_cpu(mpi_reply->IOCStatus),
			    le32_to_cpu(mpi_reply->IOCLogInfo));
	}
out:
	ioc->scsih_cmds.status = LEAPIORAID_CMD_NOT_USED;
	mutex_unlock(&ioc->scsih_cmds.mutex);
}

static int
leapioraid_scsihost_get_shost_and_ioc(struct pci_dev *pdev,
			    struct Scsi_Host **shost,
			    struct LEAPIORAID_ADAPTER **ioc)
{
	*shost = pci_get_drvdata(pdev);
	if (*shost == NULL) {
		dev_err(&pdev->dev, "pdev's driver data is null\n");
		return -ENXIO;
	}
	*ioc = leapioraid_shost_private(*shost);
	if (*ioc == NULL) {
		dev_err(&pdev->dev, "shost's private data is null\n");
		return -ENXIO;
	}
	return 0;
}

static void
leapioraid_scsihost_remove(struct pci_dev *pdev)
{
	struct Scsi_Host *shost = NULL;
	struct LEAPIORAID_ADAPTER *ioc = NULL;
	struct leapioraid_sas_port *leapioraid_port, *next_port;
	struct leapioraid_raid_device *raid_device, *next;
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	struct workqueue_struct *wq;
	unsigned long flags;
	struct leapioraid_hba_port *port, *port_next;
	struct leapioraid_virtual_phy *vphy, *vphy_next;
	struct LeapioraidCfgRep_t mpi_reply;

	if (leapioraid_scsihost_get_shost_and_ioc(pdev, &shost, &ioc)) {
		dev_err(&pdev->dev, "unable to remove device\n");
		return;
	}

	while (ioc->is_driver_loading)
		ssleep(1);

	ioc->remove_host = 1;
	leapioraid_wait_for_commands_to_complete(ioc);
	spin_lock_irqsave(&ioc->hba_hot_unplug_lock, flags);
	if (leapioraid_base_pci_device_is_unplugged(ioc)) {
		leapioraid_base_pause_mq_polling(ioc);
		leapioraid_scsihost_flush_running_cmds(ioc);
	}
	leapioraid_scsihost_fw_event_cleanup_queue(ioc);
	spin_unlock_irqrestore(&ioc->hba_hot_unplug_lock, flags);
	spin_lock_irqsave(&ioc->fw_event_lock, flags);
	wq = ioc->firmware_event_thread;
	ioc->firmware_event_thread = NULL;
	spin_unlock_irqrestore(&ioc->fw_event_lock, flags);
	if (wq)
		destroy_workqueue(wq);
	leapioraid_config_set_ioc_pg1(ioc, &mpi_reply,
						&ioc->ioc_pg1_copy);
	leapioraid_scsihost_ir_shutdown(ioc);
	sas_remove_host(shost);
	scsi_remove_host(shost);
	list_for_each_entry_safe(raid_device, next, &ioc->raid_device_list,
				 list) {
		if (raid_device->starget) {
			sas_target_priv_data = raid_device->starget->hostdata;
			sas_target_priv_data->deleted = 1;
			scsi_remove_target(&raid_device->starget->dev);
		}
		pr_info("%s removing handle(0x%04x), wwid(0x%016llx)\n",
			ioc->name, raid_device->handle,
		    (unsigned long long)raid_device->wwid);
		leapioraid_scsihost_raid_device_remove(ioc, raid_device);
	}
	list_for_each_entry_safe(leapioraid_port, next_port,
				 &ioc->sas_hba.sas_port_list, port_list) {
		if (leapioraid_port->remote_identify.device_type ==
		    SAS_END_DEVICE)
			leapioraid_device_remove_by_sas_address(ioc,
				leapioraid_port->remote_identify.sas_address,
				leapioraid_port->hba_port);
		else if (leapioraid_port->remote_identify.device_type ==
			 SAS_EDGE_EXPANDER_DEVICE
			 || leapioraid_port->remote_identify.device_type ==
			 SAS_FANOUT_EXPANDER_DEVICE)
			leapioraid_expander_remove(ioc,
				leapioraid_port->remote_identify.sas_address,
				leapioraid_port->hba_port);
	}
	list_for_each_entry_safe(port, port_next, &ioc->port_table_list, list) {
		if (port->vphys_mask) {
			list_for_each_entry_safe(vphy, vphy_next,
						 &port->vphys_list, list) {
				list_del(&vphy->list);
				kfree(vphy);
			}
		}
		list_del(&port->list);
		kfree(port);
	}
	if (ioc->sas_hba.num_phys) {
		kfree(ioc->sas_hba.phy);
		ioc->sas_hba.phy = NULL;
		ioc->sas_hba.num_phys = 0;
	}
	leapioraid_base_detach(ioc);
	spin_lock(&leapioraid_gioc_lock);
	list_del(&ioc->list);
	spin_unlock(&leapioraid_gioc_lock);
	scsi_host_put(shost);
}

static void
leapioraid_scsihost_shutdown(struct pci_dev *pdev)
{
	struct Scsi_Host *shost = NULL;
	struct LEAPIORAID_ADAPTER *ioc = NULL;
	struct workqueue_struct *wq;
	unsigned long flags;
	struct LeapioraidCfgRep_t mpi_reply;

	if (leapioraid_scsihost_get_shost_and_ioc(pdev, &shost, &ioc)) {
		dev_err(&pdev->dev, "unable to shutdown device\n");
		return;
	}
	ioc->remove_host = 1;
	leapioraid_wait_for_commands_to_complete(ioc);
	leapioraid_scsihost_fw_event_cleanup_queue(ioc);
	spin_lock_irqsave(&ioc->fw_event_lock, flags);
	wq = ioc->firmware_event_thread;
	ioc->firmware_event_thread = NULL;
	spin_unlock_irqrestore(&ioc->fw_event_lock, flags);
	if (wq)
		destroy_workqueue(wq);
	leapioraid_config_set_ioc_pg1(ioc, &mpi_reply,
						&ioc->ioc_pg1_copy);
	leapioraid_scsihost_ir_shutdown(ioc);
	leapioraid_base_mask_interrupts(ioc);
	ioc->shost_recovery = 1;
	leapioraid_base_make_ioc_ready(ioc, SOFT_RESET);
	ioc->shost_recovery = 0;
	leapioraid_base_free_irq(ioc);
	leapioraid_base_disable_msix(ioc);
}

static void
leapioraid_scsihost_probe_boot_devices(struct LEAPIORAID_ADAPTER *ioc)
{
	u32 channel;
	void *device;
	struct leapioraid_sas_device *sas_device;
	struct leapioraid_raid_device *raid_device;
	u16 handle;
	u64 sas_address_parent;
	u64 sas_address;
	unsigned long flags;
	int rc;
	struct leapioraid_hba_port *port;
	u8 protection_mask;

	if (!ioc->bios_pg3.BiosVersion)
		return;

	device = NULL;
	if (ioc->req_boot_device.device) {
		device = ioc->req_boot_device.device;
		channel = ioc->req_boot_device.channel;
	} else if (ioc->req_alt_boot_device.device) {
		device = ioc->req_alt_boot_device.device;
		channel = ioc->req_alt_boot_device.channel;
	} else if (ioc->current_boot_device.device) {
		device = ioc->current_boot_device.device;
		channel = ioc->current_boot_device.channel;
	}
	if (!device)
		return;
	if (channel == RAID_CHANNEL) {
		raid_device = device;
		if (raid_device->starget)
			return;
		if (!ioc->disable_eedp_support) {
			protection_mask = scsi_host_get_prot(ioc->shost);
			if (protection_mask & SHOST_DIX_TYPE0_PROTECTION) {
				scsi_host_set_prot(ioc->shost,
						   protection_mask & 0x77);
				pr_err(
					"%s: Disabling DIX0 because of unsupport!\n",
						ioc->name);
			}
		}
		rc = scsi_add_device(ioc->shost, RAID_CHANNEL,
				     raid_device->id, 0);
		if (rc)
			leapioraid_scsihost_raid_device_remove(ioc, raid_device);
	} else {
		sas_device = device;
		if (sas_device->starget)
			return;
		spin_lock_irqsave(&ioc->sas_device_lock, flags);
		handle = sas_device->handle;
		sas_address_parent = sas_device->sas_address_parent;
		sas_address = sas_device->sas_address;
		port = sas_device->port;
		list_move_tail(&sas_device->list, &ioc->sas_device_list);
		spin_unlock_irqrestore(&ioc->sas_device_lock, flags);

		if (!port)
			return;

		if (ioc->hide_drives)
			return;

		if (!leapioraid_transport_port_add(ioc, handle,
						   sas_address_parent, port)) {
			leapioraid_scsihost_sas_device_remove(ioc, sas_device);
		} else if (!sas_device->starget) {
			if (!ioc->is_driver_loading) {
				leapioraid_transport_port_remove(ioc,
								 sas_address,
								 sas_address_parent,
								 port);
				leapioraid_scsihost_sas_device_remove(ioc, sas_device);
			}
		}
	}
}

static void
leapioraid_scsihost_probe_raid(struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_raid_device *raid_device, *raid_next;
	int rc;

	list_for_each_entry_safe(raid_device, raid_next,
				 &ioc->raid_device_list, list) {
		if (raid_device->starget)
			continue;
		rc = scsi_add_device(ioc->shost, RAID_CHANNEL,
				     raid_device->id, 0);
		if (rc)
			leapioraid_scsihost_raid_device_remove(ioc, raid_device);
	}
}

static
struct leapioraid_sas_device *leapioraid_get_next_sas_device(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_sas_device *sas_device = NULL;
	unsigned long flags;

	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	if (!list_empty(&ioc->sas_device_init_list)) {
		sas_device = list_first_entry(&ioc->sas_device_init_list,
					      struct leapioraid_sas_device, list);
		leapioraid_sas_device_get(sas_device);
	}
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	return sas_device;
}

static void
leapioraid_sas_device_make_active(struct LEAPIORAID_ADAPTER *ioc,
				   struct leapioraid_sas_device *sas_device)
{
	unsigned long flags;

	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	if (!list_empty(&sas_device->list)) {
		list_del_init(&sas_device->list);
		leapioraid_sas_device_put(sas_device);
	}
	leapioraid_sas_device_get(sas_device);
	list_add_tail(&sas_device->list, &ioc->sas_device_list);
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
}

static void
leapioraid_scsihost_probe_sas(struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_sas_device *sas_device;

	while ((sas_device = leapioraid_get_next_sas_device(ioc))) {
		if (ioc->hide_drives) {
			leapioraid_sas_device_make_active(ioc, sas_device);
			leapioraid_sas_device_put(sas_device);
			continue;
		}
		if (!leapioraid_transport_port_add(ioc, sas_device->handle,
						   sas_device->sas_address_parent,
						   sas_device->port)) {
			leapioraid_scsihost_sas_device_remove(ioc, sas_device);
			leapioraid_sas_device_put(sas_device);
			continue;
		} else if (!sas_device->starget) {
			if (!ioc->is_driver_loading) {
				leapioraid_transport_port_remove(ioc,
								 sas_device->sas_address,
								 sas_device->sas_address_parent,
								 sas_device->port);
				leapioraid_scsihost_sas_device_remove(ioc, sas_device);
				leapioraid_sas_device_put(sas_device);
				continue;
			}
		}
		leapioraid_sas_device_make_active(ioc, sas_device);
		leapioraid_sas_device_put(sas_device);
	}
}

static void
leapioraid_scsihost_probe_devices(struct LEAPIORAID_ADAPTER *ioc)
{
	u16 volume_mapping_flags;

	if (!(ioc->facts.ProtocolFlags
		& LEAPIORAID_IOCFACTS_PROTOCOL_SCSI_INITIATOR))
		return;
	leapioraid_scsihost_probe_boot_devices(ioc);

	if (ioc->ir_firmware) {
		volume_mapping_flags =
		    le16_to_cpu(ioc->ioc_pg8.IRVolumeMappingFlags) &
		    LEAPIORAID_IOCPAGE8_IRFLAGS_MASK_VOLUME_MAPPING_MODE;
		if (volume_mapping_flags ==
		    LEAPIORAID_IOCPAGE8_IRFLAGS_LOW_VOLUME_MAPPING) {
			leapioraid_scsihost_probe_raid(ioc);
			leapioraid_scsihost_probe_sas(ioc);
		} else {
			leapioraid_scsihost_probe_sas(ioc);
			leapioraid_scsihost_probe_raid(ioc);
		}
	} else {
		leapioraid_scsihost_probe_sas(ioc);
	}
}

static void
leapioraid_scsihost_scan_start(struct Scsi_Host *shost)
{
	struct LEAPIORAID_ADAPTER *ioc = shost_priv(shost);
	int rc;

	if (disable_discovery > 0)
		return;
	ioc->start_scan = 1;
	rc = leapioraid_port_enable(ioc);
	if (rc != 0)
		pr_info("%s port enable: FAILED\n",
		       ioc->name);
}

void
leapioraid_scsihost_complete_devices_scanning(struct LEAPIORAID_ADAPTER *ioc)
{
	if (ioc->wait_for_discovery_to_complete) {
		ioc->wait_for_discovery_to_complete = 0;
		leapioraid_scsihost_probe_devices(ioc);
	}
	leapioraid_base_start_watchdog(ioc);
	ioc->is_driver_loading = 0;
}

static int
leapioraid_scsihost_scan_finished(
	struct Scsi_Host *shost, unsigned long time)
{
	struct LEAPIORAID_ADAPTER *ioc = shost_priv(shost);
	u32 ioc_state;
	int issue_hard_reset = 0;

	if (disable_discovery > 0) {
		ioc->is_driver_loading = 0;
		ioc->wait_for_discovery_to_complete = 0;
		goto out;
	}
	if (time >= (300 * HZ)) {
		ioc->port_enable_cmds.status = LEAPIORAID_CMD_NOT_USED;
		pr_info("%s port enable: FAILED with timeout (timeout=300s)\n",
			ioc->name);
		ioc->is_driver_loading = 0;
		goto out;
	}
	if (ioc->start_scan) {
		ioc_state = leapioraid_base_get_iocstate(ioc, 0);
		if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) ==
		    LEAPIORAID_IOC_STATE_FAULT) {
			leapioraid_print_fault_code(ioc,
						    ioc_state &
						    LEAPIORAID_DOORBELL_DATA_MASK);
			issue_hard_reset = 1;
			goto out;
		} else if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) ==
			   LEAPIORAID_IOC_STATE_COREDUMP) {
			leapioraid_base_coredump_info(ioc,
						      ioc_state &
						      LEAPIORAID_DOORBELL_DATA_MASK);
			leapioraid_base_wait_for_coredump_completion(ioc,
								     __func__);
			issue_hard_reset = 1;
			goto out;
		}
		return 0;
	}
	if (ioc->port_enable_cmds.status & LEAPIORAID_CMD_RESET) {
		pr_err("%s port enable: aborted due to diag reset\n",
			ioc->name);
		ioc->port_enable_cmds.status = LEAPIORAID_CMD_NOT_USED;
		goto out;
	}
	if (ioc->start_scan_failed) {
		pr_info("%s port enable: FAILED with (ioc_status=0x%08x)\n",
			ioc->name, ioc->start_scan_failed);
		ioc->is_driver_loading = 0;
		ioc->wait_for_discovery_to_complete = 0;
		ioc->remove_host = 1;
		goto out;
	}
	pr_info("%s port enable: SUCCESS\n", ioc->name);
	ioc->port_enable_cmds.status = LEAPIORAID_CMD_NOT_USED;
	leapioraid_scsihost_complete_devices_scanning(ioc);
out:
	if (issue_hard_reset) {
		ioc->port_enable_cmds.status = LEAPIORAID_CMD_NOT_USED;
		if (leapioraid_base_hard_reset_handler(ioc, SOFT_RESET))
			ioc->is_driver_loading = 0;
	}
	return 1;
}

SCSIH_MAP_QUEUE(struct Scsi_Host *shost)
{
	struct LEAPIORAID_ADAPTER *ioc =
	    (struct LEAPIORAID_ADAPTER *)shost->hostdata;
	struct blk_mq_queue_map *map;
	int i, qoff, offset;
	int nr_msix_vectors = ioc->iopoll_q_start_index;
	int iopoll_q_count = ioc->reply_queue_count - nr_msix_vectors;

	if (shost->nr_hw_queues == 1)
		return;
	for (i = 0, qoff = 0; i < shost->nr_maps; i++) {
		map = &shost->tag_set.map[i];
		map->nr_queues = 0;
		offset = 0;
		if (i == HCTX_TYPE_DEFAULT) {
			map->nr_queues =
			    nr_msix_vectors - ioc->high_iops_queues;
			offset = ioc->high_iops_queues;
		} else if (i == HCTX_TYPE_POLL)
			map->nr_queues = iopoll_q_count;
		if (!map->nr_queues)
			BUG_ON(i == HCTX_TYPE_DEFAULT);
		map->queue_offset = qoff;
		if (i != HCTX_TYPE_POLL)
			blk_mq_pci_map_queues(map, ioc->pdev, offset);
		else
			blk_mq_map_queues(map);
		qoff += map->nr_queues;
	}
}

static struct scsi_host_template leapioraid_driver_template = {
	.module = THIS_MODULE,
	.name = "LEAPIO RAID Host",
	.proc_name = LEAPIORAID_DRIVER_NAME,
	.queuecommand = leapioraid_scsihost_qcmd,
	.target_alloc = leapioraid_scsihost_target_alloc,
	.slave_alloc = leapioraid_scsihost_slave_alloc,
	.slave_configure = leapioraid_scsihost_slave_configure,
	.target_destroy = leapioraid_scsihost_target_destroy,
	.slave_destroy = leapioraid_scsihost_slave_destroy,
	.scan_finished = leapioraid_scsihost_scan_finished,
	.scan_start = leapioraid_scsihost_scan_start,
	.change_queue_depth = leapioraid_scsihost_change_queue_depth,
	.eh_abort_handler = leapioraid_scsihost_abort,
	.eh_device_reset_handler = leapioraid_scsihost_dev_reset,
	.eh_target_reset_handler = leapioraid_scsihost_target_reset,
	.eh_host_reset_handler = leapioraid_scsihost_host_reset,
	.bios_param = leapioraid_scsihost_bios_param,
	.can_queue = 1,
	.this_id = -1,
	.sg_tablesize = LEAPIORAID_SG_DEPTH,
	.max_sectors = 128,
	.max_segment_size = 0xffffffff,
	.cmd_per_lun = 128,
	.shost_groups = leapioraid_host_groups,
	.sdev_groups = leapioraid_dev_groups,
	.track_queue_depth = 1,
	.cmd_size = sizeof(struct leapioraid_scsiio_tracker),
	.map_queues = leapioraid_scsihost_map_queues,
	.mq_poll = leapioraid_blk_mq_poll,
};

static struct raid_function_template leapioraid_raid_functions = {
	.cookie = &leapioraid_driver_template,
	.is_raid = leapioraid_scsihost_is_raid,
	.get_resync = leapioraid_scsihost_get_resync,
	.get_state = leapioraid_scsihost_get_state,
};

static int
leapioraid_scsihost_probe(
	struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct LEAPIORAID_ADAPTER *ioc;
	struct Scsi_Host *shost = NULL;
	int rv;

	shost = scsi_host_alloc(&leapioraid_driver_template,
				sizeof(struct LEAPIORAID_ADAPTER));
	if (!shost)
		return -ENODEV;
	ioc = shost_priv(shost);
	memset(ioc, 0, sizeof(struct LEAPIORAID_ADAPTER));
	ioc->id = leapioraid_ids++;
	sprintf(ioc->driver_name, "%s", LEAPIORAID_DRIVER_NAME);

	ioc->combined_reply_queue = 1;
	ioc->nc_reply_index_count = 16;
	ioc->multipath_on_hba = 1;

	ioc = leapioraid_shost_private(shost);
	INIT_LIST_HEAD(&ioc->list);
	spin_lock(&leapioraid_gioc_lock);
	list_add_tail(&ioc->list, &leapioraid_ioc_list);
	spin_unlock(&leapioraid_gioc_lock);
	ioc->shost = shost;
	ioc->pdev = pdev;

	ioc->scsi_io_cb_idx = scsi_io_cb_idx;
	ioc->tm_cb_idx = tm_cb_idx;
	ioc->ctl_cb_idx = ctl_cb_idx;
	ioc->ctl_tm_cb_idx = ctl_tm_cb_idx;
	ioc->base_cb_idx = base_cb_idx;
	ioc->port_enable_cb_idx = port_enable_cb_idx;
	ioc->transport_cb_idx = transport_cb_idx;
	ioc->scsih_cb_idx = scsih_cb_idx;
	ioc->config_cb_idx = config_cb_idx;
	ioc->tm_tr_cb_idx = tm_tr_cb_idx;
	ioc->tm_tr_volume_cb_idx = tm_tr_volume_cb_idx;
	ioc->tm_tr_internal_cb_idx = tm_tr_internal_cb_idx;
	ioc->tm_sas_control_cb_idx = tm_sas_control_cb_idx;

	ioc->logging_level = logging_level;
	ioc->schedule_dead_ioc_flush_running_cmds =
	    &leapioraid_scsihost_flush_running_cmds;
	ioc->open_pcie_trace = open_pcie_trace;
	ioc->enable_sdev_max_qd = 0;
	ioc->max_shutdown_latency = 6;
	ioc->drv_support_bitmap |= 0x00000001;
	ioc->drv_support_bitmap |= 0x00000002;

	mutex_init(&ioc->reset_in_progress_mutex);
	mutex_init(&ioc->hostdiag_unlock_mutex);
	mutex_init(&ioc->pci_access_mutex);
	spin_lock_init(&ioc->ioc_reset_in_progress_lock);
	spin_lock_init(&ioc->scsi_lookup_lock);
	spin_lock_init(&ioc->sas_device_lock);
	spin_lock_init(&ioc->sas_node_lock);
	spin_lock_init(&ioc->fw_event_lock);
	spin_lock_init(&ioc->raid_device_lock);
	spin_lock_init(&ioc->scsih_q_internal_lock);
	spin_lock_init(&ioc->hba_hot_unplug_lock);
	INIT_LIST_HEAD(&ioc->sas_device_list);
	INIT_LIST_HEAD(&ioc->port_table_list);
	INIT_LIST_HEAD(&ioc->sas_device_init_list);
	INIT_LIST_HEAD(&ioc->sas_expander_list);
	INIT_LIST_HEAD(&ioc->enclosure_list);
	INIT_LIST_HEAD(&ioc->fw_event_list);
	INIT_LIST_HEAD(&ioc->raid_device_list);
	INIT_LIST_HEAD(&ioc->sas_hba.sas_port_list);
	INIT_LIST_HEAD(&ioc->delayed_tr_list);
	INIT_LIST_HEAD(&ioc->delayed_sc_list);
	INIT_LIST_HEAD(&ioc->delayed_event_ack_list);
	INIT_LIST_HEAD(&ioc->delayed_tr_volume_list);
	INIT_LIST_HEAD(&ioc->delayed_internal_tm_list);
	INIT_LIST_HEAD(&ioc->scsih_q_intenal_cmds);
	INIT_LIST_HEAD(&ioc->reply_queue_list);
	sprintf(ioc->name, "%s_cm%d", ioc->driver_name, ioc->id);

	shost->max_cmd_len = 32;
	shost->max_lun = 8;
	shost->transportt = leapioraid_transport_template;
	shost->unique_id = ioc->id;

	ioc->drv_internal_flags |= LEAPIORAID_DRV_INTERNAL_BITMAP_BLK_MQ;

	ioc->disable_eedp_support = 1;
	snprintf(ioc->firmware_event_name, sizeof(ioc->firmware_event_name),
		 "fw_event_%s%u", ioc->driver_name, ioc->id);
	ioc->firmware_event_thread =
	    alloc_ordered_workqueue(ioc->firmware_event_name, 0);
	if (!ioc->firmware_event_thread) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rv = -ENODEV;
		goto out_thread_fail;
	}

	shost->host_tagset = 0;
	ioc->is_driver_loading = 1;
	if ((leapioraid_base_attach(ioc))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rv = -ENODEV;
		goto out_attach_fail;
	}
	ioc->hide_drives = 0;

	shost->nr_hw_queues = 1;
	rv = scsi_add_host(shost, &pdev->dev);
	if (rv) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		spin_lock(&leapioraid_gioc_lock);
		list_del(&ioc->list);
		spin_unlock(&leapioraid_gioc_lock);
		goto out_add_shost_fail;
	}

	scsi_scan_host(shost);

	return 0;
out_add_shost_fail:
	leapioraid_base_detach(ioc);
out_attach_fail:
	destroy_workqueue(ioc->firmware_event_thread);
out_thread_fail:
	spin_lock(&leapioraid_gioc_lock);
	list_del(&ioc->list);
	spin_unlock(&leapioraid_gioc_lock);
	scsi_host_put(shost);
	return rv;
}

#ifdef CONFIG_PM
static int
leapioraid_scsihost_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct Scsi_Host *shost = NULL;
	struct LEAPIORAID_ADAPTER *ioc = NULL;
	pci_power_t device_state;
	int rc;

	rc = leapioraid_scsihost_get_shost_and_ioc(pdev, &shost, &ioc);
	if (rc) {
		dev_err(&pdev->dev, "unable to suspend device\n");
		return rc;
	}
	leapioraid_base_stop_watchdog(ioc);
	leapioraid_base_stop_hba_unplug_watchdog(ioc);
	scsi_block_requests(shost);
	device_state = pci_choose_state(pdev, state);
	leapioraid_scsihost_ir_shutdown(ioc);
	pr_info("%s pdev=0x%p, slot=%s, entering operating state [D%d]\n",
		ioc->name, pdev,
	    pci_name(pdev), device_state);
	pci_save_state(pdev);
	leapioraid_base_free_resources(ioc);
	pci_set_power_state(pdev, device_state);
	return 0;
}

static int
leapioraid_scsihost_resume(struct pci_dev *pdev)
{
	struct Scsi_Host *shost = NULL;
	struct LEAPIORAID_ADAPTER *ioc = NULL;
	pci_power_t device_state = pdev->current_state;
	int r;

	r = leapioraid_scsihost_get_shost_and_ioc(pdev, &shost, &ioc);
	if (r) {
		dev_err(&pdev->dev, "unable to resume device\n");
		return r;
	}
	pr_info("%s pdev=0x%p, slot=%s, previous operating state [D%d]\n",
		ioc->name, pdev,
	    pci_name(pdev), device_state);
	pci_set_power_state(pdev, PCI_D0);
	pci_enable_wake(pdev, PCI_D0, 0);
	pci_restore_state(pdev);
	ioc->pdev = pdev;
	r = leapioraid_base_map_resources(ioc);
	if (r)
		return r;
	pr_err("%s issuing hard reset as part of OS resume\n",
		ioc->name);
	leapioraid_base_hard_reset_handler(ioc, SOFT_RESET);
	scsi_unblock_requests(shost);
	leapioraid_base_start_watchdog(ioc);
	leapioraid_base_start_hba_unplug_watchdog(ioc);
	return 0;
}
#endif

static pci_ers_result_t
leapioraid_scsihost_pci_error_detected(
	struct pci_dev *pdev, pci_channel_state_t state)
{
	struct Scsi_Host *shost = NULL;
	struct LEAPIORAID_ADAPTER *ioc = NULL;

	if (leapioraid_scsihost_get_shost_and_ioc(pdev, &shost, &ioc)) {
		dev_err(&pdev->dev, "device unavailable\n");
		return PCI_ERS_RESULT_DISCONNECT;
	}
	pr_err("%s PCI error: detected callback, state(%d)!!\n",
		ioc->name, state);
	switch (state) {
	case pci_channel_io_normal:
		return PCI_ERS_RESULT_CAN_RECOVER;
	case pci_channel_io_frozen:
		ioc->pci_error_recovery = 1;
		scsi_block_requests(ioc->shost);
		leapioraid_base_stop_watchdog(ioc);
		leapioraid_base_stop_hba_unplug_watchdog(ioc);
		leapioraid_base_free_resources(ioc);
		return PCI_ERS_RESULT_NEED_RESET;
	case pci_channel_io_perm_failure:
		ioc->pci_error_recovery = 1;
		leapioraid_base_stop_watchdog(ioc);
		leapioraid_base_stop_hba_unplug_watchdog(ioc);
		leapioraid_base_pause_mq_polling(ioc);
		leapioraid_scsihost_flush_running_cmds(ioc);
		return PCI_ERS_RESULT_DISCONNECT;
	}
	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t
leapioraid_scsihost_pci_slot_reset(struct pci_dev *pdev)
{
	struct Scsi_Host *shost = NULL;
	struct LEAPIORAID_ADAPTER *ioc = NULL;
	int rc;

	if (leapioraid_scsihost_get_shost_and_ioc(pdev, &shost, &ioc)) {
		dev_err(&pdev->dev, "unable to perform slot reset\n");
		return PCI_ERS_RESULT_DISCONNECT;
	}
	pr_err("%s PCI error: slot reset callback!!\n",
	       ioc->name);
	ioc->pci_error_recovery = 0;
	ioc->pdev = pdev;
	pci_restore_state(pdev);
	rc = leapioraid_base_map_resources(ioc);
	if (rc)
		return PCI_ERS_RESULT_DISCONNECT;
	pr_info("%s issuing hard reset as part of PCI slot reset\n",
		ioc->name);
	rc = leapioraid_base_hard_reset_handler(ioc, FORCE_BIG_HAMMER);
	pr_info("%s hard reset: %s\n",
		ioc->name, (rc == 0) ? "success" : "failed");
	if (!rc)
		return PCI_ERS_RESULT_RECOVERED;
	else
		return PCI_ERS_RESULT_DISCONNECT;
}

static void
leapioraid_scsihost_pci_resume(struct pci_dev *pdev)
{
	struct Scsi_Host *shost = NULL;
	struct LEAPIORAID_ADAPTER *ioc = NULL;

	if (leapioraid_scsihost_get_shost_and_ioc(pdev, &shost, &ioc)) {
		dev_err(&pdev->dev, "unable to resume device\n");
		return;
	}
	pr_err("%s PCI error: resume callback!!\n",
	       ioc->name);

	pci_aer_clear_nonfatal_status(pdev);

	leapioraid_base_start_watchdog(ioc);
	leapioraid_base_start_hba_unplug_watchdog(ioc);
	scsi_unblock_requests(ioc->shost);
}

static pci_ers_result_t
leapioraid_scsihost_pci_mmio_enabled(struct pci_dev *pdev)
{
	struct Scsi_Host *shost = NULL;
	struct LEAPIORAID_ADAPTER *ioc = NULL;

	if (leapioraid_scsihost_get_shost_and_ioc(pdev, &shost, &ioc)) {
		dev_err(&pdev->dev, "unable to enable mmio\n");
		return PCI_ERS_RESULT_DISCONNECT;
	}

	pr_err("%s: PCI error: mmio enabled callback!!!\n",
	       ioc->name);
	return PCI_ERS_RESULT_RECOVERED;
}

u8 leapioraid_scsihost_ncq_prio_supp(struct scsi_device *sdev)
{
	u8 ncq_prio_supp = 0;

	struct scsi_vpd *vpd;

	rcu_read_lock();
	vpd = rcu_dereference(sdev->vpd_pg89);
	if (!vpd || vpd->len < 214)
		goto out;
	ncq_prio_supp = (vpd->data[213] >> 4) & 1;
out:
	rcu_read_unlock();
	return ncq_prio_supp;
}

static const struct pci_device_id leapioraid_pci_table[] = {
	{ 0x1556, 0x1111, PCI_ANY_ID, PCI_ANY_ID },
	{ LEAPIORAID_VENDOR_ID, LEAPIORAID_DEVICE_ID_1, PCI_ANY_ID, PCI_ANY_ID },
	{ LEAPIORAID_VENDOR_ID, LEAPIORAID_DEVICE_ID_2, PCI_ANY_ID, PCI_ANY_ID },
	{ LEAPIORAID_VENDOR_ID, LEAPIORAID_HBA, PCI_ANY_ID, PCI_ANY_ID },
	{ LEAPIORAID_VENDOR_ID, LEAPIORAID_RAID, PCI_ANY_ID, PCI_ANY_ID },
	{ 0 }
};

MODULE_DEVICE_TABLE(pci, leapioraid_pci_table);
static struct pci_error_handlers leapioraid_err_handler = {
	.error_detected = leapioraid_scsihost_pci_error_detected,
	.mmio_enabled = leapioraid_scsihost_pci_mmio_enabled,
	.slot_reset = leapioraid_scsihost_pci_slot_reset,
	.resume = leapioraid_scsihost_pci_resume,
};

static struct pci_driver leapioraid_driver = {
	.name = LEAPIORAID_DRIVER_NAME,
	.id_table = leapioraid_pci_table,
	.probe = leapioraid_scsihost_probe,
	.remove = leapioraid_scsihost_remove,
	.shutdown = leapioraid_scsihost_shutdown,
	.err_handler = &leapioraid_err_handler,
#ifdef CONFIG_PM
	.suspend = leapioraid_scsihost_suspend,
	.resume = leapioraid_scsihost_resume,
#endif
};

static int
leapioraid_scsihost_init(void)
{
	leapioraid_ids = 0;
	leapioraid_base_initialize_callback_handler();

	scsi_io_cb_idx =
	    leapioraid_base_register_callback_handler(
			leapioraid_scsihost_io_done);
	tm_cb_idx =
	    leapioraid_base_register_callback_handler(
			leapioraid_scsihost_tm_done);
	base_cb_idx =
	    leapioraid_base_register_callback_handler(
			leapioraid_base_done);
	port_enable_cb_idx =
	    leapioraid_base_register_callback_handler(
			leapioraid_port_enable_done);
	transport_cb_idx =
	    leapioraid_base_register_callback_handler(
			leapioraid_transport_done);
	scsih_cb_idx =
	    leapioraid_base_register_callback_handler(
			leapioraid_scsihost_done);
	config_cb_idx =
	    leapioraid_base_register_callback_handler(
			leapioraid_config_done);
	ctl_cb_idx =
	    leapioraid_base_register_callback_handler(
			leapioraid_ctl_done);
	ctl_tm_cb_idx =
	    leapioraid_base_register_callback_handler(
			leapioraid_ctl_tm_done);
	tm_tr_cb_idx =
	    leapioraid_base_register_callback_handler(
			leapioraid_scsihost_tm_tr_complete);
	tm_tr_volume_cb_idx =
	    leapioraid_base_register_callback_handler(
			leapioraid_scsihost_tm_volume_tr_complete);
	tm_tr_internal_cb_idx =
	    leapioraid_base_register_callback_handler(
			leapioraid_scsihost_tm_internal_tr_complete);
	tm_sas_control_cb_idx =
	    leapioraid_base_register_callback_handler(
			leapioraid_scsihost_sas_control_complete);

	return 0;
}

static void
leapioraid_scsihost_exit(void)
{
	leapioraid_base_release_callback_handler(scsi_io_cb_idx);
	leapioraid_base_release_callback_handler(tm_cb_idx);
	leapioraid_base_release_callback_handler(base_cb_idx);
	leapioraid_base_release_callback_handler(port_enable_cb_idx);
	leapioraid_base_release_callback_handler(transport_cb_idx);
	leapioraid_base_release_callback_handler(scsih_cb_idx);
	leapioraid_base_release_callback_handler(config_cb_idx);
	leapioraid_base_release_callback_handler(ctl_cb_idx);
	leapioraid_base_release_callback_handler(ctl_tm_cb_idx);
	leapioraid_base_release_callback_handler(tm_tr_cb_idx);
	leapioraid_base_release_callback_handler(tm_tr_volume_cb_idx);
	leapioraid_base_release_callback_handler(tm_tr_internal_cb_idx);
	leapioraid_base_release_callback_handler(tm_sas_control_cb_idx);

	raid_class_release(leapioraid_raid_template);
	sas_release_transport(leapioraid_transport_template);
}

static int __init leapioraid_init(void)
{
	int error;

	pr_info("%s version %s loaded\n", LEAPIORAID_DRIVER_NAME,
		LEAPIORAID_DRIVER_VERSION);
	leapioraid_transport_template =
	    sas_attach_transport(&leapioraid_transport_functions);

	if (!leapioraid_transport_template)
		return -ENODEV;

	leapioraid_raid_template =
	    raid_class_attach(&leapioraid_raid_functions);
	if (!leapioraid_raid_template) {
		sas_release_transport(leapioraid_transport_template);
		return -ENODEV;
	}

	error = leapioraid_scsihost_init();
	if (error) {
		leapioraid_scsihost_exit();
		return error;
	}
	leapioraid_ctl_init();
	error = pci_register_driver(&leapioraid_driver);
	if (error)
		leapioraid_scsihost_exit();
	return error;
}

static void __exit leapioraid_exit(void)
{
	pr_info("leapioraid_ids version %s unloading\n",
		LEAPIORAID_DRIVER_VERSION);
	leapioraid_ctl_exit();
	pci_unregister_driver(&leapioraid_driver);
	leapioraid_scsihost_exit();
}

module_init(leapioraid_init);
module_exit(leapioraid_exit);
