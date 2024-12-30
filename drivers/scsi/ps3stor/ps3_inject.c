
#include <linux/random.h>
#include "ps3_cli.h"
#include "ps3_err_inject.h"
#include "ps3_cmd_channel.h"
#include "ps3_mgr_cmd.h"
#include "ps3_module_para.h"
#include "ps3_scsih_cmd_parse.h"
#include "ps3_ioc_manager.h"
#include "ps3_scsih.h"
#include "ps3_inject.h"
#include "ps3_cmd_statistics.h"
#include "ps3_r1x_write_lock.h"

#ifdef PS3_SUPPORT_INJECT

static PS3Inject_s g_inject_list;
static PS3HitCmd_s g_hit_cmd_list;

PS3Inject_s * get_inject(void){
	return &g_inject_list;
}
PS3HitCmd_s * get_hit_inject(void){
	return &g_hit_cmd_list;
}

void ps3_inject_list_init(void)
{
	INIT_LIST_HEAD(&g_inject_list.scsi_rw_list);
	INIT_LIST_HEAD(&g_inject_list.scsi_task_list);
	INIT_LIST_HEAD(&g_inject_list.mgr_list);
	ps3_mutex_init(&g_inject_list.lock);
}

void ps3_hit_cmd_list_init(void)
{
	INIT_LIST_HEAD(&g_hit_cmd_list.scsi_rw_list);
	INIT_LIST_HEAD(&g_hit_cmd_list.scsi_task_list);
	INIT_LIST_HEAD(&g_hit_cmd_list.mgr_list);
}

void ps3_inject_init(void)
{
	ps3_hit_cmd_list_init();
	ps3_inject_list_init();
}

void ps3_inject_list_exit(void)
{
	struct inject_cmds_t *pitem = NULL;
	struct inject_cmds_t *pitem_next = NULL;
	ps3_mutex_lock(&g_inject_list.lock);
	list_for_each_entry_safe(pitem, pitem_next, &g_inject_list.scsi_rw_list, list) {
		list_del(&pitem->list);
		kfree(pitem);
	}

	list_for_each_entry_safe(pitem, pitem_next, &g_inject_list.scsi_task_list, list) {
		list_del(&pitem->list);
		kfree(pitem);
	}

	list_for_each_entry_safe(pitem, pitem_next, &g_inject_list.mgr_list, list) {
		list_del(&pitem->list);
		kfree(pitem);
	}
	ps3_mutex_unlock(&g_inject_list.lock);
	ps3_mutex_destroy(&g_inject_list.lock);
}

void ps3_hit_cmd_list_exit(void)
{
	struct inject_hit_cmds_t *pitem = NULL;
	struct inject_hit_cmds_t *pitem_next = NULL;

	ps3_mutex_lock(&g_inject_list.lock);
	list_for_each_entry_safe(pitem, pitem_next, &g_hit_cmd_list.scsi_rw_list, list) {
		list_del(&pitem->list);
		kfree(pitem);
	}

	list_for_each_entry_safe(pitem, pitem_next, &g_hit_cmd_list.scsi_task_list, list) {
		list_del(&pitem->list);
		kfree(pitem);
	}

	list_for_each_entry_safe(pitem, pitem_next, &g_hit_cmd_list.mgr_list, list) {
		list_del(&pitem->list);
		kfree(pitem);
	}
	ps3_mutex_unlock(&g_inject_list.lock);
	return;
}

void ps3_inject_exit(void)
{
	ps3_hit_cmd_list_exit();
	ps3_inject_list_exit();
}
void ps3_delete_scsi_rw_inject(struct inject_cmds_t *this_pitem)
{
	struct inject_cmds_t *pitem = NULL;
	struct inject_cmds_t *pitem_next = NULL;

	if (list_empty(&g_inject_list.scsi_rw_list)) {
		LOG_DEBUG("rw inject list is empty!\n");
		goto l_out;
	}

	list_for_each_entry_safe(pitem, pitem_next, &g_inject_list.scsi_rw_list, list) {
		if(this_pitem == pitem) {
			list_del(&pitem->list);
			kfree(pitem);
		}
	}

l_out:
	return;
}
void ps3_delete_scsi_task_inject(struct inject_cmds_t *this_pitem)
{
	struct inject_cmds_t *pitem = NULL;
	struct inject_cmds_t *pitem_next = NULL;

	if (list_empty(&g_inject_list.scsi_task_list)) {
		LOG_DEBUG("task inject list is empty!\n");
		goto l_out;
	}

	list_for_each_entry_safe(pitem, pitem_next, &g_inject_list.scsi_task_list, list) {
		if(this_pitem == pitem) {
			list_del(&pitem->list);
			kfree(pitem);
		}
	}

l_out:
	return;
}
void ps3_delete_mgr_inject(struct inject_cmds_t *this_pitem)
{
	struct inject_cmds_t *pitem = NULL;
	struct inject_cmds_t *pitem_next = NULL;

	if (list_empty(&g_inject_list.mgr_list)) {
		LOG_DEBUG("mgr inject list is empty!\n");
		goto l_out;
	}

	list_for_each_entry_safe(pitem, pitem_next, &g_inject_list.mgr_list, list) {
		if(this_pitem == pitem) {
			list_del(&pitem->list);
			kfree(pitem);
		}
	}

l_out:
	return;
}

