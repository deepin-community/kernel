#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/jiffies.h>
#include <linux/version.h>
#include <scsi/scsi_host.h>
#include "ps3_ioc_manager.h"
#include "ps3_util.h"
#include "ps3_types.h"
#include "ps3_instance_manager.h"
#include "ps3_htp_def.h"
#include "ps3_scsih.h"
#include "ps3_mgr_cmd.h"
#include "ps3_driver_log.h"
#include "ps3_htp_dev.h"
#include "ps3_scsih_cmd_parse.h"
#include "ps3_scsi_cmd_err.h"
#include "ps3_r1x_write_lock.h"
#include "ps3_cmd_statistics.h"
#include "ps3_scsih_raid_engine.h"
#include "ps3_ioc_state.h"

#define PS3_QOS_PD_IS_VD_MEMBER(qos_pd_mgr) ((qos_pd_mgr)->vd_id > 0)

#define PS3_QOS_JBOD_PD_WAITQ_ID(qos_pd_mgr) ((qos_pd_mgr)->waitq_cnt - 1)

#define PS3_QOS_VD_HDD_DELAY_THD_MS 500

#define PS3_QOS_VD_SDD_DELAY_THD_MS 1

static inline Bool ps3_tfifo_depth_get(struct ps3_instance *instance,
	U64 *depth)
{
	Bool ret = PS3_TRUE;
	PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3TfifoDepth, *depth);
	if (*depth == U64_MAX) {
		LOG_INFO("host_no:%u read reg ps3TfifoDepth failed!\n",
			PS3_HOST(instance));
		*depth = 0;
		ret = PS3_FALSE;
		goto l_out;
	}

	*depth &= 0xffff;

l_out:
	return ret;
}

static inline Bool ps3_cmdq_depth_get(struct ps3_instance *instance,
	U64 *depth)
{
	Bool ret = PS3_TRUE;
	PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3CmdQueueDepth, *depth);

	if (*depth == U64_MAX) {
		LOG_INFO("host_no:%u read reg ps3CmdQueueDepth failed!\n",
			PS3_HOST(instance));
		*depth = 0;
		ret = PS3_FALSE;
		goto l_out;
	}

	*depth &= 0xffff;

l_out:
	return ret;
}

static inline Bool ps3_mgrq_depth_get(struct ps3_instance *instance,
	U64 *depth)
{
	Bool ret = PS3_TRUE;
	PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3MgrQueueDepth, *depth);

	if (*depth == U64_MAX) {
		LOG_INFO("host_no:%u read reg ps3MgrQueueDepth failed!\n",
			PS3_HOST(instance));
		*depth = 0;
		ret = PS3_FALSE;
		goto l_out;
	}

	*depth &= 0xffff;

l_out:
	return ret;
}

static inline struct ps3_qos_pd_mgr *ps3_qos_pd_mgr_get(struct ps3_instance *instance, U16 disk_id)
{
		return &instance->qos_context.pd_ctx.qos_pd_mgrs[disk_id];
}

static inline bool ps3_is_nvme_direct_cmd(struct ps3_qos_pd_mgr *qos_pd_mgr, struct ps3_cmd *cmd)
{
	return (qos_pd_mgr->dev_type == PS3_DEV_TYPE_NVME_SSD &&
				cmd->io_attr.direct_flag == PS3_CMDWORD_DIRECT_OK);
}

static void ps3_qos_update_pd_quota(struct ps3_cmd *cmd)
{
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	U16 i = 0;
	U16 disk_id = 0;
	S32 pd_used_quota = 0;
	ps3_atomic32 *pd_rsc = NULL;

	for (i = 0; i < cmd->target_pd_count; i++) {
		if (cmd->target_pd[i].get_quota) {
			disk_id = cmd->target_pd[i].flat_disk_id;
			qos_pd_mgr = ps3_qos_pd_mgr_get(cmd->instance, disk_id);
			if (ps3_is_nvme_direct_cmd(qos_pd_mgr, cmd)) {
				pd_rsc = &qos_pd_mgr->direct_used_quota;
			} else {
				pd_rsc = &qos_pd_mgr->pd_used_quota;
			}
			pd_used_quota = ps3_atomic_dec_return(pd_rsc);
			LOG_DEBUG("update pd quota. host_no:%u t_id:0x%llx CFID:%u direct:%u pid:%u used_quota:%d\n",
				PS3_HOST(cmd->instance), cmd->trace_id, cmd->index, cmd->cmd_word.direct,
				disk_id, pd_used_quota);
			break;
		}
	}
}

static inline struct ps3_qos_vd_mgr *ps3_qos_vd_mgr_get(struct ps3_cmd *cmd)
{
	U16 disk_id = 0;
	U16 flat_disk_id = 0;
	struct ps3_instance *instance = cmd->instance;

	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		disk_id = PS3_VDID(&cmd->io_attr.vd_entry->diskPos);
		flat_disk_id = get_offset_of_vdid(PS3_VDID_OFFSET(instance), disk_id);
		return &instance->qos_context.vd_ctx.qos_vd_mgrs[flat_disk_id];
	} else {
		return &instance->qos_context.vd_ctx.qos_vd_mgrs[instance->qos_context.max_vd_count];
	}
}

static inline struct ps3_qos_vd_mgr *ps3_qos_vd_mgr_get_by_id(struct ps3_instance *instance,
				U16 disk_id)
{
	struct ps3_qos_vd_context *qos_vd_ctx = NULL;
	U16 flat_disk_id = 0;

	qos_vd_ctx = &instance->qos_context.vd_ctx;
	flat_disk_id = get_offset_of_vdid(PS3_VDID_OFFSET(instance), disk_id);
	return &qos_vd_ctx->qos_vd_mgrs[flat_disk_id];
}

static void ps3_qos_update_vd_quota(struct ps3_cmd *cmd)
{
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;

	if (cmd->io_attr.dev_type != PS3_DEV_TYPE_VD) {
		qos_vd_mgr =  ps3_qos_vd_mgr_get(cmd);
		ps3_atomic_inc(&qos_vd_mgr->vd_quota);
	}
}

Bool ps3_qos_enable(struct ps3_instance *instance)
{
	return instance->qos_context.qos_switch;
}

static void  ps3_qos_cmd_resend_fail(struct ps3_cmd *cmd, S32 ret)
{
	struct ps3_scsi_priv_data *pri_data = NULL;
	struct ps3_instance *instance = cmd->instance;
	struct scsi_cmnd *s_cmd = cmd->scmd;

	if (ret == -PS3_RECOVERED || ret == -PS3_RETRY) {
		s_cmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_RESET);
	} else {
		s_cmd->result = PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT);
	}

	if (cmd->is_got_r1x == 1) {
		pri_data = (struct ps3_scsi_priv_data *)s_cmd->device->hostdata;
		ps3_r1x_write_unlock(&pri_data->lock_mgr, cmd);
	}

	LOG_INFO_IN_IRQ(instance, "t_id:0x%llx hno:%u tag:%d cmd send err ret:%d\n",
		cmd->trace_id, PS3_HOST(instance), cmd->index, ret);
	ps3_scsi_dma_unmap(cmd);
	s_cmd = cmd->scmd;
	PS3_DEV_IO_START_ERR_INC(instance, cmd);
	PS3_IO_OUTSTAND_DEC(cmd->instance, s_cmd);
	PS3_IO_BACK_ERR_INC(cmd->instance, s_cmd);
	PS3_VD_OUTSTAND_DEC(cmd->instance, s_cmd);
	PS3_DEV_BUSY_DEC(s_cmd);
	ps3_scsi_cmd_free(cmd);
	SCMD_IO_DONE(s_cmd);
}

struct qos_wait_queue *ps3_qos_cmd_waitq_get(struct ps3_qos_tg_context *qos_tg_ctx, struct ps3_cmd *cmd)
{
	struct qos_wait_queue *wait_q = NULL;
	U16 disk_id = 0;
	struct ps3_qos_context *qos_ctx = NULL;

	if (cmd->cmd_word.type == PS3_CMDWORD_TYPE_MGR) {
		wait_q = &qos_tg_ctx->mgr_cmd_wait_q;
	} else {
		if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
			disk_id = get_offset_of_vdid(PS3_VDID_OFFSET(cmd->instance),
						PS3_VDID(&cmd->io_attr.vd_entry->diskPos));
			wait_q = &qos_tg_ctx->vd_cmd_waitqs[disk_id];
		} else {
			qos_ctx = &cmd->instance->qos_context;
			wait_q = &qos_tg_ctx->vd_cmd_waitqs[qos_ctx->max_vd_count];
		}
	}

	return wait_q;
}

static void ps3_hba_qos_cmd_update(struct ps3_cmd *cmd)
{
	struct ps3_qos_tg_context *qos_tg_ctx = &cmd->instance->qos_context.tg_ctx;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
	if (cmd->cmd_word.type == PS3_CMDWORD_TYPE_MGR) {
		if (ps3_atomic_dec_return(&qos_tg_ctx->mgr_share_used) >= 0) { 
			ps3_atomic_inc(&qos_tg_ctx->share_free_cnt);
		} else {
			ps3_atomic_inc(&qos_tg_ctx->mgr_share_used);
			ps3_atomic_inc(&qos_tg_ctx->mgr_free_cnt);
		}
	} else {
		ps3_qos_update_pd_quota(cmd);
		if (cmd->cmd_word.direct != PS3_CMDWORD_DIRECT_OK) {
			qos_vd_mgr = ps3_qos_vd_mgr_get(cmd);
			if (cmd->io_attr.dev_type != PS3_DEV_TYPE_VD) { 
				ps3_atomic_inc(&qos_vd_mgr->vd_quota);
			}

			if (ps3_atomic_dec_return(&qos_vd_mgr->share_cmd_used) >= 0) {
				ps3_atomic_inc(&qos_tg_ctx->share_free_cnt);
			} else {
				ps3_atomic_inc(&qos_vd_mgr->share_cmd_used);
				ps3_atomic_inc(&qos_vd_mgr->exclusive_cmd_cnt);
			}
		}
	}
	LOG_DEBUG("qos cmd update. host_no:%u dev_t:%u share:%u\n",
		PS3_HOST(cmd->instance), cmd->io_attr.dev_type,
		ps3_atomic_read(&qos_tg_ctx->share_free_cnt));
}

static inline Bool ps3_qos_share_cmdword_dec(struct ps3_qos_tg_context *qos_tg_ctx)
{
	Bool can_get = PS3_FALSE;
	if (ps3_atomic_dec_return(&qos_tg_ctx->share_free_cnt) >= 0) {
		can_get = PS3_TRUE;
	} else {
		ps3_atomic_inc(&qos_tg_ctx->share_free_cnt);
		can_get = PS3_FALSE;
	}

	return can_get;
}

static Bool ps3_qos_share_cmdword_get(struct ps3_qos_tg_context *qos_tg_ctx,
			 struct ps3_cmd *cmd, struct qos_wait_queue *wait_q)
{
	Bool can_get = PS3_FALSE;
	ULong flag = 0;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;

	can_get = ps3_qos_share_cmdword_dec(qos_tg_ctx);
	if (!can_get) {
		ps3_spin_lock_irqsave(&qos_tg_ctx->lock, &flag);
		list_add_tail(&cmd->qos_list, &wait_q->wait_list);
		wait_q->count++;
		qos_tg_ctx->total_wait_cmd_cnt++;
		cmd->qos_waitq_flag = PS3_QOS_CMD_IN_FRAME;
		ps3_spin_unlock_irqrestore(&qos_tg_ctx->lock, flag);
		LOG_DEBUG("insert qos tag waitq.host_no:%u:t_id:0x%llx CFID:%u diskid:%u waitq:%u\n",
			PS3_HOST(cmd->instance), cmd->trace_id, cmd->index,
			cmd->io_attr.disk_id, wait_q->count);
		PS3_QOS_STAT_START(cmd->instance, cmd, PS3_QOS_TAG_QUEUE);
	} else {
		if (cmd->cmd_word.type == PS3_CMDWORD_TYPE_MGR) {
			ps3_atomic_inc(&qos_tg_ctx->mgr_share_used);
		} else {
			qos_vd_mgr = ps3_qos_vd_mgr_get(cmd);
			ps3_atomic_inc(&qos_vd_mgr->share_cmd_used);
		}
	}

	return can_get;
}

static bool ps3_qos_mgr_cmdword_get(struct ps3_cmd *cmd)
{
	bool can_get = PS3_FALSE;
	struct ps3_instance *instance = cmd->instance;
	struct ps3_qos_tg_context *qos_tg_ctx = &instance->qos_context.tg_ctx;

	if (ps3_atomic_dec_return(&qos_tg_ctx->mgr_free_cnt) >= 0) {
		can_get = PS3_TRUE;
	} else {
		ps3_atomic_inc(&qos_tg_ctx->mgr_free_cnt);
		can_get = PS3_FALSE;
	}

	return can_get;
}

Bool ps3_qos_vd_cmdword_get(struct ps3_cmd *cmd)
{
	Bool can_get = PS3_FALSE;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;

	qos_vd_mgr = ps3_qos_vd_mgr_get(cmd);
	if (ps3_atomic_dec_return(&qos_vd_mgr->exclusive_cmd_cnt) >= 0) {
		can_get = PS3_TRUE;
	} else {
		ps3_atomic_inc(&qos_vd_mgr->exclusive_cmd_cnt);
		can_get = PS3_FALSE;
	}

	return can_get;
}

Bool ps3_qos_exclusive_cmdword_get(struct ps3_cmd *cmd)
{
	Bool can_get = PS3_FALSE;

	if (cmd->cmd_word.type == PS3_CMDWORD_TYPE_MGR) {
		can_get = ps3_qos_mgr_cmdword_get(cmd);
	} else {
		can_get = ps3_qos_vd_cmdword_get(cmd);
	}

	return can_get;
}

Bool ps3_qos_tg_decision(struct ps3_cmd *cmd)
{
	Bool can_get = PS3_FALSE;
	struct ps3_instance *instance = cmd->instance;
	ULong flag = 0;
	struct qos_wait_queue *cmd_wait_q = NULL;
	struct ps3_qos_tg_context *qos_tg_ctx = &instance->qos_context.tg_ctx;

	PS3_QOS_STAT_START(instance, cmd, PS3_QOS_TAG_PRO);
	cmd_wait_q = ps3_qos_cmd_waitq_get(qos_tg_ctx, cmd);
	if (cmd_wait_q->count == 0) {
		can_get = ps3_qos_exclusive_cmdword_get(cmd); 
		if (can_get){
			goto out;
		}
	}

	if (likely(qos_tg_ctx->total_wait_cmd_cnt == 0)) {
		can_get = ps3_qos_share_cmdword_get(qos_tg_ctx, cmd, cmd_wait_q);
	} else {
		INJECT_START(PS3_ERR_IJ_QOS_WAIT_TAG_WAITQ_CLEAR, cmd->instance)
		ps3_spin_lock_irqsave(&qos_tg_ctx->lock, &flag);
		if (qos_tg_ctx->total_wait_cmd_cnt > 0){
			list_add_tail(&cmd->qos_list, &cmd_wait_q->wait_list);
			cmd_wait_q->count++;
			qos_tg_ctx->total_wait_cmd_cnt++;
			cmd->qos_waitq_flag = PS3_QOS_CMD_IN_FRAME;
			ps3_spin_unlock_irqrestore(&qos_tg_ctx->lock, flag);
			can_get = PS3_FALSE;
			LOG_DEBUG("insert qos tag waitq.host_no:%u:t_id:0x%llx CFID:%u diskid:%u waitq:%u\n",
				 PS3_HOST(instance), cmd->trace_id, cmd->index,
				 cmd->io_attr.disk_id, cmd_wait_q->count);
			PS3_QOS_STAT_START(instance, cmd, PS3_QOS_TAG_QUEUE);
		} else {
			ps3_spin_unlock_irqrestore(&qos_tg_ctx->lock, flag);
			can_get = ps3_qos_share_cmdword_get(qos_tg_ctx, cmd, cmd_wait_q);
		}
	}
out:
	PS3_QOS_STAT_END(instance, cmd, PS3_QOS_TAG_PRO);
	return can_get;
}

static inline bool ps3_qos_pd_quota_add(struct qos_wait_queue *waitq)
{
	Bool can_get = PS3_TRUE;

	if (ps3_atomic_inc_return(waitq->used_rsc) > *waitq->free_rsc) {
		ps3_atomic_dec(waitq->used_rsc);
		can_get = PS3_FALSE;
	}

	return can_get;
}

static void ps3_qos_pd_in_q(struct ps3_cmd *cmd, struct qos_wait_queue *waitq,
								U16 pd_idx) {
	ULong flag = 0;
	ps3_spin_lock_irqsave(waitq->rsc_lock, &flag);
	list_add_tail(&cmd->qos_list, &waitq->wait_list);
	waitq->count++;
	(*waitq->total_waited_cnt)++;
	cmd->qos_waitq_flag = PS3_QOS_CMD_IN_PD;
	cmd->first_over_quota_pd_idx = pd_idx;
	ps3_spin_unlock_irqrestore(waitq->rsc_lock, flag);
	PS3_QOS_STAT_START(cmd->instance, cmd, PS3_QOS_PD_QUEUE);
}

static struct qos_wait_queue* ps3_qos_pd_waitq_get(struct ps3_qos_pd_mgr *qos_pd_mgr,
				 							struct ps3_cmd *cmd)
{
	U16 que_id = 0;
	struct ps3_instance *instance = NULL;

	instance = cmd->instance;
	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		que_id = get_offset_of_vdid(PS3_VDID_OFFSET(instance),
						PS3_VDID(&cmd->io_attr.vd_entry->diskPos));
	} else {
		if (ps3_is_nvme_direct_cmd(qos_pd_mgr, cmd)) { 
			que_id = 0;
		} else {
			que_id = instance->qos_context.max_vd_count;
		}
	}

	return &qos_pd_mgr->waitqs[que_id];
}

static Bool ps3_qos_pd_quota_get(struct ps3_qos_pd_mgr *qos_pd_mgr,
				 struct ps3_cmd *cmd, U16 pd_idx, struct qos_wait_queue *waitq)
{
	Bool can_get = PS3_FALSE;

	can_get = ps3_qos_pd_quota_add(waitq);
	if (!can_get) {
		ps3_qos_pd_in_q(cmd, waitq, pd_idx);
		LOG_DEBUG("insert qos pd quota waitq.host_no:%u:t_id:0x%llx CFID:%u waitq[%u,%u] did[%u,%u]\n",
				PS3_HOST(cmd->instance), cmd->trace_id, cmd->index,
				waitq->id, waitq->count, qos_pd_mgr->disk_id, pd_idx);
	} else {
		cmd->target_pd[pd_idx].get_quota = PS3_TRUE;
	}

	return  can_get;
}

static bool ps3_qos_pd_quota_req(struct ps3_qos_pd_mgr *qos_pd_mgr,
				struct ps3_cmd *cmd, U16 pd_idx)
{
	bool can_get = PS3_FALSE;
	ULong flag = 0;
	struct qos_wait_queue *waitq = NULL;

	waitq = ps3_qos_pd_waitq_get(qos_pd_mgr, cmd);
	if (likely((*waitq->total_waited_cnt) == 0)) {
		can_get = ps3_qos_pd_quota_get(qos_pd_mgr, cmd, pd_idx, waitq);
	} else {
		INJECT_START(PS3_ERR_IJ_QOS_WAIT_PD_WAITQ_CLEAR, cmd->instance)
		ps3_spin_lock_irqsave(waitq->rsc_lock, &flag);
		if ((*waitq->total_waited_cnt) > 0) {
			list_add_tail(&cmd->qos_list, &waitq->wait_list);
			waitq->count++;
			(*waitq->total_waited_cnt)++;
			cmd->qos_waitq_flag = PS3_QOS_CMD_IN_PD;
			cmd->first_over_quota_pd_idx = pd_idx;
			ps3_spin_unlock_irqrestore(waitq->rsc_lock, flag);
			can_get = PS3_FALSE;
			LOG_DEBUG("insert qos pd quota waitq.host_no:%u:t_id:0x%llx CFID:%u waitq[%u,%u] did[%u,%u]\n",
				PS3_HOST(cmd->instance), cmd->trace_id, cmd->index,
				waitq->id, waitq->count, qos_pd_mgr->disk_id, pd_idx);
			PS3_QOS_STAT_START(cmd->instance, cmd, PS3_QOS_PD_QUEUE);
		} else {
			ps3_spin_unlock_irqrestore(waitq->rsc_lock, flag);
			can_get = ps3_qos_pd_quota_get(qos_pd_mgr, cmd, pd_idx, waitq);
		}
	}

	return can_get;
}

static bool ps3_qos_pd_quota_check(struct ps3_qos_pd_mgr *qos_pd_mgr,
				struct ps3_cmd *cmd)
{
	bool can_get = PS3_FALSE;
	ULong flag = 0;
	struct qos_wait_queue *waitq = NULL;

	waitq = ps3_qos_pd_waitq_get(qos_pd_mgr, cmd);
	if (likely((*waitq->total_waited_cnt) == 0)) {
		can_get = ps3_qos_pd_quota_add(waitq);
	} else {
		INJECT_START(PS3_ERR_IJ_QOS_WAIT_PD_WAITQ_CLEAR_2, cmd->instance)
		ps3_spin_lock_irqsave(waitq->rsc_lock, &flag);
		if ((*waitq->total_waited_cnt) > 0) {
			ps3_spin_unlock_irqrestore(waitq->rsc_lock, flag);
			can_get = PS3_FALSE;
		} else {
			ps3_spin_unlock_irqrestore(waitq->rsc_lock, flag);
			can_get = ps3_qos_pd_quota_add(waitq);
		}
	}

	return can_get;
}

static inline bool ps3_qos_vd_quota_dec(struct ps3_qos_vd_mgr *qos_vd_mgr)
{
	Bool can_get = PS3_FALSE;
	if (ps3_atomic_dec_return(&qos_vd_mgr->vd_quota) >= 0) {
		can_get  = PS3_TRUE;
	} else {
		ps3_atomic_inc(&qos_vd_mgr->vd_quota);
		can_get =  PS3_FALSE;
	}

	return  can_get;
}

static void ps3_qos_insert_vd_quota_waitq(struct ps3_qos_vd_mgr *qos_vd_mgr, struct ps3_cmd *cmd)
{
	ULong flag = 0;
	struct qos_wait_queue *wait_q = NULL;

	wait_q = &qos_vd_mgr->vd_quota_wait_q;
	ps3_spin_lock_irqsave(wait_q->rsc_lock, &flag);
	list_add_tail(&cmd->qos_list, &wait_q->wait_list);
	wait_q->count++;
	cmd->qos_waitq_flag = PS3_QOS_CMD_IN_VD;
	ps3_spin_unlock_irqrestore(wait_q->rsc_lock, flag);
	PS3_QOS_STAT_START(cmd->instance, cmd, PS3_QOS_VD_QUEUE);
	LOG_DEBUG("insert qos vd quota waitq.host_no:%u:tid:0x%llx CFID:%u diskid:%u waitq:%u\n",
		PS3_HOST(cmd->instance), cmd->trace_id, cmd->index,
		cmd->io_attr.disk_id, wait_q->count);
}

