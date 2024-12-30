
#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/blkdev.h>
#include <linux/version.h>

#include <scsi/scsi.h>
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_eh.h>
#endif

#include "ps3_htp_def.h"
#include "ps3_util.h"
#include "ps3_instance_manager.h"
#include "ps3_ioc_state.h"
#include "ps3_mgr_cmd_err.h"
#include "ps3_mgr_cmd.h"
#include "ps3_scsih.h"
#include "ps3_cmd_complete.h"
#include "ps3_driver_log.h"
#include "ps3_cmd_statistics.h"
#include "ps3_scsih_cmd_parse.h"
#include "ps3_module_para.h"
#include "ps3_r1x_write_lock.h"
#include "ps3_nvme_spec.h"
#include "ps3_nvme_resp_to_scsi.h"
#include "ps3_scsi_cmd_err.h"
#include "ps3_ioc_manager.h"

static void ps3_scsi_sense_print(struct ps3_instance *instance,
	const U8 *sense_buffer, U8 len, S32 CFID);

static void ps3_internal_errcode_to_scsi(struct scsi_cmnd *s_cmd, S32 err_code,
	U32 xfer_cnt, Bool is_sata_jbod_mgr_cmd)
{
	S32 host_status = DID_OK;
	S32 status = SCSI_STATUS_GOOD;

	switch (err_code) {
	case PS3_STATUS_VD_OFFLINE:
	case PS3_STATUS_DEVICE_NOT_FOUND:
		host_status = DID_BAD_TARGET;
		break;
	case PS3_STATUS_INTERNAL_ERR:
		host_status = DID_ERROR;
		break;

	case PS3_STATUS_IO_ABORTED:
	case PS3_STATUS_INTERNAL_SOFT_ERR:
#ifndef _WINDOWS
		host_status = DID_SOFT_ERROR;
#else
		host_status = DID_ERROR;
#endif
		break;
	case PS3_STATUS_ACCESS_BLOCK:
		host_status = DID_OK;
#ifndef _WINDOWS
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,14,0))
		s_cmd->result |= PS3_SCSI_RESULT_DRIVER_STATUS(DRIVER_SENSE);
#endif
		status = SAM_STAT_CHECK_CONDITION;
		scsi_build_sense_buffer(0, s_cmd->sense_buffer,
			ILLEGAL_REQUEST, 0x20, 0x02);
		scsi_set_resid(s_cmd, scsi_bufflen(s_cmd));
		break;
	case PS3_STATUS_REQ_ILLEGAL:
		host_status = DID_OK;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,14,0))
		s_cmd->result |= PS3_SCSI_RESULT_DRIVER_STATUS(DRIVER_SENSE);
#endif
		status = SAM_STAT_CHECK_CONDITION;
		scsi_build_sense_buffer(0, s_cmd->sense_buffer,
			ILLEGAL_REQUEST, 0x20, 0x00);
		scsi_set_resid(s_cmd, scsi_bufflen(s_cmd));
#else
		scsi_build_sense_buffer(s_cmd, 0, ILLEGAL_REQUEST, 0x20, 0x02);
#endif
		break;
	case PS3_STATUS_HOST_NOT_FOUND:
    case PS3_STATUS_PCI_RECOVERY:
		host_status = DID_NO_CONNECT;
		break;
	case PS3_STATUS_VD_MEMBER_OFFLINE:
		host_status = DID_IMM_RETRY;
		break;
	case PS3_STATUS_HOST_RESET:
		#if ((defined(RHEL_MAJOR) && (RHEL_MAJOR == 7) && (RHEL_MINOR == 3)) || \
			((LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)) && \
			(LINUX_VERSION_CODE < KERNEL_VERSION(4,4,14))))
			host_status  = DID_REQUEUE;
		#else
			host_status = DID_RESET;
		#endif
		break;
	case PS3_STATUS_UNDERRUN:
		if ((scsi_bufflen(s_cmd) < xfer_cnt)
			&& (!is_sata_jbod_mgr_cmd)) {
			host_status = DID_SOFT_ERROR;
			scsi_set_resid(s_cmd, 0);
		} else if (scsi_bufflen(s_cmd) == xfer_cnt) {
			host_status = DID_OK;
		} else {
			if (scsi_bufflen(s_cmd) > xfer_cnt) {
				scsi_set_resid(s_cmd, scsi_bufflen(s_cmd) - xfer_cnt);
			} else {
				scsi_set_resid(s_cmd, 0);
			}
			if (xfer_cnt < s_cmd->underflow) {
				host_status = DID_SOFT_ERROR;
			} else if (!xfer_cnt && s_cmd->cmnd[0] == REPORT_LUNS) {
				host_status = DID_OK;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,14,0))
				s_cmd->result |= PS3_SCSI_RESULT_DRIVER_STATUS(DRIVER_SENSE);
#endif
				status = SAM_STAT_CHECK_CONDITION;
				scsi_build_sense_buffer(0, s_cmd->sense_buffer, ILLEGAL_REQUEST, 0x20, 0x0);
			} else {
				host_status = DID_OK;
			}
		}
		break;
	case PS3_STATUS_OVERRUN:
		scsi_set_resid(s_cmd, 0);
		host_status = DID_SOFT_ERROR;
		break;

	default:
		host_status = DID_ERROR;
		break;
	}
#ifndef _WINDOWS
	s_cmd->result |= PS3_SCSI_RESULT_HOST_STATUS(host_status) | status;
#else
	scsi_cmnd_scsistatus_set(s_cmd, status);
	scsi_cmnd_hoststatus_set(s_cmd, host_status);
#endif
}

static void ps3_standard_errcode_to_scsi(struct ps3_instance *instance,
	struct scsi_cmnd *s_cmd, S32 err_code,
	PS3RespFrame_u *resp_frame)
{
	S32 host_status = DID_OK;
	S32 status = SCSI_STATUS_GOOD;

	switch (err_code) {
	case SCSI_STATUS_GOOD:
	case SCSI_STATUS_CONDITION_MET:
	case SCSI_STATUS_BUSY:
	case SCSI_STATUS_TASK_SET_FULL:
	case SCSI_STATUS_ACA_ACTIVE:
	case SCSI_STATUS_TASK_ABORTED:
		status = err_code;
		break;
	case SCSI_STATUS_RESERVATION_CONFLICT:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,1,38))
		host_status = DID_NEXUS_FAILURE;
#endif
		status = err_code;
		break;
	case SCSI_STATUS_CHECK_CONDITION:
		host_status = DID_OK;
		status = err_code;
		if (resp_frame != NULL) {
			memcpy(s_cmd->sense_buffer, resp_frame->normalRespFrame.sense,
				SCSI_SENSE_BUFFERSIZE);
			ps3_scsi_sense_print(instance,
				s_cmd->sense_buffer,
				SCSI_SENSE_BUFFERSIZE, SCMD_GET_REQUEST(s_cmd)->tag);
		}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,14,0))
		s_cmd->result |= PS3_SCSI_RESULT_DRIVER_STATUS(DRIVER_SENSE);
#endif
		break;
	default:
		host_status = DID_ERROR;
		break;
	}

	s_cmd->result |= PS3_SCSI_RESULT_HOST_STATUS(host_status) | status;
}