Bool ps3_add_scsi_rw_cmd_filter(struct ps3_instance *instance, struct PS3CmdWord *cmd_word)
{
	struct ps3_cmd *cmd = ps3_cmd_find(instance, cmd_word->cmdFrameID);
	struct inject_cmds_t *pitem = NULL;
	struct inject_hit_cmds_t *p_cmd = NULL;
	struct inject_cmds_t *pitem_next = NULL;
	U64 lba;
	U32 len;
	Bool is_find = PS3_FALSE;

	ps3_mutex_lock(&g_inject_list.lock);
	if (list_empty(&g_inject_list.scsi_rw_list)) {
		goto l_out;
	}

	if (cmd == NULL) {
		LOG_DEBUG("[inject] tag[%u] cmd null!\n",cmd_word->cmdFrameID);
	}
	ps3_scsih_lba_parse(cmd->scmd->cmnd, &lba);
	ps3_scsih_len_parse(cmd->scmd->cmnd, &len);
	list_for_each_entry_safe(pitem, pitem_next, &g_inject_list.scsi_rw_list, list) {
		if ((cmd->instance->host->host_no ==  pitem->item.scsi_cmd.host_no) &&
			(cmd->scmd->device->channel == pitem->item.scsi_cmd.channel) &&
			(cmd->scmd->device->id == pitem->item.scsi_cmd.id)) {
			if (((lba >= pitem->item.scsi_cmd.lba) &&
				(lba <= pitem->item.scsi_cmd.lba + pitem->item.scsi_cmd.len)) ||
				((lba + len >= pitem->item.scsi_cmd.lba) &&
				(lba + len <= pitem->item.scsi_cmd.lba + pitem->item.scsi_cmd.len))) {
				LOG_DEBUG("[inject] add rw cmd [%u:%u:%u] lba[0x%llx], len[0x%x], times[%d], tag[%u]!\n",
					pitem->item.scsi_cmd.host_no, pitem->item.scsi_cmd.channel,
					pitem->item.scsi_cmd.id, lba,
					len, pitem->item.scsi_cmd.inject_count, cmd->index);
				is_find = PS3_TRUE;
				break;
			}
		}
	}
	if(!is_find){
		goto l_out;
	}

	p_cmd = (struct inject_hit_cmds_t *)kmalloc(sizeof(struct inject_hit_cmds_t), GFP_KERNEL);
	if (p_cmd == NULL) {
		is_find = PS3_FALSE;
		goto l_out ;
	}
	p_cmd->cmd = cmd;
	p_cmd->pitem = pitem;
	pitem->item.scsi_cmd.inject_count--;
	list_add_tail(&p_cmd->list, &g_hit_cmd_list.scsi_rw_list);
l_out:
	ps3_mutex_unlock(&g_inject_list.lock);
	return is_find;
}
Bool ps3_add_scsi_task_cmd_filter(struct ps3_instance *instance, struct PS3CmdWord *cmd_word)
{
	struct ps3_cmd *cmd;
	struct ps3_cmd *abort_cmd;
	U16 abort_cmd_id = 0;
	struct inject_cmds_t *pitem = NULL;
	struct inject_cmds_t *pitem_next = NULL;
	struct inject_hit_cmds_t *p_cmd = NULL;
	Bool is_find = PS3_FALSE;
	struct ps3_scsi_priv_data *scsi_priv_data = NULL;

	ps3_mutex_lock(&g_inject_list.lock);
	if (list_empty(&g_inject_list.scsi_task_list)) {
		goto l_out;
	}
	cmd = ps3_cmd_find(instance, cmd_word->cmdFrameID);
	if (cmd == NULL) {
		return is_find;
	}
	if (cmd->req_frame->taskReq.reqHead.cmdSubType == PS3_TASK_CMD_SCSI_TASK_ABORT
		&& cmd->req_frame->taskReq.reqHead.cmdType == PS3_CMD_SCSI_TASK_MANAGEMENT) {
		abort_cmd_id = cmd->req_frame->taskReq.taskID;
		abort_cmd= ps3_cmd_find(instance, abort_cmd_id);
		LOG_DEBUG("[inject] {%u:%u:%u} CFID[%u], abort CFID[%u]!\n",
			 abort_cmd->instance->host->host_no, abort_cmd->scmd->device->channel,
			 abort_cmd->scmd->device->id, cmd->index, abort_cmd_id);

		list_for_each_entry_safe(pitem, pitem_next, &g_inject_list.scsi_task_list, list) {
			if ((abort_cmd->instance->host->host_no ==  pitem->item.scsi_task_cmd.host_no) &&
				(abort_cmd->scmd->device->channel == pitem->item.scsi_task_cmd.channel) &&
				(abort_cmd->scmd->device->id == pitem->item.scsi_task_cmd.id)) {
				if (cmd->req_frame->taskReq.reqHead.cmdType == pitem->item.scsi_task_cmd.cmd_type &&
					cmd->req_frame->taskReq.reqHead.cmdSubType == pitem->item.scsi_task_cmd.cmd_sub_type) {
					LOG_DEBUG("[inject] add abort [%u:%u:%u] type[0x%x], subType[0x%x], CFID[%u], abort CFID[%u]!\n",
						pitem->item.scsi_task_cmd.host_no, pitem->item.scsi_task_cmd.channel,
						pitem->item.scsi_task_cmd.id, pitem->item.scsi_task_cmd.cmd_type,
						pitem->item.scsi_task_cmd.cmd_sub_type, cmd->index, abort_cmd_id);
					is_find = PS3_TRUE;
					if (pitem->item.scsi_task_cmd.dealType == PS3_SCSI_TASK_CMD_TIMEOUT) {
 					} else if (pitem->item.scsi_task_cmd.dealType == PS3_SCSI_TASK_CMD_ERROE) {
						INJECT_ACTIVE(PS3_ERR_IJ_ABORT_CMD_ERROR, pitem->item.scsi_task_cmd.inject_count)
					} else if (pitem->item.scsi_task_cmd.dealType == PS3_SCSI_TASK_CMD_NORMAL) {
						INJECT_ACTIVE(PS3_ERR_IJ_ABORT_CMD_NORMAL, pitem->item.scsi_task_cmd.inject_count)
					} else {
						LOG_DEBUG("[inject] dealType[%u] is invalid!\n",
							pitem->item.scsi_task_cmd.dealType);
					}
					break;
				}
			}
		}
	}else if (cmd->req_frame->taskReq.reqHead.cmdSubType == PS3_TASK_CMD_SCSI_TASK_RESET
	&& cmd->req_frame->taskReq.reqHead.cmdType == PS3_CMD_SCSI_TASK_MANAGEMENT) {
		LOG_DEBUG("[inject] type %d subType %d CFID[%u]!\n",
		cmd->req_frame->taskReq.reqHead.cmdType,cmd->req_frame->taskReq.reqHead.cmdSubType,
		cmd->index);
		list_for_each_entry_safe(pitem, pitem_next, &g_inject_list.scsi_task_list, list) {
			scsi_priv_data = PS3_SDEV_PRI_DATA(pitem->item.scsi_task_cmd.device);
			if (cmd->req_frame->taskReq.reqHead.cmdType == pitem->item.scsi_task_cmd.cmd_type &&
				cmd->req_frame->taskReq.reqHead.cmdSubType == pitem->item.scsi_task_cmd.cmd_sub_type &&
				(cmd->req_frame->taskReq.reqHead.devID.diskID == scsi_priv_data->disk_pos.diskDev.diskID)) {
				LOG_DEBUG("[inject] add reset [%u:%u:%u] type[0x%x], subType[0x%x], CFID[%u]!\n",
					pitem->item.scsi_task_cmd.host_no, pitem->item.scsi_task_cmd.channel,
					pitem->item.scsi_task_cmd.id, pitem->item.scsi_task_cmd.cmd_type,
					pitem->item.scsi_task_cmd.cmd_sub_type, cmd->index);
				is_find = PS3_TRUE;
				if (pitem->item.scsi_task_cmd.dealType == PS3_SCSI_TASK_CMD_TIMEOUT) {
 				} else if (pitem->item.scsi_task_cmd.dealType == PS3_SCSI_TASK_CMD_ERROE) {
					INJECT_ACTIVE(PS3_ERR_IJ_RESET_CMD_ERROR, pitem->item.scsi_task_cmd.inject_count)
				} else if (pitem->item.scsi_task_cmd.dealType == PS3_SCSI_TASK_CMD_NORMAL) {
					INJECT_ACTIVE(PS3_ERR_IJ_RESET_CMD_NORMAL, pitem->item.scsi_task_cmd.inject_count)
				} else {
					LOG_DEBUG("[inject] dealType[%u] is invalid!\n",
						pitem->item.scsi_task_cmd.dealType);
				}
				break;
			}
		}
	}
	if (!is_find){
		goto l_out;
	}

	p_cmd = (struct inject_hit_cmds_t *)kmalloc(sizeof(struct inject_hit_cmds_t), GFP_KERNEL);
	if (p_cmd == NULL) {
		is_find = PS3_FALSE;
		goto l_out ;
	}
	pitem->item.scsi_task_cmd.inject_count--;
	p_cmd->cmd = cmd;
	p_cmd->pitem = pitem;
	list_add_tail(&p_cmd->list, &g_hit_cmd_list.scsi_task_list);
l_out:
	ps3_mutex_unlock(&g_inject_list.lock);
	return is_find;
}