static bool ps3_qos_vd_quota_get_waitq(struct ps3_qos_vd_mgr *qos_vd_mgr, struct ps3_cmd *cmd)
{
	Bool can_get = PS3_FALSE;
	can_get = ps3_qos_vd_quota_dec(qos_vd_mgr);
	if (!can_get) {
		ps3_qos_insert_vd_quota_waitq(qos_vd_mgr, cmd);
	}

	return  can_get;
}

static bool ps3_qos_vd_quota_check(struct ps3_cmd *cmd)
{
	bool can_get = PS3_FALSE;
	ULong flag = 0;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
	struct qos_wait_queue *wait_q = NULL;

	qos_vd_mgr = ps3_qos_vd_mgr_get(cmd);
	wait_q = &qos_vd_mgr->vd_quota_wait_q;
	if (likely(wait_q->count == 0)){
		can_get = ps3_qos_vd_quota_get_waitq(qos_vd_mgr, cmd);
	} else {
		INJECT_START(PS3_ERR_IJ_QOS_WAIT_VD_WAITQ_CLEAR, cmd->instance)
		ps3_spin_lock_irqsave(wait_q->rsc_lock, &flag);
		if (wait_q->count > 0) {
			list_add_tail(&cmd->qos_list, &wait_q->wait_list);
			wait_q->count++;
			cmd->qos_waitq_flag = PS3_QOS_CMD_IN_VD;
			ps3_spin_unlock_irqrestore(wait_q->rsc_lock, flag);
			can_get = PS3_FALSE;
			LOG_DEBUG("insert qos vd quota waitq.host_no:%u:tid:0x%llx CFID:%u "
				"diskid:%u waitq:%u\n",
				PS3_HOST(cmd->instance), cmd->trace_id, cmd->index,
				cmd->io_attr.disk_id, wait_q->count);
			PS3_QOS_STAT_START(cmd->instance, cmd, PS3_QOS_VD_QUEUE);
		} else {
			ps3_spin_unlock_irqrestore(wait_q->rsc_lock, flag);
			can_get = ps3_qos_vd_quota_get_waitq(qos_vd_mgr, cmd);
		}
	}

	return can_get;
}

static inline Bool ps3_qos_pd_notify_judge(struct ps3_qos_pd_mgr *qos_pd_mgr)
{
	Bool can_notify = PS3_FALSE;

	can_notify = qos_pd_mgr->total_wait_cmd_cnt &&
		(ps3_atomic_read(&qos_pd_mgr->pd_used_quota) < qos_pd_mgr->pd_quota);
	if (PS3_QOS_PD_IS_VD_MEMBER(qos_pd_mgr) || can_notify) {
		goto _out;
	} else {
		can_notify = qos_pd_mgr->total_waited_direct_cmd &&
		(ps3_atomic_read(&qos_pd_mgr->direct_used_quota) < qos_pd_mgr->direct_quota);
	}
_out:
	INJECT_START(PS3_ERR_IJ_QOS_NOT_AWAKE_PD, &can_notify)
	return 	can_notify;
}

static Bool ps3_qos_single_pd_notify(struct ps3_qos_pd_context *qos_pd_ctx,
			 struct ps3_qos_pd_mgr *qos_pd_mgr)
{
	struct workqueue_struct *workq = NULL;
	Bool notified = PS3_FALSE;

	if (ps3_atomic_read(&qos_pd_mgr->valid) == 1) {
		if (ps3_qos_pd_notify_judge(qos_pd_mgr)) {
			workq = qos_pd_ctx->work_queues[qos_pd_mgr->workq_id];
			queue_work(workq, &qos_pd_mgr->resend_work);
			notified = PS3_TRUE;
		}
	}

	return notified;
}

static void ps3_qos_pd_notify(struct ps3_instance *instance)
{
	U16 id = 0;
	struct ps3_qos_pd_context *qos_pd_ctx = NULL;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;

	qos_pd_ctx = &instance->qos_context.pd_ctx;
	for (id = 1; id <= instance->qos_context.max_pd_count; id++) {
		qos_pd_mgr = ps3_qos_pd_mgr_get(instance, id);
		ps3_qos_single_pd_notify(qos_pd_ctx, qos_pd_mgr);
	}
}

static Bool ps3_qos_single_vd_notify(struct ps3_qos_vd_context *qos_vd_ctx,
			 struct ps3_qos_vd_mgr *qos_vd_mgr)
{
	struct workqueue_struct *workq = NULL;
	Bool notify = PS3_FALSE;

	if (qos_vd_mgr->valid) {
		if (qos_vd_mgr->vd_quota_wait_q.count > 0 &&
			  ps3_atomic_read(&qos_vd_mgr->vd_quota) > 0) {
			workq = qos_vd_ctx->work_queues[qos_vd_mgr->workq_id];
			queue_work(workq, &qos_vd_mgr->resend_work);
			notify = PS3_TRUE;
		}
	}

	return notify;
}

static U8 ps3_qos_vd_notify(struct ps3_instance *instance)
{
	U8 notify = PS3_FALSE;
	U16 id = 0;
	struct ps3_qos_vd_context *qos_vd_ctx = NULL;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;

	qos_vd_ctx = &instance->qos_context.vd_ctx;
	for (id = 1; id <= instance->qos_context.max_vd_count; id++) {
		qos_vd_mgr = &qos_vd_ctx->qos_vd_mgrs[id];
		if (ps3_qos_single_vd_notify(qos_vd_ctx, qos_vd_mgr)) {
			notify = PS3_TRUE;
		}
	}

	return notify;
}

static Bool ps3_qos_tag_rsc_available(struct ps3_instance *instance)
{
	Bool rsc_avail = PS3_FALSE;
	U16 i = 0;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
	struct ps3_qos_tg_context *qos_tg_ctx = &instance->qos_context.tg_ctx;
	if (ps3_atomic_read(&qos_tg_ctx->share_free_cnt) > 0) {
		rsc_avail = PS3_TRUE;
		goto _lout;
	}

	if (qos_tg_ctx->mgr_cmd_wait_q.count > 0 &&
		ps3_atomic_read(&qos_tg_ctx->mgr_free_cnt) > 0) {
		rsc_avail = PS3_TRUE;
		goto _lout;
	}

	for (i = 1; i <= instance->qos_context.max_vd_count; i++) {
		qos_vd_mgr = &instance->qos_context.vd_ctx.qos_vd_mgrs[i];
		if (ps3_atomic_read(&qos_vd_mgr->exclusive_cmd_cnt) > 0 &&
				qos_tg_ctx->vd_cmd_waitqs[i].count > 0) {
			rsc_avail = PS3_TRUE;
			break;
		}
	}

_lout:
	INJECT_START(PS3_ERR_IJ_QOS_NOT_AWAKE_TAG, &rsc_avail);
	return rsc_avail;
}

static Bool ps3_qos_tg_notify(struct ps3_instance *instance)
{
	Bool notified = PS3_FALSE;
	struct ps3_qos_tg_context *qos_tg_ctx = &instance->qos_context.tg_ctx;

	if (qos_tg_ctx->total_wait_cmd_cnt > 0 &&
			ps3_qos_tag_rsc_available(instance)) {
		queue_work(qos_tg_ctx->work_queue, &qos_tg_ctx->resend_work);
		notified = PS3_TRUE;
	}

	return notified;
}

Bool ps3_qos_all_pd_rc_get(struct ps3_cmd *cmd)
{
	cmd->target_pd[cmd->first_over_quota_pd_idx].get_quota = PS3_TRUE;
	PS3_QOS_STAT_END(cmd->instance, cmd, PS3_QOS_PD_QUEUE);
	LOG_DEBUG("resend cmd judge:host_no:%u t_id:0x%llx cmd:%u cmd_t:%u dev_t:%u ret[%u,%u]\n",
				PS3_HOST(cmd->instance), cmd->trace_id, cmd->index, cmd->cmd_word.type, cmd->io_attr.dev_type,
				cmd->first_over_quota_pd_idx, 1);
	return PS3_TRUE;
}

static void ps3_vd_quota_waitq_clean(		 struct ps3_qos_vd_mgr *qos_vd_mgr,
				struct ps3_scsi_priv_data *priv_data, S32 resp_status)
{
	ULong flag = 0;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *cmd_next = NULL;

	ps3_spin_lock_irqsave(qos_vd_mgr->vd_quota_wait_q.rsc_lock, &flag);
	if (qos_vd_mgr->vd_quota_wait_q.count > 0) {
		list_for_each_entry_safe(cmd, cmd_next, &qos_vd_mgr->vd_quota_wait_q.wait_list, qos_list) {
			if (priv_data == NULL ||
				priv_data == scsi_device_private_data(cmd->scmd)) {
				list_del(&cmd->qos_list);
				qos_vd_mgr->vd_quota_wait_q.count--;
				LOG_DEBUG("qos clean vd quota waitq. hno:%u t_id:0x%llx cmd:%d vd_id:%u\n",
					PS3_HOST(qos_vd_mgr->instance), cmd->trace_id, cmd->index, qos_vd_mgr->id);
				ps3_qos_update_pd_quota(cmd);
				ps3_scsih_drv_io_reply_scsi(cmd->scmd, cmd, resp_status, PS3_FALSE);
			}
		}
	}
	ps3_spin_unlock_irqrestore(qos_vd_mgr->vd_quota_wait_q.rsc_lock, flag);
	LOG_FILE_INFO("clean qos device vd quota waitq host_no:%u diskid:%u\n",
		PS3_HOST(qos_vd_mgr->instance), qos_vd_mgr->id);
}

static void ps3_qos_vd_quota_waitq_resend(struct ps3_qos_vd_mgr *qos_vd_mgr)
{
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *cmd_next = NULL;
	ULong flag = 0;
	S32 ret = PS3_SUCCESS;
	Bool waitq_cleared = PS3_FALSE;
	struct ps3_instance *instance = qos_vd_mgr->instance;

	ps3_spin_lock_irqsave(qos_vd_mgr->vd_quota_wait_q.rsc_lock, &flag);
	if (qos_vd_mgr->vd_quota_wait_q.count > 0
			&& ps3_atomic_read(&qos_vd_mgr->vd_quota) > 0) {
		list_for_each_entry_safe(cmd, cmd_next, &qos_vd_mgr->vd_quota_wait_q.wait_list, qos_list) {
			if (ps3_atomic_dec_return(&qos_vd_mgr->vd_quota) < 0) {
				ps3_atomic_inc(&qos_vd_mgr->vd_quota);
				break;
			}
			list_del(&cmd->qos_list);
			qos_vd_mgr->vd_quota_wait_q.count--;
			PS3_QOS_STAT_END(instance, cmd, PS3_QOS_VD_QUEUE);
			if (ps3_qos_tg_decision(cmd)) {
				ret = ps3_scsi_cmd_send(instance, cmd);
				if (unlikely(ret != PS3_SUCCESS)) {
					ps3_qos_cmd_update(instance, cmd);
					ps3_qos_cmd_resend_fail(cmd, ret);
				} else {
					cmd->qos_waitq_flag = 0;
					LOG_DEBUG("qos vd quota waitq resend:host_no:%u vid:%u t_id:0x%llx CFID:%u type:%u\n",
						PS3_HOST(instance), qos_vd_mgr->id, cmd->trace_id, cmd->index, cmd->cmd_word.type);
				}
			}
		}

		if (qos_vd_mgr->vd_quota_wait_q.count == 0) {
			waitq_cleared = PS3_TRUE;
		}
	}
	ps3_spin_unlock_irqrestore(qos_vd_mgr->vd_quota_wait_q.rsc_lock, flag);
	if (waitq_cleared) {
		LOG_DEBUG("qos vd quota waitq cleared.host_no:%u vd:%u\n",
				   	PS3_HOST(instance), qos_vd_mgr->id);
	}

	qos_vd_mgr->last_sched_jiffies = jiffies;
	ps3_qos_pd_notify(instance);
}

static void ps3_qos_vd_resend_work(struct work_struct *work)
{
	struct ps3_qos_vd_mgr *qos_vd_mgr =
			ps3_container_of(work, struct ps3_qos_vd_mgr, resend_work);
	ps3_qos_vd_quota_waitq_resend(qos_vd_mgr);
}

static void ps3_qos_waitq_clean(struct ps3_qos_pd_mgr *qos_pd_mgr,
			 struct qos_wait_queue *waitq, S32 resp_status)
{
	ULong flag = 0;
	struct ps3_cmd *cmd = NULL;
	ps3_spin_lock_irqsave(waitq->rsc_lock, &flag);
	while (waitq->count > 0) {
		cmd = list_first_entry(&waitq->wait_list, struct ps3_cmd, qos_list);
		LOG_DEBUG("qos clean pd waitq. hno:%u t_id:0x%llx CFID:%d pid:%u que_id:%u\n",
			PS3_HOST(qos_pd_mgr->instance), cmd->trace_id, cmd->index,
			qos_pd_mgr->disk_id, waitq->id);
		list_del(&cmd->qos_list);
		waitq->count--;
		(*waitq->total_waited_cnt)--;
		ps3_scsih_drv_io_reply_scsi(cmd->scmd, cmd, resp_status, PS3_FALSE);
	}
	ps3_spin_unlock_irqrestore(waitq->rsc_lock, flag);
}

void ps3_pd_quota_waitq_clear_all(struct ps3_qos_pd_mgr *qos_pd_mgr,
			S32 resp_status)
{
	U16 i = 0;
	struct qos_wait_queue *waitq = NULL;

	for (i = 0; i < qos_pd_mgr->waitq_cnt; i++) {
		waitq = &qos_pd_mgr->waitqs[i];
		ps3_qos_waitq_clean(qos_pd_mgr, waitq, resp_status);
	}

	LOG_DEBUG("clear all qos pd quota waitq host_no:%u did:%u vid:%u\n",
		PS3_HOST(qos_pd_mgr->instance), qos_pd_mgr->disk_id, qos_pd_mgr->vd_id);
}

void ps3_pd_quota_waitq_clean(struct ps3_qos_pd_mgr *qos_pd_mgr,
			U16 que_id,  S32 resp_status)
{
	struct qos_wait_queue *waitq = NULL;
	if (que_id == 0) { 
		ps3_pd_quota_waitq_clear_all(qos_pd_mgr, resp_status);
	} else {
		waitq = &qos_pd_mgr->waitqs[que_id]; 
		ps3_qos_waitq_clean(qos_pd_mgr, waitq, resp_status);
	}

	LOG_DEBUG("clean qos pd quota waitq host_no:%u did:%u vid:%u\n",
		 PS3_HOST(qos_pd_mgr->instance), qos_pd_mgr->disk_id, qos_pd_mgr->vd_id);
}

static Bool ps3_hba_qos_pd_resend_check(struct ps3_cmd *cmd)
{
	Bool can_get = PS3_FALSE;

	if (cmd->io_attr.dev_type != PS3_DEV_TYPE_VD) {
		if (!ps3_qos_vd_quota_check(cmd)) {
			goto _out;
		}
	}
	can_get = ps3_qos_tg_decision(cmd);
_out:
	return can_get;
}

static Bool ps3_qos_pd_resend_check(struct ps3_instance *instance, struct ps3_cmd *cmd)
{
	Bool can_get = PS3_FALSE;

	if (cmd->io_attr.direct_flag == PS3_CMDWORD_DIRECT_OK) {
		cmd->target_pd[cmd->first_over_quota_pd_idx].get_quota = PS3_TRUE;
		PS3_QOS_STAT_END(cmd->instance, cmd, PS3_QOS_PD_QUEUE);
		can_get = PS3_TRUE;
	} else {
		if (!ps3_qos_all_pd_rc_get(cmd)) {
			goto _out;
		}
		can_get = instance->qos_context.opts.qos_pd_resend_check(cmd);
	}
_out:
	return can_get;
}

static Bool ps3_qos_vd_seq_check(struct ps3_cmd *cmd)
{
	struct PS3VDEntry *vd_entry = NULL;
	U16 vd_id = 0;
	Bool seq_changed = PS3_FALSE;

	INJECT_START(PS3_ERR_IJ_QOS_FORCE_VD_SEQ_CHANGE, cmd);
	if (cmd->cmd_word.direct == PS3_CMDWORD_DIRECT_ADVICE) {
		vd_id = PS3_VDID(&cmd->io_attr.vd_entry->diskPos);
		vd_entry = ps3_dev_mgr_lookup_vd_info_by_id(cmd->instance, vd_id);
		if (!vd_entry || vd_entry->virtDiskSeq !=
				cmd->req_frame->hwReq.reqHead.virtDiskSeq) {
			seq_changed = PS3_TRUE;
			LOG_DEBUG("qos pd waitq vd seq check:host_no:%u"
				"t_id:0x%llx CFID:%u seq[%u %u]\n",
				PS3_HOST(cmd->instance), cmd->trace_id, cmd->index,
				 cmd->req_frame->hwReq.reqHead.virtDiskSeq,
				(!vd_entry) ? 0 : vd_entry->virtDiskSeq);
			ps3_qos_update_pd_quota(cmd);
			ps3_scsih_drv_io_reply_scsi(cmd->scmd, cmd,
				 SCSI_STATUS_TASK_ABORTED, PS3_FALSE);
		}
	}

	return seq_changed;
}

static U32 ps3_qos_pd_resend_cmd(struct ps3_qos_pd_mgr *qos_pd_mgr,
			struct qos_wait_queue *waitq, S32 count)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	struct ps3_instance *instance = qos_pd_mgr->instance;
	U32 can_send = 0;
	U32 que_cnt = 0;
	S32 avail_rsc = 0;
	S32 used_rsc = ps3_atomic_read(waitq->used_rsc);

	if (used_rsc < *waitq->free_rsc) {
		avail_rsc = *waitq->free_rsc - used_rsc;
	}
	can_send = PS3_MIN(avail_rsc, count);
	que_cnt = waitq->count;
	while(can_send > 0 && waitq->count > 0) {
		cmd = list_first_entry(&waitq->wait_list, struct ps3_cmd, qos_list);
		list_del(&cmd->qos_list);
		waitq->count--;
		(*waitq->total_waited_cnt)--;
		if (ps3_qos_vd_seq_check(cmd)) {
			continue;
		}
		can_send--;
		ps3_atomic_inc(waitq->used_rsc);
		if (ps3_qos_pd_resend_check(instance, cmd)) {
			ret = ps3_scsi_cmd_send(instance, cmd);
			if (unlikely(ret != PS3_SUCCESS)) {
				ps3_qos_cmd_update(instance, cmd);
				ps3_qos_cmd_resend_fail(cmd, ret);
			} else {
				cmd->qos_waitq_flag = 0;
				LOG_DEBUG("qos pd quota waitq resend:host_no:%u qid:%u pid:%u"
					"t_id:0x%llx CFID:%u resend[%u %u]\n",
					PS3_HOST(instance), waitq->id, qos_pd_mgr->disk_id,
					cmd->trace_id, cmd->index, waitq->has_resend, waitq->can_resend);
			}
		}
	}

	return (que_cnt - waitq->count);
}

static void ps3_qos_pd_waitq_resend(struct ps3_qos_pd_mgr *qos_pd_mgr,
									struct qos_wait_queue *waitq )
{
	ULong flag = 0;
	Bool waitq_cleared = PS3_FALSE;
	if (waitq->count > 0) {
		ps3_spin_lock_irqsave(waitq->rsc_lock, &flag);
		if (ps3_atomic_read(waitq->used_rsc) < *waitq->free_rsc) {
			ps3_qos_pd_resend_cmd(qos_pd_mgr, waitq, waitq->count);
		}

		if (*waitq->total_waited_cnt == 0) {
			waitq_cleared = PS3_TRUE;
		}
		ps3_spin_unlock_irqrestore(waitq->rsc_lock, flag);

		if (waitq_cleared) {
			LOG_DEBUG("qos pd quota waitq is cleard. host_no:%u did:%u qid:%u\n",
				 PS3_HOST(qos_pd_mgr->instance), qos_pd_mgr->disk_id, waitq->id);
		}
	}
}

static void ps3_qos_pd_jbod_resend(struct ps3_qos_pd_mgr *qos_pd_mgr)
{
	struct ps3_instance *instance = qos_pd_mgr->instance;
	struct qos_wait_queue *waitq = NULL;
	U16 que_id = 0;

	if (qos_pd_mgr->dev_type == PS3_DEV_TYPE_NVME_SSD) {
		waitq = &qos_pd_mgr->waitqs[0]; 
		ps3_qos_pd_waitq_resend(qos_pd_mgr, waitq);
	}

	que_id = instance->qos_context.max_vd_count;
	waitq = &qos_pd_mgr->waitqs[que_id]; 
	ps3_qos_pd_waitq_resend(qos_pd_mgr, waitq);
}

void ps3_qos_pd_waitq_ratio_update(struct ps3_qos_pd_mgr *qos_pd_mgr)
{
	U16 min_waitq_cnt = 0;
	U16 i = 0;
	struct qos_wait_queue *waitq = NULL;
	ULong min_cmd_jiffies = 0;
	struct ps3_cmd *cmd = NULL;
	U16 poll_que_id = 1;

	if (qos_pd_mgr->poll_que_id != qos_pd_mgr->poll_start_que_id) {
		return;
	}

	waitq = &qos_pd_mgr->waitqs[qos_pd_mgr->poll_que_id];
	if (waitq->has_resend > 0) {
		return;
	}

	for (i = 1; i < qos_pd_mgr->waitq_cnt - 1; i++) {
		waitq = &qos_pd_mgr->waitqs[i];
		if (waitq->count > 0) {
			cmd = list_first_entry(&waitq->wait_list, struct ps3_cmd, qos_list);
			if (min_waitq_cnt == 0) {
				min_waitq_cnt = waitq->count;
				min_cmd_jiffies = cmd->scmd->jiffies_at_alloc;
			} else {
				if (min_waitq_cnt > waitq->count) {
					min_waitq_cnt = waitq->count;
				}

				if (min_cmd_jiffies > cmd->scmd->jiffies_at_alloc) {
					min_cmd_jiffies = cmd->scmd->jiffies_at_alloc;
					poll_que_id = i;
				}
			}
		}
	}

	for (i = 1; i < qos_pd_mgr->waitq_cnt - 1; i++) {
		waitq = &qos_pd_mgr->waitqs[i];
		if (waitq->count > 0) {
			waitq->can_resend = waitq->count/min_waitq_cnt > 0 ?
						waitq->count/min_waitq_cnt : 1;
			waitq->has_resend = 0;
		}
	}

	qos_pd_mgr->poll_que_id = poll_que_id;
	qos_pd_mgr->poll_start_que_id = poll_que_id;

	LOG_DEBUG("qos pd ratio update:host_no:%u pid:%u first:%u\n",
		PS3_HOST(qos_pd_mgr->instance), qos_pd_mgr->disk_id, poll_que_id);
}

