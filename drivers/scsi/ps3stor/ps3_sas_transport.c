#ifndef _WINDOWS
#include <linux/compiler.h>

#include "ps3_sas_transport.h"
#include "ps3_mgr_channel.h"
#include "ps3_irq.h"
#include "ps3_mgr_cmd_err.h"
#include "ps3_mgr_cmd.h"
#include "ps3_driver_log.h"
#include "ps3_device_manager_sas.h"

static struct scsi_transport_template *ps3_sas_transport_template = NULL;
extern struct ps3_sas_node* ps3_sas_find_node_by_sas_addr(
	struct ps3_instance *instance, U64 sas_addr);
struct scsi_transport_template *ps3_sas_transport_get(void)
{
	return ps3_sas_transport_template;
}

static inline struct ps3_instance *phy_to_ps3_instance(struct sas_phy *phy)
{
	struct Scsi_Host *s_host = dev_to_shost(phy->dev.parent);
	return (struct ps3_instance *)s_host->hostdata;
}

static inline struct ps3_instance *rphy_to_ps3_instance(struct sas_rphy *rphy)
{
	struct Scsi_Host *s_host = dev_to_shost(rphy->dev.parent);
	return (struct ps3_instance *)s_host->hostdata;
}

static inline S32 ps3_sas_request_pre_check(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	if (!instance->state_machine.is_load) {
		LOG_WARN("hno:%u instance state not is_load\n", PS3_HOST(instance));
		ret = -EFAULT;
		goto l_out;
	}

	if (!ps3_is_instance_state_normal(instance)) {
		ret = -EFAULT;
		goto l_out;
	}

	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN("hno[%u] host in pci recovery\n",
			PS3_HOST(instance));
		ret = -EFAULT;
		goto l_out;
	}

l_out:
	return ret;
}

static inline S32 ps3_sas_smp_pre_check(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	ret = ps3_sas_request_pre_check(instance);
	if (ret != PS3_SUCCESS) {
		goto l_out;
	}

	LOG_INFO("hno:%u ready get semaphore\n", PS3_HOST(instance));
	if (down_interruptible(&instance->sas_dev_context.ps3_sas_smp_semaphore)) {
		LOG_WARN("hno:%u smp concurrency full\n",
			PS3_HOST(instance));
		ret = -EFAULT;
	}
	LOG_INFO("hno:%u got semaphore\n", PS3_HOST(instance));

l_out:
	return ret;
}

static inline void ps3_sas_linkerror_reqframe_build(struct ps3_cmd *cmd,
	struct sas_phy *phy, U8 encl_id)
{
	PS3MgrReqFrame_s *mgrReq = &cmd->req_frame->mgrReq;
	mgrReq->reqHead.timeout = cmd->time_out;
	mgrReq->reqHead.traceID = cmd->trace_id;
	mgrReq->reqHead.cmdType = PS3_CMD_SAS_MANAGEMENT;
	mgrReq->reqHead.cmdSubType = PS3_SAS_GET_LINK_ERR;
	mgrReq->reqHead.cmdFrameID = cmd->index;
	mgrReq->reqHead.control = 0;
	mgrReq->syncFlag = 1;
	mgrReq->timeout = 0;
	mgrReq->sgeOffset =
		offsetof(PS3MgrReqFrame_s, sgl) >> PS3_MGR_CMD_SGL_OFFSET_DWORD_SHIFT;
	mgrReq->sgeCount = 1;
	mgrReq->sgl[0].length = cpu_to_le32(sizeof(struct PS3LinkErrInfo));
	mgrReq->sgl[0].addr = cpu_to_le64(cmd->ext_buf_phys);
	mgrReq->sgl[0].lastSge = 1;
	mgrReq->sgl[0].ext = 0;

	mgrReq->value.sasMgr.sasAddr = cpu_to_le64(phy->identify.sas_address);
	mgrReq->value.sasMgr.enclID = encl_id;
	mgrReq->value.sasMgr.phyCount = 1;
	mgrReq->value.sasMgr.startPhyID = phy->identify.phy_identifier;
}