Bool ps3_add_mgr_cmd_filter(struct ps3_instance *instance, struct PS3CmdWord *cmd_word)
{
	struct ps3_cmd *cmd;
	struct inject_cmds_t *pitem = NULL;
	struct inject_cmds_t *pitem_next = NULL;
	struct inject_hit_cmds_t *p_cmd = NULL;
	Bool is_find = PS3_FALSE;

	ps3_mutex_lock(&g_inject_list.lock);
	if (list_empty(&g_inject_list.mgr_list)) {
		goto l_out;
	}
	cmd = ps3_cmd_find(instance, cmd_word->cmdFrameID);
	if (cmd == NULL) {
		return is_find;
	}
	if (cmd->req_frame->taskReq.reqHead.cmdType == PS3_CMD_SAS_MANAGEMENT
		|| cmd->req_frame->taskReq.reqHead.cmdType == PS3_CMD_MANAGEMENT) {
			LOG_DEBUG("[inject] hostno[%u], mgr CFID[%u], cmdType[%u], cmdSubType[%u]!\n",
			 cmd->instance->host->host_no, cmd->index, cmd->req_frame->taskReq.reqHead.cmdType,
			 cmd->req_frame->taskReq.reqHead.cmdSubType);

		list_for_each_entry_safe(pitem, pitem_next, &g_inject_list.mgr_list, list) {
			if ((cmd->instance->host->host_no ==  pitem->item.mgr_cmd.host_no) &&
				(cmd->req_frame->taskReq.reqHead.cmdType == pitem->item.mgr_cmd.cmd_type) &&
				(cmd->req_frame->taskReq.reqHead.cmdSubType == pitem->item.mgr_cmd.cmd_sub_type)) {
					LOG_DEBUG("[inject] add mgr type[0x%x], subType[0x%x], CFID[%u]!\n",
						pitem->item.mgr_cmd.cmd_type,
						pitem->item.mgr_cmd.cmd_sub_type, cmd->index);
					if (pitem->item.mgr_cmd.dealType == PS3_MGR_CMD_TIMEOUT) {
					} else if (pitem->item.mgr_cmd.dealType == PS3_SCSI_TASK_CMD_ERROE) {
						INJECT_ACTIVE(PS3_ERR_IJ_GET_LINKERRORS_CMD_ERROR, pitem->item.mgr_cmd.inject_count)
						INJECT_ACTIVE(PS3_ERR_IJ_PHY_CTRL_CMD_ERROR, pitem->item.mgr_cmd.inject_count)
						INJECT_ACTIVE(PS3_ERR_IJ_SMP_CMD_ERROR, pitem->item.mgr_cmd.inject_count)
						is_find = PS3_TRUE;
					} else {
						LOG_DEBUG("[inject] dealType[%u] is invalid!\n",
							pitem->item.mgr_cmd.dealType);
					}
					break;
			}
		}
	}

	if (!is_find){
		goto l_out;
	}

	p_cmd = (struct inject_hit_cmds_t *)kmalloc(sizeof(struct inject_hit_cmds_t), GFP_KERNEL);
	if (p_cmd == NULL) {
		is_find = PS3_FALSE;
		goto l_out ;
	}
	pitem->item.mgr_cmd.inject_count--;
	p_cmd->cmd = cmd;
	p_cmd->pitem = pitem;
	list_add_tail(&p_cmd->list, &g_hit_cmd_list.mgr_list);
l_out:
	ps3_mutex_unlock(&g_inject_list.lock);
	return is_find;
}

