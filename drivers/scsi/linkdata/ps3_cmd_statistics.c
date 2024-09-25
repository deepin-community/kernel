
#ifndef _WINDOWS
#include <linux/ktime.h>
#endif

#include "ps3_cmd_statistics.h"
#include "ps3_cmd_channel.h"
#include "ps3_driver_log.h"

static S32 ps3_cmd_stat_buf_alloc(struct ps3_instance *instance);
static void ps3_cmd_stat_buf_free(struct ps3_instance *instance);
static inline void ps3_stat_data_collect(struct ps3_instance *instance);
static void ps3_cmd_stat_content_init(struct ps3_instance *instance);
static inline void ps3_stat_buf_init(struct ps3_instance *instance);
static void ps3_dev_io_recv_bytes_stat_inc(struct ps3_instance *ins,
	const struct ps3_cmd *cmd);
static S32 ps3_cmd_stat_backup_buf_alloc(struct ps3_instance *instance);
static S32 ps3_last_stat_buf_alloc(struct ps3_instance *instance);
static void ps3_cmd_stat_backup_buf_free(struct ps3_instance *instance);
static void ps3_last_stat_buf_free(struct ps3_instance *instance);
static inline void ps3_cmd_stat_backup_buf_init(struct ps3_instance *instance);
static inline void ps3_last_stat_init(struct ps3_instance *instance);

S32 ps3_cmd_statistics_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	ps3_atomic_set(&instance->cmd_statistics.cmd_outstanding,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic_set(&instance->cmd_statistics.io_outstanding,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic_set(&instance->cmd_statistics.vd_io_outstanding,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic_set(&instance->cmd_statistics.cmd_delivering,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic_set(&instance->cmd_statistics.scsi_cmd_delivering,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&instance->cmd_statistics.cmd_word_send_count,
		PS3_CMD_STAT_INIT_VALUE);
    ps3_atomic64_set(&instance->cmd_statistics.cmd_qos_total,
        PS3_CMD_STAT_INIT_VALUE);
    ps3_atomic_set(&instance->cmd_statistics.cmd_qos_processing,
        PS3_CMD_STAT_INIT_VALUE);

	ret = ps3_cmd_stat_buf_alloc(instance);
	if (ret != PS3_SUCCESS) {
		goto l_free_stat_buf;
	}

	ret = ps3_cmd_stat_backup_buf_alloc(instance);
	if (ret != PS3_SUCCESS) {
		goto l_free_stat_buf;
	}

	ret = ps3_last_stat_buf_alloc(instance);
	if (ret != PS3_SUCCESS) {
		goto l_free_stat_buf;
	}

	ps3_cmd_stat_content_init(instance);

	ret = ps3_stat_workq_start(instance);
	if (ret != PS3_SUCCESS) {
		goto l_stop_workq;
	}

	goto l_out;

l_stop_workq:
	ps3_stat_workq_stop(instance);

l_free_stat_buf:
	ps3_cmd_stat_buf_free(instance);
	ps3_cmd_stat_backup_buf_free(instance);
	ps3_last_stat_buf_free(instance);
l_out:
	return ret;
}

void ps3_cmd_statistics_exit(struct ps3_instance *instance)
{
	ps3_stat_workq_stop(instance);
	ps3_cmd_stat_buf_free(instance);
	ps3_cmd_stat_backup_buf_free(instance);
	ps3_last_stat_buf_free(instance);
}

void ps3_cmd_stat_content_clear(struct ps3_instance *instance)
{
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;
	memset(&ctx->total_stat, 0, sizeof(struct ps3_total_cmd_stat));
	memset(&ctx->inc_stat, 0, sizeof(struct ps3_total_cmd_stat));
	ps3_stat_buf_init(instance);
	ps3_cmd_stat_backup_buf_init(instance);
	ps3_last_stat_init(instance);
}

static void ps3_cmd_stat_content_init(struct ps3_instance *instance)
{
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;

	ps3_cmd_stat_content_clear(instance);
	ctx->stat_interval = PS3_STAT_WORKQ_INTERVAL_DEFAULT;
	ctx->log_record_count = PS3_STAT_LOG_COUNT;
#ifdef PS3_CFG_RELEASE
	ctx->cmd_stat_switch = PS3_STAT_OUTSTAND_SWITCH_OPEN;
#else
	ctx->cmd_stat_switch = PS3_STAT_OUTSTAND_SWITCH_OPEN | PS3_STAT_INC_SWITCH_OPEN
                | PS3_STAT_QOS_SWITCH_OPEN;
#endif
}

static inline Bool ps3_cmd_is_vd(const struct ps3_cmd *cmd)
{
	return cmd->io_attr.dev_type == PS3_DEV_TYPE_VD;
}

void ps3_dev_io_statis_init(struct ps3_dev_io_statis *statis)
{
	ps3_atomic64_set(&statis->read_send_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->read_send_ok_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->read_send_wait_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->read_send_err_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->read_recv_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->read_recv_ok_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->read_recv_err_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->read_ok_bytes,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->write_send_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->write_send_ok_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->write_send_wait_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->write_send_err_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->write_recv_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->write_recv_ok_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->write_recv_err_cnt,
		PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic64_set(&statis->write_ok_bytes,
		PS3_CMD_STAT_INIT_VALUE);
    ps3_atomic64_set(&statis->qos_processing_cnt,
        PS3_CMD_STAT_INIT_VALUE);
}

void ps3_io_statis_inc(struct scsi_device *sdev, enum ps3_dev_io_stat_type type)
{
	struct ps3_scsi_priv_data *data = NULL;
	data = PS3_SDEV_PRI_DATA(sdev);

	switch(type) {
	case PS3_DEV_IO_STAT_TYPE_R_SEND:
		ps3_atomic64_inc(&data->statis.read_send_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_R_SEND_OK:
		ps3_atomic64_inc(&data->statis.read_send_ok_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_R_SEND_WAIT:
		ps3_atomic64_inc(&data->statis.read_send_wait_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_R_SEND_ERR:
		ps3_atomic64_inc(&data->statis.read_send_err_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_R_RECV:
		ps3_atomic64_inc(&data->statis.read_recv_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_R_RECV_OK:
		ps3_atomic64_inc(&data->statis.read_recv_ok_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_R_RECV_ERR:
		ps3_atomic64_inc(&data->statis.read_recv_err_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_W_SEND:
		ps3_atomic64_inc(&data->statis.write_send_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_W_SEND_OK:
		ps3_atomic64_inc(&data->statis.write_send_ok_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_W_SEND_WAIT:
		ps3_atomic64_inc(&data->statis.write_send_wait_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_W_SEND_ERR:
		ps3_atomic64_inc(&data->statis.write_send_err_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_W_RECV:
		ps3_atomic64_inc(&data->statis.write_recv_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_W_RECV_OK:
		ps3_atomic64_inc(&data->statis.write_recv_ok_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_W_RECV_ERR:
		ps3_atomic64_inc(&data->statis.write_recv_err_cnt);
		break;
	default:
		LOG_INFO("unknown io statis type %d", type);
		break;
	}
}

static inline void ps3_io_statis_add(struct scsi_device *sdev,
	enum ps3_dev_io_stat_type type, U64 offset)
{
	struct ps3_scsi_priv_data *data = NULL;
	data = PS3_SDEV_PRI_DATA(sdev);

	switch(type) {
	case PS3_DEV_IO_STAT_TYPE_R_OK_BYTES:
		ps3_atomic64_add(offset, &data->statis.read_ok_bytes);
		break;
	case PS3_DEV_IO_STAT_TYPE_W_OK_BYTES:
		ps3_atomic64_add(offset, &data->statis.write_ok_bytes);
		break;
	default:
		LOG_INFO("unknown io statis type %d", type);
		break;
	}
}

void ps3_io_statis_dec(struct scsi_device *sdev,
	enum ps3_dev_io_stat_type type)
{
	struct ps3_scsi_priv_data *data = NULL;
	data = PS3_SDEV_PRI_DATA(sdev);

	switch(type) {
	case PS3_DEV_IO_STAT_TYPE_R_SEND_WAIT:
		ps3_atomic64_dec(&data->statis.read_send_wait_cnt);
		break;
	case PS3_DEV_IO_STAT_TYPE_W_SEND_WAIT:
		ps3_atomic64_dec(&data->statis.write_send_wait_cnt);
		break;
	default:
		LOG_INFO("io statis type %d not support to dec", type);
		break;
	}
}

void ps3_io_statis_clear(struct scsi_device *sdev)
{
	struct ps3_scsi_priv_data *data = NULL;
	struct ps3_instance *instance =
		(struct ps3_instance*)sdev->host->hostdata;

	ps3_mutex_lock(&instance->dev_context.dev_priv_lock);
	data = PS3_SDEV_PRI_DATA(sdev);
	if (data != NULL) {
		ps3_dev_io_statis_init(&data->statis);
	}
	ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
}

void ps3_dev_io_start_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd)
{
	if (instance == NULL || cmd == NULL) {
		goto l_out;
	}

	if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_R_SEND);
	} else if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag)) {
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_W_SEND);
	}

l_out:
	return;
}

void ps3_dev_io_start_err_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd)
{
	if (instance == NULL || cmd == NULL) {
		goto l_out;
	}

	if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_R_SEND_ERR);
	} else if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag)) {
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_W_SEND_ERR);
	}

l_out:
	return;
}

void ps3_dev_io_start_ok_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd)
{
	if (instance == NULL || cmd == NULL) {
		goto l_out;
	}

	if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_R_SEND_OK);
	} else if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag)) {
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_W_SEND_OK);
	}