S32 ps3_sas_linkerrors_get(struct sas_phy *phy)
{
	S32 ret = 0;
	S32 send_result = PS3_SUCCESS;
	U8 encl_id = PS3_SAS_INVALID_ID;
	struct ps3_cmd *cmd = NULL;
	struct PS3LinkErrInfo *erroInfo = NULL;
	struct ps3_instance *instance = phy_to_ps3_instance(phy);

	ret = ps3_sas_smp_pre_check(instance);
	if (ret != PS3_SUCCESS) {
		ret = -ENXIO;
		goto l_out;
	}

	encl_id = ps3_sas_encl_id_get(instance, phy->identify.sas_address);
	INJECT_START(PS3_ERR_IJ_SAS_ENCL_ID_GET_FAILED, &encl_id)
	if (encl_id == PS3_SAS_INVALID_ID) {
		LOG_ERROR("hno:%u cannot foud PS3 node by sas_addr[%016llx]\n",
			PS3_HOST(instance), phy->identify.sas_address);
		ret = -EINVAL;
		goto l_no_free_cmd;
	}

	cmd = ps3_mgr_cmd_alloc(instance);
	INJECT_START(PS3_ERR_IJ_MGR_CMD_ALLOC_FAILED, &cmd)
	if (cmd == NULL) {
		LOG_WARN("hno:%u not get a cmd packet\n",
			PS3_HOST(instance));
		ret = -ENOMEM;
		goto l_no_free_cmd;
	}

	cmd->time_out = PS3_SAS_TIMEOUT_SEC;
	cmd->is_interrupt = PS3_DRV_FALSE;

	LOG_DEBUG("hno:%u trace_id[0x%llx] CFID [%u], sas_addr[%016llx] get mgr cmd succeed\n",
		PS3_HOST(instance), cmd->trace_id, cmd->index, phy->identify.sas_address);

	ps3_sas_linkerror_reqframe_build(cmd, phy, encl_id);
	ps3_mgr_cmd_word_build(cmd);
	INJECT_START(PS3_ERR_IJ_CMD_POLLING, &cmd)
	send_result = ps3_cmd_send_sync(instance, cmd);
	ret = ps3_mgr_complete_proc(instance, cmd, send_result);
	if (ret == -PS3_CMD_NO_RESP) {
		LOG_ERROR("hno:%u  %d respStatus NOK CFID[%d] respStatus[%d]\n",
			PS3_HOST(cmd->instance), ret,
			cmd->cmd_word.cmdFrameID,
			ps3_cmd_resp_status(cmd));
		ret = -ETIMEDOUT;
		goto l_no_free_cmd;
	}

	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  %d respStatus NOK CFID[%d] respStatus[%d]\n",
			PS3_HOST(cmd->instance), ret,
			cmd->cmd_word.cmdFrameID,
			ps3_cmd_resp_status(cmd));
		ret = -EFAULT;
	} else {
		erroInfo = (struct PS3LinkErrInfo *)cmd->ext_buf;
		phy->invalid_dword_count = le32_to_cpu(erroInfo->invalidDwordCount);
		phy->running_disparity_error_count =
			le32_to_cpu(erroInfo->runningDisparityErrCount);
		phy->loss_of_dword_sync_count =
			le32_to_cpu(erroInfo->lossOfDwordSyncCount);
		phy->phy_reset_problem_count =
			le32_to_cpu(erroInfo->phyResetProblemCount);
		ret = 0;
	}

	LOG_DEBUG("hno:%u trace_id[0x%llx] CFID [%u], end, ret[%d]\n",
		PS3_HOST(instance), cmd->trace_id, cmd->index, ret);

	ps3_mgr_cmd_free(instance, cmd);
l_no_free_cmd:
	up(&instance->sas_dev_context.ps3_sas_smp_semaphore);
l_out:
	return ret;
}

S32 ps3_sas_enclosure_identifier_get(struct sas_rphy *rphy, u64 *identifier)
{
	S32 ret = 0;
	struct ps3_instance *instance = rphy_to_ps3_instance(rphy);
	*identifier = 0;

	LOG_DEBUG("hno:%u ----1---ready get encl identifier sas_addr[%016llx]\n",
		PS3_HOST(instance), rphy->identify.sas_address);

	ret = ps3_sas_smp_pre_check(instance);
	if (ret != PS3_SUCCESS) {
		ret = -ENXIO;
		goto l_out;
	}

	LOG_DEBUG("hno:%u ready get encl identifier sas_addr[%016llx]\n",
		PS3_HOST(instance), rphy->identify.sas_address);

	*identifier = ps3_sas_rphy_parent_sas_addr_get(instance, rphy->identify.sas_address);
	INJECT_START(PS3_ERR_IJ_RPHY_PARENT_SAS_ADDR_GET_FAILED,identifier)
	if (*identifier == PS3_SAS_INVALID_SAS_ADDR) {
		ret = -ENXIO;
		*identifier = 0;
	}

	up(&instance->sas_dev_context.ps3_sas_smp_semaphore);
l_out:
	LOG_DEBUG("hno:%u end get encl identifier sas_addr[%016llx], identifier[%llu]\n",
		PS3_HOST(instance), rphy->identify.sas_address, *identifier);
	return ret;
}

