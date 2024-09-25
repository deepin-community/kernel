
#include "ps3_ioctl.h"

#ifdef _WINDOWS
#include <storport.h>

#define PS3_FUNC_FROM_CTL_CODE(ctrlCode)	(((ULong)(ctrlCode & 0x3FFC)) >> 2)
#define PS3_ACC_FROM_CTL_CODE(ctrlCode)	(((ULong)(ctrlCode & 0xC000)) >> 14)
#else

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uio.h>
#include <linux/irq_poll.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>

#include <scsi/scsi_host.h>
#endif

#include "ps3_cmd_statistics.h"
#include "ps3_htp.h"
#include "ps3_htp_def.h"
#include "ps3_mgr_cmd.h"
#include "ps3_driver_log.h"
#include "ps3_mgr_channel.h"
#include "ps3_mgr_cmd_err.h"
#include "ps3_util.h"
#include "ps3_err_inject.h"
#include "ps3_event.h"

S32 ps3_ioctl_init(struct ps3_instance*instance, S32 cmd_num)
{
#ifdef _WINDOWS
	(void)cmd_num;
	ps3_atomic_set(&instance->ioctl_count, 0);
#else
	sema_init(&instance->ioctl_sem, cmd_num);
#endif
	ps3_atomic_set(&instance->cmd_statistics.cli_cnt, PS3_CMD_STAT_INIT_VALUE);
	return PS3_SUCCESS;
}

static S32 ps3_ioctl_capbility_pre_check(struct ps3_instance *instance,
	struct PS3IoctlSyncCmd* ioc)
{
	S32 ret = PS3_SUCCESS;

	if (ioc->sgeCount > PS3_MAX_IOCTL_SGE_NUM || ioc->sgeCount == 0) {
		LOG_WARN("hno:%u ioctl req, SGE count [%d] >  max limit [%d], or SGE count = 0\n",
			PS3_HOST(instance), ioc->sgeCount, PS3_MAX_IOCTL_SGE_NUM);
		ret = -PS3_EINVAL;
		goto l_out;
	}

	(void)instance;
l_out:
	return ret;
}

void ps3_ioctl_buff_bit_pos_update(struct ps3_cmd *cmd)
{
	U8 i = 0;
	struct PS3Sge *pSge = (struct PS3Sge*)cmd->req_frame->mgrReq.sgl;
	struct ps3_instance *instance = cmd->instance;
	U8 bit_pos = 0;

	if (cmd->transient == NULL) {
		goto l_out;
	}

	if (cmd->transient->sge_num == 0) {
		goto l_out;
	}

	for (i = 0; i < cmd->transient->sge_num; i++) {
		if (cmd->transient->transient_buff[i] != NULL) {
			if (pSge->ext == 1) {
				pSge = (struct PS3Sge*)cmd->ext_buf;
			}
			bit_pos = ps3_dma_addr_bit_pos_check((dma_addr_t)le64_to_cpu(pSge->addr));
			if (bit_pos != instance->dma_addr_bit_pos) {
				pSge->addr = cpu_to_le64(PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, le64_to_cpu(pSge->addr)));
				pSge->addr = cpu_to_le64(PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos,
						le64_to_cpu(pSge->addr)));
			}
		}
		pSge++;
	}
l_out:
	return;
}

void ps3_ioctl_buff_release(struct ps3_cmd *cmd)
{
	U8 i = 0;
	struct PS3Sge *pSge = (struct PS3Sge*)cmd->req_frame->mgrReq.sgl;
	struct ps3_instance *instance = cmd->instance;

	if (cmd->transient == NULL) {
		return;
	}

	if (cmd->transient->sge_num == 0) {
		return;
	}

	for (i = 0; i < cmd->transient->sge_num; i++) {
		if (cmd->transient->transient_buff[i] != NULL) {
			if (pSge->ext == 1) {
				pSge = (struct PS3Sge*)cmd->ext_buf;
			}
			ps3_dma_free_coherent(instance,
				cpu_to_le32(pSge->length),
				cmd->transient->transient_buff[i],
				(dma_addr_t)cpu_to_le64(pSge->addr));

			cmd->transient->transient_buff[i] = NULL;
			memset(pSge, 0, sizeof(struct PS3Sge));
		}
		pSge++;
	}
	memset((void*)cmd->req_frame, 0, sizeof(union PS3ReqFrame));
	cmd->transient->sge_num = 0;
}

