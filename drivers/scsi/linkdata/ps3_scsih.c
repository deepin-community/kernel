#include "ps3_scsih.h"

#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/compiler.h>
#include <scsi/scsi.h>
#include "ps3_module_para.h"
#include <linux/log2.h>
#include <linux/jiffies.h>

#endif

#include "ps3_htp.h"
#include "ps3_meta.h"
#include "ps3_err_def.h"
#include "ps3_instance_manager.h"
#include "ps3_cmd_channel.h"
#include "ps3_scsi_cmd_err.h"
#include "ps3_scsih_cmd_parse.h"
#include "ps3_util.h"
#include "ps3_cmd_statistics.h"
#include "ps3_scsih_raid_engine.h"
#include "ps3_module_para.h"
#include "ps3_device_manager.h"
#include "ps3_r1x_write_lock.h"
#include "ps3_io_trace.h"
#include "ps3_htp_sas.h"

static S32 ps3_scsih_vd_req_frame_build(struct ps3_cmd *cmd);
static S32 ps3_scsih_sas_hw_req_frame_build(struct ps3_cmd *cmd,
	U16 disk_id, U64 data_addr, U32 sge_count);
static S32 ps3_scsih_sas_req_frame_build(struct ps3_cmd *cmd);
static S32 ps3_scsih_sata_req_frame_build(struct ps3_cmd *cmd);
static S32 ps3_scsih_nvme_req_frame_build(struct ps3_cmd *cmd);
static inline U16 ps3_scsih_is_use_hard_cmd(const struct ps3_cmd *cmd);
static S32 ps3_vd_access_policy_check(struct ps3_instance *instance,
	U8 channel, U16 id, struct scsi_cmnd *s_cmd);
static U16 ps3_scsih_frontend_data_buf_build(struct ps3_cmd *cmd,
	PS3FrontEndReqFrame_s *req);
static void ps3_scsih_req_frame_head_build(struct ps3_cmd *cmd,
	U8 req_frame_format);
static inline void ps3_scsih_print_req_head(struct ps3_cmd *cmd, U8 log_level);
static U32 ps3_scsih_datelen_calc(const struct ps3_cmd *cmd);

#ifdef _WINDOWS
static void ps3_scsi_need_remap_check(struct ps3_cmd *cmd);
static S32 ps3_scsi_remap_sgl(struct ps3_cmd *cmd);
#endif

#define PS3_CMD_WORD_QUE_CALC(vd_id, lba, map_block, que_num) \
	(((vd_id) + ((lba) / (map_block))) & que_num)

#define PS3_SCSIH_NOT_SAME_MAPBLOCK(lba, map_block, num_blocks) \
	(((map_block) - ((lba) % (map_block))) < (num_blocks))
#define PS3_LBA(lba_hi, lba_lo) \
	((((U64)(lba_hi)) << PS3_SHIFT_DWORD) | (lba_lo))

#ifndef _WINDOWS
#define sg_dma_address_u64(sg) sg_dma_address(sg)
#endif

#define UNMAP_PARAM_LEN (8) 
#define UNMAP_DESCRIPTOR_LEN (16) 
#define UNMAP_CDB_VALID_LEN (10) 

static struct disk_type_to_proc_func_table g_req_frame_func_table[] = {
	{PS3_DEV_TYPE_VD,           ps3_scsih_vd_req_frame_build},
	{PS3_DEV_TYPE_SAS_HDD,      ps3_scsih_sas_req_frame_build},
	{PS3_DEV_TYPE_SATA_HDD,     ps3_scsih_sata_req_frame_build},
	{PS3_DEV_TYPE_SATA_SSD,     ps3_scsih_sata_req_frame_build},
	{PS3_DEV_TYPE_SAS_SSD,      ps3_scsih_sas_req_frame_build},
	{PS3_DEV_TYPE_NVME_SSD,     ps3_scsih_nvme_req_frame_build},
	{PS3_DEV_TYPE_SES,          ps3_scsih_sas_req_frame_build},
	{PS3_DEV_TYPE_VEP,			ps3_scsih_sas_req_frame_build}

};
#ifndef _WINDOWS

Bool ps3_scsih_stream_is_detect(struct ps3_cmd *cmd)
{
	struct ps3_scsi_priv_data *data =
		(struct ps3_scsi_priv_data *)cmd->scmd->device->hostdata;

	struct ps3_vd_stream_detect *current_ld_sd;
	u64 *track_stream;
	u8 stream_num;
	u64 shifted_values, unshifted_values;
	u64 index_value_mask, shifted_values_mask;
	u32 index;
	struct ps3_stream_detect *current_sd;
	u64 lba = 0;
	u32 len = 0;
	Bool is_stream = PS3_FALSE;
	U32 typeIndex;
	u32 type = PS3_SCSI_CMD_TYPE(ps3_scsih_cdb_rw_type_get(cmd->scmd->cmnd));
	ULong flag = 0;
	S64 value = 0;

	if (PS3_IF_QUIT_STREAM_DIRECT_DETECT()) {
		goto l_out;
	}


	if((type != PS3_SCSI_CMD_TYPE_READ) && (type != PS3_SCSI_CMD_TYPE_WRITE)){
		goto l_out;
	}

	typeIndex = type - PS3_SCSI_CMD_TYPE_READ;
	current_ld_sd = &(data->vd_sd[typeIndex]);
	ps3_spin_lock_irqsave(&current_ld_sd->ps3_sequence_stream_lock, &flag);
	track_stream = &current_ld_sd->mru_bit_map;

	ps3_scsih_lba_parse(cmd->scmd->cmnd, &lba);
	ps3_scsih_len_parse(cmd->scmd->cmnd, &len);
	for (index = 0; index < PS3_IO_MAX_STREAMS_TRACKED; ++index) {
		stream_num = (*track_stream >>
			(index << BITS_PER_INDEX_STREAM_SHIFT)) &
			STREAM_MASK;
		current_sd = &current_ld_sd->stream_track[stream_num];
		value = current_sd->next_seq_lba - IO_STREAM_DETECT_RANGE;

		if ((((value >= 0) &&
			(lba >= current_sd->next_seq_lba - IO_STREAM_DETECT_RANGE) &&
			(lba <= (current_sd->next_seq_lba + IO_STREAM_DETECT_RANGE * 2))) ||
			((current_sd->next_seq_lba) &&
			(lba >= current_sd->next_seq_lba) &&
			(lba <= (current_sd->next_seq_lba + 32))))&&
			(type ==  current_sd->rw_type)){
			is_stream = PS3_TRUE;
			cmd->io_attr.seq_flag = SCSI_RW_SEQ_CMD;
			cmd->req_frame->frontendReq.reqHead.isStream1 = PS3_TRUE;
			current_sd->next_seq_lba =
			lba + len;

			shifted_values_mask =
				(1ULL <<  (index << BITS_PER_INDEX_STREAM_SHIFT)) - 1;
			shifted_values = ((*track_stream & shifted_values_mask)
				<< BITS_PER_INDEX_STREAM);
			index_value_mask =
				STREAM_MASK << (index << BITS_PER_INDEX_STREAM_SHIFT);
			unshifted_values =
				*track_stream & ~(shifted_values_mask |
				index_value_mask);
			*track_stream =
				unshifted_values | shifted_values | stream_num;
			goto end;
		}
	}
	stream_num = (*track_stream >> ((PS3_IO_MAX_STREAMS_TRACKED - 1) <<
		BITS_PER_INDEX_STREAM_SHIFT)) & STREAM_MASK;
	current_sd = &current_ld_sd->stream_track[stream_num];
	current_sd->rw_type = type;
	current_sd->next_seq_lba = lba + len;
	*track_stream = (((*track_stream & ZERO_LAST_STREAM) << 4) | stream_num);
	cmd->io_attr.seq_flag = SCSI_RW_RANDOM_CMD;

end:
	ps3_spin_unlock_irqrestore(&current_ld_sd->ps3_sequence_stream_lock, flag);
l_out:
	INJECT_START(PS3_ERR_IJ_FORCE_SEQ_CMD, cmd);
	INJECT_START(PS3_ERR_IJ_FORCE_STREAM_DETECT_TRUE, &is_stream);
	
	INJECT_START(PS3_ERR_IJ_FORCE_RAND_CMD, cmd);
	INJECT_START(PS3_ERR_IJ_FORCE_STREAM_DETECT_FALSE, &is_stream);
	return is_stream;
}
Bool ps3_raid_scsih_stream_is_direct(const struct ps3_cmd *cmd)
{
	struct ps3_scsi_priv_data *p_priv_data = scsi_device_private_data(cmd->scmd);
	Bool is_direct = PS3_TRUE;

	if (p_priv_data->dev_type == PS3_DEV_TYPE_VD) {
		if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
			if (cmd->io_attr.vd_entry->isNvme || !(cmd->io_attr.vd_entry->isSsd &&
				(ps3_atomic_read(&p_priv_data->rd_io_outstand) > PS3_VD_IO_16_OUTSTANDING) &&
				(cmd->io_attr.num_blocks > PS3_BLOCK_NUM_OF_32K))) {
				is_direct = PS3_FALSE;
			}
		}
	}

	return is_direct;
}
Bool ps3_hba_scsih_stream_is_direct(const struct ps3_cmd *cmd)
{
	Bool is_direct = PS3_TRUE;
	struct ps3_scsi_priv_data *p_priv_data = scsi_device_private_data(cmd->scmd);
	if (p_priv_data->dev_type == PS3_DEV_TYPE_VD && 
		cmd->io_attr.vd_entry->isNvme && 
		ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
		if (ps3_atomic_read(&p_priv_data->rd_io_outstand) > PS3_VD_IO_16_OUTSTANDING &&
				cmd->io_attr.num_blocks > PS3_BLOCK_NUM_OF_32K) {
			is_direct = PS3_FALSE;
		}
	}

	return is_direct;
}

static Bool ps3_scsih_sys_state_check(struct ps3_instance *instance,
	struct scsi_cmnd *s_cmd, S32 *scsi_result)
{
	Bool ret = PS3_TRUE;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;

	*scsi_result = PS3_SUCCESS;
	if (!instance->state_machine.is_load) {
		LOG_INFO_LIM("hno:%u instance state not is_load\n", PS3_HOST(instance));
		goto l_scsi_done;
	}

	PS3_IO_START_INC(instance, s_cmd);
	cur_state = atomic_read(&instance->state_machine.state);
	if (cur_state == PS3_INSTANCE_STATE_DEAD ||
		cur_state == PS3_INSTANCE_STATE_QUIT) {
		PS3_IO_BACK_ERR_INC(instance, s_cmd);
		LOG_INFO_LIM("hno:%u instance state is dead/quit\n", PS3_HOST(instance));
		goto l_scsi_done;
	}

	if (instance->recovery_context->host_reset_state != PS3_HOST_RESET_INIT) {
		*scsi_result = SCSI_MLQUEUE_HOST_BUSY;
		ret = PS3_FALSE;
		PS3_IO_BACK_ERR_INC(instance, s_cmd);
		PS3_DEV_BUSY_DEC(s_cmd);
		LOG_ERROR_LIM("hno:%u host_Reset_state:%d\n", PS3_HOST(instance),instance->recovery_context->host_reset_state);
		goto l_out;
	}

	if (cur_state != PS3_INSTANCE_STATE_OPERATIONAL &&
		cur_state != PS3_INSTANCE_STATE_PRE_OPERATIONAL &&
		cur_state != PS3_INSTANCE_STATE_SOFT_RECOVERY) {
		*scsi_result = SCSI_MLQUEUE_HOST_BUSY;
		ret = PS3_FALSE;
		PS3_IO_BACK_ERR_INC(instance, s_cmd);
		PS3_DEV_BUSY_DEC(s_cmd);
		LOG_INFO_LIM("hno:%u cannot request scsi cmd tag:%d due to %s\n",
			PS3_HOST(instance), SCMD_GET_REQUEST(s_cmd)->tag,
			namePS3InstanceState(cur_state));
		goto l_out;
	}

	goto l_out;

l_scsi_done:
	PS3_DEV_BUSY_DEC(s_cmd);
	s_cmd->result =  PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT);
	SCMD_IO_DONE(s_cmd);
	*scsi_result = PS3_SUCCESS;
	ret = PS3_FALSE;

l_out:
	return ret;
}

static inline Bool ps3_scsih_is_illegel_vd_io(const struct scsi_cmnd *s_cmd,
	struct ps3_instance *instance)
{
	return(!ps3_dev_id_valid_check(instance, s_cmd->device->channel,
		s_cmd->device->id, PS3_DISK_TYPE_VD) || s_cmd->device->lun);
}

static Bool ps3_scsih_device_attr_check(struct ps3_instance *instance,
	struct scsi_cmnd *s_cmd, S32 *scsi_result)
{
	Bool ret = PS3_TRUE;
	struct ps3_scsi_priv_data *data = NULL;
	static unsigned long j;
	*scsi_result = PS3_SUCCESS;
	if (unlikely(instance->task_manager_host_busy)) {
		PS3_IO_BACK_ERR_INC(instance, s_cmd);
		PS3_DEV_BUSY_DEC(s_cmd);
		*scsi_result = SCSI_MLQUEUE_DEVICE_BUSY;
		ret = PS3_FALSE;
		LOG_INFO_LIM("hno:%u task_manager_host_busy\n", PS3_HOST(instance));
		goto l_out;
	}

	data = (struct ps3_scsi_priv_data*)s_cmd->device->hostdata;
	if (unlikely(data == NULL)) {
		s_cmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT);
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u device_priv_data is null s_cmd result:%d\n",
			PS3_HOST(instance), s_cmd->result);
		goto l_scsi_done;
	}

	if (unlikely(data->task_manager_busy)) {
		PS3_IO_BACK_ERR_INC(instance, s_cmd);
		PS3_DEV_BUSY_DEC(s_cmd);
		*scsi_result = SCSI_MLQUEUE_DEVICE_BUSY;
		ret = PS3_FALSE;
		LOG_INFO_LIM("hno:%u task_manager_busy\n", PS3_HOST(instance));
		goto l_out;
	}

	if (data->dev_type == PS3_DEV_TYPE_VD) {
		if (unlikely(ps3_scsih_is_illegel_vd_io(s_cmd, instance))) {
			s_cmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_BAD_TARGET);
			LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u dev_type:%s id:%u "
				"max_vd_count:%u vd id NOK\n", PS3_HOST(instance),
				namePS3DevType((enum PS3DevType)data->dev_type),
				s_cmd->device->id, instance->ctrl_info.maxVdCount);
			goto l_scsi_done;
		}

		if (unlikely(ps3_vd_access_policy_check(instance, s_cmd->device->channel,
			s_cmd->device->id, s_cmd) != PS3_SUCCESS)) {
			goto l_scsi_done;
		}
	}

#if 0
	if ((s_cmd->cmnd[0] == SYNCHRONIZE_CACHE) && is_vd_io &&
		!instance->is_support_sync_cache) {
		s_cmd->result = DID_OK << PS3_SHIFT_WORD;
		LOG_ERROR("hno:%u SYNCHRONIZE_CACHE cmd, return\n",
			PS3_HOST(instance));
		goto l_out;
	}
#endif

	goto l_out;

l_scsi_done:
	PS3_IO_BACK_ERR_INC(instance, s_cmd);
	PS3_DEV_BUSY_DEC(s_cmd);
	*scsi_result = PS3_SUCCESS;
	ret = PS3_FALSE;
	SCMD_IO_DONE(s_cmd);
l_out:
	return ret;
}

static inline U8 ps3_scsih_dev_type_get(const struct scsi_cmnd *s_cmd)
{
	struct ps3_scsi_priv_data *data =
		(struct ps3_scsi_priv_data*)s_cmd->device->hostdata;
	return data->dev_type;
}

static inline Bool ps3_scsih_is_vd_scsi_rw_cmd(const struct scsi_cmnd *s_cmd)
{
	return (ps3_scsih_dev_type_get(s_cmd) == PS3_DEV_TYPE_VD) ?
		ps3_scsih_cdb_is_rw_cmd(s_cmd->cmnd) : PS3_DRV_FALSE;
}

static Bool ps3_scsih_qos_check(struct ps3_instance *instance,
	struct scsi_cmnd *s_cmd, S32 *scsi_result)
{
	Bool ret = PS3_DRV_TRUE;
	S32 vd_io_threshold = instance->cmd_attr.vd_io_threshold;
	S32 io_outstand = 0;
	S32 vd_io_outstand = 0;

	if (unlikely(!ps3_stat_outstand_switch_is_open(instance))) {
		goto l_out;
	}

	io_outstand = atomic_inc_return(&instance->cmd_statistics.io_outstanding);
	if (unlikely(io_outstand > instance->host->can_queue)) {
		PS3_IO_BACK_ERR_INC(instance, s_cmd);
		PS3_DEV_BUSY_DEC(s_cmd);
		*scsi_result = SCSI_MLQUEUE_HOST_BUSY;
		ret = PS3_FALSE;

		LOG_INFO_LIM("hno:%u ioc io exceed can_queue:%d io_outstanding:%d\n",
			PS3_HOST(instance), instance->host->can_queue, io_outstand);
		goto l_dec_io_outstand;
	}

	if (unlikely(vd_io_threshold && ps3_scsih_is_vd_scsi_rw_cmd(s_cmd))) {
		vd_io_outstand = atomic_inc_return(&instance->cmd_statistics.vd_io_outstanding);
		if (unlikely(vd_io_outstand > vd_io_threshold)) {
			PS3_IO_BACK_ERR_INC(instance, s_cmd);
			PS3_DEV_BUSY_DEC(s_cmd);
			*scsi_result = SCSI_MLQUEUE_DEVICE_BUSY;
			ret = PS3_FALSE;
			LOG_INFO_LIM("hno:%u vd io exceed vd_io_threshold:%d vd_outstanding:%d\n",
				PS3_HOST(instance), vd_io_threshold, vd_io_outstand);
			goto l_dec_vd_io_outstand;
		}
	}

	PS3_VD_OUTSTAND_INC(instance, s_cmd);

	goto l_out;

l_dec_vd_io_outstand:
	atomic_dec(&instance->cmd_statistics.vd_io_outstanding);
l_dec_io_outstand:
	atomic_dec(&instance->cmd_statistics.io_outstanding);
l_out:
	return ret;
}