Bool ps3_add_cmd_filter(struct ps3_instance *instance, struct PS3CmdWord *cmd_word)
{
	Bool is_find = PS3_FALSE;
	struct ps3_cmd *cmd = ps3_cmd_find(instance, cmd_word->cmdFrameID);

	if(ps3_err_inject_state_query() == 0) {
		return is_find;
	}
	if(cmd == NULL) {
		LOG_DEBUG("[inject] CFID[%u] is invalid!\n",
			cmd->index);
		return is_find;
	}

	if ((cmd_word->type == PS3_CMDWORD_TYPE_READ) ||
		(cmd_word->type == PS3_CMDWORD_TYPE_WRITE)) {
		if (cmd->req_frame->taskReq.reqHead.cmdType != PS3_CMD_SCSI_TASK_MANAGEMENT)
		{
			is_find = ps3_add_scsi_rw_cmd_filter(instance, cmd_word);
		} else {
			is_find = ps3_add_scsi_task_cmd_filter(instance, cmd_word);
		}
	} else {
		if (cmd->req_frame->taskReq.reqHead.cmdType == PS3_CMD_SCSI_TASK_MANAGEMENT) {
			is_find = ps3_add_scsi_task_cmd_filter(instance, cmd_word);
		} else if (cmd->req_frame->taskReq.reqHead.cmdType == PS3_CMD_MANAGEMENT ||
			cmd->req_frame->taskReq.reqHead.cmdType == PS3_CMD_SAS_MANAGEMENT) {
			is_find = ps3_add_mgr_cmd_filter(instance, cmd_word);
		}
	}

	if (is_find) {
		LOG_DEBUG("[inject] add filter CFID[%u]!\n",
			cmd_word->cmdFrameID);
	}

	return is_find;
}
void ps3_inject_scsi_rw_force_stop(struct ps3_cmd *cmd)
{
			struct scsi_cmnd *s_cmd;
			S32 ret_code = 0 ;
			struct ps3_scsi_priv_data *data = NULL;
			ret_code = 0;

			if ((cmd->cmd_state.state == PS3_CMD_STATE_INIT) ||
				(cmd->cmd_state.state == PS3_CMD_STATE_COMPLETE)) {
				return;
			}
			s_cmd = cmd->scmd;
			LOG_DEBUG("[inject] hno:%u force stop scsi CFID[%d]\n",
				PS3_HOST(cmd->instance), cmd->index);

			PS3_IO_OUTSTAND_DEC(cmd->instance, s_cmd);
			PS3_VD_OUTSTAND_DEC(cmd->instance, s_cmd);
			PS3_DEV_IO_OUTSTAND_DEC(cmd->instance, cmd);
			PS3_IOC_DRV2IOC_BACK_INC(cmd->instance, cmd, PS3_REPLY_WORD_FLAG_FAIL);

			ps3_qos_cmd_update(cmd->instance, cmd);

#ifndef _WINDOWS
			PS3_IO_BACK_ERR_INC(cmd->instance, s_cmd);
			PS3_DEV_BUSY_DEC(s_cmd);
			s_cmd->result = ret_code;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0))
			s_cmd->SCp.ptr = NULL;
