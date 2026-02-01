// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 LeapIO Tech Inc.
 *
 * LeapRAID Storage and RAID Controller driver.
 */

#include <linux/module.h>

#include "leapraid_func.h"
#include "leapraid.h"

LIST_HEAD(leapraid_adapter_list);
DEFINE_SPINLOCK(leapraid_adapter_lock);

MODULE_AUTHOR(LEAPRAID_AUTHOR);
MODULE_DESCRIPTION(LEAPRAID_DESCRIPTION);
MODULE_LICENSE("GPL");
MODULE_VERSION(LEAPRAID_DRIVER_VERSION);

static int leapraid_ids;

static int open_pcie_trace = 1;
module_param(open_pcie_trace, int, 0644);
MODULE_PARM_DESC(open_pcie_trace, "open_pcie_trace: default=1(open)/0(close)");

static int enable_mp = 1;
module_param(enable_mp, int, 0444);
MODULE_PARM_DESC(enable_mp,
		 "enable multipath on target device. default=1(enable)");

static inline void leapraid_get_sense_data(char *sense,
					   struct sense_info *data)
{
	bool desc_format = (sense[0] & SCSI_SENSE_RESPONSE_CODE_MASK) >=
			    DESC_FORMAT_THRESHOLD;

	if (desc_format) {
		data->sense_key = sense[1] & SENSE_KEY_MASK;
		data->asc = sense[2];
		data->ascq = sense[3];
	} else {
		data->sense_key = sense[2] & SENSE_KEY_MASK;
		data->asc = sense[12];
		data->ascq = sense[13];
	}
}

static struct Scsi_Host *pdev_to_shost(struct pci_dev *pdev)
{
	return pci_get_drvdata(pdev);
}

static struct leapraid_adapter *pdev_to_adapter(struct pci_dev *pdev)
{
	struct Scsi_Host *shost = pdev_to_shost(pdev);

	if (!shost)
		return NULL;

	return shost_priv(shost);
}

struct leapraid_io_req_tracker *leapraid_get_scmd_priv(struct scsi_cmnd *scmd)
{
	return scsi_cmd_priv(scmd);
}

void leapraid_set_tm_flg(struct leapraid_adapter *adapter, u16 hdl)
{
	struct leapraid_sdev_priv *sdev_priv;
	struct scsi_device *sdev;
	bool skip = false;

	/* don't break out of the loop */
	shost_for_each_device(sdev, adapter->shost) {
		if (skip)
			continue;

		sdev_priv = sdev->hostdata;
		if (!sdev_priv)
			continue;

		if (sdev_priv->starget_priv->hdl == hdl) {
			sdev_priv->starget_priv->tm_busy = true;
			skip = true;
		}
	}
}

void leapraid_clear_tm_flg(struct leapraid_adapter *adapter, u16 hdl)
{
	struct leapraid_sdev_priv *sdev_priv;
	struct scsi_device *sdev;
	bool skip = false;

	/* don't break out of the loop */
	shost_for_each_device(sdev, adapter->shost) {
		if (skip)
			continue;

		sdev_priv = sdev->hostdata;
		if (!sdev_priv)
			continue;

		if (sdev_priv->starget_priv->hdl == hdl) {
			sdev_priv->starget_priv->tm_busy = false;
			skip = true;
		}
	}
}

static int leapraid_tm_cmd_map_status(struct leapraid_adapter *adapter,
				      uint channel,
				      uint id,
				      uint lun,
				      u8 type,
				      u16 taskid_task)
{
	int rc = FAILED;

	if (taskid_task <= adapter->shost->can_queue) {
		switch (type) {
		case LEAPRAID_TM_TASKTYPE_ABRT_TASK_SET:
		case LEAPRAID_TM_TASKTYPE_LOGICAL_UNIT_RESET:
			if (!leapraid_scmd_find_by_lun(adapter, id, lun,
						       channel))
				rc = SUCCESS;
			break;
		case LEAPRAID_TM_TASKTYPE_TARGET_RESET:
			if (!leapraid_scmd_find_by_tgt(adapter, id, channel))
				rc = SUCCESS;
			break;
		default:
			rc = SUCCESS;
		}
	}

	if (taskid_task == adapter->driver_cmds.driver_scsiio_cmd.taskid) {
		if ((adapter->driver_cmds.driver_scsiio_cmd.status &
		     LEAPRAID_CMD_DONE) ||
		    (adapter->driver_cmds.driver_scsiio_cmd.status &
		     LEAPRAID_CMD_NOT_USED))
			rc = SUCCESS;
	}

	if (taskid_task == adapter->driver_cmds.ctl_cmd.hp_taskid) {
		if ((adapter->driver_cmds.ctl_cmd.status &
		     LEAPRAID_CMD_DONE) ||
		    (adapter->driver_cmds.ctl_cmd.status &
		     LEAPRAID_CMD_NOT_USED))
			rc = SUCCESS;
	}

	return rc;
}

static int leapraid_tm_post_processing(struct leapraid_adapter *adapter,
				       u16 hdl, uint channel, uint id,
				       uint lun, u8 type, u16 taskid_task)
{
	int rc;

	rc = leapraid_tm_cmd_map_status(adapter, channel, id, lun,
					type, taskid_task);
	if (rc == SUCCESS)
		return rc;

	leapraid_mask_int(adapter);
	leapraid_sync_irqs(adapter, true);
	leapraid_unmask_int(adapter);

	rc = leapraid_tm_cmd_map_status(adapter, channel, id, lun, type,
					taskid_task);
	return rc;
}

static void leapraid_build_tm_req(struct leapraid_scsi_tm_req *scsi_tm_req,
				  u16 hdl, uint lun, u8 type, u8 tr_method,
				  u16 target_taskid)
{
	memset(scsi_tm_req, 0, sizeof(*scsi_tm_req));
	scsi_tm_req->func = LEAPRAID_FUNC_SCSI_TMF;
	scsi_tm_req->dev_hdl = cpu_to_le16(hdl);
	scsi_tm_req->task_type = type;
	scsi_tm_req->msg_flg = tr_method;
	if (type == LEAPRAID_TM_TASKTYPE_ABORT_TASK ||
	    type == LEAPRAID_TM_TASKTYPE_QUERY_TASK)
		scsi_tm_req->task_mid = cpu_to_le16(target_taskid);
	int_to_scsilun(lun, (struct scsi_lun *)scsi_tm_req->lun);
}

int leapraid_issue_tm(struct leapraid_adapter *adapter, u16 hdl, uint channel,
		      uint id, uint lun, u8 type,
		      u16 target_taskid, u8 tr_method)
{
	struct leapraid_scsi_tm_req *scsi_tm_req;
	struct leapraid_scsiio_req *scsiio_req;
	struct leapraid_io_req_tracker *io_req_tracker = NULL;
	u16 msix_task = 0;
	bool issue_reset = false;
	u32 db;
	int rc;

	lockdep_assert_held(&adapter->driver_cmds.tm_cmd.mutex);

	if (adapter->access_ctrl.shost_recovering ||
	    adapter->access_ctrl.host_removing ||
	    adapter->access_ctrl.pcie_recovering) {
		dev_info(&adapter->pdev->dev,
			 "%s %s: host is recovering, skip tm command!\n",
			 __func__, adapter->adapter_attr.name);
		return FAILED;
	}

	db = leapraid_readl(&adapter->iomem_base->db);
	if (db & LEAPRAID_DB_USED) {
		dev_info(&adapter->pdev->dev,
			 "%s unexpected db status, issuing hard reset!\n",
			 adapter->adapter_attr.name);
		dev_info(&adapter->pdev->dev, "%s:%d call hard_reset\n",
			 __func__, __LINE__);
		rc = leapraid_hard_reset_handler(adapter, FULL_RESET);
		return (!rc) ? SUCCESS : FAILED;
	}

	if ((db & LEAPRAID_DB_MASK) == LEAPRAID_DB_FAULT) {
		dev_info(&adapter->pdev->dev, "%s:%d call hard_reset\n",
			 __func__, __LINE__);
		rc = leapraid_hard_reset_handler(adapter, FULL_RESET);
		return (!rc) ? SUCCESS : FAILED;
	}

	if (type == LEAPRAID_TM_TASKTYPE_ABORT_TASK)
		io_req_tracker = leapraid_get_io_tracker_from_taskid(adapter,
								     target_taskid);

	adapter->driver_cmds.tm_cmd.status = LEAPRAID_CMD_PENDING;
	scsi_tm_req =
		leapraid_get_task_desc(adapter,
				       adapter->driver_cmds.tm_cmd.hp_taskid);
	leapraid_build_tm_req(scsi_tm_req, hdl, lun, type, tr_method,
			      target_taskid);
	memset((void *)(&adapter->driver_cmds.tm_cmd.reply), 0,
	       sizeof(struct leapraid_scsi_tm_rep));
	leapraid_set_tm_flg(adapter, hdl);
	init_completion(&adapter->driver_cmds.tm_cmd.done);
	if (type == LEAPRAID_TM_TASKTYPE_ABORT_TASK &&
	    io_req_tracker &&
	    io_req_tracker->msix_io < adapter->adapter_attr.rq_cnt)
		msix_task = io_req_tracker->msix_io;
	else
		msix_task = 0;
	leapraid_fire_hpr_task(adapter,
			       adapter->driver_cmds.tm_cmd.hp_taskid,
			       msix_task);
	wait_for_completion_timeout(&adapter->driver_cmds.tm_cmd.done,
				    LEAPRAID_TM_CMD_TIMEOUT * HZ);
	if (!(adapter->driver_cmds.tm_cmd.status & LEAPRAID_CMD_DONE)) {
		issue_reset =
			leapraid_check_reset(
				adapter->driver_cmds.tm_cmd.status);
		if (issue_reset) {
			dev_info(&adapter->pdev->dev,
				 "%s:%d call hard_reset\n",
				 __func__, __LINE__);
			rc = leapraid_hard_reset_handler(adapter, FULL_RESET);
			rc = (!rc) ? SUCCESS : FAILED;
			goto out;
		}
	}

	leapraid_sync_irqs(adapter, false);

	switch (type) {
	case LEAPRAID_TM_TASKTYPE_TARGET_RESET:
	case LEAPRAID_TM_TASKTYPE_ABRT_TASK_SET:
	case LEAPRAID_TM_TASKTYPE_LOGICAL_UNIT_RESET:
		rc = leapraid_tm_post_processing(adapter, hdl, channel, id, lun,
						 type, target_taskid);
		break;
	case LEAPRAID_TM_TASKTYPE_ABORT_TASK:
		rc = SUCCESS;
		scsiio_req = leapraid_get_task_desc(adapter, target_taskid);
		if (le16_to_cpu(scsiio_req->dev_hdl) != hdl)
			break;
		dev_err(&adapter->pdev->dev, "%s abort failed, hdl=0x%04x\n",
			adapter->adapter_attr.name, hdl);
		rc = FAILED;
		break;
	case LEAPRAID_TM_TASKTYPE_QUERY_TASK:
		rc = SUCCESS;
		break;
	default:
		rc = FAILED;
		break;
	}

out:
	leapraid_clear_tm_flg(adapter, hdl);
	adapter->driver_cmds.tm_cmd.status = LEAPRAID_CMD_NOT_USED;
	return rc;
}

