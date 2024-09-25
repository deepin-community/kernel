
#include <linux/random.h>

#include "ps3_ioc_manager.h"
#include "ps3_err_inject.h"
#include "ps3_cmd_channel.h"
#include "ps3_mgr_cmd.h"
#include "ps3_module_para.h"
#include "ps3_scsih_cmd_parse.h"
#include "ps3_ioc_state.h"
#include "ps3_scsih.h"
#include "ps3_sas_transport.h"

#include <linux/types.h>
#include "ps3_inject.h"
#include "ps3_cmd_statistics.h"
#include <linux/kthread.h>

#ifdef PS3_SUPPORT_INJECT

struct ps3_rec_work_context g_work_que_recovery;
static Ps3Injection_t g_ps3_err_scene[PS3_ERR_SCENE_NUM];
static Bool is_inject_init = PS3_FALSE;
static U64 sgl_addr;

void ps3_err_inject_sleep(U32 ms, U16 err_type)
{
	if ((err_type < PS3_ERR_SCENE_NUM &&
		err_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) &&
		g_ps3_err_scene[err_type - 1].active) {
		ps3_msleep(ms);
	}
}

void ps3_err_inject_sleep_rand(U32 min, U32 max, U16 err_type)
{
	U16 slepp_ms = min;
	if ((err_type < PS3_ERR_SCENE_NUM &&
		err_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) &&
		g_ps3_err_scene[err_type - 1].active) {
		slepp_ms = min + prandom_u32_max(max - min);
		ps3_msleep(slepp_ms);
	}
}

void ps3_err_inject_err_type_valid(U16 err_type)
{
	if (err_type < PS3_ERR_SCENE_NUM &&
		err_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) {
		g_ps3_err_scene[err_type - 1].active = PS3_TRUE;
	}
}

void ps3_err_inject_err_type_clean(U16 err_type)
{
	if (err_type < PS3_ERR_SCENE_NUM &&
		err_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) {
		g_ps3_err_scene[err_type - 1].active = PS3_FALSE;
	}
}

Bool ps3_err_inject_err_type_get(U16 err_type)
{
	if (err_type < PS3_ERR_SCENE_NUM &&
		err_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) {
		return g_ps3_err_scene[err_type - 1].type;
	}

	return PS3_FALSE;
}

void ps3_err_inject_wait_pre(U16 err_type_wait)
{
	while(g_ps3_err_scene[err_type_wait-1].active &&
		!g_ps3_err_scene[err_type_wait-1].is_hit_pre) {
		msleep(5);
	}
}

void ps3_err_inject_wait_post(U16 err_type_wait)
{
	while(g_ps3_err_scene[err_type_wait-1].active &&
		!g_ps3_err_scene[err_type_wait-1].is_hit_pre) {
		msleep(5);
	}
}

void inject_register(U16 err_type, injectCallback callback)
{
	if (err_type < PS3_ERR_SCENE_NUM &&
		err_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) {
		g_ps3_err_scene[err_type - 1].callback = callback;
	}

    return ;
}

void inject_active_intf(U16 err_type, U16 count)
{
	if (err_type < PS3_ERR_SCENE_NUM &&
		err_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) {
		g_ps3_err_scene[err_type - 1].active = PS3_TRUE;
		g_ps3_err_scene[err_type - 1].count = count;
		g_ps3_err_scene[err_type - 1].is_hit_pre = PS3_FALSE;
		g_ps3_err_scene[err_type - 1].is_hit_post = PS3_FALSE;
	}

    return ;
}
void inject_execute_callback(U16 err_type, void * data)
{
	S32 ret = PS3_SUCCESS;

	if (err_type < PS3_ERR_SCENE_NUM &&
		err_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) {
		if (g_ps3_err_scene[err_type-1].active == PS3_TRUE &&
			g_ps3_err_scene[err_type-1].count != 0) {
			g_ps3_err_scene[err_type-1].is_hit_pre = PS3_TRUE;
			if (g_ps3_err_scene[err_type-1].callback != NULL) {
				ret = g_ps3_err_scene[err_type-1].callback(data);
			}
			g_ps3_err_scene[err_type-1].is_hit_post = PS3_TRUE;
			if(g_ps3_err_scene[err_type-1].count > 0 && ret == PS3_SUCCESS){
				g_ps3_err_scene[err_type-1].count--;
			}
		}
	}

	return ;
}

void inject_execute_callback_at_time(U16 err_type, void * data)
{
	S32 ret = PS3_SUCCESS;

	if (err_type < PS3_ERR_SCENE_NUM &&
		err_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) {

		if (g_ps3_err_scene[err_type-1].active == PS3_TRUE) {
			if (g_ps3_err_scene[err_type-1].count > 1) {
				g_ps3_err_scene[err_type-1].count--;
			} else if (g_ps3_err_scene[err_type-1].count == 1) {
				g_ps3_err_scene[err_type-1].is_hit_pre = PS3_TRUE;
				if (g_ps3_err_scene[err_type-1].callback != NULL) {
					ret = g_ps3_err_scene[err_type-1].callback(data);
				}
				g_ps3_err_scene[err_type-1].is_hit_post = PS3_TRUE;
				if(g_ps3_err_scene[err_type-1].count > 0 && ret == PS3_SUCCESS){
					g_ps3_err_scene[err_type-1].count--;
				}
			}
		}
	}

	return;
}

void inject_execute_callback_relevance_wait_pre(U16 err_type, U16 err_type_wait, void * data)
{
	S32 ret = PS3_SUCCESS;
	if (err_type < PS3_ERR_SCENE_NUM &&
		err_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) {
		if (g_ps3_err_scene[err_type-1].active == PS3_TRUE &&
			g_ps3_err_scene[err_type_wait-1].active == PS3_TRUE &&
			g_ps3_err_scene[err_type-1].count != 0) {
			g_ps3_err_scene[err_type-1].is_hit_pre = PS3_TRUE;
			while(g_ps3_err_scene[err_type_wait-1].active &&
				!g_ps3_err_scene[err_type_wait-1].is_hit_pre) {
				msleep(5);
			}
			if (g_ps3_err_scene[err_type-1].callback != NULL) {
				ret = g_ps3_err_scene[err_type-1].callback(data);
			}
			g_ps3_err_scene[err_type-1].is_hit_post = PS3_TRUE;
			if(g_ps3_err_scene[err_type-1].count > 0 && ret == PS3_SUCCESS){
				g_ps3_err_scene[err_type-1].count--;
			}
		}
	}

	return ;
}

void inject_execute_callback_relevance_wait_post(U16 err_type, U16 err_type_wait, void * data)
{
	S32 ret = PS3_SUCCESS;
	if (err_type < PS3_ERR_SCENE_NUM &&
		err_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) {
		if (g_ps3_err_scene[err_type-1].active == PS3_TRUE &&
			g_ps3_err_scene[err_type_wait-1].active == PS3_TRUE &&
			g_ps3_err_scene[err_type-1].count != 0) {
			g_ps3_err_scene[err_type-1].is_hit_pre = PS3_TRUE;
			while(g_ps3_err_scene[err_type_wait-1].active &&
				!g_ps3_err_scene[err_type_wait-1].is_hit_post) {
				msleep(5);
			}
			if (g_ps3_err_scene[err_type-1].callback != NULL) {
				ret = g_ps3_err_scene[err_type-1].callback(data);
			}
			g_ps3_err_scene[err_type-1].is_hit_post = PS3_TRUE;
			if(g_ps3_err_scene[err_type-1].count > 0 && ret == PS3_SUCCESS){
				g_ps3_err_scene[err_type-1].count--;
			}
		}
	}

	return ;
}

void inject_execute_callback_at_time_relevance_wait_pre(U16 err_type,
	U16 err_type_wait, void * data)
{
	S32 ret = PS3_SUCCESS;
	if (err_type < PS3_ERR_SCENE_NUM &&
		err_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) {

		if (g_ps3_err_scene[err_type-1].active == PS3_TRUE) {
			if (g_ps3_err_scene[err_type-1].count > 1) {
				g_ps3_err_scene[err_type-1].count--;
			} else if (g_ps3_err_scene[err_type-1].count == 1) {
				g_ps3_err_scene[err_type-1].is_hit_pre = PS3_TRUE;

				while(g_ps3_err_scene[err_type_wait-1].active &&
					!g_ps3_err_scene[err_type_wait-1].is_hit_pre) {
					msleep(5);
				}

				if (g_ps3_err_scene[err_type-1].callback != NULL) {
				ret = g_ps3_err_scene[err_type-1].callback(data);
			}
				g_ps3_err_scene[err_type-1].is_hit_post = PS3_TRUE;
				if(g_ps3_err_scene[err_type-1].count > 0 && ret == PS3_SUCCESS){
					g_ps3_err_scene[err_type-1].count--;
				}
			}
		}
	}

	return;
}

void inject_execute_callback_at_time_relevance_wait_post(U16 err_type,
	U16 err_type_wait, void * data)
{
	S32 ret = PS3_SUCCESS;
	if (err_type < PS3_ERR_SCENE_NUM &&
		err_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) {

		if (g_ps3_err_scene[err_type-1].active == PS3_TRUE) {
			if (g_ps3_err_scene[err_type-1].count > 1) {
				g_ps3_err_scene[err_type-1].count--;
			} else if (g_ps3_err_scene[err_type-1].count == 1) {
				g_ps3_err_scene[err_type-1].is_hit_pre = PS3_TRUE;

				while(g_ps3_err_scene[err_type_wait-1].active &&
					!g_ps3_err_scene[err_type_wait-1].is_hit_post) {
					msleep(5);
				}

				if (g_ps3_err_scene[err_type-1].callback != NULL) {
				ret = g_ps3_err_scene[err_type-1].callback(data);
			}
				g_ps3_err_scene[err_type-1].is_hit_post = PS3_TRUE;
				if(g_ps3_err_scene[err_type-1].count > 0 && ret == PS3_SUCCESS){
					g_ps3_err_scene[err_type-1].count--;
				}
			}
		}
	}

	return;
}

S32 force_ioctl_cmd_dead(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;

	if (cmd == NULL) {
		goto end;
	}
	if (PS3_MGR_CMD_TYPE(cmd) == PS3_CMD_IOCTL) {
		cmd->cmd_state.state = PS3_CMD_STATE_DEAD;
	}
end:
	return PS3_SUCCESS;
}
S32 force_ioctl_cmd_pending(void * ptr)
{
	*(S32 *)ptr = 1;
	return PS3_SUCCESS;
}

S32 force_ioctl_cmd_sig_interrupt(void * ptr)
{
	*(S32 *)ptr = 1;
	return PS3_SUCCESS;
}

S32 force_ioctl_cmd_interrupt(void * ptr)
{
	*(S32 *)ptr = -PS3_FAILED;
	return PS3_SUCCESS;
}
S32 force_ioctl_cmd_no_resp(void * ptr)
{
	*(S32 *)ptr = -PS3_TIMEOUT;
	return PS3_SUCCESS;
}
S32 force_return_fail(void * ptr)
{
	*(S32 *)ptr = -PS3_FAILED;
	return PS3_SUCCESS;
}
S32 force_ioctl_cmd_abort_no_resp(void * ptr)
{
	*(S32 *)ptr = -PS3_CMD_NO_RESP;
	return PS3_SUCCESS;
}

S32 force_ioctl_cmd_force_done(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	msleep(500);
	complete(&cmd->sync_done);
	return PS3_SUCCESS;
}

S32 ioctl_cmd_retry_done(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	cmd->retry_cnt = 3;
	return PS3_SUCCESS;
}

S32 force_ioctl_cmd_recovery_cancel(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	S32 ret;
	ret = ps3_hard_recovery_request(cmd->instance);
	if (ret == -PS3_FAILED) {
		LOG_ERROR("hno:%u  hard recovery request failed\n",
			PS3_HOST(cmd->instance));
		return -PS3_FAILED;
	}
	while(ps3_atomic_read(&cmd->instance->state_machine.state) != PS3_INSTANCE_STATE_RECOVERY){
		msleep(50);
	}

	return PS3_SUCCESS;
}