#endif
			ps3_scsi_dma_unmap(cmd);
			if (likely(cmd && cmd->scmd && cmd->scmd->device && cmd->scmd->device->hostdata)) {
				data = (struct ps3_scsi_priv_data *)cmd->scmd->device->hostdata;
				if (likely(data != NULL) &&
						data->lock_mgr.hash_mgr != NULL) {
					ps3_r1x_write_unlock(&data->lock_mgr, cmd);
				}
			} else {
				LOG_WARN("hno:%u force stop scsi has Null pointer CFID[%u]\n",
					PS3_HOST(cmd->instance), cmd->index);
			}
			if (cmd->r1x_peer_cmd != NULL) {
				LOG_DEBUG("[inject] hno:%u force stop r1x scsi CFID[%d] and CFID[%d]\n",
					PS3_HOST(cmd->instance), cmd->index, cmd->r1x_peer_cmd->index);
				if (!ps3_r1x_peer_cmd_free_nolock(cmd->r1x_peer_cmd)) {
					PS3_IOC_DRV2IOC_BACK_INC(cmd->instance, cmd->r1x_peer_cmd, PS3_REPLY_WORD_FLAG_FAIL);
					ps3_scsi_cmd_free(cmd->r1x_peer_cmd);
				}

				if (!ps3_r1x_peer_cmd_free_nolock(cmd)) {
					ps3_scsi_cmd_free(cmd);
				}
			} else {
				ps3_scsi_cmd_free(cmd);
			}
			SCMD_IO_DONE(s_cmd);