int leapraid_issue_locked_tm(struct leapraid_adapter *adapter, u16 hdl,
			     uint channel, uint id, uint lun, u8 type,
			     u16 target_taskid, u8 tr_method)
{
	int rc;

	mutex_lock(&adapter->driver_cmds.tm_cmd.mutex);
	rc = leapraid_issue_tm(adapter, hdl, channel, id, lun, type,
			       target_taskid, tr_method);
	mutex_unlock(&adapter->driver_cmds.tm_cmd.mutex);

	return rc;
}

void leapraid_smart_fault_detect(struct leapraid_adapter *adapter, u16 hdl)
{
	struct leapraid_starget_priv *starget_priv;
	struct leapraid_sas_dev *sas_dev;
	struct scsi_target *starget;
	unsigned long flags;

	spin_lock_irqsave(&adapter->dev_topo.sas_dev_lock, flags);
	sas_dev = leapraid_hold_lock_get_sas_dev_by_hdl(adapter, hdl);
	if (!sas_dev) {
		spin_unlock_irqrestore(&adapter->dev_topo.sas_dev_lock, flags);
		goto out;
	}

	starget = sas_dev->starget;
	starget_priv = starget->hostdata;
	if ((starget_priv->flg & LEAPRAID_TGT_FLG_RAID_MEMBER) ||
	    (starget_priv->flg & LEAPRAID_TGT_FLG_VOLUME)) {
		spin_unlock_irqrestore(&adapter->dev_topo.sas_dev_lock, flags);
		goto out;
	}

	spin_unlock_irqrestore(&adapter->dev_topo.sas_dev_lock, flags);
	leapraid_async_turn_on_led(adapter, hdl);
out:
	if (sas_dev)
		leapraid_sdev_put(sas_dev);
}

static void leapraid_process_sense_data(struct leapraid_adapter *adapter,
					struct leapraid_scsiio_rep *scsiio_rep,
					struct scsi_cmnd *scmd, u16 taskid)
{
	struct sense_info data;
	const void *sense_data;
	u32 sz;

	if (!(scsiio_rep->scsi_state & LEAPRAID_SCSI_STATE_AUTOSENSE_VALID))
		return;

	sense_data = leapraid_get_sense_buffer(adapter, taskid);
	sz = min_t(u32, SCSI_SENSE_BUFFERSIZE,
		   le32_to_cpu(scsiio_rep->sense_count));

	memcpy(scmd->sense_buffer, sense_data, sz);
	leapraid_get_sense_data(scmd->sense_buffer, &data);
	if (data.asc == ASC_FAILURE_PREDICTION_THRESHOLD_EXCEEDED)
		leapraid_smart_fault_detect(adapter,
					    le16_to_cpu(scsiio_rep->dev_hdl));
}

static void leapraid_handle_data_underrun(
		struct leapraid_scsiio_rep *scsiio_rep,
		struct scsi_cmnd *scmd, u32 xfer_cnt)
{
	u8 scsi_status = scsiio_rep->scsi_status;
	u8 scsi_state = scsiio_rep->scsi_state;

	scmd->result = (DID_OK << LEAPRAID_SCSI_HOST_SHIFT) | scsi_status;

	if (scsi_state & LEAPRAID_SCSI_STATE_AUTOSENSE_VALID)
		return;

	if (xfer_cnt < scmd->underflow) {
		if (scsi_status == SAM_STAT_BUSY)
			scmd->result = SAM_STAT_BUSY;
		else
			scmd->result = DID_SOFT_ERROR <<
				LEAPRAID_SCSI_HOST_SHIFT;
	} else if (scsi_state & (LEAPRAID_SCSI_STATE_AUTOSENSE_FAILED |
				 LEAPRAID_SCSI_STATE_NO_SCSI_STATUS)) {
		scmd->result = DID_SOFT_ERROR << LEAPRAID_SCSI_HOST_SHIFT;
	} else if (scsi_state & LEAPRAID_SCSI_STATE_TERMINATED) {
		scmd->result = DID_RESET << LEAPRAID_SCSI_HOST_SHIFT;
	} else if (!xfer_cnt && scmd->cmnd[0] == REPORT_LUNS) {
		scsiio_rep->scsi_state = LEAPRAID_SCSI_STATE_AUTOSENSE_VALID;
		scsiio_rep->scsi_status = SAM_STAT_CHECK_CONDITION;
		scsi_build_sense(scmd, 0, ILLEGAL_REQUEST,
				 LEAPRAID_SCSI_ASC_INVALID_CMD_CODE,
				 LEAPRAID_SCSI_ASCQ_DEFAULT);
	}
}

static void leapraid_handle_success_status(
		struct leapraid_scsiio_rep *scsiio_rep,
		struct scsi_cmnd *scmd,
		u32 response_code)
{
	u8 scsi_status = scsiio_rep->scsi_status;
	u8 scsi_state = scsiio_rep->scsi_state;

	scmd->result = (DID_OK << LEAPRAID_SCSI_HOST_SHIFT) | scsi_status;

	if (response_code == LEAPRAID_TM_RSP_INVALID_FRAME ||
	    (scsi_state & (LEAPRAID_SCSI_STATE_AUTOSENSE_FAILED |
			   LEAPRAID_SCSI_STATE_NO_SCSI_STATUS)))
		scmd->result = DID_SOFT_ERROR << LEAPRAID_SCSI_HOST_SHIFT;
	else if (scsi_state & LEAPRAID_SCSI_STATE_TERMINATED)
		scmd->result = DID_RESET << LEAPRAID_SCSI_HOST_SHIFT;
}

static void leapraid_scsiio_done_dispatch(struct leapraid_adapter *adapter,
		struct leapraid_scsiio_rep *scsiio_rep,
		struct leapraid_sdev_priv *sdev_priv,
		struct scsi_cmnd *scmd,
		u16 taskid, u32 response_code)
{
	u8 scsi_status = scsiio_rep->scsi_status;
	u8 scsi_state = scsiio_rep->scsi_state;
	u16 adapter_status;
	u32 xfer_cnt;
	u32 sz;

	adapter_status = le16_to_cpu(scsiio_rep->adapter_status) &
				     LEAPRAID_ADAPTER_STATUS_MASK;

	xfer_cnt = le32_to_cpu(scsiio_rep->transfer_count);
	scsi_set_resid(scmd, scsi_bufflen(scmd) - xfer_cnt);

	if (adapter_status == LEAPRAID_ADAPTER_STATUS_SCSI_DATA_UNDERRUN &&
	    xfer_cnt == 0 &&
	    (scsi_status == LEAPRAID_SCSI_STATUS_BUSY ||
	    scsi_status == LEAPRAID_SCSI_STATUS_RESERVATION_CONFLICT ||
	    scsi_status == LEAPRAID_SCSI_STATUS_TASK_SET_FULL)) {
		adapter_status = LEAPRAID_ADAPTER_STATUS_SUCCESS;
	}

	switch (adapter_status) {
	case LEAPRAID_ADAPTER_STATUS_SCSI_DEVICE_NOT_THERE:
		scmd->result = DID_NO_CONNECT << LEAPRAID_SCSI_HOST_SHIFT;
		break;

	case LEAPRAID_ADAPTER_STATUS_BUSY:
	case LEAPRAID_ADAPTER_STATUS_INSUFFICIENT_RESOURCES:
		scmd->result = SAM_STAT_BUSY;
		break;

	case LEAPRAID_ADAPTER_STATUS_SCSI_RESIDUAL_MISMATCH:
		if (xfer_cnt == 0 || scmd->underflow > xfer_cnt)
			scmd->result = DID_SOFT_ERROR <<
				LEAPRAID_SCSI_HOST_SHIFT;
		else
			scmd->result = (DID_OK << LEAPRAID_SCSI_HOST_SHIFT) |
				scsi_status;
		break;

	case LEAPRAID_ADAPTER_STATUS_SCSI_ADAPTER_TERMINATED:
		if (sdev_priv->block) {
			scmd->result = DID_TRANSPORT_DISRUPTED <<
				LEAPRAID_SCSI_HOST_SHIFT;
			return;
		}

		if (scmd->device->channel == RAID_CHANNEL &&
		    scsi_state == (LEAPRAID_SCSI_STATE_TERMINATED |
				    LEAPRAID_SCSI_STATE_NO_SCSI_STATUS)) {
			scmd->result = DID_RESET << LEAPRAID_SCSI_HOST_SHIFT;
			break;
		}

		scmd->result = DID_SOFT_ERROR << LEAPRAID_SCSI_HOST_SHIFT;
		break;

	case LEAPRAID_ADAPTER_STATUS_SCSI_TASK_TERMINATED:
	case LEAPRAID_ADAPTER_STATUS_SCSI_EXT_TERMINATED:
		scmd->result = DID_RESET << LEAPRAID_SCSI_HOST_SHIFT;
		break;

	case LEAPRAID_ADAPTER_STATUS_SCSI_DATA_UNDERRUN:
		leapraid_handle_data_underrun(scsiio_rep, scmd, xfer_cnt);
		break;

	case LEAPRAID_ADAPTER_STATUS_SCSI_DATA_OVERRUN:
		scsi_set_resid(scmd, 0);
		leapraid_handle_success_status(scsiio_rep, scmd,
					       response_code);
		break;
	case LEAPRAID_ADAPTER_STATUS_SCSI_RECOVERED_ERROR:
	case LEAPRAID_ADAPTER_STATUS_SUCCESS:
		leapraid_handle_success_status(scsiio_rep, scmd,
					       response_code);
		break;

	case LEAPRAID_ADAPTER_STATUS_SCSI_PROTOCOL_ERROR:
	case LEAPRAID_ADAPTER_STATUS_INTERNAL_ERROR:
	case LEAPRAID_ADAPTER_STATUS_SCSI_IO_DATA_ERROR:
	case LEAPRAID_ADAPTER_STATUS_SCSI_TASK_MGMT_FAILED:
	default:
		scmd->result = DID_SOFT_ERROR << LEAPRAID_SCSI_HOST_SHIFT;
		break;
	}

	if (!scmd->result)
		return;

	scsi_print_command(scmd);
	dev_warn(&adapter->pdev->dev,
		 "scsiio warn: hdl=0x%x, status are: 0x%x, 0x%x, 0x%x\n",
		 le16_to_cpu(scsiio_rep->dev_hdl), adapter_status,
		 scsi_status, scsi_state);

	if (scsi_state & LEAPRAID_SCSI_STATE_AUTOSENSE_VALID) {
		struct scsi_sense_hdr sshdr;

		sz = min_t(u32, SCSI_SENSE_BUFFERSIZE,
			   le32_to_cpu(scsiio_rep->sense_count));
		if (scsi_normalize_sense(scmd->sense_buffer, sz,
					 &sshdr)) {
			dev_warn(&adapter->pdev->dev,
				 "sense: key=0x%x asc=0x%x ascq=0x%x\n",
				 sshdr.sense_key, sshdr.asc,
				 sshdr.ascq);
		} else {
			dev_warn(&adapter->pdev->dev,
				 "sense: invalid sense data\n");
		}
	}
}