void ps3_errcode_to_scsi_status(struct ps3_instance *instance,
	struct scsi_cmnd *s_cmd, U32 err_code,
	PS3RespFrame_u *resp_frame, U32 xfer_cnt, struct ps3_cmd *cmd)
{
	Bool is_sata_jbod_mgr_cmd = 0;
	if (cmd != NULL) {
		is_sata_jbod_mgr_cmd = ps3_scsih_is_sata_jbod_mgr_cmd(cmd);
	}

	s_cmd->result = SCSI_STATUS_GOOD;

	if (err_code < PS3_STATUS_DEVICE_NOT_FOUND) {
		ps3_standard_errcode_to_scsi(instance,
		s_cmd, err_code, resp_frame);
	} else {
		ps3_internal_errcode_to_scsi(s_cmd, err_code, xfer_cnt, is_sata_jbod_mgr_cmd);
	}
}

static inline void ps3_err_scsi_errcode_mapping(struct ps3_cmd *cmd, U16 mode,
	Bool is_sas_direct)
{
	U8 opcode = 0;
	U16 sub_opcode = 0;
	U32 resp_status = 0;
	PS3RespFrame_u *resp_frame = cmd->resp_frame;
	U32 xfer_cnt = 0;
	Ps3SasDirectRespFrameIU_s *frame = NULL;

	ps3_scsih_cdb_opcode_get(cmd->scmd->cmnd, &opcode, &sub_opcode);
	LOG_FILE_INFO("tid:0x%llx hno:%u op:0x%x sub_op:0x%x "
		"tag:%u\n", cmd->trace_id, PS3_HOST(cmd->instance),
		opcode, sub_opcode, cmd->index);

	if (ps3_err_is_resp_from_direct_cmd(mode)
		&& (cmd->io_attr.dev_type != PS3_DEV_TYPE_VD)) {
		resp_status = resp_frame->sasRespFrame.status & PS3_SCSI_STATUS_MASK;
	} else {
		resp_status = resp_frame->normalRespFrame.respStatus & PS3_SCSI_STATUS_MASK;
	}
	xfer_cnt = ps3_scsih_xfer_cnt_get(cmd);
	ps3_errcode_to_scsi_status(cmd->instance, cmd->scmd, resp_status, resp_frame, xfer_cnt, cmd);

	if (is_sas_direct) {
		frame = &resp_frame->sasRespFrame;
		LOG_INFO_IN_IRQ(cmd->instance,
		"tid:0x%llx hno:%u [%u:%u:%u], op:0x%x sub_op:0x%x "
		"tag:%u data_pres:%u resp_status:0x%x scmd status:0x%x "
		"scmd cmd_flags:0x%llx, retries:%d, allowd:%d,"
		"xfer_cnt:%u scsi_buflen:%u\n",
		cmd->trace_id, PS3_HOST(cmd->instance), cmd->scmd->device->channel,
		cmd->scmd->device->id, cmd->io_attr.disk_id, opcode, sub_opcode,
		cmd->index, frame->dataPres, frame->status, cmd->scmd->result,
		(U64)SCMD_GET_REQUEST(cmd->scmd)->cmd_flags,
		cmd->scmd->retries, cmd->scmd->allowed,
		xfer_cnt, scsi_bufflen(cmd->scmd));
	} else {
		LOG_INFO_IN_IRQ(cmd->instance,
		"tid:0x%llx hno:%u [%u:%u:%u], op:0x%x sub_op:0x%x "
		"tag:%u resp_status:0x%x scmd status:0x%x"
		"scmd cmd_flags:0x%llx, retries:%d, allowd:%d, "
		"xfer_cnt:%u scsi_buflen:%u\n",
		cmd->trace_id, PS3_HOST(cmd->instance), cmd->scmd->device->channel,
		cmd->scmd->device->id, cmd->io_attr.disk_id, opcode, sub_opcode,
		cmd->index, resp_frame->normalRespFrame.respStatus, cmd->scmd->result,
		(U64)SCMD_GET_REQUEST(cmd->scmd)->cmd_flags, cmd->scmd->retries,
		cmd->scmd->allowed, xfer_cnt, scsi_bufflen(cmd->scmd));
	}
}

static Bool ps3_err_retry_sys_state_check(struct ps3_instance *instance)
{
	Bool ret = PS3_DRV_TRUE;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;

	if (!instance->state_machine.is_load) {
		LOG_ERROR_IN_IRQ(instance,
			"hno:%u instance state not is_load\n", PS3_HOST(instance));
		ret = PS3_DRV_FALSE;
		goto l_out;
	}

	cur_state = ps3_atomic_read(&instance->state_machine.state);
	if (cur_state == PS3_INSTANCE_STATE_DEAD ||
		cur_state == PS3_INSTANCE_STATE_QUIT) {
		LOG_ERROR_IN_IRQ(instance,
			"hno:%u instance state is dead/quit\n", PS3_HOST(instance));
		ret = PS3_DRV_FALSE;
		goto l_out;
	}

l_out:
	return ret;
}

static S32 ps3_is_state_abnormal(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	static unsigned long j;
	if (!ps3_is_instance_state_normal(instance)){
		ret = -PS3_FAILED;
		goto l_out;
	}

	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u host in pci recovery\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}
l_out:
	return ret;
}

static Bool ps3_err_code_is_need_change(const struct ps3_cmd *cmd)
{
	U32 err_code = 0;
	PS3RespFrame_u *resp_frame = cmd->resp_frame;
	U16 mode = cmd->reply_word.mode;
	Bool ret = PS3_TRUE;
	PS3NvmeCmdStatus_s status;
	ps3_nvme_scsi_status cpl;

	if (ps3_err_is_resp_from_direct_cmd(mode)) {
		if (cmd->reply_word.diskType == PS3_DEV_TYPE_SAS_SATA) {
			err_code = resp_frame->sasRespFrame.status & PS3_SCSI_STATUS_MASK;
		} else {
			status.cmdStatus = cmd->reply_word.retStatus;
			memset(&cpl, 0, sizeof(ps3_nvme_scsi_status));
			ps3_nvme_error_to_scsi_status(status, &cpl);
			err_code =  cpl.status;
		}
	}

	switch (err_code) {
		case SCSI_STATUS_BUSY:
		case SCSI_STATUS_TASK_SET_FULL:
		case SCSI_STATUS_CONDITION_MET:
		case SCSI_STATUS_ACA_ACTIVE:
		case PS3_STATUS_OVERRUN:
		case PS3_STATUS_UNDERRUN:
			ret = PS3_FALSE;
			break;
		default:
			ret = PS3_TRUE;
	}

	LOG_DEBUG("tid:0x%llx hno:%u tag:%u status:0x%x %s change\n",
		cmd->trace_id, PS3_HOST(cmd->instance), cmd->index, err_code,
		ret == PS3_TRUE?"need":"no need");

	return ret;
}

static inline Bool ps3_is_need_direct_to_normal(const struct ps3_instance *instance,
		const struct ps3_cmd *cmd)
{
	return ((instance->ioc_adpter->is_need_direct_to_normal != NULL)
		&& (instance->ioc_adpter->is_need_direct_to_normal(cmd))
		&& (cmd->scmd->allowed != 0));
}

void ps3_scsih_drv_io_reply_scsi(struct scsi_cmnd *s_cmd, struct ps3_cmd *cmd, U8 resp_status, Bool cmd_lock);

