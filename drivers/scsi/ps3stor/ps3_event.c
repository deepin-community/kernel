#include "ps3_event.h"
#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/fs.h>
#else

#endif
#include "ps3_platform_utils.h"
#include "ps3_instance_manager.h"
#include "ps3_inner_data.h"
#include "ps3_htp_event.h"
#include "ps3_mgr_cmd.h"
#include "ps3_htp.h"
#include "ps3_device_update.h"
#include "ps3_util.h"
#include "ps3_cmd_statistics.h"
#include "ps3_err_inject.h"
U16 ps3_event_code_pd_count[] = {
	PS3_EVT_CODE(MGR_EVT_DEVM_DISK_IN),
	PS3_EVT_CODE(MGR_EVT_DEVM_DISK_OUT),
	PS3_EVT_CODE(MGR_EVT_MULITPILE_PD_IN),
	PS3_EVT_CODE(MGR_EVT_MULITPILE_PD_OUT),
	PS3_EVT_CODE(MGR_EVT_DEVM_JBOD),
	PS3_EVT_CODE(MGR_EVT_DEVM_READY),
	PS3_EVT_CODE(MGR_EVT_MULITPILE_JBOD),
	PS3_EVT_CODE(MGR_EVT_MULITPILE_READY),
	PS3_EVT_CODE(MGR_EVT_BACKPLANE_ON),
	PS3_EVT_CODE(MGR_EVT_BACKPLANE_OFF),
	PS3_EVT_CODE(MGR_EVT_DEVM_DISK_CHANGE),
	PS3_EVT_CODE(MGR_EVT_MULITPILE_PD_STATE_CHANGE),
	PS3_EVT_CODE(MGR_EVT_PD_PRE_READY)
};

U16 ps3_event_code_pd_attr[] = {
	PS3_EVT_CODE(MGR_EVT_PD_INFO_CHANGE),
	PS3_EVT_CODE(MGR_EVT_PD_MARKED_READY),
	PS3_EVT_CODE(MGR_EVT_PD_MARKED_ONLINE),
	PS3_EVT_CODE(MGR_EVT_PD_MARKED_FAILED),
	PS3_EVT_CODE(MGR_EVT_PD_MARKED_REBUILD),
	PS3_EVT_CODE(MGR_EVT_PD_MARKED_REPLACE),
	PS3_EVT_CODE(MGR_EVT_PD_R_GLOBAL_SPARE_ADDED),
	PS3_EVT_CODE(MGR_EVT_PD_R_GLOBAL_SPARE_DELETED),
	PS3_EVT_CODE(MGR_EVT_PD_R_DEDICATED_SPARE_ADDED),
	PS3_EVT_CODE(MGR_EVT_PD_R_DEDICATED_SPARE_DELETED),
	PS3_EVT_CODE(MGR_EVT_PD_MARKED_UNCONFIGUREDBAD),
	PS3_EVT_CODE(MGR_EVT_PD_MARKED_OFFLINE)
};

U16 ps3_event_code_vd_attr[] = {
	PS3_EVT_CODE(MGR_EVT_VD_SETTINGS_CHANGE),
	PS3_EVT_CODE(MGR_EVT_VD_PD_CHANGE),
	PS3_EVT_CODE(MGR_EVT_VD_STATE_CHANGE),
	PS3_EVT_CODE(MGR_EVT_VD_ALTER_ATTR_BY_MIGRATION),
	PS3_EVT_CODE(MGR_EVT_VD_EXPAND_FINISH)
};

struct ps3_event_type_desc_map {
	MgrEvtType_e event_type;		
	const char *event_type_desc;	
};

static struct ps3_event_type_desc_map g_event_type_desc_table[] = {
	{PS3_EVT_PD_COUNT, "PS3_EVT_PD_COUNT"},
	{PS3_EVT_VD_COUNT, "PS3_EVT_VD_COUNT"},
	{PS3_EVT_CTRL_INFO, "PS3_EVT_CTRL_INFO"},
	{PS3_EVT_PD_ATTR, "PS3_EVT_PD_ATTR"},
	{PS3_EVT_VD_ATTR, "PS3_EVT_VD_ATTRs"},
	{PS3_EVT_SAS_INFO, "PS3_EVT_SAS_INFO"},
	{PS3_EVT_DG_INFO, "MGR_EVT_DG_DELETED"},
};

static struct fasync_struct *g_async_queue;

static void ps3_event_print_cmd(struct ps3_cmd *cmd, Bool is_send)
{
	LOG_INFO("trace_id[0x%llx], hno:%u  event[%s] print cmd word type[%d],"
		"direct[%d], qmask[0x%x], CFID[%d], isr_sn[%d], vid[%d], pid[%d]\n",
		cmd->trace_id, PS3_HOST(cmd->instance), (is_send) ? "send" : "recv",
		cmd->cmd_word.type, cmd->cmd_word.direct,
		cmd->cmd_word.qMask, cmd->cmd_word.cmdFrameID,
		cmd->cmd_word.isrSN, cmd->cmd_word.virtDiskID,
		cmd->cmd_word.phyDiskID);
}

void ps3_vd_pending_filter_table_build(U8 *data)
{
	PS3EventFilter_s *event_filter = (PS3EventFilter_s*)data;
	memset(event_filter, 0, sizeof(PS3EventFilter_s));

	event_filter->eventType = PS3_EVT_VD_ATTR_LOCAL;
	event_filter->eventCodeCnt = ARRAY_SIZE(ps3_event_code_vd_attr);

	data += sizeof(PS3EventFilter_s);
	memcpy(data, ps3_event_code_vd_attr, sizeof(ps3_event_code_vd_attr));
}