l_out:
	return;
}

void ps3_dev_io_outstand_dec(const struct ps3_cmd *cmd)
{
	if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
		ps3_io_statis_dec(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_R_SEND_WAIT);
	} else if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag)) {
		ps3_io_statis_dec(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_W_SEND_WAIT);
	}
}

void ps3_dev_io_qos_inc(struct ps3_scsi_priv_data* priv_data)
{
    ps3_atomic64_inc(&priv_data->statis.qos_processing_cnt);
}

void ps3_dev_io_qos_dec(struct ps3_scsi_priv_data* priv_data)
{
    ps3_atomic64_dec(&priv_data->statis.qos_processing_cnt);
}

void ps3_io_outstand_dec(struct ps3_instance *instance,
	const struct scsi_cmnd *s_cmd)
{
	if (instance == NULL || s_cmd == NULL) {
		goto l_out;
	}

	ps3_atomic_dec(&instance->cmd_statistics.io_outstanding);

	if((instance->cmd_attr.vd_io_threshold != 0) && ps3_is_vd_rw_cmd(s_cmd)) {
		ps3_atomic_dec(&instance->cmd_statistics.vd_io_outstanding);
	}

l_out:
	return;
}

void ps3_vd_outstand_dec(struct ps3_instance *instance, const struct scsi_cmnd *s_cmd)
{
	struct ps3_scsi_priv_data *data = PS3_SDEV_PRI_DATA(s_cmd->device);
	struct PS3VDEntry *vd_entry = NULL;
	
	if (data->dev_type == PS3_DEV_TYPE_VD) {
		vd_entry = ps3_dev_mgr_lookup_vd_info(instance, s_cmd->device->channel,\
			s_cmd->device->id);
		if (unlikely(vd_entry != NULL)) {
			if (ps3_is_read_cmd(s_cmd)) {
				atomic_dec(&data->rd_io_outstand);
			} else if (ps3_is_write_cmd(s_cmd)) {
				if (!vd_entry->isNvme && !vd_entry->isSsd) {
					atomic_dec(&data->wr_io_outstand);
				}
			}
		}
	}
}

void ps3_dev_io_outstand_inc(const struct ps3_cmd *cmd)
{
	if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_R_SEND_WAIT);
	} else if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag)) {
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_W_SEND_WAIT);
	}
}

void ps3_vd_outstand_inc(struct ps3_instance *instance, const struct scsi_cmnd *s_cmd)
{
	struct ps3_scsi_priv_data *data = PS3_SDEV_PRI_DATA(s_cmd->device);
	struct PS3VDEntry *vd_entry = NULL;
	
	if (data->dev_type == PS3_DEV_TYPE_VD) {
		vd_entry = ps3_dev_mgr_lookup_vd_info(instance, s_cmd->device->channel,\
			s_cmd->device->id);
		if (unlikely(vd_entry != NULL)) {
			if (ps3_is_read_cmd(s_cmd)) {
				atomic_inc(&data->rd_io_outstand);
			} else if (ps3_is_write_cmd(s_cmd)) {
				if (!vd_entry->isNvme && !vd_entry->isSsd) {
					atomic_inc(&data->wr_io_outstand);
				}
			}
		}
	}
}

void ps3_io_outstand_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd)
{
	if (ps3_stat_outstand_switch_is_open(instance)) {
		ps3_atomic_inc(&instance->cmd_statistics.io_outstanding);

		if (ps3_is_vd_rw_cmd(cmd->scmd) &&
			(instance->cmd_attr.vd_io_threshold != 0)) {
			ps3_atomic_inc(&instance->cmd_statistics.vd_io_outstanding);
		}
	}

	return;
}

void ps3_io_recv_ok_stat_inc(struct ps3_instance *ins,
	const struct ps3_cmd *cmd)
{
	if (ins == NULL || cmd == NULL) {
		goto l_out;
	}

	if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)){
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_R_RECV_OK);
	} else if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag)){
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_W_RECV_OK);
	}

l_out:
	return;
}