S32 ps3_err_scsi_io_processing(struct ps3_instance *instance,
		U32 id, U32 channel)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd_context *cmd_context = &instance->cmd_context;
	struct ps3_cmd *cmd = NULL;
	U16 idx = 0;
	struct scsi_cmnd *s_cmd = NULL;
	static unsigned long j;
	ps3_msleep(ps3_task_reset_delay_time_query());

	ps3_atomic_inc(&instance->is_err_scsi_processing);
	rmb();

	ret = ps3_is_state_abnormal(instance);
	if (ret != PS3_SUCCESS) {
		LOG_INFO("hno:%u ps3_err_scsi_io_processing [%d:%d] fail, abnormal\n",
			PS3_HOST(instance), channel, id);
		ret = -PS3_FAILED;
		ps3_atomic_dec(&instance->is_err_scsi_processing);
		goto l_out;
	}

	instance->ioc_adpter->irq_disable(instance);
	INJECT_START(PS3_ERR_IJ_WAIT_IOCTL_IN_DEVICE_RESET, instance);
#ifndef _WINDOWS
	ps3_irqs_sync(instance);
#endif

	ps3_all_reply_fifo_complete(instance);

	for (idx = 0 ; idx < cmd_context->max_scsi_cmd_count; idx++) {
		cmd = ps3_cmd_find(instance, idx);
		if(cmd == NULL) {
			ret = -PS3_FAILED;
			break;
		}
		if (cmd->cmd_state.state == PS3_CMD_STATE_INIT) {
			continue;
		}
		s_cmd = cmd->scmd;
		if (s_cmd == NULL) {
			continue;
		}
		if (s_cmd->device == NULL) {
			continue;
		}
		if (PS3_SDEV_TARGET(s_cmd->device) == id &&
			PS3_SDEV_CHANNEL(s_cmd->device) == channel) {
#ifndef _WINDOWS
			scsi_print_command(s_cmd);
#endif
			LOG_WARN_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u SCSI commands pending to target"
				" tid:0x%llx [%u:%u]: frame_id:%d\n",
				PS3_HOST(instance), cmd->trace_id, channel, id, cmd->index);
			ret = -PS3_FAILED;
			break;
		}
	}

	instance->ioc_adpter->irq_enable(instance);
#ifndef _WINDOWS
	ps3_irqpolls_enable(instance);
#endif

	ps3_atomic_dec(&instance->is_err_scsi_processing);
l_out:
	return ret;
}

static void ps3_err_sas_errcode_mapping_with_sense(struct ps3_cmd *cmd,
	PS3RespFrame_u *frame)
{
	U32 sense_data_len = 0;
	struct scsi_cmnd *scmd = cmd->scmd;
	struct ps3_instance *instance = cmd->instance;
#ifndef _WINDOWS
	S32 host_status = DID_OK;
        Ps3SasDirectRespFrameIU_s frame_sas = frame->sasRespFrame;
	if (scmd == NULL || scmd->sense_buffer == NULL) {
		LOG_WARN_IN_IRQ(instance, "scmd is null\n");
		goto l_out;
	}
	scmd->result = 0;

	if (frame == NULL) {
		LOG_ERROR_IN_IRQ(instance, "direct frame is null, retry\n");
		scmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_SOFT_ERROR);
		goto l_out;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,14,0))
	scmd->result |= PS3_SCSI_RESULT_DRIVER_STATUS(DRIVER_SENSE);
#endif
	scmd->result |= PS3_SCSI_RESULT_HOST_STATUS(host_status) | frame_sas.status;

	memset(scmd->sense_buffer, 0, SCSI_SENSE_BUFFERSIZE);
	sense_data_len = min_t(U32, SCSI_SENSE_BUFFERSIZE, sizeof(frame_sas.data));
	if (sense_data_len == 0) {
		LOG_ERROR_IN_IRQ(instance, "sense_data_len len is zero\n");
		goto l_out;
	}
	memcpy(scmd->sense_buffer, frame_sas.data, sense_data_len);
	ps3_scsi_sense_print(instance, scmd->sense_buffer,
		SCSI_SENSE_BUFFERSIZE, SCMD_GET_REQUEST(scmd)->tag);
#else
	if (frame == NULL) {
		LOG_ERROR_IN_IRQ(instance, "direct frame is null, retry\n");
		scsi_cmnd_scsistatus_set(scmd, SCSI_STATUS_BUSY);
		goto l_out;
	}
	sense_data_len = min(SCSI_SENSE_BUFFERSIZE, sizeof(frame_sas.data));
        if (sense_data_len == 0) {
                LOG_ERROR_IN_IRQ(instance, "sense_data_len len is zero\n");
                goto l_out;
        }

	ps3_sense_info_set(scmd, frame_sas.data, (U16)sense_data_len);
#endif
l_out:
	return;
}

static void ps3_err_sas_errcode_mapping(struct ps3_instance *instance, struct ps3_cmd *cmd, U16 mode)
{
	struct scsi_cmnd *scmd = cmd->scmd;
	U32 channel = 0;
	U32 id = 0;
	U16 disk_id = 0;
	PS3RespFrame_u *frame = cmd->resp_frame;
	static unsigned long j;
#ifndef _WINDOWS
	channel = scmd->device->channel;
	id = scmd->device->id;
	disk_id = cmd->io_attr.disk_id;
#endif
	if (ps3_err_is_resp_from_direct_cmd(mode)){
		Ps3SasDirectRespFrameIU_s frame_sas = frame->sasRespFrame;

		switch (frame_sas.dataPres) {
			case PS3_SAS_PRES_NO_DATA:
			case PS3_SAS_PRES_REPSNSE_DATA:
			ps3_err_scsi_errcode_mapping(cmd, mode, PS3_TRUE);
			break;
		case PS3_SAS_PRES_SENSE_DATA:
			LOG_INFO_IN_IRQ(instance,
				"tid:0x%llx hno:%u [%u:%u:%u], CFID:%u data_pres:%u status:%u\n",
				cmd->trace_id, PS3_HOST(instance), channel,
				id, disk_id, cmd->index,
				frame->sasRespFrame.dataPres, frame->sasRespFrame.status);
			ps3_err_sas_errcode_mapping_with_sense(cmd, frame);
			break;
		default:
#ifndef _WINDOWS
			scmd->result |= PS3_SCSI_RESULT_HOST_STATUS(DID_ERROR);
			LOG_INFO_IN_IRQ(instance,
				"tid:0x%llx hno:%d tag:%u scmd result:%x \n",
				cmd->trace_id, PS3_HOST(cmd->instance),
				cmd->index, cmd->scmd->result);
#else
			scsi_cmnd_hoststatus_set(scmd, DID_ERROR);
				LOG_INFO_IN_IRQ(instance,
				"tid:0x%llx hno:%d tag:%u\n",
				cmd->trace_id, PS3_HOST(cmd->instance), cmd->index);
#endif
			break;
		}

	} else {
		LOG_WARN_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, \
			"tid:0x%llx hno:%u [chl:%u, id:%u, devid:%u], CFID:%u respStatus:%u\n",
			cmd->trace_id, PS3_HOST(instance), channel, id, disk_id,
			cmd->index, frame->normalRespFrame.respStatus);
		ps3_err_scsi_errcode_mapping(cmd, mode, PS3_FALSE);
	}
	return;
}