void ps3_event_filter_table_get_raid(U8 *data)
{
	PS3EventFilter_s event_filter;
	memset(&event_filter, 0, sizeof(PS3EventFilter_s));

	event_filter.eventType = PS3_EVT_PD_COUNT_LOCAL;
	event_filter.eventCodeCnt = ARRAY_SIZE(ps3_event_code_pd_count);
	memcpy(data, &event_filter, sizeof(PS3EventFilter_s));
	data += sizeof(PS3EventFilter_s);
	memcpy(data, ps3_event_code_pd_count, sizeof(ps3_event_code_pd_count));
	data += sizeof(ps3_event_code_pd_count);

	event_filter.eventType = PS3_EVT_VD_COUNT_LOCAL;
	event_filter.eventCodeCnt = 0;
	memcpy(data, &event_filter, sizeof(PS3EventFilter_s));
	data += sizeof(PS3EventFilter_s);

	event_filter.eventType = PS3_EVT_CTRL_INFO_LOCAL;
	event_filter.eventCodeCnt = 0;
	memcpy(data, &event_filter, sizeof(PS3EventFilter_s));
	data += sizeof(PS3EventFilter_s);

	event_filter.eventType = PS3_EVT_PD_ATTR_LOCAL;
	event_filter.eventCodeCnt = ARRAY_SIZE(ps3_event_code_pd_attr);
	memcpy(data, &event_filter, sizeof(PS3EventFilter_s));
	data += sizeof(PS3EventFilter_s);
	memcpy(data, ps3_event_code_pd_attr, sizeof(ps3_event_code_pd_attr));
	data += sizeof(ps3_event_code_pd_attr);
}

void ps3_event_filter_table_get_hba(U8 *data)
{
	PS3EventFilter_s event_filter;
	memset(&event_filter, 0, sizeof(PS3EventFilter_s));

	event_filter.eventType = PS3_EVT_SAS_INFO_LOCAL;
	event_filter.eventCodeCnt = 0;
	memcpy(data, &event_filter, sizeof(PS3EventFilter_s));
	data += sizeof(PS3EventFilter_s);

	ps3_event_filter_table_get_raid(data);
}

void ps3_event_filter_table_get_switch(U8 *data)
{
	PS3EventFilter_s event_filter;
	memset(&event_filter, 0, sizeof(PS3EventFilter_s));

	event_filter.eventType = PS3_EVT_PD_COUNT_LOCAL;
	event_filter.eventCodeCnt = ARRAY_SIZE(ps3_event_code_pd_count);
	memcpy(data, &event_filter, sizeof(PS3EventFilter_s));
	data += sizeof(PS3EventFilter_s);
	memcpy(data, ps3_event_code_pd_count, sizeof(ps3_event_code_pd_count));
	data += sizeof(ps3_event_code_pd_count);
}

static S32 ps3_event_type_proc(struct ps3_instance *instance,
	MgrEvtType_e event_type)
{
	S32 ret = PS3_SUCCESS;

	switch (event_type) {
	case PS3_EVT_PD_COUNT:
	case PS3_EVT_VD_COUNT:
	case PS3_EVT_PD_ATTR:
	case PS3_EVT_VD_ATTR:
		ret = ps3_dev_update_full_proc(instance, event_type);
		break;
	case PS3_EVT_CTRL_INFO:
		ret = ps3_ctrl_info_get(instance);
		break;
	default:
		LOG_INFO("hno:%u Not cared event type %s\n",
			PS3_HOST(instance), ps3_event_print(event_type));
		break;
	}

	return ret;
}

#ifndef _WINDOWS
S32 ps3_event_delay_set(struct ps3_instance *instance, U32 delay)
{
	struct ps3_event_context *event_ctx = NULL;
	struct ps3_event_delay_work *delay_work = NULL;

	if (instance) {
		event_ctx = &instance->event_context;
		if (event_ctx->delay_work) {
			delay_work = event_ctx->delay_work;
			delay_work->event_delay = delay;
			return PS3_SUCCESS;
		}
	}

	return -PS3_FAILED;
}
#endif

static void ps3_event_resubscribe(struct ps3_instance *instance,
	struct ps3_cmd *cmd, U32 event_proc_result_bitmap)
{
	ULong flags = 0;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;
	struct PS3MgrEvent *event_req_info = NULL;

	if (!instance->state_machine.is_load) {
		LOG_WARN_IN_IRQ(instance,
			"hno:%u instance is suspend or instance unload!\n",
			PS3_HOST(instance));
		return;
	}

	mgr_req_frame = (struct PS3MgrReqFrame *)cmd->req_frame;
	event_req_info = (struct PS3MgrEvent *)&mgr_req_frame->value.event;

	LOG_INFO_IN_IRQ(instance,
		"trace_id[0x%llx], hno:%u old event proc result bitmap: [0x%x], "
		"current event proc result bitmap: [0x%x]\n",
		cmd->trace_id, PS3_HOST(instance),
		event_req_info->eventTypeMapProcResult,
		event_proc_result_bitmap);

	event_req_info->eventTypeMapProcResult = event_proc_result_bitmap;

	LOG_INFO_IN_IRQ(instance,
		"trace_id[0x%llx], hno:%u subcribe event proc result bitmap: [0x%x]\n",
		cmd->trace_id, PS3_HOST(instance),
		event_req_info->eventTypeMapProcResult);

	if (instance->ioc_adpter->event_filter_table_get != NULL) {
		instance->ioc_adpter->event_filter_table_get((U8*)cmd->ext_buf);
	}
	wmb();
	PS3_MGR_CMD_STAT_INC(instance, cmd);

	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	cmd->cmd_state.state = PS3_CMD_STATE_PROCESS;
	ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);

	if (ps3_async_cmd_send(cmd->instance, cmd) != PS3_SUCCESS) {
		LOG_INFO_IN_IRQ(instance,
			"trace_id[0x%llx],CFID[%d],hno:%u  re send event cmd faild\n",
			cmd->trace_id,cmd->index, PS3_HOST(cmd->instance));

		ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
		cmd->cmd_state.state = PS3_CMD_STATE_DEAD;
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	}
	return;
}

static U32 ps3_event_proc_without_details(struct ps3_instance *instance,
	struct PS3EventInfo *event_info)
{
	S32 ret = -PS3_FAILED;
	U32 event_proc_result_bitmap = 0;
	U32 mask_bit = 0X00000001;
	U32 event_type = event_info->eventTypeMap & mask_bit;
#ifndef _WINDOWS 
	if (event_info->eventTypeMap & PS3_EVT_SAS_INFO) {
		ret = ps3_sas_expander_event_refresh(instance);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u Event type PS3_EVT_SAS_INFO proc NOK!\n",
				PS3_HOST(instance));
			event_proc_result_bitmap |= PS3_EVT_SAS_INFO;
		}

		event_info->eventTypeMap ^= PS3_EVT_SAS_INFO;
	}