S32 ps3_sas_bay_identifier_get(struct sas_rphy *rphy)
{
	U32 solt_id = 0;
	struct ps3_instance *instance = rphy_to_ps3_instance(rphy);

	LOG_DEBUG("hno:%u ----1---ready get bay identifier sas_addr[%016llx]\n",
		PS3_HOST(instance), rphy->identify.sas_address);

	if (ps3_sas_smp_pre_check(instance) != PS3_SUCCESS) {
		solt_id = -ENXIO;
		goto l_out;
	}

	LOG_DEBUG("hno:%u ready get bay identifier sas_addr[%016llx]\n",
		PS3_HOST(instance), rphy->identify.sas_address);

	if (ps3_sas_rphy_slot_get(instance, rphy->identify.sas_address, &solt_id)
		!= PS3_SUCCESS) {
		solt_id = -ENXIO;
	}

	up(&instance->sas_dev_context.ps3_sas_smp_semaphore);
l_out:
	LOG_DEBUG("hno:%u end get bay identifier sas_addr[%016llx], slot_id[%d]\n",
		PS3_HOST(instance), rphy->identify.sas_address, solt_id);
	return solt_id;
}

static inline void ps3_sas_ctrl_reqframe_build(struct ps3_cmd *cmd,
	struct sas_phy *phy,
	enum PhyCtrl ctrl_code)
{
	PS3MgrReqFrame_s *mgrReq = &cmd->req_frame->mgrReq;
	mgrReq->reqHead.timeout = cmd->time_out;
	mgrReq->reqHead.traceID = cmd->trace_id;
	mgrReq->reqHead.cmdType = PS3_CMD_SAS_MANAGEMENT;
	mgrReq->reqHead.cmdSubType = PS3_SAS_PHY_CTRL;
	mgrReq->reqHead.cmdFrameID = cmd->index;
	mgrReq->reqHead.control = 0;
	mgrReq->syncFlag = 1;
	mgrReq->timeout = 0;
	mgrReq->sgeCount = 0;

	mgrReq->value.phySet.sasAddr =
		cpu_to_le64(phy->identify.sas_address);
	mgrReq->value.phySet.phyCtrl = ctrl_code;
	mgrReq->value.phySet.phyID = phy->identify.phy_identifier;
	mgrReq->value.phySet.maxLinkRate = phy->maximum_linkrate;
	mgrReq->value.phySet.minLinkRate = phy->minimum_linkrate;
}

static inline S32 __ps3_sas_phy_ctrl(struct sas_phy *phy,
	enum PhyCtrl ctrl_code)
{
	S32 ret = PS3_SUCCESS;
	S32 send_result = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	struct ps3_instance *instance = phy_to_ps3_instance(phy);

	ret = ps3_sas_smp_pre_check(instance);
	if (ret != PS3_SUCCESS) {
		ret = -ENXIO;
		goto l_out;
	}

	cmd = ps3_mgr_cmd_alloc(instance);
	INJECT_START(PS3_ERR_IJ_MGR_CMD_ALLOC_FAILED, &cmd)
	if (cmd == NULL) {
		LOG_WARN("hno:%u not get a cmd packet\n",
			PS3_HOST(instance));
		ret = -ENOMEM;
		goto l_no_free_cmd;
	}
	cmd->time_out = PS3_SAS_TIMEOUT_SEC;
	cmd->is_interrupt = PS3_DRV_FALSE;

	LOG_DEBUG("hno:%u trace_id[0x%llx] CFID [%u], sas_addr[%016llx] ctrl_code[%s] get mgr cmd succeed\n",
		PS3_HOST(instance), cmd->trace_id, cmd->index,
		phy->identify.sas_address, namePhyCtrl((enum PhyCtrl)ctrl_code));

