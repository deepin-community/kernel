
#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uio.h>
#include <linux/irq_poll.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>

#include <scsi/scsi_host.h>
#endif

#include "ps3_mgr_cmd.h"
#include "ps3_event.h"
#include "ps3_device_update.h"
#include "ps3_device_manager.h"
#include "ps3_cmd_complete.h"
#include "ps3_mgr_channel.h"
#include "ps3_mgr_cmd_err.h"
#include "ps3_scsih.h"
#include "ps3_util.h"
#include "ps3_ioc_manager.h"
#include "ps3_err_inject.h"
#include "ps3_ioc_state.h"
#include "ps3_ioctl.h"

static S32 ps3_mgr_cmd_sync_proc(struct ps3_instance *instance,
	struct ps3_cmd *cmd, U16 time_out);

S32 ps3_ctrl_info_buf_alloc(struct ps3_instance *instance)
{
	instance->ctrl_info_buf = (struct PS3IocCtrlInfo *)
		ps3_dma_alloc_coherent(instance, sizeof(struct PS3IocCtrlInfo),
		&instance->ctrl_info_buf_h);

	if (instance->ctrl_info_buf == NULL) {
		LOG_ERROR("host_no:%u alloc ctrl info buffer failed !\n",
			PS3_HOST(instance));
		goto l_fail;
	}
	return PS3_SUCCESS;
l_fail:
	return -PS3_ENOMEM;
}

void ps3_ctrl_info_buf_free(struct ps3_instance *instance)
{
	if (instance->ctrl_info_buf != NULL) {
		LOG_INFO("ctrl_info_buf = %p\n", instance->ctrl_info_buf);
		ps3_dma_free_coherent(instance, sizeof(struct PS3IocCtrlInfo),
			instance->ctrl_info_buf,
			instance->ctrl_info_buf_h);
		instance->ctrl_info_buf = NULL;
	}

	memset(&instance->ctrl_info, 0, sizeof(struct PS3IocCtrlInfo));
}

static S32 ps3_mgr_pd_list_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_dev_context *dev_context = &instance->dev_context;

	dev_context->pd_list_buf = (struct PS3DevList *)
		ps3_dma_alloc_coherent(instance,
		PS3_MAX_PD_COUNT(instance) *
		sizeof(struct PS3PhyDevice) + sizeof(struct PS3DevList),
		&dev_context->pd_list_buf_phys);

	if (dev_context->pd_list_buf == NULL) {
		LOG_ERROR("host_no:%u alloc pd list buffer failed !\n",
			PS3_HOST(instance));
		ret = -PS3_ENOMEM;
	}

	return ret;
}

static S32 ps3_mgr_vd_list_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_dev_context *dev_context = &instance->dev_context;

	dev_context->vd_list_buf = (struct PS3DevList *)
		ps3_dma_alloc_coherent(instance,
		PS3_MAX_VD_COUNT(instance) *
		sizeof(struct PS3VirtDevice) + sizeof(struct PS3DevList),
		&dev_context->vd_list_buf_phys);

	if (dev_context->vd_list_buf == NULL) {
		LOG_ERROR("host_no:%u alloc vd list buffer failed !\n",
			PS3_HOST(instance));
		ret = -PS3_ENOMEM;
	}
	return ret;
}

static S32 ps3_mgr_pd_info_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_dev_context *dev_context = &instance->dev_context;

	dev_context->pd_info_buf = (struct PS3PDInfo *)
		ps3_dma_alloc_coherent(instance, sizeof(struct PS3PDInfo),
		&dev_context->pd_info_buf_phys);

	if (dev_context->pd_info_buf == NULL) {
		LOG_ERROR("host_no:%u alloc pd info buffer failed !\n",
			PS3_HOST(instance));
		ret = -PS3_ENOMEM;
	}
	return ret;
}

static inline void ps3_mgr_pd_list_exit(struct ps3_instance *instance)
{
	struct ps3_dev_context *dev_context = &instance->dev_context;

	if (dev_context->pd_list_buf != NULL) {
		ps3_dma_free_coherent(instance,
			PS3_MAX_PD_COUNT(instance) *
			sizeof(struct PS3PhyDevice) +
			sizeof(struct PS3DevList),
			dev_context->pd_list_buf,
			dev_context->pd_list_buf_phys);
		dev_context->pd_list_buf = NULL;
	}
}

static inline void ps3_mgr_vd_list_exit(struct ps3_instance *instance)
{
	struct ps3_dev_context *dev_context = &instance->dev_context;

	if (dev_context->vd_list_buf != NULL) {
		ps3_dma_free_coherent(instance,
			PS3_MAX_VD_COUNT(instance) *
			sizeof(struct PS3VirtDevice) +
			sizeof(struct PS3DevList),
			dev_context->vd_list_buf,
			dev_context->vd_list_buf_phys);
		dev_context->vd_list_buf = NULL;
	}
}

static inline void ps3_mgr_pd_info_exit(struct ps3_instance *instance)
{
	struct ps3_dev_context *dev_context = &instance->dev_context;

	if (dev_context->pd_info_buf != NULL) {
		ps3_dma_free_coherent(instance, sizeof(struct PS3PDInfo),
			dev_context->pd_info_buf,
			dev_context->pd_info_buf_phys);
		dev_context->pd_info_buf = NULL;
	}
}

static inline void ps3_mgr_vd_info_exit(struct ps3_instance *instance)
{
	struct ps3_dev_context *dev_context = &instance->dev_context;

	if (dev_context->vd_info_buf_sync != NULL) {
		ps3_dma_free_coherent(instance, PS3_MAX_VD_COUNT(instance)
			* sizeof(struct PS3VDEntry) +
			sizeof(struct PS3VDInfo),
			dev_context->vd_info_buf_sync,
			dev_context->vd_info_buf_phys_sync);
		dev_context->vd_info_buf_sync = NULL;
	}

	if (dev_context->vd_info_buf_async != NULL) {
		ps3_dma_free_coherent(instance, PS3_MAX_VD_COUNT(instance)
			* sizeof(struct PS3VDEntry) +
			sizeof(struct PS3VDInfo),
			dev_context->vd_info_buf_async,
			dev_context->vd_info_buf_phys_async);
		dev_context->vd_info_buf_async = NULL;
	}

}

static S32 ps3_mgr_vd_info_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_dev_context *dev_context = &instance->dev_context;

	dev_context->vd_table_idx = 0;
	dev_context->vd_info_buf_sync = (struct PS3VDInfo *)
		ps3_dma_alloc_coherent(instance,
		PS3_MAX_VD_COUNT(instance) * sizeof(struct PS3VDEntry)
			+ sizeof(struct PS3VDInfo),
		&dev_context->vd_info_buf_phys_sync);

	if (dev_context->vd_info_buf_sync == NULL) {
		LOG_ERROR("host_no:%u alloc vd sync info buffer failed !\n",
			PS3_HOST(instance));
		ret = -PS3_ENOMEM;
		goto l_out;
	}

	dev_context->vd_info_buf_async = (struct PS3VDInfo *)
		ps3_dma_alloc_coherent(instance,
		PS3_MAX_VD_COUNT(instance) * sizeof(struct PS3VDEntry)
			+ sizeof(struct PS3VDInfo),
		&dev_context->vd_info_buf_phys_async);

	if (dev_context->vd_info_buf_async == NULL) {
		LOG_ERROR("host_no:%u alloc vd async info buffer failed !\n",
			PS3_HOST(instance));
		ret = -PS3_ENOMEM;
		goto l_failed;
	}

	return ret;

l_failed:
	ps3_mgr_vd_info_exit(instance);
l_out:
	return ret;
}

S32 ps3_mgr_cmd_init(struct ps3_instance *instance)
{
	LOG_INFO("host_no:%u, Soc VD max is:%d\n", PS3_HOST(instance),
		PS3_MAX_VD_COUNT(instance));
	LOG_INFO("host_no:%u, Soc PD max is:%d\n", PS3_HOST(instance),
		PS3_MAX_PD_COUNT(instance));

	if (PS3_MAX_PD_COUNT(instance) > 0) {
		if (ps3_mgr_pd_list_init(instance) != PS3_SUCCESS) {
			goto free_mgr_cmd;
		}

		if (ps3_mgr_pd_info_init(instance) != PS3_SUCCESS) {
			goto free_mgr_cmd;
		}
	}

	if (PS3_MAX_VD_COUNT(instance) > 0) {
		if (ps3_mgr_vd_list_init(instance) != PS3_SUCCESS) {
			goto free_mgr_cmd;
		}

		if (ps3_mgr_vd_info_init(instance) != PS3_SUCCESS) {
			goto free_mgr_cmd;
		}
	}

	return PS3_SUCCESS;

free_mgr_cmd:
	ps3_mgr_cmd_exit(instance);
	return -PS3_ENOMEM;
}

void ps3_mgr_cmd_exit(struct ps3_instance *instance)
{
	ps3_mgr_pd_list_exit(instance);
	ps3_mgr_vd_list_exit(instance);
	ps3_mgr_pd_info_exit(instance);
	ps3_mgr_vd_info_exit(instance);

	return ;
}

void ps3_mgr_cmd_word_build(struct ps3_cmd *cmd)
{
	struct PS3CmdWord *cmd_word = &cmd->cmd_word;
	memset(cmd_word, 0, sizeof(*cmd_word));

	cmd_word->type = PS3_CMDWORD_TYPE_MGR;
	cmd_word->direct = PS3_CMDWORD_DIRECT_NORMAL;
	cmd_word->cmdFrameID = ps3_cmd_frame_id(cmd);
#ifndef _WINDOWS
	cmd_word->isrSN = ps3_msix_index_get(cmd, 1);
#else
	cmd_word->isrSN = 0;
#endif
}