u8 leapraid_scsiio_done(struct leapraid_adapter *adapter, u16 taskid,
			u8 msix_index, u32 rep)
{
	struct leapraid_scsiio_rep *scsiio_rep = NULL;
	struct leapraid_sdev_priv *sdev_priv = NULL;
	struct scsi_cmnd *scmd = NULL;
	u32 response_code = 0;

	if (likely(taskid != adapter->driver_cmds.driver_scsiio_cmd.taskid))
		scmd = leapraid_get_scmd_from_taskid(adapter, taskid);
	else
		scmd = adapter->driver_cmds.internal_scmd;
	if (!scmd)
		return 1;

	scsiio_rep = leapraid_get_reply_vaddr(adapter, rep);
	if (!scsiio_rep) {
		scmd->result = DID_OK << LEAPRAID_SCSI_HOST_SHIFT;
		goto out;
	}

	sdev_priv = scmd->device->hostdata;
	if (!sdev_priv ||
	    !sdev_priv->starget_priv ||
	    sdev_priv->starget_priv->deleted) {
		scmd->result = DID_NO_CONNECT << LEAPRAID_SCSI_HOST_SHIFT;
		goto out;
	}

	if (scsiio_rep->scsi_state & LEAPRAID_SCSI_STATE_RESPONSE_INFO_VALID)
		response_code = le32_to_cpu(scsiio_rep->resp_info) & 0xFF;

	leapraid_process_sense_data(adapter, scsiio_rep, scmd, taskid);
	leapraid_scsiio_done_dispatch(adapter, scsiio_rep, sdev_priv, scmd,
				      taskid, response_code);

out:
	scsi_dma_unmap(scmd);
	if (unlikely(taskid == adapter->driver_cmds.driver_scsiio_cmd.taskid)) {
		adapter->driver_cmds.driver_scsiio_cmd.status =
			LEAPRAID_CMD_DONE;
		complete(&adapter->driver_cmds.driver_scsiio_cmd.done);
		return 0;
	}
	leapraid_free_taskid(adapter, taskid);
	scsi_done(scmd);
	return 0;
}

static void leapraid_probe_raid(struct leapraid_adapter *adapter)
{
	struct leapraid_raid_volume *raid_volume, *raid_volume_next;
	int rc;

	list_for_each_entry_safe(raid_volume, raid_volume_next,
				 &adapter->dev_topo.raid_volume_list, list) {
		if (raid_volume->starget)
			continue;

		rc = scsi_add_device(adapter->shost, RAID_CHANNEL,
				     raid_volume->id, 0);
		if (rc)
			leapraid_raid_volume_remove(adapter, raid_volume);
	}
}

static void leapraid_sas_dev_make_active(struct leapraid_adapter *adapter,
					 struct leapraid_sas_dev *sas_dev)
{
	unsigned long flags;

	spin_lock_irqsave(&adapter->dev_topo.sas_dev_lock, flags);
	if (!list_empty(&sas_dev->list)) {
		list_del_init(&sas_dev->list);
		leapraid_sdev_put(sas_dev);
	}

	leapraid_sdev_get(sas_dev);
	list_add_tail(&sas_dev->list, &adapter->dev_topo.sas_dev_list);
	spin_unlock_irqrestore(&adapter->dev_topo.sas_dev_lock, flags);
}

static void leapraid_probe_sas(struct leapraid_adapter *adapter)
{
	struct leapraid_sas_dev *sas_dev;
	bool added;

	for (;;) {
		sas_dev = leapraid_get_next_sas_dev_from_init_list(adapter);
		if (!sas_dev)
			break;

		added = leapraid_transport_port_add(adapter,
						    sas_dev->hdl,
						    sas_dev->parent_sas_addr,
						    sas_dev->card_port);

		if (!added)
			goto remove_dev;

		if (!sas_dev->starget &&
		    !adapter->scan_dev_desc.driver_loading) {
			leapraid_transport_port_remove(adapter,
						       sas_dev->sas_addr,
						       sas_dev->parent_sas_addr,
						       sas_dev->card_port);
			goto remove_dev;
		}

		leapraid_sas_dev_make_active(adapter, sas_dev);
		leapraid_sdev_put(sas_dev);
		continue;

remove_dev:
		leapraid_sas_dev_remove(adapter, sas_dev);
		leapraid_sdev_put(sas_dev);
	}
}

static bool leapraid_get_boot_dev(struct leapraid_boot_dev *boot_dev,
				  void **pdev, u32 *pchnl)
{
	if (boot_dev->dev) {
		*pdev = boot_dev->dev;
		*pchnl = boot_dev->chnl;
		return true;
	}
	return false;
}

static void leapraid_probe_boot_dev(struct leapraid_adapter *adapter)
{
	void *dev = NULL;
	u32 chnl;

	if (leapraid_get_boot_dev(&adapter->boot_devs.requested_boot_dev, &dev,
				  &chnl))
		goto boot_dev_found;

	if (leapraid_get_boot_dev(&adapter->boot_devs.requested_alt_boot_dev,
				  &dev, &chnl))
		goto boot_dev_found;

	if (leapraid_get_boot_dev(&adapter->boot_devs.current_boot_dev, &dev,
				  &chnl))
		goto boot_dev_found;

	return;

boot_dev_found:
	switch (chnl) {
	case RAID_CHANNEL:
	{
		struct leapraid_raid_volume *raid_volume =
			(struct leapraid_raid_volume *)dev;

		if (raid_volume->starget)
			return;

		/* TODO eedp */

		if (scsi_add_device(adapter->shost, RAID_CHANNEL,
				    raid_volume->id, 0))
			leapraid_raid_volume_remove(adapter, raid_volume);
		break;
	}
	default:
	{
		struct leapraid_sas_dev *sas_dev =
			(struct leapraid_sas_dev *)dev;
		struct leapraid_sas_port *sas_port;
		unsigned long flags;

		if (sas_dev->starget)
			return;

		spin_lock_irqsave(&adapter->dev_topo.sas_dev_lock, flags);
		list_move_tail(&sas_dev->list,
			       &adapter->dev_topo.sas_dev_list);
		spin_unlock_irqrestore(&adapter->dev_topo.sas_dev_lock, flags);

		if (!sas_dev->card_port)
			return;

		sas_port = leapraid_transport_port_add(adapter, sas_dev->hdl,
						       sas_dev->parent_sas_addr,
						       sas_dev->card_port);
		if (!sas_port)
			leapraid_sas_dev_remove(adapter, sas_dev);
		break;
	}
	}
}

static void leapraid_probe_devices(struct leapraid_adapter *adapter)
{
	leapraid_probe_boot_dev(adapter);

	if (adapter->adapter_attr.raid_support) {
		leapraid_probe_raid(adapter);
		leapraid_probe_sas(adapter);
	} else {
		leapraid_probe_sas(adapter);
	}
}

void leapraid_scan_dev_done(struct leapraid_adapter *adapter)
{
	if (adapter->scan_dev_desc.wait_scan_dev_done) {
		adapter->scan_dev_desc.wait_scan_dev_done = false;
		leapraid_probe_devices(adapter);
	}

	leapraid_check_scheduled_fault_start(adapter);
	leapraid_fw_log_start(adapter);
	adapter->scan_dev_desc.driver_loading = false;
	leapraid_smart_polling_start(adapter);
}

static void leapraid_ir_shutdown(struct leapraid_adapter *adapter)
{
	struct leapraid_raid_act_req *raid_act_req;
	struct leapraid_raid_act_rep *raid_act_rep;
	struct leapraid_driver_cmd *raid_action_cmd;

	if (!adapter || !adapter->adapter_attr.raid_support)
		return;

	if (list_empty(&adapter->dev_topo.raid_volume_list))
		return;

	if (leapraid_pci_removed(adapter))
		return;

	raid_action_cmd = &adapter->driver_cmds.raid_action_cmd;

	mutex_lock(&raid_action_cmd->mutex);
	raid_action_cmd->status = LEAPRAID_CMD_PENDING;

	raid_act_req = leapraid_get_task_desc(adapter,
					      raid_action_cmd->inter_taskid);
	memset(raid_act_req, 0, sizeof(struct leapraid_raid_act_req));
	raid_act_req->func = LEAPRAID_FUNC_RAID_ACTION;
	raid_act_req->act = LEAPRAID_RAID_ACT_SYSTEM_SHUTDOWN_INITIATED;

	dev_info(&adapter->pdev->dev, "ir shutdown start\n");
	init_completion(&raid_action_cmd->done);
	leapraid_fire_task(adapter, raid_action_cmd->inter_taskid);
	wait_for_completion_timeout(&raid_action_cmd->done,
				    LEAPRAID_RAID_ACTION_CMD_TIMEOUT * HZ);

	if (!(raid_action_cmd->status & LEAPRAID_CMD_DONE)) {
		dev_err(&adapter->pdev->dev,
			"%s: timeout waiting for ir shutdown\n", __func__);
		goto out;
	}

	if (raid_action_cmd->status & LEAPRAID_CMD_REPLY_VALID) {
		raid_act_rep = (void *)(&raid_action_cmd->reply);
		dev_info(&adapter->pdev->dev,
			 "ir shutdown done, adapter status=0x%04x\n",
			 le16_to_cpu(raid_act_rep->adapter_status));
	}

out:
	raid_action_cmd->status = LEAPRAID_CMD_NOT_USED;
	mutex_unlock(&raid_action_cmd->mutex);
}

static const struct pci_device_id leapraid_pci_table[] = {
	{ PCI_DEVICE(LEAPRAID_VENDOR_ID, LEAPRAID_DEVID_HBA) },
	{ PCI_DEVICE(LEAPRAID_VENDOR_ID, LEAPRAID_DEVID_RAID) },
	{ 0, }
};

static inline bool leapraid_is_scmd_permitted(struct leapraid_adapter *adapter,
					      struct scsi_cmnd *scmd)
{
	u8 opcode;

	if (adapter->access_ctrl.pcie_recovering ||
	    adapter->access_ctrl.adapter_thermal_alert)
		return false;

	if (adapter->access_ctrl.host_removing) {
		if (leapraid_pci_removed(adapter))
			return false;

		opcode = scmd->cmnd[0];
		if (opcode == SYNCHRONIZE_CACHE || opcode == START_STOP)
			return true;
		else
			return false;
	}
	return true;
}

static bool leapraid_should_queuecommand(struct leapraid_adapter *adapter,
					 struct leapraid_sdev_priv *sdev_priv,
					 struct scsi_cmnd *scmd, int *rc)
{
	struct leapraid_starget_priv *starget_priv;

	if (!sdev_priv || !sdev_priv->starget_priv)
		goto no_connect;

	if (!leapraid_is_scmd_permitted(adapter, scmd))
		goto no_connect;

	starget_priv = sdev_priv->starget_priv;
	if (starget_priv->hdl == LEAPRAID_INVALID_DEV_HANDLE)
		goto no_connect;

	if (sdev_priv->block &&
	    scmd->device->host->shost_state == SHOST_RECOVERY &&
	    scmd->cmnd[0] == TEST_UNIT_READY) {
		scsi_build_sense(scmd, 0, UNIT_ATTENTION,
				 LEAPRAID_SCSI_ASC_POWER_ON_RESET,
				 LEAPRAID_SCSI_ASCQ_POWER_ON_RESET);
		goto done_out;
	}

	if (adapter->access_ctrl.shost_recovering ||
	    adapter->reset_desc.adapter_link_resetting) {
		*rc = SCSI_MLQUEUE_HOST_BUSY;
		goto out;
	} else if (starget_priv->deleted || sdev_priv->deleted) {
		goto no_connect;
	} else if (starget_priv->tm_busy || sdev_priv->block) {
		*rc = SCSI_MLQUEUE_DEVICE_BUSY;
		goto out;
	}

	return true;

no_connect:
	scmd->result = DID_NO_CONNECT << LEAPRAID_SCSI_HOST_SHIFT;
done_out:
	if (likely(scmd != adapter->driver_cmds.internal_scmd))
		scsi_done(scmd);
out:
	return false;
}

static u32 build_scsiio_req_control(struct scsi_cmnd *scmd,
				    struct leapraid_sdev_priv *sdev_priv)
{
	u32 control;