static Bool ps3_scsih_que_cmd_check(struct ps3_instance *instance,
	struct scsi_cmnd *s_cmd, S32 *scsi_result)
{
	Bool ret = PS3_TRUE;

	s_cmd->result = DID_OK;
	ret = ps3_scsih_sys_state_check(instance, s_cmd, scsi_result);
	if (unlikely(!ret)) {
		goto l_out;
	}

	ret = ps3_scsih_device_attr_check(instance, s_cmd, scsi_result);
	if (unlikely(!ret)) {
		goto l_out;
	}

	ret = ps3_scsih_qos_check(instance, s_cmd, scsi_result);

l_out:
	return ret;
}

#else
Bool ps3_scsih_sys_state_check(struct ps3_instance *instance, S32 *host_status)
{
	Bool ret = PS3_DRV_TRUE;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;
	static unsigned long j;
	if (!instance->state_machine.is_load) {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u instance state not is_load\n", PS3_HOST(instance));
		*host_status = DID_ERROR;
		goto l_out;
	}

	cur_state = ps3_atomic_read(&instance->state_machine.state);
	if (cur_state == PS3_INSTANCE_STATE_DEAD ||
		cur_state == PS3_INSTANCE_STATE_QUIT) {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u instance state is dead/quit\n", PS3_HOST(instance));
		*host_status = DID_BAD_TARGET;
		goto l_out;
	}

	if (cur_state != PS3_INSTANCE_STATE_OPERATIONAL &&
		cur_state != PS3_INSTANCE_STATE_PRE_OPERATIONAL &&
		cur_state != PS3_INSTANCE_STATE_SOFT_RECOVERY) {
		ret = PS3_DRV_FALSE;
		*host_status = DID_BUS_BUSY;
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u cannot request scsi cmd due to %s\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state));
		goto l_out;
	}

l_out:
	return ret;
}

Bool ps3_scsih_stream_is_detect(struct ps3_cmd *cmd)
{
	(void)cmd;
	return PS3_FALSE;
}
#endif

#if 0
static Bool ps3_synchronize_cache_filter(struct scsi_cmnd *s_cmd)
{
	(void)s_cmd;
	return PS3_DRV_FALSE;
}
#endif

#ifndef _WINDOWS
S32 ps3_scsih_queue_command(struct Scsi_Host *s_host,
	struct scsi_cmnd *s_cmd)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	struct ps3_instance *instance = NULL;
	struct ps3_scsi_priv_data *data = (struct ps3_scsi_priv_data *)s_cmd->device->hostdata;
	static unsigned long j;
	instance = (struct ps3_instance *)s_host->hostdata;
	if (unlikely(instance == NULL)) {
		s_cmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT);
		PS3_BUG_ON(instance == NULL);
		goto l_scsi_done;
	}

	LOG_DEBUG("hno:%u tag:%d op:0x%x cmd_len:%d chl:%u "
		"id:%u timeout:%u\n", s_host->host_no, SCMD_GET_REQUEST(s_cmd)->tag,
		s_cmd->cmnd[0], s_cmd->cmd_len, s_cmd->device->channel,
		s_cmd->device->id, SCMD_GET_REQUEST(s_cmd)->timeout);

	PS3_DEV_BUSY_INC(s_cmd);
	if (unlikely(!ps3_scsih_que_cmd_check(instance, s_cmd, &ret))) {
		goto l_out;
	}
	if (unlikely(s_cmd->cmd_len > PS3_FRAME_CDB_BUFLEN)) {
		LOG_ERROR("hno:%u cmd len %u check NOK\n", PS3_HOST(instance), s_cmd->cmd_len);
		PS3_BUG();
		s_cmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_ERROR);
		goto l_scsi_done;
	}
	ps3_scsi_cmd_deliver_get(instance);
	cmd = ps3_scsi_cmd_alloc(instance, SCMD_GET_REQUEST(s_cmd)->tag);
	if (unlikely(cmd == NULL)) {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u tag:%d ps3_cmd_get NOK ret:%d\n",
			PS3_HOST(instance), SCMD_GET_REQUEST(s_cmd)->tag, ret);
		PS3_IO_BACK_ERR_INC(instance, s_cmd);
		PS3_IO_OUTSTAND_DEC(instance, s_cmd);
		PS3_VD_OUTSTAND_DEC(instance, s_cmd);
		PS3_DEV_BUSY_DEC(s_cmd);
		ret = SCSI_MLQUEUE_HOST_BUSY;
		ps3_scsi_cmd_deliver_put(instance);
		goto l_scsi_done;
	}

	if (cmd->scmd != NULL) {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u CFID:%u cmd is exist\n",
			PS3_HOST(instance), SCMD_GET_REQUEST(s_cmd)->tag);
		PS3_BUG();
		s_cmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT);
		goto l_cmd_stat_dec;
	}

	cmd->scmd = s_cmd;

	ret = instance->ioc_adpter->io_cmd_build(cmd);
	if (unlikely(ret != PS3_SUCCESS)) {
		switch (ret) {
			case -PS3_IO_CONFLICT:
				ps3_errcode_to_scsi_status(instance, s_cmd, SCSI_STATUS_BUSY, NULL, 0, cmd);
				ret = SCSI_MLQUEUE_DEVICE_BUSY;
				goto l_cmd_release;
				break;
			case -PS3_IO_REQUEUE:
				s_cmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_REQUEUE);
				break;
			case -PS3_IO_CONFLICT_IN_Q:
				LOG_DEBUG("tid:0x%llx hno:%u tag:%d conflict in queue, "
				"timeout:%u alloc_time:%lu cur_time:%lu\n",
					cmd->trace_id, PS3_HOST(instance),
					SCMD_GET_REQUEST(s_cmd)->tag,
					SCMD_GET_REQUEST(s_cmd)->timeout,
					s_cmd->jiffies_at_alloc, jiffies);
				ret = PS3_SUCCESS;
				cmd->flighting = PS3_FALSE;
				wmb();
				ps3_scsi_cmd_deliver_put(instance);
				goto l_out;
				break;
			case -PS3_IN_QOS_Q:
				ret = PS3_SUCCESS;
				cmd->flighting = PS3_FALSE;
				wmb();
				ps3_scsi_cmd_deliver_put(instance);
				goto l_out;
			case -PS3_RETRY:
				ret = SCSI_MLQUEUE_DEVICE_BUSY;
				goto l_cmd_release;
			case -PS3_RECOVERED:
			case SCSI_MLQUEUE_HOST_BUSY:
				ret = SCSI_MLQUEUE_HOST_BUSY;
				goto l_cmd_release;
			default:
				s_cmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT);
				break;
		}
		LOG_DEBUG("tid:0x%llx hno:%u tag:%d cmd build err ret:%d\n",
			cmd->trace_id, PS3_HOST(instance), SCMD_GET_REQUEST(s_cmd)->tag, ret);
		ret = PS3_SUCCESS;
		goto l_cmd_release;
	}

	ret = ps3_scsi_cmd_send(instance, cmd);
	if (unlikely(ret != PS3_SUCCESS)) {
		ps3_qos_cmd_update(instance, cmd);
		if (ret == -PS3_RECOVERED) {
			ret = SCSI_MLQUEUE_HOST_BUSY;
		} else if (ret == -PS3_RETRY) {
			ret = SCSI_MLQUEUE_DEVICE_BUSY;
		} else {
			s_cmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT);
			ret = PS3_SUCCESS;
		}
		LOG_ERROR_LIM("tid:0x%llx hno:%u tag:%d cmd send NOK ret:%d\n",
			cmd->trace_id, PS3_HOST(instance), SCMD_GET_REQUEST(s_cmd)->tag, ret);
			PS3_DEV_IO_START_ERR_INC(instance, cmd);
		goto l_cmd_release;
	}

	ps3_scsi_cmd_deliver_put(instance);
	goto l_out;

l_cmd_release:
	ps3_scsi_dma_unmap(cmd);
	if (cmd->is_got_r1x == 1) {
		data = (struct ps3_scsi_priv_data *)s_cmd->device->hostdata;
		ps3_r1x_write_unlock(&data->lock_mgr, cmd);
	}

	ps3_scsi_cmd_free(cmd);
l_cmd_stat_dec:
	PS3_IO_BACK_ERR_INC(instance, s_cmd);
	PS3_IO_OUTSTAND_DEC(instance, s_cmd);
	PS3_VD_OUTSTAND_DEC(instance, s_cmd);
	PS3_DEV_BUSY_DEC(s_cmd);
	ps3_scsi_cmd_deliver_put(instance);
l_scsi_done:
	if (ret != SCSI_MLQUEUE_HOST_BUSY && ret != SCSI_MLQUEUE_DEVICE_BUSY) {
		SCMD_IO_DONE(s_cmd);
	}
l_out:
	return ret;
}

#endif
static inline Bool ps3_is_need_build_hw_req_frame(const struct ps3_cmd *cmd)
{
	return ps3_scsih_is_use_hard_cmd(cmd) == PS3_CMDWORD_FORMAT_HARDWARE;
}

Bool ps3_is_r1x_write_cmd(const struct ps3_cmd *cmd)
{
	Bool ret = PS3_FALSE;

	switch(cmd->io_attr.vd_entry->raidLevel) {
	case RAID1:
	case RAID10:
	case RAID1E:
		if (cmd->io_attr.rw_flag == PS3_SCSI_CMD_TYPE_WRITE) {
			ret = PS3_TRUE;
		}
		break;
	default:
		ret = PS3_FALSE;
		break;
	}

	return ret;
}

static Bool  direct_cmd_is_raidlevel_support(struct ps3_cmd *cmd)
{
	Bool ret = PS3_DRV_TRUE;

	switch(cmd->io_attr.vd_entry->raidLevel) {
	case RAID0:
	case RAID00:
		break;
	case RAID1:
	case RAID10:
	case RAID1E:
		if (cmd->instance->cmd_context.max_r1x_cmd_count > 0) {
			if (cmd->io_attr.rw_flag == PS3_SCSI_CMD_TYPE_READ ||
				cmd->io_attr.rw_flag == PS3_SCSI_CMD_TYPE_WRITE) {
				ret = PS3_TRUE;
			} else {
				ret = PS3_FALSE;
			}
		} else if (cmd->io_attr.rw_flag != PS3_SCSI_CMD_TYPE_READ) {
			ret = PS3_FALSE;
		}
		break;
	case RAID5:
	case RAID6:
	case RAID50:
	case RAID60:
		if (cmd->io_attr.rw_flag != PS3_SCSI_CMD_TYPE_READ) {
			ret = PS3_FALSE;
		}
		break;
	default:
		ret = PS3_FALSE;
		break;
	}

	return ret;
}

static Bool direct_cmd_is_plba_sector_aligned(struct ps3_cmd *cmd,
	const struct ps3_pd_entry *pd)
{
	const struct PS3VDEntry *vd = cmd->io_attr.vd_entry;
	Bool ret = PS3_DRV_FALSE;

	if (unlikely(pd->sector_size == 0)) {
		LOG_FILE_ERROR("pd chl:%u id:%u sector_size is 0\n",
			PS3_CHANNEL(&pd->disk_pos),
			PS3_TARGET(&pd->disk_pos));
		goto l_out;
	}
#ifndef _WINDOWS
	if ((cmd->io_attr.plba << ilog2(vd->sectorSize)) % pd->sector_size) {
#else
	if ((cmd->io_attr.plba * vd->sectorSize) % pd->sector_size) {
#endif
		LOG_DEBUG("pd chl:%u id:%u plba:0x%llx pd sector_size:%u\n",
			PS3_CHANNEL(&pd->disk_pos), PS3_TARGET(&pd->disk_pos),
			cmd->io_attr.plba, pd->sector_size);
		goto l_out;
	}
#ifndef _WINDOWS
	if ((cmd->io_attr.num_blocks << ilog2(vd->sectorSize)) % pd->sector_size) {
#else
	if ((cmd->io_attr.num_blocks * vd->sectorSize) % pd->sector_size) {
#endif
		LOG_DEBUG("num_blocks:%u pd sector_size:%u vd sector_size:%u\n",
			cmd->io_attr.num_blocks, pd->sector_size, vd->sectorSize);
		goto l_out;
	}
	ret = PS3_DRV_TRUE;
l_out:
	return ret;
}
Bool ps3_scsih_sata_direct_is_support(struct ps3_cmd *cmd,
	const struct ps3_pd_entry *pd_entry)
{
	Bool ret = PS3_FALSE;

	if (cmd == NULL || cmd->instance == NULL ||
		pd_entry == NULL) {
		LOG_ERROR_LIM("sata direct support judge parameter NOK\n");
		goto l_out;
	}

	if (cmd->instance->ctrl_info.capabilities.supportSataNcq == 0) {
		LOG_DEBUG("IOC SATA NCQ not support\n");
		goto l_out;
	}

	if (pd_entry->support_ncq == 0) {
		LOG_DEBUG("SATA ncq not support\n");
		goto l_out;
	}
	ret = PS3_TRUE;

l_out:
	return ret;
}

static inline Bool ps3_scsih_datelen_buflen_is_match(const struct ps3_cmd *cmd)
{
	Bool ret = PS3_FALSE;
	U32 data_len = 0;

	data_len = ps3_scsih_datelen_calc(cmd);

	if(data_len != scsi_bufflen(cmd->scmd)) {
		LOG_INFO("data_len:%u not equal to bufflen:%u\n",
			data_len, scsi_bufflen(cmd->scmd));
		goto l_out;
	}
	ret = PS3_TRUE;

l_out:
	return ret;
}

static U32 ps3_scsih_datelen_calc(const struct ps3_cmd *cmd)
{
	U16 sector_size = 0;
	U32 num_blocks = 0;
	U32 date_len = 0;
	static unsigned long j;
	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		if (cmd->io_attr.vd_entry == NULL) {
			LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "host parameter NOK\n");
			goto l_out;
		}
		sector_size = cmd->io_attr.vd_entry->sectorSize;

	} else {
		sector_size = cmd->io_attr.pd_entry->sector_size;
	}
	if (sector_size == 0) {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "host parameter NOK\n");
		goto l_out;
	}
	num_blocks = cmd->io_attr.num_blocks;
	date_len = num_blocks << ilog2(sector_size);

l_out:
	return date_len;
}

Bool ps3_scsih_sata_direct_is_need(struct ps3_cmd *cmd)
{
	Bool ret = PS3_FALSE;

	if (cmd->scmd->cmd_len == 32) {
		LOG_DEBUG("read32 or write32 no need direct\n");
		goto l_out;
	}

	if(ps3_scsih_datelen_buflen_is_match(cmd) != PS3_TRUE) {
		goto l_out;
	}

	if (ps3_scsih_cdb_opts_parse(cmd) == PS3_SUCCESS) {
		if (likely(cmd->io_attr.cdb_opts.protect == 0 &&
			cmd->io_attr.cdb_opts.fua == 0)) {
			ret = PS3_TRUE;
		} else {
			LOG_DEBUG("SATA cdb_opts is 0x%x\n", cmd->io_attr.cdb_opts.option);
		}
	} else {
		LOG_DEBUG("SATA options parse failure\n");
	}
l_out:
	return ret;
}

static inline Bool ps3_scsih_sas_direct_is_need(const struct ps3_cmd *cmd)
{
	(void)cmd;
	return PS3_TRUE;
}

static inline Bool ps3_scsih_nvme_direct_is_need(struct ps3_cmd *cmd)
{
	if (ps3_scsih_cdb_opts_parse(cmd) == PS3_SUCCESS) {
		if (likely(cmd->io_attr.cdb_opts.protect != 0)) {
			LOG_DEBUG("NVMe protect is 0x%x\n", cmd->io_attr.cdb_opts.protect);
			return PS3_FALSE;
		}
	} else {
		LOG_DEBUG("NVMe options parse failure\n");
		return PS3_FALSE;
	}

	return (ps3_scsih_datelen_buflen_is_match(cmd) == PS3_TRUE);
}

static Bool ps3_scsih_direct_cmd_is_supported_logic(struct ps3_cmd *cmd,
	const struct ps3_pd_entry *pd_entry)
{
	Bool ret = PS3_FALSE;
	if (pd_entry == NULL) {
		PS3_BUG();
		goto l_out;
	}

	switch (pd_entry->dev_type)
	{
		case PS3_DEV_TYPE_SATA_SSD:
		case PS3_DEV_TYPE_SATA_HDD:
			ret = ps3_scsih_sata_direct_is_need(cmd);
			break;
		case PS3_DEV_TYPE_SAS_SSD:
		case PS3_DEV_TYPE_SAS_HDD:
			ret = ps3_scsih_sas_direct_is_need(cmd);
			break;
		case PS3_DEV_TYPE_NVME_SSD:
			ret = ps3_scsih_nvme_direct_is_need(cmd);
			break;
		default:
			ret = PS3_FALSE;
			break;
	}
l_out:
	return ret;
}

static Bool direct_cmd_is_supported_device(struct ps3_cmd *cmd,
	const struct ps3_pd_entry *pd_entry)
{
	Bool ret = PS3_FALSE;
	if (pd_entry == NULL) {
		LOG_ERROR("tid:0x%llx hno:%u CFID:%d pd entry is NULL\n",
			cmd->trace_id, PS3_HOST(cmd->instance), cmd->index);
		PS3_BUG();
		return ret;
	}
	switch (pd_entry->dev_type)
	{
		case PS3_DEV_TYPE_SATA_SSD:
		case PS3_DEV_TYPE_SATA_HDD:
			ret = ps3_scsih_sata_direct_is_support(cmd, pd_entry);
			break;
		case PS3_DEV_TYPE_SAS_SSD:
		case PS3_DEV_TYPE_SAS_HDD:
			ret = PS3_TRUE;
			break;
		case PS3_DEV_TYPE_NVME_SSD:
			ret = ps3_scsih_is_protocal_rw(cmd->scmd->cmnd);
			break;
		default:
			ret = PS3_FALSE;
			break;
	}

	return ret;
}

static inline S32 ps3_scsih_is_valid_vlba(struct ps3_cmd *cmd)
{
	return ((U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo) +
		cmd->io_attr.num_blocks) > cmd->io_attr.vd_entry->capacity) ?
		-PS3_FAILED : PS3_SUCCESS;
}

static inline Bool ps3_scsih_is_direct_to_normal(const struct ps3_instance *instance,
	const struct ps3_cmd *cmd)
{
	return (instance->ioc_adpter->is_need_direct_to_normal != NULL &&
		instance->ioc_adpter->is_need_direct_to_normal(cmd));
}