static void ps3_qos_pd_vd_member_resend(struct ps3_qos_pd_mgr *qos_pd_mgr)
{
	ULong flag = 0;
	struct ps3_instance *instance = qos_pd_mgr->instance;
	struct qos_wait_queue *waitq = NULL;
	ULong timeout_jiffies = 0;
	Bool waitq_cleared = PS3_FALSE;
	Bool vd_expired = PS3_FALSE;
	U32 pd_wait_count = 0;

	ps3_spin_lock_irqsave(&qos_pd_mgr->rc_lock, &flag);
	pd_wait_count = qos_pd_mgr->waitqs[instance->qos_context.max_vd_count].count;
	while ((ps3_atomic_read(&qos_pd_mgr->pd_used_quota) <
			qos_pd_mgr->pd_quota) && qos_pd_mgr->total_wait_cmd_cnt > pd_wait_count) {
		ps3_qos_pd_waitq_ratio_update(qos_pd_mgr);
		waitq = &qos_pd_mgr->waitqs[qos_pd_mgr->poll_que_id];
		if (waitq->count == 0) {
			goto _next_waitq;
		}

		if (waitq->has_resend > 0) {
			if (qos_pd_mgr->dev_type == PS3_DEV_TYPE_SAS_HDD ||
					qos_pd_mgr->dev_type == PS3_DEV_TYPE_SATA_HDD) {
				timeout_jiffies = waitq->last_sched_jiffies +
						 msecs_to_jiffies(PS3_QOS_VD_HDD_DELAY_THD_MS);
			} else {
				timeout_jiffies = waitq->last_sched_jiffies +
						 msecs_to_jiffies(PS3_QOS_VD_SDD_DELAY_THD_MS);
			}

			vd_expired = time_after(jiffies, timeout_jiffies);
			INJECT_START(PS3_ERR_IJ_QOS_PD_RESEND_NOT_EXPIRED, &vd_expired)
			if (vd_expired) {
				LOG_DEBUG("go to next vd waitq for delay:host_no:%u qid:%u pid:%u",
					PS3_HOST(instance), waitq->id, qos_pd_mgr->disk_id);
				goto _next_waitq;
			}
		}

		waitq->last_sched_jiffies = jiffies;
		waitq->has_resend += ps3_qos_pd_resend_cmd(qos_pd_mgr, waitq, waitq->can_resend - waitq->has_resend);
		if (waitq->has_resend >= waitq->can_resend) {
			goto _next_waitq;
		}
		continue;
_next_waitq:
		waitq->has_resend = 0; 
		if (++qos_pd_mgr->poll_que_id == instance->qos_context.max_vd_count) {
			qos_pd_mgr->poll_que_id = 1;
		}
	}

	if (qos_pd_mgr->total_wait_cmd_cnt == 0) {
		waitq_cleared = PS3_TRUE;
	}
	ps3_spin_unlock_irqrestore(&qos_pd_mgr->rc_lock, flag);

	if (waitq_cleared) {
		LOG_DEBUG("qos pd quota waitq is cleard. host_no:%u diskid:%u\n",
			 PS3_HOST(instance), qos_pd_mgr->disk_id);
	}
}

static void ps3_qos_pd_resend_work(struct work_struct *work)
{
	struct ps3_qos_pd_mgr *qos_pd_mgr =
		ps3_container_of(work, struct ps3_qos_pd_mgr, resend_work);

	ps3_qos_pd_jbod_resend(qos_pd_mgr);
	INJECT_START(PS3_ERR_IJ_QOS_WAIT_VD_SEND, qos_pd_mgr)
	ps3_qos_pd_vd_member_resend(qos_pd_mgr);
	qos_pd_mgr->last_sched_jiffies = jiffies;

	return;
}

static void ps3_poll_vd_cmd_waitq(struct ps3_qos_tg_context *qos_tg_ctx)
{
	struct qos_wait_queue *waitq = NULL;
	U32 free_cnt =  0;
	U8 vd_id = 0;
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	U32 sent_count = 0;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
	struct ps3_instance *instance = qos_tg_ctx->instance;
	U32 mgrq_cnt = qos_tg_ctx->mgr_cmd_wait_q.count;

	INJECT_START(PS3_ERR_IJ_QOS_SET_SHARE_COUNT, instance)
	free_cnt = ps3_atomic_read(&qos_tg_ctx->share_free_cnt);
	vd_id = qos_tg_ctx->poll_vd_id;
	while (free_cnt > sent_count && qos_tg_ctx->total_wait_cmd_cnt > mgrq_cnt) {
		waitq = &qos_tg_ctx->vd_cmd_waitqs[vd_id];
		if (waitq->count > 0) {
			qos_vd_mgr = &instance->qos_context.vd_ctx.qos_vd_mgrs[vd_id];
			cmd = list_first_entry(&waitq->wait_list, struct ps3_cmd, qos_list);
			list_del(&cmd->qos_list);
			cmd->qos_waitq_flag = 0;
			waitq->count--;
			qos_tg_ctx->total_wait_cmd_cnt--;
			PS3_QOS_STAT_END(instance, cmd, PS3_QOS_TAG_QUEUE);
			if (ps3_qos_vd_seq_check(cmd)) {
				continue;
			}
			ret = ps3_scsi_cmd_send(instance, cmd);
			if (unlikely(ret != PS3_SUCCESS)) {
				ps3_qos_update_vd_quota(cmd);
				ps3_qos_update_pd_quota(cmd);
				ps3_qos_cmd_resend_fail(cmd, ret);
			} else {
				ps3_atomic_inc(&qos_vd_mgr->share_cmd_used);
				++sent_count;
				LOG_DEBUG("qos cmd waitq resend:host_no:%u t_id:0x%llx CFID:%u type:%u\n",
					 PS3_HOST(instance), cmd->trace_id, cmd->index, cmd->cmd_word.type);
			}
		}

		if (++vd_id > instance->qos_context.max_vd_count) {
			vd_id = 1;
		}
	}
	ps3_atomic_sub(sent_count, &qos_tg_ctx->share_free_cnt);
	LOG_DEBUG("resend vd cmd waitq. host_no:%u poll_vd_id:%u sent:%u\n",
			PS3_HOST(instance), vd_id, sent_count);

	qos_tg_ctx->poll_vd_id = vd_id;
}

static void ps3_mgr_cmd_waitq_resend(struct ps3_qos_tg_context *qos_tg_ctx)
{
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *cmd_next = NULL;
	struct qos_wait_queue *wait_q = NULL;
	struct ps3_instance *instance = qos_tg_ctx->instance;
	S32 ret = PS3_SUCCESS;
	U32 free_cnt = ps3_atomic_read(&qos_tg_ctx->share_free_cnt);
	U32 sent_count = 0;

	wait_q = &qos_tg_ctx->mgr_cmd_wait_q;
	if (wait_q->count > 0 && free_cnt > 0) {
		list_for_each_entry_safe(cmd, cmd_next, &wait_q->wait_list, qos_list) {
			list_del(&cmd->qos_list);
			cmd->qos_waitq_flag = 0;
			wait_q->count--;
			qos_tg_ctx->total_wait_cmd_cnt--;
			PS3_QOS_STAT_END(instance, cmd, PS3_QOS_TAG_QUEUE);
			ret = ps3_scsi_cmd_send(instance, cmd);
			if (unlikely(ret != PS3_SUCCESS)) {
				ps3_qos_cmd_resend_fail(cmd, ret);
			} else {
				LOG_DEBUG("qos cmd waitq resend:host_no:%u t_id:0x%llx CFID:%u type:%u\n",
					PS3_HOST(instance), cmd->trace_id, cmd->index, cmd->cmd_word.type);
				ps3_atomic_inc(&qos_tg_ctx->mgr_share_used);
				if(++sent_count == free_cnt) {
					break;
				}
			}
		}

		ps3_atomic_sub(sent_count, &qos_tg_ctx->share_free_cnt);
	}
}

static void ps3_qos_tg_share_resend(struct ps3_qos_tg_context *qos_tg_ctx)
{
	ps3_mgr_cmd_waitq_resend(qos_tg_ctx);
	ps3_poll_vd_cmd_waitq(qos_tg_ctx);
}

static void ps3_qos_tg_exclusive_resend(struct ps3_qos_tg_context *qos_tg_ctx)
{
   struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
   struct qos_wait_queue *waitq = NULL;
   struct ps3_cmd *cmd = NULL;
   struct ps3_cmd *cmd_next = NULL;
   S32 ret = PS3_SUCCESS;
   U8 id = 0;
   struct ps3_instance *instance = qos_tg_ctx->instance;

   waitq =  &qos_tg_ctx->mgr_cmd_wait_q;
   if (waitq->count > 0 &&
		ps3_atomic_read(&qos_tg_ctx->mgr_free_cnt) > 0) {
		list_for_each_entry_safe(cmd, cmd_next, &waitq->wait_list, qos_list) {
			if (ps3_atomic_dec_return(&qos_tg_ctx->mgr_free_cnt) < 0) {
				ps3_atomic_inc(&qos_tg_ctx->mgr_free_cnt);
				break;
			}
			list_del(&cmd->qos_list);
			cmd->qos_waitq_flag = 0;
			waitq->count--;
			qos_tg_ctx->total_wait_cmd_cnt--;
			PS3_QOS_STAT_END(instance, cmd, PS3_QOS_TAG_QUEUE);
			ret = ps3_scsi_cmd_send(instance, cmd);
			if (unlikely(ret != PS3_SUCCESS)) {
				ps3_qos_cmd_resend_fail(cmd, ret);
				ps3_atomic_inc(&qos_tg_ctx->mgr_free_cnt);
			} else {
				LOG_DEBUG("qos cmd waitq resend:host_no:%u t_id:0x%llx CFID:%u type:%u\n",
					 PS3_HOST(instance), cmd->trace_id, cmd->index, cmd->cmd_word.type);
			}
		}
	}

	for (id = 1; id <= instance->qos_context.max_vd_count; id++) {
		waitq = &qos_tg_ctx->vd_cmd_waitqs[id];
		qos_vd_mgr = &instance->qos_context.vd_ctx.qos_vd_mgrs[id];
		if (waitq->count > 0 &&
			ps3_atomic_read(&qos_vd_mgr->exclusive_cmd_cnt) > 0) {
			list_for_each_entry_safe(cmd, cmd_next, &waitq->wait_list, qos_list) {
				if (ps3_atomic_dec_return(&qos_vd_mgr->exclusive_cmd_cnt) < 0) {
					ps3_atomic_inc(&qos_vd_mgr->exclusive_cmd_cnt);
					break;
				}
				list_del(&cmd->qos_list);
				cmd->qos_waitq_flag = 0;
				waitq->count--;
				qos_tg_ctx->total_wait_cmd_cnt--;
				PS3_QOS_STAT_END(instance, cmd, PS3_QOS_TAG_QUEUE);
				if (ps3_qos_vd_seq_check(cmd)) {
					ps3_atomic_inc(&qos_vd_mgr->exclusive_cmd_cnt);
					continue;
				}
				ret = ps3_scsi_cmd_send(instance, cmd);
				if (unlikely(ret != PS3_SUCCESS)) {
					ps3_qos_update_vd_quota(cmd);
					ps3_qos_update_pd_quota(cmd);
					ps3_qos_cmd_resend_fail(cmd, ret);
					ps3_atomic_inc(&qos_vd_mgr->exclusive_cmd_cnt);
				} else {
					LOG_DEBUG("qos cmd waitq resend:host_no:%u t_id:0x%llx CFID:%u type:%u\n",
						 PS3_HOST(instance), cmd->trace_id, cmd->index, cmd->cmd_word.type);
				}
			}
		}
	}
}

static void ps3_qos_tg_waitq_resend(struct ps3_qos_tg_context *qos_tg_ctx)
{
	ULong flag = 0;
	Bool waitq_cleard = PS3_FALSE;
	struct ps3_instance *instance = qos_tg_ctx->instance;

	ps3_spin_lock_irqsave(&qos_tg_ctx->lock, &flag);
	ps3_qos_tg_share_resend(qos_tg_ctx);
	ps3_qos_tg_exclusive_resend(qos_tg_ctx);

	if (qos_tg_ctx->total_wait_cmd_cnt == 0) {
		waitq_cleard = PS3_TRUE;
	}
	ps3_spin_unlock_irqrestore(&qos_tg_ctx->lock, flag);
	if (waitq_cleard) {
		LOG_DEBUG("hno:%u resend all tag waited cmds\n",
			PS3_HOST(instance));
	}

	if (!ps3_qos_vd_notify(instance)) {
		ps3_qos_pd_notify(instance);
	}
	qos_tg_ctx->last_sched_jiffies = jiffies;
}

static void ps3_qos_tg_resend_work(struct work_struct *work)
{
	struct ps3_qos_tg_context *qos_tg_ctx =
			ps3_container_of(work, struct ps3_qos_tg_context, resend_work);
	ps3_qos_tg_waitq_resend(qos_tg_ctx);
}

U32 g_ps3_qos_cmdq_depth = PS3_QOS_CMDQ_DEPTH;
U32 g_ps3_qos_mgrq_depth = PS3_QOS_MGRQ_DEPTH;
U32 g_ps3_qos_hdd_pd_quota = PS3_QOS_DEFAULT_PD_QUOTA;
U32 g_ps3_qos_ssd_pd_quota = PS3_QOS_DEFAULT_PD_QUOTA;
U32 g_ps3_qos_sas_pd_quota = PS3_QOS_SAS_PD_QUOTA;
U32 g_ps3_qos_nvme_pd_quota = PS3_QOS_NVME_DIRECT_QUOTA;
U32 g_ps3_qos_nvme_member_quota = PS3_QOS_NVME_MEMBER_QUOTA;
U32 g_ps3_qos_hba_nvme_normal_quota = PS3_QOS_HBA_NVME_NORMAL_QUOTA;
U32 g_ps3_qos_raid_nvme_normal_quota = PS3_QOS_RAID_NVME_NORMAL_QUOTA;
U32 g_ps3_qos_vd_quota = PS3_QOS_FUNC1_JBOD_VD_QUOTA;
U32 g_ps3_qos_max_cmds = PS3_QOS_HBA_MAX_CMD;
U32 g_ps3_qos_jbod_exclusive = PS3_QOS_JBOD_EXCLUSIVE_CMD_COUNT;
U32 g_ps3_qos_vd_exclusive = PS3_QOS_VD_EXCLUSIVE_CMD_COUNT;
U32 g_ps3_qos_mgr_exclusive = QOS_MGR_EXCLUSIVE_CMD_COUNT;

#define PS3_QOS_MAX_WORKQ_NAME_LENGTH 32
static S32 ps3_qos_pd_context_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U16 i = 0;
	U16 j = 0;
	U16 k = 0;
	struct ps3_qos_pd_context *qos_pd_ctx = NULL;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	char workq_name[PS3_QOS_MAX_WORKQ_NAME_LENGTH] = {0};
	U16 max_pd_count = 0;
	struct qos_wait_queue *waitq = NULL;
	U32 cpu_num = 0;

	qos_pd_ctx = &instance->qos_context.pd_ctx;
	if (ps3_get_pci_function(instance->pdev) == PS3_FUNC_ID_1) {
		cpu_num = num_online_cpus();
		if (instance->irq_context.is_balance_current_perf_mode) {
			INJECT_START(PS3_ERR_IJ_SET_HIGH_IOPS_CHANNEL_IN_QOS, instance);
			if (cpu_num > instance->irq_context.high_iops_msix_vectors
								 + PS3_QOS_FUNC1_PD_WORKQ_COUNT) {
				qos_pd_ctx->workq_count = 
						cpu_num - instance->irq_context.high_iops_msix_vectors;
			} else {
				qos_pd_ctx->workq_count = PS3_QOS_FUNC1_PD_WORKQ_COUNT;
			}
		} else {
			qos_pd_ctx->workq_count = PS3_QOS_FUNC1_PD_WORKQ_COUNT;
		}
	} else {
		qos_pd_ctx->workq_count = PS3_QOS_FUNC0_PD_WORKQ_COUNT;
	}
	
	max_pd_count = instance->qos_context.max_pd_count;
	qos_pd_ctx->qos_pd_mgrs = (struct ps3_qos_pd_mgr *)ps3_vzalloc(instance,
								(max_pd_count + 1) * sizeof(struct ps3_qos_pd_mgr));
	INJECT_START(PS3_ERR_IJ_QOS_PD_INIT_FAIL_2, instance)
	if (qos_pd_ctx->qos_pd_mgrs == NULL ) {
		LOG_ERROR("hno:%u:Failed to kcalloc memory for qos_pd_mgrs\n",
			  PS3_HOST(instance));
		ret =  -PS3_FAILED;
		goto _out;
	}
	memset(qos_pd_ctx->qos_pd_mgrs, 0, sizeof(struct ps3_qos_pd_mgr) * (max_pd_count + 1));
	for (i = 1; i <= max_pd_count; i++) {
		qos_pd_mgr = &qos_pd_ctx->qos_pd_mgrs[i];
		INIT_WORK(&qos_pd_mgr->resend_work, ps3_qos_pd_resend_work);
		ps3_spin_lock_init(&qos_pd_mgr->rc_lock);
		ps3_spin_lock_init(&qos_pd_mgr->direct_rsc_lock);
		ps3_spin_lock_init(&qos_pd_mgr->adjust_quota_lock);

		qos_pd_mgr->workq_id = 0;
		qos_pd_mgr->clearing = PS3_FALSE;
		qos_pd_mgr->poll_cmd_cnt = 1;
		qos_pd_mgr->poll_que_id = 1;
		qos_pd_mgr->poll_start_que_id = 1;
		ps3_atomic_set(&qos_pd_mgr->processing_cnt, 0);
		qos_pd_mgr->pd_init_quota = 0;
		qos_pd_mgr->instance = instance;
		qos_pd_mgr->total_wait_cmd_cnt = 0;
		qos_pd_mgr->total_waited_direct_cmd = 0;
		qos_pd_mgr->waitq_cnt =  instance->qos_context.max_vd_count + 1;
		qos_pd_mgr->waitqs = (struct qos_wait_queue *)ps3_vzalloc(instance,
							qos_pd_mgr->waitq_cnt * sizeof(struct qos_wait_queue));
		INJECT_START(PS3_ERR_IJ_QOS_PD_INIT_FAIL_3, qos_pd_ctx)
		if (qos_pd_mgr->waitqs == NULL) {
			LOG_ERROR("hno:%u:Failed to kcalloc memory for pd waitqs\n",
				  PS3_HOST(instance));
			ret =  -PS3_FAILED;
			goto _release_pd_waitq;
		}

		for (j = 0; j < qos_pd_mgr->waitq_cnt; j++) {
			waitq = &qos_pd_mgr->waitqs[j];
			waitq->count = 0;
			waitq->id = j;
			waitq->can_resend = 0;
			waitq->has_resend = 0;
			INIT_LIST_HEAD(&waitq->wait_list);
			if (j == 0) { 
				waitq->free_rsc = &qos_pd_mgr->direct_quota;
				waitq->used_rsc = &qos_pd_mgr->direct_used_quota;
				waitq->rsc_lock = &qos_pd_mgr->direct_rsc_lock;
				waitq->total_waited_cnt = &qos_pd_mgr->total_waited_direct_cmd;
			} else {
				waitq->free_rsc = &qos_pd_mgr->pd_quota;
				waitq->used_rsc = &qos_pd_mgr->pd_used_quota;
				waitq->rsc_lock = &qos_pd_mgr->rc_lock;
				waitq->total_waited_cnt = &qos_pd_mgr->total_wait_cmd_cnt;
			}
		}
	}

	qos_pd_ctx->work_queues = (struct workqueue_struct **)ps3_vzalloc(instance,
								(qos_pd_ctx->workq_count) * sizeof(struct workqueue_struct *));
	INJECT_START(PS3_ERR_IJ_QOS_PD_INIT_FAIL_4, instance)
	if (qos_pd_ctx->work_queues == NULL) {
		LOG_ERROR("hno:%u:Failed to kcalloc memory for work_queues\n",
			  PS3_HOST(instance));
		ret =  -PS3_FAILED;
		goto _release_pd_mgr;
	}
	memset(qos_pd_ctx->work_queues, 0, (qos_pd_ctx->workq_count) * sizeof(struct workqueue_struct *));
	for (j = 0; j < qos_pd_ctx->workq_count; j++) {
		snprintf(workq_name, PS3_QOS_MAX_WORKQ_NAME_LENGTH, "pd_wq_f%u_%u",
					ps3_get_pci_function(instance->pdev), j);
		qos_pd_ctx->work_queues[j] = create_singlethread_workqueue(workq_name);
		INJECT_START(PS3_ERR_IJ_QOS_PD_INIT_FAIL_1, qos_pd_ctx)
		if (qos_pd_ctx->work_queues[j] == NULL) {
			LOG_ERROR("qos pd workq:%u create failed\n", j);
			ret = -PS3_FAILED;
			goto  _destroy_created_workqueue;
		}
	}

	LOG_INFO("hno:%u: qos pd context init success\n", PS3_HOST(instance));
	goto _out;
_destroy_created_workqueue:
	for (k = 0; k < j; k++) {
		destroy_workqueue(qos_pd_ctx->work_queues[k]);
		qos_pd_ctx->work_queues[k] = NULL;
	}
	ps3_vfree(instance, qos_pd_ctx->work_queues);
	qos_pd_ctx->work_queues = NULL;
_release_pd_waitq:
	for (k = 1;  k < i; k++) {
		qos_pd_mgr = &qos_pd_ctx->qos_pd_mgrs[k];
		if (qos_pd_mgr->waitqs) {
			ps3_vfree(instance, qos_pd_mgr->waitqs);
			qos_pd_mgr->waitqs = NULL;
		}
	}
_release_pd_mgr:
	ps3_vfree(instance, qos_pd_ctx->qos_pd_mgrs);
	qos_pd_ctx->qos_pd_mgrs = NULL;
_out:
	return ret;
}

static void ps3_qos_pd_context_exit(struct ps3_instance *instance)
{
	U16 i = 0;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	struct ps3_qos_pd_context *qos_pd_ctx = NULL;
	struct workqueue_struct *wq = NULL;

	qos_pd_ctx = &instance->qos_context.pd_ctx;
	if (qos_pd_ctx->qos_pd_mgrs != NULL) {
		for (i = 1; i <= instance->qos_context.max_pd_count; i++) {
			qos_pd_mgr = &qos_pd_ctx->qos_pd_mgrs[i];
			cancel_work_sync(&qos_pd_mgr->resend_work);
			ps3_vfree(instance, qos_pd_mgr->waitqs);
			qos_pd_mgr->waitqs = NULL;
		}
		ps3_vfree(instance, qos_pd_ctx->qos_pd_mgrs);
		qos_pd_ctx->qos_pd_mgrs = NULL;
	}

	if (qos_pd_ctx->work_queues != NULL) {
		for (i = 0; i < qos_pd_ctx->workq_count; i++) {
			wq = qos_pd_ctx->work_queues[i];
			flush_workqueue(wq);
			destroy_workqueue(wq);
			qos_pd_ctx->work_queues[i] = NULL;
		}
		ps3_vfree(instance, qos_pd_ctx->work_queues);
		qos_pd_ctx->work_queues = NULL;
	}
}