S32 wait_instance_normal(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;

	while(ps3_atomic_read(&instance->state_machine.state) != PS3_INSTANCE_STATE_OPERATIONAL &&
		ps3_atomic_read(&instance->state_machine.state) != PS3_INSTANCE_STATE_PRE_OPERATIONAL){
		msleep(5);
	}
	return PS3_SUCCESS;
}

S32 wait_instance_operational(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;

	while(ps3_atomic_read(&instance->state_machine.state) != PS3_INSTANCE_STATE_OPERATIONAL){
		msleep(5);
	}
	return PS3_SUCCESS;
}
S32 host_reset_wait_decide_normal(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	LOG_INFO("inject host_reset_wait_decide_normal enter,recovery_state:%d\n",
		instance->recovery_context->recovery_state);

	while(instance->recovery_context->host_reset_state == PS3_HOST_RESET_INIT){
		msleep(5);
	}

	msleep(50);

	LOG_INFO("inject host_reset_wait_decide_normal end,host_reset_state:%d\n",
		instance->recovery_context->host_reset_state);
	return PS3_SUCCESS;
}

S32 force_instance_unload(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->state_machine.is_load = PS3_FALSE;
    return PS3_SUCCESS;
}

S32 force_instance_state_unnormal(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_OPERATIONAL,
		PS3_INSTANCE_STATE_RECOVERY);
    return PS3_SUCCESS;
}

S32 force_instance_state_pci_err(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->state_machine.is_pci_err_recovery = PS3_TRUE;
    return PS3_SUCCESS;
}

S32 wait_instance_unnormal(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;

	while(ps3_atomic_read(&instance->state_machine.state) == PS3_INSTANCE_STATE_OPERATIONAL){
		msleep(5);
	}
	return PS3_SUCCESS;
}

S32 wait_ioc_ready(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U32 ioc_state = 0;
	if (instance->recovery_context->heartbeat_recovery != PS3_HEARTBEAT_HARDRESET_RECOVERY) {
		goto l_out;
	}

	LOG_INFO("wait_ioc_ready inject\n");
	ioc_state = instance->ioc_adpter->ioc_state_get(instance);
	while(ioc_state != PS3_FW_STATE_READY){
		ioc_state = instance->ioc_adpter->ioc_state_get(instance);
		msleep(5);
	}

l_out:
	return PS3_SUCCESS;
}

static void force_work_queue_unull_test(struct work_struct* work)
{
	LOG_INFO("force_work_queue_unull_test inject\n");

	(void) work;
	return;
}

S32 force_work_queue_unull(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	struct work_struct* recovery_irq_work = &instance->recovery_irq_work;
	S8 request_irq_queue_name[PS3_RECOVERY_IRQ_NAME_MAX_LENTH];
	LOG_INFO("force_work_queue_unull inject\n");

	memset(request_irq_queue_name, 0, PS3_RECOVERY_IRQ_NAME_MAX_LENTH);
	INIT_WORK(recovery_irq_work, force_work_queue_unull_test);

	snprintf(request_irq_queue_name, PS3_RECOVERY_IRQ_NAME_MAX_LENTH,
		"ps3_irq_recovery_start_service_host%d", instance->host->host_no);

	instance->recovery_irq_queue =
		create_singlethread_workqueue(request_irq_queue_name);

	return PS3_SUCCESS;
}

S32 force_work_queue_failed(void * ptr)
{
	S32 *ret = (S32 *)ptr;
	LOG_INFO("force_work_queue_failed\n");
	*ret = -PS3_FAILED;

	return PS3_SUCCESS;
}

S32 force_fw_state_ff(void * ptr)
{
	U64 *heartbeat_value = (U64 *)ptr;
	LOG_INFO("force_fw_state_ff\n");
	*heartbeat_value = U64_MAX;

	return PS3_SUCCESS;
}

S32 wait_remove_is_load_flag(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	LOG_INFO("wait_remove_is_load_flag\n");
	while(instance->state_machine.is_load){
		msleep(5);
	}
	return PS3_SUCCESS;
}

S32 wait_reco_dump_valid(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;

	while(instance->dump_context.is_hard_recovered == 0){
		msleep(5);

	}
	return PS3_SUCCESS;
}

S32 force_ret_fail(void * ptr)
{
	S32 *ret = (S32 *)ptr;
	*ret = -PS3_FAILED;
	return PS3_SUCCESS;
}

S32 force_reg_fail(void * ptr)
{
	U64 *ret = (U64 *)ptr;
	*ret = U64_MAX;
	return PS3_SUCCESS;
}

S32 force_ready_failed(void * ptr)
{
	U32 *fw_cur_state = (U32 *)ptr;
	*fw_cur_state = PS3_FW_STATE_START;
	return PS3_SUCCESS;
}

S32 force_ioc_critical(void * ptr)
{
	U32 *fw_cur_state = (U32 *)ptr;
	*fw_cur_state = PS3_FW_STATE_CRITICAL;
	return PS3_SUCCESS;
}

S32 force_change_sgl_addr_to_page_mode_5(void * ptr)
{
	U64 *addr = (U64 *)ptr;
	sgl_addr = *addr;
	*addr = 0xfffffc5600000;
	return PS3_SUCCESS;
}

S32 force_sgl_addr_restore(void * ptr)
{
	U64 *addr = (U64 *)ptr;
	*addr = sgl_addr;
	return PS3_SUCCESS;
}

S32 force_atu_support_failed(void * ptr)
{
	U8 *bit_pos = (U8 *)ptr;
	*bit_pos = 0xe0;
	return PS3_SUCCESS;
}

S32 force_reg_zero(void * ptr)
{
	U64 *reg = (U64 *)ptr;
	*reg = 0x0;
	return PS3_SUCCESS;
}

S32 force_scan_host_delay(void * ptr)
{
	U32 times = *(U32 *)ptr;
	LOG_INFO("inject ps3_scsi_scan_host dalay start:%u\n", times);
	while(times) {
		msleep(100);
		times -= 100;
	}
	LOG_INFO("inject ps3_scsi_scan_host dalay end:%u\n", times);
	return PS3_SUCCESS;
}
S32 force_scan_host_fail(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->is_scan_host_finish = PS3_FALSE;
	return PS3_SUCCESS;
}

S32 force_ignore_task_io(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	U8 type;
	S32 ret = PS3_FAILED;

	if (cmd == NULL) {
		goto end;
	}
	type = PS3_SCSI_CMD_TYPE(ps3_scsih_cdb_rw_type_get(cmd->scmd->cmnd));
	if (ps3_scsih_is_rw_type(type)) {
		cmd->cmd_state.state = PS3_CMD_STATE_PROCESS;
		init_completion(&cmd->sync_done);

		LOG_INFO("inject force_ignore_task_io CFID %u no resp\n", cmd->index);
		ret = PS3_SUCCESS;
	}
end:
	return ret;
}

S32 force_ignore_task_abort(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	S32 ret = PS3_FAILED;

	if (cmd == NULL) {
		goto end;
	}
	if (PS3_MGR_CMD_TYPE(cmd) == PS3_CMD_SCSI_TASK_MANAGEMENT &&
		PS3_MGR_CMD_SUBTYPE(cmd) == PS3_TASK_CMD_SCSI_TASK_ABORT) {
		cmd->cmd_state.state = PS3_CMD_STATE_PROCESS;
		init_completion(&cmd->sync_done);
		LOG_INFO("inject force_ignore_task_abort CFID %u no resp\n", cmd->index);
		ret = PS3_SUCCESS;
	}
end:
	return ret;
}
S32 force_ignore_task_reset(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	S32 ret = PS3_FAILED;

	if (cmd == NULL) {
		goto end;
	}
	if (PS3_MGR_CMD_TYPE(cmd) == PS3_CMD_SCSI_TASK_MANAGEMENT &&
		PS3_MGR_CMD_SUBTYPE(cmd) == PS3_TASK_CMD_SCSI_TASK_RESET) {
		cmd->cmd_state.state = PS3_CMD_STATE_PROCESS;
		init_completion(&cmd->sync_done);
		LOG_INFO("inject force_ignore_task_reset CFID %u no resp\n", cmd->index);
		ret = PS3_SUCCESS;
	}
end:
	return ret;
}
S32 force_event_handle(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance*)ptr;
	schedule_delayed_work(&instance->event_context.delay_work->event_work, 0);
	return PS3_SUCCESS;
}
S32 force_del_disk(void * ptr)
{
	struct scsi_device *sdev = (struct scsi_device *)ptr;
	if (sdev) {
		sdev = NULL;
	}
	return PS3_SUCCESS;
}

S32 force_priv_data_null(void * ptr)
{
	struct scsi_device *sdev = (struct scsi_device *)ptr;
	struct ps3_instance *instance = (struct ps3_instance*)(sdev->host->hostdata);

	if(sdev->hostdata != NULL) {
		ps3_kfree(instance, sdev->hostdata);
	}
	sdev->hostdata = NULL;
	return PS3_SUCCESS;
}
S32 force_wait_hard_flag(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U32 wait_time = 180*HZ;
	while (instance->hardreset_event != 1) {
		ps3_msleep(1);
		wait_time--;
		if (wait_time == 0) {
			break;
		}
	}
	LOG_INFO("event wait hard flag %d\n",
		instance->hardreset_event);
	return PS3_SUCCESS;
}
S32 force_int_wait_hard_flag(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U32 wait_time = 10*HZ;
	while (instance->hardreset_event != 0) {
		ps3_mdelay(1);
		wait_time--;
		if (wait_time == 0) {
			break;
		}
	}
	LOG_INFO("int wait hard flag %d\n",
		instance->hardreset_event);
	return PS3_SUCCESS;
}

S32 force_int_wait_hard_subcribe(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U32 wait_time = 10*HZ;

	while (instance->recovery_context->recovery_state != PS3_HARD_RECOVERY_SHALLOW) {
		ps3_mdelay(1);
		wait_time--;
		if (wait_time == 0) {
			break;
		}
	}
	LOG_INFO("int wait hard recovery finish %d\n",
		instance->recovery_context->recovery_state);
	return PS3_SUCCESS;
}

S32 force_wait_hard_subcribe(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U32 wait_time = 180*HZ;
	while (instance->recovery_context->recovery_state != PS3_HARD_RECOVERY_SHALLOW) {
		ps3_msleep(1);
		wait_time--;
		if (wait_time == 0) {
			break;
		}
	}
	while (instance->recovery_context->recovery_state != PS3_HARD_RECOVERY_FINISH) {
		ps3_msleep(1);
		wait_time--;
		if (wait_time == 0) {
			break;
		}
	}
	LOG_INFO("event wait hard recovery finish %d\n",
		instance->recovery_context->recovery_state);
	return PS3_SUCCESS;
}

S32 force_wait_event_int(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U32 wait_time = 180*HZ;
	while (instance->event_context.subwork != 1) {
		ps3_msleep(1);
		wait_time--;
		if (wait_time == 0) {
			break;
		}
	}
	LOG_INFO("recovery wait event int %d\n",
		instance->event_context.subwork);
	return PS3_SUCCESS;
}

S32 force_wait_event_proc(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U32 wait_time = 180*HZ;
	while (instance->event_context.subwork != 1) {
		ps3_msleep(1);
		wait_time--;
		if (wait_time == 0) {
			break;
		}
	}

	ps3_msleep(100*HZ);
	LOG_INFO("recovery wait event proc %d\n",
		instance->event_context.subwork);
	return PS3_SUCCESS;
}

S32 force_wait_event_subscribe(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U32 wait_time = 180*HZ;
	while (instance->event_context.subwork != 1) {
		ps3_msleep(1);
		wait_time--;
		if (wait_time == 0) {
			break;
		}
	}
	while (instance->event_context.subwork != 0) {
		ps3_msleep(1);
		wait_time--;
		if (wait_time == 0) {
			break;
		}
	}
	LOG_INFO("recovery wait event subc %d\n",
		instance->event_context.subwork);
	return PS3_SUCCESS;
}

S32 force_hard_ready_failed(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;

	instance->reg_set->reg_f.Excl_reg.ps3SocFwState.reg.ps3SocFwState = PS3_FW_STATE_FAULT;
	LOG_INFO("force_hard_ready_failed success \n");

	return PS3_SUCCESS;
}