void ps3_dev_io_back_inc(struct ps3_instance *ins,
	const struct ps3_cmd *cmd, U8 status)
{
	if (ins == NULL || cmd == NULL) {
		goto l_out;
	}

	if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)){
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_R_RECV);
	} else if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag)){
		ps3_io_statis_inc(cmd->scmd->device, PS3_DEV_IO_STAT_TYPE_W_RECV);
	}

	if (status == PS3_REPLY_WORD_FLAG_SUCCESS) {
		ps3_io_recv_ok_stat_inc(ins, cmd);
		ps3_dev_io_recv_bytes_stat_inc(ins, cmd);
	}

l_out:
	return;
}

static void ps3_dev_io_recv_bytes_stat_inc(struct ps3_instance *ins,
	const struct ps3_cmd *cmd)
{
	if (ins == NULL || cmd == NULL || cmd->scmd == NULL) {
		goto l_out;
	}

	if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)){
		ps3_io_statis_add(cmd->scmd->device,
			PS3_DEV_IO_STAT_TYPE_R_OK_BYTES,
			scsi_bufflen(cmd->scmd));
	} else if (ps3_scsih_is_write_cmd(cmd->io_attr.rw_flag)){
		ps3_io_statis_add(cmd->scmd->device,
			PS3_DEV_IO_STAT_TYPE_W_OK_BYTES,
			scsi_bufflen(cmd->scmd));
	}

l_out:
	return;
}

void ps3_qos_cmd_inc(struct ps3_instance *instance)
{
    ps3_atomic64_inc(&instance->cmd_statistics.cmd_qos_total);
}
static void ps3_cmd_stat_buf_free(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;

	if (ctx->cmd_stat_buf == NULL) {
		goto l_out;
	}

	for (; i < ctx->stat_entry_max_count; ++i) {
		if (ctx->cmd_stat_buf[i] == NULL) {
			continue;
		}
		ps3_kfree(instance, ctx->cmd_stat_buf[i]);
		ctx->cmd_stat_buf[i] = NULL;

	}

	ps3_kfree(instance, ctx->cmd_stat_buf);
	ctx->cmd_stat_buf = NULL;
l_out:
	return;
}

static S32 ps3_cmd_stat_buf_alloc(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 i = 0;
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;

	ctx->stat_entry_max_count = instance->cmd_context.max_cmd_count;
	ctx->cmd_stat_buf = (struct ps3_single_cmd_stat**)ps3_kzalloc(instance,
		ctx->stat_entry_max_count * sizeof(struct ps3_single_cmd_stat*));
	if (ctx->cmd_stat_buf == NULL) {
		LOG_ERROR("failed to kcalloc for cmd_stat_buf\n");
		ret = -PS3_FAILED;
		goto l_out;
	}

	for (; i < ctx->stat_entry_max_count; ++i) {
		ctx->cmd_stat_buf[i] = (struct ps3_single_cmd_stat*)ps3_kzalloc(instance,
			sizeof(struct ps3_single_cmd_stat));
		if (ctx->cmd_stat_buf[i] == NULL) {
			LOG_ERROR("Failed to alloc mem for ps3_single_cmd_stat\n");
			ret = -PS3_FAILED;
			goto l_free_mem;
		}
	}

	goto l_out;

l_free_mem:
	ps3_cmd_stat_buf_free(instance);
l_out:
	return ret;
}

static void ps3_last_stat_buf_free(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;

	if (ctx->last_stat_buf == NULL) {
		goto l_out;
	}

	for (; i < ctx->stat_entry_max_count; ++i) {
		if (ctx->last_stat_buf[i] == NULL) {
			continue;
		}
		ps3_kfree(instance, ctx->last_stat_buf[i]);
		ctx->last_stat_buf[i] = NULL;

	}

	ps3_kfree(instance, ctx->last_stat_buf);
	ctx->last_stat_buf = NULL;
l_out:
	return;
}

static S32 ps3_last_stat_buf_alloc(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 i = 0;
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;

	ctx->stat_entry_max_count = instance->cmd_context.max_cmd_count;
	ctx->last_stat_buf = (struct ps3_single_cmd_stat**)ps3_kzalloc(instance,
		ctx->stat_entry_max_count * sizeof(struct ps3_single_cmd_stat*));
	if (ctx->last_stat_buf == NULL) {
		LOG_ERROR("failed to kzalloc for last_stat_buf\n");
		ret = -PS3_FAILED;
		goto l_out;
	}

	for (; i < ctx->stat_entry_max_count; ++i) {
		ctx->last_stat_buf[i] = (struct ps3_single_cmd_stat*)ps3_kzalloc(instance,
			sizeof(struct ps3_single_cmd_stat));
		if (ctx->last_stat_buf[i] == NULL) {
			LOG_ERROR("Failed to alloc mem for ps3_single_cmd_stat\n");
			ret = -PS3_FAILED;
			goto l_free_mem;
		}
	}

	goto l_out;

l_free_mem:
	ps3_last_stat_buf_free(instance);
l_out:
	return ret;
}

static inline struct ps3_single_cmd_stat* ps3_last_stat_entry_find(U32 index,
	struct ps3_instance *instance)
{
	return (index < instance->cmd_statistics.stat_entry_max_count) ?
		instance->cmd_statistics.last_stat_buf[index] : NULL;
}

static inline void ps3_last_stat_init(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;

	for (; i < ctx->stat_entry_max_count; ++i) {
		memset(ctx->last_stat_buf[i], 0, sizeof(struct ps3_single_cmd_stat));
		mb();
	}

	return;
}
static void ps3_cmd_stat_backup_buf_free(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;

	if (ctx->cmd_stat_backup_buf == NULL) {
		goto l_out;
	}

	for (; i < ctx->stat_entry_max_count; ++i) {
		if (ctx->cmd_stat_backup_buf[i] == NULL) {
			continue;
		}
		ps3_kfree(instance, ctx->cmd_stat_backup_buf[i]);
		ctx->cmd_stat_backup_buf[i] = NULL;

	}

	ps3_kfree(instance, ctx->cmd_stat_backup_buf);
	ctx->cmd_stat_backup_buf = NULL;
l_out:
	return;
}

static S32 ps3_cmd_stat_backup_buf_alloc(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 i = 0;
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;

	ctx->stat_entry_max_count = instance->cmd_context.max_cmd_count;
	ctx->cmd_stat_backup_buf = (struct ps3_single_cmd_stat**)ps3_kzalloc(instance,
		ctx->stat_entry_max_count * sizeof(struct ps3_single_cmd_stat*));
	if (ctx->cmd_stat_backup_buf == NULL) {
		LOG_ERROR("failed to kzalloc for cmd_stat_backup_buf\n");
		ret = -PS3_FAILED;
		goto l_out;
	}

	for (; i < ctx->stat_entry_max_count; ++i) {
		ctx->cmd_stat_backup_buf[i] = (struct ps3_single_cmd_stat*)ps3_kzalloc(instance,
			sizeof(struct ps3_single_cmd_stat));
		if (ctx->cmd_stat_backup_buf[i] == NULL) {
			LOG_ERROR("Failed to alloc mem for ps3_single_cmd_stat\n");
			ret = -PS3_FAILED;
			goto l_free_mem;
		}
	}

	goto l_out;

l_free_mem:
	ps3_cmd_stat_backup_buf_free(instance);
l_out:
	return ret;
}