	ps3_sas_ctrl_reqframe_build(cmd, phy, ctrl_code);
	ps3_mgr_cmd_word_build(cmd);
	INJECT_START(PS3_ERR_IJ_CMD_POLLING, &cmd)
	send_result = ps3_cmd_send_sync(instance, cmd);
	ret = ps3_mgr_complete_proc(instance, cmd, send_result);
	if (ret == -PS3_CMD_NO_RESP) {
		LOG_ERROR("hno:%u  %d respStatus NOK CFID[%d] respStatus[%d]\n",
			PS3_HOST(cmd->instance), ret,
			cmd->cmd_word.cmdFrameID,
			ps3_cmd_resp_status(cmd));
		ret = -ETIMEDOUT;
		goto l_no_free_cmd;
	}

	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  %d respStatus NOK CFID[%d] respStatus[%d]\n",
			PS3_HOST(cmd->instance), ret,
			cmd->cmd_word.cmdFrameID,
			ps3_cmd_resp_status(cmd));
		ret = -EFAULT;
	}

	LOG_DEBUG("hno:%u trace_id[0x%llx] CFID [%u], end, ret[%d]\n",
		PS3_HOST(instance), cmd->trace_id, cmd->index, ret);
	ps3_mgr_cmd_free(instance, cmd);
l_no_free_cmd:
	up(&instance->sas_dev_context.ps3_sas_smp_semaphore);
l_out:
	return ret;
}

S32 ps3_sas_phy_reset(struct sas_phy *phy, int hard_reset)
{
	enum PhyCtrl ctrl_code =
		hard_reset ? PS3_SAS_CTRL_RESET_HARD : PS3_SAS_CTRL_RESET;
	struct sas_phy tmp_phy = *phy;
	tmp_phy.maximum_linkrate = 0;
	tmp_phy.minimum_linkrate = 0;

	LOG_INFO("enter phy reset, phy sas_addr[%016llx], is_hard[%d]\n",
		phy->identify.sas_address, hard_reset);
	return __ps3_sas_phy_ctrl(&tmp_phy, ctrl_code);
}

S32 ps3_sas_phy_enable(struct sas_phy *phy, int enable)
{
	enum PhyCtrl ctrl_code =
		enable ? PS3_SAS_CTRL_RESET : PS3_SAS_CTRL_DISABLE;
	struct sas_phy tmp_phy = *phy;
	tmp_phy.maximum_linkrate = 0;
	tmp_phy.minimum_linkrate = 0;

	LOG_INFO("enter phy enable, phy sas_addr[%016llx], is_enable[%d]\n",
		phy->identify.sas_address, enable);
	return __ps3_sas_phy_ctrl(&tmp_phy, ctrl_code);
}

S32 ps3_sas_update_phy_info(struct sas_phy *phy)
{
	S32 ret = -PS3_FAILED;
	struct PS3SasMgr sas_req_param;

	struct ps3_instance *instance = phy_to_ps3_instance(phy);
	struct PS3PhyInfo *phy_info = instance->sas_dev_context.ps3_sas_phy_buff;
	ULong flags = 0;

	struct ps3_sas_node* sas_node = ps3_sas_find_node_by_sas_addr(
		instance, phy->identify.sas_address);

	memset(&sas_req_param, 0, sizeof(sas_req_param));
	sas_req_param.enclID = sas_node->encl_id;
	sas_req_param.sasAddr =cpu_to_le64(sas_node->sas_address);
	sas_req_param.startPhyID = phy->identify.phy_identifier;
	sas_req_param.phyCount = 1;

	LOG_DEBUG("hno:%u ready get phys[%d] of encl_id[%d] !\n",
		PS3_HOST(instance), phy->identify.phy_identifier, sas_req_param.enclID);

	memset(phy_info, 0, PS3_SAS_REQ_BUFF_LEN);
	ret = ps3_sas_phy_get(instance, &sas_req_param);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u get phy info NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	LOG_INFO_IN_IRQ(instance, "hno:%u ready update phy %d of encl_id[%d]!\n",
		PS3_HOST(instance), phy->identify.phy_identifier, sas_req_param.enclID);

	ps3_sas_node_phy_update(instance, &sas_node->phys[phy->identify.phy_identifier], &phy_info[0]);
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);
l_out:
	return ret;
}