#endif
	while (mask_bit != 0) {
		if (event_type != 0) {
			LOG_INFO("hno:%u Event type %s report without details!\n",
				PS3_HOST(instance), ps3_event_print((MgrEvtType_e)event_type));

			ret = ps3_event_type_proc(instance,
				(MgrEvtType_e)event_type);

		} else {
			ret = PS3_SUCCESS;
		}

		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u Event type %s proc NOK!\n",
				PS3_HOST(instance),
				ps3_event_print((MgrEvtType_e)event_type));
			event_proc_result_bitmap |= event_type;
		}

		mask_bit = mask_bit << 1;
		event_type = event_info->eventTypeMap & mask_bit;
	}

	return event_proc_result_bitmap;
}

static void ps3_event_details_pre_proc(struct ps3_instance *instance,
	struct PS3EventInfo *event_info, struct ps3_cmd *cmd)
{
	U32 idx = 0;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;
	struct PS3MgrEvent *event_req_info = NULL;
	struct PS3EventDetail *event_detail = NULL;

	mgr_req_frame = (PS3MgrReqFrame_s *)cmd->req_frame;
	event_req_info = (struct PS3MgrEvent *)&mgr_req_frame->value.event;

	for(idx = 0;idx < event_info->eventCount;idx++) {
		event_detail = &event_info->eventDetail[idx];
		if (event_detail->eventType &
			event_req_info->eventTypeMapProcResult) {
			LOG_INFO("trace_id[0x%llx], hno:%u Event type %s code %s with "
				"timestamp[%d] dev id[0x%x] magic %#x do not process in details!\n",
				cmd->trace_id, PS3_HOST(instance),
				ps3_event_print(event_detail->eventType),
				mgrEvtCodeTrans(event_detail->eventCode),
				event_detail->timestamp,
				event_detail->devicePos.diskDev.diskID,
				event_detail->devicePos.diskMagicNum);

			event_detail->eventType = PS3_EVT_ILLEGAL_TYPE;
			continue;
		}

		if (event_detail->eventType ==
			PS3_EVT_CTRL_INFO) {
			event_detail->eventType = PS3_EVT_ILLEGAL_TYPE;
			continue;
		}

		event_info->eventTypeMap &= (~event_detail->eventType);
		LOG_INFO("trace_id[0x%llx], hno:%u Event type %s code %s reports "
			"in details with timestamp %d dev id 0x%x magic %#x!\n",
			cmd->trace_id, PS3_HOST(instance),
			ps3_event_print(event_detail->eventType),
			mgrEvtCodeTrans(event_detail->eventCode), event_detail->timestamp,
			event_detail->devicePos.diskDev.diskID,
			event_detail->devicePos.diskMagicNum);
	}

	event_info->eventTypeMap |= event_req_info->eventTypeMapProcResult;
}

static U32 ps3_event_proc_with_details(struct ps3_instance *instance,
	struct PS3EventInfo *event_info, struct ps3_cmd *cmd)
{
	U32 event_proc_result_bitmap = 0;

	ps3_event_details_pre_proc(instance, event_info, cmd);
#ifndef _WINDOWS 
	event_proc_result_bitmap |= ps3_sas_update_detail_proc(instance,
		event_info->eventDetail, event_info->eventCount);
#endif
	event_proc_result_bitmap = ps3_dev_update_detail_proc(instance,
		event_info->eventDetail, event_info->eventCount);

	return event_proc_result_bitmap;
}

static void ps3_event_delay(struct ps3_instance *instance,
	struct ps3_event_delay_work *delay_work)
{
	struct ps3_cmd *cmd = delay_work->cmd;
	U32 event_delay = 0;

	event_delay = delay_work->event_delay;
	LOG_WARN("trace_id[0x%llx], hno:%u  Event handle will delay %d seconds\n",
		cmd->trace_id, PS3_HOST(instance), event_delay);

	while (event_delay > 0) {
		if (kthread_should_stop() ||
			delay_work->event_delay == 0){
			LOG_WARN("trace_id[0x%llx], hno:%u  Event handle delay cancel\n",
				cmd->trace_id, PS3_HOST(instance));
			return;
		}

		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(HZ);
		event_delay--;
	}
	LOG_WARN("trace_id[0x%llx], hno:%u  Event handle delay %d seconds timeout\n",
		cmd->trace_id, PS3_HOST(instance), delay_work->event_delay);
}