static void ps3_err_nvme_errcode_mapping(struct ps3_instance *instance,
	struct ps3_cmd *cmd, U16 mode)
{
	ps3_nvme_resp_to_scsi_status(cmd);
	ps3_err_scsi_errcode_mapping(cmd, mode, PS3_FALSE);
	(void)instance;

	LOG_INFO_IN_IRQ(instance,
		"tid:0x%llx hno:%d tag:%d scmd result:%x \n",
		cmd->trace_id, PS3_HOST(instance),
		cmd->index, cmd->scmd->result);
}

static void ps3_err_protocol_errcode_mapping(struct ps3_instance *instance,
	struct ps3_cmd *cmd, struct PS3ReplyWord *reply_word)
{
	if (reply_word->diskType == PS3_DEV_TYPE_SAS_SATA){
		ps3_err_sas_errcode_mapping(instance, cmd, reply_word->mode);
	} else {
		ps3_err_nvme_errcode_mapping(instance, cmd, reply_word->mode);
	}

	return;
}

Bool ps3_err_is_resp_from_direct_cmd(U16 mode)
{
	return ((mode == PS3_REPLY_WORD_MODE_DIRECT_OK)
		|| (mode == PS3_REPLY_WORD_MODE_DIRECT_ADVICE_TO_DIRECT));
}

S32 ps3_err_scsi_cmd_fault_proc(struct ps3_instance *instance,
	struct ps3_cmd *cmd)
{
	U16 mode = cmd->reply_word.mode;
	S32 ret = PS3_SUCCESS;

	LOG_FILE_INFO("tid:0x%llx hno:%u scsi cmd fault proc CFID:%u "
		"reply_word info: mode:%d retStatus[%02x], rettype:%d disktype:%d\n",
		cmd->trace_id, PS3_HOST(cmd->instance), cmd->index, mode,
		cmd->reply_word.retStatus, cmd->reply_word.retType, cmd->reply_word.diskType);

	if(cmd->reply_word.retType == 0) {
		if ((cmd->io_attr.dev_type != PS3_DEV_TYPE_VD)
			&& ps3_err_is_resp_from_direct_cmd(mode)) {
			if (ps3_err_retry_sys_state_check(instance)) {
				LOG_FILE_INFO("hno:%u Direct cmd needs retry!"
					" [scmd cmd_flags:0x%llx, retries:%d, allowd:%d]\n",
					PS3_HOST(instance), (U64)SCMD_GET_REQUEST(cmd->scmd)->cmd_flags,
					cmd->scmd->retries, cmd->scmd->allowed);

				if ((ps3_is_need_direct_to_normal(instance, cmd))
					&& (ps3_err_code_is_need_change(cmd))) {
					cmd->scmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_SOFT_ERROR);
				} else {
					ps3_err_protocol_errcode_mapping(instance, cmd, &(cmd->reply_word));
				}
			} else {
				cmd->scmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT);
			}
		} else {
			ps3_err_scsi_errcode_mapping(cmd, mode, PS3_FALSE);
		}
	}else {
		if (ps3_err_retry_sys_state_check(instance)) {
			LOG_FILE_INFO("hno:%u Hil hardware err,need retry!"
				" [scmd cmd_flags:0x%llx, retries:%d, allowd:%d]\n",
				PS3_HOST(instance), SCMD_GET_REQUEST(cmd->scmd)->cmd_flags,
				cmd->scmd->retries, cmd->scmd->allowed);

			cmd->scmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_SOFT_ERROR);
		} else {
			cmd->scmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT);
		}
	}
	if (cmd->scmd->result == SCSI_STATUS_TASK_SET_FULL) {
		ps3_qos_adjust_pd_rsc(cmd->scmd->device, instance, PS3_QOS_QUOTA_ADJUST_QFULL);
	}
	return ret;
}

S32 ps3_err_scsi_task_pre_check(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	if (!instance->state_machine.is_load) {
		LOG_WARN("hno:%u instance state not is_load\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	if(!ps3_is_instance_state_normal(instance)) {
		ret = -PS3_FAILED;
		goto l_out;
	}

	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN("hno:%u host in pci recovery\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}
l_out:
	return ret;
}

void ps3_scsih_drv_io_reply_scsi(struct scsi_cmnd *s_cmd, struct ps3_cmd *cmd, U8 resp_status, Bool cmd_lock)
{
	struct  ps3_instance *instance = cmd->instance;
	struct ps3_scsi_priv_data *data = NULL;

	if (cmd_lock) {
		ps3_mutex_lock(&instance->recovery_context->free_cmd_lock);
		LOG_INFO("hno:%u tid:0x%llx CFID:%u drv reply scsi independently!\n",
			PS3_HOST(instance), cmd->trace_id, cmd->index);
	}
	if (likely(cmd->scmd == NULL)) {
		LOG_DEBUG("scmd is has reply scsi\n");
		goto l_out;
	}

	PS3_IO_OUTSTAND_DEC(instance, s_cmd);
	PS3_VD_OUTSTAND_DEC(instance, s_cmd);

	PS3_IO_BACK_ERR_INC(instance, s_cmd);
	PS3_DEV_BUSY_DEC(s_cmd);

	ps3_errcode_to_scsi_status(cmd->instance, s_cmd, resp_status, NULL, 0, cmd);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0))
	s_cmd->SCp.ptr = NULL;
#endif
	ps3_scsi_dma_unmap(cmd);
	data = (struct ps3_scsi_priv_data *)s_cmd->device->hostdata;
	if(likely(data != NULL)){
		ps3_r1x_write_unlock(&data->lock_mgr, cmd);
	}

	if (cmd->r1x_peer_cmd != NULL) {
		LOG_DEBUG("host_no:%u task repaly r1x scsi write CFID:%d and CFID:%d\n",
			PS3_HOST(instance), cmd->index, cmd->r1x_peer_cmd->index);
		if (!ps3_r1x_peer_cmd_free_nolock(cmd->r1x_peer_cmd)) {
			ps3_scsi_cmd_free(cmd->r1x_peer_cmd);
		}

		if (!ps3_r1x_peer_cmd_free_nolock(cmd)) {
			ps3_scsi_cmd_free(cmd);
		}
	} else {
		ps3_scsi_cmd_free(cmd);
	}
	SCMD_IO_DONE(s_cmd);

l_out:
	if (cmd_lock) {
		ps3_mutex_unlock(&instance->recovery_context->free_cmd_lock);
	}
	return ;
}