static S32 ps3_ioctl_sge_fill(struct ps3_instance *instance, U8 *base_addr,
	struct PS3Sge *psrc_sge, struct PS3Sge *pdst_sge, void **transient_buff)
{
	S32 ret = PS3_SUCCESS;
	dma_addr_t transient_addr = 0;

	*transient_buff = ps3_dma_alloc_coherent(instance,
		psrc_sge->length, &transient_addr);

	if (*transient_buff == NULL) {
		LOG_ERROR("hno:%u Failed to alloc kernel SGL buffer for IOCTL\n",
			PS3_HOST(instance));
		ret = -PS3_ENOMEM;
		goto l_out;
	}

	pdst_sge->length = cpu_to_le32(psrc_sge->length);
	pdst_sge->ext = 0;
	pdst_sge->lastSge = 0;
	pdst_sge->addr = (U64)cpu_to_le64(transient_addr);
#ifdef _WINDOWS
	LOG_DEBUG("===base_addr[%p],psrc_sge->addr[%d], psrc_sge->length[%d] \n",
		base_addr, psrc_sge->addr, psrc_sge->length);
	memcpy(*transient_buff, (void*)(base_addr + psrc_sge->addr), psrc_sge->length);
#else
	(void)base_addr;
	if (copy_from_user(*transient_buff, (void __user*)psrc_sge->addr, psrc_sge->length)) {
		LOG_ERROR("hno:%u, copy from user err\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}
#endif
l_out:
	return ret;
}

static S32 ps3_ioctl_sgl_fill(struct ps3_instance *instance,
	struct ps3_cmd *cmd, struct PS3IoctlSyncCmd* ioc)
{
	U8 i = 0;
	S32 ret = PS3_SUCCESS;
	struct PS3Sge *pdst_sge = NULL;
	struct PS3Sge *psrc_sge = (struct PS3Sge*)((U32*)ioc +
		ioc->sglOffset);
	U8 pre_sge_count = (U8)((ioc->sgeCount > PS3_FRAME_REQ_SGE_NUM_MGR) ?
		(PS3_FRAME_REQ_SGE_NUM_MGR - 1) : ioc->sgeCount);

	if (cmd->transient->sge_num != 0) {
		LOG_ERROR("trace_id[0x%llx], hno:%u got cmd NOK: %d\n",
			cmd->trace_id, PS3_HOST(instance), cmd->index);
		PS3_BUG();
		ret = -PS3_FAILED;
		goto l_out;
	}
	cmd->transient->sge_num = ioc->sgeCount;

	cmd->req_frame->mgrReq.sgeOffset =
		offsetof(PS3MgrReqFrame_s, sgl) >> PS3_MGR_CMD_SGL_OFFSET_DWORD_SHIFT;
	cmd->req_frame->mgrReq.sgeCount = ioc->sgeCount;

	pdst_sge = cmd->req_frame->mgrReq.sgl;

	for (i = 0; i < pre_sge_count; i++) {
		LOG_DEBUG("trace_id[0x%llx], hno:%u sge[%u] size : %d\n",
			cmd->trace_id, PS3_HOST(instance), i,
			psrc_sge->length);

		pdst_sge->length = 0;

		if (psrc_sge->length > 0) {
			ret = ps3_ioctl_sge_fill(instance, (U8*)ioc, psrc_sge, pdst_sge,
				&cmd->transient->transient_buff[i]);
			if (ret != PS3_SUCCESS) {
				LOG_ERROR("trace_id[0x%llx], hno:%u sge[%u] "
					"fill data error(%d)\n", cmd->trace_id,
					PS3_HOST(instance), i, ret);
				goto l_out;
			}
		}

		pdst_sge++;
		psrc_sge++;
	}

	if (ioc->sgeCount > PS3_FRAME_REQ_SGE_NUM_MGR) {
		LOG_FILE_INFO("hno:%u ioctl req num %d is bigger than req frame num %d\n",
			PS3_HOST(instance), ioc->sgeCount, PS3_FRAME_REQ_SGE_NUM_MGR);
		pdst_sge->addr = (U64)cpu_to_le64(cmd->ext_buf_phys);
		pdst_sge->length = 0;
		pdst_sge->ext = 1;

		pdst_sge = (struct PS3Sge*)cmd->ext_buf;
		cmd->req_frame->mgrReq.sgeCount++;

		for (i = PS3_FRAME_REQ_SGE_NUM_MGR - 1; i < ioc->sgeCount; i++) {
			LOG_DEBUG("trace_id[0x%llx], hno:%u sge[%u] size : %d\n",
				cmd->trace_id, PS3_HOST(instance), i,
				psrc_sge->length);

			pdst_sge->length = 0;

			if (psrc_sge->length > 0) {
				ret = ps3_ioctl_sge_fill(instance, (U8*)ioc, psrc_sge, pdst_sge,
					&cmd->transient->transient_buff[i]);
				if (ret != PS3_SUCCESS) {
					LOG_ERROR("trace_id[0x%llx], hno:%u sge[%u] fill data error(%d)\n",
						cmd->trace_id, PS3_HOST(instance), i,
						ret);
					goto l_out;
				}
			}

			pdst_sge++;
			psrc_sge++;
		}
	}

	pdst_sge--;
	pdst_sge->lastSge = 1;
l_out:
	return ret;
}

static S32 ps3_ioctl_complete(struct ps3_instance *instance,
	struct ps3_cmd *cmd,
	struct PS3IoctlSyncCmd* ioc)
{
	S32 ret = PS3_SUCCESS;
	U32 i = 0;

	struct PS3Sge *psrc_sge = (struct PS3Sge*)((U32*)ioc +
		ioc->sglOffset);

	for (i = 0; i < ioc->sgeCount; i++) {
		LOG_DEBUG("trace_id[0x%llx], hno:%u  CFID [%d], transient_buff[%u] address: %p, data size: %d\n",
			cmd->trace_id, PS3_HOST(instance), cmd->index,
			i, cmd->transient->transient_buff[i], psrc_sge->length);

		if (psrc_sge->length > 0) {
#ifdef _WINDOWS
			memcpy((void*)((U8*)ioc + psrc_sge->addr), cmd->transient->transient_buff[i], psrc_sge->length);
#else
			if (copy_to_user((void __user*)psrc_sge->addr,
				cmd->transient->transient_buff[i], psrc_sge->length)) {
				LOG_ERROR("hno:%u  copy to user err\n",
					PS3_HOST(instance));
				ret = -PS3_FAILED;
				goto l_out;
			}
#endif
		}
		psrc_sge++;
	}
#ifndef _WINDOWS
	l_out:
#endif
	LOG_FILE_INFO("trace_id[0x%llx], ioctl complete hno:%u  CFID [%u], ret[%d]\n",
		cmd->trace_id, PS3_HOST(instance), cmd->index, ret);
	return ret;
}

#ifdef _WINDOWS
static S32 ps3_ioctl_mgr_handle(struct ps3_instance *instance,
	struct PS3IoctlSyncCmd* ioc, PSCSI_REQUEST_BLOCK Srb)
#else
static S32 ps3_ioctl_mgr_handle(struct ps3_instance *instance,
	struct PS3IoctlSyncCmd* ioc)
#endif

{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
#ifndef _WINDOWS
	S32 send_result = PS3_SUCCESS;
#endif
	cmd = ps3_mgr_cmd_alloc(instance);
	INJECT_START(PS3_ERR_IJ_FORCE_IOCTL_ALLOC_CMD_FAILED, &cmd)
	if (cmd == NULL) {
		LOG_WARN("hno:%u ioctl req, not get a cmd packet\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_no_free_cmd;
	}
	LOG_DEBUG("trace_id[0x%llx], hno:%u CFID [%u], get mgr cmd succeed\n",
		cmd->trace_id, PS3_HOST(instance), cmd->index);

	cmd->time_out = 0;
	cmd->is_interrupt = PS3_DRV_TRUE;

	if (ioc->traceId != 0) {
		ps3_cmd_trace_id_replace(cmd, ioc->traceId);
		LOG_DEBUG("trace_id[0x%llx], hno:%u CFID [%u], change trace id from cli\n",
			cmd->trace_id, PS3_HOST(instance), cmd->index);
	}

	memset(&cmd->req_frame->mgrReq, 0, sizeof(struct PS3MgrReqFrame));

	memset(cmd->transient->transient_buff, 0, sizeof(void*) * PS3_MAX_IOCTL_SGE_NUM);
	ret = ps3_ioctl_sgl_fill(instance, cmd, ioc);
	if (ret != PS3_SUCCESS) {
		goto l_out;
	}

	ps3_ioctl_req_frame_build(cmd);
	ps3_mgr_cmd_word_build(cmd);

#ifdef _WINDOWS
	ps3_cmd_bind_srb_extension(cmd, Srb);
	cmd->cmd_receive_cb = ps3_ioctl_callback_proc;
	cmd->cmd_word.noReplyWord = PS3_CMD_WORD_NEED_REPLY_WORD;
	goto l_no_free_cmd;
#else
	INJECT_START(PS3_ERR_IJ_IOCTL_WAIT_IRQ_DISABLE, cmd)
	send_result = ps3_cmd_send_sync(instance, cmd);
	INJECT_START(PS3_ERR_IJ_IOCTL_CMD_NO_RESP, &send_result)
	ret = ps3_mgr_complete_proc(instance, cmd, send_result);
	if (ret == -PS3_CMD_NO_RESP) {
		LOG_ERROR("trace_id[0x%llx], hno:%u ioctl request no response!\n",
			cmd->trace_id, PS3_HOST(instance));
		goto l_no_free_cmd;
	}

	if (ret != PS3_SUCCESS) {
		LOG_ERROR("trace_id[0x%llx], hno:%u ioctl send request NOK!%d\n",
			cmd->trace_id, PS3_HOST(instance), ret);
		goto l_out;
	}

	ret = ps3_ioctl_complete(instance, cmd, ioc);

	LOG_INFO("trace_id[0x%llx], hno:%u  CFID [%u], ioctl cmd_type[%d], sub_type[%d] end\n",
		cmd->trace_id, PS3_HOST(instance), cmd->index,
		ioc->msg.header.cmdType, ioc->msg.header.cmdSubType);
#endif

l_out:
	ps3_ioctl_buff_release(cmd);
	ps3_mgr_cmd_free(instance, cmd);
l_no_free_cmd:
	return ret;
}

#ifdef _WINDOWS

static U8 ps3_ioctl_windows_pre_check(struct ps3_instance *instance,
	PPS3_IO_CONTROL ioctl)
{
	U8 ret = PS3_TRUE;
	U8 i = 0;
	U8 *base_addr = (U8*)&ioctl->ps3Ioctl;
	U8 *last_addr = base_addr + ioctl->SrbHeader.Length;

	if (ioctl->ps3Ioctl.data == NULL || ioctl->SrbHeader.Length == 0) {
		LOG_WARN("hno:%u ioctl req, data is [%p], or data len is [%llu]\n",
			PS3_HOST(instance), ioctl->ps3Ioctl.data, ioctl->SrbHeader.Length);
		ret = PS3_FALSE;
		goto l_out;
	}

	if (ps3_ioctl_capbility_pre_check(instance, &ioctl->ps3Ioctl) != PS3_SUCCESS) {
		ret = PS3_FALSE;
		goto l_out;
	}

	LOG_INFO("hno:%u base_addr[%p], Length[%d], last_addr[%p]\n",
		PS3_HOST(instance), base_addr, ioctl->SrbHeader.Length, last_addr);

	for (i = 0; i < ioctl->ps3Ioctl.sgeCount; i++) {
		if (base_addr + ioctl->ps3Ioctl.Sgl[i].addr +
			ioctl->ps3Ioctl.Sgl[i].length > last_addr) {

			LOG_ERROR("hno:%u ioctl req, sge[%d] len[%d] > total len[%d], sge offset[%p], base addr[%p]\n",
				PS3_HOST(instance), i, ioctl->ps3Ioctl.Sgl[i].length, ioctl->SrbHeader.Length,
				ioctl->ps3Ioctl.Sgl[i].addr, &ioctl->ps3Ioctl);
			ret = PS3_FALSE;
			goto l_out;
		}
	}

	(void)instance;

l_out:
	return ret;
}

S32 ps3_ioctl_callback_proc(struct ps3_cmd *cmd, U8 reply_flags)
{
	ULong flags = 0;
	S32 ret = PS3_SUCCESS;
	struct ps3_instance *instance = cmd->instance;
	S32 send_result = -PS3_RESP_ERR;
	SCSI_REQUEST_BLOCK *srb = cmd->srb;
	PPS3_IO_CONTROL ps3_ioctl = NULL;

	LOG_DEBUG("hno:%u trace_id[0x%llx], got a ioctl response, reply_flags[0x%x]\n",
		PS3_HOST(instance), cmd->trace_id, reply_flags);

	PS3_MGR_CMD_BACK_INC(instance, cmd, reply_flags);

	if (unlikely(reply_flags != PS3_REPLY_WORD_FLAG_SUCCESS) &&
			cmd->retry_cnt < PS3_ERR_MGR_CMD_FAULT_RETRY_MAX) {
		LOG_ERROR("trace_id[0x%llx], hno:%u  ioctl cmd return failed\n",
			cmd->trace_id, PS3_HOST(instance));

		ret = ps3_err_mgr_cmd_proc(instance, send_result, cmd);
		if (ret == -PS3_CMD_NO_RESP) {
			ret = ps3_mgr_cmd_no_resp_proc(instance, cmd);
		}

		if (ret != -PS3_RETRY) {
			LOG_WARN("hno:%u  CFID:%d retry, retries:%d, ret:%d\n",
				PS3_HOST(instance), cmd->index, cmd->retry_cnt, ret);
			goto l_out;
		}

		ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
		cmd->cmd_state.state = PS3_CMD_STATE_PROCESS;
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
		cmd->retry_cnt++;
		if (cmd->retry_cnt >= PS3_ERR_MGR_CMD_FAULT_RETRY_MAX) {
			LOG_ERROR("hno:%u  retry time full.\n",
				PS3_HOST(instance));
			ret = -PS3_FAILED;
			goto l_out;
		}
		LOG_INFO("hno:%u  mgr CFID[%d] retry:%u\n",
			PS3_HOST(instance), cmd->index, cmd->retry_cnt);

		ret = ps3_srb_send(instance, srb);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("trace_id[0x%llx], hno:%u ioctl send request failed!%d\n",
				cmd->trace_id, PS3_HOST(instance), ret);
			goto l_out;
		}

		goto l_retry;
	}

l_out:
	SrbSetSrbStatus(srb, SRB_STATUS_ERROR);
	if((cmd->resp_frame->normalRespFrame.respStatus == PS3_DRV_MGR_RESTART_COMMAND_RSP) &&
        (reply_flags == PS3_REPLY_WORD_FLAG_FAIL)){
        reply_flags = PS3_REPLY_WORD_FLAG_SUCCESS;
	}
	if (likely(reply_flags == PS3_REPLY_WORD_FLAG_SUCCESS)) {
		ps3_ioctl = (PPS3_IO_CONTROL)SrbGetDataBuffer(srb);
		ps3_ioctl_complete(instance, cmd, &ps3_ioctl->ps3Ioctl);
		SrbSetSrbStatus(srb, SRB_STATUS_SUCCESS);
	}

	ps3_ioctl_buff_release(cmd);
	ps3_mgr_cmd_free(instance, cmd);
	StorPortNotification(RequestComplete, instance, srb);
	ps3_atomic_dec(&instance->ioctl_count);

l_retry:
	LOG_DEBUG("hno:%u trace_id[0x%llx], end proc a ioctl response, ret[%d]\n",
		PS3_HOST(instance), cmd->trace_id, ret);

	return ret;
}

U8 ps3_ioctl_build_io(_In_ struct ps3_instance *instance, _In_ PSCSI_REQUEST_BLOCK Srb)
{
	U8 ret = PS3_FALSE;
	PPS3_IO_CONTROL ps3_ioctl = (PPS3_IO_CONTROL)SrbGetDataBuffer(Srb);

	LOG_INFO("ioctl parase: Type[0x%x], Method[0x%x], Function[0x%x], Access[0x%x] \n",
		DEVICE_TYPE_FROM_CTL_CODE(ps3_ioctl->SrbHeader.ControlCode), METHOD_FROM_CTL_CODE(ps3_ioctl->SrbHeader.ControlCode),
		PS3_FUNC_FROM_CTL_CODE(ps3_ioctl->SrbHeader.ControlCode), PS3_ACC_FROM_CTL_CODE(ps3_ioctl->SrbHeader.ControlCode));

	if (memcmp(ps3_ioctl->SrbHeader.Signature, PS3_IOCTL_SIG, sizeof(PS3_IOCTL_SIG)) != 0)	{
		LOG_ERROR("hno:%u ioctl signature error\n",
			PS3_HOST(instance));
		SrbSetSrbStatus(Srb, SRB_STATUS_INVALID_REQUEST);
		goto l_out;
	}

	if (!instance->state_machine.is_load) {
		LOG_WARN("trace_id[0x%llx], hno:%u instance state not is_load\n",
			ps3_ioctl->ps3Ioctl.traceId, PS3_HOST(instance));
		SrbSetSrbStatus(Srb, SRB_STATUS_NO_HBA);
		goto l_out;
	}

	if (!ps3_atomic_add_unless(&instance->ioctl_count, 1, PS3_MAX_IOCTL_CMDS)) {
		LOG_WARN("hno:%u ioctl concurrency full\n",
			PS3_HOST(instance));

		SrbSetSrbStatus(Srb, SRB_STATUS_BUSY);
		goto l_out;
	}

	if (!ps3_is_instance_state_allow_cmd_execute(instance)) {
		SrbSetSrbStatus(Srb, SRB_STATUS_NO_DEVICE);
		goto l_out;
	}

	ret = ps3_ioctl_windows_pre_check(instance, ps3_ioctl);
	if (!ret) {
		LOG_WARN("hno:%u ioctl capblity not support\n",
			PS3_HOST(instance));
		SrbSetSrbStatus(Srb, SRB_STATUS_INVALID_REQUEST);
		ps3_atomic_dec(&instance->ioctl_count);
		goto l_out;
	}

	if (ps3_ioctl_mgr_handle(instance, &ps3_ioctl->ps3Ioctl, Srb) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u ioctl handle err\n",
			PS3_HOST(instance));
		SrbSetSrbStatus(Srb, SRB_STATUS_ERROR);
		ps3_atomic_dec(&instance->ioctl_count);
		goto l_out;
	}

	ret = PS3_TRUE;
	SrbSetSrbStatus(Srb, SRB_STATUS_SUCCESS);
l_out:
	if (!ret) {
		StorPortNotification(RequestComplete, instance, Srb);
	}
	return ret;
}

U8 ps3_ioctl_start_io(_In_ struct ps3_instance *instance, _In_ PSCSI_REQUEST_BLOCK Srb)
{
	struct ps3_cmd *cmd = ps3_cmd_from_srb_extension_get(Srb);
	U8 ret = PS3_FALSE;
	S32 ret_tmp = PS3_SUCCESS;
	SrbSetSrbStatus(Srb, SRB_STATUS_ERROR);

	ret_tmp = ps3_srb_send(instance, Srb);
	if (ret_tmp != PS3_SUCCESS) {
		LOG_ERROR("trace_id[0x%llx], hno:%u ioctl send request failed!%d\n",
			cmd->trace_id, PS3_HOST(instance), ret_tmp);
		goto l_out;
	}

	ret = PS3_TRUE;
	SrbSetSrbStatus(Srb, SRB_STATUS_SUCCESS);
l_out:
	if (!ret) {
		ps3_ioctl_buff_release(cmd);
		ps3_mgr_cmd_free(instance, cmd);
		StorPortNotification(RequestComplete, instance, Srb);
		ps3_atomic_dec(&instance->ioctl_count);
	}
	return ret;
}

#else

static S32 ps3_ioctl_mgr_sync(ULong arg)
{
	S32 ret = PS3_SUCCESS;
	struct PS3IoctlSyncCmd* ioc = NULL;
	struct ps3_instance *instance = NULL;
	struct PS3IoctlSyncCmd __user* user_ioc =
		(struct PS3IoctlSyncCmd __user*)arg;

	ioc = (struct PS3IoctlSyncCmd*)memdup_user(
		(const void __user*)user_ioc, sizeof(*ioc));
	INJECT_START(PS3_ERR_IJ_FORCE_MEMDUP_USER_RETURN_FAILED, &ioc)
	if (IS_ERR(ioc)) {
		LOG_ERROR("ioctl memdup_user err\n");
		return PTR_ERR(ioc);
	}

	LOG_INFO("ioctl ps3_ioctl_sync_cmd: trace_id[0x%llx], hno:%u  "
		"cmd_type[%d], sgl_offset[%d], sge_count[%d], result[%d]\n",
		ioc->traceId, ioc->hostId, ioc->msg.header.cmdType,
		ioc->sglOffset, ioc->sgeCount, ioc->resultCode);

	spin_lock(&ps3_mgmt_info_get()->spin_lock_obj);
	instance = ps3_instance_lookup(ioc->hostId);
	if (instance == NULL) {
		spin_unlock(&ps3_mgmt_info_get()->spin_lock_obj);
		LOG_WARN("can found instance, host no is %d\n", ioc->hostId);
		ret = -ENODEV;
		goto l_out_free_ioc;
	}

	if (!instance->state_machine.is_load) {
		spin_unlock(&ps3_mgmt_info_get()->spin_lock_obj);
		LOG_WARN("trace_id[0x%llx], hno:%u instance state [%d]\n",
			ioc->traceId, PS3_HOST(instance),
			instance->state_machine.is_load);
		ret = -ENODEV;
		goto l_out_free_ioc;
	}

	INJECT_START(PS3_ERR_IJ_SCSI_SCAN_HOST_NOT_FINISH, instance)
	if (!instance->is_scan_host_finish) {
		spin_unlock(&ps3_mgmt_info_get()->spin_lock_obj);
		LOG_WARN("trace_id[0x%llx], hno:%u instance scan host not finish\n",
			ioc->traceId, PS3_HOST(instance));
		ret = -EAGAIN;
		goto l_out_free_ioc;
	}

	ps3_atomic_inc(&instance->cmd_statistics.cli_cnt);
	spin_unlock(&ps3_mgmt_info_get()->spin_lock_obj);

	LOG_FILE_INFO("hno:%u ioctl ready get sem\n",
		PS3_HOST(instance));
	if (down_interruptible(&instance->ioctl_sem)) {
		LOG_WARN("hno:%u ioctl concurrency full\n",
			PS3_HOST(instance));
		ret = -ERESTARTSYS;
		goto l_out_dec_cnt;
	}

	ret = ps3_instance_wait_for_normal(instance);
	INJECT_START(PS3_ERR_IJ_FORCE_IOCTL_WAIT_NORMAL_FAILED, &ret);
	if (ret != PS3_SUCCESS) {
		LOG_WARN("trace_id[0x%llx], hno:%u instance state not allowed ioctl req\n",
			ioc->traceId, PS3_HOST(instance));
		ret = -ENODEV;
		goto l_out_up;
	}

	LOG_INFO("hno:%u ioctl got sem\n",
		PS3_HOST(instance));

    if (ioc->msg.header.cmdType == PS3_IOCTL_CMD_WEB_SUBSCRIBE) {
        if (ps3_atomic_read(&instance->webSubscribe_context.is_subscribe) != 1) {
            if (ps3_atomic_add_unless(&instance->webSubscribe_context.subscribe_count, 1, 1) != 0) {
                ret = ps3_web_subscribe(instance);
                if (ret != PS3_SUCCESS) {
                    ps3_atomic_set(&instance->webSubscribe_context.subscribe_count, 0);
                    ret = -EBUSY;
                    goto l_out_up;
                }
                ps3_atomic_set(&instance->webSubscribe_context.is_subscribe, 1);
                goto l_out_dec_cnt;
            } else {
                ret = -EAGAIN;
                goto l_out_up;
            }
       } else {
            ret = PS3_SUCCESS;
            goto l_out_up;
       }
    } else {
        ret = ps3_ioctl_capbility_pre_check(instance, ioc);
        if (ret != PS3_SUCCESS) {
            LOG_WARN("hno:%u ioctl capblity not support\n",
                PS3_HOST(instance));
            ret = -EINVAL;
            goto l_out_up;
        }
        ret = ps3_ioctl_mgr_handle(instance, ioc);
        if (ret != PS3_SUCCESS) {
           LOG_ERROR("hno:%u ioctl handle NOK\n",
               PS3_HOST(instance));
           ret = -EBUSY;
        }
    }

l_out_up:
	up(&instance->ioctl_sem);
l_out_dec_cnt:
	ps3_atomic_dec(&instance->cmd_statistics.cli_cnt);
l_out_free_ioc:
	kfree(ioc);
	return ret;
}

#if 0
static S32 ps3_ioctl_mgr_async(struct file *file, ULong arg)
{
	(void)file;
	(void)arg;
	return PS3_SUCCESS;
}
#endif

long ps3_ioctl_fops(struct file *file, U32 cmd, ULong arg)
{
	S32 ret = -ENOTTY;
	if (file == NULL || arg == 0) {
		goto l_out;
	}

	switch (cmd) {
	case PS3_CMD_IOCTL_SYNC_CMD:
		ret = ps3_ioctl_mgr_sync(arg);
		break;
#if 0
	case PS3_CMD_IOCTL_ASYNC_CMD:
		return ps3_ioctl_mgr_async(file, arg);
		break;
#endif
	default:
		break;
	}
l_out:
	return ret;
}

void ps3_clean_mgr_cmd(struct ps3_instance *instance)
{
	struct ps3_cmd_context *context = &instance->cmd_context;
	U32 mgr_cmd_idx = 0;
	U32 mgr_start_idx = context->max_scsi_cmd_count;
	U32 mgr_end_idx = context->max_scsi_cmd_count +
			instance->max_mgr_cmd_count;
	struct ps3_cmd *cmd = NULL;

	for (mgr_cmd_idx = mgr_start_idx; mgr_cmd_idx < mgr_end_idx;
			mgr_cmd_idx++) {
		cmd = context->cmd_buf[mgr_cmd_idx];
		if (cmd->cmd_word.type == PS3_CMDWORD_TYPE_MGR) {
			LOG_WARN("hno:%u mgr cmd[%d] complete force!\n",
				PS3_HOST(instance), cmd->index);

			cmd->resp_frame->normalRespFrame.respStatus = PS3_MGR_REC_FORCE;
			cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
			complete(&cmd->sync_done);
		}
	}
}

void ps3_ioctl_clean(struct ps3_instance *instance)
{
	if (ps3_atomic_read(&instance->state_machine.state) !=
			PS3_INSTANCE_STATE_QUIT){
		goto l_out;
	}

	while (ps3_atomic_read(&instance->cmd_statistics.cli_cnt) != 0) {
		LOG_INFO("hno:%u ioctls not finish\n",
			PS3_HOST(instance));
		ps3_clean_mgr_cmd(instance);
		ps3_msleep(1000);
	}
l_out:
	return;
}

#endif