	switch (scmd->sc_data_direction) {
	case DMA_FROM_DEVICE:
		control = LEAPRAID_SCSIIO_CTRL_READ;
		break;
	case DMA_TO_DEVICE:
		control = LEAPRAID_SCSIIO_CTRL_WRITE;
		break;
	default:
		control = LEAPRAID_SCSIIO_CTRL_NODATATRANSFER;
		break;
	}

	control |= LEAPRAID_SCSIIO_CTRL_SIMPLEQ;

	if (sdev_priv->ncq &&
	    (IOPRIO_PRIO_CLASS(req_get_ioprio(scsi_cmd_to_rq(scmd))) ==
	     IOPRIO_CLASS_RT))
		control |= LEAPRAID_SCSIIO_CTRL_CMDPRI;
	if (scmd->cmd_len == 32)
		control |= 4 << LEAPRAID_SCSIIO_CTRL_ADDCDBLEN_SHIFT;

	return control;
}

int leapraid_queuecommand(struct Scsi_Host *shost, struct scsi_cmnd *scmd)
{
	struct leapraid_adapter *adapter = shost_priv(scmd->device->host);
	struct leapraid_sdev_priv *sdev_priv = scmd->device->hostdata;
	struct leapraid_starget_priv *starget_priv;
	struct leapraid_scsiio_req *scsiio_req;
	u32 control;
	u16 taskid;
	u16 hdl;
	int rc = 0;

	if (!leapraid_should_queuecommand(adapter, sdev_priv, scmd, &rc))
		goto out;

	starget_priv = sdev_priv->starget_priv;
	hdl = starget_priv->hdl;
	control = build_scsiio_req_control(scmd, sdev_priv);

	if (unlikely(scmd == adapter->driver_cmds.internal_scmd))
		taskid = adapter->driver_cmds.driver_scsiio_cmd.taskid;
	else
		taskid = leapraid_alloc_scsiio_taskid(adapter, scmd);
	scsiio_req = leapraid_get_task_desc(adapter, taskid);

	scsiio_req->func = LEAPRAID_FUNC_SCSIIO_REQ;
	if (sdev_priv->starget_priv->flg & LEAPRAID_TGT_FLG_RAID_MEMBER)
		scsiio_req->func = LEAPRAID_FUNC_RAID_SCSIIO_PASSTHROUGH;
	else
		scsiio_req->func = LEAPRAID_FUNC_SCSIIO_REQ;

	scsiio_req->dev_hdl = cpu_to_le16(hdl);
	scsiio_req->data_len = cpu_to_le32(scsi_bufflen(scmd));
	scsiio_req->ctrl = cpu_to_le32(control);
	scsiio_req->io_flg = cpu_to_le16(scmd->cmd_len);
	scsiio_req->msg_flg = 0;
	scsiio_req->sense_buffer_len = SCSI_SENSE_BUFFERSIZE;
	scsiio_req->sense_buffer_low_add =
		leapraid_get_sense_buffer_dma(adapter, taskid);
	scsiio_req->sgl_offset0 =
		offsetof(struct leapraid_scsiio_req, sgl) /
		LEAPRAID_DWORDS_BYTE_SIZE;
	int_to_scsilun(sdev_priv->lun, (struct scsi_lun *)scsiio_req->lun);
	memcpy(scsiio_req->cdb.cdb32, scmd->cmnd, scmd->cmd_len);
	if (scsiio_req->data_len) {
		if (leapraid_build_scmd_ieee_sg(adapter, scmd, taskid)) {
			leapraid_free_taskid(adapter, taskid);
			rc = SCSI_MLQUEUE_HOST_BUSY;
			goto out;
		}
	} else {
		leapraid_build_ieee_nodata_sg(adapter, &scsiio_req->sgl);
	}

	if (likely(scsiio_req->func == LEAPRAID_FUNC_SCSIIO_REQ)) {
		leapraid_fire_scsi_io(adapter, taskid,
			 le16_to_cpu(scsiio_req->dev_hdl));
	} else {
		leapraid_fire_task(adapter, taskid);
	}
	dev_dbg(&adapter->pdev->dev,
		"LEAPRAID_SCSIIO: Send Descriptor taskid %d, req type 0x%x\n",
		taskid, scsiio_req->func);
out:
	return rc;
}

static int leapraid_init_cmd_priv(struct Scsi_Host *shost,
				  struct scsi_cmnd *scmd)
{
	struct leapraid_adapter *adapter = shost_priv(shost);
	struct leapraid_io_req_tracker *io_tracker;

	io_tracker = leapraid_get_scmd_priv(scmd);
	leapraid_internal_init_cmd_priv(adapter, io_tracker);

	return 0;
}

static int leapraid_exit_cmd_priv(struct Scsi_Host *shost,
				  struct scsi_cmnd *scmd)
{
	struct leapraid_adapter *adapter = shost_priv(shost);
	struct leapraid_io_req_tracker *io_tracker;

	io_tracker = leapraid_get_scmd_priv(scmd);
	leapraid_internal_exit_cmd_priv(adapter, io_tracker);

	return 0;
}

static int leapraid_error_handler(struct scsi_cmnd *scmd,
				  const char *str, u8 type)
{
	struct leapraid_adapter *adapter = shost_priv(scmd->device->host);
	struct scsi_target *starget = scmd->device->sdev_target;
	struct leapraid_starget_priv *starget_priv = starget->hostdata;
	struct leapraid_io_req_tracker *io_req_tracker = NULL;
	struct leapraid_sdev_priv *sdev_priv;
	struct leapraid_sas_dev *sas_dev = NULL;
	u16 hdl;
	int rc;

	dev_info(&adapter->pdev->dev,
		 "EH enter: type=%s, scmd=0x%p, req tag=%d\n", str, scmd,
		 scsi_cmd_to_rq(scmd)->tag);
	scsi_print_command(scmd);

	if (type == LEAPRAID_TM_TASKTYPE_ABORT_TASK) {
		io_req_tracker = leapraid_get_scmd_priv(scmd);
		dev_info(&adapter->pdev->dev,
			 "EH ABORT: scmd=0x%p, pending=%u ms, tout=%u ms, req tag=%d\n",
			 scmd,
			 jiffies_to_msecs(jiffies - scmd->jiffies_at_alloc),
			 (scsi_cmd_to_rq(scmd)->timeout / HZ) * 1000,
			 scsi_cmd_to_rq(scmd)->tag);
	}

	if (leapraid_pci_removed(adapter) ||
	    adapter->access_ctrl.host_removing) {
		dev_err(&adapter->pdev->dev,
			"EH %s failed: %s scmd=0x%p\n", str,
			(adapter->access_ctrl.host_removing ?
			"shost removing!" : "pci_dev removed!"), scmd);
		if (type == LEAPRAID_TM_TASKTYPE_ABORT_TASK)
			if (io_req_tracker && io_req_tracker->taskid)
				leapraid_free_taskid(adapter,
						     io_req_tracker->taskid);
		scmd->result = DID_NO_CONNECT << LEAPRAID_SCSI_HOST_SHIFT;
#ifdef FAST_IO_FAIL
		rc = FAST_IO_FAIL;
#else
		rc = FAILED;
#endif
		goto out;
	}

	sdev_priv = scmd->device->hostdata;
	if (!sdev_priv || !sdev_priv->starget_priv) {
		dev_warn(&adapter->pdev->dev,
			 "EH %s: sdev or starget gone, scmd=0x%p\n",
			 str, scmd);
		scmd->result = DID_NO_CONNECT << LEAPRAID_SCSI_HOST_SHIFT;
		scsi_done(scmd);
		rc = SUCCESS;
		goto out;
	}

	if (type == LEAPRAID_TM_TASKTYPE_ABORT_TASK) {
		if (!io_req_tracker) {
			dev_warn(&adapter->pdev->dev,
				 "EH ABORT: no io tracker, scmd 0x%p\n", scmd);
			scmd->result = DID_RESET << LEAPRAID_SCSI_HOST_SHIFT;
			rc = SUCCESS;
			goto out;
		}

		if (sdev_priv->starget_priv->flg &
			LEAPRAID_TGT_FLG_RAID_MEMBER ||
		    sdev_priv->starget_priv->flg & LEAPRAID_TGT_FLG_VOLUME) {
			dev_err(&adapter->pdev->dev,
				"EH ABORT: skip RAID/VOLUME target, scmd=0x%p\n",
				scmd);
			scmd->result = DID_RESET << LEAPRAID_SCSI_HOST_SHIFT;
			rc = FAILED;
			goto out;
		}

		hdl = sdev_priv->starget_priv->hdl;
	} else {
		hdl = 0;
		if (sdev_priv->starget_priv->flg &
		    LEAPRAID_TGT_FLG_RAID_MEMBER) {
			sas_dev = leapraid_get_sas_dev_from_tgt(adapter,
								starget_priv);
			if (sas_dev)
				hdl = sas_dev->volume_hdl;
		} else {
			hdl = sdev_priv->starget_priv->hdl;
		}

		if (!hdl) {
			dev_err(&adapter->pdev->dev,
				"EH %s failed: target handle is 0, scmd=0x%p\n",
				str, scmd);
			scmd->result = DID_RESET << LEAPRAID_SCSI_HOST_SHIFT;
			rc = FAILED;
			goto out;
		}
	}

	dev_info(&adapter->pdev->dev,
		 "EH issue TM: type=%s, scmd=0x%p, hdl=0x%x\n",
		 str, scmd, hdl);

	rc = leapraid_issue_locked_tm(adapter, hdl, scmd->device->channel,
				      scmd->device->id,
		(type == LEAPRAID_TM_TASKTYPE_TARGET_RESET ?
			0 : scmd->device->lun),
		type,
		(type == LEAPRAID_TM_TASKTYPE_ABORT_TASK ?
			io_req_tracker->taskid : 0),
		LEAPRAID_TM_MSGFLAGS_LINK_RESET);

out:
	if (type == LEAPRAID_TM_TASKTYPE_ABORT_TASK) {
		dev_info(&adapter->pdev->dev,
			 "EH ABORT result: %s, scmd=0x%p\n",
			 ((rc == SUCCESS) ? "success" : "failed"), scmd);
	} else {
		dev_info(&adapter->pdev->dev,
			 "EH %s result: %s, scmd=0x%p\n",
			 str, ((rc == SUCCESS) ? "success" : "failed"), scmd);
		if (sas_dev)
			leapraid_sdev_put(sas_dev);
	}
	return rc;
}

static int leapraid_eh_abort_handler(struct scsi_cmnd *scmd)
{
	return leapraid_error_handler(scmd, "ABORT TASK",
				      LEAPRAID_TM_TASKTYPE_ABORT_TASK);
}

static int leapraid_eh_device_reset_handler(struct scsi_cmnd *scmd)
{
	return leapraid_error_handler(scmd, "UNIT RESET",
				      LEAPRAID_TM_TASKTYPE_LOGICAL_UNIT_RESET);
}

static int leapraid_eh_target_reset_handler(struct scsi_cmnd *scmd)
{
	return leapraid_error_handler(scmd, "TARGET RESET",
				      LEAPRAID_TM_TASKTYPE_TARGET_RESET);
}