S32 ps3_err_scsi_task_mgr_abort(struct scsi_cmnd *scmd)
{
#ifdef _WINDOWS
	(void)scmd;
#else
	S32 ret = SUCCESS;

	struct ps3_cmd *aborted_cmd = NULL;
	struct ps3_cmd *aborted_peer_cmd = NULL;
	U16 cmd_frame_id = 0;
	struct ps3_instance *instance = scsi_host_data(scmd);
	struct ps3_scsi_priv_data *scsi_priv_data = scsi_device_private_data(scmd);
	U32 cmd_flighting_wait_cnt = 0;

	LOG_WARN("hno:%u enter, ready aborted CFID:%u op:0x%x "
		"chl:%u id:%u\n", PS3_HOST(instance), SCMD_GET_REQUEST(scmd)->tag,
		scmd->cmnd[0], ps3_scsi_channel_query(scmd),
		ps3_scsi_target_query(scmd));

	INJECT_START(PS3_ERR_IJ_SCSI_TASK_PRE_CHECK_FAILED, instance)
	if (ps3_err_scsi_task_pre_check(instance) != PS3_SUCCESS) {
		LOG_WARN("hno:%u pre check NOK!\n",
			PS3_HOST(instance));
		ret = FAILED;
		goto l_out;
	}

	if (!scsi_priv_data->is_taskmgmt_enable) {
		LOG_ERROR("hno:%u task mgmt is disable!\n",
			PS3_HOST(instance));
		ret = FAILED;
		goto l_out;
	}
	cmd_frame_id = SCMD_GET_REQUEST(scmd)->tag;
	INJECT_START(PS3_ERR_IJ_ABORT_PRE1_FORCE_ABORTED_CMD_DONE, scmd)
	aborted_cmd = ps3_cmd_find(instance, cmd_frame_id);
	if (aborted_cmd == NULL || aborted_cmd->scmd == NULL) {
		LOG_INFO("host_no:%u there is no aborted cmd CFID:%u\n",
			PS3_HOST(instance), cmd_frame_id);

		ret = SUCCESS;
		goto l_out;
	}
	INJECT_START(PS3_ERR_IJ_ABORT_PRE2_FORCE_ABORTED_CMD_DONE, scmd)

	LOG_FILE_WARN("tid:0x%llx hno:%u task aborted CFID:%u ready \n",
		aborted_cmd->trace_id, PS3_HOST(instance), cmd_frame_id);

	scsi_priv_data->task_manager_busy = 1;

	while(aborted_cmd->flighting &&
		cmd_flighting_wait_cnt < PS3_ABORT_WAIT_CMD_FLIGHT_END) {
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
		cmd_flighting_wait_cnt++;
	}

	if (aborted_cmd->flighting) {
		ret = FAILED;
		LOG_WARN("task abort wait cmd filghting end NOK. hno:%u tid:0x%llx CFID:%u\n",
			 PS3_HOST(instance), aborted_cmd->trace_id, aborted_cmd->index);
		goto l_busy_clean_out;
	}

	LOG_DEBUG("hno:%u task abort wait flighting cmd end\n",
		 PS3_HOST(instance));

	if (ps3_r1x_conflict_queue_abort(aborted_cmd, scmd)) {
		ret = SUCCESS;
		goto l_busy_clean_out;
	}

	if (ps3_qos_waitq_abort(aborted_cmd)) {
		ret = SUCCESS;
		goto l_busy_clean_out;
	}

	aborted_peer_cmd = aborted_cmd->r1x_peer_cmd;
	if (aborted_peer_cmd != NULL) {
		INJECT_START(PS3_ERR_IJ_WAIT_CMD_ALLOC1, aborted_peer_cmd);
		ps3_mutex_lock(&instance->task_abort_lock);
		aborted_peer_cmd->is_aborting = 1;
		wmb();
		INJECT_START(PS3_ERR_IJ_WAIT_CMD_FREE, aborted_cmd);
		if (aborted_cmd->r1x_peer_cmd == NULL) {
			INJECT_START(PS3_ERR_IJ_WAIT_CMD_ALLOC, aborted_peer_cmd);
			INJECT_START(PS3_ERR_IJ_WAIT_CMD_ALLOC2, aborted_peer_cmd);
			INJECT_START(PS3_ERR_IJ_WAIT_CMD_FREE, aborted_peer_cmd);
			aborted_peer_cmd->is_aborting = 0;
			INJECT_START(PS3_ERR_IJ_WAIT_CMD_ALLOC2, aborted_peer_cmd);
			ps3_mutex_unlock(&instance->task_abort_lock);
			goto l_busy_clean_out;
		}
		ps3_mutex_unlock(&instance->task_abort_lock);
		INJECT_START(PS3_ERR_IJ_WAIT_CMD_FREE1, aborted_cmd);
		if (!aborted_peer_cmd->is_r1x_scsi_complete) {
			ret = ps3_scsi_task_mgr_abort(instance, scsi_priv_data,
				aborted_peer_cmd->index, scmd);
			ret = (ret == PS3_SUCCESS) ? SUCCESS : FAILED;
			aborted_peer_cmd->is_aborting = 0;
			if (ret == SUCCESS) {
				LOG_INFO("hno:%u task abort peer cmd:%d success !\n",
					PS3_HOST(instance), aborted_peer_cmd->index);
				aborted_peer_cmd = ps3_cmd_find(instance,
					aborted_peer_cmd->index);
				if (unlikely(aborted_peer_cmd == NULL)) {
					ret = FAILED;
					goto l_busy_clean_out;
				}
			} else {
				goto l_busy_clean_out;
			}
		}
		aborted_peer_cmd->is_aborting = 0;
	}

	ret = ps3_scsi_task_mgr_abort(instance, scsi_priv_data,
		cmd_frame_id, scmd);
	ret = (ret == PS3_SUCCESS) ? SUCCESS : FAILED;

	if(ret == SUCCESS) {
		LOG_INFO("hno:%u task abort cmd success !\n", PS3_HOST(instance));
		aborted_cmd = ps3_cmd_find(instance, cmd_frame_id);
		if (aborted_cmd == NULL) {
			ret = FAILED;
		} else {
			if (unlikely(aborted_cmd->scmd)) {
				LOG_WARN("task abort success but scmd is not freed. hno:%u tid:0x%llx CFID:%u\n",
					 PS3_HOST(instance), aborted_cmd->trace_id, cmd_frame_id);
				ret = FAILED;
			}
			ps3_can_queue_depth_update(instance);
		}
	}
l_busy_clean_out:
	scsi_priv_data->task_manager_busy = 0;
	wmb();
l_out:
	LOG_WARN("hno:%u quit ret:%#x\n",PS3_HOST(instance), ret);
	return ret;
#endif
	return -PS3_FAILED;
}

void ps3_set_task_manager_busy(struct ps3_instance *instance, U32 channel, U32 id, U32 state)
{
	struct ps3_scsi_priv_data *scsi_priv_data = NULL;

	ps3_mutex_lock(&instance->dev_context.dev_priv_lock);

	scsi_priv_data = ps3_dev_mgr_lookup_vd_pri_data(instance, channel, id);
	if (scsi_priv_data == NULL) {
		ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
		LOG_ERROR("hno:%u device has been deleted.channel:%u, id:%u!\n",
			PS3_HOST(instance), channel, id);
		goto l_out;
	}
	scsi_priv_data->task_manager_busy = state;

	ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);

l_out:
	return;
}

S32 ps3_err_scsi_task_mgr_reset(struct scsi_cmnd *scmd)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_instance *instance = scsi_host_data(scmd);
	struct ps3_scsi_priv_data *scsi_priv_data = NULL;
	struct ps3_scsi_priv_data pending_priv_data;
	struct ps3_cmd * cmd = NULL;

	LOG_INFO("hno:%u enter device reset channel:%u id:%u\n",
		PS3_HOST(instance), PS3_SDEV_CHANNEL(scmd->device),
		PS3_SDEV_TARGET(scmd->device));

	ps3_mutex_lock(&instance->dev_context.dev_priv_lock);
	INJECT_START(PS3_ERR_IJ_PRIV_DATA_NULL1, scmd->device)
	scsi_priv_data = scsi_device_private_data(scmd);
	if(scsi_priv_data == NULL) {
		ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
		LOG_ERROR("hno:%u device has been deleted!\n",
			PS3_HOST(instance));
#ifndef _WINDOWS
		scmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT);
		ret = SUCCESS;