static inline Bool ps3_scsih_is_sas_jbod_cmd(const struct ps3_cmd *cmd)
{
	return cmd->io_attr.dev_type == PS3_DEV_TYPE_SAS_HDD ||
		cmd->io_attr.dev_type == PS3_DEV_TYPE_SAS_SSD;
}

static inline Bool ps3_scsih_is_sata_jbod_cmd(const struct ps3_cmd *cmd)
{
	return cmd->io_attr.dev_type == PS3_DEV_TYPE_SATA_HDD ||
		cmd->io_attr.dev_type == PS3_DEV_TYPE_SATA_SSD;
}

Bool ps3_scsih_is_sata_jbod_mgr_cmd(const struct ps3_cmd *cmd)
{
	return (ps3_scsih_is_sata_jbod_cmd(cmd))
		&& (!ps3_scsih_cdb_is_rw_cmd(cmd->scmd->cmnd));
}

U32 ps3_scsih_xfer_cnt_get(const struct ps3_cmd *cmd)
{
	U32 xfer_cnt = 0;

	PS3RespFrame_u *resp_frame = cmd->resp_frame;
	U16 mode = cmd->reply_word.mode;

	if (ps3_err_is_resp_from_direct_cmd(mode)
		&& (cmd->io_attr.dev_type != PS3_DEV_TYPE_VD)) {
		xfer_cnt = resp_frame->sasRespFrame.xfer_cnt;
	} else {
		xfer_cnt = resp_frame->normalRespFrame.respDetail.xfer_cnt;
	}
	return xfer_cnt;
}
static inline Bool ps3_scsih_hdd_vd_direct_check(const struct ps3_cmd *cmd)
{
	Bool is_direct = PS3_TRUE;
	struct ps3_scsi_priv_data *p_priv_data = scsi_device_private_data(cmd->scmd);

	if (p_priv_data->dev_type == PS3_DEV_TYPE_VD) {
		if (!cmd->io_attr.vd_entry->isNvme && !cmd->io_attr.vd_entry->isSsd) {
			if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag) &&
				(ps3_atomic_read(&p_priv_data->wr_io_outstand) != PS3_VD_IO_1_OUTSTANDING)) {
					is_direct = PS3_FALSE;
			} else if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
				is_direct = PS3_FALSE;
			}
		}
	}
	return is_direct;
}

static void ps3_scsih_build_direct_cmd(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	U64 lba = 0;

	struct ps3_cmd_attr_context *cmd_attr = &cmd->instance->cmd_attr;

	if (!cmd_attr->is_support_direct_cmd) {
		goto l_out;
	}

	if (cmd->io_attr.is_force_normal) {
		goto l_out;
	}

	if (!ps3_scsih_is_rw_type(cmd->io_attr.rw_flag)) {
		goto l_out;
	}

	if ((!ps3_scsih_is_protocal_rw(cmd->scmd->cmnd)) &&
		ps3_scsih_is_sas_jbod_cmd(cmd)) {
		goto l_out;
	}

	if (unlikely(cmd->io_attr.num_blocks == 0)) {
		LOG_DEBUG("tid:0x%llx hno:%u num blocks is zero\n",
			cmd->trace_id, PS3_HOST(cmd->instance));
		goto l_out;
	}

	if (unlikely(cmd->io_attr.is_retry_cmd &&
		ps3_scsih_is_direct_to_normal(cmd->instance, cmd))) {
			LOG_FILE_WARN("tid:0x%llx hno:%u is_retry_cmd \n",
				cmd->trace_id, PS3_HOST(cmd->instance));
			goto l_out;
	}

	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		if (cmd->instance->ioc_adpter->scsih_stream_is_detect != NULL &&
		 		cmd->instance->ioc_adpter->scsih_stream_is_detect(cmd)) {

			LOG_DEBUG("hno[%u], cmd index[%u], tid[0x%llx], detected it is sequential io\n",
				PS3_HOST(cmd->instance), cmd->index, cmd->trace_id);
			if (cmd->instance->ioc_adpter->scsih_stream_is_direct != NULL && \
		 		!cmd->instance->ioc_adpter->scsih_stream_is_direct(cmd)) {
					goto l_out;
			}
		}
		if (unlikely(!cmd->io_attr.vd_entry->isDirectEnable)) {
			LOG_DEBUG("tid:0x%llx hno:%u direct disable\n",
				cmd->trace_id, PS3_HOST(cmd->instance));
			goto l_out;
		}

		if (cmd->instance->ioc_adpter->write_direct_enable != NULL &&
				!cmd->instance->ioc_adpter->write_direct_enable(cmd)) {
			LOG_DEBUG("tid:0x%llx hno:%u write direct disable\n",
				cmd->trace_id, PS3_HOST(cmd->instance));
			goto l_out;
		}

		if (unlikely(ps3_scsih_is_valid_vlba(cmd) != PS3_SUCCESS)) {
			LOG_ERROR_LIM("hno:%u tid:%#llx lba:0x%llx "
				"num_blks:0x%x vd capacity:%llu cmd out of range\n",
				PS3_HOST(cmd->instance), cmd->trace_id,
				U32_HIGH_LOW_TO_U64(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo),
				cmd->io_attr.num_blocks, cmd->io_attr.vd_entry->capacity);
			goto l_out;
		}

		if (!direct_cmd_is_raidlevel_support(cmd)) {
			goto l_out;
		}

		if (!ps3_scsih_is_protocal_rw(cmd->scmd->cmnd)) {
			goto l_out;
		}

		if ((cmd->io_attr.vd_entry->raidLevel == RAID1)) {
			if (cmd->io_attr.vd_entry->mapBlock == 0) {
				PS3_BUG();
				goto l_out;
			}
			if (cmd->instance->is_raid1_direct_skip_mapblock_check) {
				LOG_DEBUG("hba raid1, skip MapBlock check\n");
			} else {
				lba = PS3_LBA(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
				if (PS3_SCSIH_NOT_SAME_MAPBLOCK(lba, cmd->io_attr.vd_entry->mapBlock,
					cmd->io_attr.num_blocks)) {
					LOG_DEBUG("hno:%u map_block:%llu lba:0x%llx num_blks:0x%x not same map block\n",
						PS3_HOST(cmd->instance), cmd->io_attr.vd_entry->mapBlock,
						lba, cmd->io_attr.num_blocks);
					goto l_out;
				}
			}

		} else {
			if ((cmd->instance->is_single_disk_raid0_direct_skip_strip_check) &&
					(cmd->io_attr.vd_entry->raidLevel == RAID0) &&
					(cmd->io_attr.vd_entry->physDrvCnt == 1)) {
				LOG_DEBUG("hba raid0 with single disk, skip strip check\n");
			} else {
				if ( !ps3_scsih_is_same_strip(cmd->io_attr.vd_entry,
					cmd->io_attr.lba_lo, cmd->io_attr.num_blocks) ) {
					LOG_DEBUG("not same strip lba:0x%x num_blks:0x%x\n",
						cmd->io_attr.lba_lo, cmd->io_attr.num_blocks);
					goto l_out;
				}
			}
		}

		ret = ps3_scsih_vd_rw_io_to_pd_calc(cmd);
		if (ret != PS3_SUCCESS) {
			LOG_DEBUG("cmd index:%u vlba to plba fail\n", cmd->index);
			goto l_out;
		}

		if (!direct_cmd_is_supported_device(cmd, cmd->io_attr.pd_entry)) {
			LOG_DEBUG("tid:0x%llx hno:%u not support to direct for this cmd\n",
				cmd->trace_id, PS3_HOST(cmd->instance));
			goto l_out;
		}

		if (ps3_scsih_direct_cmd_is_supported_logic(cmd, cmd->io_attr.pd_entry) != PS3_TRUE) {
			LOG_DEBUG("tid:0x%llx hno:%u not need to direct for this cmd\n",
				cmd->trace_id, PS3_HOST(cmd->instance));
			goto l_out;
		}

		if (!direct_cmd_is_plba_sector_aligned(cmd,
				cmd->io_attr.pd_entry)) {
			goto l_out;
		}

		if (ps3_is_r1x_write_cmd(cmd)) {
			if (!direct_cmd_is_supported_device(cmd,
					cmd->io_attr.peer_pd_entry)) {
				LOG_DEBUG("tid:0x%llx hno:%u peer_pd[%u:%u:%u] not "
					"support to direct for this cmd\n",
					cmd->trace_id,
					PS3_HOST(cmd->instance),
					PS3_CHANNEL(&cmd->io_attr.peer_pd_entry->disk_pos),
					PS3_TARGET(&cmd->io_attr.peer_pd_entry->disk_pos),
					PS3_PDID(&cmd->io_attr.peer_pd_entry->disk_pos));
				goto l_out;
			}

			if (ps3_scsih_direct_cmd_is_supported_logic(cmd,
					cmd->io_attr.peer_pd_entry) != PS3_TRUE) {
				goto l_out;
			}

			if (!direct_cmd_is_plba_sector_aligned(cmd,
					cmd->io_attr.peer_pd_entry)) {
				goto l_out;
			}
		}
		if (!ps3_scsih_hdd_vd_direct_check(cmd)) {
			goto l_out;
		}
		cmd->io_attr.direct_flag = (U8)PS3_CMDWORD_DIRECT_ADVICE;
	} else {
		if (!direct_cmd_is_supported_device(cmd, cmd->io_attr.pd_entry)) {
			goto l_out;
		}

		if (ps3_scsih_direct_cmd_is_supported_logic(cmd, cmd->io_attr.pd_entry) != PS3_TRUE) {
			LOG_DEBUG("tid:0x%llx hno:%u not need to direct for this cmd\n",
				cmd->trace_id, PS3_HOST(cmd->instance));
			goto l_out;
		}

		cmd->io_attr.direct_flag = (U8)PS3_CMDWORD_DIRECT_OK;
		cmd->io_attr.plba = PS3_LBA(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
	}

l_out:
	return ;
}

static inline Bool ps3_scsih_is_invalid_dev(U8 type)
{
	return (type <= PS3_DEV_TYPE_UNKNOWN || type >= PS3_DEV_TYPE_COUNT);
}

static inline void ps3_scsih_use_frontend_prp_check(struct ps3_cmd *cmd)
{
	if (!cmd->instance->is_use_frontend_prp) {
		goto l_out;
	}

	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		if (!cmd->io_attr.vd_entry->isNvme) {
			goto l_out;
		}
	} else {
		if (cmd->io_attr.pd_entry->dev_type != PS3_DEV_TYPE_NVME_SSD) {
			goto l_out;
		}
	}

	cmd->io_attr.is_use_frontend_prp = ps3_scsih_is_protocal_rw(cmd->scmd->cmnd);

l_out:
	return ;
}

static Bool ps3_unmap_check_block_valid(struct ps3_cmd *cmd)
{
	Bool ret = PS3_FALSE;
	U8 umap_xbuf[32] = {0};
	U32 *p_umap_unmblk = &cmd->io_attr.num_blocks;
	U16 payload_len = 0;
	U16 desc_len = 0;

	if (cmd->scmd->cmnd[0] != UNMAP) {
		ret = PS3_TRUE;
		goto l_out;
	}

	payload_len = ps3_get_unaligned_be16(&cmd->scmd->cmnd[7]);
	if (unlikely(payload_len != scsi_bufflen(cmd->scmd))) {
		LOG_DEBUG("hno:%u. unmap: buf len:%u != cdb:%u\n",
			PS3_HOST(cmd->instance), scsi_bufflen(cmd->scmd),
			payload_len);
		goto l_out;
	}

	if (unlikely(scsi_bufflen(cmd->scmd) > (UNMAP_PARAM_LEN + UNMAP_DESCRIPTOR_LEN))) {
		LOG_DEBUG("hno:%u. unmap: buf len:%u > %u\n",
			PS3_HOST(cmd->instance), scsi_bufflen(cmd->scmd),
			(UNMAP_PARAM_LEN + UNMAP_DESCRIPTOR_LEN));
		goto l_out;
	}

	scsi_sg_copy_to_buffer(cmd->scmd, umap_xbuf, scsi_bufflen(cmd->scmd));

	if (ps3_get_unaligned_be16(&umap_xbuf[0]) != (payload_len - 2)) {
		LOG_DEBUG("hno:%u. unmap data len: :%u > payload:%u\n",
			PS3_HOST(cmd->instance), ps3_get_unaligned_be16(&umap_xbuf[0]),
			payload_len - 2);
		goto l_out;
	}

	desc_len = ps3_get_unaligned_be16(&umap_xbuf[2]);
	if ((desc_len >> 4) > cmd->io_attr.vd_entry->umapBlkDescCnt) {
		LOG_DEBUG("hno:%u. unmap: desc len:%u > desc cnt:%u*16\n",
			PS3_HOST(cmd->instance), desc_len,
			 cmd->io_attr.vd_entry->umapBlkDescCnt);
		goto l_out;
	}

	ps3_scsih_unmap_desc_parse(&umap_xbuf[UNMAP_PARAM_LEN], p_umap_unmblk,
		&cmd->io_attr.lba_lo, &cmd->io_attr.lba_hi);
	if (*p_umap_unmblk > cmd->io_attr.vd_entry->umapNumblk) {
		LOG_DEBUG("hno:%u. unmap: numblk:%u > limit %u\n",
			PS3_HOST(cmd->instance), *p_umap_unmblk,
			cmd->io_attr.vd_entry->umapNumblk);
		goto l_out;
	}

	ret = PS3_TRUE;
l_out:
	return ret;
}

static inline void ps3_req_frame_head_init(PS3ReqFrameHead_s *req_head)
{
	req_head->cmdType = 0;
	req_head->cmdSubType = 0;
	req_head->cmdFrameID = 0;
	req_head->control= 0;
	req_head->devID.diskID = 0;
	req_head->timeout = 0;
	req_head->virtDiskSeq = 0;
	req_head->reserved1[0] = 0;
	req_head->reserved1[1] = 0;
	req_head->reserved1[2] = 0;
	req_head->reserved1[3] = 0;
	req_head->traceID = 0;
}

static S32 ps3_scsih_cmd_build_prepare(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	S32 dma_map_ret = 0;
	Bool is_need_split = PS3_FALSE;
	struct ps3_scsi_priv_data *data = scsi_device_private_data(cmd->scmd);

	cmd->io_attr.dev_type = data->dev_type;
	if (cmd->is_inserted_c_q == 0) {
		dma_map_ret = ps3_scsi_dma_map(cmd);
		if (unlikely(dma_map_ret < 0)) {
			LOG_WARN_LIM("hno:%u. cfid[%u] dma_map NOK:%d\n",
				PS3_HOST(cmd->instance), cmd->index, dma_map_ret);
			ret = -PS3_ENOMEM;
			goto l_out;
		}
	}

	if (unlikely(ps3_scsih_is_invalid_dev(cmd->io_attr.dev_type))) {
		LOG_ERROR_LIM("hno:%u get dev type NOK :%d dev_type:%s\n",
			PS3_HOST(cmd->instance), cmd->index,
			namePS3DevType((enum PS3DevType)cmd->io_attr.dev_type));
		ret = -PS3_DEV_TYPE_UNKOWN;
		goto l_out;
	}

	ps3_req_frame_head_init(&cmd->req_frame->hwReq.reqHead);

	cmd->io_attr.disk_id = data->disk_pos.diskDev.ps3Dev.phyDiskID;
#ifndef _WINDOWS
	cmd->io_attr.is_retry_cmd = (cmd->scmd->retries != 0);
#endif
	cmd->io_attr.direct_flag = (U8)PS3_CMDWORD_DIRECT_NORMAL;
	cmd->io_attr.rw_type = ps3_scsih_cdb_rw_type_get(scsi_cmnd_cdb(cmd->scmd));

	if (ps3_scsih_is_rw_type(cmd->io_attr.rw_flag)) {
		ps3_scsih_cdb_parse(scsi_cmnd_cdb(cmd->scmd), &cmd->io_attr.num_blocks,
			&cmd->io_attr.lba_lo, &cmd->io_attr.lba_hi, &is_need_split);
	}

	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		cmd->io_attr.vd_entry = ps3_dev_mgr_lookup_vd_info_by_id(cmd->instance,
			cmd->io_attr.disk_id);
		if (unlikely(cmd->io_attr.vd_entry == NULL)) {
			ret = -PS3_ENODEV;
			goto l_out;
		}

		if (unlikely(cmd->io_attr.num_blocks > cmd->io_attr.vd_entry->maxIOSize)) {
			LOG_DEBUG("hno:%u. numblk:%u > maxio:%u\n",
				PS3_HOST(cmd->instance), cmd->io_attr.num_blocks,
				cmd->io_attr.vd_entry->maxIOSize);
			cmd->io_attr.is_force_normal = PS3_TRUE;
			goto l_skip_r1x_write_lock;
		}

		if (!ps3_unmap_check_block_valid(cmd)) {
			cmd->io_attr.is_force_normal = PS3_TRUE;
			goto l_skip_r1x_write_lock;
		}

		ret = ps3_r1x_write_lock(&data->lock_mgr, cmd);
		if(unlikely(ret != PS3_SUCCESS)){
			goto l_out;
		}
	} else {
		cmd->io_attr.pd_entry = ps3_dev_mgr_lookup_pd_info_by_id(cmd->instance,
			cmd->io_attr.disk_id);
		if (unlikely(cmd->io_attr.pd_entry == NULL)) {
			ret = -PS3_ENODEV;
			goto l_out;
		}
	}

l_skip_r1x_write_lock:

	ps3_scsih_use_frontend_prp_check(cmd);

	ps3_scsih_build_direct_cmd(cmd);

	cmd->cmd_receive_cb = ps3_scsih_io_done;

l_out:
	return ret;
}

static S32 ps3_scsih_req_frame_build(struct ps3_cmd *cmd)
{
	S32 ret = -PS3_FAILED;
	U32 table_size = ARRAY_SIZE(g_req_frame_func_table);
	U32 i = 0;
#ifdef _WINDOWS
	ps3_scsi_need_remap_check(cmd);
	if (cmd->scmd->is_remap_databuff) {
		if (ps3_scsi_remap_sgl(cmd) != PS3_SUCCESS) {
			LOG_ERROR("remap sgl error\n");
			return ret;
		}
	}
#endif

	for (; i < table_size; ++i) {
		if (cmd->io_attr.dev_type == g_req_frame_func_table[i].type) {
			ret = g_req_frame_func_table[i].func(cmd);
			break;
		}
	}

	return ret;
}