static S32 ps3_qos_vd_context_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U16 i = 0;
	U16 j = 0;
	struct ps3_qos_vd_context *qos_vd_ctx = NULL;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
	char workq_name[PS3_QOS_MAX_WORKQ_NAME_LENGTH] = {0};
	U16 max_vd_count = 0;
	U16 jbod_vd_quota = 0;

	qos_vd_ctx = &instance->qos_context.vd_ctx;
	if (ps3_get_pci_function(instance->pdev) == PS3_FUNC_ID_0) {
		qos_vd_ctx->vd_exclusive_cnt = 0;
		qos_vd_ctx->jbod_exclusive_cnt = 0;
	} else {
		qos_vd_ctx->vd_exclusive_cnt = g_ps3_qos_vd_exclusive;
		qos_vd_ctx->jbod_exclusive_cnt = g_ps3_qos_jbod_exclusive;
	}
	jbod_vd_quota = instance->qos_context.tg_ctx.share + qos_vd_ctx->jbod_exclusive_cnt;

	max_vd_count = instance->qos_context.max_vd_count;
	qos_vd_ctx->qos_vd_mgrs = (struct ps3_qos_vd_mgr *)ps3_vzalloc(instance,
								(max_vd_count + 1) * sizeof(struct ps3_qos_vd_mgr));
	INJECT_START(PS3_ERR_IJ_QOS_VD_INIT_FAIL_1, instance)
	if (qos_vd_ctx->qos_vd_mgrs == NULL ) {
		LOG_ERROR("hno:%u:Failed to kcalloc memory for qos_vd_mgrs\n",
			  PS3_HOST(instance));
		ret =  -PS3_FAILED;
		goto _out;
	}
	memset(qos_vd_ctx->qos_vd_mgrs, 0, sizeof(struct ps3_qos_vd_mgr) * (max_vd_count + 1));
	for (i = 1; i <= max_vd_count; i++) {
		qos_vd_mgr = &qos_vd_ctx->qos_vd_mgrs[i];
		qos_vd_mgr->instance = instance;
		ps3_spin_lock_init(&qos_vd_mgr->rsc_lock);
		INIT_LIST_HEAD(&qos_vd_mgr->vd_quota_wait_q.wait_list);
		qos_vd_mgr->vd_quota_wait_q.rsc_lock = &qos_vd_mgr->rsc_lock;
		qos_vd_mgr->vd_quota_wait_q.count = 0;
		INIT_WORK(&qos_vd_mgr->resend_work, ps3_qos_vd_resend_work);
		qos_vd_mgr->workq_id = 0;
		qos_vd_mgr->id = i;
		if (i == max_vd_count) { 
			ps3_atomic_set(&qos_vd_mgr->vd_quota, jbod_vd_quota);
			ps3_atomic_set(&qos_vd_mgr->exclusive_cmd_cnt, qos_vd_ctx->jbod_exclusive_cnt);
			qos_vd_mgr->valid = PS3_TRUE;
		} else {
			ps3_atomic_set(&qos_vd_mgr->exclusive_cmd_cnt, qos_vd_ctx->vd_exclusive_cnt);
		}
		ps3_atomic_set(&qos_vd_mgr->share_cmd_used, 0);
	}

	qos_vd_ctx->workq_count = 1;
	qos_vd_ctx->work_queues = (struct workqueue_struct **)ps3_vzalloc(instance,
								(qos_vd_ctx->workq_count) * sizeof(struct workqueue_struct *));
	INJECT_START(PS3_ERR_IJ_QOS_VD_INIT_FAIL_2, instance)
	if (qos_vd_ctx->work_queues == NULL) {
		LOG_ERROR("hno:%u:Failed to kcalloc memory for work_queues\n",
			  PS3_HOST(instance));
		ret =  -PS3_FAILED;
		goto _release_vd_mgr;
	}
	memset(qos_vd_ctx->work_queues, 0, (qos_vd_ctx->workq_count) * sizeof(struct workqueue_struct *));
	for (i = 0; i < qos_vd_ctx->workq_count; i++) {
		snprintf(workq_name, PS3_QOS_MAX_WORKQ_NAME_LENGTH, "qos_vd_wq_f%u_%u",
			 ps3_get_pci_function(instance->pdev), i);
		qos_vd_ctx->work_queues[i] = create_singlethread_workqueue(workq_name);
		INJECT_START(PS3_ERR_IJ_QOS_VD_INIT_FAIL_3, instance)
		if (qos_vd_ctx->work_queues[i] == NULL) {
			LOG_ERROR("qos vd workq:%u create failed\n", i);
			ret = -PS3_FAILED;
			goto  _destroy_created_workqueue;
		}
	}

	LOG_INFO("hno:%u: qos vd context init success\n", PS3_HOST(instance));

	goto _out;
_destroy_created_workqueue:
	for (j = 0; j < i; j++) {
		destroy_workqueue(qos_vd_ctx->work_queues[j]);
		qos_vd_ctx->work_queues[j] = NULL;
	}
	ps3_vfree(instance, qos_vd_ctx->work_queues);
	qos_vd_ctx->work_queues = NULL;
_release_vd_mgr:
	ps3_vfree(instance, qos_vd_ctx->qos_vd_mgrs);
	qos_vd_ctx->qos_vd_mgrs = NULL;
_out:
	return ret;
}

static void ps3_qos_vd_context_exit(struct ps3_instance *instance)
{
	U16 i = 0;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
	struct ps3_qos_vd_context *qos_vd_ctx = NULL;
	struct workqueue_struct *wq = NULL;

	qos_vd_ctx = &instance->qos_context.vd_ctx;
	if (qos_vd_ctx->qos_vd_mgrs != NULL) {
		for (i = 1; i <= instance->qos_context.max_vd_count; i++) {
			qos_vd_mgr = &qos_vd_ctx->qos_vd_mgrs[i];
			cancel_work_sync(&qos_vd_mgr->resend_work);
		}
		ps3_vfree(instance, qos_vd_ctx->qos_vd_mgrs);
		qos_vd_ctx->qos_vd_mgrs = NULL;
	}

	if (qos_vd_ctx->work_queues != NULL) {
		for (i = 0; i < qos_vd_ctx->workq_count; i++) {
			wq = qos_vd_ctx->work_queues[i];
			flush_workqueue(wq);
			destroy_workqueue(wq);
		}
		ps3_vfree(instance, qos_vd_ctx->work_queues);
		qos_vd_ctx->work_queues = NULL;
	}
}

static S32 ps3_qos_tg_context_init(struct ps3_instance *instance)
{
	struct ps3_qos_tg_context *qos_tg_ctx = NULL;
	S32 ret = PS3_SUCCESS;
	U64 tfifo_depth = 0;
	U16 i = 0;
	struct qos_wait_queue *wait_q = NULL;
	char workq_name[PS3_QOS_MAX_WORKQ_NAME_LENGTH] = {0};

	ps3_tfifo_depth_get(instance, &tfifo_depth);
	INJECT_START(PS3_ERR_IJ_QOS_FORCE_TFIFO_ZERO_DEPTH, &tfifo_depth)
	if (tfifo_depth == 0) {
		ret = -PS3_FAILED;
		LOG_ERROR("hno:%u:tfifo_depth is invalid func:%u\n",
			PS3_HOST(instance),
			ps3_get_pci_function(instance->pdev));
		goto out;
	}
	qos_tg_ctx = &instance->qos_context.tg_ctx;
	qos_tg_ctx->instance = instance;
	qos_tg_ctx->high_pri_exclusive_cnt = QOS_HIGH_PRI_EXCLUSIVE_CMD_COUNT >> 1;
	qos_tg_ctx->mgr_exclusive_cnt = g_ps3_qos_mgr_exclusive >> 1;
	if (ps3_get_pci_function(instance->pdev) == PS3_FUNC_ID_0) {
		qos_tg_ctx->share = PS3_QOS_CS_FUNC0_SHARE_CMD;
	} else {
		qos_tg_ctx->share = tfifo_depth - qos_tg_ctx->mgr_exclusive_cnt - qos_tg_ctx->high_pri_exclusive_cnt
				 - instance->ctrl_info.maxVdCount * g_ps3_qos_vd_exclusive
				 - g_ps3_qos_jbod_exclusive;
	}

	ps3_atomic_set(&qos_tg_ctx->mgr_free_cnt, qos_tg_ctx->mgr_exclusive_cnt);
	ps3_atomic_set(&qos_tg_ctx->share_free_cnt, qos_tg_ctx->share);
	ps3_atomic_set(&qos_tg_ctx->mgr_share_used, 0);
	ps3_spin_lock_init(&qos_tg_ctx->lock);
	qos_tg_ctx->poll_vd_id = 1;

	wait_q = &qos_tg_ctx->mgr_cmd_wait_q;
	INIT_LIST_HEAD(&wait_q->wait_list);
	qos_tg_ctx->vd_cmd_waitqs = (struct qos_wait_queue *)ps3_vzalloc(instance,
									 (instance->qos_context.max_vd_count + 1) * sizeof(struct qos_wait_queue));
	INJECT_START(PS3_ERR_IJ_QOS_TAG_INIT_FAIL_1, qos_tg_ctx)
	if (qos_tg_ctx->vd_cmd_waitqs == NULL) {
		LOG_ERROR("hno:%u:Failed to kcalloc memory for qos_context\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto out;
	}
	memset(qos_tg_ctx->vd_cmd_waitqs, 0, (instance->qos_context.max_vd_count + 1) * sizeof(struct qos_wait_queue));
	for (i = 1; i < instance->qos_context.max_vd_count + 1; i++) {
		wait_q =  &qos_tg_ctx->vd_cmd_waitqs[i];
		INIT_LIST_HEAD(&wait_q->wait_list);
	}

	INIT_WORK(&qos_tg_ctx->resend_work, ps3_qos_tg_resend_work);
	snprintf(workq_name, PS3_QOS_MAX_WORKQ_NAME_LENGTH, "qos_tag_wq_f%u", ps3_get_pci_function(instance->pdev));
	qos_tg_ctx->work_queue = create_singlethread_workqueue(workq_name);
	INJECT_START(PS3_ERR_IJ_QOS_TAG_INIT_FAIL_2, qos_tg_ctx)
	if (qos_tg_ctx->work_queue == NULL) {
		LOG_ERROR("qos tag workq:%u create failed\n", i);
		ret = -PS3_FAILED;
		goto  release_vd_cmd_waitqs;
	}

	LOG_INFO("hno:%u func:%u: qos tg context init success tfifo:%llu\n",
			PS3_HOST(instance), ps3_get_pci_function(instance->pdev), tfifo_depth);
	goto out;
release_vd_cmd_waitqs:
	ps3_vfree(instance, qos_tg_ctx->vd_cmd_waitqs);
	qos_tg_ctx->vd_cmd_waitqs = NULL;
out:
	return ret;
}

static void ps3_qos_tg_context_exit(struct ps3_instance *instance)
{
	struct ps3_qos_tg_context *qos_tg_ctx = NULL;

	qos_tg_ctx = &instance->qos_context.tg_ctx;
	if (qos_tg_ctx->work_queue != NULL) {
		cancel_work_sync(&qos_tg_ctx->resend_work);
		flush_workqueue(qos_tg_ctx->work_queue);
		destroy_workqueue(qos_tg_ctx->work_queue);
		qos_tg_ctx->work_queue = NULL;
	}

	if (qos_tg_ctx->vd_cmd_waitqs != NULL) {
		ps3_vfree(instance, qos_tg_ctx->vd_cmd_waitqs);
		qos_tg_ctx->vd_cmd_waitqs = NULL;
	}
}

S32 ps3_hba_qos_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	ret = ps3_qos_tg_context_init(instance);
	if (ret != PS3_SUCCESS) {
		goto _out;
	}

	ret = ps3_qos_vd_context_init(instance);
	if (ret != PS3_SUCCESS) {
		goto _tg_ctx_exit;
	}

	ret = ps3_qos_pd_context_init(instance);
	if (ret != PS3_SUCCESS) {
		goto _vd_ctx_exit;
	}

	goto _out;
_vd_ctx_exit:
	ps3_qos_vd_context_exit(instance);
_tg_ctx_exit:
	ps3_qos_tg_context_exit(instance);
_out:
	return ret;
}

void ps3_hba_qos_exit(struct ps3_instance *instance)
{
	ps3_qos_tg_context_exit(instance);
	ps3_qos_vd_context_exit(instance);
	ps3_qos_pd_context_exit(instance);

	LOG_INFO("hba qos exit. host_no:%u\n", PS3_HOST(instance));
}

static Bool ps3_qos_vd_member_insert(struct ps3_cmd *cmd)
{
	U8 i = 0;
	U16 disk_id = 0;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	struct qos_wait_queue *waitq = NULL;
	Bool in_queue = PS3_FALSE;

	for (i = 0; i < cmd->target_pd_count; i++) {
		disk_id = cmd->target_pd[i].flat_disk_id;
		qos_pd_mgr = ps3_qos_pd_mgr_get(cmd->instance, disk_id);
		ps3_atomic_inc(&qos_pd_mgr->processing_cnt);
		INJECT_START(PS3_ERR_IJ_QOS_VD_MEMBER_CLEARING, qos_pd_mgr);
		if (unlikely(qos_pd_mgr->clearing)) {
			LOG_DEBUG("qos vd member is clearing .host_no:%u:t_id:0x%llx CFID:%u did[%u %u]\n",
				PS3_HOST(qos_pd_mgr->instance), cmd->trace_id, cmd->index,
				qos_pd_mgr->disk_id, qos_pd_mgr->vd_id);
			ps3_atomic_dec(&qos_pd_mgr->processing_cnt);
		} else {
			in_queue = PS3_TRUE;
			break;
		}
	}

	if (in_queue) {
		waitq = ps3_qos_pd_waitq_get(qos_pd_mgr, cmd);
		ps3_qos_pd_in_q(cmd, waitq, i);
		ps3_atomic_dec(&qos_pd_mgr->processing_cnt);
		LOG_DEBUG("insert qos pd quota waitq.host_no:%u:t_id:0x%llx"
			" CFID:%u waitq[%u,%u] did[%u %u]\n",
			PS3_HOST(cmd->instance), cmd->trace_id, cmd->index,
			waitq->id, waitq->count, qos_pd_mgr->disk_id, i);
	}

	return in_queue;
}

static Bool ps3_qos_pd_quota_decision(struct ps3_cmd *cmd)
{
	Bool can_get = PS3_FALSE;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	U16 disk_id = 0;
	U16 i = 0;
	struct scsi_device *sdev = NULL;
	S32 pd_quota = 0;

	PS3_QOS_STAT_START(cmd->instance, cmd, PS3_QOS_PD_PRO);
	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		ps3_qos_cmd_member_pd_calc(cmd);
		for (i = 0; i < cmd->target_pd_count; i++) {
			disk_id = cmd->target_pd[i].flat_disk_id;
			qos_pd_mgr = ps3_qos_pd_mgr_get(cmd->instance, disk_id);
			if (ps3_qos_pd_quota_check(qos_pd_mgr, cmd)) {
				cmd->target_pd[i].get_quota = PS3_TRUE;
				can_get = PS3_TRUE;
				break;
			}
		}

		if (cmd->target_pd_count == 0) {
			can_get = PS3_TRUE;
		}

		if (!can_get) {
			if (!ps3_qos_vd_member_insert(cmd)) {
				can_get = PS3_TRUE;
			}
		}
	} else { 
		disk_id = cmd->io_attr.disk_id;
		qos_pd_mgr = ps3_qos_pd_mgr_get(cmd->instance, disk_id);
		if (ps3_is_nvme_direct_cmd(qos_pd_mgr, cmd)) {
			pd_quota = qos_pd_mgr->direct_quota;
		} else {
			pd_quota = qos_pd_mgr->pd_quota;
		}
		sdev = cmd->scmd->device;
		if (pd_quota >= sdev->queue_depth) {
			can_get = PS3_TRUE;
		} else {
			ps3_qos_cmd_member_pd_calc(cmd);
			can_get = ps3_qos_pd_quota_req(qos_pd_mgr, cmd, 0);
		}
	}
	PS3_QOS_STAT_END(cmd->instance, cmd, PS3_QOS_PD_PRO);
	return can_get;
}

static Bool ps3_qos_vd_quota_decision(struct ps3_cmd *cmd)
{
	Bool can_get = PS3_TRUE;

	PS3_QOS_STAT_START(cmd->instance, cmd, PS3_QOS_VD_PRO);
	if (cmd->io_attr.dev_type != PS3_DEV_TYPE_VD){ 
		can_get = ps3_qos_vd_quota_check(cmd);
	}
	PS3_QOS_STAT_END(cmd->instance, cmd, PS3_QOS_VD_PRO);

	return can_get;
}

static inline Bool ps3_qos_dev_valid(struct ps3_cmd *cmd)
{
	Bool valid = PS3_TRUE;
	struct ps3_scsi_priv_data* priv_data = NULL;

	priv_data = scsi_device_private_data(cmd->scmd);
	if (priv_data->dev_deling) {
		LOG_INFO_LIM("qos dev is deling. host_no:%u chanel:%u id:%u\n",
				PS3_HOST(cmd->instance),
				PS3_SDEV_CHANNEL(cmd->scmd->device),
				PS3_SDEV_TARGET(cmd->scmd->device));
		valid = PS3_FALSE;
	}

	return valid;
}