#else
			PS3_IO_BACK_ERR_INC(cmd->instance, s_cmd, cmd->index);
			scsi_cmnd_hoststatus_set(s_cmd, DID_RESET);
			data = scsi_device_private_data(s_cmd);
			if(likely(data != NULL) && data->lock_mgr.hash_mgr != NULL){
				ps3_r1x_write_unlock(&data->lock_mgr, cmd);
			}
			ps3_scsi_cmd_done(cmd);

#endif
return;
}
void ps3_task_abort_force_scsi_cmd(struct ps3_cmd *cmd)
{
	struct ps3_cmd *abort_cmd = NULL;
	struct inject_hit_cmds_t *p_cmd = NULL;
	struct inject_hit_cmds_t *p_cmd_next = NULL;
	ULong cmd_lock_flags = 0;
	struct inject_cmds_t *pitem = NULL;

	abort_cmd = ps3_cmd_find(cmd->instance, cmd->req_frame->taskReq.taskID);
	if (abort_cmd == NULL) {
		goto l_out;
	}
	list_for_each_entry_safe(p_cmd, p_cmd_next, &g_hit_cmd_list.scsi_rw_list, list) {
		if (abort_cmd == p_cmd->cmd) {
			LOG_DEBUG("[inject] hno:%u del scsi CFID [%u] from hit_rw_list\n",
				PS3_HOST(cmd->instance), abort_cmd->index);
			list_del(&p_cmd->list);
			pitem = p_cmd->pitem;
			kfree(p_cmd);
			break;
		}
	}

	if (pitem->item.scsi_cmd.inject_count == 0) {
		LOG_DEBUG("[inject] del inject_rw host_no %u channel %u id %u lba 0x%llx len 0x%u type %u, time %u!\n",
			pitem->item.scsi_cmd.host_no, pitem->item.scsi_cmd.channel,
			pitem->item.scsi_cmd.id, pitem->item.scsi_cmd.lba,
			pitem->item.scsi_cmd.len, pitem->item.scsi_cmd.dealType,
			pitem->item.scsi_cmd.inject_count);
		ps3_delete_scsi_rw_inject(pitem);
	}

	ps3_inject_scsi_rw_force_stop(abort_cmd);

l_out:

	cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &cmd_lock_flags);
	cmd->resp_frame->normalRespFrame.respStatus = SCSI_STATUS_GOOD;
	complete(&cmd->sync_done);
	LOG_DEBUG("[inject] hno:%u force complete task abort CFID:%d\n",
		PS3_HOST(cmd->instance), cmd->index);
	ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, cmd_lock_flags);
}