static inline struct ps3_single_cmd_stat* ps3_backup_stat_entry_find(U32 index,
	struct ps3_instance *instance)
{
	return (index < instance->cmd_statistics.stat_entry_max_count) ?
		instance->cmd_statistics.cmd_stat_backup_buf[index] : NULL;
}

static inline void ps3_cmd_stat_backup_buf_init(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;

	for (; i < ctx->stat_entry_max_count; ++i) {
		memset(ctx->cmd_stat_backup_buf[i], 0,
			sizeof(struct ps3_single_cmd_stat));
		mb();
	}

	return;
}
static inline struct ps3_single_cmd_stat* ps3_stat_entry_find(U32 index,
	struct ps3_instance *instance)
{
	return (index < instance->cmd_statistics.stat_entry_max_count) ?
		instance->cmd_statistics.cmd_stat_buf[index] : NULL;
}

static inline void ps3_stat_buf_init(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;

	for (; i < ctx->stat_entry_max_count; ++i) {
		memset(ctx->cmd_stat_buf[i], 0, sizeof(struct ps3_single_cmd_stat));
		mb();
	}

	return;
}

void ps3_stat_all_clear(struct ps3_instance *instance)
{
	ps3_stat_buf_init(instance);
	ps3_cmd_stat_backup_buf_init(instance);
	ps3_last_stat_init(instance);
	memset(&instance->cmd_statistics.total_stat, 0, sizeof(struct ps3_total_cmd_stat));
	memset(&instance->cmd_statistics.inc_stat, 0, sizeof(struct ps3_total_cmd_stat));
}

static inline void ps3_stat_start_inc(U16 type,
	struct ps3_single_cmd_stat *cmd_stat)
{
	if (type >= PS3_CMD_STAT_COUNT) {
		LOG_ERROR("type:%u overflow\n", type);
		goto l_out;
	}

	cmd_stat->stat[type].start++;

	cmd_stat->stat[type].lagency.start_time = ps3_tick_count_get();

l_out:
	return;
}

static inline void ps3_stat_lagency_update(struct ps3_lagency_info *lagency,
	U64 start_time, U64 back_time)
{
	U64 intervals = back_time - start_time;

	lagency->all += intervals;
	if (intervals > lagency->max_lagency) {
		lagency->max_lagency = intervals;
	}

	if (intervals < lagency->min_lagency || lagency->min_lagency == 0) {
		lagency->min_lagency = intervals;
	}

	return;
}

static inline void ps3_stat_lagency_merge(struct ps3_lagency_info *target,
	const struct ps3_lagency_info *source)
{
	target->all += source->all;
	if (source->max_lagency > target->max_lagency) {
		target->max_lagency = source->max_lagency;
	}

	if ((source->min_lagency < target->min_lagency &&
		source->min_lagency != 0) || target->min_lagency == 0) {
		target->min_lagency = source->min_lagency;
	}

	return;
}

static inline void ps3_stat_lagency_avg_calc(struct ps3_total_cmd_stat *total_stat)
{
	U64 back_count = 0;
	U16 i = 0;

	for(; i < PS3_CMD_STAT_COUNT; ++i) {
		back_count = total_stat->stat[i].back_good +
			total_stat->stat[i].back_err;
		if (back_count != 0) {
			total_stat->stat[i].lagency.avg =
				(total_stat->stat[i].lagency.all / back_count);
		}
		mb();
	}
}

static inline void ps3_stat_lagency_inc_cal(struct ps3_lagency_info *inc,
	const struct ps3_lagency_info *cur,
	const struct ps3_lagency_info *last)
{
	inc->all += cur->all - last->all;
	inc->max_lagency = (cur->max_lagency > inc->max_lagency) ?
		cur->max_lagency : inc->max_lagency;
	inc->min_lagency = ((cur->min_lagency < inc->min_lagency && cur->min_lagency != 0)
		|| inc->min_lagency == 0) ? cur->min_lagency : inc->min_lagency;

	return;
}

static inline void ps3_stat_back_inc(U16 type,
	struct ps3_single_cmd_stat *single_stat, U8 status)
{
	if (type >= PS3_CMD_STAT_COUNT) {
		LOG_ERROR("type:%u err\n", type);
		goto l_out;
	}

	single_stat->stat[type].lagency.back_time = ps3_tick_count_get();

	switch (status) {
	case PS3_STAT_BACK_OK:
		single_stat->stat[type].back_good++;
		break;
	case PS3_STAT_BACK_FAIL:
		single_stat->stat[type].back_err++;
		break;
	case PS3_STAT_BACK_NO:
		single_stat->stat[type].not_back++;
		break;
	default:
		single_stat->stat[type].back_err++;
		break;
	}

	ps3_stat_lagency_update(&single_stat->stat[type].lagency,
		single_stat->stat[type].lagency.start_time,
		single_stat->stat[type].lagency.back_time);
	if (type == PS3_SCSI_ABORT) {
		LOG_WARN("start:%llu back:%llu all:%llu\n",
			single_stat->stat[type].lagency.start_time,
			single_stat->stat[type].lagency.back_time,
			single_stat->stat[type].lagency.all);
	}

l_out:
	return;
}

#ifndef _WINDOWS
static inline void ps3_scmd_start_inc(struct ps3_single_cmd_stat* stat_entry,
	const struct scsi_cmnd *s_cmd)
{
	U8 type = PS3_SCSI_CMD_TYPE(ps3_scsih_cdb_rw_type_get(s_cmd->cmnd));
	if (ps3_scsih_is_rw_type(type)) {
		if (ps3_scsih_is_read_cmd(type)) {
			ps3_stat_start_inc(PS3_SCSI_DRV_READ, stat_entry);
		} else {
			ps3_stat_start_inc(PS3_SCSI_DRV_WRITE, stat_entry);
		}
	} else {
		ps3_stat_start_inc(PS3_SCSI_DRV_NORW, stat_entry);
	}

	ps3_stat_start_inc(PS3_SCSI_DRV_ALL, stat_entry);

	if (s_cmd->retries != 0) {
		ps3_stat_start_inc(PS3_SCSI_RETRY_CMD, stat_entry);
	}

	return;
}

static inline void ps3_scmd_back_inc(struct ps3_single_cmd_stat* stat_entry,
	const struct scsi_cmnd *s_cmd, U8 status)
{
	U8 type = PS3_SCSI_CMD_TYPE(ps3_scsih_cdb_rw_type_get(s_cmd->cmnd));

	if (ps3_scsih_is_rw_type(type)) {
		if (ps3_scsih_is_read_cmd(type)) {
			ps3_stat_back_inc(PS3_SCSI_DRV_READ, stat_entry, status);
		} else {
			ps3_stat_back_inc(PS3_SCSI_DRV_WRITE, stat_entry, status);
		}
	} else {
		ps3_stat_back_inc(PS3_SCSI_DRV_NORW, stat_entry, status);
	}

	ps3_stat_back_inc(PS3_SCSI_DRV_ALL, stat_entry, status);

	if (s_cmd->retries != 0) {
		ps3_stat_back_inc(PS3_SCSI_RETRY_CMD, stat_entry, status);
	}

	return;
}