static void ps3_mgr_print_cmd(struct ps3_cmd *cmd, const S8 *cmd_type, Bool is_send)
{
	LOG_DEBUG("host_no:%u t_id:0x%llx  mgr:%s:%s cmd word type:%d "
		"direct:%d qmask:0x%x CFID:%d isr_sn:%d vid:%d pid:%d function:%d\n",
		PS3_HOST(cmd->instance), cmd->trace_id,
		(is_send) ? "send" : "recv", cmd_type,
		cmd->cmd_word.type, cmd->cmd_word.direct,
		cmd->cmd_word.qMask, cmd->cmd_word.cmdFrameID,
		cmd->cmd_word.isrSN, cmd->cmd_word.virtDiskID,
		cmd->cmd_word.phyDiskID, ps3_get_pci_function(cmd->instance->pdev));
}

static inline void ps3_mgr_req_head_init(struct ps3_cmd *cmd,
	PS3ReqFrameHead_s *req_header, U8  cmdSubType)
{
	req_header->timeout = PS3_DEFAULT_MGR_CMD_TIMEOUT;
	req_header->traceID = ps3_cmd_trace_id(cmd);
	req_header->cmdType = PS3_CMD_MANAGEMENT;
	req_header->cmdSubType = cmdSubType;
	req_header->cmdFrameID = ps3_cmd_frame_id(cmd);
	req_header->control = 0;
	req_header->reqFrameFormat = PS3_REQFRAME_FORMAT_FRONTEND;
	req_header->noReplyWord = PS3_CMD_WORD_NEED_REPLY_WORD;
}

static inline void ps3_mgr_req_frame_sge_build(PS3MgrReqFrame_s *mgr_req_frame,
		dma_addr_t dma_addr, U32 dma_len)
{
	struct PS3Sge *p_sge = mgr_req_frame->sgl;

	mgr_req_frame->sgeOffset = offsetof(PS3MgrReqFrame_s, sgl) >> PS3_MGR_CMD_SGL_OFFSET_DWORD_SHIFT;
	mgr_req_frame->sgeCount = 1;

	p_sge->addr = cpu_to_le64(dma_addr);
	p_sge->length = cpu_to_le32(dma_len);
	p_sge->lastSge  = 1;
	p_sge->ext  = 0;
}

static S32 ps3_mgr_unload_cmd_send(struct ps3_cmd *cmd, U16 time_out)
{
	S32 ret = PS3_SUCCESS;
	enum PS3MgrCmdSubType cmd_type =
		(enum PS3MgrCmdSubType)cmd->req_frame->mgrReq.reqHead.cmdSubType;

	ps3_mgr_cmd_word_build(cmd);
	cmd->time_out = time_out;
	cmd->is_interrupt = PS3_DRV_FALSE;
	ps3_mgr_print_cmd(cmd, namePS3MgrCmdSubType(cmd_type), PS3_DRV_TRUE);
	ret = ps3_unload_cmd_send_sync(cmd->instance, cmd);
	ps3_mgr_print_cmd(cmd, namePS3MgrCmdSubType(cmd_type), PS3_DRV_FALSE);

	if (ret == PS3_SUCCESS) {
		LOG_INFO("host_no:%u send %s success\n", PS3_HOST(cmd->instance),
			namePS3MgrCmdSubType(cmd_type));
	} else {
		LOG_ERROR("host_no:%u send %s %s respStatus:%d\n", PS3_HOST(cmd->instance),
			namePS3MgrCmdSubType(cmd_type),
			(ret == -PS3_TIMEOUT) ? "timeout" : "failed",
			ps3_cmd_resp_status(cmd));
	}

	return ret;
}

S32 ps3_pd_list_get(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;
	U32 pd_list_size = PS3_MAX_PD_COUNT(instance) *
		sizeof(struct PS3PhyDevice) + sizeof(struct PS3DevList);

	LOG_INFO("host_no:%u enter !\n", PS3_HOST(instance));

	cmd = ps3_mgr_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_ERROR("host_no:%u mgr cmd get NOK !\n",
			PS3_HOST(instance));
		ret = -PS3_EBUSY;
		goto l_out;
	}

	mgr_req_frame = &cmd->req_frame->mgrReq;
	memset(instance->dev_context.pd_list_buf, 0, pd_list_size);
	memset(mgr_req_frame, 0, sizeof(*mgr_req_frame));

	ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_GET_PD_LIST);
	mgr_req_frame->timeout = 0;
	mgr_req_frame->syncFlag = 1;
	mgr_req_frame->value.dev.devID.diskDev.diskID = 0;
	mgr_req_frame->value.dev.num = PS3_MAX_PD_COUNT(instance);

	ps3_mgr_req_frame_sge_build(mgr_req_frame,
		instance->dev_context.pd_list_buf_phys,
		pd_list_size);

	ret = ps3_mgr_cmd_sync_proc(instance, cmd, PS3_DEFAULT_MGR_CMD_TIMEOUT);
l_out:
	if ((ret != -PS3_CMD_NO_RESP) && (cmd != NULL)) {
		ps3_mgr_cmd_free(instance, cmd);
	}

	LOG_INFO("host_no:%u exit !\n", PS3_HOST(instance));
	return ret;
}

S32 ps3_vd_list_get(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;
	U32 vd_list_size = PS3_MAX_VD_COUNT(instance) *
		sizeof(struct PS3VirtDevice) + sizeof(struct PS3DevList);

	LOG_INFO("host_no:%u enter !\n", PS3_HOST(instance));

	cmd = ps3_mgr_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_ERROR("host_no:%u mgr cmd get NOK !\n",
			PS3_HOST(instance));
		ret = -PS3_EBUSY;
		goto l_out;
	}

	mgr_req_frame = &cmd->req_frame->mgrReq;
	memset(instance->dev_context.vd_list_buf, 0, vd_list_size);
	memset(mgr_req_frame, 0, sizeof(*mgr_req_frame));

	ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_GET_VD_LIST);
	mgr_req_frame->timeout = 0;
	mgr_req_frame->syncFlag = 1;
	mgr_req_frame->value.dev.devID.diskDev.diskID = 0;
	mgr_req_frame->value.dev.num = PS3_MAX_VD_COUNT(instance);

	ps3_mgr_req_frame_sge_build(mgr_req_frame,
		instance->dev_context.vd_list_buf_phys,
		vd_list_size);

	ret = ps3_mgr_cmd_sync_proc(instance, cmd, PS3_DEFAULT_MGR_CMD_TIMEOUT);
l_out:
	if ((ret != -PS3_CMD_NO_RESP) && (cmd != NULL)) {
		ps3_mgr_cmd_free(instance, cmd);
	}
	LOG_INFO("host_no:%u exit !\n", PS3_HOST(instance));
	return ret;
}

S32 ps3_pd_info_get(struct ps3_instance *instance, U16 channel, U16 target_id, U16 pd_disk_id)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;
	U32 pd_info_size = sizeof(struct PS3PDInfo);

	LOG_DEBUG("host_no:%u enter !\n", PS3_HOST(instance));

	cmd = ps3_mgr_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_ERROR("host_no:%u mgr cmd get NOK !\n",
			PS3_HOST(instance));
		ret = -PS3_EBUSY;
		goto l_out;
	}

	mgr_req_frame = &cmd->req_frame->mgrReq;
	memset(instance->dev_context.pd_info_buf, 0, pd_info_size);
	memset(mgr_req_frame, 0, sizeof(*mgr_req_frame));

	ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_GET_PD_INFO);

	mgr_req_frame->timeout = 0;
	mgr_req_frame->syncFlag = 1;

	mgr_req_frame->value.dev.num = 1;
	mgr_req_frame->value.dev.devID.diskDev.ps3Dev.softChan = channel;
	mgr_req_frame->value.dev.devID.diskDev.ps3Dev.devID = target_id;
	mgr_req_frame->value.dev.devID.diskDev.ps3Dev.phyDiskID = pd_disk_id;

	ps3_mgr_req_frame_sge_build(mgr_req_frame,
		instance->dev_context.pd_info_buf_phys,
		pd_info_size);

	LOG_INFO("host_no:%u ready send, reqFrameId=%d, [%d:%d:%d]!\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd), channel, target_id,
		pd_disk_id);

	ret = ps3_mgr_cmd_sync_proc(instance, cmd, PS3_DEFAULT_MGR_CMD_TIMEOUT);
	LOG_INFO("host_no:%u get pd info [%d:%d:%d] finished!:ret = %d\n",
		PS3_HOST(instance), channel, target_id, pd_disk_id, ret);
l_out:
	if ((ret != -PS3_CMD_NO_RESP) && (cmd != NULL)) {
		ps3_mgr_cmd_free(instance, cmd);
	}

	INJECT_START(PS3_ERR_IJ_GET_PD_INFO_RETURN_FAILED, &ret);

	LOG_DEBUG("host_no:%u exit ret[%d]!\n", PS3_HOST(instance), ret);
	return ret;
}