static S32 ps3_qos_pre_check(struct ps3_instance *instance, struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;

	INJECT_START(PS3_ERR_IJ_FORCE_TASK_MGR_BUSY, instance)
	if (instance->task_manager_host_busy) {
		LOG_INFO_LIM("hno:%u task_manager_host_busy\n",
			 PS3_HOST(instance));
		ret = -PS3_RETRY;
		goto l_out;
	}

	INJECT_START(PS3_ERR_IJ_FORCE_INS_UNLOAD, instance)
	if (instance->is_probe_finish && !instance->state_machine.is_load) {
		LOG_INFO_LIM("hno:%u instance state not is_load\n", PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	if (!ps3_qos_dev_valid(cmd)){
		ret = -PS3_FAILED;
	}

	INJECT_START(PS3_ERR_IJ_FORCE_INSTANCE_UNNORMAL, instance)
	if (!ps3_is_instance_state_normal(instance)) {
		LOG_INFO_LIM("hno:%u instance state is not normal\n", PS3_HOST(instance));
		ret = -PS3_RECOVERED;
		goto l_out;
	}
l_out:
	return ret;
}

Bool ps3_hba_qos_decision(struct ps3_cmd *cmd)
{
	Bool can_get = PS3_FALSE;
	if (PS3_CMD_TYPE_IS_RW(cmd->cmd_word.type)) {
		can_get = ps3_qos_pd_quota_decision(cmd); 
		if (!can_get ||  
				cmd->io_attr.direct_flag == PS3_CMDWORD_DIRECT_OK) {
			goto _out;
		}

		can_get = ps3_qos_vd_quota_decision(cmd); 
		if (!can_get) {
			goto _out;
		}
	}

	can_get = ps3_qos_tg_decision(cmd);  
_out:
	return can_get;
}

void ps3_hba_qos_waitq_notify(struct ps3_instance *instance)
{
	if (!ps3_qos_tg_notify(instance)) {
		if (!ps3_qos_vd_notify(instance)) {
			ps3_qos_pd_notify(instance);
		}
	}
}

static Bool ps3_pd_quota_waiq_abort(struct ps3_cmd *cmd)
{
	ULong lock_flag_pd = 0;
	U16 disk_id = 0;
	Bool found = PS3_FALSE;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	struct qos_wait_queue *waitq = NULL;
	U16 index = 0;

	for (index = cmd->first_over_quota_pd_idx;
			index < cmd->target_pd_count; index++) {
		disk_id = cmd->target_pd[index].flat_disk_id;
		qos_pd_mgr = ps3_qos_pd_mgr_get(cmd->instance, disk_id);
		waitq = ps3_qos_pd_waitq_get(qos_pd_mgr, cmd);
		ps3_spin_lock_irqsave(waitq->rsc_lock, &lock_flag_pd);
		if (cmd->qos_waitq_flag == PS3_QOS_CMD_IN_PD &&
				index == cmd->first_over_quota_pd_idx) {
			list_del(&cmd->qos_list);
			waitq->count--;
			(*waitq->total_waited_cnt)--;
			ps3_spin_unlock_irqrestore(waitq->rsc_lock, lock_flag_pd);
			found = PS3_TRUE;
			break;
		}
		ps3_spin_unlock_irqrestore(waitq->rsc_lock, lock_flag_pd);
		LOG_INFO("abort pd waitq. t_id:0x%llx did[%u,%u] qid:%u found:%u\n",
			cmd->trace_id, index, disk_id, waitq->id, found);
	}

	return found;
}

static bool ps3_vd_quota_waiq_abort(struct ps3_cmd *aborted_cmd)
{
	ULong flag = 0;
	Bool found = PS3_FALSE;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;

	qos_vd_mgr = ps3_qos_vd_mgr_get(aborted_cmd);
	ps3_spin_lock_irqsave(qos_vd_mgr->vd_quota_wait_q.rsc_lock, &flag);
	if (aborted_cmd->qos_waitq_flag == PS3_QOS_CMD_IN_VD) {
		list_del(&aborted_cmd->qos_list);
		qos_vd_mgr->vd_quota_wait_q.count--;
		found = PS3_TRUE;
	}
	ps3_spin_unlock_irqrestore(qos_vd_mgr->vd_quota_wait_q.rsc_lock, flag);

	return found;
}

bool ps3_cmd_waitq_abort(struct ps3_cmd *aborted_cmd)
{
	ULong lock_flag_mgr = 0;
	struct ps3_instance *instance = NULL;
	Bool found = PS3_FALSE;
	struct qos_wait_queue *cmd_wait_q = NULL;
	struct ps3_qos_tg_context *qos_tg_ctx = NULL;

	instance = aborted_cmd->instance;
	qos_tg_ctx = &instance->qos_context.tg_ctx;
	ps3_spin_lock_irqsave(&qos_tg_ctx->lock, &lock_flag_mgr);
	if (aborted_cmd->qos_waitq_flag == PS3_QOS_CMD_IN_FRAME) {
		cmd_wait_q = ps3_qos_cmd_waitq_get(qos_tg_ctx, aborted_cmd);
		list_del(&aborted_cmd->qos_list);
		cmd_wait_q->count--;
		qos_tg_ctx->total_wait_cmd_cnt--;
		found = PS3_TRUE;
	}
	ps3_spin_unlock_irqrestore(&qos_tg_ctx->lock, lock_flag_mgr);

	return found;
}

static Bool ps3_hba_qos_waitq_abort(struct ps3_cmd *cmd)
{
	Bool found = PS3_FALSE;
	if (cmd->qos_waitq_flag == 0) {
		goto out;
	}

	if (PS3_CMD_TYPE_IS_RW(cmd->cmd_word.type)) {
		if (cmd->qos_waitq_flag == PS3_QOS_CMD_IN_PD) {
			found = ps3_pd_quota_waiq_abort(cmd);
			if (found) {
				goto out;
			}
		}

		if (cmd->qos_waitq_flag == PS3_QOS_CMD_IN_VD) {
			found = ps3_vd_quota_waiq_abort(cmd);
			if (found) {
				ps3_qos_update_pd_quota(cmd);
				goto out;
			}
		}
	}

	if (cmd->qos_waitq_flag == PS3_QOS_CMD_IN_FRAME) {
		found = ps3_cmd_waitq_abort(cmd);
		if (found) {
			ps3_qos_update_vd_quota(cmd);
			ps3_qos_update_pd_quota(cmd);
		}
	}
out:
	return found;
}

static void ps3_vd_cmd_waitq_clean(struct ps3_instance *instance, U16 disk_id, int result)
{
	ULong flag = 0;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *cmd_next = NULL;
	struct ps3_qos_tg_context *qos_tg_ctx = &instance->qos_context.tg_ctx;
	struct qos_wait_queue *wait_q = &qos_tg_ctx->vd_cmd_waitqs[disk_id];

	ps3_spin_lock_irqsave(&qos_tg_ctx->lock, &flag);
	if (wait_q->count > 0) {
		list_for_each_entry_safe(cmd, cmd_next, &wait_q->wait_list, qos_list) {
			LOG_DEBUG("qos clean vd cmd waitq. hno:%u t_id:0x%llx cmd:%d vd_id:%u\n",
				PS3_HOST(instance), cmd->trace_id, cmd->index, disk_id);
			list_del(&cmd->qos_list);
			wait_q->count--;
			qos_tg_ctx->total_wait_cmd_cnt--;
			ps3_qos_update_vd_quota(cmd);
			ps3_qos_update_pd_quota(cmd);
			ps3_scsih_drv_io_reply_scsi(cmd->scmd, cmd, result, PS3_FALSE);
		}
	}
	ps3_spin_unlock_irqrestore(&qos_tg_ctx->lock, flag);
}

static void ps3_jbod_cmd_waitq_clean(struct ps3_instance *instance,
				 struct ps3_scsi_priv_data *priv_data, int result)
{
	ULong flag = 0;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *cmd_next = NULL;
	struct ps3_qos_tg_context *qos_tg_ctx = &instance->qos_context.tg_ctx;
	struct qos_wait_queue *wait_q = &qos_tg_ctx->vd_cmd_waitqs
						[instance->qos_context.max_vd_count];
	U16 disk_id = priv_data->disk_pos.diskDev.ps3Dev.virtDiskID;

	ps3_spin_lock_irqsave(&qos_tg_ctx->lock, &flag);
	if (wait_q->count > 0) {
		list_for_each_entry_safe(cmd, cmd_next, &wait_q->wait_list, qos_list) {
			if (scsi_device_private_data(cmd->scmd) == priv_data) {
				LOG_DEBUG("qos clean jbod cmd waitq. hno:%u t_id:0x%llx cmd:%d dev_t:%u diskid:%u\n",
					PS3_HOST(instance), cmd->trace_id, cmd->index,
					priv_data->dev_type, disk_id);
				list_del(&cmd->qos_list);
				wait_q->count--;
				qos_tg_ctx->total_wait_cmd_cnt--;
				ps3_qos_update_vd_quota(cmd);
				ps3_qos_update_pd_quota(cmd);
				ps3_scsih_drv_io_reply_scsi(cmd->scmd, cmd, result, PS3_FALSE);
			}
		}
	}
	ps3_spin_unlock_irqrestore(&qos_tg_ctx->lock, flag);
}

static void ps3_mgr_cmd_waitq_clean(struct ps3_instance *instance, struct ps3_scsi_priv_data *priv_data, int result)
{
	ULong flag = 0;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *cmd_next = NULL;
	struct ps3_qos_tg_context *qos_tg_ctx = &instance->qos_context.tg_ctx;
	struct qos_wait_queue *wait_q = &qos_tg_ctx->mgr_cmd_wait_q;
	U16 disk_id = priv_data->disk_pos.diskDev.ps3Dev.virtDiskID;

	ps3_spin_lock_irqsave(&qos_tg_ctx->lock, &flag);
	if (wait_q->count > 0) {
		list_for_each_entry_safe(cmd, cmd_next, &wait_q->wait_list, qos_list) {
			if (cmd->scmd && scsi_device_private_data(cmd->scmd) == priv_data) {
				LOG_DEBUG("qos clean mgr cmd waitq. hno:%u t_id:0x%llx cmd:%d dev_t:%u diskid:%u\n",
					PS3_HOST(instance), cmd->trace_id, cmd->index,
					priv_data->dev_type, disk_id);
				list_del(&cmd->qos_list);
				wait_q->count--;
				qos_tg_ctx->total_wait_cmd_cnt--;
				ps3_scsih_drv_io_reply_scsi(cmd->scmd, cmd, result, PS3_FALSE);
			}
		}
	}
	ps3_spin_unlock_irqrestore(&qos_tg_ctx->lock, flag);
}

static void ps3_hba_qos_vd_clean(struct ps3_instance *instance,
			struct ps3_scsi_priv_data *priv_data, S32 resp_status)
{
	U16 vd_id = 0;
	U16 pd_id = 0;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;

	vd_id = get_offset_of_vdid(PS3_VDID_OFFSET(instance), PS3_VDID(&priv_data->disk_pos));
	qos_vd_mgr = ps3_qos_vd_mgr_get_by_id(instance, PS3_VDID(&priv_data->disk_pos));
	for (pd_id = 1; pd_id <= instance->qos_context.max_pd_count; pd_id++) {
		qos_pd_mgr = ps3_qos_pd_mgr_get(instance, pd_id);
		if (ps3_atomic_read(&qos_pd_mgr->valid) == 1 &&
			qos_pd_mgr->vd_id == vd_id) {
			ps3_pd_quota_waitq_clean(qos_pd_mgr, vd_id, resp_status);
			cancel_work_sync(&qos_pd_mgr->resend_work);
		}
	}

	ps3_vd_quota_waitq_clean(qos_vd_mgr, NULL, resp_status);
	cancel_work_sync(&qos_vd_mgr->resend_work);

	ps3_vd_cmd_waitq_clean(instance, vd_id, resp_status);
	ps3_mgr_cmd_waitq_clean(instance, priv_data, resp_status);

	LOG_FILE_INFO("qos clean vd. host_no:%u type:%u disk_id:%u\n",
		 PS3_HOST(instance), priv_data->dev_type, vd_id);
}

#define PS3_QOS_JBOD_VD_MGR(instance) \
 (&instance->qos_context.vd_ctx.qos_vd_mgrs[instance->qos_context.max_vd_count])

#if (LINUX_VERSION_CODE > KERNEL_VERSION(4,9,0) && LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0))
void ps3_linx80_vd_member_change(struct ps3_instance *instance, struct ps3_pd_entry *pd_entry)
{
	U16 pd_id = 0;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	if (!PS3_QOS_INITED(instance)) {
		goto _out;
	}

	pd_id = PS3_PDID(&pd_entry->disk_pos);
	qos_pd_mgr = ps3_qos_pd_mgr_get(instance, pd_id);
	if (ps3_atomic_read(&qos_pd_mgr->valid) != PS3_TRUE) {
		goto _out;
	}
	ps3_pd_quota_waitq_clean(qos_pd_mgr, 0, PS3_STATUS_VD_MEMBER_OFFLINE);
	cancel_work_sync(&qos_pd_mgr->resend_work);
	LOG_INFO("linx80 update pd qos rsc. host_no:%u pd_id:%u dev_type:%u\n",
		PS3_HOST(instance), pd_id, pd_entry->dev_type);

_out:
	return;
}
#endif
static void ps3_hba_qos_pd_clean(struct ps3_instance *instance,
			struct ps3_scsi_priv_data *priv_data, S32 resp_status)
{
	U16 disk_id = 0;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;

	disk_id = PS3_PDID(&priv_data->disk_pos);
	qos_pd_mgr = ps3_qos_pd_mgr_get(instance, disk_id);
	ps3_pd_quota_waitq_clean(qos_pd_mgr, 0, resp_status);
	cancel_work_sync(&qos_pd_mgr->resend_work);

	qos_vd_mgr = PS3_QOS_JBOD_VD_MGR(instance);
	ps3_vd_quota_waitq_clean(qos_vd_mgr, priv_data, resp_status);
	ps3_jbod_cmd_waitq_clean(instance, priv_data, resp_status);
	ps3_mgr_cmd_waitq_clean(instance, priv_data, resp_status);

	LOG_FILE_INFO("qos clean pd. host_no:%u diskid:%u\n",
		 PS3_HOST(instance), disk_id);
}

static void ps3_qos_wait_io_end(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd *cmd = NULL;
	LOG_DEBUG("host_no:%u wait cmd in qos flighting start\n",
		PS3_HOST(instance));
	for (i = 0; i < instance->cmd_context.max_scsi_cmd_count; i++) {
		cmd = instance->cmd_context.cmd_buf[i];
		while (cmd->qos_processing) {
			ps3_msleep(PS3_LOOP_TIME_INTERVAL_20MS);
		}
	}
	LOG_DEBUG("host_no:%u wait cmd in qos flighting end\n",
		PS3_HOST(instance));
}

void ps3_qos_device_clean(struct ps3_instance *instance, struct ps3_scsi_priv_data *priv_data,
		 S32 resp_status)
{
	if (!PS3_QOS_INITED(instance)) {
		return;
	}

	ps3_qos_wait_io_end(instance);
	if (priv_data->dev_type == PS3_DEV_TYPE_VD) {
		instance->qos_context.opts.qos_vd_clean(instance, priv_data, resp_status);
	} else {
		instance->qos_context.opts.qos_pd_clean(instance, priv_data, resp_status);
	}
}

static inline void ps3_qos_dev_end(struct ps3_instance *instance,
					 struct ps3_scsi_priv_data *priv_data)
{
	U32 i = 0;
	struct ps3_cmd *cmd = NULL;
	struct scsi_cmnd *scmd = NULL;
	struct scsi_device *sdev = NULL;
	U32 softChan = PS3_CHANNEL(&priv_data->disk_pos);
	U32 devID = PS3_TARGET(&priv_data->disk_pos);

	LOG_DEBUG("host_no:%u dev wait cmd in qos flighting start\n",
		PS3_HOST(instance));
	for (i = 0; i < instance->cmd_context.max_scsi_cmd_count; i++) {
		cmd = instance->cmd_context.cmd_buf[i];
		if (likely(!cmd->qos_processing)) {
			continue;
		}

		scmd = cmd->scmd;
		if (scmd == NULL) {
			continue;
		}

		sdev = scmd->device;
		if (sdev == NULL) {
			continue;
		}

		if (PS3_SDEV_CHANNEL(sdev) == softChan &&
			PS3_SDEV_TARGET(sdev) == devID) {
			LOG_DEBUG("cmd is flighting. host_no:%u CFID:%u\n",
				PS3_HOST(instance), cmd->index);
			while (cmd->qos_processing) {
				ps3_msleep(PS3_LOOP_TIME_INTERVAL_20MS);
			}
		}
	}
	LOG_DEBUG("host_no:%u dev wait cmd in qos flighting end\n",
		PS3_HOST(instance));
}

void ps3_qos_disk_del(struct ps3_instance *instance, struct ps3_scsi_priv_data *priv_data)
{
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	U16 disk_id = 0;
	S32 resp_status = PS3_STATUS_DEVICE_NOT_FOUND;

	if (!PS3_QOS_INITED(instance)) {
		return;
	}

	priv_data->dev_deling = PS3_TRUE;
	wmb();
	disk_id = PS3_PDID(&priv_data->disk_pos);
	LOG_FILE_INFO("qos disk del. host_no:%u dev_t:%u diskid:%u\n",
		PS3_HOST(instance), priv_data->dev_type, disk_id);
	if (priv_data->dev_type == PS3_DEV_TYPE_VD) {
		ps3_qos_dev_end(instance, priv_data);
		instance->qos_context.opts.qos_vd_clean(instance, priv_data, resp_status);
	} else {
		qos_pd_mgr = ps3_qos_pd_mgr_get(instance, disk_id);
		ps3_qos_dev_end(instance, priv_data);
		if (PS3_QOS_PD_IS_VD_MEMBER(qos_pd_mgr)) {
			resp_status = PS3_STATUS_VD_MEMBER_OFFLINE;
			qos_pd_mgr->clearing = PS3_TRUE;
			wmb();
			LOG_INFO("host_no:%u wait cmd in qos vd member flighting count:%d did:%u\n",
				PS3_HOST(instance),
				ps3_atomic_read(&qos_pd_mgr->processing_cnt), disk_id);
			while (ps3_atomic_read(&qos_pd_mgr->processing_cnt) > 0) {
				ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
			}
		}
		instance->qos_context.opts.qos_pd_clean(instance, priv_data, resp_status);
	}
}

void ps3_qos_vd_member_del(struct ps3_instance *instance, struct PS3DiskDevPos *dev_pos)
{
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	U16 disk_id = 0;

	disk_id = PS3_PDID(dev_pos);
	qos_pd_mgr = ps3_qos_pd_mgr_get(instance, disk_id);
	LOG_INFO("qos disk del. host_no:%u did:%u vid:%u\n",
		PS3_HOST(instance), disk_id, qos_pd_mgr->vd_id);

	if (PS3_QOS_PD_IS_VD_MEMBER(qos_pd_mgr)) {
		ps3_pd_quota_waitq_clean(qos_pd_mgr, 0, PS3_STATUS_VD_MEMBER_OFFLINE);
	}
}

void ps3_hba_qos_waitq_clear_all(struct ps3_instance *instance, S32 resp_status)
{
	ULong flag = 0;
	U16 i = 0;
	struct ps3_qos_tg_context *qos_tg_ctx = &instance->qos_context.tg_ctx;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	struct ps3_qos_vd_mgr *qos_vd_mgr =  NULL;
	struct qos_wait_queue *wait_q = NULL;
	struct ps3_cmd *cmd = NULL;

	for (i = 1; i <= instance->qos_context.max_pd_count; i++) {
		qos_pd_mgr = ps3_qos_pd_mgr_get(instance, i);
		if (ps3_atomic_read(&qos_pd_mgr->valid) == 1) {
			ps3_pd_quota_waitq_clear_all(qos_pd_mgr, resp_status);
			cancel_work_sync(&qos_pd_mgr->resend_work);
		}
	}

	for (i = 1; i <= instance->qos_context.max_vd_count; i++) {
		qos_vd_mgr = &instance->qos_context.vd_ctx.qos_vd_mgrs[i];
		if (qos_vd_mgr->valid) {
			ps3_vd_quota_waitq_clean(qos_vd_mgr, NULL, resp_status);
			cancel_work_sync(&qos_vd_mgr->resend_work);
		}
	}

	ps3_spin_lock_irqsave(&qos_tg_ctx->lock, &flag);
	for (i = 1; i <= instance->qos_context.max_vd_count; i++) {
		wait_q = &qos_tg_ctx->vd_cmd_waitqs[i];
		while (wait_q->count > 0) {
			cmd = list_first_entry(&wait_q->wait_list, struct ps3_cmd, qos_list);
			LOG_DEBUG("qos clear tg vd waitq. hno:%u qid:%u t_id:0x%llx cmd:%d\n",
					PS3_HOST(instance), i, cmd->trace_id, cmd->index);
			list_del(&cmd->qos_list);
			wait_q->count--;
			qos_tg_ctx->total_wait_cmd_cnt--;
			ps3_qos_update_vd_quota(cmd);
			ps3_qos_update_pd_quota(cmd);
			ps3_scsih_drv_io_reply_scsi(cmd->scmd, cmd, resp_status, PS3_FALSE);
		}
	}

	wait_q = &qos_tg_ctx->mgr_cmd_wait_q;
	while (wait_q->count > 0) {
		cmd = list_first_entry(&wait_q->wait_list, struct ps3_cmd, qos_list);
		LOG_DEBUG("qos clear tg mgr waitq. hno:%u t_id:0x%llx cmd:%d\n",
				PS3_HOST(instance), cmd->trace_id, cmd->index);
		list_del(&cmd->qos_list);
		wait_q->count--;
		qos_tg_ctx->total_wait_cmd_cnt--;
		ps3_scsih_drv_io_reply_scsi(cmd->scmd, cmd, resp_status, PS3_FALSE);
	}
	ps3_spin_unlock_irqrestore(&qos_tg_ctx->lock, flag);
	cancel_work_sync(&qos_tg_ctx->resend_work);

	LOG_INFO("host_no:%u:clear all qos waitq\n", PS3_HOST(instance));
}

static inline U16 ps3_qos_pd_workq_id_get(struct ps3_qos_pd_context *qos_pd_ctx)
{
	return ps3_atomic_inc_return(&qos_pd_ctx->workq_id_cnt) % qos_pd_ctx->workq_count;
}

static void ps3_qos_pd_rsc_init(struct ps3_qos_pd_mgr *qos_pd_mgr, struct ps3_pd_entry *pd_entry)
{
	struct ps3_qos_pd_context *pd_ctx = NULL;
	struct PS3QosInfo *qos_cfg_info = NULL;

	qos_cfg_info = &qos_pd_mgr->instance->ctrl_info.qosInfo;
	qos_pd_mgr->pd_quota = pd_entry->normal_quota;
	qos_pd_mgr->direct_quota = pd_entry->direct_quota;
	switch (pd_entry->dev_type) {
		case PS3_DEV_TYPE_SAS_HDD:
			if (qos_pd_mgr->pd_quota == 0) {
				qos_pd_mgr->pd_quota = qos_cfg_info->sasHddQuota > 0 ?
							 qos_cfg_info->sasHddQuota : g_ps3_qos_sas_pd_quota;
			}
			qos_pd_mgr->pd_init_quota = qos_pd_mgr->pd_quota;
			qos_pd_mgr->adjust_max_quota = qos_pd_mgr->pd_quota;
			qos_pd_mgr->adjust_min_quota = qos_pd_mgr->pd_quota - (qos_pd_mgr->pd_quota / 10);
			break;
		case PS3_DEV_TYPE_SAS_SSD:
			if (qos_pd_mgr->pd_quota == 0) {
				qos_pd_mgr->pd_quota = qos_cfg_info->sasSsdQuota > 0 ?
						 qos_cfg_info->sasSsdQuota : g_ps3_qos_sas_pd_quota;
			}
			qos_pd_mgr->pd_init_quota = qos_pd_mgr->pd_quota;
			qos_pd_mgr->adjust_max_quota = qos_pd_mgr->pd_quota;
			qos_pd_mgr->adjust_min_quota = qos_pd_mgr->pd_quota - (qos_pd_mgr->pd_quota / 10);
			break;
		case PS3_DEV_TYPE_SATA_HDD:
			if (qos_pd_mgr->pd_quota == 0) {
				qos_pd_mgr->pd_quota = qos_cfg_info->sataHddQuota > 0 ?
						 qos_cfg_info->sataHddQuota : g_ps3_qos_hdd_pd_quota;
			}
			qos_pd_mgr->pd_init_quota = qos_pd_mgr->pd_quota;
			qos_pd_mgr->adjust_max_quota = qos_pd_mgr->pd_quota;
			qos_pd_mgr->adjust_min_quota = qos_pd_mgr->pd_quota - (qos_pd_mgr->pd_quota / 10);
			break;
		case PS3_DEV_TYPE_SATA_SSD:
			if (qos_pd_mgr->pd_quota == 0) {
				qos_pd_mgr->pd_quota = qos_cfg_info->sataSsdQuota > 0 ?
						qos_cfg_info->sataSsdQuota : g_ps3_qos_ssd_pd_quota;
			}
			qos_pd_mgr->pd_init_quota = qos_pd_mgr->pd_quota;
			qos_pd_mgr->adjust_max_quota = qos_pd_mgr->pd_quota;
			qos_pd_mgr->adjust_min_quota = qos_pd_mgr->pd_quota - (qos_pd_mgr->pd_quota / 10);
			break;
		case PS3_DEV_TYPE_NVME_SSD:
			pd_ctx = &qos_pd_mgr->instance->qos_context.pd_ctx;
			if (qos_pd_mgr->pd_quota == 0) {
				qos_pd_mgr->pd_quota = qos_cfg_info->nvmeNormalQuota > 0 ?
						 qos_cfg_info->nvmeNormalQuota : pd_ctx->nvme_normal_quota;
			}
			if (qos_pd_mgr->direct_quota == 0) {
				qos_pd_mgr->direct_quota = qos_cfg_info->nvmeDirectQuota > 0 ?
						 qos_cfg_info->nvmeDirectQuota : g_ps3_qos_nvme_pd_quota;
			}
			qos_pd_mgr->pd_init_quota = qos_pd_mgr->direct_quota;
			qos_pd_mgr->adjust_max_quota = qos_pd_mgr->direct_quota;
			qos_pd_mgr->adjust_min_quota = qos_pd_mgr->direct_quota - (qos_pd_mgr->direct_quota / 10);
			break;
	}

	LOG_DEBUG("qos pd rsc init. hno:%u type:%u normal_quota:%u direct_quota:%u\n",
			PS3_HOST(qos_pd_mgr->instance), pd_entry->dev_type,
			pd_entry->normal_quota, pd_entry->direct_quota);
}

void ps3_qos_adjust_pd_rsc(struct scsi_device *sdev,
	struct ps3_instance *instance, S32 reason)
{
	S32 *quota = NULL;
	ULong flag = 0;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;

	if (!PS3_QOS_INITED(instance) ||
		(PS3_SDEV_PRI_DATA(sdev) == NULL) ||
		(PS3_SDEV_PRI_DATA(sdev)->dev_type == PS3_DEV_TYPE_VD)) {
		goto l_out;
	}

	qos_pd_mgr = ps3_qos_pd_mgr_get(instance, PS3_PDID(&PS3_SDEV_PRI_DATA(sdev)->disk_pos));

	switch (PS3_SDEV_PRI_DATA(sdev)->dev_type) {
		case PS3_DEV_TYPE_SAS_HDD:
		case PS3_DEV_TYPE_SAS_SSD:
		case PS3_DEV_TYPE_SATA_HDD:
		case PS3_DEV_TYPE_SATA_SSD:
			quota = &qos_pd_mgr->pd_quota;
			break;
		case PS3_DEV_TYPE_NVME_SSD:
			quota = &qos_pd_mgr->direct_quota;
			break;
		default:
			goto l_out;
	}
	if ((*quota > sdev->queue_depth && reason == PS3_QOS_QUOTA_ADJUST_QFULL) ||
		((*quota == qos_pd_mgr->adjust_max_quota ||
		time_after(sdev->last_queue_full_time + sdev->queue_ramp_up_period, jiffies) ||
		time_after(sdev->last_queue_ramp_up + sdev->queue_ramp_up_period, jiffies))
		&& reason == PS3_QOS_QUOTA_ADJUST_UP)) {
		goto l_out;
	}
	if (reason == PS3_QOS_QUOTA_ADJUST_UP) {
		ps3_spin_lock_irqsave(&qos_pd_mgr->adjust_quota_lock, &flag);
		if (*quota < qos_pd_mgr->adjust_max_quota) {
			(*quota)++;
		}
	} else if (reason == PS3_QOS_QUOTA_ADJUST_QFULL) {
		ps3_spin_lock_irqsave(&qos_pd_mgr->adjust_quota_lock, &flag);
		if (*quota > qos_pd_mgr->adjust_min_quota) {
			(*quota)--;
		}
	} else if (reason == PS3_QOS_QUOTA_ADJUST_DEFULAT) {
		ps3_spin_lock_irqsave(&qos_pd_mgr->adjust_quota_lock, &flag);
		*quota = qos_pd_mgr->adjust_max_quota;
	} else {
		goto l_out;
	}
	ps3_spin_unlock_irqrestore(&qos_pd_mgr->adjust_quota_lock, flag);
	LOG_INFO_IN_IRQ(instance,"hno:%u  dev[%u:%u] qos quota change to [%d] reason [%d]\n",
		PS3_HOST(instance), sdev->channel, sdev->id, *quota, reason);
l_out:
	return;
}

 struct ps3_qos_pd_mgr* ps3_qos_pd_mgr_init(struct ps3_instance *instance, struct ps3_pd_entry *pd_entry)
{
	U16 pd_id = 0;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	if (!PS3_QOS_INITED(instance)) {
		goto _out;
	}

	pd_id = PS3_PDID(&pd_entry->disk_pos);
	qos_pd_mgr = ps3_qos_pd_mgr_get(instance, pd_id);
	if ((ps3_atomic_add_unless(&qos_pd_mgr->valid, 1, 1) != 0) ||
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4,9,0) && LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0))
		((ps3_sas_is_support_smp(instance)) && (!ps3_check_pd_is_vd_member(pd_entry->config_flag))) ||
		(pd_entry->config_flag == MIC_PD_STATE_JBOD)) {
#else
		(pd_entry->config_flag == MIC_PD_STATE_JBOD)) {
#endif
		if (pd_entry->config_flag == MIC_PD_STATE_JBOD) {
			ps3_qos_vd_member_del(instance, &pd_entry->disk_pos);
		}

		ps3_qos_pd_rsc_init(qos_pd_mgr, pd_entry);
		qos_pd_mgr->dev_type = pd_entry->dev_type;
		qos_pd_mgr->disk_id = pd_id;
		qos_pd_mgr->vd_id = 0;
		qos_pd_mgr->workq_id = ps3_qos_pd_workq_id_get(&instance->qos_context.pd_ctx);
		qos_pd_mgr->clearing = PS3_FALSE;
		LOG_INFO_IN_IRQ(instance, "host_no:%u pd_id:%u dev_type:%u device qos init\n",
			PS3_HOST(instance), pd_id, qos_pd_mgr->dev_type);
	}