#else
		scsi_cmnd_hoststatus_set(scmd, DID_ERROR);
#endif
		goto l_out;
	}

#ifndef _WINDOWS
	LOG_WARN("hno:%u enter ready reset[%u:%u:%u:%#x], CFID:%d op:0x%x\n",
		PS3_HOST(instance), scsi_priv_data->disk_pos.diskDev.ps3Dev.softChan,
		scsi_priv_data->disk_pos.diskDev.ps3Dev.devID,
		scsi_priv_data->disk_pos.diskDev.ps3Dev.phyDiskID,
		scsi_priv_data->disk_pos.diskMagicNum, SCMD_GET_REQUEST(scmd)->tag,
		scmd->cmnd[0]);
#else
	LOG_WARN("hno:%u enter ready reset[%u:%u:%u:%#x], op:0x%x\n",
		PS3_HOST(instance), scsi_priv_data->disk_pos.diskDev.ps3Dev.softChan,
		scsi_priv_data->disk_pos.diskDev.ps3Dev.devID,
		scsi_priv_data->disk_pos.diskDev.ps3Dev.phyDiskID,
		scsi_priv_data->disk_pos.diskMagicNum,
		scsi_cmnd_cdb(scmd)[0]);
#endif

	if (ps3_err_scsi_task_pre_check(instance) != PS3_SUCCESS) {
		LOG_WARN("hno:%u pre check NOK!\n",
			PS3_HOST(instance));
#ifndef _WINDOWS
		ret = FAILED;
#else
		ret = -PS3_FAILED;
#endif
		ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
		goto l_out;
	}

	if (!scsi_priv_data->is_taskmgmt_enable) {
		LOG_ERROR("hno:%u task mgmt is disable!\n",
			PS3_HOST(instance));
#ifndef _WINDOWS
		ret = FAILED;
#else
		ret = -PS3_FAILED;
#endif
		ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
		goto l_out;
	}

	LOG_INFO("hno:%u task ready reset[%u:%u:%u] \n",PS3_HOST(instance),
		scsi_priv_data->disk_pos.diskDev.ps3Dev.softChan,
		scsi_priv_data->disk_pos.diskDev.ps3Dev.devID,
		scsi_priv_data->disk_pos.diskDev.ps3Dev.phyDiskID);
	scsi_priv_data->task_manager_busy = 1;

	ps3_wait_scsi_cmd_done(instance, PS3_TRUE);
	LOG_WARN("hno[%u], task reset wait scsi delivering cmd count[%d] !\n", PS3_HOST(instance),
		ps3_atomic_read(&instance->cmd_statistics.cmd_delivering));
	while (ps3_atomic_read(&instance->cmd_statistics.cmd_delivering) != 0){
		LOG_INFO("host_no:%u scsi commands in delivering, wait a while!\n",
			PS3_HOST(instance));
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
	}
	LOG_WARN("hno:%u task reset wait delivering cmd end !\n", PS3_HOST(instance));
	
	ps3_r1x_conflict_queue_clean(scsi_priv_data, SCSI_STATUS_TASK_ABORTED);
	memcpy(&pending_priv_data, scsi_priv_data, sizeof(struct ps3_scsi_priv_data));

	ps3_qos_device_clean(instance, scsi_priv_data, SCSI_STATUS_TASK_ABORTED);
	ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);

	cmd = ps3_scsi_task_mgr_reset_build(instance, &pending_priv_data);
	if (cmd == NULL) {
		LOG_ERROR("hno:%u task reset cmd NOK!\n",
			PS3_HOST(instance));
		ret = FAILED;
		goto l_out;
	}

	ret = ps3_scsi_task_mgr_reset(instance, cmd);
#ifndef _WINDOWS
	ret = (ret == PS3_SUCCESS) ? SUCCESS : FAILED;

	if (ret == SUCCESS) {
		LOG_INFO("hno:%u target reset success !\n", PS3_HOST(instance));
		ret = ps3_err_scsi_io_processing(instance, scmd->device->id,
			scmd->device->channel);
		INJECT_START(PS3_ERR_IJ_HT_ABNORMAL1, &ret)
		ret = (ret == PS3_SUCCESS) ? SUCCESS : FAILED;
		ps3_can_queue_depth_update(instance);
	}
#else
	if (ret == PS3_SUCCESS) {
		LOG_INFO("hno:%u target reset success !\n", PS3_HOST(instance));
		ret = ps3_err_scsi_io_processing(instance, PS3_SDEV_TARGET(scmd->device),
			PS3_SDEV_CHANNEL(scmd->device));
		ret = (ret == PS3_SUCCESS) ? SUCCESS : FAILED;
	}
#endif
	ps3_mutex_lock(&instance->dev_context.dev_priv_lock);
	INJECT_START(PS3_ERR_IJ_PRIV_DATA_NULL2, scmd->device)
	scsi_priv_data = scsi_device_private_data(scmd);
	if (scsi_priv_data == NULL) {
		ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
		LOG_ERROR("hno:%u device has been deleted!\n",
			PS3_HOST(instance));
		goto l_out;
	}
	scsi_priv_data->task_manager_busy = 0;
	ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
l_out:
	LOG_WARN("hno:%u quit ret:%#x\n",PS3_HOST(instance), ret);
	return ret;
}

S32 ps3_device_reset_handler(struct scsi_cmnd *scmd)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_instance *instance = scsi_host_data(scmd);

	LOG_INFO("hno:%u ready device[%u:%u] reset!\n",
		PS3_HOST(instance), PS3_SDEV_CHANNEL(scmd->device),
		PS3_SDEV_TARGET(scmd->device));

	ps3_mutex_lock(&instance->task_mgr_reset_lock);
	instance->task_manager_host_busy = PS3_TRUE;
	ret = ps3_err_scsi_task_mgr_reset(scmd);
	instance->task_manager_host_busy = PS3_FALSE;
	ps3_mutex_unlock(&instance->task_mgr_reset_lock);

	return ret;
}