S32 ps3_vd_info_sync_get(struct ps3_instance *instance, U32 disk_id,
		U16 vd_num)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;
	U32 vd_info_size = PS3_MAX_VD_COUNT(instance) *
		sizeof(struct PS3VDEntry) + sizeof(struct PS3VDInfo);
	struct ps3_dev_context *dev_context = &instance->dev_context;

	LOG_DEBUG("host_no:%u enter !\n", PS3_HOST(instance));

	cmd = ps3_mgr_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_ERROR("host_no:%u mgr cmd get NOK !\n",
			PS3_HOST(instance));
		ret = -PS3_EBUSY;
		goto l_out;
	}

	mgr_req_frame = &cmd->req_frame->mgrReq;
	memset(dev_context->vd_info_buf_sync, 0, vd_info_size);
	memset(mgr_req_frame, 0, sizeof(*mgr_req_frame));

	ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_GET_VD_INFO);
	mgr_req_frame->timeout = 0;
	mgr_req_frame->syncFlag = 1;
	mgr_req_frame->value.dev.devID.diskDev.diskID = disk_id;
	mgr_req_frame->value.dev.num = vd_num;

	ps3_mgr_req_frame_sge_build(mgr_req_frame,
		dev_context->vd_info_buf_phys_sync,
		vd_info_size);

	LOG_INFO("host_no:%u ready send, reqFrameId=%d, disk_id=0x%x, num=%d !\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd), disk_id, vd_num);

	ret = ps3_mgr_cmd_sync_proc(instance, cmd, PS3_DEFAULT_MGR_CMD_TIMEOUT);
	LOG_INFO("host_no:%u get vd info, reqFrameId=%d, disk_id=0x%x, num=%d finish!\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd), disk_id, vd_num);
l_out:
	if ((ret != -PS3_CMD_NO_RESP) && (cmd != NULL)) {
		ps3_mgr_cmd_free(instance, cmd);
	}
	LOG_DEBUG("host_no:%u exit !\n", PS3_HOST(instance));
	return ret;
}

S32 ps3_vd_info_async_get(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;
	U32 vd_info_size = PS3_MAX_VD_COUNT(instance) *
		sizeof(struct PS3VDEntry) + sizeof(struct PS3VDInfo);
	struct ps3_dev_context *dev_context = &instance->dev_context;

	LOG_DEBUG("host_no:%u enter !\n", PS3_HOST(instance));

	cmd = dev_context->vd_pending_cmd;
	if (cmd == NULL) {
		cmd = ps3_mgr_cmd_alloc(instance);
		if (cmd == NULL) {
			LOG_FILE_ERROR("host_no:%u mgr cmd get failed !\n",
				PS3_HOST(instance));
			ret = -PS3_EBUSY;
			goto l_out;
		}

		mgr_req_frame = &cmd->req_frame->mgrReq;
		memset(dev_context->vd_info_buf_async,
			0, vd_info_size);
		memset(mgr_req_frame, 0, sizeof(*mgr_req_frame));

		ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_GET_VD_INFO);
		mgr_req_frame->reqHead.timeout = 0;
		mgr_req_frame->timeout = 0;
		mgr_req_frame->syncFlag = 0;
		mgr_req_frame->pendingFlag = 1;

		ps3_mgr_req_frame_sge_build(mgr_req_frame,
			dev_context->vd_info_buf_phys_async,
			vd_info_size);

		ps3_vd_pending_filter_table_build((U8*)
			dev_context->vd_info_buf_async);

		ps3_mgr_cmd_word_build(cmd);
		LOG_FILE_INFO("host_no:%u ready send, reqFrameId=%d, t_id:0x%llx!\n",
                	PS3_HOST(instance), ps3_cmd_frame_id(cmd), cmd->trace_id);
		ps3_mgr_print_cmd(cmd, "vd info async", PS3_DRV_TRUE);
		dev_context->vd_pending_cmd = cmd;
		ret = ps3_cmd_send_async(instance, cmd, ps3_dev_vd_pending_proc);
		ps3_mgr_print_cmd(cmd, "vd info async", PS3_DRV_FALSE);
		if (ret != PS3_SUCCESS) {
			LOG_FILE_ERROR("host_no:%u send error, reqFrameId=%d, t_id:0x%llx ret:%d!\n",
                        	PS3_HOST(instance), ps3_cmd_frame_id(cmd),
                        	cmd->trace_id, ret);
			dev_context->vd_pending_cmd = NULL;
			ps3_mgr_cmd_free(instance, cmd);
		}
	} else {
		LOG_FILE_INFO("host_no:%u vd info already subscribed\n", PS3_HOST(instance));
	}
l_out:
	LOG_DEBUG("host_no:%u exit !\n", PS3_HOST(instance));
	return ret;
}

static inline void ps3_ctrl_info_capablity_dump(struct ps3_instance *instance)
{
	LOG_WARN("host_no:%u ctrl info--supportUnevenSpans:%d supportJbodSecure:%d "
		"supportNvmePassthru:%d supportDirectCmd:%d "
		"supportAcceleration:%d supportSataDirectCmd:%d supportSataNcq:%d ioTimeOut:%u cancelTimeOut:%u isotoneTimeOut:%u\n",
		PS3_HOST(instance),
		instance->ctrl_info.capabilities.supportUnevenSpans,
		instance->ctrl_info.capabilities.supportJbodSecure,
		instance->ctrl_info.capabilities.supportNvmePassthru,
		instance->ctrl_info.capabilities.supportDirectCmd,
		instance->ctrl_info.capabilities.supportAcceleration,
		instance->ctrl_info.capabilities.supportSataDirectCmd,
		instance->ctrl_info.capabilities.supportSataNcq,
		instance->ctrl_info.ioTimeOut,
		instance->ctrl_info.cancelTimeOut,
		instance->ctrl_info.isotoneTimeOut
		);
}

static void ps3_ctrl_info_update(struct ps3_instance *instance)
{
	Bool is_need_dump_info = (0 != memcmp(&instance->ctrl_info,
		instance->ctrl_info_buf, sizeof(instance->ctrl_info)));

	memcpy(&instance->ctrl_info, instance->ctrl_info_buf,
		sizeof(instance->ctrl_info));

	if (is_need_dump_info) {
		ps3_ctrl_info_capablity_dump(instance);
	}

	if (instance->ctrl_info.capabilities.supportDirectCmd) {
		instance->cmd_attr.is_support_direct_cmd = PS3_DRV_TRUE;
		LOG_INFO("host_no:%u change is_support_direct_cmd to :%d !\n",
			PS3_HOST(instance),
			instance->cmd_attr.is_support_direct_cmd);
	} else {
		instance->cmd_attr.is_support_direct_cmd = PS3_DRV_FALSE;
		LOG_INFO("host_no:%u change is_support_direct_cmd to :%d !\n",
			PS3_HOST(instance),
			instance->cmd_attr.is_support_direct_cmd);
	}

	instance->cmd_attr.vd_io_threshold =
		le32_to_cpu(instance->ctrl_info.vdIOThreshold);

	LOG_INFO("host_no:%u change vdIOThreshold to :%d !\n",
		PS3_HOST(instance),
		instance->cmd_attr.vd_io_threshold);
	ps3_perf_update(instance, instance->ctrl_info.iocPerfMode);
	if (instance->ctrl_info.vdQueueNum == 0) {
		LOG_ERROR("host_no:%u ctrl info update vd Queue Num is 0!\n",
			PS3_HOST(instance));
		instance->ctrl_info.vdQueueNum = 1;
	}
	LOG_DEBUG("host_no:%u offsetOfVDID:%u\n",
		PS3_HOST(instance), instance->ctrl_info.offsetOfVDID);

	LOG_INFO("host_no:%u change is_balance_current_perf_mode to :%d !\n",
		PS3_HOST(instance),
		instance->irq_context.is_balance_current_perf_mode);
}

S32 ps3_ctrl_info_get(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;

	LOG_DEBUG("host_no:%u enter !\n", PS3_HOST(instance));

	cmd = ps3_mgr_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_ERROR("host_no:%u mgr cmd get NOK !\n",
			PS3_HOST(instance));
		ret = -PS3_EBUSY;
		goto l_out;
	}

	mgr_req_frame = &cmd->req_frame->mgrReq;
	memset(instance->ctrl_info_buf, 0, sizeof(struct PS3IocCtrlInfo));
	memset(mgr_req_frame, 0, sizeof(*mgr_req_frame));

	ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_GET_CTRL_INFO);
	mgr_req_frame->timeout = 0;
	mgr_req_frame->syncFlag = 1;
	cmd->is_force_polling = 1;

	ps3_mgr_req_frame_sge_build(mgr_req_frame,
		instance->ctrl_info_buf_h,
		sizeof(struct PS3IocCtrlInfo));

	ret = ps3_mgr_cmd_sync_proc(instance, cmd, PS3_DEFAULT_MGR_CMD_TIMEOUT);
	if (ret == PS3_SUCCESS) {
		ps3_ctrl_info_update(instance);
	}
l_out:
	if ((ret != -PS3_CMD_NO_RESP) && (cmd != NULL)) {
		ps3_mgr_cmd_free(instance, cmd);
	}
	LOG_DEBUG("host_no:%u exit !\n", PS3_HOST(instance));
	return ret;
}