void ps3_scmd_inc(struct ps3_instance *instance,
	struct scsi_cmnd *s_cmd, U8 type, U8 status)
{
	struct ps3_single_cmd_stat* stat_entry = NULL;

	if (s_cmd == NULL) {
		LOG_ERROR("scsi cmd is null\n");
		goto l_out;
	}

	if (SCMD_GET_REQUEST(s_cmd) == NULL) {
		LOG_ERROR("scsi_cmnd->request is null\n");
		goto l_out;
	}

	stat_entry = ps3_stat_entry_find(SCMD_GET_REQUEST(s_cmd)->tag, instance);
	if (stat_entry == NULL) {
		LOG_ERROR("host_no:%u CFID:%u stat entry find fail\n",
			PS3_HOST(instance), SCMD_GET_REQUEST(s_cmd)->tag);
		goto l_out;
	}

	if (type == PS3_STAT_START) {
		ps3_scmd_start_inc(stat_entry, s_cmd);
	} else if (type == PS3_STAT_BACK) {
		ps3_scmd_back_inc(stat_entry, s_cmd, status);
	} else {
		LOG_ERROR("type:%u is err\n", type);
	}

l_out:
	return;
}
#else
static inline void ps3_scmd_start_inc(struct ps3_single_cmd_stat* stat_entry,
	const struct scsi_cmnd *s_cmd)
{
	U8 type = PS3_SCSI_CMD_TYPE(ps3_scsih_cdb_rw_type_get(s_cmd->cmnd));
	if (ps3_scsih_is_rw_type(type)) {
		if (ps3_scsih_is_read_cmd(type)) {
			ps3_stat_start_inc(PS3_SCSI_DRV_READ, stat_entry);
		} else {
			ps3_stat_start_inc(PS3_SCSI_DRV_WRITE, stat_entry);
		}
	} else {
		ps3_stat_start_inc(PS3_SCSI_DRV_NORW, stat_entry);
	}

	ps3_stat_start_inc(PS3_SCSI_DRV_ALL, stat_entry);

	return;
}

static inline void ps3_scmd_back_inc(struct ps3_single_cmd_stat* stat_entry,
	const struct scsi_cmnd *s_cmd, U8 status)
{
	U8 type = PS3_SCSI_CMD_TYPE(ps3_scsih_cdb_rw_type_get(scsi_cmnd_cdb(s_cmd)));

	if (ps3_scsih_is_rw_type(type)) {
		if (ps3_scsih_is_read_cmd(type)) {
			ps3_stat_back_inc(PS3_SCSI_DRV_READ, stat_entry, status);
		} else {
			ps3_stat_back_inc(PS3_SCSI_DRV_WRITE, stat_entry, status);
		}
	} else {
		ps3_stat_back_inc(PS3_SCSI_DRV_NORW, stat_entry, status);
	}

	ps3_stat_back_inc(PS3_SCSI_DRV_ALL, stat_entry, status);

	return;
}

void ps3_scmd_inc(struct ps3_instance *instance,
	const struct scsi_cmnd *s_cmd, U32 tag, U8 type, U8 status)
{
	struct ps3_single_cmd_stat* stat_entry = NULL;

	if (s_cmd == NULL) {
		LOG_ERROR("scsi cmd is null\n");
		goto l_out;
	}

	stat_entry = ps3_stat_entry_find(tag, instance);
	if (stat_entry == NULL) {
		LOG_ERROR("host_no:%u CFID:%u stat entry find fail\n",
			PS3_HOST(instance), tag);
		goto l_out;
	}

	if (type == PS3_STAT_START) {
		ps3_scmd_start_inc(stat_entry, s_cmd);
	}
	else if (type == PS3_STAT_BACK) {
		ps3_scmd_back_inc(stat_entry, s_cmd, status);
	} else {
		LOG_ERROR("type:%u is err\n", type);
	}

l_out:
	return;
}
#endif
static inline Bool ps3_is_direct_cmd(U8 type)
{
	return (type == PS3_CMDWORD_DIRECT_OK ||
		type == PS3_CMDWORD_DIRECT_ADVICE);
}

static inline void ps3_direct_cmd_start_inc(const struct ps3_cmd *cmd,
	struct ps3_single_cmd_stat *stat_entry, Bool vd_flag)
{
	U16 type = PS3_DRV_IOC_READ;

	if (ps3_is_direct_cmd(cmd->io_attr.direct_flag)) {
		if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
			type = vd_flag ? PS3_DRV_IOC_VD_D_READ :
				PS3_DRV_IOC_PD_D_READ;
		} else {
			type = vd_flag ? PS3_DRV_IOC_VD_D_WRITE :
				PS3_DRV_IOC_PD_D_WRITE;
		}
		ps3_stat_start_inc(type, stat_entry);
	}

	return;
}

static inline void ps3_scmd_drv2ioc_start_vd_inc(const struct ps3_cmd *cmd,
	struct ps3_single_cmd_stat *stat_entry)
{
	if (ps3_scsih_is_rw_type(cmd->io_attr.rw_flag)) {
		if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
			ps3_stat_start_inc(PS3_DRV_IOC_VD_READ, stat_entry);
			ps3_stat_start_inc(PS3_DRV_IOC_READ, stat_entry);
		} else {
			ps3_stat_start_inc(PS3_DRV_IOC_VD_WRITE, stat_entry);
			ps3_stat_start_inc(PS3_DRV_IOC_WRITE, stat_entry);
		}
	} else {
		ps3_stat_start_inc(PS3_DRV_IOC_NORW, stat_entry);
		ps3_stat_start_inc(PS3_DRV_IOC_VD_NORW, stat_entry);
	}

	ps3_direct_cmd_start_inc(cmd, stat_entry, PS3_TRUE);
}

static inline void ps3_scmd_drv2ioc_start_pd_inc(const struct ps3_cmd *cmd,
	struct ps3_single_cmd_stat *stat_entry)
{
	if (ps3_scsih_is_rw_type(cmd->io_attr.rw_flag)) {
		if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
			ps3_stat_start_inc(PS3_DRV_IOC_PD_READ, stat_entry);
			ps3_stat_start_inc(PS3_DRV_IOC_READ, stat_entry);
		} else {
			ps3_stat_start_inc(PS3_DRV_IOC_PD_WRITE, stat_entry);
			ps3_stat_start_inc(PS3_DRV_IOC_WRITE, stat_entry);
		}
	} else {
		ps3_stat_start_inc(PS3_DRV_IOC_PD_NORW, stat_entry);
		ps3_stat_start_inc(PS3_DRV_IOC_NORW, stat_entry);
	}

	ps3_direct_cmd_start_inc(cmd, stat_entry, PS3_FALSE);
}