static int leapraid_eh_host_reset_handler(struct scsi_cmnd *scmd)
{
	struct leapraid_adapter *adapter = shost_priv(scmd->device->host);
	int rc;

	dev_info(&adapter->pdev->dev,
		 "EH HOST RESET enter: scmd=%p, req tag=%d\n",
		 scmd,
		 scsi_cmd_to_rq(scmd)->tag);
	scsi_print_command(scmd);

	if (adapter->scan_dev_desc.driver_loading ||
	    adapter->access_ctrl.host_removing) {
		dev_err(&adapter->pdev->dev,
			"EH HOST RESET failed: %s scmd=0x%p\n",
			(adapter->access_ctrl.host_removing ?
			"shost removing!" : "driver loading!"), scmd);
		rc = FAILED;
		goto out;
	}

	dev_info(&adapter->pdev->dev, "%s:%d issuing hard reset\n",
		 __func__, __LINE__);
	if (leapraid_hard_reset_handler(adapter, FULL_RESET) < 0)
		rc = FAILED;
	else
		rc = SUCCESS;

out:
	dev_info(&adapter->pdev->dev, "EH HOST RESET result: %s, scmd=0x%p\n",
		 ((rc == SUCCESS) ? "success" : "failed"), scmd);
	return rc;
}

static int leapraid_slave_alloc(struct scsi_device *sdev)
{
	struct leapraid_raid_volume *raid_volume;
	struct leapraid_starget_priv *stgt_priv;
	struct leapraid_sdev_priv *sdev_priv;
	struct leapraid_adapter *adapter;
	struct leapraid_sas_dev *sas_dev;
	struct scsi_target *tgt;
	struct Scsi_Host *shost;
	unsigned long flags;

	sdev_priv = kzalloc(sizeof(*sdev_priv), GFP_KERNEL);
	if (!sdev_priv)
		return -ENOMEM;

	sdev_priv->lun = sdev->lun;
	sdev_priv->flg = LEAPRAID_DEVICE_FLG_INIT;
	tgt = scsi_target(sdev);
	stgt_priv = tgt->hostdata;
	stgt_priv->num_luns++;
	sdev_priv->starget_priv = stgt_priv;
	sdev->hostdata = sdev_priv;
	if ((stgt_priv->flg & LEAPRAID_TGT_FLG_RAID_MEMBER))
		sdev->no_uld_attach = LEAPRAID_NO_ULD_ATTACH;

	shost = dev_to_shost(&tgt->dev);
	adapter = shost_priv(shost);
	if (tgt->channel == RAID_CHANNEL) {
		spin_lock_irqsave(&adapter->dev_topo.raid_volume_lock, flags);
		raid_volume = leapraid_raid_volume_find_by_id(adapter,
							      tgt->id,
							      tgt->channel);
		if (raid_volume)
			raid_volume->sdev = sdev;
		spin_unlock_irqrestore(&adapter->dev_topo.raid_volume_lock,
				       flags);
	}

	if (!(stgt_priv->flg & LEAPRAID_TGT_FLG_VOLUME)) {
		spin_lock_irqsave(&adapter->dev_topo.sas_dev_lock, flags);
		sas_dev = leapraid_hold_lock_get_sas_dev_by_addr(adapter,
								 stgt_priv->sas_address,
								 stgt_priv->card_port);
		if (sas_dev && !sas_dev->starget) {
			sdev_printk(KERN_INFO, sdev,
				    "%s: assign starget to sas_dev\n", __func__);
			sas_dev->starget = tgt;
		}

		if (sas_dev)
			leapraid_sdev_put(sas_dev);
		spin_unlock_irqrestore(&adapter->dev_topo.sas_dev_lock, flags);
	}
	return 0;
}

static int leapraid_slave_cfg_volume(struct scsi_device *sdev)
{
	struct Scsi_Host *shost = sdev->host;
	struct leapraid_adapter *adapter = shost_priv(shost);
	struct leapraid_raid_volume *raid_volume;
	struct leapraid_starget_priv *starget_priv;
	struct leapraid_sdev_priv *sdev_priv;
	unsigned long flags;
	int qd;
	u16 hdl;

	sdev_priv = sdev->hostdata;
	starget_priv = sdev_priv->starget_priv;
	hdl = starget_priv->hdl;

	spin_lock_irqsave(&adapter->dev_topo.raid_volume_lock, flags);
	raid_volume = leapraid_raid_volume_find_by_hdl(adapter, hdl);
	spin_unlock_irqrestore(&adapter->dev_topo.raid_volume_lock, flags);
	if (!raid_volume) {
		sdev_printk(KERN_WARNING, sdev,
			    "%s: raid_volume not found, hdl=0x%x\n",
			    __func__, hdl);
		return 1;
	}

	if (leapraid_get_volume_cap(adapter, raid_volume)) {
		sdev_printk(KERN_ERR, sdev,
			    "%s: failed to get volume cap, hdl=0x%x\n",
			    __func__, hdl);
		return 1;
	}

	qd = (raid_volume->dev_info & LEAPRAID_DEVTYP_SSP_TGT) ?
		LEAPRAID_SAS_QUEUE_DEPTH : LEAPRAID_SATA_QUEUE_DEPTH;
	if (raid_volume->vol_type != LEAPRAID_VOL_TYPE_RAID0)
		qd = LEAPRAID_RAID_QUEUE_DEPTH;

	sdev_printk(KERN_INFO, sdev,
		    "raid volume: hdl=0x%04x, wwid=0x%016llx\n",
		    raid_volume->hdl, (unsigned long long)raid_volume->wwid);

	if (shost->max_sectors > LEAPRAID_MAX_SECTORS)
		blk_queue_max_hw_sectors(sdev->request_queue,
					 LEAPRAID_MAX_SECTORS);

	leapraid_adjust_sdev_queue_depth(sdev, qd);
	return 0;
}

static int leapraid_slave_configure_extra(struct scsi_device *sdev,
					  struct leapraid_sas_dev **psas_dev,
					  u16 vol_hdl, u64 volume_wwid,
					  bool *is_target_ssp, int *qd)
{
	struct leapraid_sas_dev *sas_dev;
	struct leapraid_sdev_priv *sdev_priv;
	struct Scsi_Host *shost = sdev->host;
	struct leapraid_adapter *adapter = shost_priv(shost);
	unsigned long flags;

	sdev_priv = sdev->hostdata;
	spin_lock_irqsave(&adapter->dev_topo.sas_dev_lock, flags);
	*is_target_ssp = false;
	sas_dev = leapraid_hold_lock_get_sas_dev_by_addr(adapter,
					sdev_priv->starget_priv->sas_address,
					sdev_priv->starget_priv->card_port);
	if (!sas_dev) {
		spin_unlock_irqrestore(&adapter->dev_topo.sas_dev_lock, flags);
		sdev_printk(KERN_WARNING, sdev,
			    "%s: sas_dev not found, sas=0x%llx\n",
			    __func__, sdev_priv->starget_priv->sas_address);
		return 1;
	}

	*psas_dev = sas_dev;
	sas_dev->volume_hdl = vol_hdl;
	sas_dev->volume_wwid = volume_wwid;
	if (sas_dev->dev_info & LEAPRAID_DEVTYP_SSP_TGT) {
		*qd = (sas_dev->port_type > 1) ?
			adapter->adapter_attr.wideport_max_queue_depth :
			adapter->adapter_attr.narrowport_max_queue_depth;
		*is_target_ssp = true;
		if (sas_dev->dev_info & LEAPRAID_DEVTYP_SEP)
			sdev_priv->sep = true;
	} else {
		*qd = adapter->adapter_attr.sata_max_queue_depth;
	}

	sdev_printk(KERN_INFO, sdev,
		    "sdev: dev name=0x%016llx, sas addr=0x%016llx\n",
		    (unsigned long long)sas_dev->dev_name,
		    (unsigned long long)sas_dev->sas_addr);
	leapraid_sdev_put(sas_dev);
	spin_unlock_irqrestore(&adapter->dev_topo.sas_dev_lock, flags);
	return 0;
}

static int leapraid_slave_configure(struct scsi_device *sdev)
{
	struct leapraid_sas_dev *sas_dev;
	struct leapraid_sdev_priv *sdev_priv;
	struct Scsi_Host *shost = sdev->host;
	struct leapraid_starget_priv *starget_priv;
	struct leapraid_adapter *adapter;
	u16 hdl, vol_hdl = 0;
	bool is_target_ssp = false;
	u64 volume_wwid = 0;
	int qd = 1;

	adapter = shost_priv(shost);
	sdev_priv = sdev->hostdata;
	sdev_priv->flg &= ~LEAPRAID_DEVICE_FLG_INIT;
	starget_priv = sdev_priv->starget_priv;
	hdl = starget_priv->hdl;
	if (starget_priv->flg & LEAPRAID_TGT_FLG_VOLUME)
		return leapraid_slave_cfg_volume(sdev);

	if (starget_priv->flg & LEAPRAID_TGT_FLG_RAID_MEMBER) {
		if (leapraid_cfg_get_volume_hdl(adapter, hdl, &vol_hdl)) {
			sdev_printk(KERN_WARNING, sdev,
				    "%s: get volume hdl failed, hdl=0x%x\n",
				    __func__, hdl);
			return 1;
		}

		if (vol_hdl && leapraid_cfg_get_volume_wwid(adapter, vol_hdl,
							    &volume_wwid)) {
			sdev_printk(KERN_WARNING, sdev,
				    "%s: get wwid failed, volume_hdl=0x%x\n",
				    __func__, vol_hdl);
			return 1;
		}
	}

	if (leapraid_slave_configure_extra(sdev, &sas_dev, vol_hdl,
					   volume_wwid, &is_target_ssp, &qd)) {
		sdev_printk(KERN_WARNING, sdev,
			    "%s: slave_configure_extra failed\n", __func__);
		return 1;
	}

	leapraid_adjust_sdev_queue_depth(sdev, qd);
	if (is_target_ssp)
		sas_read_port_mode_page(sdev);

	return 0;
}

static void leapraid_slave_destroy(struct scsi_device *sdev)
{
	struct leapraid_adapter *adapter;
	struct Scsi_Host *shost;
	struct leapraid_sas_dev *sas_dev;
	struct leapraid_starget_priv *starget_priv;
	struct scsi_target *stgt;
	unsigned long flags;

	if (!sdev->hostdata)
		return;

	stgt = scsi_target(sdev);
	starget_priv = stgt->hostdata;
	starget_priv->num_luns--;
	shost = dev_to_shost(&stgt->dev);
	adapter = shost_priv(shost);
	if (!(starget_priv->flg & LEAPRAID_TGT_FLG_VOLUME)) {
		spin_lock_irqsave(&adapter->dev_topo.sas_dev_lock, flags);
		sas_dev = leapraid_hold_lock_get_sas_dev_from_tgt(adapter,
								  starget_priv);
		if (sas_dev && !starget_priv->num_luns)
			sas_dev->starget = NULL;
		if (sas_dev)
			leapraid_sdev_put(sas_dev);
		spin_unlock_irqrestore(&adapter->dev_topo.sas_dev_lock, flags);
	}

	kfree(sdev->hostdata);
	sdev->hostdata = NULL;
}

static int leapraid_target_alloc_raid(struct scsi_target *tgt)
{
	struct leapraid_starget_priv *starget_priv;
	struct leapraid_raid_volume *raid_volume;
	struct Scsi_Host *shost = dev_to_shost(&tgt->dev);
	struct leapraid_adapter *adapter = shost_priv(shost);
	unsigned long flags;

	starget_priv = (struct leapraid_starget_priv *)tgt->hostdata;
	spin_lock_irqsave(&adapter->dev_topo.raid_volume_lock, flags);
	raid_volume = leapraid_raid_volume_find_by_id(adapter, tgt->id,
						      tgt->channel);
	if (raid_volume) {
		starget_priv->hdl = raid_volume->hdl;
		starget_priv->sas_address = raid_volume->wwid;
		starget_priv->flg |= LEAPRAID_TGT_FLG_VOLUME;
		raid_volume->starget = tgt;
	}
	spin_unlock_irqrestore(&adapter->dev_topo.raid_volume_lock, flags);
	return 0;
}