S32 ps3_soc_unload(struct ps3_instance *instance, Bool is_polling, U8 type, U8 suspend_type)
{
	S32 ret = PS3_SUCCESS;
	U32 i = 0;
	struct ps3_cmd *cmd = NULL;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;

	LOG_DEBUG("host_no:%u enter !\n", PS3_HOST(instance));

	if (!ps3_check_ioc_state_is_normal_in_unload(instance)) {
		ret = -PS3_FAILED;
		goto l_out;
	}

	ps3_wait_scsi_cmd_done(instance, PS3_TRUE);
	while ((ps3_atomic_read(&instance->cmd_statistics.cmd_delivering) != 0) &&
		++i < PS3_FW_STATE_TO_UNLOAD_TMO_LOOP_COUNT) {
		LOG_INFO("host_no:%u commands in delivering, wait a while!\n",
			PS3_HOST(instance));
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
	}

	cmd = ps3_mgr_cmd_alloc(instance);
	INJECT_START(PS3_ERR_IJ_ROMOVE_UNLOAD_FAIL, &cmd)
	if (cmd == NULL) {
		LOG_ERROR("host_no:%u mgr cmd get NOK !\n",
			PS3_HOST(instance));
		ret = -PS3_EBUSY;
		goto l_out;
	}

	mgr_req_frame = &cmd->req_frame->mgrReq;
	memset(mgr_req_frame, 0, sizeof(*mgr_req_frame));

	ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_UNLOAD);
	mgr_req_frame->reqHead.timeout = instance->unload_timeout;
	mgr_req_frame->sgeCount = 0;
	mgr_req_frame->timeout = 0;
	mgr_req_frame->syncFlag = 1;
	mgr_req_frame->osType = PS3_LINUX_FRAME;
	mgr_req_frame->value.unLoadType = type;
	mgr_req_frame->suspend_type = suspend_type;
	cmd->is_force_polling = is_polling;

	LOG_WARN("host_no:%u ready send unload, reqFrameId=%d suspend_type:%d!\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd), suspend_type);
	ret = ps3_mgr_unload_cmd_send(cmd, instance->unload_timeout);
	INJECT_START(PS3_ERR_IJ_WAIT_SUSPEND_WAIT_RECOVERY, &ret)
	LOG_WARN("host_no:%u reqFrameId=%d finished ret:%d!\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd), ret);
l_out:
	if (cmd != NULL) {
		ps3_mgr_cmd_free(instance, cmd);
	}
	LOG_DEBUG("host_no:%u exit !\n", PS3_HOST(instance));
	return ret;
}

struct ps3_cmd *ps3_dump_notify_cmd_build(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;
	struct ps3_dump_context *dump_context = &instance->dump_context;

	LOG_DEBUG("host_no:%u enter !\n", PS3_HOST(instance));
	cmd = ps3_mgr_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_ERROR("host_no:%u mgr cmd get failed !\n",
			PS3_HOST(instance));
		goto l_out;
	}

	mgr_req_frame = &cmd->req_frame->mgrReq;
	memset(dump_context->dump_dma_buf, 0, PS3_DUMP_DMA_BUF_SIZE);
	memset(mgr_req_frame, 0, sizeof(*mgr_req_frame));

	ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_AUTODUMP_NOTIFY);

	mgr_req_frame->timeout = 0;
	mgr_req_frame->syncFlag = 0;
	mgr_req_frame->pendingFlag = 1;

	ps3_mgr_req_frame_sge_build(mgr_req_frame, dump_context->dump_dma_addr,
		PS3_DUMP_DMA_BUF_SIZE);

	ps3_mgr_cmd_word_build(cmd);

	LOG_DEBUG("host_no:%u reqFrameId=%d !\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd));

l_out:
	LOG_DEBUG("host_no:%u exit ! ret %d\n", PS3_HOST(instance), ret);
	return cmd;
}

S32 ps3_scsi_remove_device_done(struct ps3_instance *instance,
	struct PS3DiskDevPos *disk_pos, U8 dev_type)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;

	LOG_DEBUG("host_no:%u enter !\n", PS3_HOST(instance));

	cmd = ps3_mgr_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_ERROR("host_no:%u mgr cmd get NOK !\n",
			PS3_HOST(instance));
		ret = -PS3_EBUSY;
		goto l_out;
	}

	mgr_req_frame = &cmd->req_frame->mgrReq;
	memset(mgr_req_frame, 0, sizeof(*mgr_req_frame));

	ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_DEV_DEL_DONE);
	mgr_req_frame->timeout = 0;
	mgr_req_frame->syncFlag = 1;
	mgr_req_frame->reqHead.devID.ps3Dev = disk_pos->diskDev.ps3Dev;
	mgr_req_frame->value.dev.devID.diskDev = disk_pos->diskDev;
	mgr_req_frame->value.dev.devID.diskMagicNum = disk_pos->diskMagicNum;
	mgr_req_frame->value.dev.devType = dev_type;


	LOG_INFO("host_no:%u ready send, t_id:0x%llx reqFrameId=%d, dev_type=%d, "
		"[%d:%d:%d:%u]!\n",
		PS3_HOST(instance), cmd->trace_id, ps3_cmd_frame_id(cmd), dev_type,
		PS3_CHANNEL(disk_pos), PS3_TARGET(disk_pos),
		PS3_VDID(disk_pos), disk_pos->diskMagicNum);

	ret = ps3_mgr_cmd_sync_proc(instance, cmd, PS3_DEFAULT_MGR_CMD_TIMEOUT);
	if (ret != PS3_SUCCESS) {
		LOG_INFO("host_no:%u send [%u:%u:%u:%u][type:%d] del done failed!:ret = %d\n",
			PS3_HOST(instance), PS3_CHANNEL(disk_pos),
			PS3_TARGET(disk_pos), PS3_VDID(disk_pos),
			disk_pos->diskMagicNum,
			dev_type, ret);
	}

l_out:
	if ((ret != -PS3_CMD_NO_RESP) && (cmd != NULL)) {
		ps3_mgr_cmd_free(instance, cmd);
	}

	LOG_INFO("host_no:%u send [%d:%d:%d:%u] finish!:ret = %d\n",
		PS3_HOST(instance), PS3_CHANNEL(disk_pos),
		PS3_TARGET(disk_pos), PS3_VDID(disk_pos),
		disk_pos->diskMagicNum, ret);

	LOG_DEBUG("host_no:%u exit !\n", PS3_HOST(instance));
	return ret;
}

S32 ps3_scsi_add_device_ack(struct ps3_instance *instance,
	struct PS3DiskDevPos *disk_pos, U8 dev_type)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;

	LOG_DEBUG("host_no:%u enter !\n", PS3_HOST(instance));

	cmd = ps3_mgr_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_ERROR("host_no:%u mgr cmd get NOK !\n",
			PS3_HOST(instance));
		ret = -PS3_EBUSY;
		goto l_out;
	}

	mgr_req_frame = &cmd->req_frame->mgrReq;
	memset(mgr_req_frame, 0, sizeof(*mgr_req_frame));

	ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_DEV_ADD_ACK);
	mgr_req_frame->timeout = 0;
	mgr_req_frame->syncFlag = 1;
	mgr_req_frame->reqHead.devID.ps3Dev = disk_pos->diskDev.ps3Dev;
	mgr_req_frame->value.dev.devID.diskDev = disk_pos->diskDev;
	mgr_req_frame->value.dev.devID.diskMagicNum = disk_pos->diskMagicNum;
	mgr_req_frame->value.dev.devType = dev_type;

	LOG_INFO("t_id:0x%llx host_no:%u ready send, CFID:%d dev_type:%d "
		"[%u:%u:%u:%u]!\n",
		cmd->trace_id, PS3_HOST(instance), ps3_cmd_frame_id(cmd), dev_type,
		PS3_CHANNEL(disk_pos), PS3_TARGET(disk_pos),
		PS3_VDID(disk_pos), disk_pos->diskMagicNum);

	ret = ps3_mgr_cmd_sync_proc(instance, cmd, PS3_DEFAULT_MGR_CMD_TIMEOUT);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("host_no:%u send [%u:%u:%u:%u][type:%d] add ack NOK!:ret = %d\n",
			PS3_HOST(instance), PS3_CHANNEL(disk_pos),
			PS3_TARGET(disk_pos), PS3_VDID(disk_pos),
			disk_pos->diskMagicNum,
			dev_type, ret);
	}

l_out:
	if ((ret != -PS3_CMD_NO_RESP) && (cmd != NULL)) {
		ps3_mgr_cmd_free(instance, cmd);
	}

	LOG_INFO("host_no:%u send [%u:%u:%u:%u] finish!:ret = %d\n",
		PS3_HOST(instance), PS3_CHANNEL(disk_pos),
		PS3_TARGET(disk_pos), PS3_VDID(disk_pos),
		disk_pos->diskMagicNum, ret);

	LOG_DEBUG("host_no:%u exit !\n", PS3_HOST(instance));
	return ret;
}