static void ps3_scsih_vd_cmd_word_devid_build(struct ps3_cmd *cmd,
	const struct PS3VDEntry *vd_info)
{
	cmd->cmd_word.virtDiskID = (U8)PS3_VDID(&vd_info->diskPos);

	switch (cmd->io_attr.direct_flag) {
	case PS3_CMDWORD_DIRECT_OK:
	case PS3_CMDWORD_DIRECT_ADVICE:
		cmd->cmd_word.phyDiskID =
			PS3_PDID(&cmd->io_attr.pd_entry->disk_pos);
		break;
	case PS3_CMDWORD_DIRECT_RESERVE:
	case PS3_CMDWORD_DIRECT_NORMAL:
	default:
		cmd->cmd_word.phyDiskID = 0;
		break;
	}

	return;
}

static Bool ps3_is_no_calc_mapblocks(const struct PS3VDEntry *vd_info,
	const struct ps3_cmd *cmd)
{
	Bool ret = PS3_FALSE;
	(void)vd_info;

	if (ps3_scsih_is_sync_cache(scsi_cmnd_cdb(cmd->scmd))) {
		LOG_DEBUG("hno:%u is SYNCHRONIZE_CACHE\n",
			PS3_HOST(cmd->instance));
		ret = PS3_FALSE;
		goto l_out;
	}

	if (ps3_scsih_is_rw_type(cmd->io_attr.rw_flag) == PS3_FALSE) {
		LOG_DEBUG("hno:%u not rw cmd\n",
			PS3_HOST(cmd->instance));
		ret = PS3_TRUE;
		goto l_out;
	}

l_out:
	return ret;
}

static S32 ps3_scsih_vd_cmd_word_cpu_set(struct ps3_cmd *cmd,
	const struct PS3VDEntry *vd_info)
{
	S32 ret = PS3_SUCCESS;
	U32 num_blocks = cmd->io_attr.num_blocks; 
	U32 lba_lo = cmd->io_attr.lba_lo;
	U32 lba_hi = cmd->io_attr.lba_hi;
	U64 lba = 0;
	U8 que_offset = 0;
	PS3ReqFrameHead_s *req_head= &cmd->req_frame->frontendReq.reqHead;

	cmd->cmd_word.qMask = 0;
	req_head->mapBlockVer = PS3_CMDWORD_VER_INVALID;

	lba = ((U64)lba_hi << PS3_SHIFT_DWORD) | lba_lo;

	LOG_DEBUG("tid:0x%llx hno:%u CFID:%d lba:0x%llx map_block[%lld], num_blks:0x%x\n",
		cmd->trace_id, PS3_HOST(cmd->instance), cmd->index, lba,
		vd_info->mapBlock, num_blocks);

	if (ps3_is_no_calc_mapblocks(vd_info, cmd)) {
		LOG_DEBUG("tid:0x%llx hno:%u CFID:%u lba:0x%llx map_block[%lld], num_blks:0x%x\n",
			cmd->trace_id, PS3_HOST(cmd->instance), cmd->index, lba,
			vd_info->mapBlock, num_blocks);
		goto l_out;
	}

	req_head->mapBlockVer = cmd->io_attr.vd_entry->mapBlockVer;

	if (vd_info->mapBlock == 0) {
		PS3_BUG();
		ret = -PS3_FAILED;
		goto l_out;
	}
	que_offset = PS3_CMD_WORD_QUE_CALC(vd_info->diskPos.diskDev.ps3Dev.virtDiskID, lba,
		vd_info->mapBlock, (cmd->instance->ctrl_info.vdQueueNum - 1));
	cmd->cmd_word.qMask = 1 << que_offset;

	if ((PS3_SCSIH_NOT_SAME_MAPBLOCK(lba, vd_info->mapBlock, num_blocks))
		&& (cmd->instance->ioc_adpter->rw_cmd_is_need_split != NULL)
		&& (cmd->instance->ioc_adpter->rw_cmd_is_need_split(cmd))) {

		if (que_offset == (cmd->instance->ctrl_info.vdQueueNum - 1)) {
			cmd->cmd_word.qMask |= 0x1;
		} else {
			cmd->cmd_word.qMask |= 1 << (que_offset + 1);
		}
		req_head->mapBlockVer = PS3_CMDWORD_VER_INVALID;
		LOG_DEBUG("hno:%u map_block:%llu not same map block qMask 0x:%x\n",
			PS3_HOST(cmd->instance), vd_info->mapBlock, cmd->cmd_word.qMask);
	}

	LOG_DEBUG("tid:0x%llx hno:%u CFID:%d que:%d ver:%u\n",
		cmd->trace_id, PS3_HOST(cmd->instance), cmd->index,
		cmd->cmd_word.qMask, req_head->mapBlockVer);

l_out:
	return ret;

}

static S32 ps3_scsih_cmd_word_type_set(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;

	INJECT_START(PS3_ERR_IJ_IO_RW_FLAG_SET_UNKOWN, cmd);

	switch (cmd->io_attr.rw_flag) {
	case PS3_SCSI_CMD_TYPE_READ:
		cmd->cmd_word.type = PS3_CMDWORD_TYPE_READ;
		break;
	case PS3_SCSI_CMD_TYPE_WRITE:
	case PS3_SCSI_CMD_TYPE_UNMAP:
	case PS3_SCSI_CMD_TYPE_RW:
		cmd->cmd_word.type = PS3_CMDWORD_TYPE_WRITE;
		break;
	case PS3_SCSI_CMD_TYPE_NORW:
		cmd->cmd_word.type = PS3_CMDWORD_TYPE_MGR;
		break;
	default:
		cmd->cmd_word.type = PS3_CMDWORD_TYPE_MGR;
		ret = -PS3_FAILED;
		LOG_ERROR_LIM("hno:%u cmd word set type NOK CFID:%d ret:%d rw_flag:%d\n",
			PS3_HOST(cmd->instance), cmd->index, ret, cmd->io_attr.rw_flag);
		break;
	}

	return ret;
}

static inline U16 ps3_scsih_is_use_hard_cmd(const struct ps3_cmd *cmd)
{
	return ((cmd->cmd_word.direct == PS3_CMDWORD_DIRECT_OK) ||
		(cmd->cmd_word.direct == PS3_CMDWORD_DIRECT_ADVICE)) ?
		PS3_CMDWORD_FORMAT_HARDWARE : PS3_CMDWORD_FORMAT_FRONTEND;
}

static S32 ps3_scsih_vd_cmd_word_build(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	const struct PS3VDEntry *vd_info = cmd->io_attr.vd_entry;

	memset(&cmd->cmd_word, 0, sizeof(cmd->cmd_word));
	ret = ps3_scsih_cmd_word_type_set(cmd);
	if (unlikely(ret != PS3_SUCCESS)) {
		LOG_ERROR_LIM("tid:0x%llx hno:%u set cmd word fail "
			"cmd:%d chl:%u id:%u\n",
			cmd->trace_id, PS3_HOST(cmd->instance), cmd->index,
			PS3_SDEV_CHANNEL(cmd->scmd->device),
			PS3_SDEV_TARGET(cmd->scmd->device));
		ret = -PS3_FAILED;
		goto l_out;
	}

	ret = ps3_scsih_vd_cmd_word_cpu_set(cmd, vd_info);

#if 0
	if (unlikely(ret != PS3_SUCCESS)) {
		LOG_ERROR_LIM("tid:0x%llx hno:%u vd cmd word cpu set NOK, "
			"cmd:%u chl:%u id:%u\n",
			cmd->trace_id, PS3_HOST(cmd->instance), cmd->index,
			PS3_SDEV_CHANNEL(cmd->scmd->device),
			PS3_SDEV_TARGET(cmd->scmd->device));
		ret = -PS3_FAILED;
		goto l_out;
	}
#endif

	cmd->cmd_word.direct = cmd->io_attr.direct_flag; 
#ifndef _WINDOWS
	cmd->cmd_word.isrSN = ps3_msix_index_get(cmd, vd_info->dev_busy_scale);
#else
	cmd->cmd_word.isrSN = 0;
#endif

	ps3_scsih_vd_cmd_word_devid_build(cmd, vd_info);
	cmd->cmd_word.cmdFrameID = cmd->index;

l_out:
	return ret;
}

static S32 ps3_scsih_pd_cmd_word_build(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_scsi_priv_data *priv_data = NULL;

	cmd->cmd_word.direct = cmd->io_attr.direct_flag;
	ret = ps3_scsih_cmd_word_type_set(cmd);
	if (ret != PS3_SUCCESS) {
		goto l_out;
	}

#ifndef _WINDOWS
	cmd->cmd_word.isrSN = ps3_msix_index_get(cmd, 1);
#else
	cmd->cmd_word.isrSN = 0; 
#endif

	cmd->cmd_word.cmdFrameID = cmd->index;
	priv_data = scsi_device_private_data(cmd->scmd);
	cmd->cmd_word.phyDiskID = priv_data->disk_pos.diskDev.ps3Dev.phyDiskID;
	cmd->cmd_word.virtDiskID = PS3_INVALID_DEV_ID;

	if ((cmd->cmd_word.type == PS3_CMDWORD_TYPE_READ)
		|| (cmd->cmd_word.type == PS3_CMDWORD_TYPE_WRITE)){
		cmd->cmd_word.qMask = 0x1;
	}

l_out:
	return ret;
}

static S32 ps3_scsih_cmd_word_build(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;

	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		ret = ps3_scsih_vd_cmd_word_build(cmd);
	} else {
		ret = ps3_scsih_pd_cmd_word_build(cmd);
	}

	if (unlikely(ret != PS3_SUCCESS)) {
		LOG_ERROR_LIM("tid:0x%llx hno:%u CFID:%d cmd word build NOK!\n",
			cmd->trace_id, PS3_HOST(cmd->instance), cmd->index);
	}

	return ret;
}

static inline const char *ps3_scsih_print_rw_type(U8 rw_flag)
{
	static const char *rw_type_table[] = {
		[PS3_SCSI_CMD_TYPE_UNKOWN] = "SCMD_T_UNKOWN",
		[PS3_SCSI_CMD_TYPE_READ] ="SCMD_T_R",
		[PS3_SCSI_CMD_TYPE_WRITE] ="SCMD_T_W",
		[PS3_SCSI_CMD_TYPE_RW] ="SCMD_T_RW",
		[PS3_SCSI_CMD_TYPE_UNMAP] ="SCMD_T_UNMAP",
		[PS3_SCSI_CMD_TYPE_NORW] ="SCMD_T_NORW",
		[PS3_SCSI_CMD_TYPE_COUNT] ="SCMD_T_CNT"
	};

	if (rw_flag >= PS3_SCSI_CMD_TYPE_COUNT) {
		return rw_type_table[rw_flag];
	}

	return  rw_type_table[rw_flag];
}

static void ps3_scsih_print_cdb(const U8 *cdb)
{
	LOG_DEBUG("CDB: %02x %02x %02x %02x %02x %02x %02x %02x"
		" %02x %02x %02x %02x %02x %02x %02x %02x"
		" %02x %02x %02x %02x %02x %02x %02x %02x"
		" %02x %02x %02x %02x %02x %02x %02x %02x\n",
 		cdb[0], cdb[1], cdb[2], cdb[3], cdb[4],
 		cdb[5], cdb[6], cdb[7], cdb[8], cdb[9],
		cdb[10], cdb[11], cdb[12], cdb[13], cdb[14],
		cdb[15], cdb[16], cdb[17], cdb[18], cdb[19],
		cdb[20], cdb[21], cdb[22], cdb[23], cdb[24],
		cdb[25], cdb[26], cdb[27], cdb[28], cdb[29],
		cdb[30], cdb[31]);
}

static inline void ps3_scsih_print_req_head(struct ps3_cmd *cmd, U8 log_level)
{
	if (cmd->req_frame->frontendReq.reqHead.reqFrameFormat == PS3_REQFRAME_FORMAT_FRONTEND){
		PS3ReqFrameHead_s reqHead = cmd->req_frame->frontendReq.reqHead;
		LOG_LEVEL(log_level, "tid:0x%llx hno:%u fe head cmd_t:%d"
			" stype:%d CFID:%d ctrl:0x%x dev id:0x%x noReplyWord:%d"
			"dataFormat:%d reqFrameFormat:%d mapBlockVer:%d isWrite:%d"
			"virtDiskSeq:%d\n",
			reqHead.traceID, PS3_HOST(cmd->instance), reqHead.cmdType,
			reqHead.cmdSubType, reqHead.cmdFrameID, reqHead.control,
			reqHead.devID.diskID, reqHead.noReplyWord, reqHead.dataFormat,
			reqHead.reqFrameFormat, reqHead.mapBlockVer, reqHead.isWrite,
			reqHead.virtDiskSeq);
	} else {
		PS3ReqFrameHead_s reqHead = cmd->req_frame->hwReq.reqHead;
		LOG_LEVEL(log_level, "tid:0x%llx hno:%u fe head cmd_t:%d"
			" stype:%d CFID:%d ctrl:0x%x dev id:0x%x noReplyWord:%d"
			"dataFormat:%d reqFrameFormat:%d mapBlockVer:%d isWrite:%d"
			"virtDiskSeq:%d\n",
			reqHead.traceID, PS3_HOST(cmd->instance), reqHead.cmdType,
			reqHead.cmdSubType, reqHead.cmdFrameID, reqHead.control,
			reqHead.devID.diskID, reqHead.noReplyWord, reqHead.dataFormat,
			reqHead.reqFrameFormat, reqHead.mapBlockVer, reqHead.isWrite,
			reqHead.virtDiskSeq);
	}
}

static void ps3_scsih_print_hw_req(struct ps3_cmd *cmd, U8 log_level)
{
	IODT_V1_s *req = NULL;
	struct ps3_pd_entry * pd_entry = (struct ps3_pd_entry *)cmd->io_attr.pd_entry;
	if (pd_entry == NULL) {
		return;
	}
	ps3_scsih_print_req_head(cmd, log_level);
	switch (pd_entry->dev_type) {
	case PS3_DEV_TYPE_SAS_HDD:
	case PS3_DEV_TYPE_SAS_SSD:
		req = &cmd->req_frame->hwReq.sasReqFrame;
		LOG_LEVEL(log_level, "hno:%u cmdProto:%d cmdType:%d "
			"cmdLen:%d dataAddr:0x%llx sgeMode:%d "
			"direct:%d function:%d phyDiskID:%d "
			"reqFrameID:%d CmdWordType:%d cmdDir:%d "
			"dataBufLenDWAlign:0x%x iuSrc:%d sataCtl:%d\n",
			PS3_HOST(cmd->instance), req->protocolType, req->frameType,
			le32_to_cpu(req->cmdLen), le64_to_cpu(req->dataBaseAddr),
			req->dmaCfg.sgMode, req->commonWord.direct, req->commonWord.function,
			le16_to_cpu(req->commonWord.phyDiskID),
			le16_to_cpu(req->commonWord.reqFrameID),
			req->commonWord.type, req->cmdDir, le32_to_cpu(req->dataBufLenDWAlign),
			req->iuSrc, req->sasCtl);
		ps3_scsih_print_cdb(req->B.cdb);
		break;
	case PS3_DEV_TYPE_SATA_HDD:
	case PS3_DEV_TYPE_SATA_SSD:
		req = &cmd->req_frame->hwReq.sasReqFrame;
		LOG_LEVEL(log_level, "hno:%u cmdProto:%d cmdType:%d "
			"dataAddr:0x%llx sgeMode:%d "
			"direct:%d function:%d phyDiskID:%d "
			"reqFrameID:%d CmdWordType:%d cmdDir:%d "
			"dataBufLenDWAlign:0x%x lba:0x%llx "
			"opCode:0x%llx iuSrc:%d sataCtl:%d\n",
			PS3_HOST(cmd->instance), req->protocolType, req->frameType,
			le64_to_cpu(req->dataBaseAddr), req->dmaCfg.sgMode,
			req->commonWord.direct, req->commonWord.function,
			le16_to_cpu(req->commonWord.phyDiskID),
			le16_to_cpu(req->commonWord.reqFrameID), req->commonWord.type,
			req->cmdDir, le32_to_cpu(req->dataBufLenDWAlign),
			le64_to_cpu(req->C.lba), (U64)req->C.opCode, req->iuSrc, req->sataCtl);
		break;
	case PS3_DEV_TYPE_NVME_SSD:
		break;
	default :
		LOG_LEVEL(log_level, "hno:%u dev_type:%d\n",
			PS3_HOST(cmd->instance), pd_entry->dev_type);
		break;
	}
}

static void ps3_scsih_print_frontend_req(struct ps3_cmd *cmd, U8 log_level)
{
	ps3_scsih_print_req_head(cmd, log_level);
	LOG_LEVEL(log_level, "cmd sge_cnt:%d sge_of:%d data_len:%d isStream:%d"
		" num_blks:0x%x\n",
		cmd->req_frame->frontendReq.sgeCount,
		cmd->req_frame->frontendReq.sgeOffset,
		cmd->req_frame->frontendReq.dataXferLen,
		cmd->req_frame->frontendReq.vdAccAttr.isStream,
		cmd->io_attr.num_blocks);
	ps3_scsih_print_cdb(cmd->req_frame->frontendReq.cdb);
}

void ps3_scsih_print_req(struct ps3_cmd *cmd, U8 log_level)
{
	if (cmd->req_frame->frontendReq.reqHead.reqFrameFormat == PS3_REQFRAME_FORMAT_FRONTEND) {
		ps3_scsih_print_frontend_req(cmd, log_level);
	} else {
		ps3_scsih_print_hw_req(cmd, log_level);
	}
}