static int leapraid_target_alloc_sas(struct scsi_target *tgt)
{
	struct sas_rphy *rphy;
	struct Scsi_Host *shost;
	struct leapraid_sas_dev *sas_dev;
	struct leapraid_adapter *adapter;
	struct leapraid_starget_priv *starget_priv;
	unsigned long flags;

	shost = dev_to_shost(&tgt->dev);
	adapter = shost_priv(shost);
	starget_priv = (struct leapraid_starget_priv *)tgt->hostdata;
	spin_lock_irqsave(&adapter->dev_topo.sas_dev_lock, flags);
	rphy = dev_to_rphy(tgt->dev.parent);
	sas_dev = leapraid_hold_lock_get_sas_dev_by_addr_and_rphy(adapter,
								  rphy->identify.sas_address,
								  rphy);
	if (sas_dev) {
		starget_priv->sas_dev = sas_dev;
		starget_priv->card_port = sas_dev->card_port;
		starget_priv->sas_address = sas_dev->sas_addr;
		starget_priv->hdl = sas_dev->hdl;
		sas_dev->channel = tgt->channel;
		sas_dev->id = tgt->id;
		sas_dev->starget = tgt;
		if (test_bit(sas_dev->hdl,
			     (unsigned long *)adapter->dev_topo.pd_hdls))
			starget_priv->flg |= LEAPRAID_TGT_FLG_RAID_MEMBER;
	}
	spin_unlock_irqrestore(&adapter->dev_topo.sas_dev_lock, flags);

	return 0;
}

static int leapraid_target_alloc(struct scsi_target *tgt)
{
	struct leapraid_starget_priv *starget_priv;

	starget_priv = kzalloc(sizeof(*starget_priv), GFP_KERNEL);
	if (!starget_priv)
		return -ENOMEM;

	tgt->hostdata = starget_priv;
	starget_priv->starget = tgt;
	starget_priv->hdl = LEAPRAID_INVALID_DEV_HANDLE;
	if (tgt->channel == RAID_CHANNEL)
		return leapraid_target_alloc_raid(tgt);

	return leapraid_target_alloc_sas(tgt);
}

static void leapraid_target_destroy_raid(struct scsi_target *tgt)
{
	struct leapraid_raid_volume *raid_volume;
	struct Scsi_Host *shost = dev_to_shost(&tgt->dev);
	struct leapraid_adapter *adapter = shost_priv(shost);
	unsigned long flags;

	spin_lock_irqsave(&adapter->dev_topo.raid_volume_lock, flags);
	raid_volume = leapraid_raid_volume_find_by_id(adapter, tgt->id,
						      tgt->channel);
	if (raid_volume) {
		raid_volume->starget = NULL;
		raid_volume->sdev = NULL;
	}
	spin_unlock_irqrestore(&adapter->dev_topo.raid_volume_lock, flags);
}

static void leapraid_target_destroy_sas(struct scsi_target *tgt)
{
	struct leapraid_adapter *adapter;
	struct leapraid_sas_dev *sas_dev;
	struct leapraid_starget_priv *starget_priv;
	struct Scsi_Host *shost;
	unsigned long flags;

	shost = dev_to_shost(&tgt->dev);
	adapter = shost_priv(shost);
	starget_priv = tgt->hostdata;

	spin_lock_irqsave(&adapter->dev_topo.sas_dev_lock, flags);
	sas_dev = leapraid_hold_lock_get_sas_dev_from_tgt(adapter,
							  starget_priv);
	if (sas_dev &&
	    sas_dev->starget == tgt &&
	    sas_dev->id == tgt->id &&
	    sas_dev->channel == tgt->channel)
		sas_dev->starget = NULL;

	if (sas_dev) {
		starget_priv->sas_dev = NULL;
		leapraid_sdev_put(sas_dev);
		leapraid_sdev_put(sas_dev);
	}
	spin_unlock_irqrestore(&adapter->dev_topo.sas_dev_lock, flags);
}

static void leapraid_target_destroy(struct scsi_target *tgt)
{
	struct leapraid_starget_priv *starget_priv;

	starget_priv = tgt->hostdata;
	if (!starget_priv)
		return;

	if (tgt->channel == RAID_CHANNEL) {
		leapraid_target_destroy_raid(tgt);
		goto out;
	}

	leapraid_target_destroy_sas(tgt);

out:
	kfree(starget_priv);
	tgt->hostdata = NULL;
}

static bool leapraid_scan_check_status(struct leapraid_adapter *adapter,
				       bool *need_hard_reset)
{
	u32 adapter_state;

	if (adapter->scan_dev_desc.scan_start) {
		adapter_state = leapraid_get_adapter_state(adapter);
		if (adapter_state == LEAPRAID_DB_FAULT) {
			*need_hard_reset = true;
			return true;
		}
		return false;
	}

	if (adapter->driver_cmds.scan_dev_cmd.status & LEAPRAID_CMD_RESET) {
		dev_err(&adapter->pdev->dev,
			"device scan: aborted due to reset\n");
		adapter->driver_cmds.scan_dev_cmd.status =
			LEAPRAID_CMD_NOT_USED;
		adapter->scan_dev_desc.driver_loading = false;
		return true;
	}

	if (adapter->scan_dev_desc.scan_start_failed) {
		dev_err(&adapter->pdev->dev,
			"device scan: failed with adapter_status=0x%08x\n",
			adapter->scan_dev_desc.scan_start_failed);
		adapter->scan_dev_desc.driver_loading = false;
		adapter->scan_dev_desc.wait_scan_dev_done = false;
		adapter->access_ctrl.host_removing = true;
		return true;
	}

	dev_info(&adapter->pdev->dev, "device scan: SUCCESS\n");
	adapter->driver_cmds.scan_dev_cmd.status = LEAPRAID_CMD_NOT_USED;
	leapraid_scan_dev_done(adapter);
	return true;
}

static int leapraid_scan_finished(struct Scsi_Host *shost, unsigned long time)
{
	struct leapraid_adapter *adapter = shost_priv(shost);
	bool need_hard_reset = false;

	if (time >= (LEAPRAID_SCAN_DEV_CMD_TIMEOUT * HZ)) {
		adapter->driver_cmds.scan_dev_cmd.status =
			LEAPRAID_CMD_NOT_USED;
		dev_err(&adapter->pdev->dev,
			"device scan: failed with timeout 300s\n");
		adapter->scan_dev_desc.driver_loading = false;
		return 1;
	}

	if (!leapraid_scan_check_status(adapter, &need_hard_reset))
		return 0;

	if (need_hard_reset) {
		adapter->driver_cmds.scan_dev_cmd.status =
			LEAPRAID_CMD_NOT_USED;
		dev_info(&adapter->pdev->dev, "%s:%d call hard_reset\n",
			 __func__, __LINE__);
		if (leapraid_hard_reset_handler(adapter, PART_RESET))
			adapter->scan_dev_desc.driver_loading = false;
	}

	return 1;
}

static void leapraid_scan_start(struct Scsi_Host *shost)
{
	struct leapraid_adapter *adapter = shost_priv(shost);

	adapter->scan_dev_desc.scan_start = true;
	leapraid_scan_dev(adapter, true);
}

static int leapraid_calc_max_queue_depth(struct scsi_device *sdev, int qdepth)
{
	struct Scsi_Host *shost;
	int max_depth;

	shost = sdev->host;
	max_depth = shost->can_queue;

	if (!sdev->tagged_supported)
		max_depth = 1;

	if (qdepth > max_depth)
		qdepth = max_depth;

	return qdepth;
}

static int leapraid_change_queue_depth(struct scsi_device *sdev, int qdepth)
{
	qdepth = leapraid_calc_max_queue_depth(sdev, qdepth);
	scsi_change_queue_depth(sdev, qdepth);
	return sdev->queue_depth;
}

void leapraid_adjust_sdev_queue_depth(struct scsi_device *sdev, int qdepth)
{
	leapraid_change_queue_depth(sdev, qdepth);
}

static void leapraid_map_queues(struct Scsi_Host *shost)
{
	struct leapraid_adapter *adapter;
	struct blk_mq_queue_map *queue_map;
	int msix_queue_count;
	int poll_queue_count;
	int queue_offset;
	int map_index;

	adapter = (struct leapraid_adapter *)shost->hostdata;
	if (shost->nr_hw_queues == 1)
		goto out;

	msix_queue_count = adapter->notification_desc.iopoll_qdex;
	poll_queue_count = adapter->adapter_attr.rq_cnt - msix_queue_count;

	queue_offset = 0;
	for (map_index = 0; map_index < shost->nr_maps; map_index++) {
		queue_map = &shost->tag_set.map[map_index];
		queue_map->nr_queues = 0;

		switch (map_index) {
		case HCTX_TYPE_DEFAULT:
			queue_map->nr_queues = msix_queue_count;
			queue_map->queue_offset = queue_offset;
			BUG_ON(!queue_map->nr_queues);
			blk_mq_pci_map_queues(queue_map, adapter->pdev, 0);
			break;
		case HCTX_TYPE_POLL:
			queue_map->nr_queues = poll_queue_count;
			queue_map->queue_offset = queue_offset;
			blk_mq_map_queues(queue_map);
			break;
		default:
			queue_map->queue_offset = queue_offset;
			blk_mq_pci_map_queues(queue_map, adapter->pdev, 0);
			break;
		}
		queue_offset += queue_map->nr_queues;
	}

out:
	return;
}

int leapraid_blk_mq_poll(struct Scsi_Host *shost, unsigned int queue_num)
{
	struct leapraid_adapter *adapter =
		(struct leapraid_adapter *)shost->hostdata;
	struct leapraid_blk_mq_poll_rq *blk_mq_poll_rq;
	int num_entries;
	int qid = queue_num - adapter->notification_desc.iopoll_qdex;

	if (atomic_read(&adapter->notification_desc.blk_mq_poll_rqs[qid].pause) ||
	    !atomic_add_unless(&adapter->notification_desc.blk_mq_poll_rqs[qid].busy, 1, 1))
		return 0;

	blk_mq_poll_rq = &adapter->notification_desc.blk_mq_poll_rqs[qid];
	num_entries = leapraid_rep_queue_handler(&blk_mq_poll_rq->rq);
	atomic_dec(&adapter->notification_desc.blk_mq_poll_rqs[qid].busy);
	return num_entries;
}

static int leapraid_bios_param(struct scsi_device *sdev,
			       struct block_device *bdev,
			       sector_t capacity, int geom[])
{
	int heads = 0;
	int sectors = 0;
	sector_t cylinders;

	if (scsi_partsize(bdev, capacity, geom))
		return 0;

	if ((ulong)capacity >= LEAPRAID_LARGE_DISK_THRESHOLD) {
		heads = LEAPRAID_LARGE_DISK_HEADS;
		sectors = LEAPRAID_LARGE_DISK_SECTORS;
	} else {
		heads = LEAPRAID_SMALL_DISK_HEADS;
		sectors = LEAPRAID_SMALL_DISK_SECTORS;
	}

	cylinders = capacity;
	sector_div(cylinders, heads * sectors);

	geom[0] = heads;
	geom[1] = sectors;
	geom[2] = cylinders;
	return 0;
}