static S32 ps3_hard_reset_pre_check(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;
	Bool is_support_halt = PS3_IOC_STATE_HALT_SUPPORT(instance);
	U32 hard_reset_enable = ps3_hard_reset_enable_query();

	cur_state = ps3_atomic_read(&instance->state_machine.state);

	LOG_WARN("hno:%u ready to host reset,instance state: %s\n",
		PS3_HOST(instance), namePS3InstanceState(cur_state));

	if(is_support_halt && (cur_state != PS3_INSTANCE_STATE_DEAD)){
		ps3_atomic_set(&instance->state_machine.state,
			PS3_INSTANCE_STATE_DEAD);
		instance->ioc_adpter->ioc_force_to_halt(instance);
		LOG_INFO("host_no:%u instance state while support halt!\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
	}

	if ((cur_state = ps3_atomic_read(&instance->state_machine.state)) ==
		PS3_INSTANCE_STATE_DEAD && (!hard_reset_enable || is_support_halt
			|| (!PS3_IOC_HARD_RECOVERY_SUPPORT(instance)))) {
		LOG_INFO("hno:%u host reset, instance state: %s\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state));
		ret = -PS3_FAILED;
	}

	LOG_INFO("hno:%u host reset, instance state: %s\n",
		PS3_HOST(instance), namePS3InstanceState(cur_state));

	return ret;
}
static S32 ps3_host_reset_pre_check(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	S32 cur_state = ps3_atomic_read(&instance->state_machine.state);

	LOG_WARN("hno:%u ready to host reset,instance state: %s\n",
		PS3_HOST(instance), namePS3InstanceState(cur_state));
	if (PS3_DEVICE_IS_SWITCH(instance->dev_id) &&
		instance->is_scan_host_finish == PS3_FALSE) {
		LOG_INFO("host_no:%u instance deviceId:%u is probe, exit failed!\n",
			PS3_HOST(instance), instance->dev_id);
		ret = -PS3_FAILED;
		goto l_out;
	}

	if (cur_state == PS3_INSTANCE_STATE_QUIT || cur_state == PS3_INSTANCE_STATE_DEAD ||
		!instance->state_machine.can_hostreset) {
		LOG_ERROR("hno:%u cannot perform host reset due to %s\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state));
		ret = -PS3_FAILED;
	} else {
		LOG_INFO("hno:%u host reset, instance state: %s\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state));
	}
l_out:
	return ret;
}
S32 ps3_wait_for_outstanding_complete(struct ps3_instance *instance)
{
	U32 i = 0;
	U32 ioc_state = PS3_FW_STATE_UNDEFINED;
	S32 ret = -PS3_FAILED;
	U32 waittime_for_io_completion = PS3_WAIT_FOR_OUTSTANDING_IO_COMPLETE;
	U32 interval = 1000;
	U32 count = 0;

	for (i = 0; i < waittime_for_io_completion * 10 / interval; i++) {
		if (!ps3_ioc_state_get_with_check(instance, &ioc_state)) {
			LOG_ERROR_LIM("host_no:%u get ioc state NOK\n",
				PS3_HOST(instance));
			if (count++ < 3) {
				continue;
			}
			goto l_out;
		}
		if (ioc_state == PS3_FW_STATE_FAULT ||
			ioc_state == PS3_FW_STATE_CRITICAL) {
			LOG_WARN("host_no:%u ioc_state:0x%x\n",
				PS3_HOST(instance), ioc_state);
			goto l_out;
		}

		if(atomic_read(&instance->cmd_statistics.io_outstanding) == 0) {
			LOG_WARN("host_no:%u io_outstanding is zero\n",
				PS3_HOST(instance));
			ret = PS3_SUCCESS;
			goto l_out;
		}

		ps3_msleep(interval);
	}

l_out:
	return ret;
}

S32 ps3_reset_host(struct ps3_instance *instance)
{
	S32 ret = SUCCESS;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;
	LOG_INFO("hno:%u Host reset is requested, IOC outstanding: %d!!\n",
		PS3_HOST(instance),
		ps3_atomic_read(&instance->cmd_statistics.cmd_outstanding));
	ps3_atomic_inc(&instance->host_reset_processing);
	if (ps3_host_reset_pre_check(instance) != PS3_SUCCESS) {
		ret = FAILED;
		goto l_out;
	}
	do {
		if (unlikely(ps3_pci_err_recovery_get(instance))) {
			LOG_WARN_LIM("hno:%u host in pci recovery during reset request\n",
				PS3_HOST(instance));
		} else {
			ps3_mutex_lock(&instance->state_machine.lock);
			if (instance->recovery_context->host_reset_state == PS3_HOST_RESET_INIT) {
				instance->recovery_context->host_reset_state = PS3_HOST_RESET_START;
				ps3_mutex_unlock(&instance->state_machine.lock);
				break;
			}
			ps3_mutex_unlock(&instance->state_machine.lock);
		}
		ps3_msleep(100);
	} while(1);


	ps3_mutex_lock(&instance->state_machine.lock);
	if (instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE) {
		do{
			ps3_mutex_unlock(&instance->state_machine.lock);
			ps3_msleep(100);

			ps3_mutex_lock(&instance->state_machine.lock);
			if (likely(instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE)) {
				continue;
			} else {
				ps3_mutex_unlock(&instance->state_machine.lock);
				cancel_work_sync(&instance->recovery_context->recovery_work);
				goto wait_hard_reset;
			}
		}while(1);
	} else if (instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_SHALLOW){
		ps3_mutex_unlock(&instance->state_machine.lock);
		cancel_work_sync(&instance->recovery_context->recovery_work);
	} else {
		ps3_mutex_unlock(&instance->state_machine.lock);
	}
	ps3_mutex_lock(&instance->state_machine.lock);
	if (unlikely(instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE)) {
		instance->recovery_context->recovery_state = PS3_HARD_RECOVERY_FINISH;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);

	if(ps3_wait_for_outstanding_complete(instance) == PS3_SUCCESS) {
		ret = SUCCESS;
		goto l_out;
	}

	if (ps3_host_reset_pre_check(instance) != PS3_SUCCESS) {
		ret = FAILED;
		goto l_out;
	}

	if ((ret = ps3_hard_reset_pre_check(instance) ) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u hard reset pre check NOK, %s!\n",
			PS3_HOST(instance),
			namePS3InstanceState(ps3_atomic_read(&instance->state_machine.state)));
		ret = FAILED;
		goto l_wait;
	}

	ps3_mutex_lock(&instance->state_machine.lock);
	if (instance->recovery_context->host_reset_state == PS3_HOST_RESET_HARD_RESET_DONE) {
		instance->recovery_context->host_reset_state = PS3_HOST_RESET_START;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);

	ret = ps3_hard_recovery_request_with_retry(instance);
	if (ret != PS3_SUCCESS) {
		cur_state = ps3_atomic_read(&instance->state_machine.state);
		LOG_ERROR("hno:%u recovery request NOK, %s!\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state));
		ret = FAILED;
		goto l_wait;
	}

wait_hard_reset:
	ret = ps3_instance_wait_for_hard_reset_flag_done(instance);
	if (ret == PS3_SUCCESS) {
		cur_state = ps3_atomic_read(&instance->state_machine.state);
		LOG_INFO("hno:%u recovery success, %s!\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state));
		ret = SUCCESS;
		goto l_wait;
	} else if(ret == -PS3_RETRY){
		ps3_mutex_lock(&instance->state_machine.lock);
		if (unlikely(instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE)) {
			instance->recovery_context->recovery_state = PS3_HARD_RECOVERY_FINISH;
		}
		ps3_mutex_unlock(&instance->state_machine.lock);
		ret = ps3_hard_recovery_request_with_retry(instance);
		if (ret != PS3_SUCCESS) {
			cur_state = ps3_atomic_read(&instance->state_machine.state);
			LOG_ERROR("hno:%u recovery request NOK, %s!\n",
				PS3_HOST(instance), namePS3InstanceState(cur_state));
			ret = FAILED;
			goto l_wait;
		}
		ret = ps3_instance_wait_for_hard_reset_flag_done(instance);
		if (ret != PS3_SUCCESS) {
			cur_state = ps3_atomic_read(&instance->state_machine.state);
			LOG_ERROR("hno:%u recovery NOK, %s,host_reset_state:%d!\n",
				PS3_HOST(instance),namePS3InstanceState(cur_state),
				instance->recovery_context->host_reset_state);
			ret = FAILED;
		} else {
			ret = SUCCESS;
		}
	}else {
		ret = FAILED;
		goto l_out;
	}

l_wait:
	while(atomic_read(&instance->cmd_statistics.io_outstanding) != 0) {
		ps3_msleep(3000);
	}
l_out:
	if (!instance->state_machine.is_load &&
		!instance->is_suspend) {
		instance->state_machine.can_hostreset = PS3_FALSE;
	}
	INJECT_START(PS3_ERR_IJ_BLOCK_HOST_RESET, instance)
	cur_state = ps3_atomic_read(&instance->state_machine.state);
	if (cur_state == PS3_INSTANCE_STATE_DEAD && 
		instance->state_machine.can_hostreset) {
		ps3_r1x_conflict_queue_clean_all(instance,
			PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT), PS3_TRUE);
		ps3_qos_waitq_clear_all(instance, PS3_STATUS_HOST_NOT_FOUND);
	}
	ps3_mutex_lock(&instance->state_machine.lock);
	instance->recovery_context->host_reset_state = PS3_HOST_RESET_INIT;
	ps3_mutex_unlock(&instance->state_machine.lock);
	ps3_atomic_dec(&instance->host_reset_processing);
	LOG_WARN("hno:%u host reset hard recovery %d!\n",
		PS3_HOST(instance), ret);
	return ret;
}

S32 ps3_err_reset_target(struct scsi_cmnd *scmd)
{
#ifndef _WINDOWS
	LOG_WARN("hno:%u enter target reset channel:%u id:%u\n",
		PS3_HOST(scsi_host_data(scmd)),
		PS3_SDEV_CHANNEL(scmd->device), PS3_SDEV_TARGET(scmd->device));

	return ps3_device_reset_handler(scmd);
#else
	(void)scmd;
	return PS3_SUCCESS;
#endif
}

S32 ps3_err_reset_host(struct scsi_cmnd *scmd)
{
	S32 ret = 0;
#ifndef _WINDOWS
	struct ps3_instance *instance =
		(struct ps3_instance*)scmd->device->host->hostdata;
#else
	struct ps3_instance *instance = scsi_host_data(scmd);
#endif

	ret = ps3_reset_host(instance);
	return ret;
}

#if ((defined(RHEL_MAJOR) && (RHEL_MAJOR == 9) && (RHEL_MINOR >= 3)) || \
     (LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0)))
enum scsi_timeout_action ps3_err_reset_timer(struct scsi_cmnd *scmd)
#else
enum blk_eh_timer_return ps3_err_reset_timer(struct scsi_cmnd *scmd)
#endif
{
	struct ps3_instance *instance = NULL;
	ULong flags = 0;

	instance = (struct ps3_instance *)scmd->device->host->hostdata;
	if (time_after(jiffies, scmd->jiffies_at_alloc +
		(SCMD_GET_REQUEST(scmd)->timeout * 2))) {
		LOG_FILE_WARN("reset timer not handled [%u:%u:%u:%llu], tag:%d op:0x%x "
			"timeout:%u alloc_time:%lu cur_time:%lu\n",
			PS3_HOST(instance), scmd->device->channel,
			scmd->device->id, (U64)scmd->device->lun, SCMD_GET_REQUEST(scmd)->tag,
			scmd->cmd_len > 0 ? scmd->cmnd[0] : 0xff,
			SCMD_GET_REQUEST(scmd)->timeout, scmd->jiffies_at_alloc, jiffies);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,16,0)
		return BLK_EH_NOT_HANDLED;
#elif ((defined(RHEL_MAJOR) && (RHEL_MAJOR == 9) && (RHEL_MINOR >= 3)) || \
			 (LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0)))
		return SCSI_EH_NOT_HANDLED;
#else
		return BLK_EH_DONE;
#endif
	}

	if (instance->is_support_io_limit &&
			!instance->fault_context.ioc_busy) {

		spin_lock_irqsave(instance->host->host_lock, flags);
		instance->host->can_queue =
			instance->cmd_attr.throttle_que_depth;
		instance->fault_context.last_time = jiffies;
		instance->fault_context.ioc_busy = PS3_DRV_TRUE;

		spin_unlock_irqrestore(instance->host->host_lock, flags);
	}

	LOG_WARN_LIM("reset timer [%u:%u:%u:%llu], tag:%d op:0x%x "
		"timeout:%u alloc_time:%lu cur_time:%lu\n",
		PS3_HOST(instance), scmd->device->channel,
		scmd->device->id, (U64)scmd->device->lun, SCMD_GET_REQUEST(scmd)->tag,
		scmd->cmd_len > 0 ? scmd->cmnd[0] : 0xff,
		SCMD_GET_REQUEST(scmd)->timeout, scmd->jiffies_at_alloc, jiffies);