static void ps3_scsih_print_io_cmd(struct ps3_cmd *cmd)
{
	if(cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		if (cmd->io_attr.pd_entry != NULL) {
			LOG_DEBUG("tid:0x%llx hno:%u cmd CFID:%d, dev_t:%s [%d:%d:%d]\n",
				cmd->trace_id, PS3_HOST(cmd->instance), cmd->index,
				namePS3DevType((enum PS3DevType)cmd->io_attr.pd_entry->dev_type),
				cmd->io_attr.pd_entry->disk_pos.diskDev.ps3Dev.softChan,
				cmd->io_attr.pd_entry->disk_pos.diskDev.ps3Dev.devID,
				cmd->io_attr.pd_entry->disk_pos.diskDev.ps3Dev.phyDiskID);
		}
	}

	LOG_DEBUG("tid:0x%llx hno:%u print CMD: CFID:%u"
		" dev_t:%s outstand:%u is_retry_cmd:%d "
		"direct_f:%d rw_f:%s CMD_WORD: type:%d direct:%d"
		" qmask:0x%x CFID:%d isr_sn:%d vid:%d pid:%d"
		" lba:0x%llx opcode:0x%x\n",
		cmd->trace_id, PS3_HOST(cmd->instance), cmd->index,
		namePS3DevType((enum PS3DevType)cmd->io_attr.dev_type),
		ps3_atomic_read(&cmd->instance->cmd_statistics.io_outstanding),
		cmd->io_attr.is_retry_cmd, cmd->io_attr.direct_flag,
		ps3_scsih_print_rw_type(cmd->io_attr.rw_flag),

		cmd->cmd_word.type, cmd->cmd_word.direct,
		cmd->cmd_word.qMask, cmd->cmd_word.cmdFrameID,
		cmd->cmd_word.isrSN, cmd->cmd_word.virtDiskID,
		cmd->cmd_word.phyDiskID,
		((((U64)(cmd->io_attr.lba_hi)) << 32) + (cmd->io_attr.lba_lo)),
		cmd->scmd->cmnd[0]);

	ps3_scsih_print_req(cmd, LEVEL_DEBUG);
}

S32 ps3_scsih_cmd_build(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	static unsigned long j;
	ret = ps3_scsih_cmd_build_prepare(cmd);
	if (unlikely(ret != PS3_SUCCESS)) {
		LOG_DEBUG("tid:0x%llx hno:%u cmd_word build prepare fail CFID:%d ret:%d\n",
			cmd->trace_id, PS3_HOST(cmd->instance), cmd->index, ret);
		ret = ((-PS3_IO_CONFLICT == ret || -PS3_IO_REQUEUE == ret ||
			-PS3_IO_CONFLICT_IN_Q == ret) ?
			ret : SCSI_MLQUEUE_HOST_BUSY);
		goto l_out;
	}

	ret = ps3_scsih_cmd_word_build(cmd);
	if (unlikely(ret != PS3_SUCCESS)) {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u cmd_word build NOK CFID:%d ret:%d\n",
			cmd->trace_id, PS3_HOST(cmd->instance), cmd->index, ret);
		ret = SCSI_MLQUEUE_HOST_BUSY;
		goto l_out;
	}

	ret = ps3_scsih_req_frame_build(cmd);
	if (unlikely(ret != PS3_SUCCESS)) {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u req_frame NOK CFID:%d ret:%d\n",
			cmd->trace_id, PS3_HOST(cmd->instance), cmd->index, ret);
		ret = SCSI_MLQUEUE_HOST_BUSY;
		goto l_out;
	}

	ps3_scsih_print_io_cmd(cmd);
	PS3_IO_TRACE(cmd, PS3_IO_TRACE_DIRECT_SEND);

    ret = ps3_qos_decision(cmd);
l_out:
	return ret;
}

static inline union PS3DiskDev ps3_scsih_dev_id_get(const struct scsi_cmnd *s_cmd)
{
	struct ps3_scsi_priv_data *data = scsi_device_private_data(s_cmd);
	return data->disk_pos.diskDev;
}

static inline Bool ps3_scsih_is_vd_accelerate(struct ps3_cmd *cmd)
{
	return ps3_scsih_vd_acc_att_build(cmd);
}

static inline void ps3_sas_cdb_build(struct ps3_cmd *cmd)
{
	U32 blocks = 0;

	memset(cmd->io_attr.cdb, 0, PS3_FRAME_CDB_BUFLEN);
	memcpy(cmd->io_attr.cdb, cmd->scmd->cmnd, cmd->scmd->cmd_len);
#ifndef _WINDOWS
	blocks = cmd->io_attr.num_blocks << ilog2(cmd->io_attr.vd_entry->sectorSize);
#else
	blocks = cmd->io_attr.num_blocks * cmd->io_attr.vd_entry->sectorSize;
#endif
	blocks = blocks >> ps3_blocksize_to_shift(cmd->io_attr.pd_entry->sector_size);

	ps3_scsih_cdb_rebuild(cmd->io_attr.cdb, cmd->scmd->cmd_len, blocks,
		(U32)cmd->io_attr.plba, (U32)(cmd->io_attr.plba >> PS3_SHIFT_DWORD));
}
S32 ps3_vd_direct_req_frame_build(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;

	switch(cmd->io_attr.pd_entry->dev_type) {
	case PS3_DEV_TYPE_SAS_SSD:
	case PS3_DEV_TYPE_SAS_HDD:
		ps3_sas_cdb_build(cmd);
		ret = ps3_scsih_sas_req_frame_build(cmd);
		break;
	case PS3_DEV_TYPE_SATA_SSD:
	case PS3_DEV_TYPE_SATA_HDD:
		ret = ps3_scsih_sata_req_frame_build(cmd);
		break;
	case PS3_DEV_TYPE_NVME_SSD:
		ret = ps3_scsih_nvme_req_frame_build(cmd);
		break;
	default:
		PS3_BUG();
		ret = -PS3_FAILED;
		break;
	}
	return ret;
}

static inline U32 ps3_scsih_data_direction_build(const struct ps3_cmd *cmd)
{
	U32 ret;
	static unsigned long j;
	if((cmd->scmd->sc_data_direction == DMA_BIDIRECTIONAL)
		|| (cmd->scmd->sc_data_direction == DMA_TO_DEVICE)){
		ret = PS3_DATA_DIRECTION_WRITE;
	} else if((cmd->scmd->sc_data_direction == DMA_FROM_DEVICE)
		|| (cmd->scmd->sc_data_direction == DMA_NONE)){
		ret = PS3_DATA_DIRECTION_READ;
	} else {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u tid:0x%llx date direction:%d check NOK",
			 PS3_HOST(cmd->instance),cmd->trace_id, cmd->scmd->sc_data_direction);
		ret = PS3_DATA_DIRECTION_WRITE;
	}
	return ret;
}

static S32 ps3_scsih_vd_frontend_req_build(struct ps3_cmd *cmd,
	PS3FrontEndReqFrame_s *req, U16 sge_count)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_scsi_priv_data *p_priv_data = scsi_device_private_data(cmd->scmd);
	ps3_scsih_req_frame_head_build(cmd, PS3_REQFRAME_FORMAT_FRONTEND);

	memcpy(req->cdb, cmd->scmd->cmnd, cmd->scmd->cmd_len);
	req->dataXferLen = cmd->io_attr.sgl_buf_len;
	req->sgeCount = sge_count;
	req->sgeOffset = offsetof(PS3FrontEndReqFrame_s, sgl) >> 2;
	if(cmd->io_attr.seq_flag == SCSI_RW_UNUSED_CMD &&
		cmd->instance->ioc_adpter->scsih_stream_is_detect != NULL) {
		cmd->instance->ioc_adpter->scsih_stream_is_detect(cmd);
	}
	req->vdAccAttr.isStream = ((cmd->io_attr.seq_flag == SCSI_RW_SEQ_CMD)?(1):(0));
	if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
		req->vdAccAttr.ioOutStandingCnt = (U16)ps3_atomic_read(&p_priv_data->rd_io_outstand);
	}
	if (ps3_scsih_is_vd_accelerate(cmd)) {
		LOG_DEBUG("tid:0x%llx hno:%u cmd_type:%d CFID:%d "
			"ret:%d vlba->plba:0x%llx\n",
			cmd->trace_id, PS3_HOST(cmd->instance),
			req->reqHead.cmdType, cmd->index, ret, cmd->io_attr.plba);
	}

	return ret;
}

static inline void ps3_align_16bytes_check(U64 addr)
{
	if (unlikely(addr & PS3_SCSI_ALINNMENT_MASK)) {
		LOG_ERROR_LIM("addr:0x%llx not align\n", addr);
	}
}

static inline void ps3_align_4bytes_check(U64 addr)
{
	if (unlikely(addr & PS3_SCSI_ALINNMENT_MASK)) {
		LOG_ERROR_LIM("addr:0x%llx not align\n", addr);
	}
}

static inline Bool ps3_is_invalid_ossgl_count(U16 valid_os_sge_count,
	U16 max_drv_sge_count)
{
	return (max_drv_sge_count < valid_os_sge_count + 1);
}

static inline void ps3_last_sge_build(struct PS3Sge *sgl_ptr,
	const ps3_scatter_gather_element *os_sgl)
{
	sgl_ptr->length   = cpu_to_le32(sg_dma_len(os_sgl));
	sgl_ptr->addr     = cpu_to_le64(sg_dma_address_u64(os_sgl));
	sgl_ptr->lastSge  = 1;
	sgl_ptr->ext      = 0;

	return ;
}

static inline Bool ps3_is_last_sge(S32 sgl_idx, U16 os_sge_count)
{
	return ((sgl_idx + 1) == os_sge_count);
}

static inline Bool ps3_is_list_sge(S32 sgl_idx, U16 frame_sge_count)
{
	return ((sgl_idx + 1) == frame_sge_count);
}

static inline void ps3_list_sge_build(struct PS3Sge *sgl_ptr,
	const struct ps3_cmd *cmd, U16 ext_sge_size)
{
	sgl_ptr->length  = ext_sge_size;
	sgl_ptr->addr    = cpu_to_le64(cmd->ext_buf_phys);
	sgl_ptr->lastSge = 0;
	sgl_ptr->ext     = 1;

	return;
}

static U32 ps3_scsih_data_buf_len(struct ps3_cmd *cmd, U32 os_sge_count)
{
	struct scsi_cmnd *scp = cmd->scmd;
	ps3_scatter_gather_element *os_sgl = NULL;
	U32 index = 0;

	cmd->io_attr.sgl_buf_len = 0;
	scsi_for_each_sg(scp, os_sgl, os_sge_count, index) {
		cmd->io_attr.sgl_buf_len += sg_dma_len(os_sgl);
		mb();
	}

	if (scsi_bufflen(scp) != cmd->io_attr.sgl_buf_len) {
		LOG_INFO("data_buf:%u buf_len:%u mismatch\n",
			scsi_bufflen(scp), cmd->io_attr.sgl_buf_len);
	}

	return cmd->io_attr.sgl_buf_len;
}

static inline U16 ps3_drv_max_sge_count(const struct ps3_cmd *cmd,
	U16 frame_sge_count)
{
	U16 count = 0;

	if (cmd->instance->cmd_context.sgl_mode_support) {
		count = ((U16)cmd->instance->cmd_context.ext_sge_frame_count +
			frame_sge_count);
	} else {
		count = PS3_MAX((U16)cmd->instance->cmd_context.ext_sge_frame_count + 1,
			frame_sge_count);
	}

	return count;
}

static inline U16 ps3_scsih_frame_ext_sge_pos(const struct ps3_cmd *cmd,
	U16 frame_sge_count, U16 os_sge_count)
{
	U16 pos = 0;

	if (cmd->instance->cmd_context.sgl_mode_support) {
		pos = frame_sge_count;
	} else {
		pos = (os_sge_count <= frame_sge_count) ? 0 : 1;
	}

	return pos;
}

static inline U16 ps3_scsih_ext_sge_size_calc(U16 ext_sge_pos,
	U16 frame_sge_count, U16 os_sge_count)
{
	U16 cnt = 0;

	if (ext_sge_pos == 0) {
		cnt = 0;
	} else if (ext_sge_pos == 1) {
		cnt = os_sge_count;
	} else {
		cnt = os_sge_count - frame_sge_count + 1;
	}
	return cnt * sizeof(struct PS3Sge);
}

static U16 ps3_scsih_sgl_build(struct ps3_cmd *cmd, struct PS3Sge *sge,
	U8 frame_sge_count)
{
	S32 index = 0;
	U16 max_drv_sge_count = 0;
	U16 os_sge_count = cmd->os_sge_map_count;
	struct scsi_cmnd *scp = cmd->scmd;
	ps3_scatter_gather_element *os_sgl = NULL;
	struct PS3Sge *sge_ptr = sge;
	U16 ret_sge_count = 0;
	U16 ext_sge_pos = 0; 
	U16 ext_sge_size = 0;
	static unsigned long j;
	cmd->io_attr.sgl_buf_len = 0;
	max_drv_sge_count = ps3_drv_max_sge_count(cmd, frame_sge_count);

	if (unlikely(ps3_is_invalid_ossgl_count(os_sge_count,
		max_drv_sge_count))) {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u CFID:%u io is "
			"too huge max_drv_sge_count:%d os_sge_count:%u\n",
			cmd->trace_id, PS3_HOST(cmd->instance), cmd->index,
			max_drv_sge_count , os_sge_count);
		goto l_out;
	}

	memset(&cmd->req_frame->hwReq.sgl, 0, sizeof(cmd->req_frame->hwReq.sgl));
	ext_sge_pos = ps3_scsih_frame_ext_sge_pos(cmd, frame_sge_count, os_sge_count);
	ext_sge_size = ps3_scsih_ext_sge_size_calc(ext_sge_pos, frame_sge_count,
		os_sge_count);

	ret_sge_count = os_sge_count;
	scsi_for_each_sg(scp, os_sgl, os_sge_count, index) {
		if (ps3_is_last_sge(index, os_sge_count)) {
			ps3_last_sge_build(sge_ptr, os_sgl);
			cmd->io_attr.sgl_buf_len += sge_ptr->length;
			ps3_align_4bytes_check(sge_ptr->addr);
			LOG_DEBUG("sgl addr:0x%llx len:%d\n", sge_ptr->addr,
				sge_ptr->length);
			break;
		}

		if (ps3_is_list_sge(index, ext_sge_pos)) {
			ps3_list_sge_build(sge_ptr, cmd, ext_sge_size);
			sge_ptr = (struct PS3Sge*)cmd->ext_buf;
			ret_sge_count += 1;
		}

		sge_ptr->length = cpu_to_le32(sg_dma_len(os_sgl));
		sge_ptr->addr = cpu_to_le64(sg_dma_address_u64(os_sgl));
		sge_ptr->lastSge = 0;
		sge_ptr->ext = 0;
		cmd->io_attr.sgl_buf_len += sge_ptr->length;
		ps3_align_4bytes_check(sge_ptr->addr);

		sge_ptr++;
	}
l_out:
	return ret_sge_count;
}

static S32 ps3_vd_normal_req_frame_build(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	PS3FrontEndReqFrame_s *req = &cmd->req_frame->frontendReq;
	U16 sge_count = 0;
	static unsigned long j;
	memset(&req->reserved, 0 , sizeof(req->reserved));
	req->vdAccAttr.isAccActive = 0;
	req->vdAccAttr.reserved1 = 0;
	memset(&req->vdAccAttr.reserved2, 0, sizeof(req->vdAccAttr.reserved2));

	sge_count = ps3_scsih_frontend_data_buf_build(cmd, req);
	if (sge_count == 0 && cmd->os_sge_map_count != 0) {
		cmd->io_attr.is_use_frontend_prp = PS3_FALSE;
		sge_count = ps3_scsih_frontend_data_buf_build(cmd, req);
		if (sge_count == 0 && cmd->os_sge_map_count != 0) {
			ret = -PS3_FAILED;
			LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u tid:0x%llx drv_sge_count:%u "
				"os_sge_count:%u\n", PS3_HOST(cmd->instance),
				cmd->trace_id, sge_count, cmd->os_sge_map_count);
			goto l_out;
		}
	}

	ret = ps3_scsih_vd_frontend_req_build(cmd, req, sge_count);
	if (unlikely(ret != PS3_SUCCESS)) {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u CFID:%d ret:%d frondend build NOK\n",
			cmd->trace_id, PS3_HOST(cmd->instance), cmd->index, ret);
	}

l_out:
	return ret;
}

static inline S32 ps3_vd_adv_to_normal_req_frame_rebuild(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	static unsigned long j;
	cmd->io_attr.direct_flag = PS3_CMDWORD_DIRECT_NORMAL;
	cmd->io_attr.is_use_frontend_prp = PS3_FALSE;
	memset((void *) &cmd->cmd_word, 0, sizeof(struct PS3CmdWord));
	memset((void *)cmd->req_frame, 0, sizeof(union PS3ReqFrame));
	memset(cmd->ext_buf, 0, cmd->instance->cmd_context.ext_buf_size);
	ret = ps3_scsih_vd_cmd_word_build(cmd);
	if (unlikely(ret != PS3_SUCCESS)) {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u cmd_word build NOK CFID:%d ret:%d\n",
			cmd->trace_id, PS3_HOST(cmd->instance), cmd->index, ret);
		ret = -PS3_FAILED;
		goto l_out;
	}
	ret = ps3_vd_normal_req_frame_build(cmd);

	ps3_scsih_print_io_cmd(cmd);
l_out:
	return ret;
}

static S32 ps3_scsih_vd_req_frame_build(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	static unsigned long j;
	if ((cmd->io_attr.direct_flag == PS3_CMDWORD_DIRECT_OK) ||
		(cmd->io_attr.direct_flag == PS3_CMDWORD_DIRECT_ADVICE)) {
		ret = ps3_vd_direct_req_frame_build(cmd);

		if ((ret != PS3_SUCCESS) &&
			(cmd->io_attr.pd_entry->dev_type == PS3_DEV_TYPE_NVME_SSD) ) {
			LOG_WARN_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u advice direct NVMe vd NOK, to normal"
				"is_use_frontend_prp:%d\n", cmd->trace_id,
				PS3_HOST(cmd->instance), cmd->io_attr.is_use_frontend_prp);
			ret = ps3_vd_adv_to_normal_req_frame_rebuild(cmd);
		}
	} else {
		ret = ps3_vd_normal_req_frame_build(cmd);
	}
	return ret;
}