static void ps3_event_handle(struct ps3_event_delay_work* ps3_delay_work)
{
	struct ps3_cmd *cmd = ps3_delay_work->cmd;
	struct ps3_instance *instance = cmd->instance;
	struct PS3EventInfo* event_info = instance->event_context.event_info;
	U32 event_proc_result_bitmap = PS3_EVT_ILLEGAL_TYPE;
	struct ps3_event_delay_work* ps3_new_delay_work = NULL;

	ULong flags = 0;
    S32 ret;

	INJECT_START(PS3_ERR_IJ_EVENT_CMD_NULL, instance)
	if (instance->event_context.event_cmd == NULL) {
		LOG_WARN("trace_id[0x%llx], hno:%u Event is unsubscribed\n",
			cmd->trace_id, PS3_HOST(instance));
		return;
	}

	if (!instance->state_machine.is_load) {
		LOG_WARN("trace_id[0x%llx], hno:%u instance is suspend or instance unload!\n",
			cmd->trace_id, PS3_HOST(instance));
		return;
	}

	LOG_INFO("trace_id[0x%llx], hno:%u event count[%d], bitmap[%08x]\n",
		cmd->trace_id, PS3_HOST(instance), event_info->eventCount,
		event_info->eventTypeMap);

	INJECT_START(PS3_ERR_IJ_EVENT_PROC_WAIT_HARD_FLAG, instance)
	INJECT_START(PS3_ERR_IJ_EVENT_PROC_WAIT_HARD_SUBCRIBE, instance)

#ifndef _WINDOWS
	if (ps3_delay_work->event_delay) {
		ps3_event_delay(instance, ps3_delay_work);
	}
#endif
	INJECT_START(PS3_ERR_IJ_WAIT_NORMAL, instance)
	if (event_info->eventCount <= PS3_EVENT_DETAIL_BUF_MAX) {
		LOG_INFO("trace_id[0x%llx], hno:%u Event has valid details info!\n",
			cmd->trace_id, PS3_HOST(instance));
		event_proc_result_bitmap |=
			ps3_event_proc_with_details(instance, event_info, cmd);
	}

	if (event_info->eventTypeMap) {
		event_proc_result_bitmap |=
			ps3_event_proc_without_details(instance, event_info);
	}
	PS3_IJ_SLEEP(5000, PS3_ERR_IJ_EVENT_DELAY);

	memset(event_info, 0, sizeof(*event_info));
	ps3_event_print_cmd(cmd, PS3_TRUE);
	INJECT_START(PS3_ERR_IJ_EVENT_SUBC_WAIT_HARD_FLAG, instance)
	INJECT_START(PS3_ERR_IJ_EVENT_SUBC_WAIT_HARD_SUBCRIBE, instance)
	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags);
	if (instance->event_context.abort_eventcmd == 0 &&
		instance->hardreset_event == 0) {
		ps3_new_delay_work =
			(ps3_delay_work == &instance->event_context.delay_work_pool[PS3_EVENT_WORKER_POOL_SIZE - 1])?
			(&instance->event_context.delay_work_pool[0]):(ps3_delay_work + 1);
		instance->event_context.delay_work = ps3_new_delay_work;
		ps3_event_resubscribe(instance, cmd, event_proc_result_bitmap);

		if (instance->event_context.subwork != 0) {
			instance->event_context.subwork -= 1;
		}
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags);
	} else {
		LOG_INFO_IN_IRQ(instance,
			"trace_id[0x%llx], hno:%u Event proc free cmd:%d, abort_eventcmd %d!\n",
			cmd->trace_id, PS3_HOST(instance), cmd->index,
			instance->event_context.abort_eventcmd);
				instance->event_context.event_cmd = NULL;
				ps3_mgr_cmd_free(instance, cmd);
		ps3_new_delay_work =
			(ps3_delay_work == &instance->event_context.delay_work_pool[PS3_EVENT_WORKER_POOL_SIZE - 1])?
			(&instance->event_context.delay_work_pool[0]):(ps3_delay_work + 1);
		instance->event_context.delay_work = ps3_new_delay_work;
		instance->event_req_info.eventTypeMapProcResult = instance->event_req_info.eventTypeMap;
		ret = ps3_event_subscribe(instance);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR_IN_IRQ(instance, "hno:%u  event subscribe failed!\n",
				PS3_HOST(instance));
		}

		if(instance->hardreset_event != 0) {
			instance->hardreset_event = 0;
		}

		if(instance->event_context.abort_eventcmd != 0) {
			instance->event_context.abort_eventcmd = 0;
		}
		wmb();
		if (instance->event_context.subwork != 0) {
			instance->event_context.subwork -= 1;
		}
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags);
	}

	return;
}
#ifdef _WINDOWS
static void ps3_event_polling(void *ins)
{
	struct ps3_instance *instance = (struct ps3_instance*)ins;
	ps3_event_handle(&instance->event_context.delay_work);
}
#else
static void ps3_event_polling(struct work_struct *work)
{
	struct ps3_event_delay_work *delay_work = ps3_container_of(work,
		struct ps3_event_delay_work, event_work.work);

	ps3_event_handle(delay_work);
}
#endif
const char *ps3_event_print(MgrEvtType_e event_type)
{
	U32 idx = 0;
	const char *ps3_event_type_name = NULL;

	for (idx = 0;idx < ARRAY_SIZE(g_event_type_desc_table);idx++) {
		if (g_event_type_desc_table[idx].event_type == event_type) {
			ps3_event_type_name
				= g_event_type_desc_table[idx].event_type_desc;
			break;
		}
	}

	return ps3_event_type_name;
}