S32 wait_recovery_func0_running_1(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->reserve[0] = 0xcc;

	while (instance->reserve[0] == 0xcc) {
		msleep(10);
	}
	LOG_INFO("wait_recovery_func0_running_1 success \n");

	return PS3_SUCCESS;
}
S32 wait_recovery_func1_probe(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	LOG_INFO("wait_recovery_func1_probe in \n");
	while (instance->peer_instance == NULL) {
		msleep(10);
	}
	LOG_INFO("wait_recovery_func1_probe success \n");

	return PS3_SUCCESS;
}
S32 wait_recovery_req_func1_probe(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->reserve[0] = 0xa1;

	while (instance->reserve[0] == 0xa1) {
		msleep(10);
	}
	LOG_INFO("wait_recovery_func1_probe success \n");

	return PS3_SUCCESS;
}

S32 wait_recovery_func1_remove(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->reserve[0] = 0xb0;

	while (instance->reserve[0] == 0xb0) {
		msleep(10);
	}
	LOG_INFO("wait_recovery_func1_remove success \n");

	return PS3_SUCCESS;
}
S32 wait_recovery_req_func1_remove(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->reserve[0] = 0xb1;

	while (instance->reserve[0] == 0xb1) {
		msleep(10);
	}
	LOG_INFO("wait_recovery_func1_remove success \n");

	return PS3_SUCCESS;
}

S32 wait_recovery_func0_running_2(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->reserve[0] = 0;
	if(instance->peer_instance) {
		instance->peer_instance->reserve[0] = 0;
	}
	LOG_INFO("wait_recovery_func0_running_2 \n");

	return PS3_SUCCESS;
}

S32 force_trigger_log_fail(void * ptr)
{
	U8 * ret = (U8 *)ptr;
	*ret = PS3_FALSE;

	return PS3_SUCCESS;
}
S32 force_event_cmd_null(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->event_context.event_cmd = NULL;

	return PS3_SUCCESS;
}

S32 force_event_cmd_init(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	struct ps3_cmd *cmd = instance->event_context.event_cmd;
	cmd->cmd_state.state = PS3_CMD_STATE_INIT;

	return PS3_SUCCESS;
}
S32 force_cmd_done(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	U8 reply_flags = PS3_SUCCESS;

	ps3_scsih_io_done(cmd, reply_flags);

	return PS3_SUCCESS;
}
S32 force_aborted_cmd_done(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	U16 aborted_cmd_frame_id = 0;
	U8 reply_flags = PS3_SUCCESS;
	PS3MgrTaskReqFrame_s *mgr_task_req_frame = NULL;
	struct ps3_instance *instance = cmd->instance;

	mgr_task_req_frame = &cmd->req_frame->taskReq;
	aborted_cmd_frame_id = mgr_task_req_frame->taskID;
	cmd = ps3_cmd_find(instance, aborted_cmd_frame_id);

	ps3_scsih_io_done(cmd, reply_flags);

	return PS3_SUCCESS;
}
S32 force_stop_aborted_cmd_done(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	ps3_hard_recovery_request(instance);

	ps3_recovery_cancel_work_sync(instance);

	return PS3_SUCCESS;
}

#ifdef PS3_UT
S32 force_scsi_cmd_done(void * ptr)
{
	struct scsi_cmnd *scmd = (struct scsi_cmnd *)ptr;
	U8 reply_flags = PS3_SUCCESS;
	U16 cmd_frame_id = 0;
	struct ps3_cmd *aborted_cmd;
	struct ps3_instance *instance = scsi_host_data(scmd);

	cmd_frame_id = SCMD_GET_REQUEST(scmd)->tag;
	aborted_cmd = ps3_cmd_find(instance, cmd_frame_id);

	ps3_scsih_io_done(aborted_cmd, reply_flags);

	return PS3_SUCCESS;
}

S32 force_set_ioc_in_security(void *ptr)
{
    struct ps3_instance *instance = (struct ps3_instance *)ptr;
    writeq_fake(0x1, &instance->reg_set->reg_f.Excl_reg.ps3Debug7);

    return PS3_SUCCESS;
}
S32 force_set_ioc_running(void *ptr)
{
    struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U64 state;

	if (instance->reg_set != NULL) {
		instance->reg_set->reg_f.Excl_reg.ps3SocFwState.reg.ps3SocFwState =
			PS3_FW_STATE_RUNNING;
	}
	state = instance->ioc_adpter->ioc_state_get(instance);

	LOG_WARN("ioc state [%llu]\n", state);
    return PS3_SUCCESS;
}

#else
S32 force_set_ioc_in_security(void *ptr)
{
    struct ps3_instance *instance = (struct ps3_instance *)ptr;
	PS3_IOC_REG_WRITE(instance, reg_f.Excl_reg, ps3Debug7, 0x1);

    return PS3_SUCCESS;
}
S32 force_set_ioc_running(void *ptr)
{
    struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U64 state;

	PS3_IOC_REG_READ_WITH_CHECK(instance, reg_f.Excl_reg, ps3SocFwState, state);
	state |= PS3_FW_STATE_RUNNING;
	PS3_IOC_REG_WRITE(instance, reg_f.Excl_reg, ps3SocFwState, state);

    return PS3_SUCCESS;
}
#endif
S32 force_set_ioc_fault(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U64 state;

	if (instance->reg_set != NULL) {
		instance->reg_set->reg_f.Excl_reg.ps3SocFwState.reg.ps3SocFwState =
			PS3_FW_STATE_FAULT;
	}
	state = instance->ioc_adpter->ioc_state_get(instance);

	LOG_WARN("ioc state [%llu]\n", state);
	return PS3_SUCCESS;
}

S32 force_set_f0_ioc_critical(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U64 state;

	if (instance->reg_set != NULL && instance->peer_instance != NULL && 
			ps3_get_pci_function(instance->peer_instance->pdev) == PS3_FUNC_ID_0) {
		instance->peer_instance->reg_set->reg_f.Excl_reg.ps3SocFwState.reg.ps3SocFwState =
			PS3_FW_STATE_CRITICAL;
	}
	state = instance->peer_instance->ioc_adpter->ioc_state_get(instance);
	while (ps3_atomic_read(&instance->recovery_context->hardreset_ref) == 0) {
		msleep(5);
	}
	LOG_WARN("ioc state [%llu]\n", state);
	return PS3_SUCCESS;
}

S32 set_qos_share_cnt(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	ps3_atomic_set(&instance->qos_context.tg_ctx.share_free_cnt, 1);

	return PS3_SUCCESS;
}
S32 force_event_cmd_dead(void *ptr)
{
	S32 *cur_state = (S32 *)ptr;
	*cur_state = PS3_INSTANCE_STATE_RECOVERY;

	return PS3_SUCCESS;
}

S32 wait_irq_disable(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	struct ps3_instance *instance = cmd->instance;

	instance->reserved[0] = 0;
	while(ps3_irq_is_enable(&instance->irq_context)) {
		msleep(1);
	}

	return PS3_SUCCESS;
}

S32 wait_ioctl_detect_irq_disable(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	while(instance->reserved[0] != 0xab) {
		msleep(1);
	}

	return PS3_SUCCESS;
}

S32 set_send_ioctl_block_flag(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	struct ps3_instance *instance = cmd->instance;

	if (cmd->req_frame->mgrReq.reqHead.cmdType == PS3_CMD_IOCTL) {
		instance->reserved[0] = 0xab;
	}

	return PS3_SUCCESS;
}

S32 force_has_scsi_pending_io(void * ptr)
{
	Bool *found = (Bool *)ptr;
	*found = PS3_TRUE;
	return PS3_SUCCESS;
}
#ifdef PS3_UT
void scsi_cmd_create(struct scsi_cmnd *s_cmd, struct ps3_instance *instance)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0))
	unsigned char scsi_cmnd_tmp[] = {0x2a, 0x00, 0x00, 0x00, 0x08, 0x00,
		0x00, 0x08, 0x00, 0x00};
	struct scsi_device *device;
#if defined DRIVER_SUPPORT_PRIV_BUSY
	struct ps3_scsi_priv_data *device_priv_data = (struct ps3_scsi_priv_data*)s_cmd->device->hostdata;
#endif
	memset(s_cmd, 0, sizeof(*s_cmd));
	s_cmd->cmnd = (U8 *)kmalloc(32, GFP_KERNEL);
	memset(s_cmd->cmnd, 0, sizeof(unsigned char) * 32);
	SCMD_GET_REQUEST(s_cmd) = (struct request *)kmalloc(sizeof(struct request), GFP_KERNEL);
	instance->host->can_queue = instance->cmd_context.max_scsi_cmd_count;
	instance->cmd_attr.nvme_page_size = PAGE_SIZE;
	s_cmd->sense_buffer = (unsigned char *)kmalloc(96, GFP_KERNEL);

	memcpy(s_cmd->cmnd, scsi_cmnd_tmp, 10);
	device = (struct scsi_device *)kmalloc(sizeof(struct scsi_device), GFP_KERNEL);
	s_cmd->device = device;
	s_cmd->device->host =  instance->host;
	SCMD_GET_REQUEST(s_cmd)->tag = 88;
	s_cmd->cmd_len = 10;

#if defined DRIVER_SUPPORT_PRIV_BUSY
	atomic_set(&device_priv_data->sdev_priv_busy, 9);
#else
	s_cmd->device->device_busy.counter = 9;
#endif
	instance->irq_context.high_iops_io_count.counter = 62;
#endif
}
void scsi_cmd_free(struct scsi_cmnd *s_cmd)
{
	kfree(s_cmd->cmnd);
	kfree(SCMD_GET_REQUEST(s_cmd));
	kfree(s_cmd->sense_buffer);
	kfree(s_cmd->device);
}

S32 force_host_reset(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;

	S32 ret_check = 0;

	struct scsi_cmnd pending_scsi_cmd;
	scsi_cmd_create(&pending_scsi_cmd, instance);
	ret_check = instance->host->hostt->eh_host_reset_handler(&pending_scsi_cmd);
	LOG_WARN("host reset [%d]\n", ret_check);
	scsi_cmd_free(&pending_scsi_cmd);
	return PS3_SUCCESS;
}
#endif
S32 force_return_false(void * ptr)
{
	Bool *found = (Bool *)ptr;
	*found = PS3_FALSE;
	return PS3_SUCCESS;
}

S32 force_return_true(void * ptr)
{
	Bool *ret = (Bool *)ptr;
	*ret = PS3_TRUE;
	return PS3_SUCCESS;
}

S32 force_scsi_priv_data_null(void * ptr)
{
	S32 ret_check = 0;
	struct scsi_cmnd *s_cmd = (struct scsi_cmnd *)ptr;
	if (s_cmd->device) {
		 s_cmd->device->hostdata = NULL;
	}
	return ret_check;
}
S32 force_scsi_task_cmd_error(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	struct ps3_instance *instance = cmd->instance;

	cmd->resp_frame->normalRespFrame.respStatus = PS3_DRV_MGR_TM_FAILED;
	cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
	complete(&cmd->sync_done);
	LOG_WARN("hno:%u force complete task mgr CFID:%d\n",
		PS3_HOST(instance), cmd->index);
	return PS3_SUCCESS;
}
S32 force_ioc_not_running(void * ptr)
{
	U32 * ioc_state = (U32 *)ptr;
	if(ioc_state) {
		*ioc_state = PS3_FW_STATE_RUNNING + 1;
	}

	return PS3_SUCCESS;
}
S32 force_ioc_running(void * ptr)
{
	U32 * ioc_state = (U32 *)ptr;
	if(ioc_state) {
		*ioc_state = PS3_FW_STATE_RUNNING;
	}

	return PS3_SUCCESS;
}

S32 force_task_cmd_alloc_null(void ** ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)*ptr;
	struct ps3_instance *instance = NULL;
	if(cmd) {
		instance = (struct ps3_instance *)cmd->instance;
		ps3_mgr_cmd_free(instance, cmd);
		*ptr = NULL;
	}
	return PS3_SUCCESS;
}
S32 force_pcie_err(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->state_machine.is_pci_err_recovery = PS3_TRUE;
    return PS3_SUCCESS;
}
S32 force_ptr_null(void ** ptr)
{
	if (*ptr) {
		*ptr = NULL;
	}
	return PS3_SUCCESS;
}
S32 wait_task_mgr_busy(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	while (!instance->task_manager_host_busy) {
		ps3_msleep(10);
	}
	return PS3_SUCCESS;
}
S32 wait_pd_clear(void * ptr)
{
	struct ps3_qos_pd_mgr *qos_pd_mgr = (struct ps3_qos_pd_mgr *)ptr;
	while (!qos_pd_mgr->clearing) {
		ps3_msleep(100);
	}
	return PS3_SUCCESS;
}