#if ((defined(RHEL_MAJOR) && (RHEL_MAJOR == 9) && (RHEL_MINOR >= 3)) || \
(LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0)))
	return SCSI_EH_RESET_TIMER;
#else
	return BLK_EH_RESET_TIMER;
#endif
}
static void ps3_scsi_sense_print(struct ps3_instance *instance,
	const U8 *sense_buffer, U8 len, S32 CFID)
{
	ps3_scsi_sense_hdr_s sshdr;
	memset(&sshdr, 0, sizeof(sshdr));

	(void)instance;
	if (sense_buffer == NULL || len == 0) {
		goto l_out;
	}

	sshdr.resp_code = (sense_buffer[0] & PS3_SENSE_RESP_CODE_MASK);
	if ((sshdr.resp_code & PS3_SENSE_RESP_CODE_VALID_MASK) !=
		PS3_SENSE_RESP_CODE_VALID_MASK) {
		LOG_WARN_IN_IRQ(instance,
			"CFID:%d sense rcode invalid:0x%x\n", CFID,
			sshdr.resp_code);
		goto l_out;
	}

	if (sshdr.resp_code >= PS3_SENSE_RESP_CODE_DESC_FORMAT) {
		if (len > 1)
			sshdr.sense_key = (sense_buffer[1] & PS3_SENSE_KEY_MASK);
		if (len > 2)
			sshdr.asc = sense_buffer[2];
		if (len > 3)
			sshdr.ascq = sense_buffer[3];
		if (len > 7)
			sshdr.additional_len = sense_buffer[7];
	} else {
		if (len > 2)
			sshdr.sense_key = (sense_buffer[2] & PS3_SENSE_KEY_MASK);
		if (len > 7) {
			len = (len < (sense_buffer[7] + 8)) ? len :
				(sense_buffer[7] + 8);
			if (len > 12)
				sshdr.asc = sense_buffer[12];
			if (len > 13)
				sshdr.ascq = sense_buffer[13];
		}
	}

	LOG_WARN_IN_IRQ(instance,
		"sense: CFID:%d, rcode:0x%x, key:0x%x, asc:0x%x, ascq:0x%x\n",
		CFID, sshdr.resp_code, sshdr.sense_key, sshdr.asc, sshdr.ascq);
l_out:
	return;
}
void ps3_check_and_wait_host_reset(struct ps3_instance *instance)	
{	
	while(ps3_atomic_read(&instance->host_reset_processing) != 0){	
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);	
	}	

return;
}