static S32 ps3_software_zone_build(struct ps3_cmd *cmd,
	U64 virtDiskLba, U8 type, U16 sge_count)
{
	PS3SoftwareZone_s *zone = NULL;
	U32 num_blocks = cmd->io_attr.num_blocks;
	S32 ret = PS3_SUCCESS;
	static unsigned long j;
	INJECT_START(PS3_ERR_IJ_SOFT_ZONE_TYPE_SET_UNKOWN, &type);
	if(unlikely((type < PS3_DEV_TYPE_SAS_HDD) ||
		(type >= PS3_DEV_TYPE_COUNT))) {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u type:%d\n",
			cmd->trace_id, PS3_HOST(cmd->instance), type);
		ret = -PS3_FAILED;
		goto l_ret;
	}

	zone = &cmd->req_frame->hwReq.softwareZone;
	ps3_scsih_cdb_opcode_get(cmd->scmd->cmnd, &zone->opcode, &zone->subOpcode);

	zone->virtDiskLba = cpu_to_le64(virtDiskLba);
	zone->numBlocks = cpu_to_le32(num_blocks);
	zone->sgeCount = sge_count;

	if( type == PS3_DEV_TYPE_NVME_SSD) {
		zone->sglOffset = offsetof(struct PS3NvmeCmdDw0_9, dPtr) >> 2;
		zone->sglFormat = 1; 
	} else {
		zone->sglOffset = offsetof(struct PS3HwReqFrame, sgl) >> 2;
		zone->sglFormat = 0; 
	}
	zone->isResendCmd = 0;

l_ret:
	return ret;
}

static Bool ps3_prp_build_check(struct ps3_cmd *cmd, U32 data_len)
{
	struct scsi_cmnd *scmd = cmd->scmd;
	Bool prp_convert_support = PS3_TRUE;
	S32 index = 0;
	U16 total_sge_count = cmd->os_sge_map_count;
	U32 nvme_page_size = cmd->instance->cmd_attr.nvme_page_size;
	ps3_scatter_gather_element *sge_ptr = scsi_sglist(scmd);
	U64 sge_addr = 0;
	U32 first_prp_len = 0;
	U32 nvme_page_size_mask = nvme_page_size - 1;
	static unsigned long j;
	(void)data_len;
	LOG_DEBUG("hno:%u CFID:%u sge_count:%u\n",
		PS3_HOST(cmd->instance), cmd->index, total_sge_count);

	scsi_for_each_sg(scmd, sge_ptr, total_sge_count, index) {
		sge_addr = sg_dma_address(sge_ptr);
#if 0
		LOG_DEBUG("hno:%u CFID:%u index:%u sge_addr:0x%llx len:%u\n",
			PS3_HOST(cmd->instance), cmd->index, index, sge_addr,
			sg_dma_len(sge_ptr));
#endif
		if (index == 0) {
			first_prp_len = nvme_page_size - (sge_addr & nvme_page_size_mask);
			if (first_prp_len >= sg_dma_len(sge_ptr)) {
				prp_convert_support = PS3_TRUE;
				break;
			}

			if (total_sge_count <= 1) {
				prp_convert_support = PS3_TRUE;
				break;
			} else if (ps3_utility_mod64(sge_addr + sg_dma_len(sge_ptr),
				nvme_page_size)) {
				prp_convert_support = PS3_FALSE;
				LOG_WARN_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u CFID:%u index:%u "
					"sge_addr:0x%llx len:%u\n",
					PS3_HOST(cmd->instance), cmd->index,
					index, sge_addr, sg_dma_len(sge_ptr));
				break;
			}
		} else {
			if ((total_sge_count > 1) && (index == (total_sge_count - 1))) {
				if (ps3_utility_mod64(sge_addr, nvme_page_size)) {
					LOG_WARN_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u CFID:%u index:%u "
						"sge_addr:0x%llx len:%u\n",
						PS3_HOST(cmd->instance), cmd->index,
						index, sge_addr, sg_dma_len(sge_ptr));
					prp_convert_support = PS3_FALSE;
					break;
				}
			}

			if ((total_sge_count > 1) && (index != (total_sge_count - 1))) {
				if (ps3_utility_mod64(sg_dma_len(sge_ptr), nvme_page_size) ||
					ps3_utility_mod64(sge_addr, nvme_page_size)) {
					LOG_WARN_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "middle hno:%u CFID:%u index:%u "
						"sge_addr:0x%llx len:%u\n",
						PS3_HOST(cmd->instance), cmd->index,
						index, sge_addr, sg_dma_len(sge_ptr));
					prp_convert_support = PS3_FALSE;
					break;
				}
			}
		}
	}

	return prp_convert_support;
}

static U16 ps3_scsih_prp_build(struct ps3_cmd *cmd, U64 *prp_ptr,
	U8 frame_prp_count, U32 data_len, Bool is_need_clean_sgl)
{
	struct scsi_cmnd *scp = cmd->scmd;
	ps3_scatter_gather_element *sge_scmd = scsi_sglist(scp);
	U32 page_size = cmd->instance->cmd_attr.nvme_page_size;
	U32 nvme_page_size_mask = page_size - 1;
	U32 sge_len = sg_dma_len(sge_scmd);
	U64 sge_addr = sg_dma_address_u64(sge_scmd);
	U32 first_prp_len = 0;
	U16 prp_entry_count = 0;
	static unsigned long j;
	if (is_need_clean_sgl) {
		memset(&cmd->req_frame->hwReq.sgl, 0, sizeof(cmd->req_frame->hwReq.sgl));
	}
	*prp_ptr = sge_addr;
	prp_entry_count++;
	first_prp_len = page_size - (sge_addr & nvme_page_size_mask);

	LOG_DEBUG("hno:%u CFID:%u sge0_addr:0x%llx data_len:%u sge_len:%u"
		"first_max_prp_len:%u\n", PS3_HOST(cmd->instance),
		 cmd->index, sge_addr, data_len, sge_len, first_prp_len);

	data_len = (data_len > first_prp_len)? (data_len - first_prp_len) : 0;
	if (data_len <= 0) {
		goto l_out;
	}

	if (sge_len > first_prp_len) {
		sge_addr += first_prp_len;
		sge_len = (sge_len > first_prp_len) ? (sge_len - first_prp_len) : 0 ;
	} else {
		sge_scmd = sg_next(sge_scmd);
		sge_len = sg_dma_len(sge_scmd);
		sge_addr = sg_dma_address_u64(sge_scmd);
	}

	prp_ptr++;
	frame_prp_count--;

	for (;;) {
		if (prp_entry_count >= cmd->instance->cmd_context.max_prp_count) {
			LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u CFID:%u prp_entry_count:%u "
				"max_prp_count:%u\n", cmd->trace_id, PS3_HOST(cmd->instance),
				cmd->index, prp_entry_count, cmd->instance->cmd_context.max_prp_count);
			prp_entry_count = 0;
			goto l_out;
		}

		if (frame_prp_count == 1) {
			if (data_len > page_size) {
				*prp_ptr = cmd->ext_buf_phys;
				prp_ptr = (U64*)cmd->ext_buf;
				prp_entry_count++;
			}
			frame_prp_count--;
		}

		*prp_ptr = sge_addr;
		prp_entry_count++;

		sge_len = (sge_len > page_size) ? (sge_len - page_size) : 0;
		data_len = (data_len > page_size) ? (data_len - page_size) : 0;
		if (frame_prp_count > 0) {
			frame_prp_count--;
		}

		if (data_len <= 0) {
			LOG_DEBUG("end hno:%u CFID:%u data_len:%u prp_entry:0x%llx "
				"sge_addr:0x%llx sge_len:%u prp_entry_count:%u\n",
				 PS3_HOST(cmd->instance), cmd->index, data_len, *prp_ptr,
				 sge_addr, sge_len, prp_entry_count);
			break;
		}

		if (unlikely(cmd->instance->is_print_special_log)) {
			LOG_DEBUG("end hno:%u CFID:%u data_len:%u prp_entry:0x%llx "
				"sge_addr:0x%llx sge_len:%u\n", PS3_HOST(cmd->instance),
				cmd->index, data_len, *prp_ptr, sge_addr, sge_len);
		}

		prp_ptr++;

		if (sge_len > 0) {
			sge_addr += page_size;
			continue;
		}
		sge_scmd = sg_next(sge_scmd);
		sge_addr = sg_dma_address_u64(sge_scmd);
		sge_len = sg_dma_len(sge_scmd);
	}

l_out:
	return prp_entry_count;
}

static void ps3_scsih_frontend_prp_build(struct ps3_cmd *cmd, U64 *prp_ptr,
	U32 data_len, U16 prp_count)
{
	struct scsi_cmnd *scp = cmd->scmd;
	ps3_scatter_gather_element *sge_scmd = scsi_sglist(scp);
	struct PS3Sge *sge_ptr = (struct PS3Sge *)prp_ptr;
	U32 page_size = cmd->instance->cmd_attr.nvme_page_size;
	U32 nvme_page_size_mask = page_size - 1;
	U32 first_prp_len = 0;
	U32 sge_len = sg_dma_len(sge_scmd);
	U64 sge_addr = sg_dma_address_u64(sge_scmd);

	first_prp_len = page_size - (sg_dma_address_u64(sge_scmd) & nvme_page_size_mask);
	data_len = (data_len > first_prp_len)? (data_len - first_prp_len) : 0;

	sge_ptr->length = first_prp_len;
	sge_ptr->addr = sge_addr;
	sge_ptr->lastSge = 0;
	sge_ptr->ext = 0;
	ps3_align_16bytes_check(sge_ptr->addr);

	LOG_DEBUG("hno:%u CFID:%u sge0_addr:0x%llx sge_len:%u\n",
		PS3_HOST(cmd->instance), cmd->index, sge_ptr->addr, sge_ptr->length);

	if(data_len <= 0){
		goto l_out;
	}

	if (sge_len > first_prp_len) {
		sge_addr += first_prp_len;
		sge_len = (sge_len > first_prp_len) ? (sge_len - first_prp_len) : 0 ;
	} else {
		sge_scmd = sg_next(sge_scmd);
		sge_len = sg_dma_len(sge_scmd);
		sge_addr = sg_dma_address_u64(sge_scmd);
	}

	sge_ptr++;

	if (data_len <= page_size) {
		sge_ptr->length = data_len;
		sge_ptr->addr = sge_addr;
		sge_ptr->lastSge = 1;
		sge_ptr->ext = 0;
		ps3_align_16bytes_check(sge_ptr->addr);
		goto l_out;
	} else {
		sge_ptr->length = (prp_count - PS3_FRAME_REQ_PRP_NUM_FE) * sizeof(U64);
		sge_ptr->addr = cmd->ext_buf_phys;
		sge_ptr->lastSge = 0;
		sge_ptr->ext = 1;
		sge_ptr = (struct PS3Sge *)prp_ptr;
		sge_ptr = sge_ptr + PS3_FRAME_REQ_SGE_NUM_FE - 1;
		sge_ptr->length = (prp_count - PS3_FRAME_REQ_PRP_NUM_FE) * sizeof(U64);
		sge_ptr->addr = cmd->ext_buf_phys;
		sge_ptr->lastSge = 0;
		sge_ptr->ext = 1;
	}
	LOG_DEBUG("hno:%u CFID:%u sge1_addr:0x%llx sge_len:%u sge_ext:%d\n",
		PS3_HOST(cmd->instance), cmd->index, sge_ptr->addr, sge_ptr->length, sge_ptr->ext);
l_out:
	return ;
}

static inline S32 ps3_scsih_prp_len_check(struct ps3_cmd *cmd, U32 data_len)
{
	S32 ret = PS3_SUCCESS;
	U32 max_prp_count = cmd->instance->cmd_context.max_prp_count;
	U64 max_nvme_data_size = (U64)(max_prp_count - (U32)PS3_FRAME_REQ_PRP_NUM_FE - 1) *
		cmd->instance->cmd_attr.nvme_page_size;
	static unsigned long j;
	if (unlikely(max_nvme_data_size < data_len)) {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u io is too huge "
			"[max_nvme_data_size:%llu][data_len:%d]\n",
			cmd->trace_id, PS3_HOST(cmd->instance),
			max_nvme_data_size , data_len);
		ret = -PS3_FAILED;
	}

	return ret;
}
#if 0
static U16 ps3_scsih_nvme_normal_cmd_split(struct ps3_cmd *cmd,
	PS3FrontEndReqFrame_s *req, U32 data_len)
{
	U16 prp_count = 0;

	(void)cmd;
	(void)req;
	(void)data_len;

	return prp_count;
}
#endif
static U16 ps3_scsih_frontend_data_buf_build(struct ps3_cmd *cmd,
	PS3FrontEndReqFrame_s *req)
{
	U16 sge_count = 0;
	U32 data_len = 0;
	static unsigned long j;
	if (cmd->io_attr.is_use_frontend_prp) {
		if (cmd->os_sge_map_count == 0) {
			LOG_WARN_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u CFID:%u os_sge_map_count==0\n",
				cmd->trace_id, PS3_HOST(cmd->instance), cmd->index);
			sge_count = 0;
			goto l_out;
		}
		data_len = ps3_scsih_data_buf_len(cmd, cmd->os_sge_map_count);
		if (ps3_scsih_prp_len_check(cmd, data_len) != PS3_SUCCESS) {
			sge_count = 0;
			goto l_out;
		}

		if (ps3_prp_build_check(cmd, data_len)) {
			sge_count = ps3_scsih_prp_build(cmd, &req->prp.prp1,
				PS3_FRAME_REQ_PRP_NUM_FE, data_len, PS3_TRUE);

			ps3_scsih_frontend_prp_build(cmd, &req->prp.prp1,
				data_len, sge_count);
		} else {
#if 0
			LOG_WARN("tid:0x%llx hno:%u CFID:%u need split cmd\n",
				cmd->trace_id, PS3_HOST(cmd->instance), cmd->index);

			sge_count = ps3_scsih_nvme_normal_cmd_split(cmd, req, data_len);
#else
			LOG_WARN_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u CFID:%u nvme prp change to sgl\n",
				cmd->trace_id, PS3_HOST(cmd->instance), cmd->index);
			sge_count = 0;
#endif
		}
	} else {
		sge_count = ps3_scsih_sgl_build(cmd, req->sgl, PS3_FRAME_REQ_SGE_NUM_FE);
	}

l_out:
	return sge_count;
}

static S32 ps3_scsih_pd_frontend_req_build(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	PS3FrontEndReqFrame_s *req = &cmd->req_frame->frontendReq;
	static unsigned long j;
	ps3_scsih_req_frame_head_build(cmd, PS3_REQFRAME_FORMAT_FRONTEND);

	memset(&req->reserved, 0, sizeof(req->reserved));
	req->vdAccAttr.isAccActive = 0;
	req->vdAccAttr.reserved1 = 0;
	memset(&req->vdAccAttr.reserved2, 0, sizeof(req->vdAccAttr.reserved2));

	req->sgeCount = ps3_scsih_frontend_data_buf_build(cmd, req);
	if (req->sgeCount == 0 && cmd->os_sge_map_count != 0) {
		cmd->io_attr.is_use_frontend_prp = PS3_FALSE;
		cmd->req_frame->hwReq.reqHead.dataFormat = PS3_SGL;
		req->sgeCount = ps3_scsih_frontend_data_buf_build(cmd, req);
		if (req->sgeCount == 0 && cmd->os_sge_map_count != 0) {
			ret = -PS3_FAILED;
			LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u tid:0x%llx drv_sge_count:%u "
				"os_sge_count:%u\n", PS3_HOST(cmd->instance),
				cmd->trace_id, req->sgeCount, cmd->os_sge_map_count);
			goto l_out;
		}
	}

	memcpy(req->cdb, cmd->scmd->cmnd, cmd->scmd->cmd_len);

	req->sgeOffset = offsetof(PS3FrontEndReqFrame_s, sgl) >> 2;
	req->dataXferLen = cmd->io_attr.sgl_buf_len;

l_out:
	return ret;
}

static U16 ps3_scsi_cmd_timeout_get(struct ps3_cmd *cmd)
{
	U32 time_s = SCMD_GET_REQUEST(cmd->scmd)->timeout / HZ;
	U16 timeout = 0;
	if (time_s > 0xfffe) {
		timeout = 0;
		return timeout;
	}
	timeout = time_s;
	return timeout;
}

static void ps3_scsih_req_frame_head_build(struct ps3_cmd *cmd, U8 req_frame_format)
{
	PS3ReqFrameHead_s *req_head= &cmd->req_frame->hwReq.reqHead;
	static unsigned long j;
	req_head->reqFrameFormat = req_frame_format;
	req_head->cmdSubType = 0;
	req_head->cmdFrameID = cmd->index;
	req_head->noReplyWord = PS3_CMD_WORD_NEED_REPLY_WORD;
	req_head->dataFormat = cmd->io_attr.is_use_frontend_prp || req_frame_format == PS3_REQFRAME_FORMAT_NVME ? \
		PS3_PRP : PS3_SGL;
	req_head->isWrite = ps3_scsih_data_direction_build(cmd);
	req_head->timeout = ps3_scsi_cmd_timeout_get(cmd);
	req_head->traceID = cmd->trace_id;
	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		req_head->devID = cmd->io_attr.vd_entry->diskPos.diskDev;
		if (unlikely(req_head->devID.diskID == 0)) {
			LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u chl:%u id:%u dev id:0x%x NOK\n",
				cmd->trace_id, PS3_HOST(cmd->instance), PS3_SDEV_CHANNEL(cmd->scmd->device),
				PS3_SDEV_TARGET(cmd->scmd->device), req_head->devID.diskID);
		}

		req_head->cmdType = ps3_scsih_is_rw_type(cmd->io_attr.rw_flag) ?
			PS3_CMD_VD_SCSI_IO_RW : PS3_CMD_VD_SCSI_IO_NORW;
		req_head->virtDiskSeq = cmd->io_attr.vd_entry->virtDiskSeq;

	}else {
		req_head->devID.diskID = PS3_DISKID(&cmd->io_attr.pd_entry->disk_pos);
		req_head->mapBlockVer = PS3_CMDWORD_VER_INVALID;
		req_head->cmdType = ps3_scsih_is_rw_type(cmd->io_attr.rw_flag) ?
			PS3_CMD_PD_SCSI_IO_RW : PS3_CMD_PD_SCSI_IO_NORW;
	}
	return;
}

static inline U64 ps3_scsih_direct_sgl_base_addr(const struct ps3_cmd *cmd,
	U16 sge_count)
{
	U64 data_base = 0;

	if (sge_count == PS3_SGL_MODE_SGE_COUNT_DIRECT) {
		data_base = cmd->req_frame->hwReq.sgl[0].addr;
	} else if ((sge_count > PS3_FRAME_REQ_SGE_NUM_HW) &&
			(!cmd->instance->cmd_context.sgl_mode_support)) {
			data_base = cmd->ext_buf_phys;
			ps3_align_16bytes_check(data_base);
	} else {
		data_base = cmd->req_frame_phys + offsetof(PS3HwReqFrame_s, sgl);
		ps3_align_16bytes_check(data_base);
	}

	return data_base;
}