S32 reset_target_pause(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	while(instance->reserved[0] != 0xab) {
		ps3_msleep(10);
	}
	return PS3_SUCCESS;
}

S32 set_send_cmd_task_mgr_busy_flag(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->reserved[0] = 0xab;
	return PS3_SUCCESS;
}

S32 wait_qos_vd_send(void * ptr)
{
	struct ps3_qos_pd_mgr *qos_pd_mgr = (struct ps3_qos_pd_mgr *)ptr;
	while((qos_pd_mgr->vd_id == 0) ||
		(ps3_atomic_read(&qos_pd_mgr->pd_used_quota) >=
			qos_pd_mgr->pd_quota)) {
		msleep(1);
	}

	return PS3_SUCCESS;
}
S32 force_smp_cmd_error(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	struct ps3_instance *instance = cmd->instance;

	cmd->resp_frame->normalRespFrame.respStatus = PS3_DRV_MGR_SMP_BACKEND_ERR;
	cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
	complete(&cmd->sync_done);
	LOG_WARN("hno:%u force complete task mgr CFID:%d\n",
		PS3_HOST(instance), cmd->index);

	return PS3_SUCCESS;
}

S32 force_get_linkerrors_cmd_error(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	struct ps3_instance *instance = cmd->instance;

	cmd->resp_frame->normalRespFrame.respStatus = PS3_DRV_MGR_LINK_GET_BACKEND_ERR;
	cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
	complete(&cmd->sync_done);
	LOG_WARN("hno:%u force complete task mgr CFID:%d\n",
		PS3_HOST(instance), cmd->index);

	return PS3_SUCCESS;
}
S32 force_sas_phy_ctrl_cmd_error(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	struct ps3_instance *instance = cmd->instance;

	cmd->resp_frame->normalRespFrame.respStatus = PS3_DRV_MGR_PHY_CTL_BACKEND_ERR;
	cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
	complete(&cmd->sync_done);
	LOG_WARN("hno:%u force complete task mgr CFID:%d\n",
		PS3_HOST(instance), cmd->index);

	return PS3_SUCCESS;
}

S32 force_instance_state_to_unnormal(void *ptr)
{
	S32 * state = (S32 *)ptr;
	if(state) {
		*state = PS3_INSTANCE_STATE_RECOVERY;
    }

    return PS3_SUCCESS;
}
S32 force_instance_state_to_dead(void *ptr)
{
	S32 * state = (S32 *)ptr;
	if(state) {
		*state = PS3_INSTANCE_STATE_DEAD;
    }

    return PS3_SUCCESS;
}

S32 ps3_sas_encl_id_get_failed(void *ptr)
{
	U8 *encl_id = (U8 *)ptr;
	if(encl_id) {
		*encl_id = PS3_SAS_INVALID_ID;
    }

    return PS3_SUCCESS;
}

S32 ps3_sas_rphy_parent_sas_addr_get_failed(void *ptr)
{
	u64 *encl_id = (u64 *)ptr;
	if(encl_id) {
		*encl_id = PS3_SAS_INVALID_SAS_ADDR;
    }

    return PS3_SUCCESS;
}

S32 force_mgr_cmd_alloc_null(void ** ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)*ptr;
	struct ps3_instance *instance = NULL;
	if(cmd) {
		instance = (struct ps3_instance *)cmd->instance;
		ps3_mgr_cmd_free(instance, cmd);
		*ptr = NULL;
	}

	return PS3_SUCCESS;
}
S32 force_cmd_polling(void ** ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	if(cmd) {
		cmd->is_force_polling = 1;
	}

	return PS3_SUCCESS;
}

S32 force_pci_err_recovery(void ** ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	if(instance) {
		instance->state_machine.is_pci_err_recovery = PS3_TRUE;
	}

	return PS3_SUCCESS;
}

S32 force_blk_rq_bytes_invalid(void * ptr)
{
	struct request *req = (struct request *)ptr;

	if(req) {
		req->__data_len = U32_MAX;
	}
	return PS3_SUCCESS;
}
S32 wait_cmd_done(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->reserved[0] = 0xab;
	while(instance->reserved[0] == 0xab) {
		msleep(10);
	}
	return PS3_SUCCESS;
}

S32 wait_cmd_send_block(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	while(instance->reserved[0] == 0) {
		msleep(10);
	}
	return PS3_SUCCESS;
}

S32 set_r1x_conflict_process_flag(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->reserved[0] = 0xab;
	return PS3_SUCCESS;
}

S32 force_task_mgr_busy(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->task_manager_host_busy = PS3_TRUE;
	return PS3_SUCCESS;
}

S32 block_in_cmd_send(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->reserved[0] = 0xab;
	while(instance->reserved[0] == 0xab) {
		msleep(10);
	}
	return PS3_SUCCESS;
}

S32 create_pd_worker_fail(void * ptr)
{
	struct ps3_qos_pd_context *qos_pd_ctx = NULL;
	struct workqueue_struct **work_queues = NULL;
	qos_pd_ctx = (struct ps3_qos_pd_context *)ptr;
	work_queues = qos_pd_ctx->work_queues;

	if (qos_pd_ctx->workq_count > 1 && work_queues[1] != NULL) {
		destroy_workqueue(work_queues[1]);
		work_queues[1] = NULL;
		qos_pd_ctx->work_queues[1] = NULL;
	}

	return PS3_SUCCESS;
}

S32 create_tag_waitq_fail(void *ptr)
{
	struct ps3_qos_tg_context *qos_tg_ctx = (struct ps3_qos_tg_context *)ptr;
	if (qos_tg_ctx->vd_cmd_waitqs != NULL) {
		ps3_vfree(qos_tg_ctx->instance, qos_tg_ctx->vd_cmd_waitqs);
		qos_tg_ctx->vd_cmd_waitqs = NULL;

	}
	return PS3_SUCCESS;
}

S32 create_tag_workq_fail(void *ptr)
{
	struct ps3_qos_tg_context *qos_tg_ctx = (struct ps3_qos_tg_context *)ptr;
	if (qos_tg_ctx->work_queue != NULL) {
		destroy_workqueue(qos_tg_ctx->work_queue);
		qos_tg_ctx->work_queue = NULL;
	}

	return PS3_SUCCESS;
}

S32 create_vd_mgrs_fail(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	struct ps3_qos_vd_context *qos_vd_ctx = &instance->qos_context.vd_ctx;
	if (qos_vd_ctx->qos_vd_mgrs != NULL) {
		ps3_vfree(instance, qos_vd_ctx->qos_vd_mgrs);
		qos_vd_ctx->qos_vd_mgrs = NULL;
	}
	return PS3_SUCCESS;
}

S32 create_vd_workq_fail(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	struct ps3_qos_vd_context *qos_vd_ctx = &instance->qos_context.vd_ctx;
	if (qos_vd_ctx->work_queues != NULL) {
		ps3_vfree(instance, qos_vd_ctx->work_queues);
		qos_vd_ctx->work_queues = NULL;
	}
	return PS3_SUCCESS;

}

S32 create_vd_worker_fail(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	struct ps3_qos_vd_context *qos_vd_ctx = &instance->qos_context.vd_ctx;
	if (qos_vd_ctx->work_queues[0] != NULL) {
		destroy_workqueue(qos_vd_ctx->work_queues[0]);
		qos_vd_ctx->work_queues[0] = NULL;
	}
	return PS3_SUCCESS;
}

S32 create_pd_mgrs_fail(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	struct ps3_qos_pd_context *qos_pd_ctx = &instance->qos_context.pd_ctx;
	if (qos_pd_ctx->qos_pd_mgrs != NULL) {
		ps3_vfree(instance, qos_pd_ctx->qos_pd_mgrs);
		qos_pd_ctx->qos_pd_mgrs = NULL;
	}
	return PS3_SUCCESS;
}

S32 create_pd_waitq_fail(void *ptr)
{
	struct ps3_qos_pd_context *qos_pd_ctx = (struct ps3_qos_pd_context *)ptr;
	struct ps3_qos_pd_mgr *qos_pd_mgr = &qos_pd_ctx->qos_pd_mgrs[2];
	struct ps3_instance *instance = qos_pd_mgr->instance;
	if (qos_pd_mgr->waitqs != NULL) {
		ps3_vfree(instance, qos_pd_mgr->waitqs);
		qos_pd_mgr->waitqs = NULL;
	}
	return PS3_SUCCESS;
}

S32 create_pd_workq_fail(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	struct ps3_qos_pd_context *qos_pd_ctx = &instance->qos_context.pd_ctx;
	if (qos_pd_ctx->work_queues != NULL) {
		ps3_vfree(instance, qos_pd_ctx->work_queues);
		qos_pd_ctx->work_queues = NULL;
	}
	return PS3_SUCCESS;
}

S32 force_cmd_vd_seq_change(void *ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	cmd->req_frame->hwReq.reqHead.virtDiskSeq = 0xFF;
	return PS3_SUCCESS;
}

S32 force_fifo_depth_zero(void *ptr)
{
	*(U64 *)ptr = 0;
	return PS3_SUCCESS;
}

S32 create_mgr_waitq_fail(void *ptr)
{
	struct ps3_qos_softq_mgr *softq_mgr = (struct ps3_qos_softq_mgr *)ptr;
	if (softq_mgr->waitqs) {
		ps3_vfree(softq_mgr->instance, softq_mgr->waitqs);
		softq_mgr->waitqs = NULL;
	}

	return PS3_SUCCESS;
}

S32 create_mgr_worker_fail(void *ptr)
{
	struct ps3_qos_softq_mgr *softq_mgr = (struct ps3_qos_softq_mgr *)ptr;
	if (softq_mgr->work_queue) {
		destroy_workqueue(softq_mgr->work_queue);
		softq_mgr->work_queue = NULL;
	}

	return PS3_SUCCESS;
}

S32 create_cmd_waitq_fail(void *ptr)
{
	struct ps3_qos_softq_mgr *softq_mgr = (struct ps3_qos_softq_mgr *)ptr;
	if (softq_mgr->waitqs) {
		ps3_vfree(softq_mgr->instance, softq_mgr->waitqs);
		softq_mgr->waitqs = NULL;
	}
	return PS3_SUCCESS;
}

S32 create_cmd_worker_fail(void *ptr)
{
	struct ps3_qos_softq_mgr *softq_mgr = (struct ps3_qos_softq_mgr *)ptr;
	if (softq_mgr->id == 1 && softq_mgr->work_queue) {
		destroy_workqueue(softq_mgr->work_queue);
		softq_mgr->work_queue = NULL;
	}
	return PS3_SUCCESS;
}

S32  wait_for_dead_or_pre_operational(void *ptr)
{
	U32 wait_cnt = 180 * 3;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;
	U32 idx = 0;
	struct ps3_instance *instance = (struct ps3_instance *)ptr;

	LOG_INFO("hno:%u  wait dead or pre_operational begin\n", PS3_HOST(instance));
	for (idx = 0; idx < wait_cnt; idx++) {
		if (ps3_no_need_recovery(instance)) {
			break;
		}

		cur_state = ps3_atomic_read(&instance->state_machine.state);
		if ((cur_state == PS3_INSTANCE_STATE_PRE_OPERATIONAL) ||
			(cur_state == PS3_INSTANCE_STATE_OPERATIONAL) ||
			(cur_state == PS3_INSTANCE_STATE_DEAD) ||
			(cur_state == PS3_INSTANCE_STATE_QUIT)) {
			break;
		}

		ps3_msleep(1000);
	}

	if (idx >= wait_cnt) {
		LOG_WARN("hno:%u  wait dead or pre_operational timeout!\n",
			PS3_HOST(instance));
	}

	if ((cur_state != PS3_INSTANCE_STATE_PRE_OPERATIONAL) &&
		(cur_state != PS3_INSTANCE_STATE_OPERATIONAL) &&
		(cur_state != PS3_INSTANCE_STATE_DEAD)) {
		LOG_WARN("hno:%u  wait dead or pre_operational failed!\n",
			PS3_HOST(instance));
		return -PS3_FAILED;

	}

	LOG_INFO("hno:%u  wait dead or pre_operational success!\n", PS3_HOST(instance));
	return PS3_SUCCESS;
}
S32  force_init_cmd_failed(void *ptr)
{
    struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U64 state;

	if (instance->reg_set != NULL) {
		instance->reg_set->reg_f.Excl_reg.ps3SocFwState.reg.ps3SocFwState =
			PS3_FW_STATE_UNDEFINED;
	}
	state = instance->ioc_adpter->ioc_state_get(instance);

	LOG_WARN("ioc state [%llu]\n", state);
    return PS3_SUCCESS;
}