static ssize_t fw_queue_depth_show(struct device *cdev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct leapraid_adapter *adapter = shost_priv(shost);

	return sysfs_emit(buf, "%02d\n",
			  adapter->adapter_attr.features.req_slot);
}

static ssize_t host_sas_address_show(struct device *cdev,
				     struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct leapraid_adapter *adapter = shost_priv(shost);

	return sysfs_emit(buf, "0x%016llx\n",
		(unsigned long long)adapter->dev_topo.card.sas_address);
}

static DEVICE_ATTR_RO(fw_queue_depth);
static DEVICE_ATTR_RO(host_sas_address);

static struct attribute *leapraid_shost_attrs[] = {
	&dev_attr_fw_queue_depth.attr,
	&dev_attr_host_sas_address.attr,
	NULL,
};

ATTRIBUTE_GROUPS(leapraid_shost);

static ssize_t sas_address_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct scsi_device *sdev = to_scsi_device(dev);
	struct leapraid_sdev_priv *sas_device_priv_data = sdev->hostdata;

	return sysfs_emit(buf, "0x%016llx\n",
		(unsigned long long)sas_device_priv_data->starget_priv->sas_address);
}

static ssize_t sas_device_handle_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct scsi_device *sdev = to_scsi_device(dev);
	struct leapraid_sdev_priv *sas_device_priv_data = sdev->hostdata;

	return sysfs_emit(buf, "0x%04x\n",
			  sas_device_priv_data->starget_priv->hdl);
}

static ssize_t sas_ncq_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct scsi_device *sdev = to_scsi_device(dev);
	struct leapraid_sdev_priv *sas_device_priv_data = sdev->hostdata;

	return sysfs_emit(buf, "%d\n", sas_device_priv_data->ncq);
}

static ssize_t sas_ncq_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct scsi_device *sdev = to_scsi_device(dev);
	struct leapraid_sdev_priv *sas_device_priv_data = sdev->hostdata;
	struct scsi_vpd *vpd_pg89;
	int ncq_op = 0;
	bool ncq_supported = false;

	if (kstrtoint(buf, 0, &ncq_op))
		goto out;

	rcu_read_lock();
	vpd_pg89 = rcu_dereference(sdev->vpd_pg89);
	if (!vpd_pg89 || vpd_pg89->len < LEAPRAID_VPD_PG89_MIN_LEN) {
		rcu_read_unlock();
		goto out;
	}

	ncq_supported = (vpd_pg89->data[LEAPRAID_VPD_PG89_NCQ_BYTE_IDX] >>
			 LEAPRAID_VPD_PG89_NCQ_BIT_SHIFT) &
			LEAPRAID_VPD_PG89_NCQ_BIT_MASK;
	rcu_read_unlock();
	if (ncq_supported)
		sas_device_priv_data->ncq = ncq_op;
	return strlen(buf);
out:
	return -EINVAL;
}

static DEVICE_ATTR_RO(sas_address);
static DEVICE_ATTR_RO(sas_device_handle);

static DEVICE_ATTR_RW(sas_ncq);

static struct attribute *leapraid_sdev_attrs[] = {
	&dev_attr_sas_address.attr,
	&dev_attr_sas_device_handle.attr,
	&dev_attr_sas_ncq.attr,
	NULL,
};

ATTRIBUTE_GROUPS(leapraid_sdev);

static struct scsi_host_template leapraid_driver_template = {
	.module = THIS_MODULE,
	.name = "LEAPIO RAID Host",
	.proc_name = LEAPRAID_DRIVER_NAME,
	.queuecommand = leapraid_queuecommand,
	.cmd_size = sizeof(struct leapraid_io_req_tracker),
	.init_cmd_priv = leapraid_init_cmd_priv,
	.exit_cmd_priv = leapraid_exit_cmd_priv,
	.eh_abort_handler = leapraid_eh_abort_handler,
	.eh_device_reset_handler = leapraid_eh_device_reset_handler,
	.eh_target_reset_handler = leapraid_eh_target_reset_handler,
	.eh_host_reset_handler = leapraid_eh_host_reset_handler,
	.slave_alloc = leapraid_slave_alloc,
	.slave_destroy = leapraid_slave_destroy,
	.slave_configure = leapraid_slave_configure,
	.target_alloc = leapraid_target_alloc,
	.target_destroy = leapraid_target_destroy,
	.scan_finished = leapraid_scan_finished,
	.scan_start = leapraid_scan_start,
	.change_queue_depth = leapraid_change_queue_depth,
	.map_queues = leapraid_map_queues,
	.mq_poll = leapraid_blk_mq_poll,
	.bios_param = leapraid_bios_param,
	.can_queue = LEAPRAID_CAN_QUEUE_MIN,
	.this_id = LEAPRAID_THIS_ID_NONE,
	.sg_tablesize = LEAPRAID_SG_DEPTH,
	.max_sectors = LEAPRAID_DEF_MAX_SECTORS,
	.max_segment_size = LEAPRAID_MAX_SEGMENT_SIZE,
	.cmd_per_lun = LEAPRAID_CMD_PER_LUN,
	.shost_groups = leapraid_shost_groups,
	.sdev_groups = leapraid_sdev_groups,
	.track_queue_depth = 1,
};

static void leapraid_lock_init(struct leapraid_adapter *adapter)
{
	mutex_init(&adapter->reset_desc.adapter_reset_mutex);
	mutex_init(&adapter->reset_desc.host_diag_mutex);
	mutex_init(&adapter->access_ctrl.pci_access_lock);

	spin_lock_init(&adapter->reset_desc.adapter_reset_lock);
	spin_lock_init(&adapter->dynamic_task_desc.task_lock);
	spin_lock_init(&adapter->dev_topo.sas_dev_lock);
	spin_lock_init(&adapter->dev_topo.topo_node_lock);
	spin_lock_init(&adapter->fw_evt_s.fw_evt_lock);
	spin_lock_init(&adapter->dev_topo.raid_volume_lock);
}

static void leapraid_list_init(struct leapraid_adapter *adapter)
{
	INIT_LIST_HEAD(&adapter->dev_topo.sas_dev_list);
	INIT_LIST_HEAD(&adapter->dev_topo.card_port_list);
	INIT_LIST_HEAD(&adapter->dev_topo.sas_dev_init_list);
	INIT_LIST_HEAD(&adapter->dev_topo.exp_list);
	INIT_LIST_HEAD(&adapter->dev_topo.enc_list);
	INIT_LIST_HEAD(&adapter->fw_evt_s.fw_evt_list);
	INIT_LIST_HEAD(&adapter->dev_topo.raid_volume_list);
	INIT_LIST_HEAD(&adapter->dev_topo.card.sas_port_list);
}

static int leapraid_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct leapraid_adapter *adapter = NULL;
	struct Scsi_Host *shost = NULL;
	int iopoll_q_count = 0;
	int rc;

	shost = scsi_host_alloc(&leapraid_driver_template,
				sizeof(struct leapraid_adapter));
	if (!shost)
		return -ENODEV;

	adapter = shost_priv(shost);
	memset(adapter, 0, sizeof(struct leapraid_adapter));
	adapter->adapter_attr.id = leapraid_ids++;

	adapter->adapter_attr.enable_mp = enable_mp;

	adapter = shost_priv(shost);
	INIT_LIST_HEAD(&adapter->list);
	spin_lock(&leapraid_adapter_lock);
	list_add_tail(&adapter->list, &leapraid_adapter_list);
	spin_unlock(&leapraid_adapter_lock);

	adapter->shost = shost;
	adapter->pdev = pdev;
	adapter->fw_log_desc.open_pcie_trace = open_pcie_trace;
	leapraid_lock_init(adapter);
	leapraid_list_init(adapter);
	sprintf(adapter->adapter_attr.name, "%s%d",
		LEAPRAID_DRIVER_NAME, adapter->adapter_attr.id);

	shost->max_cmd_len = LEAPRAID_MAX_CDB_LEN;
	shost->max_lun = LEAPRAID_MAX_LUNS;
	shost->transportt = leapraid_transport_template;
	shost->unique_id = adapter->adapter_attr.id;

	snprintf(adapter->fw_evt_s.fw_evt_name,
		 sizeof(adapter->fw_evt_s.fw_evt_name),
		 "fw_event_%s%d", LEAPRAID_DRIVER_NAME,
		 adapter->adapter_attr.id);
	adapter->fw_evt_s.fw_evt_thread =
		alloc_ordered_workqueue(adapter->fw_evt_s.fw_evt_name, 0);
	if (!adapter->fw_evt_s.fw_evt_thread) {
		rc = -ENODEV;
		goto evt_wq_fail;
	}

	shost->host_tagset = 1;
	adapter->scan_dev_desc.driver_loading = true;
	if ((leapraid_ctrl_init(adapter))) {
		rc = -ENODEV;
		goto ctrl_init_fail;
	}

	shost->nr_hw_queues = 1;
	if (shost->host_tagset) {
		shost->nr_hw_queues = adapter->adapter_attr.rq_cnt;
		iopoll_q_count = adapter->adapter_attr.rq_cnt -
				 adapter->notification_desc.iopoll_qdex;
		shost->nr_maps = iopoll_q_count ? 3 : 1;
		dev_info(&adapter->pdev->dev,
			 "max scsi io cmds %d shared with nr_hw_queues=%d\n",
			 shost->can_queue, shost->nr_hw_queues);
	}

	rc = scsi_add_host(shost, &pdev->dev);
	if (rc) {
		spin_lock(&leapraid_adapter_lock);
		list_del(&adapter->list);
		spin_unlock(&leapraid_adapter_lock);
		goto scsi_add_shost_fail;
	}

	scsi_scan_host(shost);
	return 0;

scsi_add_shost_fail:
	leapraid_remove_ctrl(adapter);
ctrl_init_fail:
	destroy_workqueue(adapter->fw_evt_s.fw_evt_thread);
evt_wq_fail:
	spin_lock(&leapraid_adapter_lock);
	list_del(&adapter->list);
	spin_unlock(&leapraid_adapter_lock);
	scsi_host_put(shost);
	return rc;
}

static void leapraid_cleanup_lists(struct leapraid_adapter *adapter)
{
	struct leapraid_raid_volume *raid_volume, *next_raid_volume;
	struct leapraid_starget_priv *starget_priv_data;
	struct leapraid_sas_port *leapraid_port, *next_port;
	struct leapraid_card_port *port, *port_next;
	struct leapraid_vphy *vphy, *vphy_next;

	list_for_each_entry_safe(raid_volume, next_raid_volume,
				 &adapter->dev_topo.raid_volume_list, list) {
		if (raid_volume->starget) {
			starget_priv_data = raid_volume->starget->hostdata;
			starget_priv_data->deleted = true;
			scsi_remove_target(&raid_volume->starget->dev);
		}
		pr_info("removing hdl=0x%04x, wwid=0x%016llx\n",
			raid_volume->hdl,
			(unsigned long long)raid_volume->wwid);
		leapraid_raid_volume_remove(adapter, raid_volume);
	}

	list_for_each_entry_safe(leapraid_port, next_port,
				 &adapter->dev_topo.card.sas_port_list,
				 port_list) {
		if (leapraid_port->remote_identify.device_type ==
		    SAS_END_DEVICE)
			leapraid_sas_dev_remove_by_sas_address(adapter,
				leapraid_port->remote_identify.sas_address,
				leapraid_port->card_port);
		else if (leapraid_port->remote_identify.device_type ==
				SAS_EDGE_EXPANDER_DEVICE ||
			 leapraid_port->remote_identify.device_type ==
				SAS_FANOUT_EXPANDER_DEVICE)
			leapraid_exp_rm(adapter,
					leapraid_port->remote_identify.sas_address,
					leapraid_port->card_port);
	}

	list_for_each_entry_safe(port, port_next,
				 &adapter->dev_topo.card_port_list, list) {
		if (port->vphys_mask)
			list_for_each_entry_safe(vphy, vphy_next,
						 &port->vphys_list, list) {
				list_del(&vphy->list);
				kfree(vphy);
			}
		list_del(&port->list);
		kfree(port);
	}

	if (adapter->dev_topo.card.phys_num) {
		kfree(adapter->dev_topo.card.card_phy);
		adapter->dev_topo.card.card_phy = NULL;
		adapter->dev_topo.card.phys_num = 0;
	}
}