_out:
	return qos_pd_mgr;
}

void ps3_qos_pd_mgr_reset(struct ps3_instance *instance, U16 pd_id)
{
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	if (!PS3_QOS_INITED(instance)) {
		return;
	}

	qos_pd_mgr = ps3_qos_pd_mgr_get(instance, pd_id);
	qos_pd_mgr->clearing = PS3_FALSE;
}

void ps3_qos_vd_attr_change(struct ps3_instance *instance,
	struct PS3VDEntry *vd_entry_old, struct PS3VDEntry *vd_entry)
{
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	U16 change_type = PS3_QOS_VD_TYPE_CHANGE_INVALID;
	U16 i = 0;
	U16 j = 0;
	U16 pd_id = 0;

	if (!PS3_QOS_INITED(instance) || vd_entry->isNvme) {
		goto l_out;
	}

	if (!vd_entry->isSsd) {
		change_type = PS3_QOS_VD_TYPE_CHANGE_TO_HDD;
	} else {
		change_type = PS3_QOS_VD_TYPE_CHANGE_TO_SSD;
	}

	for (i = 0; i < vd_entry->spanCount; i++) {
		for (j = 0; j < vd_entry->span[i].spanPdNum; j++) {
			pd_id = vd_entry->span[i].extent[j].phyDiskID.ps3Dev.phyDiskID;
			if (pd_id <= PS3_MAX_PD_COUNT(instance)) {
				qos_pd_mgr = ps3_qos_pd_mgr_get(instance, pd_id);
				switch (change_type) {
					case PS3_QOS_VD_TYPE_CHANGE_TO_SSD:
						qos_pd_mgr->pd_quota = qos_pd_mgr->adjust_max_quota;
						break;
					case PS3_QOS_VD_TYPE_CHANGE_TO_HDD:
						if (vd_entry_old != NULL) {
							qos_pd_mgr->pd_quota = instance->cmd_context.max_scsi_cmd_count;
						} else {
							qos_pd_mgr->pd_quota = qos_pd_mgr->adjust_max_quota;
						}
						break;
					default:
						break;
				}
			}
		}
	}

	LOG_INFO_IN_IRQ(instance, "host_no:%u qos vd[%u:%u:%u] attr change:%u complete\n",
		PS3_HOST(instance), PS3_CHANNEL(&vd_entry->diskPos),
		PS3_TARGET(&vd_entry->diskPos), PS3_VDID(&vd_entry->diskPos), change_type);
l_out:
	return;
}

void ps3_qos_vd_member_change(struct ps3_instance *instance, struct ps3_pd_entry *pd_entry,
			 struct scsi_device *sdev, Bool is_vd_member)
{
	U16 pd_id = 0;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	struct ps3_scsi_priv_data *priv_data = NULL;
	if (!PS3_QOS_INITED(instance)) {
		goto _out;
	}

	pd_id = PS3_PDID(&pd_entry->disk_pos);
	ps3_mutex_lock(&instance->dev_context.dev_priv_lock);
	priv_data = PS3_SDEV_PRI_DATA(sdev);
	if (priv_data != NULL) {
		if (is_vd_member) { 
			ps3_hba_qos_pd_clean(instance, priv_data, PS3_STATUS_DEVICE_NOT_FOUND);
			ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
		} else { 
			ps3_hba_qos_pd_clean(instance, priv_data, PS3_STATUS_VD_MEMBER_OFFLINE);
			ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
			qos_pd_mgr = ps3_qos_pd_mgr_get(instance, pd_id);
			ps3_qos_pd_rsc_init(qos_pd_mgr, pd_entry);
			qos_pd_mgr->vd_id = 0;
		}
	} else {
		ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
	}
	LOG_INFO("update pd qos rsc. host_no:%u pd_id:%u dev_type:%u is_vd_member:%u\n",
		PS3_HOST(instance), pd_id, pd_entry->dev_type, is_vd_member);

_out:
	return;
}

void ps3_hba_qos_vd_init(struct ps3_instance *instance, struct PS3VDEntry *vd_entry)
{
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
	U16 vd_id = 0;

	vd_id = PS3_VDID(&vd_entry->diskPos);
	qos_vd_mgr = ps3_qos_vd_mgr_get_by_id(instance, vd_id);
	ps3_atomic_set(&qos_vd_mgr->vd_quota, g_ps3_qos_hdd_pd_quota * vd_entry->physDrvCnt);
	qos_vd_mgr->vd_entry = vd_entry;
	qos_vd_mgr->valid = PS3_TRUE;
}

static void ps3_qos_r1x_member_init(struct ps3_qos_pd_mgr *qos_pd_mgr,
	struct ps3_pd_entry *pd_entry, U16 judge_type)
{
	switch (judge_type) {
		case PS3_IS_SSD_ODD_R1X_VD:
			if (qos_pd_mgr->dev_type == PS3_DEV_TYPE_NVME_SSD) {
				qos_pd_mgr->direct_quota = qos_pd_mgr->pd_init_quota >> 1;
				qos_pd_mgr->adjust_max_quota = qos_pd_mgr->direct_quota;
				qos_pd_mgr->adjust_min_quota = qos_pd_mgr->direct_quota - (qos_pd_mgr->direct_quota / 10);
			} else {
				qos_pd_mgr->pd_quota = qos_pd_mgr->pd_init_quota >> 1;
				qos_pd_mgr->adjust_max_quota = qos_pd_mgr->pd_quota;
				qos_pd_mgr->adjust_min_quota = qos_pd_mgr->pd_quota - (qos_pd_mgr->pd_quota / 10);
			}
			break;
		case PS3_IS_SSD_EVEN_R1X_VD:
		case PS3_IS_VALID_R1X_VD:
			if (qos_pd_mgr->vd_id > 0) {
				ps3_qos_pd_rsc_init(qos_pd_mgr, pd_entry);
			}
			break;
		case PS3_IS_HDD_R1X_VD:
		default:
			break;
	}
}

void ps3_qos_vd_init(struct ps3_instance *instance, struct PS3VDEntry *vd_entry)
{
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	U16 vd_id = 0;
	U16 pd_id = 0;
	struct ps3_pd_entry *pd_entry = NULL;
	U16 i, j = 0;
	U16 judge_type = PS3_IS_VALID_R1X_VD;

	if(!PS3_QOS_INITED(instance)) {
		return;
	}

	vd_id = get_offset_of_vdid(PS3_VDID_OFFSET(instance),
					PS3_VDID(&vd_entry->diskPos));
	judge_type = ps3_odd_r1x_judge(vd_entry);
	for (i = 0; i < vd_entry->spanCount; i++) {
		for (j = 0; j < vd_entry->span[i].spanPdNum; j++) {
			pd_id = vd_entry->span[i].extent[j].phyDiskID.ps3Dev.phyDiskID;
			if (pd_id <= PS3_MAX_PD_COUNT(instance)) {
				pd_entry = ps3_dev_mgr_lookup_pd_info_by_id(instance, pd_id);
				if (pd_entry) {
					qos_pd_mgr = ps3_qos_pd_mgr_init(instance, pd_entry);
					ps3_qos_r1x_member_init(qos_pd_mgr, pd_entry, judge_type);
					if (qos_pd_mgr->dev_type == PS3_DEV_TYPE_NVME_SSD) {
						if (qos_pd_mgr->pd_quota > qos_pd_mgr->direct_quota) {
							qos_pd_mgr->pd_quota = qos_pd_mgr->direct_quota;
						}
					}
					qos_pd_mgr->vd_id = vd_id;
				}
			}
		}
	}

	if (instance->qos_context.opts.qos_vd_init) {
		instance->qos_context.opts.qos_vd_init(instance, vd_entry);
	}

	LOG_INFO_IN_IRQ(instance, "host_no:%u disk_pos[%u:%u:%u] device qos init\n",
		PS3_HOST(instance), PS3_CHANNEL(&vd_entry->diskPos),
		PS3_TARGET(&vd_entry->diskPos), vd_id);
}

void ps3_hba_qos_vd_reset(struct ps3_instance *instance, U16 disk_id)
{
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
	qos_vd_mgr = ps3_qos_vd_mgr_get_by_id(instance, disk_id);
	qos_vd_mgr->valid = PS3_FALSE;
}

void ps3_qos_vd_reset(struct ps3_instance *instance, U16 disk_id)
{
	if (!PS3_QOS_INITED(instance)) {
		return;
	}

	if (instance->qos_context.opts.qos_vd_reset) {
		instance->qos_context.opts.qos_vd_reset(instance, disk_id);
	}
}

static void ps3_qos_pd_notify_timeout(struct ps3_instance *instance)
{
	struct ps3_qos_pd_context *qos_pd_ctx = NULL;
	U16 i = 0;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	ULong timeout_jiffies = 0;

	qos_pd_ctx = &instance->qos_context.pd_ctx;
	for (i = 1; i <= instance->qos_context.max_pd_count; i++) {
		qos_pd_mgr = ps3_qos_pd_mgr_get(instance, i);
		timeout_jiffies = qos_pd_mgr->last_sched_jiffies + PS3_QOS_WAITQ_TIMEOUT * HZ;
		if (time_after(jiffies, timeout_jiffies)) {
			if (ps3_qos_single_pd_notify(qos_pd_ctx, qos_pd_mgr)) {
				LOG_INFO("awake qos pd quota waitq by poll. host_no:%u vid:%u pid:%u\n",
					PS3_HOST(instance), qos_pd_mgr->vd_id, qos_pd_mgr->disk_id);
			}
		}

	}
}

static Bool ps3_qos_vd_notify_timeout(struct ps3_instance *instance)
{
	struct ps3_qos_vd_context *qos_vd_ctx = NULL;
	U16 i = 0;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
	ULong timeout_jiffies = 0;
	Bool notified = PS3_FALSE;

	for (i = 1; i <= instance->qos_context.max_vd_count; i++) {
		qos_vd_ctx = &instance->qos_context.vd_ctx;
		qos_vd_mgr = &qos_vd_ctx->qos_vd_mgrs[i];
		timeout_jiffies = qos_vd_mgr->last_sched_jiffies + PS3_QOS_WAITQ_TIMEOUT * HZ;
		if (time_after(jiffies, timeout_jiffies)) {
		   	if (ps3_qos_single_vd_notify(qos_vd_ctx, qos_vd_mgr)) {
				notified = PS3_TRUE;
				LOG_INFO("awake qos vd quota waitq by poll. host_no:%u vid:%u\n",
				 	PS3_HOST(instance), qos_vd_mgr->id);
			}

		}

	}

	return notified;
}

static Bool ps3_qos_tg_notify_timeout(struct ps3_instance *instance)
{
	struct ps3_qos_tg_context *qos_tg_ctx = NULL;
	Bool notified = PS3_FALSE;
	ULong timeout_jiffies = 0;

	qos_tg_ctx = &instance->qos_context.tg_ctx;
	timeout_jiffies = qos_tg_ctx->last_sched_jiffies + PS3_QOS_WAITQ_TIMEOUT * HZ;
	if (qos_tg_ctx->total_wait_cmd_cnt &&
			ps3_qos_tag_rsc_available(instance) &&
 	 		time_after(jiffies, timeout_jiffies)) {
		queue_work(qos_tg_ctx->work_queue, &qos_tg_ctx->resend_work);
		notified = PS3_TRUE;
		LOG_INFO("awake qos cmd waitq by poll. host_no:%u\n",
			PS3_HOST(instance));
	}

	return notified;
}

void ps3_hba_qos_waitq_poll(struct ps3_instance *instance)
{
	if (!ps3_qos_tg_notify_timeout(instance)) {
		if (!ps3_qos_vd_notify_timeout(instance)) {
			ps3_qos_pd_notify_timeout(instance);
		}
	}
}

void ps3_qos_waitq_poll(struct ps3_instance *instance)
{
	if (!PS3_QOS_INITED(instance)) {
		return;
	}

	if (++instance->qos_context.poll_count == PS3_QOS_POLL_INTERVAL) {
		instance->qos_context.poll_count = 0;
		instance->qos_context.opts.qos_waitq_poll(instance);
	}
}

static void ps3_hba_qos_reset(struct ps3_instance *instance)
{
	struct ps3_qos_context *qos_ctx = &instance->qos_context;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	struct ps3_qos_tg_context *qos_tg_ctx = NULL;
	U16 i = 0;
	struct ps3_pd_entry *pd_entry = NULL;

	qos_tg_ctx = &instance->qos_context.tg_ctx;
	ps3_atomic_set(&qos_tg_ctx->mgr_free_cnt, qos_tg_ctx->mgr_exclusive_cnt);
	ps3_atomic_set(&qos_tg_ctx->mgr_share_used, 0);
	ps3_atomic_set(&qos_tg_ctx->share_free_cnt, qos_tg_ctx->share);

	for (i = 1; i <= qos_ctx->max_vd_count; i++) {
		qos_vd_mgr = &instance->qos_context.vd_ctx.qos_vd_mgrs[i];
		if (i == qos_ctx->max_vd_count) {
			ps3_atomic_set(&qos_vd_mgr->vd_quota, g_ps3_qos_vd_quota);
			ps3_atomic_set(&qos_vd_mgr->exclusive_cmd_cnt, g_ps3_qos_jbod_exclusive);
		} else {
			ps3_atomic_set(&qos_vd_mgr->exclusive_cmd_cnt, g_ps3_qos_vd_exclusive);
		}
		ps3_atomic_set(&qos_vd_mgr->share_cmd_used, 0);
	}

	qos_ctx->pd_ctx.nvme_normal_quota = g_ps3_qos_hba_nvme_normal_quota;
	for (i = 1; i <= qos_ctx->max_pd_count; i++) {
		qos_pd_mgr = ps3_qos_pd_mgr_get(instance, i);
		if (ps3_atomic_read(&qos_pd_mgr->valid) == 1) {
			pd_entry = ps3_dev_mgr_lookup_pd_info_by_id(instance, i);
			if (pd_entry != NULL) {
				ps3_qos_pd_rsc_init(qos_pd_mgr, pd_entry);
			}
		}
	}
}

void ps3_qos_close(struct ps3_instance *instance)
{
	if (!PS3_QOS_INITED(instance)) {
		return;
	}

	ps3_qos_wait_io_end(instance);
	instance->qos_context.opts.qos_waitq_clear(instance,
			PS3_SCSI_RESULT_HOST_STATUS(DID_SOFT_ERROR));
	LOG_INFO("qos close. host_no:%u\n", PS3_HOST(instance));
}

void ps3_qos_open(struct ps3_instance *instance)
{
	if (!PS3_QOS_INITED(instance)) {
		return;
	}

	instance->qos_context.opts.qos_reset(instance);
}

static U8 ps3_qos_waitq_flag_get(struct ps3_qos_softq_mgr *softq_mgr)
{
	U8 waitq_flag = 0;
	switch (softq_mgr->id) {
		case PS3_QOS_MGRQ:
			waitq_flag = PS3_QOS_CMD_IN_MGR;
			break;
		case PS3_QOS_CMDQ_0:
			waitq_flag = PS3_QOS_CMD_IN_CMDQ0;
			break;
		case PS3_QOS_CMDQ_1:
			waitq_flag = PS3_QOS_CMD_IN_CMDQ1;
			break;
		case PS3_QOS_CMDQ_2:
			waitq_flag = PS3_QOS_CMD_IN_CMDQ2;
			break;
		case PS3_QOS_CMDQ_3:
			waitq_flag = PS3_QOS_CMD_IN_CMDQ3;
			break;
		default:
			LOG_ERROR_IN_IRQ(softq_mgr->instance, "invalid softq id. host_no:%u id:%u\n",
				PS3_HOST(softq_mgr->instance), softq_mgr->id);
		}

	return 	waitq_flag;
}

static Bool ps3_qos_cq_rc_get(struct ps3_cmd *cmd,
			struct ps3_qos_softq_mgr *softq_mgr, struct qos_wait_queue *wait_q)
{
	Bool can_get = PS3_FALSE;
	ULong flag = 0;
	if (ps3_atomic_dec_return(&softq_mgr->free_cnt) >= 0) {
		can_get = PS3_TRUE;
	} else {
		ps3_atomic_inc(&softq_mgr->free_cnt);
	}

	if (!can_get) {
		ps3_spin_lock_irqsave(&softq_mgr->rc_lock, &flag);
		list_add_tail(&cmd->qos_list, &wait_q->wait_list);
		wait_q->count++;
		softq_mgr->total_wait_cmd_cnt++;
		cmd->qos_waitq_flag = ps3_qos_waitq_flag_get(softq_mgr);
		ps3_spin_unlock_irqrestore(&softq_mgr->rc_lock, flag);
		LOG_DEBUG("insert qos cq waitq.host_no:%u:t_id:0x%llx "
			"CFID:%u que_id:%u diskid:%u waitq:%u\n",
			PS3_HOST(cmd->instance), cmd->trace_id, cmd->index, softq_mgr->id,
			cmd->io_attr.disk_id, wait_q->count);
	}

	return can_get;
}

static struct qos_wait_queue* ps3_qos_cq_waitq_get(struct ps3_cmd *cmd,
				struct ps3_qos_softq_mgr *softq_mgr)
{
	U16 waitq_id = 0;
	struct qos_wait_queue *waitq = NULL;
	if (PS3_CMD_TYPE_IS_RW(cmd->cmd_word.type)) {
		if (cmd->io_attr.dev_type == PS3_DEV_TYPE_VD) {
		   waitq_id = get_offset_of_vdid(PS3_VDID_OFFSET(cmd->instance),
							PS3_VDID(&cmd->io_attr.vd_entry->diskPos));
		} else {
		   waitq_id = cmd->instance->qos_context.max_vd_count;
		}
		waitq = &softq_mgr->waitqs[waitq_id];
	} else {
		waitq = &softq_mgr->waitqs[0];
	}

	return waitq;
}

static Bool ps3_qos_cq_rc_check(struct ps3_cmd *cmd,
				struct ps3_qos_softq_mgr *softq_mgr)
{
	Bool can_get = PS3_FALSE;
	ULong flag = 0;
	struct qos_wait_queue *waitq = NULL;

	waitq = ps3_qos_cq_waitq_get(cmd, softq_mgr);
	if (likely(softq_mgr->total_wait_cmd_cnt == 0)) {
		can_get = ps3_qos_cq_rc_get(cmd, softq_mgr, waitq);
	} else {
		INJECT_START(PS3_ERR_IJ_QOS_WAIT_SOFT_WAITQ_CLEAR, cmd->instance)
		ps3_spin_lock_irqsave(&softq_mgr->rc_lock, &flag);
		if (softq_mgr->total_wait_cmd_cnt > 0) {
			list_add_tail(&cmd->qos_list, &waitq->wait_list);
			waitq->count++;
			softq_mgr->total_wait_cmd_cnt++;
			cmd->qos_waitq_flag = ps3_qos_waitq_flag_get(softq_mgr);
			ps3_spin_unlock_irqrestore(&softq_mgr->rc_lock, flag);
			can_get = PS3_FALSE;
			LOG_DEBUG("insert qos cq waitq.host_no:%u:t_id:0x%llx CFID:%u "
				"que_id:%u diskid:%u waitq:%u\n",
				PS3_HOST(cmd->instance), cmd->trace_id, cmd->index, softq_mgr->id,
				cmd->io_attr.disk_id, waitq->count);
		} else {
			ps3_spin_unlock_irqrestore(&softq_mgr->rc_lock, flag);
			can_get = ps3_qos_cq_rc_get(cmd, softq_mgr, waitq);
		}
	}