S32 ps3_event_context_init(struct ps3_instance *instance)
{
	S32 ret = -PS3_FAILED;
	U32 idx = 0;
	struct ps3_event_context *event_ctx = &instance->event_context;
	struct ps3_event_delay_work *delay_work = NULL;
	struct ps3_event_delay_work *delay_work_pool = NULL;
	memset(event_ctx, 0, sizeof(*event_ctx));
#ifndef _WINDOWS
	delay_work_pool = (struct ps3_event_delay_work *)ps3_kzalloc(instance,
		sizeof(struct ps3_event_delay_work) * PS3_EVENT_WORKER_POOL_SIZE);
	if (delay_work_pool == NULL) {
		LOG_ERROR("hno:%u delay work kzalloc failed!\n",
			PS3_HOST(instance));
		goto l_out;
	}
	event_ctx->delay_work_pool = delay_work_pool;
	for(idx = 0; idx < PS3_EVENT_WORKER_POOL_SIZE; idx++){
        delay_work = &delay_work_pool[idx];
    	INIT_DELAYED_WORK(&delay_work->event_work, ps3_event_polling);
	}
	event_ctx->delay_work = &delay_work_pool[0];
	event_ctx->event_cmd = NULL;
	event_ctx->event_abort_cmd = NULL;
	event_ctx->abort_eventcmd = 0;
	event_ctx->flag = PS3_FALSE;
#else
	delay_work = &event_ctx->delay_work;
	if (ps3_worker_init(instance, &delay_work->event_work, "ps3_event_wk", ps3_event_polling) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u worker init failed\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

#endif
	event_ctx->subwork = 0;

	ret = PS3_SUCCESS;
l_out:
	return ret;
}

void ps3_event_context_exit(struct ps3_instance *instance)
{
	struct ps3_event_context *event_ctx = &instance->event_context;
#ifdef _WINDOWS
	struct ps3_event_delay_work* delay_work = NULL;

	delay_work = &event_ctx->delay_work;
	ps3_worker_exit(&delay_work->event_work);
#else
	if (event_ctx->delay_work_pool != NULL) {
		ps3_kfree(instance, event_ctx->delay_work_pool);
		event_ctx->delay_work_pool = NULL;
        event_ctx->delay_work = NULL;
	}
#endif
	memset(event_ctx, 0, sizeof(*event_ctx));
}

S32 ps3_event_subscribe(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_event_context *event_ctx = &instance->event_context;
	struct PS3MgrEvent *event_req_info = &instance->event_req_info;

	if (!instance->is_need_event) {
		LOG_WARN_IN_IRQ(instance, "hno:%u IOC no need event!!\n",
			PS3_HOST(instance));
		ret = PS3_SUCCESS;
		goto l_out;
	}

	if (event_ctx->event_cmd != NULL) {
		LOG_ERROR_IN_IRQ(instance, "hno:%u Event already subscribe!\n",
			PS3_HOST(instance));
		ret = PS3_SUCCESS;
		goto l_out;
	}

	if (!instance->state_machine.is_load) {
		LOG_WARN_IN_IRQ(instance, "hno:%u instance is suspend or instance unload!\n",
			PS3_HOST(instance));
		ret = PS3_SUCCESS;
		goto l_out;
	}

	ret = ps3_event_register(instance, event_req_info);
	if (ret != PS3_SUCCESS) {
		LOG_WARN_IN_IRQ(instance, "hno:%u Failed to register event cmd, ret: %d\n",
			PS3_HOST(instance), ret);
		goto l_out;
	}

	LOG_INFO_IN_IRQ(instance, "hno:%u Success to subscribe event, bitmap[%08x]!\n",
		PS3_HOST(instance), event_req_info->eventTypeMap);
l_out:
	return ret;
}

S32 ps3_event_unsubscribe(struct ps3_instance *instance)
{
	ULong flags = 0;
	ULong flags1 = 0;
	S32 ret = -PS3_FAILED;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *abort_cmd = NULL;
	struct ps3_event_context *event_ctx = &instance->event_context;

	if (!ps3_check_ioc_state_is_normal_in_unload(instance)) {
		ret = -PS3_FAILED;
		goto l_out;
	}

	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
	if(event_ctx->flag){
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
		goto l_out;
	}
	event_ctx->flag = PS3_TRUE;
	cmd = event_ctx->event_cmd;
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
	if (cmd == NULL) {
		LOG_WARN("hno:%u  Event is not register yet\n",
			PS3_HOST(instance));
		ret = PS3_SUCCESS;
		goto l_out;
	}

	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	INJECT_START(PS3_ERR_IJ_SUBC_EVENT_CMD_INIT, instance)
	if (cmd->cmd_state.state == PS3_CMD_STATE_INIT) {
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
		LOG_ERROR("hno:%u  had been free,CFID:%d\n",
			PS3_HOST(instance), cmd->index);
		ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
		event_ctx->event_cmd = NULL;
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
		ret = PS3_SUCCESS;
		goto l_out;
	} else {
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	}

	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
	ret = ps3_mgr_cmd_cancel_send(instance, cmd->index, PS3_CANCEL_EVENT_CMD);
	INJECT_START(PS3_ERR_IJ_CANCEL_EVENT_CMD_FAIL, &ret)
	if(ret == -PS3_ENOMEM){
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
		LOG_INFO("hno:%u  alloc failed\n",PS3_HOST(instance));
		ret = PS3_FAILED;
		goto l_out;
	}else if(ret != PS3_SUCCESS){
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
		LOG_INFO("hno:%u reqFrameId=%d cancel_cmd_frame_id[%u] free!\n",
			PS3_HOST(instance), ps3_cmd_frame_id(instance->event_context.event_abort_cmd), cmd->index);
		abort_cmd = instance->event_context.event_abort_cmd;
		instance->event_context.event_abort_cmd = NULL;
		if (abort_cmd != NULL) {
			ps3_task_cmd_free(instance, abort_cmd);
		}
		ret = PS3_FAILED;
		goto l_out;
	}
	instance->event_context.abort_eventcmd = 1;
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);

	ret = ps3_mgr_cmd_cancel_wait(instance, PS3_CANCEL_EVENT_CMD);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  event cancel cmd NOK, ret:%d\n",
			PS3_HOST(instance), ret);
		goto l_out;
	}

	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
	if(instance->event_context.subwork == 0){
		if(instance->event_context.abort_eventcmd != 0){
			instance->event_context.abort_eventcmd = 0;
			LOG_FILE_INFO("hno:%u  event cmd free, CFID:%d\n",
				PS3_HOST(instance), cmd->index);
			event_ctx->event_cmd = NULL;
			ps3_mgr_cmd_free(instance, cmd);
		}
	}
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
l_out:
	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
	event_ctx->flag = PS3_FALSE;
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
	return ret;
}
S32 ps3_soft_reset_event_resubscribe(struct ps3_instance *instance)
{
	ULong flags = 0;
	ULong flags1 = 0;
	S32 ret = -PS3_FAILED;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *abort_cmd = NULL;
	struct ps3_event_context *event_ctx = &instance->event_context;
	Bool is_need_resend = PS3_FALSE;

	if (!ps3_check_ioc_state_is_normal_in_unload(instance)) {
		ret = -PS3_FAILED;
		goto l_out;
	}

	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
	if(event_ctx->flag){
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
		goto l_out;
	}
	event_ctx->flag = PS3_TRUE;
	cmd = event_ctx->event_cmd;
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
	if (cmd == NULL) {
		LOG_WARN("hno:%u  Event is not register yet\n",
			PS3_HOST(instance));
		ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
		is_need_resend = PS3_TRUE;
		goto l_resend;
	}

	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	INJECT_START(PS3_ERR_IJ_RESUBC_EVENT_CMD_INIT, instance)
	if (cmd->cmd_state.state == PS3_CMD_STATE_INIT) {
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
		LOG_ERROR("hno:%u  had been free,CFID:%d\n",
			PS3_HOST(instance), cmd->index);
		ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
		event_ctx->event_cmd = NULL;
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
		ret = PS3_SUCCESS;
		goto l_out;
	} else {
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	}

	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
	ret = ps3_mgr_cmd_cancel_send(instance, cmd->index, PS3_CANCEL_EVENT_CMD);
	if (ret == -PS3_ENOMEM){
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
		LOG_INFO("hno:%u  alloc failed\n",PS3_HOST(instance));
		ret = PS3_FAILED;
		goto l_out;
	} else if (ret != PS3_SUCCESS){
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
		LOG_INFO("hno:%u reqFrameId=%d cancel_cmd_frame_id[%u] free!\n",
			PS3_HOST(instance), ps3_cmd_frame_id(instance->event_context.event_abort_cmd), cmd->index);
		abort_cmd = instance->event_context.event_abort_cmd;
		instance->event_context.event_abort_cmd = NULL;
		if (abort_cmd != NULL) {
			ps3_task_cmd_free(instance, abort_cmd);
		}
		ret = PS3_FAILED;
		goto l_out;
	}
	instance->event_context.abort_eventcmd = 1;
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);

	ret = ps3_mgr_cmd_cancel_wait(instance, PS3_CANCEL_EVENT_CMD);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  event cancel cmd failed, ret:%d\n",
			PS3_HOST(instance), ret);
		goto l_out;
	}
	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
	if(instance->event_context.subwork == 0){
		if(instance->event_context.abort_eventcmd != 0){
			instance->event_context.abort_eventcmd = 0;
			LOG_FILE_INFO("hno:%u  event cmd free, CFID:%d\n",
				PS3_HOST(instance), cmd->index);
			event_ctx->event_cmd = NULL;
			ps3_mgr_cmd_free(instance, cmd);

			is_need_resend = PS3_TRUE;
		}
	}