S32 ps3_mgr_cmd_cancel(struct ps3_instance *instance, U16 cancel_cmd_frame_id)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;
	U16 cancel_time_out = PS3_CANCEL_MGR_CMD_TIMEOUT;

	LOG_DEBUG("host_no:%u enter, be cancel CFID:%d !\n",
		PS3_HOST(instance), cancel_cmd_frame_id);

	cmd = ps3_task_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_FILE_ERROR("host_no:%u task cmd get failed !\n",
			PS3_HOST(instance));
		ret = -PS3_EBUSY;
		goto l_out;
	}

	if (instance->ctrl_info.cancelTimeOut >  cancel_time_out) {
		cancel_time_out = instance->ctrl_info.cancelTimeOut;
	}

	mgr_req_frame = &cmd->req_frame->mgrReq;
	memset(mgr_req_frame, 0, sizeof(*mgr_req_frame));

	ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_CANCEL);
	mgr_req_frame->reqHead.timeout = cancel_time_out;
	mgr_req_frame->sgeCount = 0;
	mgr_req_frame->timeout = 0;
	mgr_req_frame->syncFlag = 1;
	mgr_req_frame->abortFlag = 0;
	mgr_req_frame->value.originalCmdFrameID = cancel_cmd_frame_id;

	LOG_FILE_INFO("host_no:%u ready send, reqFrameId=%d cancel_cmd_frame_id:%u !\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd), cancel_cmd_frame_id);

	INJECT_START(PS3_ERR_IJ_IOCTL_CMD_RECOVERY_CANCEL, cmd)
	ret = ps3_mgr_cmd_sync_proc(instance, cmd, cancel_time_out);
	LOG_FILE_INFO("host_no:%u reqFrameId=%d cancel_cmd_frame_id:%u finished!\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd), cancel_cmd_frame_id);
l_out:
	if ((ret != -PS3_CMD_NO_RESP) && (cmd != NULL)) {
		ps3_task_cmd_free(instance, cmd);
	}
	LOG_DEBUG("host_no:%u exit !\n", PS3_HOST(instance));
	return ret;
}
S32 ps3_mgr_cmd_cancel_send(struct ps3_instance *instance,
	U16 cancel_cmd_frame_id, U8 type)
{
	S32 ret = PS3_SUCCESS;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;
	enum PS3MgrCmdSubType sub_type;
	struct ps3_cmd *cmd = NULL;
	LOG_DEBUG("host_no:%u enter, be cancel CFID:%d !\n",
		PS3_HOST(instance), cancel_cmd_frame_id);

	cmd = ps3_task_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_FILE_ERROR("host_no:%u task cmd get failed !\n",
			PS3_HOST(instance));
		ret = -PS3_ENOMEM;
		goto l_out;
	}

	mgr_req_frame = &cmd->req_frame->mgrReq;
	memset(mgr_req_frame, 0, sizeof(*mgr_req_frame));

	ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_CANCEL);
	mgr_req_frame->sgeCount = 0;
	mgr_req_frame->timeout = 0;
	mgr_req_frame->syncFlag = 1;
	mgr_req_frame->abortFlag = 0;
	mgr_req_frame->value.originalCmdFrameID = cancel_cmd_frame_id;

	LOG_FILE_INFO("host_no:%u ready send, reqFrameId=%d cancel_cmd_frame_id:%u !\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd), cancel_cmd_frame_id);

	sub_type = (enum PS3MgrCmdSubType)PS3_MGR_CMD_SUBTYPE(cmd);

	cmd->time_out = PS3_DEFAULT_MGR_CMD_TIMEOUT;
	cmd->is_interrupt = PS3_DRV_FALSE;
	ps3_mgr_cmd_word_build(cmd);

	ps3_mgr_print_cmd(cmd,
		namePS3MgrCmdSubType(sub_type), PS3_DRV_TRUE);
	if(type == PS3_CANCEL_EVENT_CMD){
		instance->event_context.event_abort_cmd = cmd;
	}else if(type == PS3_CANCEL_WEB_CMD){
        instance->webSubscribe_context.web_abort_cmd = cmd;
    }else{
		instance->dev_context.vdpending_abort_cmd = cmd;
	}
	ret = ps3_cmd_no_block_send(instance, cmd);
	if (ret != PS3_SUCCESS) {
		LOG_FILE_INFO("host_no:%u CFID:%d trace_id:0x%llx send failed\n",
			PS3_HOST(instance),
			ps3_cmd_frame_id(cmd), cmd->trace_id);
	}
	ps3_mgr_print_cmd(cmd,
		namePS3MgrCmdSubType(sub_type), PS3_DRV_FALSE);
l_out:
	return ret;
}
S32 ps3_mgr_cmd_cancel_wait(struct ps3_instance *instance, U8 type)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *abort_cmd = NULL;
	ULong flags = 0;
	S32 send_result;
	if (type == PS3_CANCEL_EVENT_CMD){
		cmd = instance->event_context.event_abort_cmd;
		if (cmd == NULL){
			LOG_INFO("host_no:%u event abort cmd null!\n",
				PS3_HOST(instance));
			ret = -PS3_FAILED;
			goto l_out;
		}
	}else if(type == PS3_CANCEL_WEB_CMD){
        cmd = instance->webSubscribe_context.web_abort_cmd;
        if (cmd == NULL){
            LOG_INFO("host_no:%u web abort cmd null!\n",
                PS3_HOST(instance));
            ret = -PS3_FAILED;
            goto l_out;
        }
	}else{
		cmd = instance->dev_context.vdpending_abort_cmd;
		if (cmd == NULL){
			LOG_INFO("host_no:%u vdpending abort cmd null!\n",
				PS3_HOST(instance));
			ret = -PS3_FAILED;
			goto l_out;
		}
	}
	send_result = ps3_block_cmd_wait(instance, cmd, 0);

	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	if(cmd->cmd_state.state != PS3_CMD_STATE_COMPLETE){
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
		ret = -PS3_RECOVERED;
		goto l_out;
	}
	ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);

	ret = ps3_mgr_complete_proc(instance, cmd, send_result);

	LOG_INFO("host_no:%u reqFrameId=%d finished!\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd));
	if ((ret != -PS3_CMD_NO_RESP) && (cmd != NULL)) {
		if(type == PS3_CANCEL_EVENT_CMD){
			abort_cmd = instance->event_context.event_abort_cmd;
			instance->event_context.event_abort_cmd = NULL;
			if (abort_cmd != NULL) {
				ps3_task_cmd_free(instance, abort_cmd);
			}
		}else if(type == PS3_CANCEL_WEB_CMD){
            abort_cmd = instance->webSubscribe_context.web_abort_cmd;
            instance->webSubscribe_context.web_abort_cmd = NULL;
            if (abort_cmd != NULL) {
                ps3_task_cmd_free(instance, abort_cmd);
            }
        }else{
			abort_cmd = instance->dev_context.vdpending_abort_cmd;
			instance->dev_context.vdpending_abort_cmd = NULL;
			if (abort_cmd != NULL) {
				ps3_task_cmd_free(instance, abort_cmd);
			}
		}
	}
l_out:
	return ret;
}

S32 ps3_event_register(struct ps3_instance *instance,
		struct PS3MgrEvent *event)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;

	LOG_DEBUG("host_no:%u enter !\n", PS3_HOST(instance));

	cmd = ps3_mgr_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_ERROR_IN_IRQ(instance, "host_no:%u mgr cmd get failed !\n",
			PS3_HOST(instance));
		ret = -PS3_EBUSY;
		goto l_out;
	}

	mgr_req_frame = &cmd->req_frame->mgrReq;
	memset(mgr_req_frame, 0, sizeof(*mgr_req_frame));

	ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_SUBSCRIBE_EVENT);
	mgr_req_frame->reqHead.timeout = 0;
	mgr_req_frame->timeout = 0;
	mgr_req_frame->syncFlag = 0;
	mgr_req_frame->pendingFlag = 1;

	mgr_req_frame->value.event.eventTypeMap =
		event->eventTypeMap;
	mgr_req_frame->value.event.eventTypeMapProcResult =
		event->eventTypeMapProcResult;
	mgr_req_frame->value.event.eventLevel = event->eventLevel;

	instance->event_context.event_info = (struct PS3EventInfo*)cmd->ext_buf;

	ps3_mgr_req_frame_sge_build(mgr_req_frame,
		cmd->ext_buf_phys,
		cmd->instance->cmd_context.ext_buf_size);

	memset(cmd->ext_buf, 0, cmd->instance->cmd_context.ext_buf_size);
	if (instance->ioc_adpter->event_filter_table_get != NULL) {
		instance->ioc_adpter->event_filter_table_get((U8*)cmd->ext_buf);
	}

	ps3_mgr_cmd_word_build(cmd);
	LOG_INFO_IN_IRQ(instance, "host_no:%u ready send, reqFrameId=%d !\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd));

	ps3_mgr_print_cmd(cmd, "event register", PS3_DRV_TRUE);
	instance->event_context.event_cmd = cmd;
	ret = ps3_cmd_send_async(instance, cmd, ps3_event_service);
	if (ret != PS3_SUCCESS) {
		instance->event_context.event_cmd = NULL;
		ps3_mgr_cmd_free(instance, cmd);
	}
	ps3_mgr_print_cmd(cmd, "event register", PS3_DRV_FALSE);
l_out:
	LOG_DEBUG("host_no:%u exit !\n", PS3_HOST(instance));
	return ret;
}

S32 ps3_web_register(struct ps3_instance *instance)
{
    S32 ret = PS3_SUCCESS;
    struct ps3_cmd *cmd = NULL;
    PS3MgrReqFrame_s *mgr_req_frame = NULL;
    struct ps3_webSubscribe_context *web_context = &instance->webSubscribe_context;

    cmd = web_context->webSubscribe_cmd;

    if (cmd == NULL) {
        cmd = ps3_mgr_cmd_alloc(instance);
        if (cmd == NULL) {
			LOG_WARN_IN_IRQ(instance,
				"host_no:%u ioctl req, Failed to get a cmd packet\n",
				PS3_HOST(instance));
            ret = -PS3_FAILED;
            goto l_out;
        }
        mgr_req_frame = &cmd->req_frame->mgrReq;
        memset(mgr_req_frame, 0, sizeof(struct PS3MgrReqFrame));

        ps3_mgr_req_head_init(cmd, &mgr_req_frame->reqHead, PS3_MGR_CMD_WEBSUBSCRIBE_EVENT);
		mgr_req_frame->reqHead.timeout = 0;
        mgr_req_frame->timeout = 0;
        mgr_req_frame->syncFlag = 0;
        mgr_req_frame->pendingFlag = 1;
        ps3_mgr_cmd_word_build(cmd);
		LOG_INFO_IN_IRQ(instance, "host_no:%u ready send, reqFrameId=%d !\n",
			PS3_HOST(instance), ps3_cmd_frame_id(cmd));
        ps3_mgr_print_cmd(cmd, "web event register", PS3_DRV_TRUE);

        web_context->webSubscribe_cmd = cmd;

        ret = ps3_cmd_send_async(instance, cmd, ps3_webSubscribe_service);
        if (ret != PS3_SUCCESS) {
            web_context->webSubscribe_cmd = NULL;
            ps3_mgr_cmd_free(instance, cmd);
            goto l_out;
        }
    }else {
		LOG_INFO_IN_IRQ(instance, "host_no:%u web event already subscribed\n",
			PS3_HOST(instance));
    }
l_out:
    LOG_DEBUG("host_no:%u exit !\n", PS3_HOST(instance));
    return ret;
}

static S32 ps3_scsi_task_abort_sync_proc(struct ps3_instance *instance,
	struct ps3_cmd *cmd, struct ps3_cmd *abort_cmd,
	struct ps3_scsi_priv_data *priv_data)
{
	S32 ret = PS3_SUCCESS;
	S32 send_result = PS3_SUCCESS;

	cmd->cmd_word_value = abort_cmd->cmd_word_value;
	if (cmd->cmd_word.type == PS3_CMDWORD_TYPE_INIT) {
		ret = -PS3_FAILED;
		goto end;
	}
	cmd->cmd_word.direct = PS3_CMDWORD_DIRECT_NORMAL;
	cmd->cmd_word.cmdFrameID = ps3_cmd_frame_id(cmd);
	cmd->cmd_word.type = PS3_CMDWORD_TYPE_ABORT;
	cmd->time_out = priv_data->task_abort_timeout;
	cmd->is_interrupt = PS3_DRV_FALSE;