	return can_get;
}

static inline struct ps3_qos_softq_mgr *ps3_qos_cmdq_mgr_get(struct ps3_instance *instance, U16 index)
{
	return &instance->qos_context.cq_ctx.cmdqs[index];
}

static void ps3_qos_update_target_cmdq(struct ps3_cmd *cmd)
{
	U8 que_id = 0;
	U8 qmask = 0;

	cmd->cmdq_count = 0;
	qmask = cmd->cmd_word.qMask;
	while (que_id < cmd->instance->ctrl_info.vdQueueNum) {
		if (qmask & (1 << que_id)) {
			cmd->cmdq_info[cmd->cmdq_count].que_id = que_id;
			if (++cmd->cmdq_count == PS3_QOS_MAX_CMDQ_ONE_CMD) {
				break;
			}
		}
		que_id++;
	}
}

static Bool ps3_qos_cq_decision(struct ps3_cmd *cmd)
{
	Bool can_get = PS3_TRUE;
	struct ps3_instance *instance = cmd->instance;
	struct ps3_qos_softq_mgr *softq_mgr = NULL;
	U16 cmdq_index = 0;
	U8 i = 0;

	if (cmd->cmd_word.type == PS3_CMDWORD_TYPE_MGR) {
		PS3_QOS_STAT_START(cmd->instance, cmd, PS3_QOS_MGR_PRO);
		softq_mgr = &instance->qos_context.cq_ctx.mgrq;
		can_get = ps3_qos_cq_rc_check(cmd, softq_mgr);
		PS3_QOS_STAT_END(cmd->instance, cmd, PS3_QOS_MGR_PRO);
		if (!can_get) {
			PS3_QOS_STAT_START(cmd->instance, cmd, PS3_QOS_MGR_QUEUE);
		}
	} else {
		PS3_QOS_STAT_START(cmd->instance, cmd, PS3_QOS_CMD_PRO);
		ps3_qos_update_target_cmdq(cmd);
		for (i = 0; i < cmd->cmdq_count; i++) {
			cmdq_index = cmd->cmdq_info[i].que_id;
			softq_mgr = ps3_qos_cmdq_mgr_get(instance, cmdq_index);
			can_get = ps3_qos_cq_rc_check(cmd, softq_mgr);
			if (can_get) {
				cmd->cmdq_info[i].get_rc = PS3_TRUE;
			} else  {
				can_get = PS3_FALSE;
				break;
			}
		}
		PS3_QOS_STAT_END(cmd->instance, cmd, PS3_QOS_CMD_PRO);
		if (!can_get) {
			PS3_QOS_STAT_START(cmd->instance, cmd, PS3_QOS_CMD_QUEUE);
		}
	}

   return can_get;
}

Bool ps3_raid_qos_decision(struct ps3_cmd *cmd)
{
	Bool can_get = PS3_FALSE;
	if (PS3_CMD_TYPE_IS_RW(cmd->cmd_word.type)) {
		can_get = ps3_qos_pd_quota_decision(cmd); 
		if (!can_get ||  
				cmd->io_attr.direct_flag == PS3_CMDWORD_DIRECT_OK) {
			goto _out;
		}
	}
	can_get = ps3_qos_cq_decision(cmd); 
_out:
	return can_get;
}

static void ps3_qos_update_cmdq_rc(struct ps3_cmd *cmd)
{
	U8 i = 0;
	U8 index = 0;
	struct ps3_qos_softq_mgr *softq_mgr = NULL;
	S32 cmdq_free = 0;

	for (i = 0; i < cmd->cmdq_count; i++) {
		index = cmd->cmdq_info[i].que_id;
		softq_mgr = ps3_qos_cmdq_mgr_get(cmd->instance, index);
		cmdq_free = ps3_atomic_inc_return(&softq_mgr->free_cnt);
		LOG_DEBUG("update cmdq rc. host_no:%u t_id:0x%llx CFID:%u rc[%u,%d] dev_t:%u\n",
			 PS3_HOST(cmd->instance), cmd->trace_id, cmd->index, softq_mgr->id, cmdq_free,
			 cmd->io_attr.dev_type);
	}
}

static void ps3_qos_update_mgrq_rc(struct ps3_cmd *cmd)
{
	struct ps3_qos_cq_context *qos_cq_ctx = &cmd->instance->qos_context.cq_ctx;
	S32 free_cnt = 0;

	free_cnt = ps3_atomic_inc_return(&qos_cq_ctx->mgrq.free_cnt);
	LOG_DEBUG("update mgrq rc. host_no:%u t_id:0x%llx CFID:%u rc:%d\n",
		PS3_HOST(cmd->instance), cmd->trace_id, cmd->index, free_cnt);
}

static void ps3_raid_qos_cmd_update(struct ps3_cmd *cmd)
{
	if (cmd->cmd_word.type == PS3_CMDWORD_TYPE_MGR) {
		ps3_qos_update_mgrq_rc(cmd);
	} else {
		ps3_qos_update_pd_quota(cmd);
		ps3_qos_update_cmdq_rc(cmd);
	}
}

void ps3_qos_mgrq_resend(struct ps3_qos_softq_mgr *softq_mgr)
{
	S32 ret = PS3_SUCCESS;
	ULong flag = 0;
	struct ps3_cmd *cmd = NULL;
	struct ps3_instance *instance = softq_mgr->instance;
	struct qos_wait_queue *waitq = &softq_mgr->waitqs[0];
	Bool waitq_cleared = PS3_FALSE;

	ps3_spin_lock_irqsave(&softq_mgr->rc_lock, &flag);
	while (waitq->count > 0) {
		cmd = list_first_entry(&waitq->wait_list, struct ps3_cmd, qos_list);
		if (ps3_atomic_dec_return(&softq_mgr->free_cnt) < 0) {
			ps3_atomic_inc(&softq_mgr->free_cnt);
			break;
		}
		list_del(&cmd->qos_list);
		waitq->count--;
		softq_mgr->total_wait_cmd_cnt--;
		PS3_QOS_STAT_END(instance, cmd, PS3_QOS_MGR_QUEUE);
		ret = ps3_scsi_cmd_send(instance, cmd);
		if (unlikely(ret != PS3_SUCCESS)) {
			ps3_raid_qos_cmd_update(cmd);
			ps3_qos_cmd_resend_fail(cmd, ret);
		} else {
			cmd->qos_waitq_flag = 0;
			LOG_DEBUG("raid qos mgr waitq resend:host_no:%u t_id:0x%llx CFID:%u type:%u\n",
				PS3_HOST(instance), cmd->trace_id, cmd->index, cmd->cmd_word.type);
		}
	}

	if (softq_mgr->total_wait_cmd_cnt == 0) {
		waitq_cleared = PS3_TRUE;
	}
	ps3_spin_unlock_irqrestore(&softq_mgr->rc_lock, flag);

	if (waitq_cleared) {
		LOG_DEBUG("hno:%u resend all mgrq waited cmds\n",
			PS3_HOST(instance));
	}
	softq_mgr->last_sched_jiffies = jiffies;
}

static void ps3_qos_mgrq_resend_work(struct work_struct *work)
{
	struct ps3_qos_softq_mgr *softq_mgr =
			ps3_container_of(work, struct ps3_qos_softq_mgr, resend_work);
	ps3_qos_mgrq_resend(softq_mgr);
	return;
}

static S32 ps3_qos_mgrq_init(struct ps3_instance *instance, struct ps3_qos_softq_mgr *softq_mgr)
{
	S32 ret = PS3_SUCCESS;
	U32 mgrq_depth = 0;
	U32 high_mgr_cmd = PS3_QOS_HIGH_PRI_MGR_CMD_COUNT >> 1;
	char workq_name[PS3_QOS_MAX_WORKQ_NAME_LENGTH] = {0};

	mgrq_depth = instance->qos_context.cq_ctx.mgrq_depth;
	if (mgrq_depth <= high_mgr_cmd) {
		ret = -PS3_FAILED;
		LOG_ERROR("hno:%u:mgrq_depth is too small func:%u depth:%u\n",
			PS3_HOST(softq_mgr->instance), ps3_get_pci_function(instance->pdev),
			mgrq_depth);
		goto _out;
	}

	softq_mgr->id = PS3_QOS_MGRQ;
	softq_mgr->instance = instance;
	ps3_spin_lock_init(&softq_mgr->rc_lock);
	ps3_atomic_set(&softq_mgr->free_cnt, mgrq_depth - high_mgr_cmd);
	softq_mgr->waitq_cnt = 1;
	softq_mgr->waitqs = (struct qos_wait_queue *)ps3_vzalloc(instance, sizeof(struct qos_wait_queue));
	INJECT_START(PS3_ERR_IJ_QOS_MGRQ_INIT_FAIL_1, softq_mgr)
	if (softq_mgr->waitqs == NULL) {
		LOG_ERROR("hno:%u:Failed to kcalloc memory for waitqs\n",
			PS3_HOST(softq_mgr->instance));
		ret =  -PS3_FAILED;
		goto _out;
	}
	INIT_LIST_HEAD(&softq_mgr->waitqs[0].wait_list);
	softq_mgr->waitqs[0].count = 0;
	INIT_WORK(&softq_mgr->resend_work, ps3_qos_mgrq_resend_work);
	softq_mgr->total_wait_cmd_cnt = 0;
	softq_mgr->last_sched_jiffies = 0;
	snprintf(workq_name, PS3_QOS_MAX_WORKQ_NAME_LENGTH, "mgrq_wq_f%u",
				ps3_get_pci_function(instance->pdev));
	softq_mgr->work_queue = create_singlethread_workqueue(workq_name);
	INJECT_START(PS3_ERR_IJ_QOS_MGRQ_INIT_FAIL_2, softq_mgr)
	if (softq_mgr->work_queue == NULL) {
		LOG_ERROR("qos mgr workq create failed\n");
		ret = -PS3_FAILED;
		goto _release_waitq;
	}

	LOG_INFO("hno:%u:mgrq init success func:%u depth:%u\n",
		PS3_HOST(softq_mgr->instance), ps3_get_pci_function(instance->pdev),
		mgrq_depth);
	goto _out;
_release_waitq:
	ps3_vfree(instance, softq_mgr->waitqs);
	softq_mgr->waitqs = NULL;
_out:
	return ret;
}

static Bool ps3_qos_softq_notify(struct ps3_qos_softq_mgr *softq_mgr)
{
	Bool notified = PS3_FALSE;
	if (softq_mgr->total_wait_cmd_cnt > 0 &&
			ps3_atomic_read(&softq_mgr->free_cnt) > 0) {
		queue_work(softq_mgr->work_queue, &softq_mgr->resend_work);
		notified = PS3_TRUE;
	}

	return notified;
}

static Bool ps3_qos_cq_notify(struct ps3_instance *instance)
{
	U8 i = 0;
	Bool notified = PS3_FALSE;
	struct ps3_qos_cq_context *qos_cq_ctx = &instance->qos_context.cq_ctx;
	struct ps3_qos_softq_mgr *softq_mgr = NULL;

	softq_mgr = &qos_cq_ctx->mgrq;
	ps3_qos_softq_notify(softq_mgr);

	for (i = 0; i < qos_cq_ctx->cmdq_cnt; i++) {
		softq_mgr = ps3_qos_cmdq_mgr_get(instance, i);
		if (ps3_qos_softq_notify(softq_mgr)) {
			notified = PS3_TRUE;
		}
	}

	return notified;
}

void ps3_raid_qos_waitq_notify(struct ps3_instance *instance)
{
	if (!ps3_qos_cq_notify(instance)) {
		ps3_qos_pd_notify(instance);
	}
}

static Bool ps3_qos_cmdq_resend_judge(struct ps3_qos_softq_mgr *softq_mgr, struct ps3_cmd *cmd)
{
	Bool can_get = PS3_TRUE;
	struct ps3_instance *instance = NULL;
	struct ps3_qos_softq_mgr *next_softq_mgr = NULL;

	instance = cmd->instance;
	if (cmd->cmdq_count == 1) {
		cmd->cmdq_info[0].get_rc = PS3_TRUE;
	} else {
		if (cmd->cmdq_info[0].que_id == softq_mgr->id) {
			cmd->cmdq_info[0].get_rc = PS3_TRUE;
			next_softq_mgr = ps3_qos_cmdq_mgr_get(instance, cmd->cmdq_info[1].que_id);
			can_get = ps3_qos_cq_rc_check(cmd, next_softq_mgr);
			if (can_get) {
				cmd->cmdq_info[1].get_rc = PS3_TRUE;
			}
		} else {
			cmd->cmdq_info[1].get_rc = PS3_TRUE;
		}
	}

	if (can_get) {
		PS3_QOS_STAT_END(instance, cmd, PS3_QOS_CMD_QUEUE);
	}

	LOG_DEBUG("raid resend cmdq judge:host_no:%u t_id:0x%llx CFID:%u cmdq_id:%u ret:%u\n",
		PS3_HOST(cmd->instance), cmd->trace_id, cmd->index, softq_mgr->id, can_get);
	return can_get;
}

static void ps3_qos_resend_multi_cmd(struct ps3_qos_softq_mgr *softq_mgr,
			struct qos_wait_queue *waitq, S32 count)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	struct ps3_instance *instance = softq_mgr->instance;
	U32 can_send = 0;

	can_send = PS3_MIN(ps3_atomic_read(&softq_mgr->free_cnt), count);
	while(waitq->count > 0 && can_send > 0) {
		cmd = list_first_entry(&waitq->wait_list, struct ps3_cmd, qos_list);
		list_del(&cmd->qos_list);
		softq_mgr->total_wait_cmd_cnt--;
		waitq->count--;

		if (ps3_qos_vd_seq_check(cmd)) {
			continue;
		}
		can_send--;
		ps3_atomic_dec(&softq_mgr->free_cnt);
		if (ps3_qos_cmdq_resend_judge(softq_mgr, cmd)) {
			ret = ps3_scsi_cmd_send(instance, cmd);
			if (unlikely(ret != PS3_SUCCESS)) {
				ps3_raid_qos_cmd_update(cmd);
				ps3_qos_cmd_resend_fail(cmd, ret);
			} else {
				cmd->qos_waitq_flag = 0;
				LOG_DEBUG("raid qos cmdq waitq resend:host_no:%u t_id:0x%llx CFID:%u qid:%u\n",
					 PS3_HOST(instance), cmd->trace_id, cmd->index, waitq->id);
			}
		}
	}
}

static void ps3_qos_cmdq_resend(struct ps3_qos_softq_mgr *softq_mgr)
{
	ULong flag = 0;
	struct ps3_instance *instance = softq_mgr->instance;
	struct qos_wait_queue *waitq = NULL;
	U16 cur_que_id = softq_mgr->poll_que_id;

	ps3_spin_lock_irqsave(&softq_mgr->rc_lock, &flag);
	while (ps3_atomic_read(&softq_mgr->free_cnt) > 0
				&& softq_mgr->total_wait_cmd_cnt > 0) {
		waitq = &softq_mgr->waitqs[cur_que_id];
		if (waitq->count > 0) {
			ps3_qos_resend_multi_cmd(softq_mgr, waitq, softq_mgr->poll_cmd_cnt);
		}
		if (++cur_que_id > instance->qos_context.max_vd_count) {
			cur_que_id = 1;
		}
	}
	ps3_spin_unlock_irqrestore(&softq_mgr->rc_lock, flag);
	softq_mgr->poll_que_id = cur_que_id;
	softq_mgr->last_sched_jiffies = jiffies;
	ps3_qos_pd_notify(instance);
}

static void ps3_qos_cmdq_resend_work(struct work_struct *work)
{
	struct ps3_qos_softq_mgr *softq_mgr =
			ps3_container_of(work, struct ps3_qos_softq_mgr, resend_work);
	ps3_qos_cmdq_resend(softq_mgr);
	return;
}

static S32 ps3_qos_cmdq_init(struct ps3_instance *instance, struct ps3_qos_softq_mgr *softq_mgr)
{
	S32 ret = PS3_SUCCESS;
	U16 i = 0;
	struct qos_wait_queue *waitq = NULL;
	char workq_name[PS3_QOS_MAX_WORKQ_NAME_LENGTH] = {0};
	U32 cmdq_depth = 0;

	cmdq_depth = instance->qos_context.cq_ctx.cmdq_depth;
	softq_mgr->instance = instance;
	ps3_spin_lock_init(&softq_mgr->rc_lock);
	ps3_atomic_set(&softq_mgr->free_cnt, cmdq_depth);
	softq_mgr->waitq_cnt = instance->qos_context.max_vd_count + 1;
	softq_mgr->waitqs = (struct qos_wait_queue *)ps3_vzalloc(instance,
							softq_mgr->waitq_cnt * sizeof(struct qos_wait_queue));
	INJECT_START(PS3_ERR_IJ_QOS_CMDQ_INIT_FAIL_1, softq_mgr)
	if (softq_mgr->waitqs == NULL) {
		LOG_ERROR("hno:%u:Failed to kcalloc memory for waitqs\n",
			PS3_HOST(instance));
		ret =  -PS3_FAILED;
		goto _out;
	}
	memset(softq_mgr->waitqs, 0, (instance->qos_context.max_vd_count + 1) * sizeof(struct qos_wait_queue));
	for (i = 1; i < softq_mgr->waitq_cnt; i++) {
		waitq =  &softq_mgr->waitqs[i];
		waitq->id = i;
		INIT_LIST_HEAD(&waitq->wait_list);
		waitq->count = 0;
	}

	softq_mgr->poll_cmd_cnt = PS3_QOS_POLL_CMD_COUNT;
	softq_mgr->poll_que_id = 1;
	INIT_WORK(&softq_mgr->resend_work, ps3_qos_cmdq_resend_work);
	softq_mgr->total_wait_cmd_cnt = 0;
	softq_mgr->last_sched_jiffies = 0;
	snprintf(workq_name, PS3_QOS_MAX_WORKQ_NAME_LENGTH, "cmdq_wq_f%u_%u",
			ps3_get_pci_function(instance->pdev), softq_mgr->id);
	softq_mgr->work_queue = create_singlethread_workqueue(workq_name);
	INJECT_START(PS3_ERR_IJ_QOS_CMDQ_INIT_FAIL_2, softq_mgr)
	if (softq_mgr->work_queue == NULL) {
		LOG_ERROR("qos mgr workq create failed\n");
		ret = -PS3_FAILED;
		goto _release_waitq;
	}

	LOG_INFO("hno:%u:cmdq init success func:%u depth:%u id:%u\n",
		PS3_HOST(softq_mgr->instance), ps3_get_pci_function(instance->pdev),
		cmdq_depth, softq_mgr->id);
	goto _out;
_release_waitq:
	ps3_vfree(instance, softq_mgr->waitqs);
	softq_mgr->waitqs = NULL;
_out:
	return ret;
}

static void ps3_qos_softq_exit(struct ps3_instance *instance, struct ps3_qos_softq_mgr *softq_mgr)
{
	cancel_work_sync(&softq_mgr->resend_work);
	flush_workqueue(softq_mgr->work_queue);
	destroy_workqueue(softq_mgr->work_queue);
	ps3_vfree(instance, softq_mgr->waitqs);
	softq_mgr->waitqs = NULL;
}

static S32 ps3_qos_cq_context_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U16 i = 0;
	U16 j = 0;
	struct ps3_qos_cq_context *qos_cq_ctx = NULL;
	struct ps3_qos_softq_mgr *softq_mgr = NULL;
	U64 mgrq_depth = 0;
	U64 cmdq_depth = 0;

	qos_cq_ctx = &instance->qos_context.cq_ctx;
	ps3_mgrq_depth_get(instance, &mgrq_depth);
	INJECT_START(PS3_ERR_IJ_QOS_FORCE_MGRQ_ZERO_DEPTH, &mgrq_depth)
	if (mgrq_depth == 0) {
		ret = -PS3_FAILED;
		LOG_ERROR("hno:%u:mgrq_depth is invalid func:%u\n",
			PS3_HOST(instance),ps3_get_pci_function(instance->pdev));
		goto _out;
	}
	qos_cq_ctx->mgrq_depth = mgrq_depth;

	ps3_cmdq_depth_get(instance, &cmdq_depth);
	INJECT_START(PS3_ERR_IJ_QOS_FORCE_CMDQ_ZERO_DEPTH, &cmdq_depth)
	if (cmdq_depth == 0) {
		ret =  -PS3_FAILED;
		LOG_ERROR("hno:%u:cmdq_depth is invalid func:%u\n",
			PS3_HOST(instance),
			ps3_get_pci_function(instance->pdev));
		goto _out;
	}
	qos_cq_ctx->cmdq_depth = cmdq_depth;

	if (ps3_qos_mgrq_init(instance, &qos_cq_ctx->mgrq) != PS3_SUCCESS) {
		ret = -PS3_FAILED;
		goto _out;
	}

	qos_cq_ctx->cmdq_cnt = instance->ctrl_info.vdQueueNum;
	qos_cq_ctx->cmdqs = (struct ps3_qos_softq_mgr *)ps3_vzalloc(instance,
				qos_cq_ctx->cmdq_cnt * sizeof(struct ps3_qos_softq_mgr));
	if (qos_cq_ctx->cmdqs == NULL) {
		LOG_ERROR("hno:%u:Failed to kcalloc memory for cmdqs\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto _mgrq_exit;
	}

	for (i = 0; i < qos_cq_ctx->cmdq_cnt; i++) {
		softq_mgr = &qos_cq_ctx->cmdqs[i];
		softq_mgr->id = i;
		if (ps3_qos_cmdq_init(instance, softq_mgr) != PS3_SUCCESS) {
			ret = -PS3_FAILED;
			goto _cmdq_exit;
		}
	}

	LOG_INFO("hno:%u: qos cq context init success cmdq_cnt:%u\n",
		PS3_HOST(instance), qos_cq_ctx->cmdq_cnt);
	goto _out;
_cmdq_exit:
	for (j = 0; j < i; j++) {
		softq_mgr = &qos_cq_ctx->cmdqs[j];
		ps3_qos_softq_exit(instance, softq_mgr);
	}
	ps3_vfree(instance, qos_cq_ctx->cmdqs);
	qos_cq_ctx->cmdqs = NULL;
_mgrq_exit:
	ps3_qos_softq_exit(instance, &qos_cq_ctx->mgrq);
_out:
	return ret;
}