S32  force_recovery_init_cmd_failed(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U64 state;

	if (instance->reg_set != NULL && instance->reserve[0]) {
		instance->reg_set->reg_f.Excl_reg.ps3SocFwState.reg.ps3SocFwState =
			PS3_FW_STATE_UNDEFINED;
	}
	state = instance->ioc_adpter->ioc_state_get(instance);

	LOG_WARN("ioc state [%llu]\n", state);
	return PS3_SUCCESS;
}

S32  force_recovery_operation_cmd_failed(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U64 state;

	if (instance->reserve[0]) {
		ps3_atomic_set(&instance->state_machine.state, PS3_INSTANCE_STATE_RECOVERY);
	}
	state = ps3_atomic_read(&instance->state_machine.state);

	LOG_WARN("ioc state [%llu]\n", state);
	return PS3_SUCCESS;
}

S32  force_init_cmd_running_failed(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	U64 state;
	if (instance->reg_set != NULL) {
		instance->reg_set->reg_f.Excl_reg.ps3SocFwState.reg.ps3SocFwState =
			PS3_FW_STATE_FAULT;
	}
	state = instance->ioc_adpter->ioc_state_get(instance);

	LOG_WARN("ioc state [%llu]\n", state);
    return PS3_SUCCESS;
}

static void ps3_test_recovery_call(struct work_struct* work)
{
	struct ps3_instance *instance = NULL;

	(void)work;
	instance = g_work_que_recovery.instance;

	LOG_WARN("ps3_test_reco_call enter\n");

	ps3_hard_recovery_request(instance);
	g_work_que_recovery.state = 1;
	LOG_WARN("ps3_test_reco_call quit\n");
}
void ps3_start_recovery_rand_thread(struct ps3_instance *instance)
{
	char _queue_name[32] = {"test_reco"};

	INIT_DELAYED_WORK(&g_work_que_recovery._work, ps3_test_recovery_call);

	g_work_que_recovery._queue = create_singlethread_workqueue(_queue_name);
	if (g_work_que_recovery._queue == NULL) {
		return;
	}

	g_work_que_recovery.instance = instance;
	g_work_que_recovery.state = 0;

	queue_delayed_work(g_work_que_recovery._queue, &g_work_que_recovery._work,
		msecs_to_jiffies(prandom_u32() % 800));
}
S32  force_hard_reset_request(void *ptr)
{
    struct ps3_instance *instance = (struct ps3_instance *)ptr;
	if (instance->peer_instance != NULL) {
		ps3_start_recovery_rand_thread(instance->peer_instance);
		while (ps3_atomic_read(&instance->recovery_context->hardreset_ref) == 0) {
			msleep(5);
		}
	} else {
		ps3_hard_recovery_request(instance);
	}

	return PS3_SUCCESS;
}
S32  force_async_hard_reset_request(void *ptr)
{
    struct ps3_instance *instance = (struct ps3_instance *)ptr;
	ps3_start_recovery_rand_thread(instance);

    return PS3_SUCCESS;
}

S32  wait_scsi_cmd_done_fail(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->reserved[0] = 0xab;
	return PS3_SUCCESS;
}
void ps3_wait_recovery_rand_finish(void)
{
	LOG_WARN("enter ps3_wait_reco_rand_finish\n");
	while (g_work_que_recovery.state != 1) {
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
	}
	if (g_work_que_recovery._queue != NULL) {
		if (!cancel_delayed_work_sync(&g_work_que_recovery._work)) {
			flush_workqueue(g_work_que_recovery._queue);
		}
		while (g_work_que_recovery.instance->recovery_context->recovery_state != \
			PS3_HARD_RECOVERY_FINISH) {
			ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
		}

		destroy_workqueue(g_work_que_recovery._queue);
		g_work_que_recovery._queue = NULL;
	}
	LOG_WARN("quit ps3_wait_reco_rand_finish\n");
}

S32 force_hard_recovery_request_nosupport_fail(void *ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance = instance;
	ps3_hard_reset_enable_modify(0);
    return PS3_SUCCESS;
}

S32 force_seq_cmd(void *ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	cmd->io_attr.seq_flag = SCSI_RW_SEQ_CMD;
	return PS3_SUCCESS;
}

S32 wait_cmd_free(void *ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	while (cmd->r1x_peer_cmd != NULL) {
		ps3_msleep(20);
	}
	return PS3_SUCCESS;
}

S32 wait_abort_flag(void *ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	while (cmd->is_aborting != 1) {
		ps3_msleep(20);
	}
	return PS3_SUCCESS;
}

S32 wait_cmd_alloc1(void *ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	Bool is_first = PS3_TRUE;
	while (cmd->trace_id != 0xff) {
		if (is_first) {
#ifdef PS3_UT
			cmd->scmd->is_in_err += 1;
#endif
			is_first = PS3_FALSE;
		}
		ps3_msleep(20);
	}
	cmd->trace_id = 0;
	return PS3_SUCCESS;
}

S32 wait_cmd_alloc2(void *ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	while (cmd->scmd == NULL) {
		ps3_msleep(20);
	}
#ifdef PS3_UT
	cmd->scmd->is_in_err += 1;
#endif
	return PS3_SUCCESS;
}

S32 set_abort_count(void *ptr)
{
#ifdef PS3_UT
	struct scsi_cmnd *s_cmnd = (struct scsi_cmnd *)ptr;
	s_cmnd->abort_count++;
#endif
	return PS3_SUCCESS;
}
S32 wait_cmd_alloc(void *ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	while (cmd->trace_id != 0xff) {
		ps3_msleep(20);
	}
	cmd->trace_id = 0;
	return PS3_SUCCESS;
}

S32 set_cmd_tid_flag(void *ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	cmd->trace_id = 0xff;
	return PS3_SUCCESS;
}

S32 set_cmd_io_rw_flag_unkown(void * ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	cmd->io_attr.rw_flag = PS3_SCSI_CMD_TYPE_UNKOWN;
	return PS3_SUCCESS;
}

S32 set_soft_zone_type_unkown(void * ptr)
{
	U8 *type = (U8 *)ptr;
	*type = PS3_DEV_TYPE_UNKNOWN;
	return PS3_SUCCESS;
}

S32 force_rand_cmd(void *ptr)
{
	struct ps3_cmd *cmd = (struct ps3_cmd *)ptr;
	cmd->io_attr.seq_flag = SCSI_RW_RANDOM_CMD;
	return PS3_SUCCESS;
}

S32 force_ret_failed(void * ptr)
{
	S32 *ret = (S32 *)ptr;
	if(ret) {
		*ret = -PS3_FAILED;
	}

	return PS3_SUCCESS;
}

S32 force_cmd_not_mgr_cmd(void * ptr)
{
	U16 *index = (U16 *)ptr;
	*index = 0;
	return PS3_SUCCESS;
}

S32 force_phy_count_zero(void * ptr)
{
	U8 *phy_count = (U8 *)ptr;
	*phy_count = 0;
	return PS3_SUCCESS;
}

S32 force_doorbell_failed(void * ptr)
{
	U32 *fw_cur_state = (U32 *)ptr;

	LOG_INFO("force doorbell failed start\n");
	*fw_cur_state = PS3_FW_STATE_READY;
	LOG_INFO("force doorbell failed end\n");

	return PS3_SUCCESS;
}

S32 force_vd_count_err(void * ptr)
{
	U16 *count = (U16 *)ptr;
	if(count) {
		*count = 2;
	}

	return PS3_SUCCESS;
}

S32 force_chan_err(void * ptr)
{
	U8 *chan = (U8 *)ptr;
	if(chan) {
		*chan = 255;
	}

	return PS3_SUCCESS;
}

S32 force_dev_err(void * ptr)
{
	struct PS3Dev *dev = (struct PS3Dev *)ptr;
	if(dev) {
		dev->softChan = 15;
	}

	return PS3_SUCCESS;
}

S32 force_target_err(void * ptr)
{
	U16 *target = (U16 *)ptr;
	if(target) {
		*target = 65535;
	}

	return PS3_SUCCESS;
}

S32 force_diskpos_err(void * ptr)
{
	U32 *disk_id = (U32 *)ptr;
	if(disk_id) {
		*disk_id = 0;
	}

	return PS3_SUCCESS;
}
S32 force_s32_zero(void * ptr)
{
	S32 *ret = (S32 *)ptr;
	if(ret) {
		*ret = 0;
	}

	return PS3_SUCCESS;
}
S32 force_s32_letter_zero(void * ptr)
{
	S32 *ret = (S32 *)ptr;
	if(ret) {
		*ret = 0;
	}

	return PS3_SUCCESS;
}

S32 force_align_err(void * ptr)
{
	U8 *align = (U8 *)ptr;
	if(align) {
		*align = 16;
	}

	return PS3_SUCCESS;
}

S32 force_lun_err(void * ptr)
{
	U64 *lun = (U64 *)ptr;
	if(lun) {
		*lun = 1;
	}

	return PS3_SUCCESS;
}

S32 force_dev_type_err(void * ptr)
{
	U8 *type = (U8 *)ptr;
	if(type) {
		*type = 0;
	}

	return PS3_SUCCESS;
}

S32 force_pd_state_err(void * ptr)
{
	U8 *align = (U8 *)ptr;
	if(align) {
		*align = DEVICE_STATE_OUTING;
	}

	return PS3_SUCCESS;
}

S32 force_thread_null(void * ptr)
{
	struct ps3_r1x_lock_mgr *mgr = (struct ps3_r1x_lock_mgr *)ptr;
	if (mgr->conflict_send_th != NULL) {
		mgr->thread_stop = PS3_TRUE;
		complete(&mgr->thread_sync);
		kthread_stop(mgr->conflict_send_th);
		mgr->conflict_send_th = NULL;
	}
	return PS3_SUCCESS;
}

S32 wait_pcie_err(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;

	LOG_INFO("wait pcie err start\n");
	instance->reserve[0] = 1;
	while(!ps3_pci_err_recovery_get(instance)){
		msleep(50);
	}
	LOG_INFO("wait pcie err end\n");

	return PS3_SUCCESS;
}

S32 set_iops_channel(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	if (ps3_get_pci_function(instance->pdev) == PS3_FUNC_ID_1) {
		instance->irq_context.high_iops_msix_vectors = 16;
	}
	return PS3_SUCCESS;
}
S32 force_pcie_frozen(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->reg_set = NULL;
	return PS3_SUCCESS;
}

S32 for_mod_so_addr(void * ptr)
{
	struct ps3_instance *instance = (struct ps3_instance *)ptr;
	instance->so_start_addr = PCIE_DMA_HOST_ADDR_BIT_POS_SET(0x5000);
	instance->so_end_addr = PCIE_DMA_HOST_ADDR_BIT_POS_SET(0x10000);
	return PS3_SUCCESS;
}