void ps3_scmd_drv2ioc_start_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd)
{
	struct ps3_single_cmd_stat *stat_entry = NULL;

	if (cmd == NULL) {
		goto l_out;
	}

	stat_entry = ps3_stat_entry_find(cmd->index, instance);
	if (stat_entry == NULL) {
		LOG_ERROR("host_no:%u index:%u stat entry find fail\n",
			PS3_HOST(instance), cmd->index);
		goto l_out;
	}

	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		ps3_scmd_drv2ioc_start_vd_inc(cmd, stat_entry);
	} else {
		ps3_scmd_drv2ioc_start_pd_inc(cmd, stat_entry);
	}

	ps3_stat_start_inc(PS3_DRV_IOC_ALL, stat_entry);
	ps3_atomic_inc(&instance->cmd_statistics.cmd_outstanding);

l_out:
	return;
}

static inline void ps3_direct_cmd_back_inc(const struct ps3_cmd *cmd,
	struct ps3_single_cmd_stat *stat_entry, Bool vd_flag, U8 status)
{
	U16 type = PS3_DRV_IOC_READ;
	if (ps3_is_direct_cmd(cmd->io_attr.direct_flag)) {
		if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
			type = vd_flag ? PS3_DRV_IOC_VD_D_READ :
				PS3_DRV_IOC_PD_D_READ;
		} else {
			type = vd_flag ? PS3_DRV_IOC_VD_D_WRITE :
				PS3_DRV_IOC_PD_D_WRITE;
		}
		ps3_stat_back_inc(type, stat_entry, status);
	}

	return;
}

static inline void ps3_scmd_drv2ioc_back_vd_inc(const struct ps3_cmd *cmd,
	struct ps3_single_cmd_stat *stat_entry, U8 status)
{
	if (ps3_scsih_is_rw_type(cmd->io_attr.rw_flag)) {
		if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
			ps3_stat_back_inc(PS3_DRV_IOC_VD_READ, stat_entry, status);
			ps3_stat_back_inc(PS3_DRV_IOC_READ, stat_entry, status);
		} else {
			ps3_stat_back_inc(PS3_DRV_IOC_VD_WRITE, stat_entry, status);
			ps3_stat_back_inc(PS3_DRV_IOC_WRITE, stat_entry, status);
		}
	} else {
		ps3_stat_back_inc(PS3_DRV_IOC_NORW, stat_entry, status);
		ps3_stat_back_inc(PS3_DRV_IOC_VD_NORW, stat_entry, status);
	}

	ps3_direct_cmd_back_inc(cmd, stat_entry, PS3_TRUE, status);

	return;
}

static inline void ps3_scmd_drv2ioc_back_pd_inc(const struct ps3_cmd *cmd,
	struct ps3_single_cmd_stat *stat_entry, U8 status)
{
	if (ps3_scsih_is_rw_type(cmd->io_attr.rw_flag)) {
		if (ps3_scsih_is_read_cmd(cmd->io_attr.rw_flag)) {
			ps3_stat_back_inc(PS3_DRV_IOC_PD_READ, stat_entry, status);
			ps3_stat_back_inc(PS3_DRV_IOC_READ, stat_entry, status);
		} else {
			ps3_stat_back_inc(PS3_DRV_IOC_PD_WRITE, stat_entry, status);
			ps3_stat_back_inc(PS3_DRV_IOC_WRITE, stat_entry, status);
		}
	} else {
		ps3_stat_back_inc(PS3_DRV_IOC_PD_NORW, stat_entry, status);
		ps3_stat_back_inc(PS3_DRV_IOC_NORW, stat_entry, status);
	}

	ps3_direct_cmd_back_inc(cmd, stat_entry, PS3_FALSE, status);
}

void ps3_scmd_drv2ioc_back_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd, U8 status)
{
	struct ps3_single_cmd_stat *stat_entry = NULL;

	if (cmd == NULL) {
		LOG_ERROR("host_no:%u cmd is null\n", PS3_HOST(instance));
		goto l_out;
	}

	stat_entry = ps3_stat_entry_find(cmd->index, instance);
	if (stat_entry == NULL) {
		LOG_ERROR("host_no:%u index:%u stat entry find fail\n",
			PS3_HOST(instance), cmd->index);
		goto l_out;
	}

	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		ps3_scmd_drv2ioc_back_vd_inc(cmd, stat_entry, status);
	} else {
		ps3_scmd_drv2ioc_back_pd_inc(cmd, stat_entry, status);
	}

	ps3_stat_back_inc(PS3_DRV_IOC_ALL, stat_entry, status);
	ps3_atomic_dec(&instance->cmd_statistics.cmd_outstanding);

l_out:
	return;
}

static inline void ps3_task_cmd_start_inc(struct ps3_single_cmd_stat *stat_entry,
	const struct ps3_cmd *cmd)
{
	switch (cmd->req_frame->mgrReq.reqHead.cmdSubType) {
	case PS3_TASK_CMD_SCSI_TASK_ABORT:
		ps3_stat_start_inc(PS3_SCSI_ABORT, stat_entry);
		break;

	case PS3_TASK_CMD_SCSI_TASK_RESET:
		ps3_stat_start_inc(PS3_SCSI_DEVICE_RESET, stat_entry);
		break;

	default:
		break;
	}

	return;
}

void ps3_mgr_start_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd)
{
	struct ps3_single_cmd_stat *stat_entry =
		ps3_stat_entry_find(cmd->index, instance);
	if (stat_entry == NULL) {
		LOG_ERROR("host_no:%u CFID:%u stat entry find fail\n",
			PS3_HOST(instance), cmd->index);
		goto l_out;
	}

	switch (cmd->req_frame->mgrReq.reqHead.cmdType) {
	case PS3_CMD_MANAGEMENT:
	case PS3_CMD_SAS_MANAGEMENT:
	case PS3_CMD_IOCTL:
		goto l_mgr_inc;
	case PS3_CMD_SCSI_TASK_MANAGEMENT:
		ps3_task_cmd_start_inc(stat_entry, cmd);
		goto l_mgr_inc;
	default:
		goto l_out;
	}

l_mgr_inc:
	ps3_stat_start_inc(PS3_DRV_IOC_MGR, stat_entry);
l_out:
	return;
}

static inline void ps3_task_cmd_back_inc(struct ps3_single_cmd_stat *stat_entry,
	const struct ps3_cmd *cmd, U8 status)
{
	switch (cmd->req_frame->mgrReq.reqHead.cmdSubType) {
	case PS3_TASK_CMD_SCSI_TASK_ABORT:
		ps3_stat_back_inc(PS3_SCSI_ABORT, stat_entry, status);
		break;

	case PS3_TASK_CMD_SCSI_TASK_RESET:
		ps3_stat_back_inc(PS3_SCSI_DEVICE_RESET, stat_entry, status);
		break;

	default:
		break;
	}

	return;
}