static void ps3_qos_cq_context_exit(struct ps3_instance *instance)
{
	struct ps3_qos_cq_context *qos_cq_ctx = NULL;
	U16 i = 0;
	struct ps3_qos_softq_mgr *softq_mgr = NULL;

	qos_cq_ctx = &instance->qos_context.cq_ctx;
	if (qos_cq_ctx->cmdqs != NULL) {
		ps3_qos_softq_exit(instance, &qos_cq_ctx->mgrq);
		for (i = 0; i < qos_cq_ctx->cmdq_cnt; i++) {
			softq_mgr = &qos_cq_ctx->cmdqs[i];
			ps3_qos_softq_exit(instance, softq_mgr);
		}
		ps3_vfree(instance, qos_cq_ctx->cmdqs);
		qos_cq_ctx->cmdqs = NULL;
	}

	LOG_INFO("hno:%u: qos cq context exit success\n", PS3_HOST(instance));
}

S32 ps3_raid_qos_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	ret = ps3_qos_cq_context_init(instance);
	if (ret != PS3_SUCCESS) {
		goto _out;
	}

	ret = ps3_qos_pd_context_init(instance);
	if (ret != PS3_SUCCESS) {
		goto _cq_ctx_exit;
	}

	goto _out;
_cq_ctx_exit:
	ps3_qos_tg_context_exit(instance);
_out:
	return ret;
}

static Bool ps3_qos_softq_abort(struct ps3_qos_softq_mgr *softq_mgr,
		 struct ps3_cmd *cmd, U8 waitq_flag)
{
	ULong lock_flag = 0;
	Bool found = PS3_FALSE;
	struct qos_wait_queue *waitq = NULL;

	waitq = ps3_qos_cq_waitq_get(cmd, softq_mgr);
	ps3_spin_lock_irqsave(&softq_mgr->rc_lock, &lock_flag);
	if (cmd->qos_waitq_flag == waitq_flag) {
		list_del(&cmd->qos_list);
		waitq->count--;
		softq_mgr->total_wait_cmd_cnt--;
		found = PS3_TRUE;
	}
	ps3_spin_unlock_irqrestore(&softq_mgr->rc_lock, lock_flag);

	return found;
}

static void ps3_qos_peer_cmdq_rc_update(struct ps3_cmd *cmd)
{
	U8 i = 0;
	struct ps3_qos_softq_mgr *softq_mgr = NULL;
	for (i = 0; i < cmd->cmdq_count; i++) {
		if (cmd->cmdq_info[i].get_rc) {
			softq_mgr = ps3_qos_cmdq_mgr_get(cmd->instance, cmd->cmdq_info[i].que_id);
			ps3_atomic_inc(&softq_mgr->free_cnt);
		}
	}
}

static Bool ps3_qos_cmdq_abort(struct ps3_cmd *cmd)
{
	U8 i = 0;
	U8 waitq_flag = 0;
	Bool found = PS3_FALSE;
	struct ps3_qos_softq_mgr *qos_softq_mgr = NULL;

	for (i = 0; i < cmd->cmdq_count; i++) {
		qos_softq_mgr = ps3_qos_cmdq_mgr_get(cmd->instance, cmd->cmdq_info[i].que_id);
		waitq_flag = ps3_qos_waitq_flag_get(qos_softq_mgr);
		if (cmd->qos_waitq_flag == waitq_flag) {
			found = ps3_qos_softq_abort(qos_softq_mgr, cmd, waitq_flag);
			if (found) {
				break;
			}
		}
	}

	return found;
}

Bool ps3_raid_qos_waitq_abort(struct ps3_cmd *cmd)
{
	Bool found = PS3_FALSE;
	struct ps3_qos_softq_mgr *softq_mgr = NULL;

	if (cmd->qos_waitq_flag == 0) {
		goto _out;
	}

	if (PS3_CMD_TYPE_IS_RW(cmd->cmd_word.type)) {
		if (cmd->qos_waitq_flag == PS3_QOS_CMD_IN_PD) {
			found = ps3_pd_quota_waiq_abort(cmd);
			if (found) {
				goto _out;
			}
		}

		found = ps3_qos_cmdq_abort(cmd);
		if (found) {
			ps3_qos_update_pd_quota(cmd);
			ps3_qos_peer_cmdq_rc_update(cmd);
		}
	} else {
		if (cmd->qos_waitq_flag == PS3_QOS_CMD_IN_MGR) {
			softq_mgr = &cmd->instance->qos_context.cq_ctx.mgrq;
			found = ps3_qos_softq_abort(softq_mgr, cmd, PS3_QOS_CMD_IN_MGR);
		}
	}

_out:
	return found;
}

static void ps3_qos_mgrq_clean(struct ps3_instance *instance,
			struct ps3_scsi_priv_data *priv_data, S32 resp_status)
{
	struct ps3_qos_softq_mgr *softq_mgr = NULL;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *cmd_next = NULL;
	struct qos_wait_queue *waitq = NULL;
	ULong flag = 0;

	softq_mgr = &instance->qos_context.cq_ctx.mgrq;
	waitq = &softq_mgr->waitqs[0];
	ps3_spin_lock_irqsave(&softq_mgr->rc_lock, &flag);
	if (waitq->count > 0) {
		list_for_each_entry_safe(cmd, cmd_next, &waitq->wait_list, qos_list) {
			if (priv_data == NULL ||
					priv_data == scsi_device_private_data(cmd->scmd)) {
				LOG_DEBUG("qos clean mgrq. hno:%u t_id:0x%llx CFID:%d\n",
					PS3_HOST(instance), cmd->trace_id, cmd->index);
				list_del(&cmd->qos_list);
				waitq->count--;
				softq_mgr->total_wait_cmd_cnt--;
				ps3_scsih_drv_io_reply_scsi(cmd->scmd, cmd, resp_status, PS3_FALSE);
			}
		}
	}
	ps3_spin_unlock_irqrestore(&softq_mgr->rc_lock, flag);
	if (priv_data == NULL) {
		cancel_work_sync(&softq_mgr->resend_work);
	}
}

static void ps3_qos_cmdq_clean(struct ps3_instance *instance, U16 disk_id,
			 struct ps3_scsi_priv_data *priv_data, S32 resp_status)
{
	U8 i = 0;
	struct ps3_qos_softq_mgr *softq_mgr = NULL;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *cmd_next = NULL;
	struct qos_wait_queue *waitq = NULL;
	ULong flag = 0;
	struct ps3_qos_cq_context *qos_cq_ctx = &instance->qos_context.cq_ctx;

	for (i = 0; i < qos_cq_ctx->cmdq_cnt; i++) {
		softq_mgr = ps3_qos_cmdq_mgr_get(instance, i);
		waitq = &softq_mgr->waitqs[disk_id];
		if (waitq->count > 0) {
			ps3_spin_lock_irqsave(&softq_mgr->rc_lock, &flag);
			list_for_each_entry_safe(cmd, cmd_next, &waitq->wait_list, qos_list) {
				if (priv_data == NULL ||
						priv_data == scsi_device_private_data(cmd->scmd)) {
					LOG_DEBUG("qos clean cmdq. hno:%u t_id:0x%llx CFID:%d cmdq:%u\n",
						PS3_HOST(instance), cmd->trace_id, cmd->index, softq_mgr->id);
					list_del(&cmd->qos_list);
					waitq->count--;
					softq_mgr->total_wait_cmd_cnt--;
					ps3_qos_update_pd_quota(cmd);
					ps3_scsih_drv_io_reply_scsi(cmd->scmd, cmd, resp_status, PS3_FALSE);
				}
			}
			ps3_spin_unlock_irqrestore(&softq_mgr->rc_lock, flag);
		}
	}
}

static void ps3_qos_cmdq_clear(struct ps3_instance *instance, S32 resp_status)
{
	U16 i = 0;
	U16 j = 0;
	struct ps3_qos_softq_mgr *softq_mgr = NULL;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *cmd_next = NULL;
	struct qos_wait_queue *waitq = NULL;
	ULong flag = 0;
	struct ps3_qos_cq_context *qos_cq_ctx = &instance->qos_context.cq_ctx;

	for (i = 0; i < qos_cq_ctx->cmdq_cnt; i++) {
		softq_mgr = ps3_qos_cmdq_mgr_get(instance, i);
		for (j = 1; j <= instance->qos_context.max_vd_count; j++) {
			waitq = &softq_mgr->waitqs[j];
			if (waitq->count > 0) {
				ps3_spin_lock_irqsave(&softq_mgr->rc_lock, &flag);
				list_for_each_entry_safe(cmd, cmd_next, &waitq->wait_list, qos_list) {
					LOG_DEBUG("qos clean cmdq. hno:%u t_id:0x%llx CFID:%d cmdq:%u\n",
						PS3_HOST(instance), cmd->trace_id, cmd->index, softq_mgr->id);
					list_del(&cmd->qos_list);
					waitq->count--;
					softq_mgr->total_wait_cmd_cnt--;
					ps3_qos_update_pd_quota(cmd);
					ps3_scsih_drv_io_reply_scsi(cmd->scmd, cmd, resp_status, PS3_FALSE);
				}
				ps3_spin_unlock_irqrestore(&softq_mgr->rc_lock, flag);
			}
		}
		cancel_work_sync(&softq_mgr->resend_work);
	}
}

static void ps3_raid_qos_pd_clean(struct ps3_instance *instance,
			struct ps3_scsi_priv_data *priv_data, S32 resp_status)
{
	U16 pd_id = 0;
	U16 vd_id = 0;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;

	pd_id = PS3_PDID(&priv_data->disk_pos);
	qos_pd_mgr = ps3_qos_pd_mgr_get(instance, pd_id);
	ps3_pd_quota_waitq_clean(qos_pd_mgr, 0, resp_status);

	if (!PS3_QOS_PD_IS_VD_MEMBER(qos_pd_mgr)) {
		vd_id = instance->qos_context.max_vd_count;
		ps3_qos_cmdq_clean(instance, vd_id, priv_data, resp_status);
		ps3_qos_mgrq_clean(instance, priv_data, resp_status);
	}

	LOG_INFO("qos clean vd. host_no:%u did:%u vid:%u\n",
		 PS3_HOST(instance), pd_id, vd_id);
}

static void ps3_raid_qos_vd_clean(struct ps3_instance *instance,
			struct ps3_scsi_priv_data *priv_data, S32 resp_status)
{
	U16 vd_id = 0;
	U16 pd_id = 0;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;

	vd_id = get_offset_of_vdid(PS3_VDID_OFFSET(instance), PS3_VDID(&priv_data->disk_pos));
	for (pd_id = 1; pd_id <= instance->qos_context.max_pd_count; pd_id++) {
		qos_pd_mgr = ps3_qos_pd_mgr_get(instance, pd_id);
		if (ps3_atomic_read(&qos_pd_mgr->valid) == 1 && PS3_QOS_PD_IS_VD_MEMBER(qos_pd_mgr)) {
			ps3_pd_quota_waitq_clean(qos_pd_mgr, vd_id, resp_status);
		}
	}

	ps3_qos_cmdq_clean(instance, vd_id, NULL, resp_status);
	ps3_qos_mgrq_clean(instance, priv_data, resp_status);

	LOG_FILE_INFO("qos clean vd. host_no:%u type:%u disk_id:%u\n",
		 PS3_HOST(instance), priv_data->dev_type, vd_id);
}

void ps3_raid_qos_waitq_clear_all(struct ps3_instance *instance, S32 resp_status)
{
	U16 i = 0;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;

	for (i = 1; i <= instance->qos_context.max_pd_count; i++) {
		qos_pd_mgr = ps3_qos_pd_mgr_get(instance, i);
		if (ps3_atomic_read(&qos_pd_mgr->valid) == 1) {
			ps3_pd_quota_waitq_clear_all(qos_pd_mgr, resp_status);
			cancel_work_sync(&qos_pd_mgr->resend_work);
		}
	}

	ps3_qos_cmdq_clear(instance, resp_status);

	ps3_qos_mgrq_clean(instance, NULL, resp_status);

	LOG_INFO("host_no:%u:clear all qos waitq\n",
		PS3_HOST(instance));
}

static Bool ps3_qos_softq_notify_timeout(struct ps3_qos_softq_mgr *qos_softq_mgr)
{
	Bool notified = PS3_FALSE;
	ULong timeout_jiffies = 0;

	timeout_jiffies = qos_softq_mgr->last_sched_jiffies + PS3_QOS_WAITQ_TIMEOUT * HZ;
	if (qos_softq_mgr->total_wait_cmd_cnt &&
			ps3_atomic_read(&qos_softq_mgr->free_cnt) > 0 &&
			time_after(jiffies, timeout_jiffies)) {
		queue_work(qos_softq_mgr->work_queue, &qos_softq_mgr->resend_work);
		notified = PS3_TRUE;
		LOG_INFO("awake qos softq by poll. host_no:%u q_id:%u\n",
			PS3_HOST(qos_softq_mgr->instance), qos_softq_mgr->id);
	}

	return notified;
}

static Bool ps3_qos_cq_notify_timeout(struct ps3_instance *instance)
{
	struct ps3_qos_cq_context *qos_cq_ctx = NULL;
	struct ps3_qos_softq_mgr *qos_softq_mgr = NULL;
	Bool notified = PS3_FALSE;
	U8 i = 0;

	qos_cq_ctx = &instance->qos_context.cq_ctx;
	ps3_qos_softq_notify_timeout(&qos_cq_ctx->mgrq);

	for (i = 0; i < qos_cq_ctx->cmdq_cnt; i++) {
		qos_softq_mgr = &qos_cq_ctx->cmdqs[i];
		if (ps3_qos_softq_notify_timeout(qos_softq_mgr)) {
			notified = PS3_TRUE;
		}
	}

	return notified;
}

void ps3_raid_qos_waitq_poll(struct ps3_instance *instance)
{
	if (!ps3_qos_cq_notify_timeout(instance)) {
		ps3_qos_pd_notify_timeout(instance);
	}
}

static void ps3_raid_qos_reset(struct ps3_instance *instance)
{
	struct ps3_qos_context *qos_ctx = &instance->qos_context;
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	struct ps3_qos_cq_context *qos_cq_ctx = NULL;
	struct ps3_qos_softq_mgr *qos_softq_mgr = NULL;
	U16 i = 0;
	struct ps3_pd_entry *pd_entry = NULL;

	qos_cq_ctx = &instance->qos_context.cq_ctx;
	qos_softq_mgr = &qos_cq_ctx->mgrq;
	ps3_atomic_set(&qos_softq_mgr->free_cnt,
			qos_ctx->cq_ctx.mgrq_depth - (PS3_QOS_HIGH_PRI_MGR_CMD_COUNT >> 1));
	for (i = 0; i < qos_cq_ctx->cmdq_cnt; i++) {
		qos_softq_mgr = &qos_cq_ctx->cmdqs[i];
		ps3_atomic_set(&qos_softq_mgr->free_cnt, qos_ctx->cq_ctx.cmdq_depth);
	}

	qos_ctx->pd_ctx.nvme_normal_quota = g_ps3_qos_raid_nvme_normal_quota;
	for (i = 1; i <= qos_ctx->max_pd_count; i++) {
		qos_pd_mgr = ps3_qos_pd_mgr_get(instance, i);
		if (ps3_atomic_read(&qos_pd_mgr->valid) == 1) {
			pd_entry = ps3_dev_mgr_lookup_pd_info_by_id(instance, i);
			if (pd_entry != NULL) {
				ps3_qos_pd_rsc_init(qos_pd_mgr, pd_entry);
			}
		}
	}
}

static Bool ps3_raid_qos_pd_resend_check(struct ps3_cmd *cmd)
{
	return ps3_qos_cq_decision(cmd);
}

void ps3_raid_qos_exit(struct ps3_instance *instance)
{
	ps3_qos_cq_context_exit(instance);
	ps3_qos_pd_context_exit(instance);

	LOG_WARN("raid qos exit. hno:%u\n", PS3_HOST(instance));
}

void ps3_hba_qos_prepare(struct ps3_instance *instance)
{
	instance->qos_context.opts.qos_init = ps3_hba_qos_init;
	instance->qos_context.opts.qos_exit = ps3_hba_qos_exit;
	instance->qos_context.opts.qos_decision = ps3_hba_qos_decision;
	instance->qos_context.opts.qos_cmd_update = ps3_hba_qos_cmd_update;
	instance->qos_context.opts.qos_waitq_notify = ps3_hba_qos_waitq_notify;
	instance->qos_context.opts.qos_pd_resend_check = ps3_hba_qos_pd_resend_check;
	instance->qos_context.opts.qos_waitq_abort = ps3_hba_qos_waitq_abort;
	instance->qos_context.opts.qos_vd_clean = ps3_hba_qos_vd_clean;
	instance->qos_context.opts.qos_pd_clean = ps3_hba_qos_pd_clean;
	instance->qos_context.opts.qos_waitq_poll = ps3_hba_qos_waitq_poll;
	instance->qos_context.opts.qos_waitq_clear = ps3_hba_qos_waitq_clear_all;
	instance->qos_context.opts.qos_reset = ps3_hba_qos_reset;
	instance->qos_context.opts.qos_vd_init = ps3_hba_qos_vd_init;
	instance->qos_context.opts.qos_vd_reset = ps3_hba_qos_vd_reset;
	instance->qos_context.qos_switch = 1;
	instance->qos_context.pd_ctx.nvme_normal_quota = g_ps3_qos_hba_nvme_normal_quota;
}

void ps3_raid_qos_prepare(struct ps3_instance *instance)
{
	instance->qos_context.opts.qos_init = ps3_raid_qos_init;
	instance->qos_context.opts.qos_exit = ps3_raid_qos_exit;
	instance->qos_context.opts.qos_decision = ps3_raid_qos_decision;
	instance->qos_context.opts.qos_cmd_update = ps3_raid_qos_cmd_update;
	instance->qos_context.opts.qos_waitq_notify = ps3_raid_qos_waitq_notify;
	instance->qos_context.opts.qos_pd_resend_check = ps3_raid_qos_pd_resend_check;
	instance->qos_context.opts.qos_waitq_abort = ps3_raid_qos_waitq_abort;
	instance->qos_context.opts.qos_vd_clean = ps3_raid_qos_vd_clean;
	instance->qos_context.opts.qos_pd_clean = ps3_raid_qos_pd_clean;
	instance->qos_context.opts.qos_waitq_poll = ps3_raid_qos_waitq_poll;
	instance->qos_context.opts.qos_waitq_clear = ps3_raid_qos_waitq_clear_all;
	instance->qos_context.opts.qos_reset = ps3_raid_qos_reset;
	instance->qos_context.opts.qos_vd_init = NULL;
	instance->qos_context.opts.qos_vd_reset = NULL;
	instance->qos_context.qos_switch = 1;
	instance->qos_context.pd_ctx.nvme_normal_quota = g_ps3_qos_raid_nvme_normal_quota;
}

S32 ps3_qos_decision(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	Bool can_get = PS3_TRUE;
	struct ps3_instance *instance = NULL;

	cmd->qos_processing = PS3_TRUE;
	wmb();
	instance = cmd->instance;
	if (!ps3_qos_enable(cmd->instance) || !PS3_QOS_INITED(instance)) {
		goto _exit;
	}

	ret = ps3_qos_pre_check(instance, cmd);
	if (ret != PS3_SUCCESS) {
		goto _exit;
	}

	can_get = instance->qos_context.opts.qos_decision(cmd);
	if (!can_get) {
		ret = -PS3_IN_QOS_Q;
		PS3_QOS_CMD_INC(instance);
	}
_exit:
	cmd->qos_processing = PS3_FALSE;
	return ret;
}

void ps3_qos_cmd_update(struct ps3_instance *instance,  struct ps3_cmd *cmd)
{
	if (!ps3_qos_enable(instance) || !PS3_QOS_INITED(instance)) {
		return;
	}

	instance->qos_context.opts.qos_cmd_update(cmd);
}

void ps3_qos_waitq_notify(struct ps3_instance *instance)
{

	if (!ps3_qos_enable(instance) || !PS3_QOS_INITED(instance)) {
		return;
	}
	instance->qos_context.opts.qos_waitq_notify(instance);
}

Bool ps3_qos_waitq_abort(struct ps3_cmd *cmd)
{
	Bool found = PS3_FALSE;
	if (!PS3_QOS_INITED(cmd->instance)) {
		goto out;
	}

	found = cmd->instance->qos_context.opts.qos_waitq_abort(cmd);
	LOG_INFO("task abort cmd in qos waitq t_id:0x%llx CFID:%u flag:%u ret:%u\n",
		cmd->trace_id, cmd->index, cmd->qos_waitq_flag, found);
	if (found) {
		ps3_scsih_drv_io_reply_scsi(cmd->scmd, cmd, SCSI_STATUS_TASK_ABORTED, PS3_FALSE);
	}
out:
	return found;
}

void ps3_qos_hard_reset(struct ps3_instance *instance)
{
	S32 resp_status = 0;
	if (ps3_pci_err_recovery_get(instance)) {
		resp_status = PS3_STATUS_PCI_RECOVERY;
	} else {
		resp_status = PS3_STATUS_HOST_RESET;
	}

	ps3_qos_waitq_clear_all(instance, resp_status);
}

void ps3_qos_waitq_clear_all(struct ps3_instance *instance, S32 resp_status)
{
	if (!PS3_QOS_INITED(instance)) {
		return;
	}

	ps3_qos_wait_io_end(instance);
	instance->qos_context.opts.qos_waitq_clear(instance, resp_status);
}

S32 ps3_qos_init(struct ps3_instance *instance)
{
	struct ps3_qos_context *qos_ctx = &instance->qos_context;
	S32 ret = PS3_SUCCESS;

	qos_ctx->max_vd_count = instance->ctrl_info.maxVdCount + 1;
	qos_ctx->max_pd_count = instance->ctrl_info.maxPdCount;
	qos_ctx->inited = 0;

	LOG_INFO("hno:%u ps3 qos info:tfifo:%u sas_pd_quota[%u %u] sata_pd_quota[%u %u] nvme_quota[%u %u %u]\n",
		PS3_HOST(instance), instance->ctrl_info.qosInfo.tfifoDepth,
		instance->ctrl_info.qosInfo.sasHddQuota,
		instance->ctrl_info.qosInfo.sasSsdQuota,
		instance->ctrl_info.qosInfo.sataHddQuota,
		instance->ctrl_info.qosInfo.sataSsdQuota,
		instance->ctrl_info.qosInfo.nvmeVdQuota,
		instance->ctrl_info.qosInfo.nvmeDirectQuota,
		instance->ctrl_info.qosInfo.nvmeNormalQuota);

	if (qos_ctx->opts.qos_init) {
		if (qos_ctx->opts.qos_init(instance) == PS3_SUCCESS) {
			qos_ctx->inited = 1;
			LOG_INFO("hno:%u:ps3 qos init success, vd_count:%u pd_count:%u\n",
				PS3_HOST(instance), qos_ctx->max_vd_count, qos_ctx->max_pd_count);
		} else {
			ret = -PS3_FAILED;
		}
	}

	return ret;
}

void ps3_qos_exit(struct ps3_instance *instance)
{
	if (instance->qos_context.opts.qos_exit) {
		instance->qos_context.opts.qos_exit(instance);
		instance->qos_context.inited = 0;
	}
}