static inline void ps3_scsih_sata_iodt_data_build(struct ps3_cmd *cmd,
	U32 sge_count, U64 data_addr)
{
	IODT_V1_s *iodt = &cmd->req_frame->hwReq.sasReqFrame;
	struct ps3_scsi_io_attr *cmd_io_attr = &cmd->io_attr;
	U16 sector_size = cmd_io_attr->pd_entry->sector_size;
#ifndef _WINDOWS
	U32 data_buf_len = cmd_io_attr->num_blocks << ilog2(sector_size);
#else
	U32 data_buf_len = cmd_io_attr->num_blocks * sector_size;
#endif
	switch(sge_count) {
	case PS3_SGL_MODE_SGE_COUNT_NO_DATA:
		iodt->dataBufLenDWAlign = 0;
		iodt->dataBaseAddr = 0;
		iodt->dmaCfg.sgMode = IODT_SGEMODE_DIRECT;
		break;
	case PS3_SGL_MODE_SGE_COUNT_DIRECT:
		iodt->dataBaseAddr = cpu_to_le64(data_addr);
		iodt->dataBufLenDWAlign = cpu_to_le32(ENCODE_CCS_XFERLEN(data_buf_len));	
		iodt->dmaCfg.sgMode = IODT_SGEMODE_DIRECT;
		break;
	default:
		iodt->dataBaseAddr = cpu_to_le64(data_addr);
		iodt->dataBufLenDWAlign = cpu_to_le32(ENCODE_CCS_XFERLEN(data_buf_len));	
		iodt->dmaCfg.sgMode = IODT_SGEMODE_SGL;
		break;
	}
	return;
}

static S32 ps3_scsih_sata_hw_req_frame_build(struct ps3_cmd *cmd, U16 sge_count, U64 data_addr)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_scsi_io_attr *cmd_io_attr = &cmd->io_attr;
	IODT_V1_s *iodt = &cmd->req_frame->hwReq.sasReqFrame;
	PS3ReqFrameHead_s *req_head = &cmd->req_frame->hwReq.reqHead;
	U8 req_frame_format = PS3_REQFRAME_FORMAT_SATA;

	memset(iodt, 0, sizeof(IODT_V1_s));

	ps3_scsih_req_frame_head_build(cmd, req_frame_format);

	iodt->protocolType = PROTOCOL_DIRT;
	iodt->frameType = FRAMETYPE_DIRECT;
	iodt->iuSrc = IU_SRC_TUPLE;

	if (cmd->cmd_word.type == PS3_CMDWORD_TYPE_READ) {
		iodt->commonWord.type = PS3_CMDWORD_TYPE_READ;
		iodt->C.opCode = PS3_ATA_OPC_READ_FPDMA;
	} else {
		iodt->commonWord.type = PS3_CMDWORD_TYPE_WRITE;
		iodt->C.opCode = PS3_ATA_OPC_WRITE_FPDMA;
	}

	iodt->cmdDir = ps3_scsih_data_direction_build(cmd);
	iodt->sataCtl = 0;
	iodt->sasCtl = 1;
	iodt->commonWord.direct = DIRECT_FLAG_DIRECT;
	iodt->commonWord.reqFrameID = req_head->cmdFrameID;
	iodt->commonWord.function = ps3_get_pci_function(cmd->instance->pdev);
	iodt->commonWord.phyDiskID = cmd->cmd_word.phyDiskID;

	iodt->C.lba = cmd_io_attr->plba;

	ps3_scsih_sata_iodt_data_build(cmd, sge_count, data_addr);

	return ret;
}

static S32 ps3_scsih_sata_req_frame_build(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	U64 lba = 0;
	U64 data_addr = 0;
	U16 sge_count = 0;

	if (ps3_is_need_build_hw_req_frame(cmd)) {
		sge_count = ps3_scsih_sgl_build(cmd, cmd->req_frame->hwReq.sgl,
			PS3_FRAME_REQ_SGE_NUM_HW);
		data_addr = ps3_scsih_direct_sgl_base_addr(cmd, sge_count);
		ret = ps3_scsih_sata_hw_req_frame_build(cmd, sge_count, data_addr);
		if (likely(ret == PS3_SUCCESS)) {
			cmd->req_frame->hwReq.reqHead.traceID = cmd->trace_id;
			lba = PS3_LBA(cmd->io_attr.lba_hi, cmd->io_attr.lba_lo);
			ret = ps3_software_zone_build(cmd, lba,
				cmd->io_attr.pd_entry->dev_type, sge_count);
		}
	} else {
		ret = ps3_scsih_pd_frontend_req_build(cmd);
	}

	return ret;
}

static inline void ps3_scsih_sas_iodt_data_build(struct ps3_cmd *cmd,
	U64 data_addr, U32 sge_count)
{
	IODT_V1_s *iodt = &cmd->req_frame->hwReq.sasReqFrame;
	switch(sge_count) {
	case PS3_SGL_MODE_SGE_COUNT_NO_DATA:
		iodt->dataBufLenDWAlign = 0;
		iodt->dataBaseAddr = 0;
		iodt->dmaCfg.sgMode = IODT_SGEMODE_DIRECT;
		break;

	case PS3_SGL_MODE_SGE_COUNT_DIRECT:
		iodt->dataBufLenDWAlign = cpu_to_le32(ENCODE_CCS_XFERLEN(cmd->io_attr.sgl_buf_len));
		iodt->dataBaseAddr = cpu_to_le64(data_addr);
		iodt->dmaCfg.sgMode = IODT_SGEMODE_DIRECT;
		break;

	default:
		iodt->dataBufLenDWAlign = cpu_to_le32(ENCODE_CCS_XFERLEN(cmd->io_attr.sgl_buf_len));
		iodt->dataBaseAddr = cpu_to_le64(data_addr);
		iodt->dmaCfg.sgMode = IODT_SGEMODE_SGL;
		break;
	}
	return;
}

static S32 ps3_scsih_sas_hw_req_frame_build(struct ps3_cmd *cmd, U16 disk_id,
	U64 data_addr, U32 sge_count)
{
	IODT_V1_s *iodt = &cmd->req_frame->hwReq.sasReqFrame;
	U8 *cdb = cmd->scmd->cmnd;
	S32 ret = PS3_SUCCESS;
	PS3ReqFrameHead_s *req_head = &cmd->req_frame->hwReq.reqHead;
	U8 req_frame_format = PS3_REQFRAME_FORMAT_SAS;

	if(cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		cdb = cmd->io_attr.cdb;
	}
	memset(iodt, 0, sizeof(IODT_V1_s));

	ps3_scsih_req_frame_head_build(cmd, req_frame_format);

	iodt->protocolType = PROTOCOL_DIRT;
	iodt->frameType = FRAMETYPE_DIRECT;
	iodt->iuSrc = IU_SRC_IODT;

	if(cmd->scmd->cmd_len != CMD_LEN_THR) {
		iodt->cmdLen = CMD_LEN_S;
	} else {
		iodt->cmdLen = CMD_LEN_L;
	}

	memcpy(iodt->B.cdb, cdb, CMD_LEN_THR);

	iodt->cmdDir = ps3_scsih_data_direction_build(cmd);

	iodt->sasCtl = 0;

	if(cmd->cmd_word.type == PS3_CMDWORD_TYPE_READ) {
		iodt->commonWord.type = PS3_CMDWORD_TYPE_READ;
	} else {
		iodt->commonWord.type = PS3_CMDWORD_TYPE_WRITE;
	}

	iodt->commonWord.direct  = DIRECT_FLAG_DIRECT;
	iodt->commonWord.function = ps3_get_pci_function(cmd->instance->pdev);
	iodt->commonWord.phyDiskID = disk_id;
	iodt->commonWord.reqFrameID = req_head->cmdFrameID;
	ps3_scsih_sas_iodt_data_build(cmd, data_addr, sge_count);

	return ret;
}

static S32 ps3_scsih_sas_req_frame_build(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	U16 sge_count = 0;
	U64 data_addr = 0;
	U64 lba = 0;

	if (ps3_is_need_build_hw_req_frame(cmd)) {
		sge_count = ps3_scsih_sgl_build(cmd,
			cmd->req_frame->hwReq.sgl, PS3_FRAME_REQ_SGE_NUM_HW);
		data_addr = ps3_scsih_direct_sgl_base_addr(cmd, sge_count);
		ret = ps3_scsih_sas_hw_req_frame_build(cmd,
			PS3_PDID(&cmd->io_attr.pd_entry->disk_pos),
			data_addr, sge_count);
		if (likely(ret == PS3_SUCCESS)) {
			lba = (U64)cmd->io_attr.lba_hi << PS3_SHIFT_DWORD | cmd->io_attr.lba_lo;
			ret = ps3_software_zone_build(cmd, lba,
				cmd->io_attr.pd_entry->dev_type, sge_count);
		}
	} else {
		ret = ps3_scsih_pd_frontend_req_build(cmd);
	}
	return ret;
}

static void ps3_hw_nvme_req_frame_build(struct ps3_cmd *cmd,
	PS3HwReqFrame_s *hw_reqframe)
{
	PS3NvmeRWCmd_s *rwReqFrame = &hw_reqframe->nvmeReqFrame.rwReqFrame;
	rwReqFrame->numLba = cpu_to_le32(cmd->io_attr.num_blocks - 1);
	rwReqFrame->numLba |= (U32)((U32)cmd->io_attr.cdb_opts.fua << 30);

	rwReqFrame->sLbaHi = cpu_to_le32((U32)(cmd->io_attr.plba >> PS3_SHIFT_DWORD));
	rwReqFrame->sLbaLo = cpu_to_le32((U32)(cmd->io_attr.plba));

	rwReqFrame->cDW0_9.cID = cpu_to_le16(cmd->index);
	rwReqFrame->cDW0_9.psdt = 0;
	rwReqFrame->cDW0_9.nsID = 1;
	if (cmd->io_attr.rw_flag == PS3_SCSI_CMD_TYPE_READ) {
		rwReqFrame->cDW0_9.opcode = 0x02;
	} else if(cmd->io_attr.rw_flag == PS3_SCSI_CMD_TYPE_WRITE) {
		rwReqFrame->cDW0_9.opcode = 0x01;
	} else {
	}

	LOG_DEBUG("hno:%u cid:%d, op:%d, nsid:%d, prp1:0x%llx, "
		"prp2:0x%llx, numLb:%d, LbaHi:0x%x, LbaLo:0x%x, dw13:0x%x, "
		"dw13:0x%x, dw13:0x%x\n",
		PS3_HOST(cmd->instance), rwReqFrame->cDW0_9.cID,
		rwReqFrame->cDW0_9.opcode, rwReqFrame->cDW0_9.nsID,
		rwReqFrame->cDW0_9.dPtr.prp.prp1,
		rwReqFrame->cDW0_9.dPtr.prp.prp2,
		rwReqFrame->numLba, rwReqFrame->sLbaHi, rwReqFrame->sLbaLo,
		rwReqFrame->cDW13, rwReqFrame->cDW14, rwReqFrame->cDW15);

	return ;
}

static void ps3_hw_nvme_ext_fill(struct ps3_cmd *cmd, U16 prp_count)
{
	if (prp_count > PS3_FRAME_REQ_PRP_NUM_HW) {
		cmd->req_frame->hwReq.sgl[PS3_FRAME_REQ_SGE_NUM_HW - 1].length =
			(prp_count - PS3_FRAME_REQ_PRP_NUM_HW) * sizeof(U64);
		cmd->req_frame->hwReq.sgl[PS3_FRAME_REQ_SGE_NUM_HW - 1].addr =
			cmd->ext_buf_phys;
		cmd->req_frame->hwReq.sgl[PS3_FRAME_REQ_SGE_NUM_HW - 1].lastSge = 0;
		cmd->req_frame->hwReq.sgl[PS3_FRAME_REQ_SGE_NUM_HW - 1].ext = 1;
	} else {
		if (cmd->req_frame->hwReq.sgl[PS3_FRAME_REQ_SGE_NUM_HW - 1].ext != 0) {
			cmd->req_frame->hwReq.sgl[PS3_FRAME_REQ_SGE_NUM_HW - 1].ext = 0;
		}
	}
}

static S32 ps3_scsih_nvme_hw_req_frame_build(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	U64 * prp_ptr = NULL;
	PS3HwReqFrame_s *hw_reqframe = NULL;
	U8 pd_dev_type = PS3_DEV_TYPE_COUNT;
	U64 lba = 0;
	U16 prp_count = 0;
	U32 data_len = ps3_scsih_data_buf_len(cmd, cmd->os_sge_map_count);
	static unsigned long j;
	hw_reqframe = &cmd->req_frame->hwReq;
	memset(&hw_reqframe->nvmeReqFrame, 0, sizeof(PS3NvmeReqFrame_u));
	prp_ptr = &hw_reqframe->nvmeReqFrame.rwReqFrame.cDW0_9.dPtr.prp.prp1;

	ps3_scsih_req_frame_head_build(cmd, PS3_REQFRAME_FORMAT_NVME);
	if (cmd->io_attr.pd_entry != NULL) {
		pd_dev_type = cmd->io_attr.pd_entry->dev_type;
	}

	if (ps3_prp_build_check(cmd, data_len)) {
		prp_count = ps3_scsih_prp_build(cmd, prp_ptr, PS3_FRAME_REQ_PRP_NUM_HW, data_len, PS3_FALSE);
		ps3_hw_nvme_ext_fill(cmd, prp_count);
		ps3_hw_nvme_req_frame_build(cmd, hw_reqframe);
		cmd->req_frame->hwReq.reqHead.traceID = cmd->trace_id;
		lba = (U64)cmd->io_attr.lba_hi << PS3_SHIFT_DWORD | cmd->io_attr.lba_lo;
		ret = ps3_software_zone_build(cmd, lba, pd_dev_type, prp_count);
	} else {
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u this io don't support prp!\n",
			cmd->trace_id, PS3_HOST(cmd->instance));
		ret = -PS3_FAILED;
	}
	return ret;
}

static S32 ps3_scsih_nvme_req_frame_build(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	static unsigned long j;
	if (ps3_is_need_build_hw_req_frame(cmd)) {
		ret = ps3_scsih_nvme_hw_req_frame_build(cmd);
		if (ret != PS3_SUCCESS) {
			LOG_WARN_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u direct NVMe NOK, to normal"
				"is_use_frontend_prp:%d\n", cmd->trace_id,
				PS3_HOST(cmd->instance), cmd->io_attr.is_use_frontend_prp);
			cmd->io_attr.direct_flag = PS3_CMDWORD_DIRECT_NORMAL;
			cmd->io_attr.is_use_frontend_prp = PS3_FALSE;
			memset((void *) &cmd->cmd_word, 0, sizeof(struct PS3CmdWord));
			memset((void *)cmd->req_frame, 0, sizeof(union PS3ReqFrame));
			memset(cmd->ext_buf, 0, cmd->instance->cmd_context.ext_buf_size);
			ret = ps3_scsih_pd_cmd_word_build(cmd);
			if (unlikely(ret != PS3_SUCCESS)) {
				LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "tid:0x%llx hno:%u cmd_word build NOK CFID:%d ret:%d\n",
					cmd->trace_id, PS3_HOST(cmd->instance), cmd->index, ret);
				ret = -PS3_FAILED;
				goto l_out;
			}
			ret = ps3_scsih_pd_frontend_req_build(cmd);
		}
	} else {
		ret = ps3_scsih_pd_frontend_req_build(cmd);
	}

l_out:
	return ret;
}

static inline void ps3_scsih_cmd_back_stat(struct ps3_cmd *cmd, U8 reply_flags)
{
	PS3_IOC_DRV2IOC_BACK_INC(cmd->instance, cmd, reply_flags);
	PS3_DEV_IO_OUTSTAND_DEC(cmd->instance, cmd);
	PS3_DEV_IO_BACK_INC(cmd->instance, cmd, reply_flags);
#ifdef _WINDOWS
	PS3_IO_BACK_INC(cmd->instance, cmd->scmd, cmd->index, reply_flags);
#else
	PS3_IO_BACK_INC(cmd->instance, cmd->scmd, reply_flags);
	PS3_DEV_BUSY_DEC(cmd->scmd);
#endif
}

static inline void ps3_scsih_overrun_underrun_verify(struct ps3_cmd *cmd)
{
	if ((ps3_scsih_is_sas_jbod_cmd(cmd))
		|| (ps3_scsih_is_sata_jbod_cmd(cmd)
		&& ps3_scsih_cdb_is_rw_cmd(cmd->scmd->cmnd))) {
		if (cmd->io_attr.sgl_buf_len > scsi_bufflen(cmd->scmd)) {
			scsi_set_resid(cmd->scmd, 0);
			cmd->scmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_SOFT_ERROR);
			LOG_DEBUG("cmd overrun, CFID:%u op:0x%x data_len:%u scsi_buf_len:%u\n",
				cmd->index, cmd->scmd->cmnd[0], cmd->io_attr.sgl_buf_len, scsi_bufflen(cmd->scmd));
		}
	}

	return;
}

static inline void ps3_r1x_read_dec(struct ps3_cmd *cmd, struct ps3_r1x_read_balance_info *rb_info)
{
	if (rb_info != NULL && cmd->r1x_read_pd > 0) {
		if (cmd->r1x_read_pd > PS3_MAX_PD_NUM_ONE_VD) {
			PS3_BUG();
			return;
		}
		ps3_atomic_dec(&rb_info->scsi_outstanding_cmds[cmd->r1x_read_pd]);
	}
}