void ps3_mgr_back_inc(struct ps3_instance *instance,
	const struct ps3_cmd *cmd, U8 status)
{
	struct ps3_single_cmd_stat *stat_entry =
		ps3_stat_entry_find(cmd->index, instance);
	if (stat_entry == NULL) {
		LOG_ERROR("host_no:%u index:%u stat entry find fail\n",
			PS3_HOST(instance), cmd->index);
		goto l_out;
	}

	switch (cmd->req_frame->mgrReq.reqHead.cmdType) {
	case PS3_CMD_MANAGEMENT:
	case PS3_CMD_SAS_MANAGEMENT:
	case PS3_CMD_IOCTL:
		goto l_mgr_inc;

	case PS3_CMD_SCSI_TASK_MANAGEMENT:
		ps3_task_cmd_back_inc(stat_entry, cmd, status);
		goto l_mgr_inc;

	default:
		goto l_out;
	}

l_mgr_inc:
	ps3_stat_back_inc(PS3_DRV_IOC_MGR, stat_entry, status);
l_out:
	return;
}

#ifndef _WINDOWS

static inline void ps3_stat_workq_service(struct work_struct *work)
{
 	struct ps3_cmd_stat_wrokq_context *ctx = ps3_container_of(work,
		struct ps3_cmd_stat_wrokq_context, stat_work.work);
	struct ps3_instance *instance = NULL;

	if (ctx == NULL) {
		LOG_ERROR("ps3_cmd_stat_wrokq_context is null\n");
		goto l_out;
	}

	instance = ctx->instance;
	if (!ps3_stat_inc_switch_is_open(instance)) {
		LOG_INFO("cmd_stat_switch is close\n");
		goto l_out;
	}

	if (ctx->is_stop) {
		LOG_INFO("cmd_stat_switch workq is stop\n");
		goto l_out;
	}

	ps3_stat_data_collect(instance);

	if (ctx->stat_queue != NULL) {
		queue_delayed_work(ctx->stat_queue, &ctx->stat_work,
			msecs_to_jiffies(instance->cmd_statistics.stat_interval));
	}

l_out:
	return;
}