S32 ps3_sas_linkrates_set(struct sas_phy *phy, struct sas_phy_linkrates *rates)
{
	S32 ret = PS3_SUCCESS;
	U8 tmp_min = 0;
	U8 tmp_max = 0;
	LOG_INFO("enter link rate set, phy sas_addr[%016llx],"
		"minimum_linkrate[%d], maximum_linkrate[%d]\n",
		phy->identify.sas_address, rates->minimum_linkrate,
		rates->maximum_linkrate);

	if (!rates->minimum_linkrate) {
		rates->minimum_linkrate = phy->minimum_linkrate;
	} else if (rates->minimum_linkrate < phy->minimum_linkrate_hw) {
		rates->minimum_linkrate = phy->minimum_linkrate_hw;
	}

	if (!rates->maximum_linkrate) {
		rates->maximum_linkrate = phy->maximum_linkrate;
	} else if (rates->maximum_linkrate > phy->maximum_linkrate_hw) {
		rates->maximum_linkrate = phy->maximum_linkrate_hw;
	}

	if (rates->maximum_linkrate < phy->minimum_linkrate ||
		rates->minimum_linkrate> phy->maximum_linkrate) {
		LOG_ERROR("linkrate set param NOK, %d phy sas_addr[%016llx],"
			"rate minimum_linkrate[%d] > cur maximum_linkrate[%d] or"
			"rate maximum_linkrate[%d] < cur minimum_linkrate[%d]\n",
			phy->identify.phy_identifier, phy->identify.sas_address,
			rates->minimum_linkrate, phy->maximum_linkrate,
			rates->maximum_linkrate, phy->minimum_linkrate);
		ret = -EINVAL;
		goto l_out;
	}

	tmp_min = phy->minimum_linkrate;
	tmp_max = phy->maximum_linkrate;

	phy->minimum_linkrate = rates->minimum_linkrate;
	phy->maximum_linkrate = rates->maximum_linkrate;

	ret = __ps3_sas_phy_ctrl(phy, PS3_SAS_CTRL_RESET);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("linkrate NOK, phy sas_addr[%016llx],"
			"minimum_linkrate[%d], maximum_linkrate[%d]\n",
			phy->identify.sas_address, rates->minimum_linkrate,
			rates->maximum_linkrate);

		phy->minimum_linkrate = tmp_min;
		phy->maximum_linkrate = tmp_max;
		goto l_out;
	}

	ps3_sas_update_phy_info(phy);
l_out:
	return ret;
}

static inline void ps3_sas_smp_reqframe_build(struct ps3_cmd *cmd,
	U64 sas_addr, U32 req_data_len)
{
	PS3MgrReqFrame_s *mgrReq = &cmd->req_frame->mgrReq;
	mgrReq->reqHead.timeout = cmd->time_out;
	mgrReq->reqHead.traceID = cmd->trace_id;
	mgrReq->reqHead.cmdType = PS3_CMD_SAS_MANAGEMENT;
	mgrReq->reqHead.cmdSubType = PS3_SAS_SMP_REQUEST;
	mgrReq->reqHead.cmdFrameID = cmd->index;
	mgrReq->reqHead.control = 0;
	mgrReq->syncFlag = 1;
	mgrReq->timeout = 0;

	mgrReq->sgeOffset =
		offsetof(PS3MgrReqFrame_s, sgl) >> PS3_MGR_CMD_SGL_OFFSET_DWORD_SHIFT;
	mgrReq->sgeCount = 1;
	mgrReq->sgl[0].length =
		cpu_to_le32(cmd->instance->cmd_context.ext_buf_size);
	mgrReq->sgl[0].addr = cpu_to_le64(cmd->ext_buf_phys);
	mgrReq->sgl[0].lastSge = 1;
	mgrReq->sgl[0].ext = 0;

	mgrReq->value.sasMgr.sasAddr = cpu_to_le64(sas_addr);
	mgrReq->value.sasMgr.reqLen = cpu_to_le16(req_data_len - PS3_SMP_CRC_LEN);
}

static inline void show_smp(U8 *data, U16 len)
{
	U16 i = 0;
	S8 tmp_buf[256] = {0};
	S8 *p_tmp = tmp_buf;
	LOG_DEBUG("smp frame data start\n");
	while (len != 0) {
		memset(tmp_buf, 0, sizeof(S8) * 256);
		for (i = 0; i < 32 && len != 0; i++, len--) {
			snprintf(p_tmp, 4, " %02x", *data++);
			p_tmp += 3;
		}
		LOG_DEBUG("smp frame data is ==[%s]==\n", tmp_buf);
		p_tmp = tmp_buf;
	}

	LOG_DEBUG("smp frame data end\n");
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,14,0)

static inline U32 ps3_sas_req_to_ext_buf(struct ps3_instance *instance,
	struct request *req, void *ext_buf)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))
	struct bio_vec bvec;
	struct bvec_iter iter;
#else
	struct bio_vec *bvec = NULL;
	U32 i = 0;