static void leapraid_remove(struct pci_dev *pdev)
{
	struct leapraid_adapter *adapter = pdev_to_adapter(pdev);
	struct Scsi_Host *shost = pdev_to_shost(pdev);
	struct workqueue_struct *wq;
	unsigned long flags;

	if (!shost || !adapter) {
		dev_err(&pdev->dev, "unable to remove!\n");
		return;
	}

	while (adapter->scan_dev_desc.driver_loading)
		ssleep(1);

	while (adapter->access_ctrl.shost_recovering)
		ssleep(1);

	adapter->access_ctrl.host_removing = true;

	leapraid_wait_cmds_done(adapter);

	leapraid_smart_polling_stop(adapter);
	leapraid_free_internal_scsi_cmd(adapter);

	if (leapraid_pci_removed(adapter)) {
		leapraid_mq_polling_pause(adapter);
		leapraid_clean_active_scsi_cmds(adapter);
	}
	leapraid_clean_active_fw_evt(adapter);

	spin_lock_irqsave(&adapter->fw_evt_s.fw_evt_lock, flags);
	wq = adapter->fw_evt_s.fw_evt_thread;
	adapter->fw_evt_s.fw_evt_thread = NULL;
	spin_unlock_irqrestore(&adapter->fw_evt_s.fw_evt_lock, flags);
	if (wq)
		destroy_workqueue(wq);

	leapraid_ir_shutdown(adapter);
	sas_remove_host(shost);
	leapraid_cleanup_lists(adapter);
	leapraid_remove_ctrl(adapter);
	spin_lock(&leapraid_adapter_lock);
	list_del(&adapter->list);
	spin_unlock(&leapraid_adapter_lock);
	scsi_host_put(shost);
}

static void leapraid_shutdown(struct pci_dev *pdev)
{
	struct leapraid_adapter *adapter = pdev_to_adapter(pdev);
	struct Scsi_Host *shost = pdev_to_shost(pdev);
	struct workqueue_struct *wq;
	unsigned long flags;

	if (!shost || !adapter) {
		dev_err(&pdev->dev, "unable to shutdown!\n");
		return;
	}

	adapter->access_ctrl.host_removing = true;
	leapraid_wait_cmds_done(adapter);
	leapraid_clean_active_fw_evt(adapter);
	leapraid_fw_log_stop(adapter);
	spin_lock_irqsave(&adapter->fw_evt_s.fw_evt_lock, flags);
	wq = adapter->fw_evt_s.fw_evt_thread;
	adapter->fw_evt_s.fw_evt_thread = NULL;
	spin_unlock_irqrestore(&adapter->fw_evt_s.fw_evt_lock, flags);
	if (wq)
		destroy_workqueue(wq);

	leapraid_ir_shutdown(adapter);
	leapraid_disable_controller(adapter);
}

static pci_ers_result_t leapraid_pci_error_detected(struct pci_dev *pdev,
						    pci_channel_state_t state)
{
	struct leapraid_adapter *adapter = pdev_to_adapter(pdev);
	struct Scsi_Host *shost = pdev_to_shost(pdev);

	if (!shost || !adapter) {
		dev_err(&pdev->dev, "failed to error detected for device\n");
		return PCI_ERS_RESULT_DISCONNECT;
	}

	pr_err("%s: pci error detected, state=%d\n",
	       adapter->adapter_attr.name, state);

	switch (state) {
	case pci_channel_io_normal:
		return PCI_ERS_RESULT_CAN_RECOVER;
	case pci_channel_io_frozen:
		adapter->access_ctrl.pcie_recovering = true;
		scsi_block_requests(adapter->shost);
		leapraid_smart_polling_stop(adapter);
		leapraid_check_scheduled_fault_stop(adapter);
		leapraid_fw_log_stop(adapter);
		leapraid_disable_controller(adapter);
		return PCI_ERS_RESULT_NEED_RESET;
	case pci_channel_io_perm_failure:
		adapter->access_ctrl.pcie_recovering = true;
		leapraid_smart_polling_stop(adapter);
		leapraid_check_scheduled_fault_stop(adapter);
		leapraid_fw_log_stop(adapter);
		leapraid_mq_polling_pause(adapter);
		leapraid_clean_active_scsi_cmds(adapter);
		return PCI_ERS_RESULT_DISCONNECT;
	}

	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t leapraid_pci_mmio_enabled(struct pci_dev *pdev)
{
	struct leapraid_adapter *adapter = pdev_to_adapter(pdev);
	struct Scsi_Host *shost = pdev_to_shost(pdev);

	if (!shost || !adapter) {
		dev_err(&pdev->dev,
			"failed to enable mmio for device\n");
		return PCI_ERS_RESULT_DISCONNECT;
	}

	dev_info(&pdev->dev, "%s: pci error mmio enabled\n",
		 adapter->adapter_attr.name);

	return PCI_ERS_RESULT_RECOVERED;
}

static pci_ers_result_t leapraid_pci_slot_reset(struct pci_dev *pdev)
{
	struct leapraid_adapter *adapter = pdev_to_adapter(pdev);
	struct Scsi_Host *shost = pdev_to_shost(pdev);
	int rc;

	if (!shost || !adapter) {
		dev_err(&pdev->dev,
			"failed to slot reset for device\n");
		return PCI_ERS_RESULT_DISCONNECT;
	}

	dev_err(&pdev->dev, "%s pci error slot reset\n",
		adapter->adapter_attr.name);

	adapter->access_ctrl.pcie_recovering = false;
	adapter->pdev = pdev;
	pci_restore_state(pdev);
	if (leapraid_set_pcie_and_notification(adapter))
		return PCI_ERS_RESULT_DISCONNECT;

	dev_info(&pdev->dev, "%s: hard reset triggered by pci slot reset\n",
		 adapter->adapter_attr.name);
	dev_info(&adapter->pdev->dev, "%s:%d call hard_reset\n",
		 __func__, __LINE__);
	rc = leapraid_hard_reset_handler(adapter, FULL_RESET);
	dev_info(&pdev->dev, "%s hard reset: %s\n",
		 adapter->adapter_attr.name, (rc == 0) ? "success" : "failed");

	return (rc == 0) ? PCI_ERS_RESULT_RECOVERED :
		 PCI_ERS_RESULT_DISCONNECT;
}

static void leapraid_pci_resume(struct pci_dev *pdev)
{
	struct Scsi_Host *shost = pdev_to_shost(pdev);
	struct leapraid_adapter *adapter = pdev_to_adapter(pdev);

	if (!shost || !adapter) {
		dev_err(&pdev->dev, "failed to resume\n");
		return;
	}

	dev_err(&pdev->dev, "PCI error resume!\n");
	pci_aer_clear_nonfatal_status(pdev);
	leapraid_check_scheduled_fault_start(adapter);
	leapraid_fw_log_start(adapter);
	scsi_unblock_requests(adapter->shost);
	leapraid_smart_polling_start(adapter);
}

MODULE_DEVICE_TABLE(pci, leapraid_pci_table);
static struct pci_error_handlers leapraid_err_handler = {
	.error_detected = leapraid_pci_error_detected,
	.mmio_enabled = leapraid_pci_mmio_enabled,
	.slot_reset = leapraid_pci_slot_reset,
	.resume = leapraid_pci_resume,
};

#ifdef CONFIG_PM
static int leapraid_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct leapraid_adapter *adapter = pdev_to_adapter(pdev);
	struct Scsi_Host *shost = pdev_to_shost(pdev);
	pci_power_t device_state;

	if (!shost || !adapter) {
		dev_err(&pdev->dev,
			"suspend failed, invalid host or adapter\n");
		return -ENXIO;
	}

	leapraid_smart_polling_stop(adapter);
	leapraid_check_scheduled_fault_stop(adapter);
	leapraid_fw_log_stop(adapter);
	scsi_block_requests(shost);
	device_state = pci_choose_state(pdev, state);
	leapraid_ir_shutdown(adapter);

	dev_info(&pdev->dev, "entering PCI power state D%d, (slot=%s)\n",
		 device_state, pci_name(pdev));

	pci_save_state(pdev);
	leapraid_disable_controller(adapter);
	pci_set_power_state(pdev, device_state);
	return 0;
}

static int leapraid_resume(struct pci_dev *pdev)
{
	struct leapraid_adapter *adapter = pdev_to_adapter(pdev);
	struct Scsi_Host *shost = pdev_to_shost(pdev);
	pci_power_t device_state = pdev->current_state;
	int rc;

	if (!shost || !adapter) {
		dev_err(&pdev->dev,
			"resume failed, invalid host or adapter\n");
		return -ENXIO;
	}

	dev_info(&pdev->dev,
		 "resuming device %s, previous state D%d\n",
		 pci_name(pdev), device_state);

	pci_set_power_state(pdev, PCI_D0);
	pci_enable_wake(pdev, PCI_D0, 0);
	pci_restore_state(pdev);
	adapter->pdev = pdev;
	rc = leapraid_set_pcie_and_notification(adapter);
	if (rc)
		return rc;

	dev_info(&adapter->pdev->dev, "%s:%d call hard_reset\n",
		 __func__, __LINE__);
	leapraid_hard_reset_handler(adapter, PART_RESET);
	scsi_unblock_requests(shost);
	leapraid_check_scheduled_fault_start(adapter);
	leapraid_fw_log_start(adapter);
	leapraid_smart_polling_start(adapter);
	return 0;
}
#endif /* CONFIG_PM */

static struct pci_driver leapraid_driver = {
	.name = LEAPRAID_DRIVER_NAME,
	.id_table = leapraid_pci_table,
	.probe = leapraid_probe,
	.remove = leapraid_remove,
	.shutdown = leapraid_shutdown,
	.err_handler = &leapraid_err_handler,
#ifdef CONFIG_PM
	.suspend = leapraid_suspend,
	.resume = leapraid_resume,
#endif /* CONFIG_PM */
};

static int __init leapraid_init(void)
{
	int error;

	pr_info("%s version %s loaded\n", LEAPRAID_DRIVER_NAME,
		LEAPRAID_DRIVER_VERSION);

	leapraid_transport_template =
		sas_attach_transport(&leapraid_transport_functions);
	if (!leapraid_transport_template)
		return -ENODEV;

	leapraid_ids = 0;

	leapraid_ctl_init();

	error = pci_register_driver(&leapraid_driver);
	if (error)
		sas_release_transport(leapraid_transport_template);

	return error;
}

static void __exit leapraid_exit(void)
{
	pr_info("leapraid version %s unloading\n",
		LEAPRAID_DRIVER_VERSION);

	leapraid_ctl_exit();
	pci_unregister_driver(&leapraid_driver);
	sas_release_transport(leapraid_transport_template);
}

module_init(leapraid_init);
module_exit(leapraid_exit);