l_resend:
	if (is_need_resend) {
		instance->event_req_info.eventTypeMapProcResult =
			instance->event_req_info.eventTypeMap;
		ret = ps3_event_subscribe(instance);
		if ((ret != PS3_SUCCESS) && (ret != -PS3_IN_UNLOAD)) {
			LOG_FILE_ERROR("hno:%u  event subscribe failed!\n",
				PS3_HOST(instance));
			ret = PS3_FAILED;
		}
	}
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);

l_out:
	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
	event_ctx->flag = PS3_FALSE;
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
	return ret;
}

S32 ps3_event_service(struct ps3_cmd *cmd, U16 reply_flags)
{
	ULong flags = 0;
	ULong flags1 = 0;
	S32 ret = PS3_SUCCESS;
	struct ps3_instance *instance = cmd->instance;
	struct ps3_event_context *event_ctx = &instance->event_context;
#ifdef _WINDOWS
	struct ps3_event_delay_work *delay_work = &event_ctx->delay_work;
#else
	struct ps3_event_delay_work *delay_work = event_ctx->delay_work;
#endif
	S32 cur_state = PS3_INSTANCE_STATE_INIT;

	PS3_MGR_CMD_BACK_INC(instance, cmd, reply_flags);
	ps3_event_print_cmd(cmd, PS3_FALSE);

	if (reply_flags != PS3_REPLY_WORD_FLAG_SUCCESS) {
		LOG_ERROR_IN_IRQ(instance,
			"trace_id[0x%llx], hno:%u Event notify fetal error!\n",
			cmd->trace_id, PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_failed;
	}
	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
	if (event_ctx->event_cmd == NULL) {
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
		LOG_INFO_IN_IRQ(instance,
		"trace_id[0x%llx], hno:%u Event is unsubscribed!\n",
			cmd->trace_id, PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);

	if (!instance->state_machine.is_load) {
		LOG_INFO_IN_IRQ(instance,
			"trace_id[0x%llx], hno:%u instance is suspend or instance unload!\n",
			cmd->trace_id, PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_failed;
	}

	cur_state = ps3_atomic_read(&instance->state_machine.state);
	if (cur_state != PS3_INSTANCE_STATE_OPERATIONAL)  {
		LOG_INFO_IN_IRQ(instance,
			"trace_id[0x%llx], hno:%u instance is not operational!\n",
			cmd->trace_id, PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_failed;
	}

	delay_work->cmd = cmd;
#ifdef _WINDOWS
	if (ps3_worker_start(&delay_work->event_work) != PS3_SUCCESS) {
		LOG_ERROR_IN_IRQ(instance,
			"trace_id[0x%llx], hno:%u Event start worker failed!\n",
			cmd->trace_id, PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_failed;
	}
	instance->event_context.subwork += 1;
#else
	INJECT_START(PS3_ERR_IJ_EVENT_INT_WAIT_HARD_FLAG, instance)
	INJECT_START(PS3_ERR_IJ_EVENT_INT_WAIT_HARD_SUBCRIBE, instance)
	instance->event_context.subwork += 1;
	schedule_delayed_work(&delay_work->event_work, 0);
#endif
	goto l_out;

l_failed:
	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	cmd->cmd_state.state = PS3_CMD_STATE_DEAD;
	ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	LOG_INFO_IN_IRQ(instance,
		"trace_id[0x%llx],CFID[%d], hno:%u failed!\n",
		cmd->trace_id, cmd->index, PS3_HOST(instance));
l_out:
	LOG_INFO_IN_IRQ(instance,
		"instance->event_context.subwork[%u]!\n",
		instance->event_context.subwork);
	return ret;
}

S32 ps3_fasync(int fd, struct file *filp, int mode)
{
    return fasync_helper(fd, filp, mode, &g_async_queue);
}

S32 ps3_webSubscribe_context_init(struct ps3_instance *instance)
{
    S32 ret = -PS3_FAILED;
    struct ps3_webSubscribe_context *web_context = &instance->webSubscribe_context;
    memset(web_context, 0, sizeof(*web_context));
    web_context->webSubscribe_cmd = NULL;
    web_context->web_abort_cmd = NULL;
    ps3_atomic_set(&web_context->subscribe_count, PS3_WEB_FLAG_INIT_VALUE);
    ps3_atomic_set(&web_context->is_subscribe, PS3_WEB_FLAG_INIT_VALUE);
    ps3_atomic_set(&web_context->is_abort, PS3_WEB_FLAG_INIT_VALUE);
    web_context->abort_webcmd = PS3_WEB_FLAG_INIT_VALUE;
    ret = PS3_SUCCESS;
    return ret;
}

void ps3_webSubscribe_context_exit(struct ps3_instance *instance)
{
    struct ps3_webSubscribe_context *web_context = &instance->webSubscribe_context;
    memset(web_context, 0, sizeof(*web_context));
}

static void ps3_web_resubscribe(struct ps3_instance *instance, struct ps3_cmd *cmd)
{
    ULong flags = 0;
    PS3_MGR_CMD_STAT_INC(instance, cmd);

    ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
    cmd->cmd_state.state = PS3_CMD_STATE_PROCESS;
    ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);

    if (ps3_async_cmd_send(cmd->instance, cmd) != PS3_SUCCESS) {
        LOG_ERROR("trace_id[0x%llx],CFID[%d],hno:%u  resend web cmd NOK\n",
            cmd->trace_id,cmd->index, PS3_HOST(cmd->instance));

        ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
        cmd->cmd_state.state = PS3_CMD_STATE_DEAD;
        ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
    }
    return;
}

S32 ps3_webSubscribe_service(struct ps3_cmd *cmd, U16 reply_flags)
{
    ULong flags = 0;
    S32 ret = PS3_SUCCESS;
    struct ps3_instance *instance = cmd->instance;
    struct ps3_webSubscribe_context *web_context = &instance->webSubscribe_context;

    S32 cur_state = PS3_INSTANCE_STATE_INIT;

    PS3_MGR_CMD_BACK_INC(instance, cmd, reply_flags);
    ps3_event_print_cmd(cmd, PS3_FALSE);

    if (reply_flags == PS3_REPLY_WORD_FLAG_FAIL) {
        LOG_ERROR_IN_IRQ(instance,
		"trace_id[0x%llx], hno:%u Web notify fetal error!\n",
            cmd->trace_id, PS3_HOST(instance));
        ret = -PS3_FAILED;
        goto l_failed;
    }

    if (web_context->webSubscribe_cmd == NULL) {
        LOG_INFO_IN_IRQ(instance,
		"trace_id[0x%llx], hno:%u Web is unsubscribed!\n",
            cmd->trace_id, PS3_HOST(instance));
        ret = -PS3_FAILED;
        goto l_out;
    }

    if (!instance->state_machine.is_load) {
        LOG_INFO_IN_IRQ(instance,
		"trace_id[0x%llx], hno:%u instance is suspend or instance unload!\n",
            cmd->trace_id, PS3_HOST(instance));
        ret = -PS3_FAILED;
        goto l_failed;
    }

    cur_state = ps3_atomic_read(&instance->state_machine.state);
    if (cur_state != PS3_INSTANCE_STATE_OPERATIONAL) {
        LOG_INFO_IN_IRQ(instance,
		"trace_id[0x%llx], hno:%u instance is not operational!\n",
            cmd->trace_id, PS3_HOST(instance));
        ret = -PS3_FAILED;
        goto l_failed;
    }

    kill_fasync(&g_async_queue, PS3_EVENT_NOTICE_SIG, POLL_IN);

    ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags);
    if (instance->webSubscribe_context.abort_webcmd == 0) {
        ps3_web_resubscribe(instance, cmd);
        ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags);
    } else {
        LOG_INFO_IN_IRQ(instance,
		"trace_id[0x%llx], hno:%u Web proc free cmd:%d, abort_webcmd %d!\n",
            cmd->trace_id, PS3_HOST(instance), cmd->index,
                instance->webSubscribe_context.abort_webcmd);

        instance->webSubscribe_context.webSubscribe_cmd = NULL;
        ps3_mgr_cmd_free(instance, cmd);
        ret = ps3_web_subscribe(instance);
        if (ret != PS3_SUCCESS) {
            LOG_ERROR_IN_IRQ(instance,
	    	"hno:%u  web subscribe failed!\n",
                PS3_HOST(instance));
        }
        instance->webSubscribe_context.abort_webcmd = 0;
        ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags);
    }
    goto l_out;

l_failed:
    ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
    cmd->cmd_state.state = PS3_CMD_STATE_DEAD;
    ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
    LOG_INFO_IN_IRQ(instance,
    	"trace_id[0x%llx],CFID[%d], hno:%u failed!\n",
        cmd->trace_id, cmd->index, PS3_HOST(instance));
l_out:
    return ret;
}

S32 ps3_web_subscribe(struct ps3_instance *instance)
{
    S32 ret = PS3_SUCCESS;
	if (!instance->is_need_event) {
		LOG_WARN_IN_IRQ(instance, "hno:%u ioc no need web server!!\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}
    ret = ps3_web_register(instance);
    if (ret != PS3_SUCCESS) {
		LOG_WARN_IN_IRQ(instance, "hno:%u Failed to register web cmd, ret: %d\n",
			PS3_HOST(instance), ret);
        goto l_out;
    }

	LOG_INFO_IN_IRQ(instance, "hno:%u Success to subscribe web event\n",
		PS3_HOST(instance));
l_out:
    return ret;
}

S32 ps3_web_unsubscribe(struct ps3_instance *instance)
{
    ULong flags = 0;
    S32 ret = -PS3_FAILED;
    struct ps3_cmd *cmd = NULL;
    struct ps3_cmd *abort_cmd = NULL;
    struct ps3_webSubscribe_context *web_context = &instance->webSubscribe_context;

    if (!ps3_check_ioc_state_is_normal_in_unload(instance)) {
        ret = -PS3_FAILED;
        goto l_out;
    }
    cmd = web_context->webSubscribe_cmd;
    if (cmd == NULL) {
        LOG_WARN("hno:%u  web event is not register yet\n",
            PS3_HOST(instance));
        ret = PS3_SUCCESS;
        goto l_out;
    }

    ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
    if (cmd->cmd_state.state == PS3_CMD_STATE_INIT) {
        ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
        LOG_ERROR("hno:%u  had been free,CFID:%d\n",
            PS3_HOST(instance), cmd->index);
        web_context->webSubscribe_cmd = NULL;
        ret = PS3_SUCCESS;
        goto l_out;
    } else {
        ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
    }

    if (ps3_atomic_add_unless(&instance->webSubscribe_context.is_abort, 1, 1) == 0) {
        ret = PS3_SUCCESS;
        goto l_out;
    }

    ret = ps3_mgr_cmd_cancel_send(instance, cmd->index, PS3_CANCEL_WEB_CMD);
	INJECT_START(PS3_ERR_IJ_CANCEL_WEB_CMD_ALLOC_NULL, &ret)
    if(ret == -PS3_ENOMEM){
        LOG_INFO("hno:%u  alloc failed\n",PS3_HOST(instance));
        ret = -PS3_FAILED;
        ps3_atomic_set(&instance->webSubscribe_context.is_abort, 0);
        goto l_out;
    } else if(ret != PS3_SUCCESS) {
        LOG_INFO("hno:%u reqFrameId=%d cancel_cmd_frame_id[%u] free!\n",
            PS3_HOST(instance), ps3_cmd_frame_id(instance->webSubscribe_context.web_abort_cmd), cmd->index);
        abort_cmd = instance->webSubscribe_context.web_abort_cmd;
        instance->webSubscribe_context.web_abort_cmd = NULL;
        if (abort_cmd != NULL) {
            ps3_task_cmd_free(instance, abort_cmd);
        }
        ret = -PS3_FAILED;
        ps3_atomic_set(&instance->webSubscribe_context.is_abort, 0);
        goto l_out;
    }
    ret = ps3_mgr_cmd_cancel_wait(instance, PS3_CANCEL_WEB_CMD);
    if (ret != PS3_SUCCESS) {
        LOG_ERROR("hno:%u  event cancel cmd NOK, ret:%d\n",
            PS3_HOST(instance), ret);
        ps3_atomic_set(&instance->webSubscribe_context.is_abort, 0);
        goto l_out;
    }
    LOG_INFO("hno:%u  web cmd free, CFID:%d\n",
        PS3_HOST(instance), cmd->index);
    web_context->webSubscribe_cmd = NULL;
    ps3_mgr_cmd_free(instance, cmd);
    ps3_atomic_set(&instance->webSubscribe_context.is_abort, 0);
l_out:
    return ret;
}

S32 ps3_soft_reset_web_resubscribe(struct ps3_instance *instance)
{
    ULong flags = 0;
    ULong flags1 = 0;
    S32 ret = -PS3_FAILED;
    struct ps3_cmd *cmd = NULL;
    struct ps3_cmd *abort_cmd = NULL;
    struct ps3_webSubscribe_context *web_context = &instance->webSubscribe_context;
    Bool is_need_resend = PS3_FALSE;

    if (!ps3_check_ioc_state_is_normal_in_unload(instance)) {
        ret = -PS3_FAILED;
        goto l_out;
    }

    cmd = web_context->webSubscribe_cmd;
    if (cmd == NULL) {
        LOG_WARN("hno:%u  Web is not register yet\n",
            PS3_HOST(instance));
        is_need_resend = PS3_TRUE;
        ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
        goto l_resend;
    }

    ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
    if (cmd->cmd_state.state == PS3_CMD_STATE_INIT) {
        ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
        LOG_ERROR("hno:%u  had been free,CFID:%d\n",
            PS3_HOST(instance), cmd->index);
        web_context->webSubscribe_cmd = NULL;
        ret = PS3_SUCCESS;
        goto l_out;
    } else {
        ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
    }

    if (ps3_atomic_add_unless(&instance->webSubscribe_context.is_abort, 1, 1) == 0) {
        ret = PS3_SUCCESS;
        goto l_out;
    }

    ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
    ret = ps3_mgr_cmd_cancel_send(instance, cmd->index, PS3_CANCEL_WEB_CMD);
    if (ret == -PS3_ENOMEM){
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
	LOG_INFO("hno:%u  alloc failed\n",PS3_HOST(instance));
        ret = -PS3_FAILED;
        ps3_atomic_set(&instance->webSubscribe_context.is_abort, 0);
        goto l_out;
    } else if (ret != PS3_SUCCESS) {
    	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
        LOG_INFO("hno:%u reqFrameId=%d cancel_cmd_frame_id[%u] free!\n",
            PS3_HOST(instance), ps3_cmd_frame_id(instance->webSubscribe_context.web_abort_cmd), cmd->index);
        abort_cmd = instance->webSubscribe_context.web_abort_cmd;
        instance->webSubscribe_context.web_abort_cmd = NULL;
        if (abort_cmd != NULL) {
            ps3_task_cmd_free(instance, abort_cmd);
        }
        ret = -PS3_FAILED;
        ps3_atomic_set(&instance->webSubscribe_context.is_abort, 0);
        goto l_out;
    }
    instance->webSubscribe_context.abort_webcmd = 1;
    ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);

    ret = ps3_mgr_cmd_cancel_wait(instance, PS3_CANCEL_WEB_CMD);
    if (ret != PS3_SUCCESS) {
        LOG_ERROR("hno:%u  web cancel cmd failed, ret:%d\n",
            PS3_HOST(instance), ret);
        ps3_atomic_set(&instance->webSubscribe_context.is_abort, 0);
        goto l_out;
    }
    ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
    if (instance->webSubscribe_context.abort_webcmd != 0){
        instance->webSubscribe_context.abort_webcmd = 0;
        LOG_FILE_INFO("hno:%u  web cmd free, CFID:%d\n",
            PS3_HOST(instance), cmd->index);
        web_context->webSubscribe_cmd = NULL;
        ps3_mgr_cmd_free(instance, cmd);
        is_need_resend = PS3_TRUE;
    }
    ps3_atomic_set(&instance->webSubscribe_context.is_abort, 0);

l_resend:
    if (is_need_resend) {
        ret = ps3_web_subscribe(instance);
        if ((ret != PS3_SUCCESS) && (ret != -PS3_IN_UNLOAD)) {
            LOG_FILE_ERROR("hno:%u  web subscribe failed!\n",
                PS3_HOST(instance));
            ret = -PS3_FAILED;
       }
    }
    ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
l_out:
    return ret;
}

void ps3_web_cmd_clear(struct ps3_instance *instance)
{
    ULong flags = 0;
    struct ps3_cmd *cmd = NULL;
    instance->webSubscribe_context.abort_webcmd = 0;

    cmd = instance->webSubscribe_context.webSubscribe_cmd;

    if (cmd == NULL) {
        LOG_WARN("hno:%u  web cmd has been cancel\n",
            PS3_HOST(instance));
        goto l_out;
    }

    ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
    if (cmd->cmd_state.state == PS3_CMD_STATE_INIT) {
        ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
        LOG_ERROR("hno:%u   free this cmd,CFID:%d\n",
            PS3_HOST(instance), cmd->index);
        instance->webSubscribe_context.webSubscribe_cmd = NULL;
        goto l_out;
    } else {
        ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
    }

    LOG_INFO("hno:%u  web cmd free, CFID:%d\n",
        PS3_HOST(instance), cmd->index);
    instance->webSubscribe_context.webSubscribe_cmd = NULL;
    ps3_mgr_cmd_free(instance, cmd);

l_out:
    return;
}