#endif

	U32 req_len = 0;

	INJECT_START(PS3_ERR_IJ_SAS_REQ_EXT_BUF_INVALLID, req)
	if (unlikely(blk_rq_bytes(req) > instance->cmd_context.ext_buf_size)) {
		LOG_ERROR("hno:%u request is too big!(req_len:%d > ext_buf_len:%d\n",
			PS3_HOST(instance), blk_rq_bytes(req),
			instance->cmd_context.ext_buf_size);
		goto l_out;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))
	bio_for_each_segment(bvec, req->bio, iter) {
		memcpy((U8*)ext_buf + req_len,
			page_address(bvec.bv_page) + bvec.bv_offset,
			bvec.bv_len);
		req_len += bvec.bv_len;
	}
#else
	bio_for_each_segment(bvec, req->bio, i) {
		memcpy((U8*)ext_buf + req_len,
			page_address(bvec->bv_page) + bvec->bv_offset,
			bvec->bv_len);
		req_len += bvec->bv_len;
	}
#endif

l_out:
	return req_len;
}

static inline S32 ps3_sas_ext_buf_to_rsp(struct ps3_instance *instance,
	struct request *req, void *ext_buf)
{
	S32 ret = PS3_SUCCESS;
	struct request *rsp = req->next_rq;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))
	struct bio_vec bvec;
	struct bvec_iter iter;
#else
	struct bio_vec *bvec = NULL;
	U32 i = 0;
#endif
	U32 offset = 0;
	U16 rsq_data_len = 0;
	U16 smp_len = 0;

	INJECT_START(PS3_ERR_IJ_EXT_BUF_TO_RSP_INVALLID, &rsp)
	if (rsp == NULL) {
		ret = -PS3_FAILED;
		LOG_ERROR("hno:%u  rsp == NULL\n", PS3_HOST(instance));
		goto l_out;
	}

	rsq_data_len = min(blk_rq_bytes(rsp), instance->cmd_context.ext_buf_size);

	smp_len = ((U8*)ext_buf)[3] * 4 + 4;
	rsp->resid_len -= smp_len;
	LOG_DEBUG("hno:%u  smp frame len[%d], rsq_data_len[%d]\n",
		PS3_HOST(instance), smp_len, rsq_data_len);

	rsq_data_len = min(smp_len, rsq_data_len);

	show_smp((U8*)ext_buf, rsq_data_len);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))	
	bio_for_each_segment(bvec, rsp->bio, iter) {
		if (rsq_data_len <= bvec.bv_len) {
			memcpy(page_address(bvec.bv_page) + bvec.bv_offset,
				(U8*)ext_buf + offset, rsq_data_len);
			break;
		} else {
			memcpy(page_address(bvec.bv_page) + bvec.bv_offset,
				(U8*)ext_buf + offset, bvec.bv_len);
			rsq_data_len -= bvec.bv_len;
		}
		offset += bvec.bv_len;
	}
#else
	bio_for_each_segment(bvec, rsp->bio, i) {
		if (rsq_data_len <= bvec->bv_len) {
			memcpy(page_address(bvec->bv_page) + bvec->bv_offset,
				(U8*)ext_buf + offset, rsq_data_len);
			break;
		} else {
			memcpy(page_address(bvec->bv_page) + bvec->bv_offset,
				(U8*)ext_buf + offset, bvec->bv_len);
			rsq_data_len -= bvec->bv_len;
		}
		offset += bvec->bv_len;
	}
#endif
l_out:
	return ret;
}
S32 ps3_sas_smp_handler(struct Scsi_Host *shost, struct sas_rphy *rphy,
	struct request *req)
{
	S32 ret = -PS3_FAILED;
	S32 send_result = PS3_SUCCESS;
	struct ps3_instance *instance = (struct ps3_instance *)shost->hostdata;
	struct ps3_cmd *cmd = NULL;
	U32 req_data_len = 0;
	U64 sas_addr = 0;
	struct ps3_sas_node *ps3_sas_node = NULL;

	ret = ps3_sas_smp_pre_check(instance);
	if (ret != PS3_SUCCESS) {
		ret = -EFAULT;
		goto l_out;
	}

	sas_addr = (rphy) ? (rphy->identify.sas_address) : (instance->sas_dev_context.ps3_hba_sas.sas_address);
	ps3_sas_node = ps3_sas_find_node_by_sas_addr(instance, sas_addr);
	INJECT_START(PS3_ERR_IJ_SAS_NODE_NOT_FOUND, &ps3_sas_node)
	if (ps3_sas_node == NULL) {
			LOG_ERROR("hno:%u cannot find node[%llx] !\n",
					PS3_HOST(instance), sas_addr);
			up(&instance->sas_dev_context.ps3_sas_smp_semaphore);
			ret = -EFAULT;
			goto l_out;
	}

