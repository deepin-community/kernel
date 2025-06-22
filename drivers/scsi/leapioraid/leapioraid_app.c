// SPDX-License-Identifier: GPL-2.0
/*
 * Management Module Support for MPT (Message Passing Technology) based
 * controllers
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/compat.h>
#include <linux/poll.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#ifdef __KERNEL__
#include <linux/miscdevice.h>
#endif
#include "leapioraid_func.h"

#define LEAPIORAID_DEV_NAME	"leapioraid_ctl"

#define LEAPIORAID_MAGIC_NUMBER	'L'
#define LEAPIORAID_IOCTL_DEFAULT_TIMEOUT (10)

#define LEAPIORAID_IOCINFO	\
	_IOWR(LEAPIORAID_MAGIC_NUMBER, 17, struct leapio_ioctl_iocinfo)
#define LEAPIORAID_COMMAND	\
	_IOWR(LEAPIORAID_MAGIC_NUMBER, 20, struct leapio_ioctl_command)
#ifdef CONFIG_COMPAT
#define LEAPIORAID_COMMAND32 \
	_IOWR(LEAPIORAID_MAGIC_NUMBER, 20, struct leapio_ioctl_command32)
#endif
#define LEAPIORAID_EVENTQUERY \
	_IOWR(LEAPIORAID_MAGIC_NUMBER, 21, struct leapio_ioctl_eventquery)
#define LEAPIORAID_EVENTENABLE	\
	_IOWR(LEAPIORAID_MAGIC_NUMBER, 22, struct leapio_ioctl_eventenable)
#define LEAPIORAID_EVENTREPORT	\
	_IOWR(LEAPIORAID_MAGIC_NUMBER, 23, struct leapio_ioctl_eventreport)
#define LEAPIORAID_HARDRESET	\
	_IOWR(LEAPIORAID_MAGIC_NUMBER, 24, struct leapio_ioctl_diag_reset)
#define LEAPIORAID_BTDHMAPPING	\
	_IOWR(LEAPIORAID_MAGIC_NUMBER, 31, struct leapio_ioctl_btdh_mapping)

struct leapio_ioctl_header {
	uint32_t ioc_number;
	uint32_t port_number;
	uint32_t max_data_size;
};

struct leapio_ioctl_diag_reset {
	struct leapio_ioctl_header hdr;
};

struct leapio_ioctl_pci_info {
	union {
		struct {
			uint32_t device:5;
			uint32_t function:3;
			uint32_t bus:24;
		} bits;
		uint32_t word;
	} u;
	uint32_t segment_id;
};

struct leapio_ioctl_iocinfo {
	struct leapio_ioctl_header hdr;
	uint32_t adapter_type;
	uint32_t port_number;
	uint32_t pci_id;
	uint32_t hw_rev;
	uint32_t subsystem_device;
	uint32_t subsystem_vendor;
	uint32_t rsvd0;
	uint32_t firmware_version;
	uint32_t bios_version;
	uint8_t driver_version[32];
	uint8_t rsvd1;
	uint8_t scsi_id;
	uint16_t rsvd2;
	struct leapio_ioctl_pci_info pci_information;
};

#define LEAPIORAID_CTL_EVENT_LOG_SIZE (200)
struct leapio_ioctl_eventquery {
	struct leapio_ioctl_header hdr;
	uint16_t event_entries;
	uint16_t rsvd;
	uint32_t event_types[LEAPIORAID_EVENT_NOTIFY_EVENTMASK_WORDS];
};

struct leapio_ioctl_eventenable {
	struct leapio_ioctl_header hdr;
	uint32_t event_types[4];
};

#define LEAPIORAID_EVENT_DATA_SIZE (192)
struct LEAPIORAID_IOCTL_EVENTS {
	uint32_t event;
	uint32_t context;
	uint8_t data[LEAPIORAID_EVENT_DATA_SIZE];
};

struct leapio_ioctl_eventreport {
	struct leapio_ioctl_header hdr;
	struct LEAPIORAID_IOCTL_EVENTS event_data[];
};

struct leapio_ioctl_command {
	struct leapio_ioctl_header hdr;
	uint32_t timeout;
	void __user *reply_frame_buf_ptr;
	void __user *data_in_buf_ptr;
	void __user *data_out_buf_ptr;
	void __user *sense_data_ptr;
	uint32_t max_reply_bytes;
	uint32_t data_in_size;
	uint32_t data_out_size;
	uint32_t max_sense_bytes;
	uint32_t data_sge_offset;
	uint8_t mf[];
};

#ifdef CONFIG_COMPAT
struct leapio_ioctl_command32 {
	struct leapio_ioctl_header hdr;
	uint32_t timeout;
	uint32_t reply_frame_buf_ptr;
	uint32_t data_in_buf_ptr;
	uint32_t data_out_buf_ptr;
	uint32_t sense_data_ptr;
	uint32_t max_reply_bytes;
	uint32_t data_in_size;
	uint32_t data_out_size;
	uint32_t max_sense_bytes;
	uint32_t data_sge_offset;
	uint8_t mf[];
};
#endif

struct leapio_ioctl_btdh_mapping {
	struct leapio_ioctl_header hdr;
	uint32_t id;
	uint32_t bus;
	uint16_t handle;
	uint16_t rsvd;
};

static struct fasync_struct *leapioraid_async_queue;
static DECLARE_WAIT_QUEUE_HEAD(leapioraid_ctl_poll_wait);

enum leapioraid_block_state {
	NON_BLOCKING,
	BLOCKING,
};

static void
leapioraid_ctl_display_some_debug(
		struct LEAPIORAID_ADAPTER *ioc, u16 smid,
		char *calling_function_name,
		struct LeapioraidDefaultRep_t *mpi_reply)
{
	struct LeapioraidCfgReq_t *mpi_request;
	char *desc = NULL;

	if (!(ioc->logging_level & LEAPIORAID_DEBUG_IOCTL))
		return;
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	switch (mpi_request->Function) {
	case LEAPIORAID_FUNC_SCSI_IO_REQUEST:
		{
			struct LeapioSCSIIOReq_t *scsi_request =
			    (struct LeapioSCSIIOReq_t *) mpi_request;
			snprintf(ioc->tmp_string, LEAPIORAID_STRING_LENGTH,
				 "scsi_io, cmd(0x%02x), cdb_len(%d)",
				 scsi_request->CDB.CDB32[0],
				 le16_to_cpu(scsi_request->IoFlags) & 0xF);
			desc = ioc->tmp_string;
			break;
		}
	case LEAPIORAID_FUNC_SCSI_TASK_MGMT:
		desc = "task_mgmt";
		break;
	case LEAPIORAID_FUNC_IOC_INIT:
		desc = "ioc_init";
		break;
	case LEAPIORAID_FUNC_IOC_FACTS:
		desc = "ioc_facts";
		break;
	case LEAPIORAID_FUNC_CONFIG:
		{
			struct LeapioraidCfgReq_t *config_request =
			    (struct LeapioraidCfgReq_t *) mpi_request;
			snprintf(ioc->tmp_string, LEAPIORAID_STRING_LENGTH,
				 "config, type(0x%02x), ext_type(0x%02x), number(%d)",
				 (config_request->Header.PageType &
				  LEAPIORAID_CONFIG_PAGETYPE_MASK),
				 config_request->ExtPageType,
				 config_request->Header.PageNumber);
			desc = ioc->tmp_string;
			break;
		}
	case LEAPIORAID_FUNC_PORT_FACTS:
		desc = "port_facts";
		break;
	case LEAPIORAID_FUNC_PORT_ENABLE:
		desc = "port_enable";
		break;
	case LEAPIORAID_FUNC_EVENT_NOTIFICATION:
		desc = "event_notification";
		break;
	case LEAPIORAID_FUNC_FW_DOWNLOAD:
		desc = "fw_download";
		break;
	case LEAPIORAID_FUNC_FW_UPLOAD:
		desc = "fw_upload";
		break;
	case LEAPIORAID_FUNC_RAID_ACTION:
		desc = "raid_action";
		break;
	case LEAPIORAID_FUNC_RAID_SCSI_IO_PASSTHROUGH:
		{
			struct LeapioSCSIIOReq_t *scsi_request =
			    (struct LeapioSCSIIOReq_t *) mpi_request;
			snprintf(ioc->tmp_string, LEAPIORAID_STRING_LENGTH,
				 "raid_pass, cmd(0x%02x), cdb_len(%d)",
				 scsi_request->CDB.CDB32[0],
				 le16_to_cpu(scsi_request->IoFlags) & 0xF);
			desc = ioc->tmp_string;
			break;
		}
	case LEAPIORAID_FUNC_SAS_IO_UNIT_CONTROL:
		desc = "sas_iounit_cntl";
		break;
	case LEAPIORAID_FUNC_SATA_PASSTHROUGH:
		desc = "sata_pass";
		break;
	case LEAPIORAID_FUNC_SMP_PASSTHROUGH:
		desc = "smp_passthrough";
		break;
	}
	if (!desc)
		return;
	pr_info("%s %s: %s, smid(%d)\n",
	       ioc->name, calling_function_name, desc, smid);
	if (!mpi_reply)
		return;
	if (mpi_reply->IOCStatus || mpi_reply->IOCLogInfo)
		pr_info(
		       "%s \tiocstatus(0x%04x), loginfo(0x%08x)\n",
		       ioc->name, le16_to_cpu(mpi_reply->IOCStatus),
		       le32_to_cpu(mpi_reply->IOCLogInfo));
	if (mpi_request->Function == LEAPIORAID_FUNC_SCSI_IO_REQUEST ||
	    mpi_request->Function ==
	    LEAPIORAID_FUNC_RAID_SCSI_IO_PASSTHROUGH) {
		struct LeapioraidSCSIIORep_t *scsi_reply =
		    (struct LeapioraidSCSIIORep_t *) mpi_reply;
		struct leapioraid_sas_device *sas_device = NULL;

		sas_device = leapioraid_get_sdev_by_handle(ioc,
							   le16_to_cpu(scsi_reply->DevHandle));
		if (sas_device) {
			pr_info("%s \tsas_address(0x%016llx), phy(%d)\n",
			       ioc->name, (unsigned long long)
			       sas_device->sas_address, sas_device->phy);
			if (sas_device->enclosure_handle != 0)
				pr_info(
				       "%s \tenclosure_logical_id(0x%016llx), slot(%d)\n",
				       ioc->name, (unsigned long long)
				       sas_device->enclosure_logical_id,
				       sas_device->slot);
			leapioraid_sas_device_put(sas_device);
		}
		if (scsi_reply->SCSIState || scsi_reply->SCSIStatus)
			pr_info(
			       "%s \tscsi_state(0x%02x), scsi_status (0x%02x)\n",
				   ioc->name, scsi_reply->SCSIState, scsi_reply->SCSIStatus);
	}
}

u8
leapioraid_ctl_done(struct LEAPIORAID_ADAPTER *ioc, u16 smid, u8 msix_index,
		    u32 reply)
{
	struct LeapioraidDefaultRep_t *mpi_reply;
	struct LeapioraidSCSIIORep_t *scsiio_reply;
	const void *sense_data;
	u32 sz;

	if (ioc->ctl_cmds.status == LEAPIORAID_CMD_NOT_USED)
		return 1;
	if (ioc->ctl_cmds.smid != smid)
		return 1;
	ioc->ctl_cmds.status |= LEAPIORAID_CMD_COMPLETE;
	mpi_reply = leapioraid_base_get_reply_virt_addr(ioc, reply);
	if (mpi_reply) {
		memcpy(ioc->ctl_cmds.reply, mpi_reply,
		       mpi_reply->MsgLength * 4);
		ioc->ctl_cmds.status |= LEAPIORAID_CMD_REPLY_VALID;
		if (mpi_reply->Function == LEAPIORAID_FUNC_SCSI_IO_REQUEST ||
		    mpi_reply->Function ==
		    LEAPIORAID_FUNC_RAID_SCSI_IO_PASSTHROUGH) {
			scsiio_reply = (struct LeapioraidSCSIIORep_t *) mpi_reply;
			if (scsiio_reply->SCSIState &
			    LEAPIORAID_SCSI_STATE_AUTOSENSE_VALID) {
				sz = min_t(u32, SCSI_SENSE_BUFFERSIZE,
					   le32_to_cpu(scsiio_reply->SenseCount));
				sense_data =
				    leapioraid_base_get_sense_buffer(ioc, smid);
				memcpy(ioc->ctl_cmds.sense, sense_data, sz);
			}
		}
	}
	leapioraid_ctl_display_some_debug(ioc, smid, "ctl_done", mpi_reply);
	ioc->ctl_cmds.status &= ~LEAPIORAID_CMD_PENDING;
	complete(&ioc->ctl_cmds.done);
	return 1;
}

static int leapioraid_ctl_check_event_type(
	struct LEAPIORAID_ADAPTER *ioc, u16 event)
{
	u16 i;
	u32 desired_event;

	if (event >= 128 || !event || !ioc->event_log)
		return 0;
	desired_event = (1 << (event % 32));
	if (!desired_event)
		desired_event = 1;
	i = event / 32;
	return desired_event & ioc->event_type[i];
}

void
leapioraid_ctl_add_to_event_log(
	struct LEAPIORAID_ADAPTER *ioc,
	struct LeapioraidEventNotificationRep_t *mpi_reply)
{
	struct LEAPIORAID_IOCTL_EVENTS *event_log;
	u16 event;
	int i;
	u32 sz, event_data_sz;
	u8 send_aen = 0;

	if (!ioc->event_log)
		return;
	event = le16_to_cpu(mpi_reply->Event);
	if (leapioraid_ctl_check_event_type(ioc, event)) {
		i = ioc->event_context % LEAPIORAID_CTL_EVENT_LOG_SIZE;
		event_log = ioc->event_log;
		event_log[i].event = event;
		event_log[i].context = ioc->event_context++;
		event_data_sz = le16_to_cpu(mpi_reply->EventDataLength) * 4;
		sz = min_t(u32, event_data_sz, LEAPIORAID_EVENT_DATA_SIZE);
		memset(event_log[i].data, 0, LEAPIORAID_EVENT_DATA_SIZE);
		memcpy(event_log[i].data, mpi_reply->EventData, sz);
		send_aen = 1;
	}
	if (event == LEAPIORAID_EVENT_LOG_ENTRY_ADDED ||
	    (send_aen && !ioc->aen_event_read_flag)) {
		ioc->aen_event_read_flag = 1;
		wake_up_interruptible(&leapioraid_ctl_poll_wait);
		if (leapioraid_async_queue)
			kill_fasync(&leapioraid_async_queue, SIGIO, POLL_IN);
	}
}

u8
leapioraid_ctl_event_callback(
	struct LEAPIORAID_ADAPTER *ioc, u8 msix_index,
	u32 reply)
{
	struct LeapioraidEventNotificationRep_t *mpi_reply;

	mpi_reply = leapioraid_base_get_reply_virt_addr(ioc, reply);
	if (mpi_reply)
		leapioraid_ctl_add_to_event_log(ioc, mpi_reply);
	return 1;
}

static int
leapioraid_ctl_verify_adapter(
	int ioc_number, struct LEAPIORAID_ADAPTER **iocpp)
{
	struct LEAPIORAID_ADAPTER *ioc;

	spin_lock(&leapioraid_gioc_lock);
	list_for_each_entry(ioc, &leapioraid_ioc_list, list) {
		if (ioc->id != ioc_number)
			continue;
		spin_unlock(&leapioraid_gioc_lock);
		*iocpp = ioc;
		return ioc_number;
	}
	spin_unlock(&leapioraid_gioc_lock);
	*iocpp = NULL;
	return -1;
}

void
leapioraid_ctl_clear_outstanding_ioctls(struct LEAPIORAID_ADAPTER *ioc)
{
	if (ioc->ctl_cmds.status & LEAPIORAID_CMD_PENDING) {
		ioc->ctl_cmds.status |= LEAPIORAID_CMD_RESET;
		leapioraid_base_free_smid(ioc, ioc->ctl_cmds.smid);
		complete(&ioc->ctl_cmds.done);
	}
}

void
leapioraid_ctl_reset_handler(struct LEAPIORAID_ADAPTER *ioc, int reset_phase)
{
	switch (reset_phase) {
	case LEAPIORAID_IOC_PRE_RESET_PHASE:
		dtmprintk(ioc, pr_info(
			"%s %s: LEAPIORAID_IOC_PRE_RESET_PHASE\n", ioc->name,
			__func__));
		break;
	case LEAPIORAID_IOC_AFTER_RESET_PHASE:
		dtmprintk(ioc, pr_info(
			"%s %s: LEAPIORAID_IOC_AFTER_RESET_PHASE\n", ioc->name,
			__func__));
		leapioraid_ctl_clear_outstanding_ioctls(ioc);
		break;
	case LEAPIORAID_IOC_DONE_RESET_PHASE:
		dtmprintk(ioc, pr_info(
			"%s %s: LEAPIORAID_IOC_DONE_RESET_PHASE\n", ioc->name,
			__func__));
		break;
	}
}

static int
leapioraid_ctl_fasync(int fd, struct file *filep, int mode)
{
	return fasync_helper(fd, filep, mode, &leapioraid_async_queue);
}

int
leapioraid_ctl_release(struct inode *inode, struct file *filep)
{
	return fasync_helper(-1, filep, 0, &leapioraid_async_queue);
}

static unsigned int
leapioraid_ctl_poll(struct file *filep, poll_table *wait)
{
	struct LEAPIORAID_ADAPTER *ioc;

	poll_wait(filep, &leapioraid_ctl_poll_wait, wait);
	spin_lock(&leapioraid_gioc_lock);
	list_for_each_entry(ioc, &leapioraid_ioc_list, list) {
		if (ioc->aen_event_read_flag) {
			spin_unlock(&leapioraid_gioc_lock);
			return POLLIN | POLLRDNORM;
		}
	}
	spin_unlock(&leapioraid_gioc_lock);
	return 0;
}

static int
leapioraid_ctl_set_task_mid(struct LEAPIORAID_ADAPTER *ioc,
		  struct leapio_ioctl_command *karg,
		  struct LeapioraidSCSITmgReq_t *tm_request)
{
	u8 found = 0;
	u16 smid;
	u16 handle;
	struct scsi_cmnd *scmd;
	struct LEAPIORAID_DEVICE *priv_data;
	struct LeapioraidSCSITmgRep_t *tm_reply;
	u32 sz;
	u32 lun;
	char *desc = NULL;
	struct leapioraid_scsiio_tracker *st = NULL;

	if (tm_request->TaskType == LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABORT_TASK)
		desc = "abort_task";
	else if (tm_request->TaskType ==
		 LEAPIORAID_SCSITASKMGMT_TASKTYPE_QUERY_TASK)
		desc = "query_task";
	else
		return 0;
	lun = scsilun_to_int((struct scsi_lun *)tm_request->LUN);
	handle = le16_to_cpu(tm_request->DevHandle);
	for (smid = ioc->shost->can_queue; smid && !found; smid--) {
		scmd = leapioraid_scsihost_scsi_lookup_get(ioc, smid);
		if (scmd == NULL || scmd->device == NULL ||
		    scmd->device->hostdata == NULL)
			continue;
		if (lun != scmd->device->lun)
			continue;
		priv_data = scmd->device->hostdata;
		if (priv_data->sas_target == NULL)
			continue;
		if (priv_data->sas_target->handle != handle)
			continue;
		st = leapioraid_base_scsi_cmd_priv(scmd);
		if ((!st) || (st->smid == 0))
			continue;
		if (!tm_request->TaskMID || tm_request->TaskMID == st->smid) {
			tm_request->TaskMID = cpu_to_le16(st->smid);
			found = 1;
		}
	}
	if (!found) {
		dctlprintk(ioc, pr_info(
			"%s %s: handle(0x%04x), lun(%d), no active mid!!\n",
				       ioc->name, desc,
				       le16_to_cpu(tm_request->DevHandle),
				       lun));
		tm_reply = ioc->ctl_cmds.reply;
		tm_reply->DevHandle = tm_request->DevHandle;
		tm_reply->Function = LEAPIORAID_FUNC_SCSI_TASK_MGMT;
		tm_reply->TaskType = tm_request->TaskType;
		tm_reply->MsgLength =
		    sizeof(struct LeapioraidSCSITmgRep_t) / 4;
		tm_reply->VP_ID = tm_request->VP_ID;
		tm_reply->VF_ID = tm_request->VF_ID;
		sz = min_t(u32, karg->max_reply_bytes, ioc->reply_sz);
		if (copy_to_user(karg->reply_frame_buf_ptr, ioc->ctl_cmds.reply,
				 sz))
			pr_err("failure at %s:%d/%s()!\n", __FILE__,
			       __LINE__, __func__);
		return 1;
	}
	dctlprintk(ioc, pr_info(
		"%s %s: handle(0x%04x), lun(%d), task_mid(%d)\n",
		ioc->name, desc,
		le16_to_cpu(tm_request->DevHandle), lun,
		le16_to_cpu(tm_request->TaskMID)));
	return 0;
}

static long
leapioraid_ctl_do_command(struct LEAPIORAID_ADAPTER *ioc,
			struct leapio_ioctl_command karg, void __user *mf)
{
	struct LeapioraidReqHeader_t *mpi_request = NULL, *request;
	struct LeapioraidDefaultRep_t *mpi_reply;
	u16 smid;
	unsigned long timeout;
	u8 issue_reset;
	u32 sz, sz_arg;
	void *psge;
	void *data_out = NULL;
	dma_addr_t data_out_dma = 0;
	size_t data_out_sz = 0;
	void *data_in = NULL;
	dma_addr_t data_in_dma = 0;
	size_t data_in_sz = 0;
	long ret;
	u16 device_handle = LEAPIORAID_INVALID_DEVICE_HANDLE;

	issue_reset = 0;
	if (ioc->ctl_cmds.status != LEAPIORAID_CMD_NOT_USED) {
		pr_err("%s %s: ctl_cmd in use\n",
		       ioc->name, __func__);
		ret = -EAGAIN;
		goto out;
	}
	ret = leapioraid_wait_for_ioc_to_operational(ioc, 10);
	if (ret)
		goto out;
	mpi_request = kzalloc(ioc->request_sz, GFP_KERNEL);
	if (!mpi_request) {
		ret = -ENOMEM;
		goto out;
	}
	if (karg.data_sge_offset * 4 > ioc->request_sz ||
	    karg.data_sge_offset > (UINT_MAX / 4)) {
		ret = -EINVAL;
		goto out;
	}
	if (copy_from_user(mpi_request, mf, karg.data_sge_offset * 4)) {
		pr_err("failure at %s:%d/%s()!\n", __FILE__, __LINE__,
		       __func__);
		ret = -EFAULT;
		goto out;
	}
	if (mpi_request->Function == LEAPIORAID_FUNC_SCSI_TASK_MGMT) {
		smid = leapioraid_base_get_smid_hpr(ioc, ioc->ctl_cb_idx);
		if (!smid) {
			pr_err(
			       "%s %s: failed obtaining a smid\n", ioc->name,
			       __func__);
			ret = -EAGAIN;
			goto out;
		}
	} else {
		smid = ioc->shost->can_queue + LEAPIORAID_INTERNAL_SCSIIO_FOR_IOCTL;
	}
	ret = 0;
	ioc->ctl_cmds.status = LEAPIORAID_CMD_PENDING;
	memset(ioc->ctl_cmds.reply, 0, ioc->reply_sz);
	request = leapioraid_base_get_msg_frame(ioc, smid);
	memset(request, 0, ioc->request_sz);
	memcpy(request, mpi_request, karg.data_sge_offset * 4);
	ioc->ctl_cmds.smid = smid;
	data_out_sz = karg.data_out_size;
	data_in_sz = karg.data_in_size;
	if (mpi_request->Function == LEAPIORAID_FUNC_SCSI_IO_REQUEST ||
	    mpi_request->Function == LEAPIORAID_FUNC_RAID_SCSI_IO_PASSTHROUGH
	    || mpi_request->Function == LEAPIORAID_FUNC_SCSI_TASK_MGMT
	    || mpi_request->Function == LEAPIORAID_FUNC_SATA_PASSTHROUGH) {
		device_handle = le16_to_cpu(mpi_request->FunctionDependent1);
		if (!device_handle || (device_handle > ioc->facts.MaxDevHandle)) {
			ret = -EINVAL;
			leapioraid_base_free_smid(ioc, smid);
			goto out;
		}
	}
	if (data_out_sz) {
		data_out = dma_alloc_coherent(&ioc->pdev->dev, data_out_sz,
					      &data_out_dma, GFP_ATOMIC);
		if (!data_out) {
			ret = -ENOMEM;
			leapioraid_base_free_smid(ioc, smid);
			goto out;
		}
		if (copy_from_user(data_out, karg.data_out_buf_ptr,
				   data_out_sz)) {
			pr_err("failure at %s:%d/%s()!\n", __FILE__,
			       __LINE__, __func__);
			ret = -EFAULT;
			leapioraid_base_free_smid(ioc, smid);
			goto out;
		}
	}
	if (data_in_sz) {
		data_in = dma_alloc_coherent(&ioc->pdev->dev, data_in_sz,
					     &data_in_dma, GFP_ATOMIC);
		if (!data_in) {
			ret = -ENOMEM;
			leapioraid_base_free_smid(ioc, smid);
			goto out;
		}
	}
	psge = (void *)request + (karg.data_sge_offset * 4);
	leapioraid_ctl_display_some_debug(ioc, smid, "ctl_request", NULL);
	init_completion(&ioc->ctl_cmds.done);
	switch (mpi_request->Function) {
	case LEAPIORAID_FUNC_SCSI_IO_REQUEST:
	case LEAPIORAID_FUNC_RAID_SCSI_IO_PASSTHROUGH:
		{
			struct LeapioSCSIIOReq_t *scsiio_request =
			    (struct LeapioSCSIIOReq_t *) request;
			scsiio_request->SenseBufferLength =
			    SCSI_SENSE_BUFFERSIZE;
			scsiio_request->SenseBufferLowAddress =
			    leapioraid_base_get_sense_buffer_dma(ioc, smid);
			memset(ioc->ctl_cmds.sense, 0, SCSI_SENSE_BUFFERSIZE);
			ioc->build_sg(ioc, psge, data_out_dma, data_out_sz,
				      data_in_dma, data_in_sz);
			if (test_bit
			    (device_handle, ioc->device_remove_in_progress)) {
				dtmprintk(ioc,
					  pr_info(
						"%s handle(0x%04x) :ioctl failed due to device removal in progress\n",
						ioc->name, device_handle));
				leapioraid_base_free_smid(ioc, smid);
				ret = -EINVAL;
				goto out;
			}
			if (mpi_request->Function ==
			    LEAPIORAID_FUNC_SCSI_IO_REQUEST)
				ioc->put_smid_scsi_io(ioc, smid, device_handle);
			else
				ioc->put_smid_default(ioc, smid);
			break;
		}
	case LEAPIORAID_FUNC_SCSI_TASK_MGMT:
		{
			struct LeapioraidSCSITmgReq_t *tm_request =
			    (struct LeapioraidSCSITmgReq_t *) request;
			dtmprintk(ioc,
				pr_info("%s TASK_MGMT: handle(0x%04x), task_type(0x%02x)\n",
					ioc->name,
					le16_to_cpu(tm_request->DevHandle),
					tm_request->TaskType));
			ioc->got_task_abort_from_ioctl = 1;
			if (tm_request->TaskType ==
			    LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABORT_TASK ||
			    tm_request->TaskType ==
			    LEAPIORAID_SCSITASKMGMT_TASKTYPE_QUERY_TASK) {
				if (leapioraid_ctl_set_task_mid(ioc, &karg, tm_request)) {
					leapioraid_base_free_smid(ioc, smid);
					ioc->got_task_abort_from_ioctl = 0;
					goto out;
				}
			}
			ioc->got_task_abort_from_ioctl = 0;
			if (test_bit
			    (device_handle, ioc->device_remove_in_progress)) {
				dtmprintk(ioc,
					pr_info(
						"%s handle(0x%04x) :ioctl failed due to device removal in progress\n",
						ioc->name, device_handle));
				leapioraid_base_free_smid(ioc, smid);
				ret = -EINVAL;
				goto out;
			}
			leapioraid_scsihost_set_tm_flag(ioc,
							le16_to_cpu(tm_request->DevHandle));
			ioc->build_sg_mpi(ioc, psge, data_out_dma, data_out_sz,
					  data_in_dma, data_in_sz);
			ioc->put_smid_hi_priority(ioc, smid, 0);
			break;
		}
	case LEAPIORAID_FUNC_SMP_PASSTHROUGH:
		{
			struct LeapioraidSmpPassthroughReq_t *smp_request =
			    (struct LeapioraidSmpPassthroughReq_t *) mpi_request;
			u8 *data;

			if (!ioc->multipath_on_hba)
				smp_request->PhysicalPort = 0xFF;
			if (smp_request->PassthroughFlags &
			    0x80)
				data = (u8 *) &smp_request->SGL;
			else {
				if (unlikely(data_out == NULL)) {
					pr_err(
					       "failure at %s:%d/%s()!\n",
					       __FILE__, __LINE__, __func__);
					leapioraid_base_free_smid(ioc, smid);
					ret = -EINVAL;
					goto out;
				}
				data = data_out;
			}
			if (data[1] == 0x91 && (data[10] == 1 || data[10] == 2)) {
				ioc->ioc_link_reset_in_progress = 1;
				ioc->ignore_loginfos = 1;
			}
			ioc->build_sg(ioc, psge, data_out_dma, data_out_sz,
				      data_in_dma, data_in_sz);
			ioc->put_smid_default(ioc, smid);
			break;
		}
	case LEAPIORAID_FUNC_SATA_PASSTHROUGH:
		{
			ioc->build_sg(ioc, psge, data_out_dma, data_out_sz,
				      data_in_dma, data_in_sz);
			if (test_bit
			    (device_handle, ioc->device_remove_in_progress)) {
				dtmprintk(ioc,
					pr_info(
						"%s handle(0x%04x) :ioctl failed due to device removal in progress\n",
						ioc->name, device_handle));
				leapioraid_base_free_smid(ioc, smid);
				ret = -EINVAL;
				goto out;
			}
			ioc->put_smid_default(ioc, smid);
			break;
		}
	case LEAPIORAID_FUNC_FW_DOWNLOAD:
	case LEAPIORAID_FUNC_FW_UPLOAD:
		{
			ioc->build_sg(ioc, psge, data_out_dma, data_out_sz,
				      data_in_dma, data_in_sz);
			ioc->put_smid_default(ioc, smid);
			break;
		}
	case LEAPIORAID_FUNC_SAS_IO_UNIT_CONTROL:
		{
			struct LeapioraidSasIoUnitControlReq_t *sasiounit_request =
			    (struct LeapioraidSasIoUnitControlReq_t *) mpi_request;
			if (sasiounit_request->Operation ==
			    LEAPIORAID_SAS_OP_PHY_HARD_RESET
			    || sasiounit_request->Operation ==
			    LEAPIORAID_SAS_OP_PHY_LINK_RESET) {
				ioc->ioc_link_reset_in_progress = 1;
				ioc->ignore_loginfos = 1;
			}
		}
		fallthrough;
	default:
		ioc->build_sg_mpi(ioc, psge, data_out_dma, data_out_sz,
				  data_in_dma, data_in_sz);
		ioc->put_smid_default(ioc, smid);
		break;
	}
	timeout = karg.timeout;
	if (timeout < LEAPIORAID_IOCTL_DEFAULT_TIMEOUT)
		timeout = LEAPIORAID_IOCTL_DEFAULT_TIMEOUT;
	wait_for_completion_timeout(&ioc->ctl_cmds.done, timeout * HZ);
	if (mpi_request->Function == LEAPIORAID_FUNC_SCSI_TASK_MGMT) {
		struct LeapioraidSCSITmgReq_t *tm_request =
		    (struct LeapioraidSCSITmgReq_t *) mpi_request;
		leapioraid_scsihost_clear_tm_flag(ioc,
						  le16_to_cpu(tm_request->DevHandle));
	} else if ((mpi_request->Function == LEAPIORAID_FUNC_SMP_PASSTHROUGH
		 || mpi_request->Function ==
		 LEAPIORAID_FUNC_SAS_IO_UNIT_CONTROL)
		&& ioc->ioc_link_reset_in_progress) {
		ioc->ioc_link_reset_in_progress = 0;
		ioc->ignore_loginfos = 0;
	}
	if (!(ioc->ctl_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		leapioraid_check_cmd_timeout(ioc,
					     ioc->ctl_cmds.status, mpi_request,
					     karg.data_sge_offset, issue_reset);
		goto issue_host_reset;
	}
	mpi_reply = ioc->ctl_cmds.reply;
	if (mpi_reply->Function == LEAPIORAID_FUNC_SCSI_TASK_MGMT &&
	    (ioc->logging_level & LEAPIORAID_DEBUG_TM)) {
		struct LeapioraidSCSITmgRep_t *tm_reply =
		    (struct LeapioraidSCSITmgRep_t *) mpi_reply;
		pr_info(
			"%s TASK_MGMT: IOCStatus(0x%04x), IOCLogInfo(0x%08x), TerminationCount(0x%08x)\n",
			ioc->name,
			le16_to_cpu(tm_reply->IOCStatus),
			le32_to_cpu(tm_reply->IOCLogInfo),
			le32_to_cpu(tm_reply->TerminationCount));
	}
	if (data_in_sz) {
		if (copy_to_user(karg.data_in_buf_ptr, data_in, data_in_sz)) {
			pr_err("failure at %s:%d/%s()!\n", __FILE__,
			       __LINE__, __func__);
			ret = -ENODATA;
			goto out;
		}
	}
	if (karg.max_reply_bytes) {
		sz = min_t(u32, karg.max_reply_bytes, ioc->reply_sz);
		if (copy_to_user(karg.reply_frame_buf_ptr, ioc->ctl_cmds.reply,
				 sz)) {
			pr_err("failure at %s:%d/%s()!\n", __FILE__,
			       __LINE__, __func__);
			ret = -ENODATA;
			goto out;
		}
	}
	if (karg.max_sense_bytes && (mpi_request->Function ==
				     LEAPIORAID_FUNC_SCSI_IO_REQUEST
				     || mpi_request->Function ==
				     LEAPIORAID_FUNC_RAID_SCSI_IO_PASSTHROUGH)) {
		if (karg.sense_data_ptr == NULL) {
			pr_err(
				"%s Response buffer provided by application is NULL; Response data will not be returned.\n",
				ioc->name);
			goto out;
		}
		sz_arg = SCSI_SENSE_BUFFERSIZE;
		sz = min_t(u32, karg.max_sense_bytes, sz_arg);
		if (copy_to_user(karg.sense_data_ptr, ioc->ctl_cmds.sense, sz)) {
			pr_err("failure at %s:%d/%s()!\n",
				__FILE__, __LINE__, __func__);
			ret = -ENODATA;
			goto out;
		}
	}
issue_host_reset:
	if (issue_reset) {
		ret = -ENODATA;
		if ((mpi_request->Function == LEAPIORAID_FUNC_SCSI_IO_REQUEST
		     || mpi_request->Function ==
		     LEAPIORAID_FUNC_RAID_SCSI_IO_PASSTHROUGH
		     || mpi_request->Function ==
		     LEAPIORAID_FUNC_SATA_PASSTHROUGH)) {
			pr_err(
			       "%s issue target reset: handle  = (0x%04x)\n",
			       ioc->name,
			       le16_to_cpu(mpi_request->FunctionDependent1));
			leapioraid_halt_firmware(ioc, 0);
			leapioraid_scsihost_issue_locked_tm(ioc,
					le16_to_cpu
					(mpi_request->FunctionDependent1),
					0, 0, 0,
					LEAPIORAID_SCSITASKMGMT_TASKTYPE_TARGET_RESET,
					smid, 30,
					LEAPIORAID_SCSITASKMGMT_MSGFLAGS_LINK_RESET);
		} else
			leapioraid_base_hard_reset_handler(ioc,
							   FORCE_BIG_HAMMER);
	}
out:
	if (data_in)
		dma_free_coherent(&ioc->pdev->dev, data_in_sz, data_in,
				  data_in_dma);
	if (data_out)
		dma_free_coherent(&ioc->pdev->dev, data_out_sz, data_out,
				  data_out_dma);
	kfree(mpi_request);
	ioc->ctl_cmds.status = LEAPIORAID_CMD_NOT_USED;
	return ret;
}

static long
leapioraid_ctl_getiocinfo(
	struct LEAPIORAID_ADAPTER *ioc, void __user *arg)
{
	struct leapio_ioctl_iocinfo karg;
	u8 revision;

	dctlprintk(ioc, pr_info("%s %s: enter\n", ioc->name,
			       __func__));
	memset(&karg, 0, sizeof(karg));
	if (ioc->pfacts)
		karg.port_number = ioc->pfacts[0].PortNumber;
	pci_read_config_byte(ioc->pdev, PCI_CLASS_REVISION, &revision);
	karg.hw_rev = revision;
	karg.pci_id = ioc->pdev->device;
	karg.subsystem_device = ioc->pdev->subsystem_device;
	karg.subsystem_vendor = ioc->pdev->subsystem_vendor;
	karg.pci_information.u.bits.bus = ioc->pdev->bus->number;
	karg.pci_information.u.bits.device = PCI_SLOT(ioc->pdev->devfn);
	karg.pci_information.u.bits.function = PCI_FUNC(ioc->pdev->devfn);
	karg.pci_information.segment_id = pci_domain_nr(ioc->pdev->bus);
	karg.firmware_version = ioc->facts.FWVersion.Word;
	strscpy(karg.driver_version, ioc->driver_name, sizeof(karg.driver_version));
	strcat(karg.driver_version, "-");
	karg.adapter_type = 0x06;
	strcat(karg.driver_version, LEAPIORAID_DRIVER_VERSION);
	karg.adapter_type = 0x07;
	karg.bios_version = le32_to_cpu(ioc->bios_pg3.BiosVersion);
	if (copy_to_user(arg, &karg, sizeof(karg))) {
		pr_err("failure at %s:%d/%s()!\n",
		       __FILE__, __LINE__, __func__);
		return -EFAULT;
	}
	return 0;
}

static long
leapioraid_ctl_eventquery(
	struct LEAPIORAID_ADAPTER *ioc, void __user *arg)
{
	struct leapio_ioctl_eventquery karg;

	if (copy_from_user(&karg, arg, sizeof(karg))) {
		pr_err("failure at %s:%d/%s()!\n",
		       __FILE__, __LINE__, __func__);
		return -EFAULT;
	}
	dctlprintk(ioc, pr_info("%s %s: enter\n", ioc->name,
			       __func__));
	karg.event_entries = LEAPIORAID_CTL_EVENT_LOG_SIZE;
	memcpy(karg.event_types, ioc->event_type,
	       LEAPIORAID_EVENT_NOTIFY_EVENTMASK_WORDS * sizeof(u32));
	if (copy_to_user(arg, &karg, sizeof(karg))) {
		pr_err("failure at %s:%d/%s()!\n",
		       __FILE__, __LINE__, __func__);
		return -EFAULT;
	}
	return 0;
}

static long
leapioraid_ctl_eventenable(
	struct LEAPIORAID_ADAPTER *ioc, void __user *arg)
{
	struct leapio_ioctl_eventenable karg;

	if (copy_from_user(&karg, arg, sizeof(karg))) {
		pr_err("failure at %s:%d/%s()!\n",
		       __FILE__, __LINE__, __func__);
		return -EFAULT;
	}
	dctlprintk(ioc, pr_info("%s %s: enter\n", ioc->name,
			       __func__));
	memcpy(ioc->event_type, karg.event_types,
	       LEAPIORAID_EVENT_NOTIFY_EVENTMASK_WORDS * sizeof(u32));
	leapioraid_base_validate_event_type(ioc, ioc->event_type);
	if (ioc->event_log)
		return 0;
	ioc->event_context = 0;
	ioc->aen_event_read_flag = 0;
	ioc->event_log = kcalloc(LEAPIORAID_CTL_EVENT_LOG_SIZE,
				 sizeof(struct LEAPIORAID_IOCTL_EVENTS),
				 GFP_KERNEL);
	if (!ioc->event_log) {
		pr_err("failure at %s:%d/%s()!\n",
		       __FILE__, __LINE__, __func__);
		return -ENOMEM;
	}
	return 0;
}

static long
leapioraid_ctl_eventreport(
	struct LEAPIORAID_ADAPTER *ioc, void __user *arg)
{
	struct leapio_ioctl_eventreport karg;
	u32 number_bytes, max_events, max;
	struct leapio_ioctl_eventreport __user *uarg = arg;

	if (copy_from_user(&karg, arg, sizeof(karg))) {
		pr_err("failure at %s:%d/%s()!\n",
		       __FILE__, __LINE__, __func__);
		return -EFAULT;
	}
	dctlprintk(ioc, pr_info("%s %s: enter\n", ioc->name,
			       __func__));
	number_bytes = karg.hdr.max_data_size -
	    sizeof(struct leapio_ioctl_header);
	max_events = number_bytes / sizeof(struct LEAPIORAID_IOCTL_EVENTS);
	max = min_t(u32, LEAPIORAID_CTL_EVENT_LOG_SIZE, max_events);
	if (!max || !ioc->event_log)
		return -ENODATA;
	number_bytes = max * sizeof(struct LEAPIORAID_IOCTL_EVENTS);
	if (copy_to_user(uarg->event_data, ioc->event_log, number_bytes)) {
		pr_err("failure at %s:%d/%s()!\n",
		       __FILE__, __LINE__, __func__);
		return -EFAULT;
	}
	ioc->aen_event_read_flag = 0;
	return 0;
}

static long
leapioraid_ctl_do_reset(
	struct LEAPIORAID_ADAPTER *ioc, void __user *arg)
{
	struct leapio_ioctl_diag_reset karg;
	int retval;

	if (copy_from_user(&karg, arg, sizeof(karg))) {
		pr_err("failure at %s:%d/%s()!\n",
		       __FILE__, __LINE__, __func__);
		return -EFAULT;
	}
	if (ioc->shost_recovery ||
	    ioc->pci_error_recovery || ioc->is_driver_loading ||
	    ioc->remove_host)
		return -EAGAIN;
	dctlprintk(ioc, pr_info("%s %s: enter\n", ioc->name,
			       __func__));
	ioc->reset_from_user = 1;
	scsi_block_requests(ioc->shost);
	retval = leapioraid_base_hard_reset_handler(ioc, FORCE_BIG_HAMMER);
	scsi_unblock_requests(ioc->shost);
	pr_info("%s ioctl: host reset: %s\n",
	       ioc->name, ((!retval) ? "SUCCESS" : "FAILED"));
	return 0;
}

static int
leapioraid_ctl_btdh_search_sas_device(struct LEAPIORAID_ADAPTER *ioc,
			    struct leapio_ioctl_btdh_mapping *btdh)
{
	struct leapioraid_sas_device *sas_device;
	unsigned long flags;
	int rc = 0;

	if (list_empty(&ioc->sas_device_list))
		return rc;
	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	list_for_each_entry(sas_device, &ioc->sas_device_list, list) {
		if (btdh->bus == 0xFFFFFFFF && btdh->id == 0xFFFFFFFF &&
		    btdh->handle == sas_device->handle) {
			btdh->bus = sas_device->channel;
			btdh->id = sas_device->id;
			rc = 1;
			goto out;
		} else if (btdh->bus == sas_device->channel && btdh->id ==
			   sas_device->id && btdh->handle == 0xFFFF) {
			btdh->handle = sas_device->handle;
			rc = 1;
			goto out;
		}
	}
out:
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	return rc;
}

static int
leapioraid_ctl_btdh_search_raid_device(struct LEAPIORAID_ADAPTER *ioc,
			     struct leapio_ioctl_btdh_mapping *btdh)
{
	struct leapioraid_raid_device *raid_device;
	unsigned long flags;
	int rc = 0;

	if (list_empty(&ioc->raid_device_list))
		return rc;
	spin_lock_irqsave(&ioc->raid_device_lock, flags);
	list_for_each_entry(raid_device, &ioc->raid_device_list, list) {
		if (btdh->bus == 0xFFFFFFFF && btdh->id == 0xFFFFFFFF &&
		    btdh->handle == raid_device->handle) {
			btdh->bus = raid_device->channel;
			btdh->id = raid_device->id;
			rc = 1;
			goto out;
		} else if (btdh->bus == raid_device->channel && btdh->id ==
			   raid_device->id && btdh->handle == 0xFFFF) {
			btdh->handle = raid_device->handle;
			rc = 1;
			goto out;
		}
	}
out:
	spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
	return rc;
}

static long
leapioraid_ctl_btdh_mapping(
	struct LEAPIORAID_ADAPTER *ioc, void __user *arg)
{
	struct leapio_ioctl_btdh_mapping karg;
	int rc;

	if (copy_from_user(&karg, arg, sizeof(karg))) {
		pr_err("failure at %s:%d/%s()!\n",
		       __FILE__, __LINE__, __func__);
		return -EFAULT;
	}
	dctlprintk(ioc, pr_info("%s %s\n", ioc->name,
			       __func__));
	rc = leapioraid_ctl_btdh_search_sas_device(ioc, &karg);
	if (!rc)
		leapioraid_ctl_btdh_search_raid_device(ioc, &karg);
	if (copy_to_user(arg, &karg, sizeof(karg))) {
		pr_err("failure at %s:%d/%s()!\n",
		       __FILE__, __LINE__, __func__);
		return -EFAULT;
	}
	return 0;
}

#ifdef CONFIG_COMPAT
static long
leapioraid_ctl_compat_command(
				struct LEAPIORAID_ADAPTER *ioc, unsigned int cmd,
			    void __user *arg)
{
	struct leapio_ioctl_command32 karg32;
	struct leapio_ioctl_command32 __user *uarg;
	struct leapio_ioctl_command karg;

	if (_IOC_SIZE(cmd) != sizeof(struct leapio_ioctl_command32))
		return -EINVAL;
	uarg = (struct leapio_ioctl_command32 __user *)arg;
	if (copy_from_user(&karg32, (char __user *)arg, sizeof(karg32))) {
		pr_err("failure at %s:%d/%s()!\n",
		       __FILE__, __LINE__, __func__);
		return -EFAULT;
	}
	memset(&karg, 0, sizeof(struct leapio_ioctl_command));
	karg.hdr.ioc_number = karg32.hdr.ioc_number;
	karg.hdr.port_number = karg32.hdr.port_number;
	karg.hdr.max_data_size = karg32.hdr.max_data_size;
	karg.timeout = karg32.timeout;
	karg.max_reply_bytes = karg32.max_reply_bytes;
	karg.data_in_size = karg32.data_in_size;
	karg.data_out_size = karg32.data_out_size;
	karg.max_sense_bytes = karg32.max_sense_bytes;
	karg.data_sge_offset = karg32.data_sge_offset;
	karg.reply_frame_buf_ptr = compat_ptr(karg32.reply_frame_buf_ptr);
	karg.data_in_buf_ptr = compat_ptr(karg32.data_in_buf_ptr);
	karg.data_out_buf_ptr = compat_ptr(karg32.data_out_buf_ptr);
	karg.sense_data_ptr = compat_ptr(karg32.sense_data_ptr);
	return leapioraid_ctl_do_command(ioc, karg, &uarg->mf);
}
#endif

static long
leapioraid_ctl_ioctl_main(
	struct file *file, unsigned int cmd, void __user *arg,
	u8 compat)
{
	struct LEAPIORAID_ADAPTER *ioc;
	struct leapio_ioctl_header ioctl_header;
	enum leapioraid_block_state state;
	long ret = -ENOIOCTLCMD;

	if (copy_from_user(&ioctl_header, (char __user *)arg,
			   sizeof(struct leapio_ioctl_header))) {
		pr_err("failure at %s:%d/%s()!\n",
		       __FILE__, __LINE__, __func__);
		return -EFAULT;
	}
	if (leapioraid_ctl_verify_adapter(ioctl_header.ioc_number,
				&ioc) == -1 || !ioc)
		return -ENODEV;
	mutex_lock(&ioc->pci_access_mutex);
	if (ioc->shost_recovery ||
	    ioc->pci_error_recovery || ioc->is_driver_loading ||
	    ioc->remove_host) {
		ret = -EAGAIN;
		goto unlock_pci_access;
	}
	state = (file->f_flags & O_NONBLOCK) ? NON_BLOCKING : BLOCKING;
	if (state == NON_BLOCKING) {
		if (!mutex_trylock(&ioc->ctl_cmds.mutex)) {
			ret = -EAGAIN;
			goto unlock_pci_access;
		}
	} else if (mutex_lock_interruptible(&ioc->ctl_cmds.mutex)) {
		ret = -ERESTARTSYS;
		goto unlock_pci_access;
	}
	switch (cmd) {
	case LEAPIORAID_IOCINFO:
		if (_IOC_SIZE(cmd) == sizeof(struct leapio_ioctl_iocinfo))
			ret = leapioraid_ctl_getiocinfo(ioc, arg);
		break;
#ifdef CONFIG_COMPAT
	case LEAPIORAID_COMMAND32:
#endif
	case LEAPIORAID_COMMAND:
		{
			struct leapio_ioctl_command __user *uarg;
			struct leapio_ioctl_command karg;

#ifdef CONFIG_COMPAT
			if (compat) {
				ret =
				    leapioraid_ctl_compat_command(ioc, cmd, arg);
				break;
			}
#endif
			if (copy_from_user(&karg, arg, sizeof(karg))) {
				pr_err("failure at %s:%d/%s()!\n",
				       __FILE__, __LINE__, __func__);
				ret = -EFAULT;
				break;
			}
			if (karg.hdr.ioc_number != ioctl_header.ioc_number) {
				ret = -EINVAL;
				break;
			}
			if (_IOC_SIZE(cmd) ==
			    sizeof(struct leapio_ioctl_command)) {
				uarg = arg;
				ret =
				    leapioraid_ctl_do_command(ioc, karg,
							    &uarg->mf);
			}
			break;
		}
	case LEAPIORAID_EVENTQUERY:
		if (_IOC_SIZE(cmd) == sizeof(struct leapio_ioctl_eventquery))
			ret = leapioraid_ctl_eventquery(ioc, arg);
		break;
	case LEAPIORAID_EVENTENABLE:
		if (_IOC_SIZE(cmd) == sizeof(struct leapio_ioctl_eventenable))
			ret = leapioraid_ctl_eventenable(ioc, arg);
		break;
	case LEAPIORAID_EVENTREPORT:
		ret = leapioraid_ctl_eventreport(ioc, arg);
		break;
	case LEAPIORAID_HARDRESET:
		if (_IOC_SIZE(cmd) == sizeof(struct leapio_ioctl_diag_reset))
			ret = leapioraid_ctl_do_reset(ioc, arg);
		break;
	case LEAPIORAID_BTDHMAPPING:
		if (_IOC_SIZE(cmd) == sizeof(struct leapio_ioctl_btdh_mapping))
			ret = leapioraid_ctl_btdh_mapping(ioc, arg);
		break;
	default:
		dctlprintk(ioc, pr_err(
				       "%s unsupported ioctl opcode(0x%08x)\n",
				       ioc->name, cmd));
		break;
	}
	mutex_unlock(&ioc->ctl_cmds.mutex);
unlock_pci_access:
	mutex_unlock(&ioc->pci_access_mutex);
	return ret;
}

static long
leapioraid_ctl_ioctl(
	struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret;

	ret = leapioraid_ctl_ioctl_main(file, cmd, (void __user *)arg, 0);
	return ret;
}

#ifdef CONFIG_COMPAT
static long
leapioraid_ctl_ioctl_compat(
	struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret;

	ret = leapioraid_ctl_ioctl_main(file, cmd, (void __user *)arg, 1);
	return ret;
}
#endif

static ssize_t
version_fw_show(
	struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, PAGE_SIZE, "%02d.%02d.%02d.%02d\n",
			(ioc->facts.FWVersion.Word & 0xFF000000) >> 24,
			(ioc->facts.FWVersion.Word & 0x00FF0000) >> 16,
			(ioc->facts.FWVersion.Word & 0x0000FF00) >> 8,
			ioc->facts.FWVersion.Word & 0x000000FF);
}
static DEVICE_ATTR_RO(version_fw);

static ssize_t
version_bios_show(
	struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);
	u32 version = le32_to_cpu(ioc->bios_pg3.BiosVersion);

	return snprintf(buf, PAGE_SIZE, "%02d.%02d.%02d.%02d\n",
			(version & 0xFF000000) >> 24,
			(version & 0x00FF0000) >> 16,
			(version & 0x0000FF00) >> 8, version & 0x000000FF);
}
static DEVICE_ATTR_RO(version_bios);

static ssize_t
version_leapioraid_show(struct device *cdev, struct device_attribute *attr,
		      char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, PAGE_SIZE, "%03x.%02x\n",
			ioc->facts.MsgVersion, ioc->facts.HeaderVersion >> 8);
}
static DEVICE_ATTR_RO(version_leapioraid);

static ssize_t
version_product_show(
	struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, 16, "%s\n", ioc->manu_pg0.ChipName);
}
static DEVICE_ATTR_RO(version_product);

static ssize_t
version_nvdata_persistent_show(struct device *cdev,
				    struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, PAGE_SIZE, "%08xh\n",
			le32_to_cpu(ioc->iounit_pg0.NvdataVersionPersistent.Word));
}
static DEVICE_ATTR_RO(version_nvdata_persistent);

static ssize_t
version_nvdata_default_show(struct device *cdev,
						struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, PAGE_SIZE, "%08xh\n",
			le32_to_cpu(ioc->iounit_pg0.NvdataVersionDefault.Word));
}
static DEVICE_ATTR_RO(version_nvdata_default);

static ssize_t
board_name_show(
	struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, 16, "%s\n", ioc->manu_pg0.BoardName);
}
static DEVICE_ATTR_RO(board_name);

static ssize_t
board_assembly_show(
	struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, 16, "%s\n", ioc->manu_pg0.BoardAssembly);
}
static DEVICE_ATTR_RO(board_assembly);

static ssize_t
board_tracer_show(
	struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, 16, "%s\n", ioc->manu_pg0.BoardTracerNumber);
}
static DEVICE_ATTR_RO(board_tracer);

static ssize_t
io_delay_show(
	struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, PAGE_SIZE, "%02d\n", ioc->io_missing_delay);
}
static DEVICE_ATTR_RO(io_delay);

static ssize_t
device_delay_show(
	struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, PAGE_SIZE, "%02d\n", ioc->device_missing_delay);
}
static DEVICE_ATTR_RO(device_delay);

static ssize_t
fw_queue_depth_show(
	struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, PAGE_SIZE, "%02d\n", ioc->facts.RequestCredit);
}
static DEVICE_ATTR_RO(fw_queue_depth);

static ssize_t
host_sas_address_show(
	struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, PAGE_SIZE, "0x%016llx\n",
			(unsigned long long)ioc->sas_hba.sas_address);
}
static DEVICE_ATTR_RO(host_sas_address);

static ssize_t
logging_level_show(
	struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, PAGE_SIZE, "%08xh\n", ioc->logging_level);
}

static ssize_t
logging_level_store(
	struct device *cdev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);
	int val = 0;

	if (kstrtoint(buf, 0, &val))
		return -EINVAL;
	ioc->logging_level = val;
	pr_info("%s logging_level=%08xh\n", ioc->name,
	       ioc->logging_level);
	return strlen(buf);
}
static DEVICE_ATTR_RW(logging_level);

static ssize_t
fwfault_debug_show(
	struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, PAGE_SIZE, "%d\n", ioc->fwfault_debug);
}

static ssize_t
fwfault_debug_store(
	struct device *cdev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);
	int val = 0;

	if (kstrtoint(buf, 0, &val))
		return -EINVAL;
	ioc->fwfault_debug = val;
	pr_info("%s fwfault_debug=%d\n", ioc->name,
	       ioc->fwfault_debug);
	return strlen(buf);
}
static DEVICE_ATTR_RW(fwfault_debug);

static
struct leapioraid_raid_device *leapioraid_ctl_raid_device_find_by_handle(
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

u8
leapioraid_ctl_tm_done(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid, u8 msix_index,
	u32 reply)
{
	u8 rc;
	unsigned long flags;
	struct leapioraid_sas_device *sas_device;
	struct leapioraid_raid_device *raid_device;
	u16 smid_task_abort;
	u16 handle;
	struct LeapioraidSCSITmgReq_t *mpi_request;
	struct LeapioraidSCSITmgRep_t *mpi_reply =
	    leapioraid_base_get_reply_virt_addr(ioc, reply);

	rc = 1;
	if (unlikely(!mpi_reply)) {
		pr_err(
		       "%s mpi_reply not valid at %s:%d/%s()!\n", ioc->name,
		       __FILE__, __LINE__, __func__);
		return rc;
	}
	handle = le16_to_cpu(mpi_reply->DevHandle);
	sas_device = leapioraid_get_sdev_by_handle(ioc, handle);
	if (sas_device) {
		smid_task_abort = 0;
		if (mpi_reply->TaskType ==
		    LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABORT_TASK) {
			mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
			smid_task_abort = le16_to_cpu(mpi_request->TaskMID);
		}
		pr_info("\tcomplete: sas_addr(0x%016llx), handle(0x%04x), smid(%d), term(%d)\n",
			(unsigned long long)sas_device->sas_address, handle,
			(smid_task_abort ? smid_task_abort : smid),
			le32_to_cpu(mpi_reply->TerminationCount));
		leapioraid_sas_device_put(sas_device);
	}
	spin_lock_irqsave(&ioc->raid_device_lock, flags);
	raid_device = leapioraid_ctl_raid_device_find_by_handle(ioc, handle);
	if (raid_device)
		pr_info("\tcomplete: wwid(0x%016llx), handle(0x%04x), smid(%d), term(%d)\n",
			(unsigned long long)raid_device->wwid, handle,
			smid, le32_to_cpu(mpi_reply->TerminationCount));
	spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
	ioc->terminated_tm_count += le32_to_cpu(mpi_reply->TerminationCount);
	if (ioc->out_of_frames) {
		rc = 0;
		leapioraid_base_free_smid(ioc, smid);
		ioc->out_of_frames = 0;
		wake_up(&ioc->no_frames_tm_wq);
	}
	ioc->pending_tm_count--;
	if (!ioc->pending_tm_count)
		wake_up(&ioc->pending_tm_wq);
	return rc;
}

static void
leapioraid_ctl_tm_sysfs(struct LEAPIORAID_ADAPTER *ioc, u8 task_type)
{
	struct leapioraid_sas_device *sas_device;
	struct leapioraid_raid_device *raid_device;
	struct LeapioraidSCSITmgReq_t *mpi_request;
	u16 smid, handle, hpr_smid;
	struct LEAPIORAID_DEVICE *device_priv_data;
	struct LEAPIORAID_TARGET *target_priv_data;
	struct scsi_cmnd *scmd;
	struct scsi_device *sdev;
	unsigned long flags;
	int tm_count;
	int lun;
	u32 doorbell;
	struct leapioraid_scsiio_tracker *st;
	u8 tr_method = 0x00;

	if (list_empty(&ioc->sas_device_list))
		return;
	spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock, flags);
	if (ioc->shost_recovery || ioc->remove_host) {
		spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock, flags);
		pr_err(
		       "%s %s: busy : host reset in progress, try later\n",
		       ioc->name, __func__);
		return;
	}
	spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock, flags);
	scsi_block_requests(ioc->shost);
	init_waitqueue_head(&ioc->pending_tm_wq);
	ioc->ignore_loginfos = 1;
	ioc->pending_tm_count = 0;
	ioc->terminated_tm_count = 0;
	ioc->out_of_frames = 0;
	tm_count = 0;
	switch (task_type) {
	case LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABORT_TASK:
		for (smid = 1; smid <= ioc->shost->can_queue; smid++) {
			if (list_empty(&ioc->hpr_free_list)) {
				ioc->out_of_frames = 1;
				init_waitqueue_head(&ioc->no_frames_tm_wq);
				wait_event_timeout(ioc->no_frames_tm_wq,
						   !ioc->out_of_frames, HZ);
			}
			scmd = leapioraid_scsihost_scsi_lookup_get(ioc, smid);
			if (!scmd)
				continue;
			st = leapioraid_base_scsi_cmd_priv(scmd);
			if ((!st) || (st->cb_idx == 0xFF) || (st->smid == 0))
				continue;
			lun = scmd->device->lun;
			device_priv_data = scmd->device->hostdata;
			if (!device_priv_data || !device_priv_data->sas_target)
				continue;
			target_priv_data = device_priv_data->sas_target;
			if (!target_priv_data)
				continue;
			if (target_priv_data->flags &
			    LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT ||
			    target_priv_data->flags & LEAPIORAID_TARGET_FLAGS_VOLUME)
				continue;
			handle = device_priv_data->sas_target->handle;
			hpr_smid = leapioraid_base_get_smid_hpr(ioc,
								ioc->ctl_tm_cb_idx);
			if (!hpr_smid) {
				pr_err(
				       "%s %s: out of hi-priority requests!!\n",
				       ioc->name, __func__);
				goto out_of_frames;
			}
			mpi_request =
			    leapioraid_base_get_msg_frame(ioc, hpr_smid);
			memset(mpi_request, 0,
			       sizeof(struct LeapioraidSCSITmgReq_t));
			mpi_request->Function = LEAPIORAID_FUNC_SCSI_TASK_MGMT;
			mpi_request->DevHandle = cpu_to_le16(handle);
			mpi_request->TaskType =
			    LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABORT_TASK;
			mpi_request->TaskMID = cpu_to_le16(st->smid);
			int_to_scsilun(lun,
				       (struct scsi_lun *)mpi_request->LUN);
			starget_printk(KERN_INFO,
				device_priv_data->sas_target->starget,
				"sending tm: sas_addr(0x%016llx), handle(0x%04x), smid(%d)\n",
				(unsigned long long)
				device_priv_data->sas_target->sas_address, handle, st->smid);
			ioc->pending_tm_count++;
			tm_count++;
			doorbell = leapioraid_base_get_iocstate(ioc, 0);
			if ((doorbell &
			     LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_FAULT
			    || (doorbell & LEAPIORAID_IOC_STATE_MASK) ==
			    LEAPIORAID_IOC_STATE_COREDUMP)
				goto fault_in_progress;
			ioc->put_smid_hi_priority(ioc, hpr_smid, 0);
		}
		break;
	case LEAPIORAID_SCSITASKMGMT_TASKTYPE_TARGET_RESET:
		spin_lock_irqsave(&ioc->sas_device_lock, flags);
		list_for_each_entry(sas_device, &ioc->sas_device_list, list) {
			if (list_empty(&ioc->hpr_free_list)) {
				spin_unlock_irqrestore(&ioc->sas_device_lock,
						       flags);
				ioc->out_of_frames = 1;
				init_waitqueue_head(&ioc->no_frames_tm_wq);
				wait_event_timeout(ioc->no_frames_tm_wq,
						   !ioc->out_of_frames, HZ);
				spin_lock_irqsave(&ioc->sas_device_lock, flags);
			}
			if (!sas_device->starget)
				continue;
			if (test_bit(sas_device->handle, ioc->pd_handles))
				continue;
			hpr_smid = leapioraid_base_get_smid_hpr(ioc,
								ioc->ctl_tm_cb_idx);
			if (!hpr_smid) {
				pr_err(
				       "%s %s: out of hi-priority requests!!\n",
				       ioc->name, __func__);
				spin_unlock_irqrestore(&ioc->sas_device_lock,
						       flags);
				goto out_of_frames;
			}
			mpi_request =
			    leapioraid_base_get_msg_frame(ioc, hpr_smid);
			memset(mpi_request, 0,
			       sizeof(struct LeapioraidSCSITmgReq_t));
			mpi_request->Function = LEAPIORAID_FUNC_SCSI_TASK_MGMT;
			mpi_request->DevHandle =
			    cpu_to_le16(sas_device->handle);
			mpi_request->TaskType =
			    LEAPIORAID_SCSITASKMGMT_TASKTYPE_TARGET_RESET;
			starget_printk(KERN_INFO,
				sas_device->starget,
				"sending tm: sas_addr(0x%016llx), handle(0x%04x), smid(%d)\n",
				(unsigned long long)sas_device->sas_address,
				sas_device->handle,
				hpr_smid);
			ioc->pending_tm_count++;
			tm_count++;
			doorbell = leapioraid_base_get_iocstate(ioc, 0);
			if ((doorbell &
			     LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_FAULT
			    || (doorbell & LEAPIORAID_IOC_STATE_MASK) ==
			    LEAPIORAID_IOC_STATE_COREDUMP) {
				spin_unlock_irqrestore(&ioc->sas_device_lock,
						       flags);
				goto fault_in_progress;
			}
			ioc->put_smid_hi_priority(ioc, hpr_smid, 0);
		}
		spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
		spin_lock_irqsave(&ioc->raid_device_lock, flags);
		list_for_each_entry(raid_device, &ioc->raid_device_list, list) {
			if (list_empty(&ioc->hpr_free_list)) {
				spin_unlock_irqrestore(&ioc->raid_device_lock,
						       flags);
				ioc->out_of_frames = 1;
				init_waitqueue_head(&ioc->no_frames_tm_wq);
				wait_event_timeout(ioc->no_frames_tm_wq,
						   !ioc->out_of_frames, HZ);
				spin_lock_irqsave(&ioc->raid_device_lock,
						  flags);
			}
			if (!raid_device->starget)
				continue;
			hpr_smid = leapioraid_base_get_smid_hpr(ioc,
								ioc->ctl_tm_cb_idx);
			if (!hpr_smid) {
				pr_err("%s %s: out of hi-priority requests!!\n",
				       ioc->name, __func__);
				spin_unlock_irqrestore(&ioc->raid_device_lock,
						       flags);
				goto out_of_frames;
			}
			mpi_request =
			    leapioraid_base_get_msg_frame(ioc, hpr_smid);
			memset(mpi_request, 0,
			       sizeof(struct LeapioraidSCSITmgReq_t));
			mpi_request->Function = LEAPIORAID_FUNC_SCSI_TASK_MGMT;
			mpi_request->DevHandle =
			    cpu_to_le16(raid_device->handle);
			mpi_request->TaskType =
			    LEAPIORAID_SCSITASKMGMT_TASKTYPE_TARGET_RESET;
			starget_printk(KERN_INFO,
				raid_device->starget,
				"sending tm: wwid(0x%016llx), handle(0x%04x), smid(%d)\n",
				(unsigned long long)raid_device->wwid,
				raid_device->handle, hpr_smid);
			ioc->pending_tm_count++;
			tm_count++;
			doorbell = leapioraid_base_get_iocstate(ioc, 0);
			if ((doorbell &
			     LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_FAULT
			    || (doorbell & LEAPIORAID_IOC_STATE_MASK) ==
			    LEAPIORAID_IOC_STATE_COREDUMP) {
				spin_unlock_irqrestore(&ioc->raid_device_lock,
						       flags);
				goto fault_in_progress;
			}
			ioc->put_smid_hi_priority(ioc, hpr_smid, 0);
		}
		spin_unlock_irqrestore(&ioc->raid_device_lock, flags);
		break;
	case LEAPIORAID_SCSITASKMGMT_TASKTYPE_LOGICAL_UNIT_RESET:
	case LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABRT_TASK_SET:
		shost_for_each_device(sdev, ioc->shost) {
			if (list_empty(&ioc->hpr_free_list)) {
				ioc->out_of_frames = 1;
				init_waitqueue_head(&ioc->no_frames_tm_wq);
				wait_event_timeout(ioc->no_frames_tm_wq,
						   !ioc->out_of_frames, HZ);
			}
			device_priv_data = sdev->hostdata;
			if (!device_priv_data || !device_priv_data->sas_target)
				continue;
			target_priv_data = device_priv_data->sas_target;
			if (!target_priv_data)
				continue;
			if (target_priv_data->flags &
			    LEAPIORAID_TARGET_FLAGS_RAID_COMPONENT)
				continue;
			if ((target_priv_data->flags & LEAPIORAID_TARGET_FLAGS_VOLUME)
			    && (task_type ==
				LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABRT_TASK_SET))
				continue;
			handle = device_priv_data->sas_target->handle;
			hpr_smid = leapioraid_base_get_smid_hpr(ioc,
								ioc->ctl_tm_cb_idx);
			if (!hpr_smid) {
				pr_err("%s %s: out of hi-priority requests!!\n",
				       ioc->name, __func__);
				scsi_device_put(sdev);
				goto out_of_frames;
			}
			mpi_request =
			    leapioraid_base_get_msg_frame(ioc, hpr_smid);
			memset(mpi_request, 0,
			       sizeof(struct LeapioraidSCSITmgReq_t));
			mpi_request->Function = LEAPIORAID_FUNC_SCSI_TASK_MGMT;
			mpi_request->DevHandle = cpu_to_le16(handle);
			mpi_request->TaskType = task_type;
			mpi_request->MsgFlags = tr_method;
			int_to_scsilun(sdev->lun, (struct scsi_lun *)
				       mpi_request->LUN);
			sdev_printk(KERN_INFO, sdev,
				"sending tm: sas_addr(0x%016llx), handle(0x%04x), smid(%d)\n",
				(unsigned long long)target_priv_data->sas_address,
				handle, hpr_smid);
			ioc->pending_tm_count++;
			tm_count++;
			doorbell = leapioraid_base_get_iocstate(ioc, 0);
			if ((doorbell &
			     LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_FAULT
			    || (doorbell & LEAPIORAID_IOC_STATE_MASK) ==
			    LEAPIORAID_IOC_STATE_COREDUMP) {
				scsi_device_put(sdev);
				goto fault_in_progress;
			}
			ioc->put_smid_hi_priority(ioc, hpr_smid, 0);
		}
		break;
	}
out_of_frames:
	if (ioc->pending_tm_count)
		wait_event_timeout(ioc->pending_tm_wq,
				   !ioc->pending_tm_count, 30 * HZ);
	pr_info("%s task management requests issued(%d)\n",
	       ioc->name, tm_count);
	pr_info("%s number IO terminated(%d)\n",
	       ioc->name, ioc->terminated_tm_count);
fault_in_progress:
	scsi_unblock_requests(ioc->shost);
	ioc->ignore_loginfos = 0;
}

static ssize_t
task_management_store(
	struct device *cdev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);
	int opcode = 0;

	if (kstrtoint(buf, 0, &opcode))
		return -EINVAL;
	switch (opcode) {
	case 1:
		ioc->reset_from_user = 1;
		scsi_block_requests(ioc->shost);
		pr_err("%s sysfs: diag reset issued: %s\n", ioc->name,
		       ((!leapioraid_base_hard_reset_handler(ioc,
							     FORCE_BIG_HAMMER))
			? "SUCCESS" : "FAILED"));
		scsi_unblock_requests(ioc->shost);
		break;
	case 2:
		ioc->reset_from_user = 1;
		scsi_block_requests(ioc->shost);
		pr_err("%s sysfs: message unit reset issued: %s\n", ioc->name,
		       ((!leapioraid_base_hard_reset_handler(ioc,
							     SOFT_RESET)) ?
			"SUCCESS" : "FAILED"));
		scsi_unblock_requests(ioc->shost);
		break;
	case 3:
		pr_err("%s sysfs: TASKTYPE_ABORT_TASK :\n", ioc->name);
		ioc->got_task_abort_from_sysfs = 1;
		leapioraid_ctl_tm_sysfs(ioc,
					LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABORT_TASK);
		ioc->got_task_abort_from_sysfs = 0;
		break;
	case 4:
		pr_err("%s sysfs: TASKTYPE_TARGET_RESET:\n", ioc->name);
		leapioraid_ctl_tm_sysfs(ioc,
					LEAPIORAID_SCSITASKMGMT_TASKTYPE_TARGET_RESET);
		break;
	case 5:
		pr_err("%s sysfs: TASKTYPE_LOGICAL_UNIT_RESET:\n", ioc->name);
		leapioraid_ctl_tm_sysfs(ioc,
					LEAPIORAID_SCSITASKMGMT_TASKTYPE_LOGICAL_UNIT_RESET);
		break;
	case 6:
		pr_info("%s sysfs: TASKTYPE_ABRT_TASK_SET\n", ioc->name);
		leapioraid_ctl_tm_sysfs(ioc,
					LEAPIORAID_SCSITASKMGMT_TASKTYPE_ABRT_TASK_SET);
		break;
	default:
		pr_info("%s unsupported opcode(%d)\n",
		       ioc->name, opcode);
		break;
	};
	return strlen(buf);
}
static DEVICE_ATTR_WO(task_management);

static ssize_t
ioc_reset_count_show(
	struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, PAGE_SIZE, "%d\n", ioc->ioc_reset_count);
}
static DEVICE_ATTR_RO(ioc_reset_count);

static ssize_t
reply_queue_count_show(struct device *cdev,
				struct device_attribute *attr, char *buf)
{
	u8 reply_queue_count;
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	if ((ioc->facts.IOCCapabilities &
	     LEAPIORAID_IOCFACTS_CAPABILITY_MSI_X_INDEX) && ioc->msix_enable)
		reply_queue_count = ioc->reply_queue_count;
	else
		reply_queue_count = 1;
	return snprintf(buf, PAGE_SIZE, "%d\n", reply_queue_count);
}
static DEVICE_ATTR_RO(reply_queue_count);

static ssize_t
drv_support_bitmap_show(struct device *cdev,
			     struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, PAGE_SIZE, "0x%08x\n", ioc->drv_support_bitmap);
}
static DEVICE_ATTR_RO(drv_support_bitmap);

static ssize_t
enable_sdev_max_qd_show(struct device *cdev,
			     struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);

	return snprintf(buf, PAGE_SIZE, "%d\n", ioc->enable_sdev_max_qd);
}

static ssize_t
enable_sdev_max_qd_store(struct device *cdev,
			      struct device_attribute *attr, const char *buf,
			      size_t count)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct LEAPIORAID_ADAPTER *ioc = leapioraid_shost_private(shost);
	struct LEAPIORAID_DEVICE *sas_device_priv_data;
	struct LEAPIORAID_TARGET *sas_target_priv_data;
	int val = 0;
	struct scsi_device *sdev;
	struct leapioraid_raid_device *raid_device;
	int qdepth;

	if (kstrtoint(buf, 0, &val))
		return -EINVAL;
	switch (val) {
	case 0:
		ioc->enable_sdev_max_qd = 0;
		shost_for_each_device(sdev, ioc->shost) {
			sas_device_priv_data = sdev->hostdata;
			if (!sas_device_priv_data)
				continue;
			sas_target_priv_data = sas_device_priv_data->sas_target;
			if (!sas_target_priv_data)
				continue;
			if (sas_target_priv_data->flags & LEAPIORAID_TARGET_FLAGS_VOLUME) {
				raid_device =
				    leapioraid_raid_device_find_by_handle(ioc,
						sas_target_priv_data->handle);
				switch (raid_device->volume_type) {
				case LEAPIORAID_RAID_VOL_TYPE_RAID0:
					if (raid_device->device_info &
					    LEAPIORAID_SAS_DEVICE_INFO_SSP_TARGET)
						qdepth =
						    LEAPIORAID_SAS_QUEUE_DEPTH;
					else
						qdepth =
						    LEAPIORAID_SATA_QUEUE_DEPTH;
					break;
				case LEAPIORAID_RAID_VOL_TYPE_RAID1E:
				case LEAPIORAID_RAID_VOL_TYPE_RAID1:
				case LEAPIORAID_RAID_VOL_TYPE_RAID10:
				case LEAPIORAID_RAID_VOL_TYPE_UNKNOWN:
				default:
					qdepth = LEAPIORAID_RAID_QUEUE_DEPTH;
				}
			} else
				qdepth =
				    (sas_target_priv_data->sas_dev->port_type >
				     1) ? ioc->max_wideport_qd : ioc->max_narrowport_qd;
			leapioraid__scsihost_change_queue_depth(sdev, qdepth);
		}
		break;
	case 1:
		ioc->enable_sdev_max_qd = 1;
		shost_for_each_device(sdev, ioc->shost) {
			leapioraid__scsihost_change_queue_depth(sdev,
							       shost->can_queue);
		}
		break;
	default:
		return -EINVAL;
	}
	return strlen(buf);
}
static DEVICE_ATTR_RW(enable_sdev_max_qd);

static struct attribute *leapioraid_host_attrs[] = {
	&dev_attr_version_fw.attr,
	&dev_attr_version_bios.attr,
	&dev_attr_version_leapioraid.attr,
	&dev_attr_version_product.attr,
	&dev_attr_version_nvdata_persistent.attr,
	&dev_attr_version_nvdata_default.attr,
	&dev_attr_board_name.attr,
	&dev_attr_board_assembly.attr,
	&dev_attr_board_tracer.attr,
	&dev_attr_io_delay.attr,
	&dev_attr_device_delay.attr,
	&dev_attr_logging_level.attr,
	&dev_attr_fwfault_debug.attr,
	&dev_attr_fw_queue_depth.attr,
	&dev_attr_host_sas_address.attr,
	&dev_attr_task_management.attr,
	&dev_attr_ioc_reset_count.attr,
	&dev_attr_reply_queue_count.attr,
	&dev_attr_drv_support_bitmap.attr,
	&dev_attr_enable_sdev_max_qd.attr,
	NULL,
};

static const struct attribute_group leapioraid_host_attr_group = {
	.attrs = leapioraid_host_attrs
};

const struct attribute_group *leapioraid_host_groups[] = {
	&leapioraid_host_attr_group,
	NULL
};

static ssize_t
sas_address_show(
	struct device *dev, struct device_attribute *attr,
	char *buf)
{
	struct scsi_device *sdev = to_scsi_device(dev);
	struct LEAPIORAID_DEVICE *sas_device_priv_data = sdev->hostdata;

	return snprintf(
		buf, PAGE_SIZE, "0x%016llx\n",
		(unsigned long long)sas_device_priv_data->sas_target->sas_address);
}
static DEVICE_ATTR_RO(sas_address);

static ssize_t
sas_device_handle_show(
	struct device *dev, struct device_attribute *attr,
	char *buf)
{
	struct scsi_device *sdev = to_scsi_device(dev);
	struct LEAPIORAID_DEVICE *sas_device_priv_data = sdev->hostdata;

	return snprintf(buf, PAGE_SIZE, "0x%04x\n",
			sas_device_priv_data->sas_target->handle);
}
static DEVICE_ATTR_RO(sas_device_handle);

static ssize_t
sas_ncq_prio_enable_show(
	struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct scsi_device *sdev = to_scsi_device(dev);
	struct LEAPIORAID_DEVICE *sas_device_priv_data = sdev->hostdata;

	return snprintf(buf, PAGE_SIZE, "%d\n",
			sas_device_priv_data->ncq_prio_enable);
}

static ssize_t
sas_ncq_prio_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct scsi_device *sdev = to_scsi_device(dev);
	struct LEAPIORAID_DEVICE *sas_device_priv_data = sdev->hostdata;
	int ncq_prio_enable = 0;

	if (kstrtoint(buf, 0, &ncq_prio_enable))
		return -EINVAL;
	if (!leapioraid_scsihost_ncq_prio_supp(sdev))
		return -EINVAL;
	sas_device_priv_data->ncq_prio_enable = ncq_prio_enable;
	return strlen(buf);
}
static DEVICE_ATTR_RW(sas_ncq_prio_enable);

static struct attribute *leapioraid_dev_attrs[] = {
	&dev_attr_sas_address.attr,
	&dev_attr_sas_device_handle.attr,
	&dev_attr_sas_ncq_prio_enable.attr,
	NULL,
};
static const struct attribute_group leapioraid_dev_attr_group = {
	.attrs = leapioraid_dev_attrs
};
const struct attribute_group *leapioraid_dev_groups[] = {
	&leapioraid_dev_attr_group,
	NULL
};

static int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct LEAPIORAID_ADAPTER *ioc;
	unsigned long pfn;
	unsigned long length = vma->vm_end - vma->vm_start;

	ioc = list_first_entry(&leapioraid_ioc_list,
		 struct LEAPIORAID_ADAPTER, list);
	if (length > (SYS_LOG_BUF_SIZE + SYS_LOG_BUF_RESERVE)) {
		pr_err("Requested mapping size is too large\n");
		return -EINVAL;
	}
	if (ioc->log_buffer == NULL) {
		pr_err("no log buffer\n");
		return -EINVAL;
	}

	pfn = virt_to_phys(ioc->log_buffer) >> PAGE_SHIFT;

	if (remap_pfn_range(vma, vma->vm_start, pfn, length, vma->vm_page_prot)) {
		pr_err("Failed to map memory to user space\n");
		return -EAGAIN;
	}

	return 0;
}

static const struct
file_operations leapioraid_ctl_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = leapioraid_ctl_ioctl,
	.poll = leapioraid_ctl_poll,
	.fasync = leapioraid_ctl_fasync,
#ifdef CONFIG_COMPAT
	.compat_ioctl = leapioraid_ctl_ioctl_compat,
#endif
	.mmap = my_mmap,
};

static struct miscdevice leapioraid_ctl_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = LEAPIORAID_DEV_NAME,
	.fops = &leapioraid_ctl_fops,
};

void leapioraid_ctl_init(void)
{
	leapioraid_async_queue = NULL;
	if (misc_register(&leapioraid_ctl_dev) < 0)
		pr_err("%s can't register misc device\n",
		       LEAPIORAID_DRIVER_NAME);
	init_waitqueue_head(&leapioraid_ctl_poll_wait);
}

void leapioraid_ctl_exit(void)
{
	struct LEAPIORAID_ADAPTER *ioc;

	list_for_each_entry(ioc, &leapioraid_ioc_list, list) {
		kfree(ioc->event_log);
	}
	misc_deregister(&leapioraid_ctl_dev);
}