	INJECT_START(PS3_ERR_IJ_ABORT_PRE_SEND_FORCE_ABORTED_CMD_DONE, abort_cmd->scmd)
	send_result = ps3_cmd_send_sync(instance, cmd);
	ret = ps3_mgr_complete_proc(instance, cmd, send_result);
end:
	return ret;
}

static S32 ps3_scsi_task_reset_sync_proc(struct ps3_instance *instance,
	struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	S32 send_result = PS3_SUCCESS;

	send_result = ps3_cmd_send_sync(instance, cmd);
	ret = ps3_mgr_complete_proc(instance, cmd, send_result);

	return ret;
}

static inline void ps3_mgr_scsi_req_head_init(struct ps3_cmd *cmd,
	PS3ReqFrameHead_s *req_header, U8  cmdSubType, U32 disk_id,
	struct ps3_scsi_priv_data *priv_data)
{
	if (cmdSubType == PS3_TASK_CMD_SCSI_TASK_RESET) {
		req_header->timeout = priv_data->task_reset_timeout;
	} else {
		req_header->timeout = priv_data->task_abort_timeout;
	}
	req_header->traceID = ps3_cmd_trace_id(cmd);
	req_header->cmdType = PS3_CMD_SCSI_TASK_MANAGEMENT;
	req_header->cmdSubType = cmdSubType;
	req_header->cmdFrameID = ps3_cmd_frame_id(cmd);
	req_header->control = 0;
	req_header->devID.diskID = disk_id;
}

struct ps3_cmd * ps3_scsi_task_mgr_reset_build(struct ps3_instance *instance,
	struct ps3_scsi_priv_data *priv_data)
{
	struct ps3_cmd *cmd = NULL;
	PS3MgrTaskReqFrame_s *mgr_task_req_frame = NULL;
	U32 disk_id = priv_data->disk_pos.diskDev.diskID;

	LOG_DEBUG("host_no:%u enter !\n", PS3_HOST(instance));

	if ((priv_data->dev_type == PS3_DEV_TYPE_UNKNOWN) ||
		(disk_id == PS3_INVALID_DEV_ID)) {
		LOG_ERROR("host_no:%u unknow dev type !\n",
			PS3_HOST(instance));
		goto l_out;
	}

	cmd = ps3_task_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_ERROR("host_no:%u task cmd get NOK !\n",
			PS3_HOST(instance));
		goto l_out;
	}

	mgr_task_req_frame = &cmd->req_frame->taskReq;
	mgr_task_req_frame->taskID = 0; 
	memset(mgr_task_req_frame, 0, sizeof(*mgr_task_req_frame));
	ps3_mgr_scsi_req_head_init(cmd, &mgr_task_req_frame->reqHead,
		PS3_TASK_CMD_SCSI_TASK_RESET, disk_id, priv_data);

	LOG_WARN("host_no:%u ready send reset, CFID:%u t_id:0x%llx [%u:%u:%u]!\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd), cmd->trace_id,
		PS3_CHANNEL(&priv_data->disk_pos),
		PS3_TARGET(&priv_data->disk_pos),
		PS3_VDID(&priv_data->disk_pos));

	ps3_mgr_cmd_word_build(cmd);

	if (priv_data->dev_type == PS3_DEV_TYPE_VD) {
		cmd->cmd_word.virtDiskID = (U8)priv_data->disk_pos.diskDev.ps3Dev.virtDiskID;
	} else {
		cmd->cmd_word.phyDiskID = priv_data->disk_pos.diskDev.ps3Dev.phyDiskID;
	}

	cmd->time_out = priv_data->task_reset_timeout;
	cmd->is_interrupt = PS3_FALSE;

l_out:
	return cmd;
}

#ifndef _WINDOWS	
#if 0
static void ps3_abort_req_frame_dump(struct ps3_cmd *cmd)
{
	PS3_BUG_ON((cmd->req_frame == NULL) || (cmd->scmd == NULL));

	switch (cmd->cmd_word.type)
	{
	case PS3_CMDWORD_TYPE_MGR:
		if ((cmd->req_frame->word[0] == PS3_CMD_VD_SCSI_IO_NORW) ||
			(cmd->req_frame->word[0] == PS3_CMD_PD_SCSI_IO_NORW)) {
			LOG_DEBUG("t_id:0x%llx host_no:%u aborted_cmd CFID:%u "
				"type:%d fe isr_sn:%d cmd_type:%d opcode:0x%x!\n",
				cmd->trace_id, PS3_HOST(cmd->instance),
				cmd->cmd_word.cmdFrameID, cmd->cmd_word.type,
				cmd->cmd_word.isrSN,
				cmd->req_frame->frontendReq.reqHead.cmdType,
				cmd->scmd->cmnd[0]);
		} else {
			LOG_ERROR("host_no:%u abort_cmd no scsi cmd! cmd_type:%d\n",
				PS3_HOST(cmd->instance),
				(int)cmd->req_frame->word[0]);
		}
	break;
	case PS3_CMDWORD_TYPE_READ:
	case PS3_CMDWORD_TYPE_WRITE:
		if (cmd->cmd_word.isHardCmd) {
			LOG_DEBUG("t_id:0x%llx host_no:%u abort_cmd CFID:%u type:%d "
				"hwReq isr_sn:%d opcode:0x%x!\n",
				cmd->trace_id, PS3_HOST(cmd->instance),
				cmd->cmd_word.cmdFrameID, cmd->cmd_word.type,
				cmd->cmd_word.isrSN, cmd->scmd->cmnd[0]);
		} else {
			LOG_DEBUG("t_id:0x%llx host_no:%u abort_cmd CFID:%u type:%d "
				"fe isr_sn:%d opcode:0x%x cmd_type:%u !\n",
				cmd->trace_id, PS3_HOST(cmd->instance),
				cmd->cmd_word.cmdFrameID, cmd->cmd_word.type,
				cmd->cmd_word.isrSN, cmd->scmd->cmnd[0],
				cmd->req_frame->frontendReq.reqHead.cmdType);
		}
	break;
	default:
		LOG_ERROR("host_no:%u abort_cmd no scsi cmd! \n",
			PS3_HOST(cmd->instance));
		break;
	}
	return;
}
#endif
S32 ps3_scsi_task_mgr_abort(struct ps3_instance *instance,
	struct ps3_scsi_priv_data *priv_data, U16 aborted_cmd_frame_id, struct scsi_cmnd *scmd)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *aborted_cmd = NULL;
	PS3MgrTaskReqFrame_s *mgr_task_req_frame = NULL;
	U32 disk_id = priv_data->disk_pos.diskDev.diskID;

	LOG_DEBUG("host_no:%u enter !\n", PS3_HOST(instance));
	if ((priv_data->dev_type == PS3_DEV_TYPE_UNKNOWN) ||
		(disk_id == PS3_INVALID_DEV_ID)) {
		LOG_ERROR("host_no:%u unknow dev type !\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	INJECT_START(PS3_ERR_IJ_ABORT_PRE_BULID_FORCE_ABORTED_CMD_DONE, scmd)
	aborted_cmd = ps3_cmd_find(instance, aborted_cmd_frame_id);
	if (aborted_cmd == NULL || aborted_cmd->scmd == NULL) {
		LOG_ERROR("host_no:%u there is no aborted cmd CFID:%u\n",
			PS3_HOST(instance), aborted_cmd_frame_id);

		ret = PS3_SUCCESS;
		goto l_out;
	}

	cmd = ps3_task_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_ERROR("host_no:%u task cmd get NOK !\n",
			PS3_HOST(instance));
		ret = -PS3_EBUSY;
		goto l_out;
	}

	LOG_DEBUG("t_id:0x%llx aborted_t_id:0x%llx host_no:%u mgr abort CFID:%d\n",
		 cmd->trace_id, aborted_cmd->trace_id, PS3_HOST(instance), cmd->index);

	mgr_task_req_frame = &cmd->req_frame->taskReq;
	memset(mgr_task_req_frame, 0, sizeof(*mgr_task_req_frame));

	ps3_mgr_scsi_req_head_init(cmd, &mgr_task_req_frame->reqHead,
		PS3_TASK_CMD_SCSI_TASK_ABORT, disk_id, priv_data);
	mgr_task_req_frame->taskID = aborted_cmd_frame_id;
	mgr_task_req_frame->abortedCmdType = aborted_cmd->cmd_word.direct;
	mgr_task_req_frame->reqHead.reqFrameFormat = PS3_REQFRAME_FORMAT_FRONTEND;

	LOG_WARN("host_no:%u ready send abort, CFID:%d t_id:0x%llx aborted cmd info:"
		"CFID:%d t_id:0x%llx op:0x%x is_retry_cmd:%d timeout:%d dev[%u:%u:%u:%u], "
		"cmdword[0x%08llx]{type:%d direct:%d que:%d isr_sn:%d}!\n ",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd), cmd->trace_id,
		aborted_cmd_frame_id, aborted_cmd->trace_id,
		scmd->cmd_len > 0 ? scmd->cmnd[0] : 0xff,
		scmd->retries,
		SCMD_GET_REQUEST(scmd)->timeout, PS3_CHANNEL(&priv_data->disk_pos),
		PS3_TARGET(&priv_data->disk_pos),
		PS3_VDID(&priv_data->disk_pos),
		priv_data->disk_pos.diskMagicNum, aborted_cmd->cmd_word_value,
		aborted_cmd->cmd_word.type, aborted_cmd->cmd_word.direct,
		aborted_cmd->cmd_word.qMask, aborted_cmd->cmd_word.isrSN);

	ps3_scsih_print_req(aborted_cmd, LEVEL_INFO);
	INJECT_START(PS3_ERR_IJ_ABORT_PRE_BULID1_FORCE_ABORTED_CMD_DONE, scmd)
	INJECT_START(PS3_ERR_IJ_SET_ABORT_COUNT, scmd);

	ret = ps3_scsi_task_abort_sync_proc(instance, cmd, aborted_cmd, priv_data);

	LOG_WARN("host_no:%u abort finish, CFID:%d aborted CFID:%d aborted cmdword[0x%08llx], "
		"[%u:%u:%u:%u]!\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd), aborted_cmd_frame_id, aborted_cmd->cmd_word_value,
		PS3_CHANNEL(&priv_data->disk_pos),
		PS3_TARGET(&priv_data->disk_pos),
		PS3_VDID(&priv_data->disk_pos),
		priv_data->disk_pos.diskMagicNum);