	cmd = ps3_mgr_cmd_alloc(instance);
	INJECT_START(PS3_ERR_IJ_MGR_CMD_ALLOC_FAILED, &cmd)
	if (cmd == NULL) {
		LOG_WARN("hno:%u not get a cmd packet\n",
			PS3_HOST(instance));
		ret = -ENOMEM;
		goto l_no_free_cmd;
	}
	cmd->time_out = PS3_SAS_TIMEOUT_SEC;
	cmd->is_interrupt = PS3_DRV_FALSE;

	req_data_len = ps3_sas_req_to_ext_buf(instance, req, cmd->ext_buf);
	if (req_data_len == 0) {
		ret = -ENOMEM;
		goto l_out_failed;
	}

	LOG_DEBUG("hno:%u trace_id[0x%llx] CFID [%u], sas_addr[%016llx], len[%u] send smp req\n",
		PS3_HOST(instance), cmd->trace_id, cmd->index, sas_addr, req_data_len);

	ps3_sas_smp_reqframe_build(cmd, sas_addr, req_data_len);
	ps3_mgr_cmd_word_build(cmd);
	send_result = ps3_cmd_send_sync(instance, cmd);
	ret = ps3_mgr_complete_proc(instance, cmd, send_result);
	if (ret == -PS3_CMD_NO_RESP) {
		LOG_ERROR("hno:%u  %d respStatus NOK CFID[%d] respStatus[%d]\n",
			PS3_HOST(cmd->instance), ret,
			cmd->cmd_word.cmdFrameID,
			ps3_cmd_resp_status(cmd));
		ret = -ETIMEDOUT;
		goto l_no_free_cmd;
	}

	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  %d respStatus NOK CFID[%d] respStatus[%d]\n",
			PS3_HOST(cmd->instance), ret,
			cmd->cmd_word.cmdFrameID,
			ps3_cmd_resp_status(cmd));
		ret = -ENXIO;
		if (ret == -PS3_TIMEOUT) {
			ret = -ETIMEDOUT;
		}
		goto l_out_failed;
	}

	ret = ps3_sas_ext_buf_to_rsp(instance, req, cmd->ext_buf);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  %d smp response NOK CFID[%d]\n",
			PS3_HOST(cmd->instance), ret,
			cmd->cmd_word.cmdFrameID);
		ret = -EINVAL;
		goto l_out_failed;
	}

	LOG_DEBUG("hno:%u trace_id[0x%llx] CFID [%u], end, ret[%d]\n",
		PS3_HOST(instance), cmd->trace_id, cmd->index, ret);
l_out_failed:
	ps3_mgr_cmd_free(instance, cmd);
l_no_free_cmd:
	up(&instance->sas_dev_context.ps3_sas_smp_semaphore);
l_out:
	return ret;
}
#else

static inline U32 ps3_sas_req_to_ext_buf(struct ps3_instance *instance,
	struct bsg_buffer *req_buf, void *ext_buf)
{
	U32 req_len = 0;

	if (unlikely(req_buf->payload_len > instance->cmd_context.ext_buf_size)) {
		LOG_ERROR("hno:%u request is too big!(req_len:%d > ext_buf_len:%d\n",
			PS3_HOST(instance), req_buf->payload_len,
			instance->cmd_context.ext_buf_size);
		goto l_out;
	}

	req_len = sg_copy_to_buffer(req_buf->sg_list, req_buf->sg_cnt, ext_buf,
		req_buf->payload_len);
l_out:
	return req_len;
}

static inline U32 ps3_sas_ext_buf_to_rsp(struct ps3_instance *instance,
	struct bsg_buffer *rsp_buf, void *ext_buf)
{
	U16 rsq_data_len = 0;
	U16 smp_len = 0;

	rsq_data_len = min(rsp_buf->payload_len, instance->cmd_context.ext_buf_size);

	smp_len = ((U8*)ext_buf)[3] * 4 + 4;
	LOG_DEBUG("hno:%u  smp frame len[%d], rsq_data_len[%d]\n",
		PS3_HOST(instance), smp_len, rsq_data_len);

	rsq_data_len = min(smp_len, rsq_data_len);

	show_smp((U8*)ext_buf, rsq_data_len);

	sg_copy_from_buffer(rsp_buf->sg_list, rsp_buf->sg_cnt, ext_buf,
		rsp_buf->payload_len);
	return rsq_data_len;
}