S32 ps3_scsih_io_done(struct ps3_cmd *cmd, U16 reply_flags)
{
	S32 ret = PS3_SUCCESS;
	struct scsi_cmnd *s_cmd = cmd->scmd;
	struct ps3_scsi_priv_data *data = NULL;
	struct ps3_cmd *err_cmd = cmd;
	struct ps3_cmd *peer_cmd = NULL;

	if (unlikely(s_cmd == NULL)) {
		LOG_ERROR_IN_IRQ(cmd->instance,
			"hno:%u CFID:%d scmd is null\n",
			PS3_HOST(cmd->instance), cmd->index);
		goto l_out;
	}

	if (cmd->r1x_peer_cmd != NULL) {
		LOG_DEBUG("hno:%u CFID:%d one of the r1x write scmd is return, rflag:%d\n",
			PS3_HOST(cmd->instance), cmd->index, reply_flags);
		cmd->r1x_reply_flag = reply_flags;
		cmd->is_r1x_scsi_complete = PS3_TRUE;
		if (!cmd->r1x_peer_cmd->is_r1x_scsi_complete) {
			PS3_IOC_DRV2IOC_BACK_INC(cmd->instance, cmd->r1x_peer_cmd, reply_flags);
			LOG_DEBUG("hno:%u CFID:%d r1x write peer cmd:%d is not return\n",
				PS3_HOST(cmd->instance), cmd->index,
				cmd->r1x_peer_cmd->index);
			goto l_out;
		}

		if (cmd->r1x_peer_cmd->r1x_reply_flag != PS3_REPLY_WORD_FLAG_SUCCESS) {
			err_cmd = cmd->r1x_peer_cmd;
			reply_flags = cmd->r1x_peer_cmd->r1x_reply_flag;
		}

		if (cmd->index >= (U32)cmd->instance->cmd_attr.cur_can_que) {
			cmd = cmd->r1x_peer_cmd;
		}
	}

	data = scsi_device_private_data(s_cmd);
	LOG_DEBUG("tid:0x%llx hno:%u scsi cmd reply cb CFID:%u rep_f:%u"
		" op:0x%x chl:%u id:%u data_len:%u scsi_buflen:%u dev_type:%s\n",
		cmd->trace_id, PS3_HOST(cmd->instance), cmd->index, reply_flags,
		s_cmd->cmnd[0], PS3_SDEV_CHANNEL(cmd->scmd->device),
		PS3_SDEV_TARGET(cmd->scmd->device), cmd->io_attr.sgl_buf_len,
		scsi_bufflen(cmd->scmd),
		namePS3DevType((enum PS3DevType)cmd->io_attr.dev_type));

	ps3_scsih_cmd_back_stat(cmd, reply_flags);
	PS3_IO_TRACE(cmd, PS3_IO_TRACE_DIRECT_RECV);

	if (unlikely(reply_flags != PS3_REPLY_WORD_FLAG_SUCCESS)) {
		PS3_DEV_IO_ERR_STAT_INC(cmd->instance, cmd);
		ret = ps3_err_scsi_cmd_fault_proc(cmd->instance, err_cmd);
	} else {
		ps3_scsih_overrun_underrun_verify(cmd);
		if (cmd->scmd->result == DID_OK) {
			ps3_qos_adjust_pd_rsc(s_cmd->device, cmd->instance, PS3_QOS_QUOTA_ADJUST_UP);
		}
	}

	ps3_r1x_read_dec(cmd, data->r1x_rb_info);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0))
	s_cmd->SCp.ptr = NULL;
#endif
	ps3_scsi_dma_unmap(cmd);

	ps3_qos_cmd_update(cmd->instance, cmd);

	ps3_r1x_write_unlock(&data->lock_mgr, cmd);
	peer_cmd = cmd->r1x_peer_cmd;
	INJECT_START(PS3_ERR_IJ_WAIT_ABORT_FLAG, peer_cmd);
	PS3_IO_OUTSTAND_DEC(cmd->instance, s_cmd);
	PS3_VD_OUTSTAND_DEC(cmd->instance, s_cmd);
	ps3_scsi_cmd_free(cmd);
	if (peer_cmd != NULL) {
		LOG_DEBUG("host_no:%u r1x scsi write done CFID:%d and CFID:%d\n",
			PS3_HOST(peer_cmd->instance), peer_cmd->r1x_peer_cmd->index,
			peer_cmd->index);
		ps3_r1x_peer_cmd_free_nolock(peer_cmd);
	}
	SCMD_IO_DONE(s_cmd);

l_out:
	return ret;
}

void ps3_scsih_direct_to_normal_req_frame_rebuild(
	struct ps3_cmd *cmd)
{
	ps3_vd_adv_to_normal_req_frame_rebuild(cmd);
}

#ifdef _WINDOWS
static Bool ps3_scsi_sge_remap_check(struct ps3_cmd *cmd,
	ps3_scatter_gather_element *os_sgl, S32 index)
{
	Bool ret = PS3_FALSE;
	struct scsi_cmnd *scp = cmd->scmd;
	U16 os_sge_count = (U16)scp->scatterlist->NumberOfElements;

	if (index != 0 && index != os_sge_count-1) {
		goto l_check;
	}

	if ( cmd->io_attr.dev_type == PS3_DEV_TYPE_VD ) {
		if (cmd->io_attr.vd_entry->isNvme) {
			goto l_nvme_check;
		}
	} else if (cmd->io_attr.dev_type == PS3_DEV_TYPE_NVME_SSD) {
		goto l_nvme_check;
	} else {
	}

	goto l_check;
l_nvme_check:
	if (index == 0) {
		if (os_sgl->PhysicalAddress.QuadPart & PS3_SCSI_ALINNMENT_MASK) {
			ret = PS3_TRUE;
		}
	} else {
		if (os_sgl->PhysicalAddress.QuadPart &
			scp->dma_addr_alignment_mask) {
			ret = PS3_TRUE;
		}
	}

	goto l_out;
l_check:
	if ((os_sgl->PhysicalAddress.QuadPart & scp->dma_addr_alignment_mask)
		|| (os_sgl->Length & scp->dma_len_alignment_mask)) {
		ret = PS3_TRUE;
	}
l_out:
	return ret;
}

static void ps3_scsi_dma_algin_calc(struct ps3_cmd *cmd)
{
	U32 dma_addr_alignment = 0;
	U32 dma_len_alignment = 0;
	struct scsi_cmnd *scp = cmd->scmd;

	if ((cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) &&
		(ps3_scsih_is_use_hard_cmd(cmd) != PS3_CMDWORD_FORMAT_HARDWARE)) {
		dma_addr_alignment = cmd->io_attr.vd_entry->dmaAddrAlignShift;
		dma_len_alignment = cmd->io_attr.vd_entry->dmaLenAlignShift;
		dma_addr_alignment = dma_addr_alignment ? 1 << dma_addr_alignment : 0;
		dma_len_alignment  = dma_len_alignment ? 1 << dma_len_alignment : 0;
	} else {
		dma_addr_alignment = cmd->io_attr.pd_entry->dma_addr_alignment;
		dma_len_alignment = cmd->io_attr.pd_entry->dma_len_alignment;
	}

	scp->dma_addr_alignment_mask = PS3_SCSI_ALINNMENT_MASK;
	scp->dma_len_alignment_mask = 0;
	if (dma_addr_alignment) {
		scp->dma_addr_alignment_mask = dma_addr_alignment - 1;
	}

	if(dma_len_alignment){
		scp->dma_len_alignment_mask = dma_len_alignment - 1;
	}

	if (( (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) &&
		(cmd->io_attr.vd_entry->isNvme)) ||
		(cmd->io_attr.dev_type == PS3_DEV_TYPE_NVME_SSD) ) {
		scp->dma_addr_alignment_mask = PS3_SCSI_4K_ALINNMENT_MASK;
		scp->dma_len_alignment_mask = PS3_SCSI_4K_ALINNMENT_MASK;
	}

	LOG_DEBUG("tid:0x%llx hno:%u align[addr:%d,len:%d][addr_mask:%x,len_mask:%x]\n",
		cmd->trace_id, PS3_HOST(cmd->instance),
		dma_addr_alignment,
		dma_len_alignment,
		scp->dma_addr_alignment_mask,
		scp->dma_len_alignment_mask);
}

static void ps3_scsi_need_remap_check(struct ps3_cmd *cmd)
{
	S32 index = 0;
	U16 os_sge_count = 0;
	struct scsi_cmnd *scp = cmd->scmd;
	ps3_scatter_gather_element *os_sgl = NULL;
	scp->is_remap_databuff = PS3_FALSE;

	if (scp->scatterlist == NULL) {
		goto l_out;
	}

	if (!ps3_scsih_cdb_is_rw_cmd(scp->cmnd) &&
		(cmd->io_attr.dev_type == PS3_DEV_TYPE_NVME_SSD)) {
		goto l_out;
	}

	ps3_scsi_dma_algin_calc(cmd);

	os_sge_count = (U16)scp->scatterlist->NumberOfElements;
	scsi_for_each_sg(scp, os_sgl, os_sge_count, index) {
		if (os_sgl == NULL) {
			break;
		}

		if (ps3_scsi_sge_remap_check(cmd, os_sgl, index)) {
			scp->is_remap_databuff = PS3_TRUE;
			break;
		}
	}
l_out:
	if(scp->is_remap_databuff) {
        LOG_DEBUG("tid:0x%llx hno:%u unalign[addr_mask:%x,len_mask:%x] "
            "sgl addr:0x%llx len:%d or SAS sge num:%d > %d, index:%d\n",
			cmd->trace_id, PS3_HOST(cmd->instance),
			scp->dma_addr_alignment_mask,
			scp->dma_len_alignment_mask,
			os_sgl->PhysicalAddress.QuadPart,
			os_sgl->Length,
			scp->scatterlist->NumberOfElements,
			PS3_FRAME_REQ_SGE_NUM_HW,
			index);
	}
	return;
}

static S32 ps3_scsi_remap_sgl(struct ps3_cmd *cmd)
{
	S32 ret = -PS3_FAILED;
	struct scsi_cmnd *scp = cmd->scmd;
	void *srb_data_buff = SrbGetDataBuffer(cmd->srb);

	if (srb_data_buff == NULL) {
		goto l_out;
	}

	scp->remap_databuff_length = scp->data_transfer_length;
	if (scp->data_transfer_length & scp->dma_len_alignment_mask) {
		scp->remap_databuff_length =
			(scp->data_transfer_length + scp->dma_len_alignment_mask + 1) &
			(~(U64)scp->dma_len_alignment_mask);
	}

	scp->remap_databuff = ps3_dma_alloc_coherent(cmd->instance,
		scp->remap_databuff_length,
		&scp->remap_databuff_phy);
	if (scp->remap_databuff == NULL) {
		goto l_out;
	}

	cmd->os_sge_map_count = 1;
	memset(&scp->remap_sgl, 0, sizeof(scp->remap_sgl));
	scp->scatterlist = (STOR_SCATTER_GATHER_LIST*)&scp->remap_sgl;
	scp->scatterlist->NumberOfElements = 1;
	scp->scatterlist->List[0].Length = scp->remap_databuff_length;
	scp->scatterlist->List[0].PhysicalAddress.QuadPart = (LONGLONG)scp->remap_databuff_phy;
	RtlMoveMemory(scp->remap_databuff,
		srb_data_buff,
		scp->data_transfer_length);
	LOG_DEBUG("remap io,%p, data_len:%d, remap_len:%d, addr:0x%llx\n",
		srb_data_buff,
		scp->data_transfer_length,
		scp->remap_databuff_length,
		scp->remap_databuff_phy);

	ret = PS3_SUCCESS;
l_out:
	return ret;
}

static S32 ps3_scsi_unremap_sgl(struct ps3_cmd *cmd)
{
	S32 ret = -PS3_FAILED;
	struct scsi_cmnd *scp = cmd->scmd;
	void *srb_data_buff = SrbGetDataBuffer(cmd->srb);

	if (srb_data_buff == NULL) {
		goto l_out;
	}

	if (scp->remap_databuff != NULL) {
		RtlMoveMemory(srb_data_buff,
			scp->remap_databuff,
			scp->data_transfer_length);

		LOG_DEBUG("unremap io,%p, len:%d \n",
			srb_data_buff,
			scp->data_transfer_length);

		ps3_dma_free_coherent(cmd->instance,
			scp->remap_databuff_length,
			scp->remap_databuff,
			scp->remap_databuff_phy );
		scp->remap_databuff = NULL;
		scp->is_remap_databuff = PS3_FALSE;
		cmd->os_sge_map_count = 0;
		ret = PS3_SUCCESS;
	}

l_out:
	return ret;
}
#endif

S32 ps3_scsi_dma_map(struct ps3_cmd *cmd)
{
	S32 index = 0;
	S32 os_sge_count = 0;
	struct scsi_cmnd *scp = cmd->scmd;
	ps3_scatter_gather_element *os_sgl = NULL;

	os_sge_count = scsi_dma_map(scp);
	if (unlikely(os_sge_count <= 0)) {
		cmd->os_sge_map_count = 0;
		goto l_out;
	}

	scsi_for_each_sg(scp, os_sgl, os_sge_count, index) {
		if (os_sgl == NULL) {
			break;
		}
		INJECT_START(PS3_ERR_IJ_SGL_ADDR_PAGE_MODE_5, &(sg_dma_address_u64(os_sgl)));
		if (index == 0 && cmd->instance->page_mode_change
			&& (sg_dma_address_u64(os_sgl) & cmd->instance->page_mode_addr_mask)) {
			cmd->instance->page_mode_addr_mask = PS3_PAGE_MODE_ABOVE_4_ADDR_MASK;
			if (sg_dma_address_u64(os_sgl) & PS3_PAGE_MODE_ABOVE_4_ADDR_MASK) {
				cmd->instance->page_mode_change = PS3_FALSE;
			} 
			dev_info(&cmd->instance->pdev->dev,
				"page mode change, addr:0x%llx\n",
				sg_dma_address_u64(os_sgl));
		}
		INJECT_START(PS3_ERR_IJ_SGL_ADDR_RESTORE, &(sg_dma_address_u64(os_sgl)));
		sg_dma_address(os_sgl) =
			PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(cmd->instance->dma_addr_bit_pos, sg_dma_address_u64(os_sgl));
	}

	cmd->os_sge_map_count = os_sge_count;
l_out:
	return os_sge_count;
}

void ps3_scsi_dma_unmap(struct ps3_cmd *cmd)
{
	S32 index = 0;
	U16 os_sge_count = 0;
	struct scsi_cmnd *scp = NULL;
	ps3_scatter_gather_element *os_sgl = NULL;

	if (cmd == NULL) {
		goto l_out;
	}

	os_sge_count = cmd->os_sge_map_count;
	scp = cmd->scmd;
	if (os_sge_count == 0) {
		goto l_out;
	}
	scsi_for_each_sg(scp, os_sgl, os_sge_count, index) {
		if (os_sgl == NULL) {
			break;
		}
		sg_dma_address(os_sgl) =
			PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(cmd->instance->dma_addr_bit_pos, sg_dma_address_u64(os_sgl));
	}
	if (scp->sdb.table.sgl) {
		scsi_dma_unmap(scp);
	}
	cmd->os_sge_map_count = 0;

l_out:
	return;
}

#if 0
struct disk_type_to_proc_func_table* ps3_req_func_entry_query(U8 dev_type)
{
	struct disk_type_to_proc_func_table *entry = NULL;
	U32 table_size = ARRAY_SIZE(g_req_frame_func_table);
	U32 i = 0;

	for (; i < table_size; ++i) {
		if (dev_type == g_req_frame_func_table[i].type) {
			entry = &g_req_frame_func_table[i];
			goto l_out;
		}
	}
l_out:
	return entry;
}
#endif

static S32 ps3_vd_access_policy_check(struct ps3_instance *instance,
	U8 channel, U16 id, struct scsi_cmnd *s_cmd)
{
	S32 ret = PS3_SUCCESS;
	U8 opcode = 0;
	U16 sub_opcode = 0;
	struct PS3VDEntry *entry = ps3_dev_mgr_lookup_vd_info(instance, channel, id);
	static unsigned long j;
	if (unlikely(entry == NULL)) {
#ifndef _WINDOWS
		s_cmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_BAD_TARGET);
#else
		scsi_cmnd_hoststatus_set(s_cmd, DID_BAD_TARGET);
#endif
		LOG_ERROR_TIME_LIM(&j, PS3_LOG_LIMIT_INTERVAL_MSEC, "hno:%u chl:%u id:%u\n",
			PS3_HOST(instance), channel, id);
		ret = -PS3_ENODEV;
		goto l_out;
	}

	if (unlikely((entry->accessPolicy == VD_ACCESS_POLICY_BLOCK) &&
		ps3_scsih_cdb_is_rw_cmd(s_cmd->cmnd))) {
		ps3_errcode_to_scsi_status(instance, s_cmd, PS3_STATUS_ACCESS_BLOCK, NULL, 0, NULL);
		ps3_scsih_cdb_opcode_get(s_cmd->cmnd, &opcode, &sub_opcode);

		LOG_DEBUG("hno:%u chl:%u id:%u vd_access_policy:%u"
			"op:0x%x sub_op:0x%x block io\n",
			PS3_HOST(instance), channel, id, entry->accessPolicy,
			opcode, sub_opcode);
		ret = -PS3_IO_BLOCK;
		goto l_out;
	} else if (unlikely((entry->accessPolicy == VD_ACCESS_POLICY_READ_ONLY) &&
		ps3_scsih_cdb_is_write_cmd(s_cmd->cmnd))) {
		ps3_errcode_to_scsi_status(instance, s_cmd, PS3_STATUS_ACCESS_BLOCK, NULL, 0, NULL);
		ps3_scsih_cdb_opcode_get(s_cmd->cmnd, &opcode, &sub_opcode);

		LOG_DEBUG("hno:%u chl:%u id:%u vd_access_policy:%u"
			"op:0x%x sub_op:0x%x block write io\n",
			PS3_HOST(instance), channel, id, entry->accessPolicy,
			opcode, sub_opcode);
		ret = -PS3_IO_BLOCK;
		goto l_out;
	}
l_out:
	return ret;
}

S32 ps3_get_requeue_or_reset(void)
{
#if ((defined(RHEL_MAJOR) && (RHEL_MAJOR == 7) && (RHEL_MINOR == 3)) || \
((LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)) && \
(LINUX_VERSION_CODE < KERNEL_VERSION(4,4,14))))
	return PS3_SCSI_RESULT_HOST_STATUS(DID_REQUEUE);
#else
	return PS3_SCSI_RESULT_HOST_STATUS(DID_RESET);
#endif
}

Bool ps3_write_direct_enable(struct ps3_cmd *cmd)
{
	Bool result = PS3_TRUE;
	if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag) &&
			!cmd->io_attr.vd_entry->isWriteDirectEnable) {
		result = PS3_FALSE;
	}

	return result;
}