S32 ps3_stat_workq_start(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd_stat_wrokq_context *ctx = &instance->cmd_statistics.stat_workq;
	char qname[PS3_STAT_WROKQ_NAME_MAX_LEN] = { 0 };
	struct delayed_work *work = &ctx->stat_work;

	if (ctx->stat_queue != NULL) {
		LOG_INFO("host_no:%u stat work for is already started!\n",
			PS3_HOST(instance));
		goto l_out;
	}

	memset(qname, 0, sizeof(qname));
	memset(ctx, 0, sizeof(struct ps3_cmd_stat_wrokq_context));
	INIT_DELAYED_WORK(work, ps3_stat_workq_service);
	snprintf(qname, sizeof(qname), "ps3_stat_%u", PS3_HOST(instance));

	ctx->stat_queue = create_singlethread_workqueue(qname);
	if (ctx->stat_queue == NULL) {
		LOG_ERROR("host_no:%u cmd stat work queue create failed\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	ctx->instance = instance;

	if (!ps3_stat_inc_switch_is_open(instance)) {
		LOG_INFO("cmd_stat_switch is close\n");
		goto l_out;
	}

	ctx->is_stop = PS3_FALSE;
	queue_delayed_work(ctx->stat_queue, work,
		msecs_to_jiffies(instance->cmd_statistics.stat_interval));

l_out:

	return ret;
}

void ps3_stat_workq_stop(struct ps3_instance *instance)
{
  	struct ps3_cmd_stat_wrokq_context *ctx = NULL;
	struct workqueue_struct *workq = NULL;

	ctx = &instance->cmd_statistics.stat_workq;

	ctx->is_stop = PS3_TRUE;

	if (ctx->stat_queue == NULL) {
		goto l_out;
	}

	workq = ctx->stat_queue;
	if (!cancel_delayed_work_sync(&ctx->stat_work)) {
		flush_workqueue(workq);
	}
	ctx->stat_queue = NULL;

	destroy_workqueue(workq);

l_out:
	memset(ctx, 0, sizeof(struct ps3_cmd_stat_wrokq_context));
	return;
}
#else

static inline void ps3_stat_workq_service(void *ins)
{
	struct ps3_instance *instance = (struct ps3_instance*)ins;
	struct ps3_cmd_stat_wrokq_context* ctx = &instance->cmd_statistics.stat_workq;

	if (ctx == NULL) {
		LOG_ERROR("ps3_cmd_stat_wrokq_context is null\n");
		goto l_out;
	}

	if (!ps3_stat_inc_switch_is_open(instance)) {
		LOG_INFO("cmd_stat_switch is close\n");
		goto l_out;
	}

	if (ctx->is_stop) {
		LOG_INFO("cmd_stat_switch workq is stop\n");
		goto l_out;
	}

	ps3_stat_data_collect(instance);

	ps3_delay_worker_start(&ctx->statis_work, instance->cmd_statistics.stat_interval);

l_out:
	return;
}

S32 ps3_stat_workq_start(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd_stat_wrokq_context* ctx = &instance->cmd_statistics.stat_workq;
	struct ps3_delay_worker* statis_work = &ctx->statis_work;

	memset(ctx, 0, sizeof(struct ps3_cmd_stat_wrokq_context));

	if (ps3_delay_worker_init(instance, statis_work, "ps3_statis", ps3_stat_workq_service) != PS3_SUCCESS) {
		LOG_ERROR("host_no:%u timer init failed\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	if (!ps3_stat_inc_switch_is_open(instance)) {
		LOG_INFO("cmd_stat_switch is close\n");
		goto l_out;
	}

	ctx->is_stop = PS3_FALSE;
	if (ps3_delay_worker_start(statis_work, instance->cmd_statistics.stat_interval) != PS3_SUCCESS) {
		LOG_ERROR("host_no:%u timer request failed\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		ctx->is_stop = PS3_TRUE;
		goto l_out;
	}
l_out:
	return ret;
}

void ps3_stat_workq_stop(struct ps3_instance *instance)
{
	struct ps3_cmd_stat_wrokq_context* ctx = NULL;
	ctx = &instance->cmd_statistics.stat_workq;

	ctx->is_stop = PS3_TRUE;
	ps3_delay_worker_exit(&ctx->statis_work);

	memset(ctx, 0, sizeof(struct ps3_cmd_stat_wrokq_context));
	return;
}
#endif
static void ps3_backup_stat_entry_build(struct ps3_instance *instance)
{
	U32 i  = 0;
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;

	for (; i < ctx->stat_entry_max_count; ++i) {
		memcpy(ctx->cmd_stat_backup_buf[i], ctx->cmd_stat_buf[i],
			sizeof(struct ps3_single_cmd_stat));
		mb();
	}

	return;
}

static inline void ps3_last_stat_entry_build(struct ps3_instance *instance)
{
	U32 i  = 0;
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;

	for (; i < ctx->stat_entry_max_count; ++i) {
		memcpy(ctx->last_stat_buf[i], ctx->cmd_stat_backup_buf[i],
			sizeof(struct ps3_single_cmd_stat));
		mb();
	}

	return;
}
static void ps3_stat_inc_data_collect(struct ps3_instance *instance)
{
	U32 i  = 0;
	U16 j = 0; 
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;
	struct ps3_single_cmd_stat *backup_stat = NULL;
	struct ps3_single_cmd_stat *last_stat = NULL;
	U64 inc_back_good = 0;
	U64 inc_back_err = 0;

	memset(&ctx->inc_stat, 0, sizeof(ctx->inc_stat));
	for (; i < ctx->stat_entry_max_count; ++i) {
		backup_stat = ps3_backup_stat_entry_find(i, instance);
		if (backup_stat == NULL) {
			LOG_WARN("host_no:%u index:%u get backup_stat err\n",
				PS3_HOST(instance), i);
			continue;
		}
		last_stat = ps3_last_stat_entry_find(i, instance);
		if (last_stat == NULL) {
			LOG_WARN("host_no:%u index:%u get last_stat err\n",
				PS3_HOST(instance), i);
			continue;
		}

		mb();

		for (j = 0; j < PS3_CMD_STAT_COUNT; ++j) {
			ctx->inc_stat.stat[j].start +=
				backup_stat->stat[j].start - last_stat->stat[j].start;
			inc_back_good = backup_stat->stat[j].back_good -
				last_stat->stat[j].back_good;
			inc_back_err = backup_stat->stat[j].back_err -
				last_stat->stat[j].back_err;
			if ((inc_back_good + inc_back_err) == 0) {
				continue;
			}
			mb();
			ctx->inc_stat.stat[j].back_good += inc_back_good;
			ctx->inc_stat.stat[j].back_err += inc_back_err;
			ctx->inc_stat.stat[j].not_back +=
				backup_stat->stat[j].not_back -
				last_stat->stat[j].not_back;
			ps3_stat_lagency_inc_cal(&ctx->inc_stat.stat[j].lagency,
				&backup_stat->stat[j].lagency,
				&last_stat->stat[j].lagency);
		}
	}
	ps3_stat_lagency_avg_calc(&ctx->inc_stat);

	return;
}

static inline void ps3_stat_total_data_collect(struct ps3_instance *instance)
{
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;
	U64 back_count = 0;
	U16 i  = 0;

	for (; i < PS3_CMD_STAT_COUNT; ++i) {
		ctx->total_stat.stat[i].start += ctx->inc_stat.stat[i].start;
		ctx->total_stat.stat[i].back_good += ctx->inc_stat.stat[i].back_good;
		ctx->total_stat.stat[i].back_err += ctx->inc_stat.stat[i].back_err;
		back_count = ctx->total_stat.stat[i].back_good + ctx->total_stat.stat[i].back_err;
		ctx->total_stat.stat[i].not_back = ctx->total_stat.stat[i].start - back_count;
		ps3_stat_lagency_merge(&ctx->total_stat.stat[i].lagency,
			&ctx->inc_stat.stat[i].lagency);
		mb();
		if (back_count != 0) {
			ctx->total_stat.stat[i].lagency.avg =
				(ctx->total_stat.stat[i].lagency.all / back_count);
		}
	}

	return;
}

static inline void ps3_stat_data_log(const struct ps3_cmd_stat_entry *stat,
	const struct ps3_cmd_stat_entry *inc_stat, U16 stat_num)
{
	U16 i = 0;

	for (; i < stat_num; ++i) {
		if (inc_stat[i].start == 0) {
			continue;
		}
		LOG_INFO("[%s:] start:%llu back_good:%llu back_err:%llu "
			"not_back:%llu avg[%lluus] max[%lluus] min[%lluus]\n",
			ps3_cmd_stat_item_tostring((enum ps3_cmd_stat_item)i),
			stat[i].start, stat[i].back_good, stat[i].back_err,
			stat[i].not_back, stat[i].lagency.avg,
			stat[i].lagency.max_lagency, stat[i].lagency.min_lagency);
	}

	return;
}

static inline void ps3_stat_total_data_log(struct ps3_instance *instance)
{
	LOG_INFO("host_no:%u total cmd stat show begin\n", PS3_HOST(instance));
	ps3_stat_data_log(instance->cmd_statistics.total_stat.stat,
		instance->cmd_statistics.inc_stat.stat, PS3_CMD_STAT_COUNT);
	LOG_INFO("host_no:%u total cmd stat show end\n", PS3_HOST(instance));

	return;
}

static inline void ps3_stat_inc_data_log(struct ps3_instance *instance)
{
	LOG_INFO("host_no:%u inc cmd stat show begin \n", PS3_HOST(instance));
	ps3_stat_data_log(instance->cmd_statistics.inc_stat.stat,
		instance->cmd_statistics.inc_stat.stat, PS3_CMD_STAT_COUNT);
	LOG_INFO("host_no:%u inc cmd stat show end \n", PS3_HOST(instance));

	return;
}

static inline Bool ps3_stat_is_write_log(struct ps3_instance *instance)
{
	return instance->cmd_statistics.log_record_count == PS3_STAT_LOG_COUNT;
}

static inline void ps3_stat_log_count_inc(struct ps3_instance *instance)
{
	instance->cmd_statistics.log_record_count++;
	if (instance->cmd_statistics.log_record_count == PS3_STAT_LOG_MAX_COUNT) {
		instance->cmd_statistics.log_record_count = 0;
	}
}

static inline void ps3_stat_write_log(struct ps3_instance *instance)
{
	if (ps3_stat_log_switch_is_open(instance) &&
		ps3_stat_inc_switch_is_open(instance)) {
		if (ps3_stat_is_write_log(instance)) {
			ps3_stat_inc_data_log(instance);

			ps3_stat_total_data_log(instance);
		}
		ps3_stat_log_count_inc(instance);
	}

	return;
}

static inline void ps3_stat_data_collect(struct ps3_instance *instance)
{
	ps3_backup_stat_entry_build(instance);

	ps3_stat_inc_data_collect(instance);

	ps3_stat_total_data_collect(instance);

	ps3_last_stat_entry_build(instance);

	ps3_stat_write_log(instance);

	return;
}

void ps3_qos_stat_inc(struct ps3_instance *instance,
	struct ps3_cmd *cmd, U16 stat, U16 type)
{
    struct ps3_single_cmd_stat *stat_entry = ps3_stat_entry_find(cmd->index, instance);
	if (stat_entry == NULL) {
		LOG_ERROR("host_no:%u index:%u stat entry find fail\n",
			PS3_HOST(instance), cmd->index);
        return;
	}

    if (type == PS3_STAT_START) {
		ps3_stat_start_inc(stat, stat_entry);
	} else if (type == PS3_STAT_BACK) {
		ps3_stat_back_inc(stat, stat_entry, PS3_STAT_BACK_OK);
	}
}