void ps3_sas_smp_handler(struct bsg_job *job, struct Scsi_Host *shost,
	struct sas_rphy *rphy)
{
	S32 ret = PS3_SUCCESS;
	S32 send_result = PS3_SUCCESS;
	struct ps3_instance *instance = (struct ps3_instance *)shost->hostdata;
	struct ps3_cmd *cmd = NULL;
	U32 req_data_len = 0;
	U32 resp_len = 0;
	U64 sas_addr = 0;
	struct ps3_sas_node *ps3_sas_node = NULL;

	ret = ps3_sas_smp_pre_check(instance);
	if (ret != PS3_SUCCESS) {
		ret = -EFAULT;
		goto l_out;
	}

	sas_addr = (rphy) ? (rphy->identify.sas_address) : (instance->sas_dev_context.ps3_hba_sas.sas_address);
	ps3_sas_node = ps3_sas_find_node_by_sas_addr(instance, sas_addr);
	if (ps3_sas_node == NULL) {
			LOG_ERROR("hno:%u cannot find node[%llx] !\n",
					PS3_HOST(instance), sas_addr);
			up(&instance->sas_dev_context.ps3_sas_smp_semaphore);
			ret = -EFAULT;
			goto l_out;
	}

	cmd = ps3_mgr_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_WARN("hno:%u Failed to get a cmd packet\n",
			PS3_HOST(instance));
		ret = -ENOMEM;
		goto l_no_free_cmd;
	}
	cmd->time_out = PS3_SAS_TIMEOUT_SEC;
	cmd->is_interrupt = PS3_DRV_FALSE;

	req_data_len = ps3_sas_req_to_ext_buf(instance, &job->request_payload, cmd->ext_buf);
	if (req_data_len == 0) {
		ret = -ENOMEM;
		goto l_out_failed;
	}

	LOG_DEBUG("hno:%u trace_id[0x%llx] CFID [%u], sas_addr[%016llx], len[%u] send smp req\n",
		PS3_HOST(instance), cmd->trace_id, cmd->index, sas_addr, req_data_len);

	ps3_sas_smp_reqframe_build(cmd, sas_addr, req_data_len);
	ps3_mgr_cmd_word_build(cmd);
	send_result = ps3_cmd_send_sync(instance, cmd);
	ret = ps3_mgr_complete_proc(instance, cmd, send_result);
	if (ret == -PS3_CMD_NO_RESP) {
		LOG_ERROR("hno:%u  %d respStatus NOK CFID[%d] respStatus[%d]\n",
			PS3_HOST(cmd->instance), ret,
			cmd->cmd_word.cmdFrameID,
			ps3_cmd_resp_status(cmd));
		ret = -ETIMEDOUT;
		goto l_no_free_cmd;
	}

	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  %d respStatus NOK CFID[%d] respStatus[%d]\n",
			PS3_HOST(cmd->instance), ret,
			cmd->cmd_word.cmdFrameID,
			ps3_cmd_resp_status(cmd));
		ret = -ENXIO;
		goto l_out_failed;
	}

	resp_len = ps3_sas_ext_buf_to_rsp(instance, &job->reply_payload, cmd->ext_buf);

	LOG_DEBUG("hno:%u trace_id[0x%llx] CFID [%u], end, ret[%d]\n",
		PS3_HOST(instance), cmd->trace_id, cmd->index, ret);
l_out_failed:
	ps3_mgr_cmd_free(instance, cmd);
l_no_free_cmd:
	up(&instance->sas_dev_context.ps3_sas_smp_semaphore);
l_out:
	bsg_job_done(job, ret, resp_len);
}
#endif

S32 ps3_sas_attach_transport(void)
{
	S32 ret = PS3_SUCCESS;
	static struct sas_function_template ps3_sas_transport_functions = {
		.get_linkerrors		= ps3_sas_linkerrors_get,
		.get_enclosure_identifier = ps3_sas_enclosure_identifier_get,
		.get_bay_identifier	= ps3_sas_bay_identifier_get,
		.phy_reset		= ps3_sas_phy_reset,
		.phy_enable		= ps3_sas_phy_enable,
		.set_phy_speed		= ps3_sas_linkrates_set,
		.smp_handler		= ps3_sas_smp_handler,
	};

	ps3_sas_transport_template =
		sas_attach_transport(&ps3_sas_transport_functions);
	if (!ps3_sas_transport_template) {
		ret = -PS3_FAILED;
	}
	return ret;
}

void ps3_sas_release_transport(void)
{
	if(ps3_sas_transport_template != NULL) {
		sas_release_transport(ps3_sas_transport_template);
		ps3_sas_transport_template = NULL;
	}
}
#endif