void active_err_inject(void)
{
	U32 idx = 0;
	U32 *arr_base = ps3_err_inject_array_query();
	U32 err_inject_num = ps3_err_inject_num_query();
	LOG_DEBUG("active err_inject_num:%u\n",
		err_inject_num);

	for(; idx < err_inject_num;)
	{
		if ((arr_base[idx] >= PS3_ERR_IJ_WATCHDOG_CONCURY) &&
			(arr_base[idx] < PS3_ERR_IJ_MAX_COUNT)) {
			INJECT_ACTIVE(arr_base[idx], arr_base[idx + 1])
			LOG_DEBUG("active err_inject:%u count:%u \n",
				arr_base[idx], arr_base[idx + 1]);
			idx += 2;
		}
	}
}
void inject_init(void)
{
	if(is_inject_init == PS3_TRUE) {
		return;
	}
	is_inject_init = PS3_TRUE;
	INJECT_REG(PS3_ERR_IJ_IOCTL_CMD_DEAD,   force_ioctl_cmd_dead)
	INJECT_REG(PS3_ERR_IJ_IOCTL_CMD_PENDING,   force_ioctl_cmd_pending)
	INJECT_REG(PS3_ERR_IJ_IOCTL_CMD_SIG_INTERRUPT,   force_ioctl_cmd_sig_interrupt)
	INJECT_REG(PS3_ERR_IJ_IOCTL_CMD_INTERRUPT,   force_ioctl_cmd_interrupt)
	INJECT_REG(PS3_ERR_IJ_IOCTL_CMD_NO_RESP,   force_ioctl_cmd_no_resp)
	INJECT_REG(PS3_ERR_IJ_IOCTL_CMD_ABORT_FAIL,   force_return_fail)
	INJECT_REG(PS3_ERR_IJ_IOCTL_CMD_RECOVERY_CANCEL, force_ioctl_cmd_recovery_cancel)
	INJECT_REG(PS3_ERR_IJ_IOCTL_CMD_ABORT_NO_RESP,   force_ioctl_cmd_abort_no_resp)
	INJECT_REG(PS3_ERR_IJ_IOCTL_CMD_FORCE_DONE,   force_ioctl_cmd_force_done)
	INJECT_REG(PS3_ERR_IJ_WAIT_NORMAL,   wait_instance_normal)
	INJECT_REG(PS3_ERR_IJ_FIRMWARE_INIT_FAIL,   force_ret_fail)
	INJECT_REG(PS3_ERR_IJ_SCSI_SCAN_HOST_DELAY,   force_scan_host_delay)
	INJECT_REG(PS3_ERR_IJ_SCSI_SCAN_HOST_NOT_FINISH,   force_scan_host_fail)
	INJECT_REG(PS3_ERR_IJ_IGNORE_TASK_IO,   force_ignore_task_io)
	INJECT_REG(PS3_ERR_IJ_IGNORE_TASK_ABORT,   force_ignore_task_abort)
	INJECT_REG(PS3_ERR_IJ_IGNORE_TASK_RESET,   force_ignore_task_reset)
	INJECT_REG(PS3_ERR_IJ_EVENT_HANDLE,        force_event_handle)
	INJECT_REG(PS3_ERR_IJ_DEL_SCSI_DEV1, force_del_disk)
	INJECT_REG(PS3_ERR_IJ_DEL_SCSI_DEV2, force_del_disk)
	INJECT_REG(PS3_ERR_IJ_R1X_DEL_SCSI_DEV1, force_del_disk)
	INJECT_REG(PS3_ERR_IJ_PRIV_DATA_NULL1, force_priv_data_null)
	INJECT_REG(PS3_ERR_IJ_PRIV_DATA_NULL2, force_priv_data_null)
	INJECT_REG(PS3_ERR_IJ_PRIV_DATA_NULL3, force_priv_data_null)
	INJECT_REG(PS3_ERR_IJ_PRIV_DATA_NULL4, force_priv_data_null)
	INJECT_REG(PS3_ERR_IJ_R1X_PRIV_DATA_NULL1, force_priv_data_null)
	INJECT_REG(PS3_ERR_IJ_HT_ABNORMAL1, force_return_fail)
	INJECT_REG(PS3_ERR_IJ_HT_ABNORMAL2, force_return_fail)
	INJECT_REG(PS3_ERR_IJ_EVENT_INT_WAIT_HARD_FLAG, force_int_wait_hard_flag)
	INJECT_REG(PS3_ERR_IJ_EVENT_INT_WAIT_HARD_SUBCRIBE, force_int_wait_hard_subcribe)
	INJECT_REG(PS3_ERR_IJ_EVENT_PROC_WAIT_HARD_FLAG, force_wait_hard_flag)
	INJECT_REG(PS3_ERR_IJ_EVENT_PROC_WAIT_HARD_SUBCRIBE, force_wait_hard_subcribe)
	INJECT_REG(PS3_ERR_IJ_EVENT_SUBC_WAIT_HARD_FLAG, force_wait_hard_flag)
	INJECT_REG(PS3_ERR_IJ_EVENT_SUBC_WAIT_HARD_SUBCRIBE, force_wait_hard_subcribe)
	INJECT_REG(PS3_ERR_IJ_FLAG_WAIT_EVENT_INT, force_wait_event_int)
	INJECT_REG(PS3_ERR_IJ_FLAG_WAIT_EVENT_PROC, force_wait_event_proc)
	INJECT_REG(PS3_ERR_IJ_FLAG_WAIT_EVENT_SUBCRIBE, force_wait_event_subscribe)
	INJECT_REG(PS3_ERR_IJ_HARD_SUBC_WAIT_EVENT_INT, force_wait_event_int)
	INJECT_REG(PS3_ERR_IJ_HARD_SUBC_WAIT_EVENT_PROC, force_wait_event_proc)
	INJECT_REG(PS3_ERR_IJ_HARD_SUBC_WAIT_EVENT_SUBCRIBE, force_wait_event_subscribe)
	INJECT_REG(PS3_ERR_IJ_STORE_NO_TRIGGER_LOG, force_trigger_log_fail)
	INJECT_REG(PS3_ERR_IJ_DETECT_NO_TRIGGER_LOG, force_trigger_log_fail)
	INJECT_REG(PS3_ERR_IJ_EVENT_CMD_NULL, force_event_cmd_null)
	INJECT_REG(PS3_ERR_IJ_SUBC_EVENT_CMD_INIT, force_event_cmd_null)
	INJECT_REG(PS3_ERR_IJ_SUBC_EVENT_CMD_INIT,force_event_cmd_init)
	INJECT_REG(PS3_ERR_IJ_RESUBC_EVENT_CMD_INIT,force_event_cmd_init)
	INJECT_REG(PS3_ERR_IJ_CANCEL_EVENT_CMD_FAIL, force_return_fail)
	INJECT_REG(PS3_ERR_IJ_CANCEL_VDPENDING_CMD_FAIL, force_return_fail)
	INJECT_REG(PS3_ERR_IJ_FW_STATE_RUNNING, force_return_fail)
	INJECT_REG(PS3_ERR_IJ_SET_IOC_IN_SECURITY, force_set_ioc_in_security)
	INJECT_REG(PS3_ERR_IJ_WAIT_UNNORMAL, wait_instance_unnormal)
	INJECT_REG(PS3_ERR_IJ_DUMP_WAIT_RECO_VALID, wait_reco_dump_valid)
	INJECT_REG(PS3_ERR_IJ_DUMP_WAIT_RECO_VALID_2, wait_reco_dump_valid)
	INJECT_REG(PS3_ERR_IJ_DUMP_WAIT_NORMAL, wait_instance_normal)
	INJECT_REG(PS3_ERR_IJ_WAIT_OPT, wait_instance_operational)
	INJECT_REG(PS3_ERR_IJ_WAIT_READY, wait_ioc_ready)
	INJECT_REG(PS3_ERR_IJ_HARD_WAIT_READY_FAILED_F1, force_hard_ready_failed)
	INJECT_REG(PS3_ERR_IJ_HARD_WAIT_READY_FAILED_F0, force_hard_ready_failed)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_WAIT_FUNC0_RUNNING_1, wait_recovery_func0_running_1)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_WAIT_FUNC0_RUNNING_2, wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FWSTATE_REMOVE, wait_remove_is_load_flag)
	INJECT_REG(PS3_ERR_IJ_IOCTL_CMD_RETRY_DONE,  ioctl_cmd_retry_done)
	INJECT_REG(PS3_ERR_IJ_QOS_SET_SHARE_COUNT, set_qos_share_cnt)
	INJECT_REG(PS3_ERR_IJ_IOCTL_WAIT_UNNORMAL, wait_instance_unnormal)
	INJECT_REG(PS3_ERR_IJ_FORCE_EVENT_CMD_FAIL_DEAD, force_event_cmd_dead)
	INJECT_REG(PS3_ERR_IJ_IOCTL_WAIT_IRQ_DISABLE, wait_irq_disable)
	INJECT_REG(PS3_ERR_IJ_WAIT_IOCTL_IN_RECOVERY, wait_ioctl_detect_irq_disable)
	INJECT_REG(PS3_ERR_IJ_WAIT_IOCTL_IN_DEVICE_RESET, wait_ioctl_detect_irq_disable)
	INJECT_REG(PS3_ERR_IJ_SEND_IOCTL_BLOCK_MODE_FLAG, set_send_ioctl_block_flag)
	INJECT_REG(PS3_ERR_IJ_FORCE_HAS_SCIS_PENDING_IO, force_has_scsi_pending_io)
	INJECT_REG(PS3_ERR_IJ_HOST_RESET_WAIT_DECIDE, host_reset_wait_decide_normal)
#ifdef PS3_UT
	INJECT_REG(PS3_ERR_IJ_ABORT_PRE1_FORCE_ABORTED_CMD_DONE, force_scsi_cmd_done)
	INJECT_REG(PS3_ERR_IJ_ABORT_PRE2_FORCE_ABORTED_CMD_DONE, force_scsi_cmd_done)
	INJECT_REG(PS3_ERR_IJ_ABORT_PRE_BULID_FORCE_ABORTED_CMD_DONE, force_scsi_cmd_done)
	INJECT_REG(PS3_ERR_IJ_ABORT_PRE_BULID1_FORCE_ABORTED_CMD_DONE, force_scsi_cmd_done)
	INJECT_REG(PS3_ERR_IJ_ABORT_PRE_SEND_FORCE_ABORTED_CMD_DONE, force_scsi_cmd_done)
	INJECT_REG(PS3_ERR_IJ_ABORT_CMD_ERROR, force_scsi_task_cmd_error)
	INJECT_REG(PS3_ERR_IJ_RESET_CMD_ERROR, force_scsi_task_cmd_error)
	INJECT_REG(PS3_ERR_IJ_SCSI_TASK_PRE_CHECK_FAILED, force_pcie_err)
	INJECT_REG(	PS3_ERR_IJ_FORCE_INSTANCE_UNNORMAL, force_instance_state_unnormal)
	INJECT_REG(PS3_ERR_IJ_TASK_CMD_ALLOC_FAILED, force_task_cmd_alloc_null)
	INJECT_REG(PS3_ERR_IJ_SMP_CMD_ERROR, force_smp_cmd_error)
	INJECT_REG(PS3_ERR_IJ_GET_LINKERRORS_CMD_ERROR, force_get_linkerrors_cmd_error)
	INJECT_REG(PS3_ERR_IJ_PHY_CTRL_CMD_ERROR, force_sas_phy_ctrl_cmd_error)
	INJECT_REG(PS3_ERR_IJ_PROBE_HOST_RESET, force_host_reset)
#else
	INJECT_REG(PS3_ERR_IJ_ABORT_PRE1_FORCE_ABORTED_CMD_DONE, ps3_scsi_rw_cmd_filter_handle)
	INJECT_REG(PS3_ERR_IJ_ABORT_PRE2_FORCE_ABORTED_CMD_DONE, ps3_scsi_rw_cmd_filter_handle)
	INJECT_REG(PS3_ERR_IJ_ABORT_PRE_BULID_FORCE_ABORTED_CMD_DONE, ps3_scsi_rw_cmd_filter_handle)
	INJECT_REG(PS3_ERR_IJ_ABORT_PRE_BULID1_FORCE_ABORTED_CMD_DONE, ps3_scsi_rw_cmd_filter_handle)
	INJECT_REG(PS3_ERR_IJ_ABORT_PRE_SEND_FORCE_ABORTED_CMD_DONE, ps3_scsi_rw_cmd_filter_handle)

	INJECT_REG(PS3_ERR_IJ_ABORT_CMD_TIMEOUT, ps3_scsi_task_cmd_filter_handle)
	INJECT_REG(PS3_ERR_IJ_RESET_CMD_TIMEOUT, ps3_scsi_task_cmd_filter_handle)
	INJECT_REG(PS3_ERR_IJ_ABORT_CMD_ERROR, ps3_scsi_task_cmd_filter_handle)
	INJECT_REG(PS3_ERR_IJ_RESET_CMD_ERROR, ps3_scsi_task_cmd_filter_handle)
	INJECT_REG(PS3_ERR_IJ_ABORT_CMD_NORMAL, ps3_scsi_task_cmd_filter_handle)
	INJECT_REG(PS3_ERR_IJ_RESET_CMD_NORMAL, ps3_scsi_task_cmd_filter_handle)

	INJECT_REG(PS3_ERR_IJ_SMP_CMD_TIMEOUT, ps3_mgr_cmd_filter_handle)
	INJECT_REG(PS3_ERR_IJ_SMP_CMD_ERROR, ps3_mgr_cmd_filter_handle)
	INJECT_REG(PS3_ERR_IJ_GET_LINKERRORS_CMD_ERROR, ps3_mgr_cmd_filter_handle)
	INJECT_REG(PS3_ERR_IJ_PHY_CTRL_CMD_ERROR, ps3_mgr_cmd_filter_handle)