l_out:
	if ((ret != -PS3_CMD_NO_RESP) && (cmd != NULL)) {
		ps3_task_cmd_free(instance, cmd);
	}
	LOG_WARN("host_no:%u exit !, ret:%d\n", PS3_HOST(instance), ret);
	return ret;
}
#endif
S32 ps3_scsi_task_mgr_reset(struct ps3_instance *instance,
	struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;

	ret = ps3_scsi_task_reset_sync_proc(instance, cmd);

	LOG_INFO("host_no:%u reset finish CFID:%u t_id:0x%llx!\n",
		PS3_HOST(instance), ps3_cmd_frame_id(cmd), cmd->trace_id);

	if ((ret != -PS3_CMD_NO_RESP) && (cmd != NULL)) {
		ps3_task_cmd_free(instance, cmd);
	}
	LOG_INFO("host_no:%u exit !\n", PS3_HOST(instance));

	return ret;
}
#ifndef _WINDOWS 
static inline void ps3_sas_info_reqframe_build(struct ps3_cmd *cmd,
	enum PS3MgrCmdSubType sub_type, dma_addr_t *sge_addr, struct PS3SasMgr *sas_req)
{
	PS3MgrReqFrame_s *mgrReq = &cmd->req_frame->mgrReq;
	mgrReq->reqHead.timeout = PS3_DEFAULT_MGR_CMD_TIMEOUT;
	mgrReq->reqHead.traceID = cmd->trace_id;
	mgrReq->reqHead.cmdType = PS3_CMD_SAS_MANAGEMENT;
	mgrReq->reqHead.cmdSubType = sub_type;
	mgrReq->reqHead.cmdFrameID = cmd->index;
	mgrReq->reqHead.control = 0;
	mgrReq->syncFlag = 1;
	mgrReq->timeout = 0;
	mgrReq->sgeOffset =
		offsetof(PS3MgrReqFrame_s, sgl) >> PS3_MGR_CMD_SGL_OFFSET_DWORD_SHIFT;
	mgrReq->sgeCount = 1;
	mgrReq->sgl[0].length = cpu_to_le32(PS3_SAS_REQ_BUFF_LEN);
	mgrReq->sgl[0].addr = cpu_to_le64(*sge_addr);
	mgrReq->sgl[0].lastSge = 1;
	mgrReq->sgl[0].ext = 0;

	if (sas_req != NULL) {
		mgrReq->value.sasMgr.sasAddr =
			cpu_to_le64(sas_req->sasAddr);
		mgrReq->value.sasMgr.enclID = sas_req->enclID;
		mgrReq->value.sasMgr.startPhyID = sas_req->startPhyID;
		mgrReq->value.sasMgr.phyCount = sas_req->phyCount;
	}
}

static S32 __ps3_sas_info_get(struct ps3_instance *instance,
	enum PS3MgrCmdSubType sub_type, dma_addr_t *sge_addr, struct PS3SasMgr *sas_req)
{
	S32 ret = 0;
	struct ps3_cmd *cmd = NULL;

	cmd = ps3_mgr_cmd_alloc(instance);
	if (cmd == NULL) {
		LOG_WARN("host_no:%u not get a cmd packet\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	ps3_sas_info_reqframe_build(cmd, sub_type, sge_addr, sas_req);
	ps3_mgr_cmd_word_build(cmd);

	LOG_INFO("host_no:%u ready send t_id:0x%llx CFID:%u req type:%s\n",
		PS3_HOST(instance), cmd->trace_id, cmd->index,
		namePS3MgrCmdSubType((enum PS3MgrCmdSubType)sub_type));

	ret = ps3_mgr_cmd_sync_proc(instance, cmd, PS3_DEFAULT_MGR_CMD_TIMEOUT);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("host_no:%u %d respStatus NOK CFID:%d respStatus:%d\n",
			PS3_HOST(cmd->instance), ret,
			cmd->cmd_word.cmdFrameID,
			ps3_cmd_resp_status(cmd));
	}

	LOG_INFO("host_no:%u t_id:0x%llx CFID :%u type:%s end, ret:%d\n",
		PS3_HOST(instance), cmd->trace_id, cmd->index,
		namePS3MgrCmdSubType((enum PS3MgrCmdSubType)sub_type), ret);
l_out:
	if ((ret != -PS3_CMD_NO_RESP) && (cmd != NULL)) {
		ps3_mgr_cmd_free(instance, cmd);
	}

	return ret;
}

S32 ps3_sas_expander_all_get(struct ps3_instance *instance)
{
	LOG_DEBUG("host_no:%u\n", PS3_HOST(instance));

	return __ps3_sas_info_get(instance, PS3_SAS_GET_EXPANDERS,
		&instance->sas_dev_context.ps3_sas_buff_dma_addr,
		NULL);
}

S32 ps3_sas_phy_get(struct ps3_instance *instance, struct PS3SasMgr *sas_req)
{
	LOG_DEBUG("host_no:%u\n", PS3_HOST(instance));

	if (sas_req->phyCount == 0) {
		LOG_WARN("host_no:%u unexpect phyCount:%d !\n",
			PS3_HOST(instance), sas_req->phyCount);
		return -PS3_FAILED;
	}
	return __ps3_sas_info_get(instance, PS3_SAS_GET_PHY_INFO,
		&instance->sas_dev_context.ps3_sas_phy_buff_dma_addr,
		sas_req);
}

S32 ps3_sas_expander_get(struct ps3_instance *instance, struct PS3SasMgr *sas_req)
{
	LOG_DEBUG("host_no:%u\n", PS3_HOST(instance));

	return __ps3_sas_info_get(instance, PS3_SAS_GET_EXPANDER_INFO,
		&instance->sas_dev_context.ps3_sas_buff_dma_addr,
		sas_req);
}
#endif
static S32 ps3_mgr_cmd_sync_proc(struct ps3_instance *instance,
	struct ps3_cmd *cmd, U16 time_out)
{
	S32 ret = PS3_SUCCESS;
	S32 send_result = PS3_SUCCESS;
	enum PS3MgrCmdSubType sub_type = (enum PS3MgrCmdSubType)PS3_MGR_CMD_SUBTYPE(cmd);

	cmd->time_out = time_out;
	cmd->is_interrupt = PS3_DRV_FALSE;
	ps3_mgr_cmd_word_build(cmd);

	ps3_mgr_print_cmd(cmd,
		namePS3MgrCmdSubType(sub_type), PS3_DRV_TRUE);

	send_result = ps3_cmd_send_sync(instance, cmd);

	ps3_mgr_print_cmd(cmd,
		namePS3MgrCmdSubType(sub_type), PS3_DRV_FALSE);
	ret = ps3_mgr_complete_proc(instance, cmd, send_result);

	return ret;
}

static S32 ps3_no_resp_cmd_dead_proc(struct ps3_instance *instance,
	struct ps3_cmd *cmd)
{
	ULong flags = 0;
	S32 ret = PS3_SUCCESS;

	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	if (cmd->cmd_state.state == PS3_CMD_STATE_INIT ||
		cmd->cmd_state.state == PS3_CMD_STATE_COMPLETE) {
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
		LOG_WARN("host_no:%u CFID:%d finished\n",
			PS3_HOST(instance), cmd->index);
		ret = -PS3_FAILED;
		goto l_out;
	}

	cmd->cmd_state.state = PS3_CMD_STATE_DEAD;
	ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	LOG_INFO("host_no:%u entry reset!\n",
		PS3_HOST(instance));

	if (instance->is_probe_finish == PS3_FALSE ||
			!instance->state_machine.is_load) {
		ps3_need_wait_hard_reset_request(instance);
		ret = ps3_hard_recovery_request_with_retry(instance);
		if (ret == PS3_SUCCESS) {
			ps3_instance_wait_for_dead_or_pre_operational(instance);
		} else {
			LOG_WARN("host_no:%u hard recovery request NOK\n",
				PS3_HOST(instance));
		}

	} else {
		ps3_need_wait_hard_reset_request(instance);
		ps3_recovery_request_with_retry(instance);
	}

	LOG_ERROR("t_id:0x%llx CFID:%d host_no:%u state %u, recovery request\n",
		cmd->trace_id, cmd->index, PS3_HOST(instance),cmd->cmd_state.state);

	ret = -PS3_CMD_NO_RESP;
l_out:
	return ret;
}

static S32 ps3_no_resp_cmd_wait_proc(struct ps3_instance *instance,
	struct ps3_cmd *cmd)
{
	ULong flags = 0;
	S32 ret = PS3_SUCCESS;
	Bool is_probe_finish = PS3_FALSE;
	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	if (cmd->cmd_state.state == PS3_CMD_STATE_INIT ||
		cmd->cmd_state.state == PS3_CMD_STATE_COMPLETE) {
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
		LOG_WARN("host_no:%u CFID:%d finished, retries:%d\n",
			PS3_HOST(instance), cmd->index, cmd->retry_cnt);
		ret = -PS3_FAILED;
		goto l_out;
	} else {
		if (cmd->req_frame->mgrReq.reqHead.noReplyWord == PS3_CMD_WORD_NEED_REPLY_WORD) {
			init_completion(&cmd->sync_done);
		}
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	}
	LOG_INFO("host_no:%u entry soft reset!\n",
		PS3_HOST(instance));

	is_probe_finish = instance->is_probe_finish;
	ps3_need_wait_hard_reset_request(instance);
	if (is_probe_finish == PS3_FALSE) {
		ret = ps3_hard_recovery_request_with_retry(instance);
	} else {
		ret = ps3_recovery_request_with_retry(instance);
	}