void ps3_task_reset_force_scsi_cmd(struct ps3_cmd *cmd)
{
	struct inject_hit_cmds_t *p_cmd = NULL;
	struct inject_hit_cmds_t *p_cmd_next = NULL;
	struct ps3_scsi_priv_data *scsi_priv_data;
	ULong cmd_lock_flags = 0;

	list_for_each_entry_safe(p_cmd, p_cmd_next, &g_hit_cmd_list.scsi_rw_list, list) {
		scsi_priv_data = scsi_device_private_data(p_cmd->cmd->scmd);
		if (scsi_priv_data->disk_pos.diskDev.diskID == cmd->req_frame->taskReq.reqHead.devID.diskID) {
			LOG_DEBUG("[inject] hno:%u del scsi CFID [%u] from hit_rw_list\n",
				PS3_HOST(cmd->instance), p_cmd->cmd->index);
			list_del(&p_cmd->list);
			ps3_inject_scsi_rw_force_stop(p_cmd->cmd);
			kfree(p_cmd);
		}
	}

	cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &cmd_lock_flags);
	cmd->resp_frame->normalRespFrame.respStatus = SCSI_STATUS_GOOD;
	complete(&cmd->sync_done);
	LOG_DEBUG("[inject] hno:%u force complete task reset CFID:%d\n",
		PS3_HOST(cmd->instance), cmd->index);
	ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, cmd_lock_flags);

}
#ifndef PS3_UT
S32 ps3_scsi_rw_cmd_filter_handle(struct scsi_cmnd *scmd)
{
	struct ps3_cmd *cmd = NULL;
	struct inject_hit_cmds_t *p_cmd = NULL;
	struct inject_hit_cmds_t *p_cmd_next = NULL;
	struct ps3_instance *instance = scsi_host_data(scmd);
	Bool is_find = PS3_FALSE;
	U16 reply_flags;
	S32 iRet = PS3_SUCCESS;
	struct inject_cmds_t *pitem = NULL;

	U8 type = PS3_SCSI_CMD_TYPE(ps3_scsih_cdb_rw_type_get(scmd->cmnd));
	if (!ps3_scsih_is_rw_type(type)) {
		goto l_out;
	}
	ps3_mutex_lock(&g_inject_list.lock);
	if (list_empty(&g_hit_cmd_list.scsi_rw_list)) {
		LOG_DEBUG("[inject] rw inject list is empty!\n");
		goto l_out;
	}

	cmd = ps3_cmd_find(instance, SCMD_GET_REQUEST(scmd)->tag);

	list_for_each_entry_safe(p_cmd, p_cmd_next, &g_hit_cmd_list.scsi_rw_list, list) {
		if (cmd == p_cmd->cmd) {
			is_find = PS3_TRUE;
			break;
		}
	}

	if (!is_find){
		goto l_out;
	}
	pitem = p_cmd->pitem;

	if (p_cmd->pitem->item.scsi_cmd.dealType == PS3_SCSI_CMD_TIMEOUT_FORCE_REPLY) {
		ps3_inject_scsi_rw_force_stop(cmd);
	} else if (p_cmd->pitem->item.scsi_cmd.dealType == PS3_SCSI_CMD_ERROR) {
		scmd->result = p_cmd->pitem->item.scsi_cmd.cmdDeal.errReply.result;
		scmd->sense_buffer[0] = p_cmd->pitem->item.scsi_cmd.cmdDeal.errReply.sshdr.resp_code;
		scmd->sense_buffer[1] = p_cmd->pitem->item.scsi_cmd.cmdDeal.errReply.sshdr.sense_key;
		scmd->sense_buffer[2] = p_cmd->pitem->item.scsi_cmd.cmdDeal.errReply.sshdr.asc;
		scmd->sense_buffer[3] = p_cmd->pitem->item.scsi_cmd.cmdDeal.errReply.sshdr.ascq;
		reply_flags = 1;
		if (cmd->cmd_receive_cb) {
			LOG_DEBUG("[inject]  sense: CFID:%d, rcode:0x%x, key:0x%x, asc:0x%x, ascq:0x%x\n",
				cmd->index, scmd->sense_buffer[0], scmd->sense_buffer[1],
				scmd->sense_buffer[2], scmd->sense_buffer[3]);
			iRet = cmd->cmd_receive_cb(cmd, reply_flags);
		} else {
			LOG_DEBUG("[inject] ps3 cmd index %d has no cmd_receive_cb\n",
				cmd->index);
		}
	} else {
		LOG_DEBUG("[inject] ps3 CFID %d dealType %d invalid\n",
			cmd->index,p_cmd->pitem->item.scsi_cmd.dealType);
	}
	list_del(&p_cmd->list);
	kfree(p_cmd);

	if (pitem->item.scsi_cmd.inject_count == 0) {
		ps3_delete_scsi_rw_inject(pitem);
	}

l_out:
	ps3_mutex_unlock(&g_inject_list.lock);
	return iRet;
}
S32 ps3_scsi_task_cmd_filter_handle(struct ps3_cmd *cmd)
{
	struct inject_cmds_t *pitem = NULL;
	struct ps3_instance *instance = cmd->instance;
	Bool is_find = PS3_FALSE;
	ULong cmd_lock_flags = 0;
	S32 iRet = PS3_SUCCESS;

	struct inject_hit_cmds_t *p_cmd = NULL;
	struct inject_hit_cmds_t *p_cmd_next = NULL;

	if (cmd->req_frame->taskReq.reqHead.cmdType != PS3_CMD_SCSI_TASK_MANAGEMENT) {
		goto l_out;
	}
	ps3_mutex_lock(&g_inject_list.lock);
	if (list_empty(&g_hit_cmd_list.scsi_task_list)) {
		LOG_DEBUG("[inject] task inject list is empty!\n");
		goto l_out;
	}

	list_for_each_entry_safe(p_cmd, p_cmd_next, &g_hit_cmd_list.scsi_task_list, list) {
		if (cmd == p_cmd->cmd) {
			list_del(&p_cmd->list);
			pitem = p_cmd->pitem;
			kfree(p_cmd);
			is_find = PS3_TRUE;
			break;
		}
	}

	if (!is_find){
		goto l_out;
	}

	if (p_cmd->pitem->item.scsi_task_cmd.dealType == PS3_SCSI_TASK_CMD_TIMEOUT) {
		LOG_DEBUG("[inject] hno:%u force task mgr CFID:%d timeout\n",
			PS3_HOST(instance), cmd->index);
	} else if (p_cmd->pitem->item.scsi_task_cmd.dealType == PS3_SCSI_TASK_CMD_ERROE) {
		cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
		ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &cmd_lock_flags);
		cmd->resp_frame->normalRespFrame.respStatus = PS3_DRV_MGR_TM_FAILED;
		complete(&cmd->sync_done);
		LOG_DEBUG("[inject] hno:%u force complete task mgr CFID:%d error\n",
			PS3_HOST(instance), cmd->index);
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, cmd_lock_flags);

	} else if (p_cmd->pitem->item.scsi_task_cmd.dealType == PS3_SCSI_TASK_CMD_NORMAL){
		if (cmd->req_frame->taskReq.reqHead.cmdSubType == PS3_TASK_CMD_SCSI_TASK_ABORT) {
			LOG_DEBUG("[inject] hno:%u force complete task abort CFID:%d normal reply\n",
				PS3_HOST(instance), cmd->index);
			ps3_task_abort_force_scsi_cmd(cmd);
		} else if(cmd->req_frame->taskReq.reqHead.cmdSubType == PS3_TASK_CMD_SCSI_TASK_RESET){
			LOG_DEBUG("[inject] hno:%u force complete task reset CFID:%d normal reply\n",
				PS3_HOST(instance), cmd->index);
			ps3_task_reset_force_scsi_cmd(cmd);
		} else {
			LOG_INFO("[inject] ps3 CFID %u type %d subType %d invalid\n",
				cmd->index,cmd->req_frame->taskReq.reqHead.cmdType,
				cmd->req_frame->taskReq.reqHead.cmdSubType);
		}
	}

	if(pitem->item.scsi_cmd.inject_count == 0) {
		ps3_delete_scsi_task_inject(pitem);
	}