#endif
	INJECT_REG(PS3_ERR_IJ_ABORT_PRE_DEAL_FORCE_ABORTED_CMD_DONE, force_aborted_cmd_done)
	INJECT_REG(PS3_ERR_IJ_FORCE_STOP_ALL_CMD_DONE, force_stop_aborted_cmd_done)
	INJECT_REG(PS3_ERR_IJ_QOS_NOT_AWAKE_PD, force_return_false)
	INJECT_REG(PS3_ERR_IJ_UNLOAD_TIMEOUT, force_ioctl_cmd_no_resp)
	INJECT_REG(PS3_ERR_IJ_QOS_PD_RESEND_NOT_EXPIRED, force_return_false)
	INJECT_REG(PS3_ERR_IJ_IOC_NOT_RUNNING, force_ioc_not_running)
	INJECT_REG(PS3_ERR_IJ_IOC_IS_NOT_NORMAL_IN_UNLOAD, force_ioc_not_running)
	INJECT_REG(PS3_ERR_IJ_HALF_HARD_RESET, force_return_true)
	INJECT_REG(PS3_ERR_IJ_ROMOVE_UNLOAD_FAIL, force_mgr_cmd_alloc_null)
	INJECT_REG(PS3_ERR_IJ_WAIT_TASK_MGR_BUSY, wait_task_mgr_busy)
	INJECT_REG(PS3_ERR_IJ_RESET_TARGET_PAUSE, reset_target_pause)
	INJECT_REG(PS3_ERR_IJ_SEND_CMD_TASK_MGR_BUSY, set_send_cmd_task_mgr_busy_flag)
	INJECT_REG(PS3_ERR_IJ_QOS_VD_MEMBER_CLEARING, wait_pd_clear)
	INJECT_REG(PS3_ERR_IJ_QOS_WAIT_VD_SEND, wait_qos_vd_send)
	INJECT_REG(PS3_ERR_IJ_SAS_REQ_PRE_CHECK, force_instance_state_to_unnormal)
	INJECT_REG(PS3_ERR_IJ_SAS_RPHY_SLOT_GET_FAILED, force_ret_fail)
	INJECT_REG(PS3_ERR_IJ_SAS_EXP_RPHY_SLOT_GET_FAILED, force_ret_fail)
	INJECT_REG(PS3_ERR_IJ_SAS_ENCL_ID_GET_FAILED, ps3_sas_encl_id_get_failed)
	INJECT_REG(PS3_ERR_IJ_MGR_CMD_ALLOC_FAILED, force_mgr_cmd_alloc_null)
	INJECT_REG(PS3_ERR_IJ_CMD_POLLING, force_cmd_polling)
	INJECT_REG(PS3_ERR_IJ_PCI_ERR_RECOVERY, force_pci_err_recovery)
	INJECT_REG(PS3_ERR_IJ_INS_STATE_UNNORMAL, force_instance_state_to_unnormal)
	INJECT_REG(PS3_ERR_IJ_INS_STATE_DEAD, force_instance_state_to_dead)
	INJECT_REG(PS3_ERR_IJ_RPHY_PARENT_SAS_ADDR_GET_FAILED, ps3_sas_rphy_parent_sas_addr_get_failed)
	INJECT_REG(PS3_ERR_IJ_SAS_NODE_NOT_FOUND, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_SAS_REQ_EXT_BUF_INVALLID, force_blk_rq_bytes_invalid)
	INJECT_REG(PS3_ERR_IJ_EXT_BUF_TO_RSP_INVALLID,force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_WAIT_CMD_DONE, wait_cmd_done)
	INJECT_REG(PS3_ERR_IJ_CMD_SEND_BLOCK, wait_cmd_send_block)
	INJECT_REG(PS3_ERR_IJ_FORCE_TASK_MGR_BUSY, force_task_mgr_busy)
	INJECT_REG(PS3_ERR_IJ_FORCE_INS_UNLOAD, force_instance_unload)
	INJECT_REG(PS3_ERR_IJ_FORCE_INSTANCE_UNNORMAL, force_instance_state_unnormal)
	INJECT_REG(PS3_ERR_IJ_QOS_PD_INIT_FAIL_1, create_pd_worker_fail)
	INJECT_REG(PS3_ERR_IJ_QOS_PD_INIT_FAIL_2, create_pd_mgrs_fail)
	INJECT_REG(PS3_ERR_IJ_QOS_PD_INIT_FAIL_3, create_pd_waitq_fail)
	INJECT_REG(PS3_ERR_IJ_QOS_PD_INIT_FAIL_4, create_pd_workq_fail)
	INJECT_REG(PS3_ERR_IJ_QOS_TAG_INIT_FAIL_1, create_tag_waitq_fail)
	INJECT_REG(PS3_ERR_IJ_QOS_TAG_INIT_FAIL_2, create_tag_workq_fail)
	INJECT_REG(PS3_ERR_IJ_QOS_VD_INIT_FAIL_1, create_vd_mgrs_fail)
	INJECT_REG(PS3_ERR_IJ_QOS_VD_INIT_FAIL_2, create_vd_workq_fail)
	INJECT_REG(PS3_ERR_IJ_QOS_VD_INIT_FAIL_3, create_vd_worker_fail)
	INJECT_REG(PS3_ERR_IJ_QOS_WAIT_PD_WAITQ_CLEAR, block_in_cmd_send)
	INJECT_REG(PS3_ERR_IJ_QOS_WAIT_PD_WAITQ_CLEAR_2, block_in_cmd_send)
	INJECT_REG(PS3_ERR_IJ_QOS_WAIT_VD_WAITQ_CLEAR, block_in_cmd_send)
	INJECT_REG(PS3_ERR_IJ_QOS_WAIT_TAG_WAITQ_CLEAR, block_in_cmd_send)
	INJECT_REG(PS3_ERR_IJ_QOS_NOT_AWAKE_TAG, force_return_false)
	INJECT_REG(PS3_ERR_IJ_CMD_SEND_FORCE_INS_UNLOAD, force_instance_unload)
	INJECT_REG(PS3_ERR_IJ_CMD_SEND_FORCE_UNORMAL, force_instance_state_unnormal)
	INJECT_REG(PS3_ERR_IJ_CMD_SEND_FORCE_PCI_ERR, force_instance_state_pci_err)
	INJECT_REG(PS3_ERR_IJ_QOS_FORCE_VD_SEQ_CHANGE, force_cmd_vd_seq_change)
	INJECT_REG(PS3_ERR_IJ_QOS_FORCE_MGRQ_ZERO_DEPTH, force_fifo_depth_zero)
	INJECT_REG(PS3_ERR_IJ_QOS_FORCE_CMDQ_ZERO_DEPTH, force_fifo_depth_zero)
	INJECT_REG(PS3_ERR_IJ_QOS_MGRQ_INIT_FAIL_1, create_mgr_waitq_fail)
	INJECT_REG(PS3_ERR_IJ_QOS_MGRQ_INIT_FAIL_2, create_mgr_worker_fail)
	INJECT_REG(PS3_ERR_IJ_QOS_CMDQ_INIT_FAIL_1, create_cmd_waitq_fail)
	INJECT_REG(PS3_ERR_IJ_QOS_CMDQ_INIT_FAIL_2, create_cmd_worker_fail)
	INJECT_REG(PS3_ERR_IJ_QOS_WAIT_SOFT_WAITQ_CLEAR, block_in_cmd_send)
	INJECT_REG(PS3_ERR_IJ_BLOCK_HOST_RESET, block_in_cmd_send)
	INJECT_REG(PS3_ERR_IJ_R1X_CONFLICTQ_PROCESS_FINISH, set_r1x_conflict_process_flag)
	INJECT_REG(PS3_ERR_IJ_WAIT_TASK_MGR_BUSY, wait_task_mgr_busy)
	INJECT_REG(PS3_ERR_IJ_RESET_TARGET_PAUSE, reset_target_pause)
	INJECT_REG(PS3_ERR_IJ_WAIT_HARD_RESET, wait_for_dead_or_pre_operational)
	INJECT_REG(PS3_ERR_IJ_FORCE_IOC_RUNNING, force_set_ioc_running)
	INJECT_REG(PS3_ERR_IJ_FORCE_INIT_FAIL, force_init_cmd_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_INIT_RUNNING_FAIL, force_init_cmd_running_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_INIT_RUNNING_FAIL2, force_init_cmd_running_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_LOAD_START_HARD, force_hard_reset_request)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_HOST_START_HARD, force_hard_reset_request)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_SCAN_START_HARD, force_hard_reset_request)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_A_SCAN_START_HARD, force_hard_reset_request)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_A_SCAN_FINISH_HARD, force_hard_reset_request)
	INJECT_REG(PS3_ERR_IJ_F1_PROBE_FORCE_IOC_FAULT, force_set_ioc_fault)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_WAIT_FUNC1_PROBE,wait_recovery_func1_probe)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_REQ_WAIT_FUNC1_PROBE,wait_recovery_req_func1_probe)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_WAIT_FUNC1_REMOVE,wait_recovery_func1_remove)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_REQ_WAIT_FUNC1_REMOVE,wait_recovery_req_func1_remove)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_F1_B_READY, wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_F1_A_READY, wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_F1_B_INIT, wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_F1_A_INIT, wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_F1_RUNNING, wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_F1_CTRL_INFO,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_F1_OPER,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_F1_SCSI_INIT,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_F1_LOAD,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_F1_B_SCAN,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_F1_A_SCAN,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_F1_SCAN_FINISH,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_RECOVERY_F1_EVENT,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_IOC_RUNNING_RESUME_RECOVERY,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_INIT_FAIL_RESUME_RECOVERY,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_INIT_RUNNING_FAIL_RESUME_RECOVERY,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_INIT_RUNNING_FAIL2_RESUME_RECOVERY,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_LOAD_START_HARD_RESUME_RECOVERY,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_HOST_START_HARD_RESUME_RECOVERY,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_F1_PROBE_FORCE_IOC_FAULT_RESUME_RECOVERY,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_SCAN_START_HARD_RESUME_RECOVERY,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_A_SCAN_START_HARD_RESUME_RECOVERY,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_IOC_NOT_RUNNING, force_set_ioc_fault)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_IOC_EVENT_FAIL, force_set_ioc_fault)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_IOC_VDINFO_FAIL, force_set_ioc_fault)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_IOC_WEB_FAIL, force_set_ioc_fault)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_IOC_UNLOAD_FAIL, force_set_ioc_fault)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_REQ_IOC_CANCEL_EVENT_WORK_FAIL_RESUME,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_REQ_IOC_EVENT_FAIL_RESUME,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_REQ_IOC_VDINFO_FAIL_RESUME,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_REQ_IOC_WEB_FAIL_RESUME,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_REQ_IOC_UNLOAD_FAIL_RESUME,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_IOC_CANCEL_EVENT_WORK_FAIL_RESUME,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_IOC_EVENT_FAIL_RESUME,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_IOC_VDINFO_FAIL_RESUME,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_IOC_WEB_FAIL_RESUME,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_IOC_UNLOAD_FAIL_RESUME,wait_recovery_func0_running_2)
	INJECT_REG(PS3_ERR_IJ_WAIT_SCSI_CMD_DONE_FAIL, wait_scsi_cmd_done_fail)
	INJECT_REG(PS3_ERR_IJ_INIT_CMD_PROC_FAIL, force_ret_fail)
	INJECT_REG(PS3_ERR_IJ_IOC_STATE_WAIT_TO_RUNNING_FAIL, force_ret_fail)
	INJECT_REG(PS3_ERR_IJ_IOC_INIT_PROC_STATE_RUNNING, force_ioc_running)
	INJECT_REG(PS3_ERR_IJ_INIT_PROC_FAIL_NOUNLOAD, force_ioc_running)
	INJECT_REG(PS3_ERR_IJ_GET_PD_INFO_RETURN_FAILED, force_ret_fail)
	INJECT_REG(PS3_ERR_IJ_FORCE_MEMDUP_USER_RETURN_FAILED, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_IOCTL_WAIT_NORMAL_FAILED, force_ret_fail)
	INJECT_REG(PS3_ERR_IJ_FORCE_IOCTL_ALLOC_CMD_FAILED, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_WAIT_CMD_FREE, wait_cmd_free)
	INJECT_REG(PS3_ERR_IJ_WAIT_ABORT_FLAG, wait_abort_flag)
	INJECT_REG(PS3_ERR_IJ_WAIT_CMD_ALLOC, wait_cmd_alloc)
	INJECT_REG(PS3_ERR_IJ_SET_CMD_TID_FLAG, set_cmd_tid_flag)
	INJECT_REG(PS3_ERR_IJ_WAIT_CMD_FREE1, wait_cmd_free)
	INJECT_REG(PS3_ERR_IJ_WAIT_ABORT_FLAG1, wait_abort_flag)
	INJECT_REG(PS3_ERR_IJ_SET_ABORT_COUNT, set_abort_count)
	INJECT_REG(PS3_ERR_IJ_WAIT_CMD_ALLOC1, wait_cmd_alloc1)
	INJECT_REG(PS3_ERR_IJ_WAIT_CMD_ALLOC2, wait_cmd_alloc2)
	INJECT_REG(PS3_ERR_IJ_FORCE_ALLOC_DMA_BUF_FAILED, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_PCIE_ERR_PCI_INIT_FAILED, force_ret_fail)
	INJECT_REG(PS3_ERR_IJ_FORCE_PCIE_ERR_PCI_INIT_COMP_FAILED, force_ret_fail)
	INJECT_REG(PS3_ERR_IJ_PCI_FORCE_HARD_RECOVERY_REQ_FAILED, force_hard_recovery_request_nosupport_fail)
	INJECT_REG(PS3_ERR_IJ_PCI_FORCE_WAIT_OPERARIONAL_FAILED, force_ret_fail)
	INJECT_REG(PS3_ERR_IJ_FORCE_PCIE_ERR_IOC_RUNNING_INIT_FAILED, force_ioc_not_running)
	INJECT_REG(PS3_ERR_IJ_FORCE_STREAM_DETECT_TRUE, force_return_true)
	INJECT_REG(PS3_ERR_IJ_FORCE_SEQ_CMD, force_seq_cmd)
	INJECT_REG(PS3_ERR_IJ_FORCE_RAND_CMD, force_rand_cmd)
	INJECT_REG(PS3_ERR_IJ_FORCE_RET_FAIL1, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_VD_COUNT_ERR, force_vd_count_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_MOD_CHAN, force_dev_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_MOD_CHAN1, force_chan_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_MOD_TARGET, force_target_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_MOD_DISKPOS, force_diskpos_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RET_FAIL2, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_LUN_ERROR,force_lun_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_INSTANCE_NULL, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_VD_ENTRY_NULL, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_VD_ENTRY_NULL1, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_PRIV_DATA_NULL, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_RB_INFO_NULL, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_HASH_MGR_NULL, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_MGR_CONFLICT_NULL, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_SEND_TH_NULL, force_thread_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_RET_FAIL3, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_INSTANCE_NULL1, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_PRIV_DATA_NULL1, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_MOD_DEV_TYPE, force_dev_type_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_VD_ENTRY_NULL2, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_RET_FAIL4, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_INSTANCE_NULL2, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_RET_FAIL5, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_RET_FAIL6, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_MOD_DISKPOS1, force_diskpos_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_MOD_ALIGN, force_align_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_MOD_ALIGN1, force_align_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_MOD_CHAN2, force_target_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_MOD_DISKID, force_target_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_MOD_CHAN3, force_chan_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_MOD_DISKPOS2, force_diskpos_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_PD_ENTRY_NULL, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_PD_ENTRY_NULL1, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_PD_STATE_ERROR, force_pd_state_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_PRIV_DATA_NULL2, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_PD_ENTRY_NULL2, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_0, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_1, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_2, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_3, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_4, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_5, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_6, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_7, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_8, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_9, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_10, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_11, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_12, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_13, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_14, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_15, wait_pcie_err)
	INJECT_REG(PS3_ERR_IJ_FORCE_RET_FAIL7, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_RET_FAIL8, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_DOORBELL_FAILED, force_doorbell_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_INIT_FAIL, force_recovery_init_cmd_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_RECOVERY_OPE_FAIL, force_recovery_operation_cmd_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_DEBUG_INIT_FAIL, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_TRANSPORT_INIT_FAIL, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_CHARDEV_INIT_FAIL, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_PCIDRV_INIT_FAIL, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_VERFILE_INIT_FAIL, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_FORCE_VERFILE2_INIT_FAIL, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_IO_RW_FLAG_SET_UNKOWN, set_cmd_io_rw_flag_unkown)
	INJECT_REG(PS3_ERR_IJ_SOFT_ZONE_TYPE_SET_UNKOWN, set_soft_zone_type_unkown)
	INJECT_REG(PS3_ERR_IJ_WATCHDOG_WAIT_RUNNING, wait_ioc_ready)
	INJECT_REG(PS3_ERR_IJ_WATCHDOG_IRQ_QUEUE, force_work_queue_unull)
	INJECT_REG(PS3_ERR_IJ_WATCHDOG_IRQ_QUEUE_1, force_work_queue_failed)
	INJECT_REG(PS3_ERR_IJ_READ_FW_STATE_REG_FF, force_fw_state_ff)
	INJECT_REG(PS3_ERR_IJ_SET_HIGH_IOPS_CHANNEL_IN_QOS, set_iops_channel)
	INJECT_REG(PS3_ERR_IJ_FORCE_F0_WATCHDOG_START_HARD, force_set_f0_ioc_critical)
	INJECT_REG(PS3_ERR_IJ_ADD_DISK_HOST_RESET, force_stop_aborted_cmd_done)
	INJECT_REG(PS3_ERR_IJ_WAIT_SUSPEND_WAIT_RECOVERY, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_WAIT_SUSPEND_WAIT_RECOVERY_1, force_stop_aborted_cmd_done)
	INJECT_REG(PS3_ERR_IJ_PCIE_FROZEN, force_pcie_frozen)
	INJECT_REG(PS3_ERR_IJ_PEER_PCIE_FROZEN, force_pcie_frozen)
	INJECT_REG(PS3_ERR_IJ_ASYNC_HARDRESET_PROBE, force_async_hard_reset_request)
	INJECT_REG(PS3_ERR_IJ_ASYNC_HARDRESET_REMOVE, force_async_hard_reset_request)
	INJECT_REG(PS3_ERR_IJ_MOD_SO_ADDR, for_mod_so_addr)
	INJECT_REG(PS3_ERR_IJ_SHUTDOWN_HARDRESET, force_hard_reset_request)
	INJECT_REG(PS3_ERR_IJ_CANCEL_WEB_CMD_ALLOC_NULL, force_return_fail)
	INJECT_REG(PS3_ERR_IJ_GET_REPLYQ_COUNT_INVALID, force_fw_state_ff)
	INJECT_REG(PS3_ERR_IJ_GET_MSIX_VEC_COUNT_INVALID, force_s32_zero)
	INJECT_REG(PS3_ERR_IJ_GET_MSI_VEC_COUNT_INVALID, force_s32_letter_zero)
	INJECT_REG(PS3_ERR_IJ_GET_MAX_CMD_COUNT_INVALID, force_fw_state_ff)
	INJECT_REG(PS3_ERR_IJ_ALLOC_LEGACY_VECTOR_FAILED, force_s32_zero)
	INJECT_REG(PS3_ERR_IJ_CPU_MSIX_TABLE_ALLOC_FAILED, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_REPLY_FIFO_DESC_BUF_POOL_ALLOC_FAILED, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_REPLY_FIFO_DESC_BUF_ALLOC_FAILED, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_REPLY_FIFO_POOL_ALLOC_FAILED, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_REPLY_VIRT_BASE_ALLOC_FAILED, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_IRQS_ALLOC_FAILED, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_REPLY_REQ_IRQS_FAILED, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_DUMP_REQ_IRQS_FAILED, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_IRQ_VECTORS_ALLOC_FAILED, force_s32_zero)
	INJECT_REG(PS3_ERR_IJ_SAS_PHY_GET_ERR, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_SAS_PORT_ALLOC_NULL, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_SAS_ALLOC_NUM_NULL, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_SAS_PORT_ADD_FAILED, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_SAS_ALLOC_PHY_NULL, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_SAS_PHY_ADD_FAILED, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_SAS_ADD_ACK_FAILED, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_ALLOC_EXP_NODE_NULL, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_ALLOC_EXP_PHYS_NULL, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_SAS_PORT_ALLOC_NULL1, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_SAS_PHY_GET_ERR1, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_SAS_ALLOC_PHY_NULL1, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_SAS_PHY_ADD_FAILED1, force_ret_failed)
	INJECT_REG(PS3_ERR_IJ_READ_ATU_REG_FAILED, force_reg_fail)
	INJECT_REG(PS3_ERR_IJ_IOC_TO_READY_FAILED, force_ready_failed)
	INJECT_REG(PS3_ERR_IJ_BIT_POS_ERR, force_atu_support_failed)
	INJECT_REG(PS3_ERR_DUMP_ALLOC_FAILED, force_ptr_null)
	INJECT_REG(PS3_ERR_RECOVERY_CONTEXT_ALLOC_FAILED, force_ptr_null)
	INJECT_REG(PS3_ERR_IJ_FORCE_STREAM_DETECT_FALSE, force_return_false)
	INJECT_REG(PS3_ERR_FORCE_SET_CMD_INDEX_NOT_MGR, force_cmd_not_mgr_cmd)
	INJECT_REG(PS3_ERR_EXP_ID_ERR, force_chan_err)
	INJECT_REG(PS3_ERR_HBA_PHY_COUNT_ZERO, force_phy_count_zero)
	INJECT_REG(PS3_ERR_IJ_IOC_TO_READY_FAILED1, force_ready_failed)
	INJECT_REG(PS3_ERR_IJ_REG_READ_ERR, force_reg_fail)
	INJECT_REG(PS3_ERR_IJ_REG_READ_ZERO, force_reg_zero)
	INJECT_REG(PS3_ERR_IJ_QOS_FORCE_TFIFO_ZERO_DEPTH, force_fifo_depth_zero)
	INJECT_REG(PS3_ERR_IJ_FEATURE_REG_READ_ERR, force_reg_fail)
	INJECT_REG(PS3_ERR_IJ_IOC_TO_CRITICAL, force_ioc_critical)
	INJECT_REG(PS3_ERR_IJ_GET_PRIV_DATA_DELAY, NULL)
	INJECT_REG(PS3_ERR_IJ_DEL_DEV_WAIT_OS_PRIV_DATA, NULL)
	INJECT_REG(PS3_ERR_IJ_ADD_DEV_WAIT_OS_PRIV_DATA, NULL)
	INJECT_REG(PS3_ERR_IJ_SGL_ADDR_PAGE_MODE_5, force_change_sgl_addr_to_page_mode_5)
	INJECT_REG(PS3_ERR_IJ_SGL_ADDR_RESTORE, force_sgl_addr_restore)
	INJECT_REG(PS3_ERR_IJ_FORCE_WAIT, NULL)

	LOG_INFO("inject_init success\n");
	ps3_inject_init();
	return;
}
void inject_exit(void)
{
	U32 idx = 0;
	for(idx = PS3_ERR_IJ_WATCHDOG_CONCURY ;idx < PS3_ERR_IJ_MAX_COUNT; idx++)
	{
		memset(&g_ps3_err_scene[idx - 1], 0, sizeof(Ps3Injection_t));
	}
	ps3_inject_exit();
	is_inject_init = PS3_FALSE;
}
#endif