	if (ret == -PS3_FAILED) {
		ret = -PS3_CMD_NO_RESP;
		LOG_ERROR("host_no:%u recovery request NOK\n",
			PS3_HOST(instance));
		goto l_dead;
	}
	INJECT_START(PS3_ERR_IJ_WAIT_HARD_RESET, instance)

	if (is_probe_finish  == PS3_FALSE) {
		ps3_instance_wait_for_dead_or_pre_operational(instance);
	}

	LOG_WARN("host_no:%u CFID:%d wait again, retries:%d\n",
		PS3_HOST(instance), cmd->index, cmd->retry_cnt);

	if (cmd->req_frame->mgrReq.reqHead.noReplyWord == PS3_CMD_WORD_NEED_REPLY_WORD) {
		ret = ps3_block_cmd_wait(instance, cmd, 0);
	} else {
		ret = ps3_cmd_reply_polling_when_recovery(instance, cmd, 0);
	}

	ret = ps3_err_mgr_cmd_proc(instance, ret, cmd);
	if (ret == -PS3_CMD_NO_RESP) {
	    LOG_WARN("host_no:%u CFID:%d wait again, retries:%d\n",
		    PS3_HOST(instance), cmd->index, cmd->retry_cnt);
	}
l_dead:
		ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
		if (cmd->cmd_state.state != PS3_CMD_STATE_INIT &&
			cmd->cmd_state.state != PS3_CMD_STATE_COMPLETE) {
			LOG_FILE_ERROR("host_no:%u CFID:%d is dead\n",
				PS3_HOST(instance), cmd->index);
			cmd->cmd_state.state = PS3_CMD_STATE_DEAD;
		}
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);

l_out:
	LOG_INFO("host_no:%u noresp cmd deal ret:%d\n",
		PS3_HOST(instance),ret);
	return ret;
}
static S32 ps3_no_resp_scsi_task_cmd_wait_proc(struct ps3_instance *instance,
	struct ps3_cmd *cmd)
{
	ULong flags = 0;
	S32 ret = PS3_SUCCESS;
	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	if (cmd->cmd_state.state == PS3_CMD_STATE_INIT ||
		cmd->cmd_state.state == PS3_CMD_STATE_COMPLETE) {
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
		LOG_WARN("host_no:%u CFID:%d finished, retries:%d\n",
			PS3_HOST(instance), cmd->index, cmd->retry_cnt);
		ret = -PS3_FAILED;
		goto l_out;
	} else {
		if (cmd->req_frame->frontendReq.reqHead.noReplyWord == PS3_CMD_WORD_NEED_REPLY_WORD) {
			init_completion(&cmd->sync_done);
		}
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	}

	ps3_need_wait_hard_reset_request(instance);
	ret = ps3_hard_recovery_request_with_retry(instance);
	if (ret == -PS3_FAILED) {
		ret = -PS3_CMD_NO_RESP;
		LOG_ERROR("host_no:%u hard recovery request NOK\n",
			PS3_HOST(instance));
		goto l_dead;
	}

	if (cmd->req_frame->frontendReq.reqHead.noReplyWord == PS3_CMD_WORD_NEED_REPLY_WORD) {
		ret = ps3_block_cmd_wait(instance, cmd, PS3_HARD_RESET_FORCE_STOP_MAX_TIME(instance));
	} else {
		ret = ps3_cmd_reply_polling_when_recovery(instance, cmd, PS3_HARD_RESET_FORCE_STOP_MAX_TIME(instance));
	}
	ret = ps3_err_mgr_cmd_proc(instance, ret, cmd);
    if (ret == -PS3_CMD_NO_RESP) {
		LOG_ERROR("host_no:%u hard recovery request,cmd timeout again\n",
			PS3_HOST(instance));
		goto l_dead;
	}else{
		goto l_out;
	}

l_dead:
		ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
		if (cmd->cmd_state.state != PS3_CMD_STATE_INIT &&
			cmd->cmd_state.state != PS3_CMD_STATE_COMPLETE) {
			LOG_FILE_ERROR("host_no:%u CFID:%d is dead\n",
				PS3_HOST(instance), cmd->index);
			cmd->cmd_state.state = PS3_CMD_STATE_DEAD;
		}
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);

l_out:
	LOG_INFO("host_no:%u noresp cmd deal ret:%d\n",
		PS3_HOST(instance),ret);
	return ret;
}

S32 ps3_mgr_cmd_no_resp_proc(struct ps3_instance *instance,
	struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	switch (PS3_MGR_CMD_TYPE(cmd))
	{
	case PS3_CMD_MANAGEMENT:
		ret = ps3_no_resp_cmd_dead_proc(instance, cmd);
		break;
	case PS3_CMD_SCSI_TASK_MANAGEMENT:
		ret = ps3_no_resp_scsi_task_cmd_wait_proc(instance, cmd);
		break;
	case PS3_CMD_IOCTL:
		ret = ps3_no_resp_cmd_wait_proc(instance, cmd);
		break;
	case PS3_CMD_SAS_MANAGEMENT:
		if ((PS3_MGR_CMD_SUBTYPE(cmd) == PS3_SAS_SMP_REQUEST) ||
			(PS3_MGR_CMD_SUBTYPE(cmd) == PS3_SAS_GET_LINK_ERR) ||
			(PS3_MGR_CMD_SUBTYPE(cmd) == PS3_SAS_PHY_CTRL) ) {
			ret = ps3_no_resp_cmd_wait_proc(instance, cmd);
		} else {
			ret = ps3_no_resp_cmd_dead_proc(instance, cmd);
		}
		break;
	default:
		LOG_ERROR("host_no:%u UNEXPEXT!!!! no response proc cmd_type:%d, sub_type:%d \n",
			PS3_HOST(instance), PS3_MGR_CMD_TYPE(cmd), PS3_MGR_CMD_SUBTYPE(cmd));
		ret = -PS3_FAILED;
		break;
	}

	return ret;
}

S32 ps3_mgr_complete_proc(struct ps3_instance *instance,
	struct ps3_cmd *cmd, S32 send_result)
{
	S32 ret = PS3_SUCCESS;
	ULong flags = 0;

	while(cmd->retry_cnt < PS3_ERR_MGR_CMD_FAULT_RETRY_MAX) {
		if (send_result == PS3_SUCCESS) {
			LOG_FILE_INFO("host_no:%u externel mgr CFID:%d respStatus:%d success\n",
				PS3_HOST(instance),
				cmd->index,
				ps3_cmd_resp_status(cmd));
			ret = PS3_SUCCESS;
			break;
		}

		if(send_result == -PS3_RECOVERED) {
			LOG_INFO("host_no:%u mgr CFID:%d respStatus:%d cannot send because recovery process\n",
				PS3_HOST(instance),
				cmd->index,
				ps3_cmd_resp_status(cmd));
			ret = -PS3_RECOVERED;
			break;
		}

		ret = ps3_err_mgr_cmd_proc(instance, send_result, cmd);
		if (ret == -PS3_CMD_NO_RESP) {
			ret = ps3_mgr_cmd_no_resp_proc(instance, cmd);
		}

		if (ret != -PS3_RETRY) {
			if ((PS3_MGR_CMD_TYPE(cmd) == PS3_CMD_MANAGEMENT) &&
				(PS3_MGR_CMD_SUBTYPE(cmd) == PS3_MGR_CMD_DEV_DEL_DONE)) {
				LOG_FILE_INFO("host_no:%u CFID:%d retry, retries:%d, ret:%d\n",
					PS3_HOST(instance), cmd->index, cmd->retry_cnt, ret);
			} else {
				LOG_INFO("host_no:%u cmd:%s CFID:%d retry, retries:%d, ret:%d\n",
					PS3_HOST(instance), namePS3MgrCmdSubType(
					(enum PS3MgrCmdSubType)PS3_MGR_CMD_SUBTYPE(cmd)),
					cmd->index, cmd->retry_cnt, ret);
			}
			break;
		}

		ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
		cmd->cmd_state.state = PS3_CMD_STATE_PROCESS;
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
		cmd->retry_cnt++;
		INJECT_START(PS3_ERR_IJ_IOCTL_CMD_RETRY_DONE, cmd)
		if (cmd->retry_cnt >= PS3_ERR_MGR_CMD_FAULT_RETRY_MAX) {
			break;
		}

		LOG_INFO("host_no:%u mgr CFID:%d retry:%u\n",
				PS3_HOST(instance), cmd->index, cmd->retry_cnt);
		cmd->resp_frame->normalRespFrame.respStatus = 0xff;
		init_completion(&cmd->sync_done);
		ps3_ioctl_buff_bit_pos_update(cmd);
		send_result = ps3_cmd_send_sync(instance, cmd);
	};

	if (cmd->retry_cnt >= PS3_ERR_MGR_CMD_FAULT_RETRY_MAX) {
		ret = -PS3_FAILED;
		LOG_ERROR("host_no:%u cmd request NOK.\n",
			PS3_HOST(instance));
	}

	return ret;
}

Bool ps3_check_ioc_state_is_normal_in_unload(
	struct ps3_instance *instance)
{
	Bool ret = PS3_TRUE;
	U32 ioc_state = PS3_FW_STATE_UNDEFINED;

	if (instance->state_machine.is_load) {
		goto l_out;
	}

	if (!ps3_ioc_state_get_with_check(instance, &ioc_state)) {
		ret = PS3_FALSE;
		goto l_out;
	}

	INJECT_START(PS3_ERR_IJ_IOC_IS_NOT_NORMAL_IN_UNLOAD, &ioc_state)
	if ((ioc_state != PS3_FW_STATE_RUNNING)) {
		LOG_ERROR("host_no:%d ioc_state:%d is not running.\n",
			PS3_HOST(instance), ioc_state);
		ret = PS3_FALSE;
		goto l_out;
	}
l_out:
	return ret;

}