l_out:
	ps3_mutex_unlock(&g_inject_list.lock);
	return iRet;
}

S32 ps3_mgr_cmd_filter_handle(struct ps3_cmd *cmd)
{
	struct inject_cmds_t *pitem = NULL;
	struct ps3_instance *instance = cmd->instance;
	Bool is_find = PS3_FALSE;
	ULong cmd_lock_flags = 0;
	S32 iRet = PS3_SUCCESS;
	struct inject_hit_cmds_t *p_cmd = NULL;
	struct inject_hit_cmds_t *p_cmd_next = NULL;

	if (cmd->req_frame->taskReq.reqHead.cmdType != PS3_CMD_MANAGEMENT &&
		cmd->req_frame->taskReq.reqHead.cmdType != PS3_CMD_SAS_MANAGEMENT) {
		goto l_out;
	}
	ps3_mutex_lock(&g_inject_list.lock);
	if (list_empty(&g_hit_cmd_list.mgr_list)) {
		LOG_DEBUG("[inject] task inject list is empty!\n");
		goto l_out;
	}

	list_for_each_entry_safe(p_cmd, p_cmd_next, &g_hit_cmd_list.mgr_list, list) {
		if (cmd == p_cmd->cmd) {
			list_del(&p_cmd->list);
			pitem = p_cmd->pitem;
			kfree(p_cmd);
			is_find = PS3_TRUE;
			break;
		}
	}

	if (!is_find){
		goto l_out;
	}

	if (p_cmd->pitem->item.mgr_cmd.dealType == PS3_MGR_CMD_TIMEOUT) {
		LOG_DEBUG("[inject] hno:%u force mgr CFID:%d timeout\n",
			PS3_HOST(instance), cmd->index);
	} else if (p_cmd->pitem->item.mgr_cmd.dealType == PS3_MGR_CMD_ERROE) {
		cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
		ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &cmd_lock_flags);
		cmd->resp_frame->normalRespFrame.respStatus = pitem->item.mgr_cmd.errType;
		complete(&cmd->sync_done);
		LOG_DEBUG("[inject] hno:%u force complete task mgr CFID:%d error\n",
			PS3_HOST(instance), cmd->index);
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, cmd_lock_flags);

	}

	if(pitem->item.mgr_cmd.inject_count == 0) {
		ps3_delete_mgr_inject(pitem);
	}

l_out:
	ps3_mutex_unlock(&g_inject_list.lock);
	return iRet;
}

#endif
#endif
