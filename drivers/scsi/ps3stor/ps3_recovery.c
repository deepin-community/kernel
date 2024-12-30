
#include "ps3_recovery.h"

#ifndef _WINDOWS

#include <scsi/scsi.h>
#endif
#include "ps3_recovery.h"
#include "ps3_ioc_state.h"
#include "ps3_event.h"
#include "ps3_mgr_channel.h"
#include "ps3_cmd_complete.h"
#include "ps3_device_update.h"
#include "ps3_ioc_manager.h"
#include "ps3_mgr_cmd.h"
#include "ps3_scsi_cmd_err.h"
#include "ps3_scsih.h"
#include "ps3_cmd_statistics.h"
#include "ps3_r1x_write_lock.h"
#include "ps3_ioc_state.h"
#include "ps3_module_para.h"
#include "ps3_err_inject.h"
#include "ps3_ioctl.h"
#include "ps3_dump.h"
#include "ps3_cli_debug.h"
S32 ps3_recovery_cancel_work_sync(struct ps3_instance *instance);
#ifndef _WINDOWS
static void ps3_recovery_work(struct work_struct* work);
#else
static void ps3_recovery_work(void *work);
#endif
static S32 ps3_hard_recovery_handle(struct ps3_instance *instance);
static S32 ps3_soft_recovery_handle(struct ps3_instance *instance);
static S32 ps3_recovery_ready_to_force_cmd_stop(struct ps3_instance *instance);
static S32 ps3_recovery_ready_to_running(struct ps3_instance *instance);
static void ps3_can_queue_reset(struct ps3_instance *instance, U32 cur_max_fw_cmds);
static void ps3_cmd_force_stop(struct ps3_instance *instance);
static void ps3_cmd_reset_flag_set(struct ps3_instance *instance, U8 reset_flag);
static S32 ps3_soft_recovery_fail_to_hard_recovery(struct ps3_instance *instance);

enum {
	PS3_RECOVERY_INTER_ERR_SUCCESS = 0,
	PS3_RECOVERY_INTER_ERR_INTERRUPT = 1, 
	PS3_RECOVERY_INTER_ERR_FAILED = 2, 
	PS3_RECOVERY_INTER_ERR_NEED_RECOVERY = 3, 
	PS3_RECOVERY_INTER_ERR_PCIE_ERR = 4, 
	PS3_RECOVERY_INTER_ERR_SUSPEND_RESUME = 5, 
};

static inline void ps3_wait_watchdog_dect_recovery(
	struct ps3_instance *instance)
{
	U16 wait_step = 100;
	while(!instance->watchdog_context.is_halt &&
		instance->watchdog_context.watchdog_queue != NULL) {
		ps3_msleep(wait_step);
	}
}

Bool ps3_no_need_recovery(struct ps3_instance *instance)
{
	Bool ret = PS3_FALSE;
	if ((!instance->state_machine.is_load) &&
			!instance->is_host_added) {
		LOG_ERROR("hno:%u  driver is load[%d] or suspend or host not add[%d]!\n",
			PS3_HOST(instance), instance->state_machine.is_load,
			instance->is_host_added);
		ret = PS3_TRUE;
	}

	return ret;
}

struct ps3_recovery_context *ps3_recovery_context_alloc(void)
{
	struct ps3_recovery_context *context;

	context = (struct ps3_recovery_context *)kzalloc(sizeof(struct ps3_recovery_context), GFP_KERNEL);
	if (!context)
		return NULL;

	return context;
}
void ps3_recovery_context_free(struct ps3_recovery_context *context)
{
	if (context != NULL) {
		kfree(context);
	}
	return;
}

void ps3_recovery_context_delete(struct ps3_recovery_context *context)
{
	if (context != NULL) {
		if (context->recovery_wq) {
			destroy_workqueue(context->recovery_wq);
			context->recovery_wq = NULL;
		}
		kfree(context);
		context = NULL;
	}
	return;
}

#ifndef _WINDOWS
static void ps3_recovery_irq_service(struct work_struct* work)
{
	struct ps3_instance *instance =
		ps3_container_of(work, struct ps3_instance,
			recovery_irq_work);
	S32 ret = PS3_SUCCESS;
	U32 count = 0;
	LOG_INFO("hno:%u recovery irq triggered\n",
		PS3_HOST(instance));

	if ((instance->recovery_context->heartbeat_recovery == PS3_HEARTBEAT_HARDRESET_DECIDE ||
		instance->recovery_context->heartbeat_recovery == PS3_HEARTBEAT_HARDRESET_RECOVERY)
		&&(instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_SHALLOW ||
		instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE)) {
		goto l_out;
	}

	ps3_mutex_lock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
	if (instance->recovery_context->heartbeat_recovery == PS3_HEARTBEAT_NULL) {
		instance->recovery_context->heartbeat_recovery =
			PS3_HEARTBEAT_HARDRESET_DECIDE;
	}
	ps3_mutex_unlock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
	while ((ret = ps3_hard_recovery_request(instance)) == -PS3_RETRY) {
		if (count++ == ps3_use_hard_reset_max_retry()) {
			ret = -PS3_FAILED;
			break;
		}
		ps3_mdelay(10);
	}
	if (ret == PS3_SUCCESS) {
		LOG_INFO("hno[%u], recovery success!\n",
			PS3_HOST(instance));
	}else{
		LOG_ERROR("hno[%u], recovery request NOK, %s!\n",
			PS3_HOST(instance),
			namePS3InstanceState(ps3_atomic_read(&instance->state_machine.state)));
	}
l_out:
	return;
}

S32 ps3_recovery_irq_start(struct ps3_instance *instance)
{
	S8 request_irq_queue_name[PS3_RECOVERY_IRQ_NAME_MAX_LENTH];
	struct work_struct* recovery_irq_work = &instance->recovery_irq_work;

	INJECT_START(PS3_ERR_IJ_WATCHDOG_IRQ_QUEUE, instance)
	if (instance->recovery_irq_queue != NULL) {
		LOG_DEBUG("hno:%u  watchdog for is already started!\n",
			PS3_HOST(instance));
		goto l_out;
	}

	memset(request_irq_queue_name, 0, PS3_RECOVERY_IRQ_NAME_MAX_LENTH);
	INIT_WORK(recovery_irq_work, ps3_recovery_irq_service);

	snprintf(request_irq_queue_name, PS3_RECOVERY_IRQ_NAME_MAX_LENTH,
		"ps3_irq_recovery_start_service_host%d", instance->host->host_no);

	instance->recovery_irq_queue =
		create_singlethread_workqueue(request_irq_queue_name);
	if (instance->recovery_irq_queue == NULL) {
		LOG_ERROR("hno:%u  watchdog work queue create failed\n",
			PS3_HOST(instance));
		return -PS3_FAILED;
	}
	instance->recovery_irq_enable = PS3_TRUE;
l_out:
	return PS3_SUCCESS;
}

irqreturn_t ps3_recovery_irq_handler(S32 virq, void *dev_id)
{
	struct ps3_irq_recovery *irq = (struct ps3_irq_recovery *)dev_id;
	struct ps3_instance *instance = irq->instance;

	if (instance->ioc_adpter->ioc_heartbeat_detect == NULL) {
		goto l_out;
	}

	if (instance->ioc_adpter->ioc_heartbeat_detect(instance) == PS3_TRUE
			&& !ps3_pci_err_recovery_get(instance)
			&& instance->recovery_irq_enable) {


		LOG_DEBUG("hno:%u  recovery irq recieved, virq: %d, dev_id: 0x%llx\n",
			PS3_HOST(instance), virq, (U64)dev_id);
		if (!work_busy(&instance->recovery_irq_work)) {
			queue_work(instance->recovery_irq_queue, &instance->recovery_irq_work);
		}
	}
l_out:
	return IRQ_HANDLED;
}

void ps3_recovery_irq_queue_destory(struct ps3_instance *instance)
{
	if (instance->recovery_irq_queue) {
		flush_workqueue(instance->recovery_irq_queue);
		destroy_workqueue(instance->recovery_irq_queue);
		instance->recovery_irq_queue = NULL;
	}
	return;
}

void ps3_recovery_work_queue_destory(struct ps3_instance *instance)
{
	struct ps3_recovery_context *context = instance->recovery_context;
	struct workqueue_struct* wq = context->recovery_wq;

	if (!ps3_is_latest_func(instance)) {
		 LOG_DEBUG("hno:%u  not latest func\n",
			 PS3_HOST(instance));
		 goto l_out;
	}

	context->recovery_wq = NULL;
	if (wq == NULL) {
		return;
	}

	flush_workqueue(wq);
	destroy_workqueue(wq);
l_out:
	return;
}
#endif
void ps3_recovery_function_init(struct ps3_instance *instance)
{
	instance->recovery_function.halt_handle_cb = NULL;
	instance->recovery_function.hardreset_handle_init_running_cb = ps3_hardreset_handle_init_running;
	instance->recovery_function.hardreset_handle_post_cb = ps3_hardreset_handle_post;
	instance->recovery_function.hardreset_handle_pre_cb = ps3_hardreset_handle_pre;
	instance->recovery_function.hardreset_handle_wait_ready_cb = ps3_hardreset_handle_wait_ready;
	instance->recovery_function.hardreset_handle_finish_cb = ps3_hardreset_handle_finish;
	instance->recovery_function.hardreset_handle_offline_cb = ps3_hardreset_handle_offline;
	instance->recovery_function.recovery_handle_cb = NULL;
	instance->recovery_function.softreset_handle_post_cb = ps3_softreset_handle_post;
	instance->recovery_function.softreset_handle_pre_cb = ps3_softreset_handle_pre;

	return;
}

S32 ps3_recovery_context_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_recovery_context *context = NULL;

	ps3_recovery_function_init(instance);

	if (instance->peer_instance != NULL) {
		instance->recovery_context = instance->peer_instance->recovery_context;
		goto l_recovery_irq_init;
	}

	context = ps3_recovery_context_alloc();
	INJECT_START(PS3_ERR_RECOVERY_CONTEXT_ALLOC_FAILED, &context);
	if (context == NULL) {
		LOG_ERROR("hno:%u, alloc recovery context failed !\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	LOG_DEBUG("recovery context add!\n");
	snprintf(context->recovery_wq_name, 20, "ps3_rec_th_%d",
		instance->host->host_no);
	context->recovery_wq =
		create_singlethread_workqueue(context->recovery_wq_name);
	if (context->recovery_wq == NULL) {
		ret = -PS3_FAILED;
		goto l_out;
	}
	INIT_WORK(&context->recovery_work, ps3_recovery_work);

	ps3_mutex_init(&context->ps3_watchdog_recovery_mutex);
	ps3_mutex_init(&context->free_cmd_lock);
	ps3_spin_lock_init(&context->recovery_lock);
	ps3_spin_lock_init(&context->ps3_hardreset_lock);
	context->parall_hardreset_state = PS3_PARALLEL_HARDRESET_STATE_INIT;
	ps3_atomic_set(&context->hardreset_ref, 0);
	context->recovery_state = PS3_SOFT_RECOVERY_PROBE_PROCESS;
	instance->recovery_context = context;
	instance->recovery_context->hardreset_count = 0;
	context->host_reset_state = PS3_HOST_RESET_INIT;
	context->instance_change = 0;

l_recovery_irq_init:
	if (ps3_recovery_irq_start(instance) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u recovery irq NOK\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_recovery_start_failed;
	}

	return ret;
l_recovery_start_failed:
	ps3_recovery_irq_queue_destory(instance);
	ps3_mutex_destroy(&context->ps3_watchdog_recovery_mutex);
	ps3_mutex_destroy(&context->free_cmd_lock);
	instance->recovery_context = NULL;
l_out:
	ps3_recovery_context_delete(context);
	return ret;
}

void ps3_recovery_clean(struct ps3_instance *instance)
{
	struct ps3_recovery_context *context = instance->recovery_context;
	if(context == NULL) {
		goto l_out;
	}

	if (!ps3_is_latest_func(instance)) {
		instance->recovery_context = NULL;
		goto l_out;
	}

	ps3_mutex_destroy(&context->ps3_watchdog_recovery_mutex);
	ps3_mutex_destroy(&context->free_cmd_lock);
	ps3_recovery_context_free(instance->recovery_context);
	instance->recovery_context = NULL;
l_out:
	return ;
}

void ps3_recovery_destory(struct ps3_instance *instance)
{
#ifndef _WINDOWS
	ps3_recovery_cancel_work_sync(instance);
	ps3_recovery_work_queue_destory(instance);
#else
	struct ps3_recovery_context *context = instance->recovery_context;
	ps3_worker_exit(&context->recovery_work);
#endif
	return ;
}

void ps3_recovery_context_exit(struct ps3_instance *instance)
{
	ps3_recovery_irq_queue_destory(instance);

	if (instance->peer_instance == NULL &&
		instance->recovery_context != NULL) {
		ps3_recovery_destory(instance);
		ps3_recovery_clean(instance);
	}

	return ;
}
S32 ps3_recovery_state_transfer(struct ps3_instance *instance,
	U32 dest_state)
{
	U32 recovery_origin_state;

	ps3_mutex_lock(&instance->state_machine.lock);
	recovery_origin_state = instance->recovery_context->recovery_state;
	if (recovery_origin_state == dest_state) {
		goto l_fail;
	}

	switch(recovery_origin_state) {
	case PS3_SOFT_RECOVERY_PROBE_PROCESS:
		if (dest_state == PS3_SOFT_RECOVERY_SHALLOW ||
			dest_state == PS3_SOFT_RECOVERY_DEEP ||
			dest_state == PS3_SOFT_RECOVERY_IOC_RECOVERY ||
			dest_state == PS3_HARD_RECOVERY_DECIDE) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_SOFT_RECOVERY_SHALLOW:
	case PS3_SOFT_RECOVERY_DEEP:
	case PS3_SOFT_RECOVERY_IOC_RECOVERY:
		if ((dest_state == PS3_HARD_RECOVERY_DECIDE) ||
			(dest_state == PS3_HARD_RECOVERY_SHALLOW)||
			(dest_state == PS3_SOFT_RECOVERY_FINISH)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_SOFT_RECOVERY_FINISH:
		if ((dest_state == PS3_HARD_RECOVERY_DECIDE) ||
			(dest_state == PS3_SOFT_RECOVERY_SHALLOW) ||
			(dest_state == PS3_SOFT_RECOVERY_DEEP) ||
			(dest_state == PS3_SOFT_RECOVERY_IOC_RECOVERY) ||
			(dest_state == PS3_HARD_RECOVERY_SHALLOW)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_HARD_RECOVERY_DECIDE:
		if (dest_state == PS3_HARD_RECOVERY_SHALLOW ||
			dest_state == PS3_HARD_RECOVERY_FINISH) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_HARD_RECOVERY_FINISH:
		if ((dest_state == PS3_HARD_RECOVERY_DECIDE) ||
			(dest_state == PS3_SOFT_RECOVERY_SHALLOW) ||
			(dest_state == PS3_SOFT_RECOVERY_DEEP) ||
			(dest_state == PS3_SOFT_RECOVERY_IOC_RECOVERY) ||
			(dest_state == PS3_HARD_RECOVERY_SHALLOW)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_HARD_RECOVERY_SHALLOW:
		if (dest_state == PS3_HARD_RECOVERY_FINISH) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	default:
		goto l_fail;
	}

l_fail:
	ps3_mutex_unlock(&instance->state_machine.lock);
	LOG_ERROR("hno:%u  recovery state transfer NOK!"
		"[cur_state:%d][dest_state:%d]\n",
		PS3_HOST(instance),
		recovery_origin_state,dest_state);
	return -PS3_FAILED;
l_success:
	instance->recovery_context->recovery_state = dest_state;
	ps3_mutex_unlock(&instance->state_machine.lock);
	LOG_FILE_WARN("hno:%u  recovery state transfer from %d to %d!\n",
		PS3_HOST(instance),
		recovery_origin_state,dest_state);

	return PS3_SUCCESS;
}

#ifndef _WINDOWS

static S32 ps3_recovery_work_start(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_recovery_context *context = instance->recovery_context;

	if (unlikely(context->recovery_wq == NULL)) {
		ret = -PS3_FAILED;
		goto l_out;
	}
	context->work_instance = instance;

	queue_work(context->recovery_wq, &context->recovery_work);

l_out:
	return ret;
}
 static inline void ps3_wait_hard_reset_finish(struct ps3_instance *instance)
{
	while(ps3_atomic_read(&instance->recovery_context->hardreset_ref) != 0){
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
	}

	if (instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE) {
		do{
			ps3_msleep(100);

			if (likely(instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE)) {
				continue;
			} else {
				cancel_work_sync(&instance->recovery_context->recovery_work);
				break;
			}
		}while(1);
	} else {
		cancel_work_sync(&instance->recovery_context->recovery_work);
	}
}
S32 ps3_recovery_cancel_work_sync(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_recovery_context *context = instance->recovery_context;
	if (unlikely(context->recovery_wq == NULL)) {
		ret = -PS3_FAILED;
		goto l_out;
	}

	ps3_wait_hard_reset_finish(instance);

l_out:
	return ret;
}

static S32 ps3_recovery_start(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	ps3_mutex_lock(&instance->state_machine.lock);
	if(instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_SHALLOW){
		LOG_ERROR("hno:%u  hard recovery work proc,exit!\n",
				PS3_HOST(instance));
		ps3_mutex_unlock(&instance->state_machine.lock);
		goto l_out;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);

	if (unlikely(instance->recovery_context->recovery_wq == NULL)) {
		ret = -PS3_FAILED;
		LOG_ERROR("hno:%u  recovery work sync NOK!\n",
			PS3_HOST(instance));
		goto l_out;
	}

	cancel_work_sync(&instance->recovery_context->recovery_work);

	ps3_mutex_lock(&instance->state_machine.lock);
	if(instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE) {
		if(ps3_instance_no_lock_state_transfer(instance, PS3_INSTANCE_STATE_RECOVERY) != PS3_SUCCESS) {
			LOG_ERROR("hno:%u  transfer to PS3_INSTANCE_STATE_RECOVERY NOK!\n",
				PS3_HOST(instance));
			ret = -PS3_FAILED;
			ps3_mutex_unlock(&instance->state_machine.lock);
			goto l_out;
		}
	}
	ps3_mutex_unlock(&instance->state_machine.lock);

	if (instance->peer_instance != NULL &&
		instance->peer_instance->recovery_context != NULL) {
		ps3_mutex_lock(&instance->peer_instance->state_machine.lock);
		if(instance->peer_instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE) {
			if(ps3_instance_no_lock_state_transfer(instance->peer_instance, PS3_INSTANCE_STATE_RECOVERY) != PS3_SUCCESS) {
				LOG_ERROR("hno:%u  transfer to PS3_INSTANCE_STATE_RECOVERY NOK!\n",
					PS3_HOST(instance->peer_instance));
				ret = -PS3_FAILED;
				ps3_mutex_unlock(&instance->peer_instance->state_machine.lock);
				goto l_out;
			}
		}
		ps3_mutex_unlock(&instance->peer_instance->state_machine.lock);
	}

	ret = ps3_recovery_work_start(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  recovery work start NOK!\n",
			PS3_HOST(instance));
		goto l_out;
	}
l_out:
	return ret;
}
#else
static S32 ps3_recovery_start(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_recovery_context* context = instance->recovery_context;
	if (ps3_worker_start(&context->recovery_work) != PS3_SUCCESS) {
		LOG_ERROR("trace_id[0x%llx], hno:%u recovery start worker failed\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}
l_out:
	return ret;
}
#endif

static Bool ps3_recovery_is_state_halt(struct ps3_instance *instance)
{
	Bool ret = PS3_FALSE;
	struct ps3_instance_state_machine *state_machine
		= &instance->state_machine;
	U32 cur_state = 0;
	U32 ioc_state = instance->ioc_adpter->ioc_state_get(instance);

	cur_state = ps3_atomic_read(&state_machine->state);
	if (cur_state == PS3_INSTANCE_STATE_DEAD) {
		LOG2_INFO_LIM("hno:%u  ioc_state:%s, "
			"drv_state:PS3_INSTANCE_STATE_DEAD, stop recovery!\n",
			PS3_HOST(instance),
			ps3_ioc_state_print(ioc_state));
		ret = PS3_TRUE;
		goto l_out;
	}

	if (ioc_state == PS3_FW_STATE_HALT &&
			instance->recovery_context->heartbeat_recovery != PS3_HEARTBEAT_HARDRESET_DECIDE) {
		ps3_atomic_set(&state_machine->state, PS3_INSTANCE_STATE_DEAD);
		LOG2_INFO_LIM("hno:%u  ioc_state:PS3_FW_STATE_HALT, stop recovery!\n",
			PS3_HOST(instance));
		while(1) {
			ps3_msleep(10);
		}
		ret = PS3_TRUE;
		goto l_out;
	}

	if (PS3_IOC_STATE_HALT_SUPPORT(instance) &&
			instance->recovery_context->heartbeat_recovery != PS3_HEARTBEAT_HARDRESET_DECIDE) {
		instance->ioc_adpter->ioc_force_to_halt(instance);
		ps3_atomic_set(&state_machine->state, PS3_INSTANCE_STATE_DEAD);
		LOG_WARN("hno:%u  IOC support HALT, enter HALT!\n",
			PS3_HOST(instance));
		while(1) {
			ps3_msleep(10);
		}
		ret = PS3_TRUE;
	}
l_out:

	return ret;
}

static inline S32 ps3_hard_recovery_request_decide(
	struct ps3_instance *instance, U32 cur_state, U32 peer_cur_state)
{
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;

	if (cur_state == PS3_INSTANCE_STATE_PRE_OPERATIONAL)  {
		if ( instance->recovery_context->recovery_state ==
			PS3_SOFT_RECOVERY_PROBE_PROCESS ) {
			LOG_INFO("hno:%u  instance state during probe process!\n",
				PS3_HOST(instance));
			ret = PS3_RECOVERY_INTER_ERR_FAILED;
			goto l_out;
		}
	}

	if (instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_SHALLOW ||
		instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE) {
		LOG_INFO("hno:%u  instance state during hard recovery!\n",
			PS3_HOST(instance));
		ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
		goto l_out;
	}

	if ((cur_state == PS3_INSTANCE_STATE_OPERATIONAL) ||
		(cur_state == PS3_INSTANCE_STATE_SOFT_RECOVERY) ||
		(cur_state == PS3_INSTANCE_STATE_PRE_OPERATIONAL) ||
		(cur_state == PS3_INSTANCE_STATE_DEAD) ||
		(cur_state == PS3_INSTANCE_STATE_PCIE_RECOVERY) ||
		(cur_state == PS3_INSTANCE_STATE_INIT)) {
		if (instance->is_need_event) {
			if(instance->event_context.event_abort_cmd != NULL){
				complete(&instance->event_context.event_abort_cmd->sync_done);
			} else if (instance->dev_context.vdpending_abort_cmd != NULL){
				complete(&instance->dev_context.vdpending_abort_cmd->sync_done);
			} else if (instance->webSubscribe_context.web_abort_cmd != NULL){
                complete(&instance->webSubscribe_context.web_abort_cmd->sync_done);
            }
		}

		if (instance->peer_instance == NULL) {
			instance->recovery_context->recovery_state = PS3_HARD_RECOVERY_DECIDE;
			LOG_INFO("hno:%u  instance state to recovery!\n",
				PS3_HOST(instance));
			ret = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
		}
	} else {
		LOG_INFO("hno:%u  instance state during hard recovery!\n",
			PS3_HOST(instance));
		ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
		goto l_out;
	}

	if (instance->peer_instance != NULL &&
		instance->peer_instance->recovery_context != NULL) {
		if (peer_cur_state == PS3_INSTANCE_STATE_PRE_OPERATIONAL)  {
			if ( instance->peer_instance->recovery_context->recovery_state ==
				PS3_SOFT_RECOVERY_PROBE_PROCESS ) {
				LOG_INFO("hno:%u  instance state during probe process!\n",
					PS3_HOST(instance->peer_instance));
				ret = PS3_RECOVERY_INTER_ERR_FAILED;
				goto l_out;
			}
		}

		if (instance->peer_instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_SHALLOW ||
			instance->peer_instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE) {
			LOG_INFO("hno:%u  instance state during hard recovery!\n",
				PS3_HOST(instance->peer_instance));
			ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
			goto l_out;
		}

		if ((peer_cur_state == PS3_INSTANCE_STATE_OPERATIONAL) ||
			(peer_cur_state == PS3_INSTANCE_STATE_SOFT_RECOVERY) ||
			(peer_cur_state == PS3_INSTANCE_STATE_PRE_OPERATIONAL) ||
			(peer_cur_state == PS3_INSTANCE_STATE_DEAD) ||
			(peer_cur_state == PS3_INSTANCE_STATE_PCIE_RECOVERY) ||
			(peer_cur_state == PS3_INSTANCE_STATE_INIT)) {
			if (instance->peer_instance->is_need_event) {
				if(instance->peer_instance->event_context.event_abort_cmd != NULL){
					complete(&instance->peer_instance->event_context.event_abort_cmd->sync_done);
				} else if (instance->peer_instance->dev_context.vdpending_abort_cmd != NULL){
					complete(&instance->peer_instance->dev_context.vdpending_abort_cmd->sync_done);
				} else if (instance->webSubscribe_context.web_abort_cmd != NULL){
                    complete(&instance->webSubscribe_context.web_abort_cmd->sync_done);
                }
			}

			instance->peer_instance->recovery_context->recovery_state = PS3_HARD_RECOVERY_DECIDE;

			LOG_INFO("hno:%u  instance state to recovery!\n",
				PS3_HOST(instance->peer_instance));
			ret = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
		} else {
			LOG_INFO("hno:%u  instance state during hard recovery!\n",
				PS3_HOST(instance->peer_instance));
			ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
		}
	}
l_out:
	return ret;
}

static S32 ps3_hard_recovery_request_prepare(struct ps3_instance *instance)
{
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
	U32 ioc_state = 0;
	struct ps3_instance_state_machine *state_machine
		= &instance->state_machine;
	U32 cur_state = 0;
	struct ps3_instance_state_machine *state_machine_peer = NULL;
	U32 peer_cur_state = PS3_INSTANCE_STATE_OPERATIONAL;
	INJECT_START(PS3_ERR_IJ_PCIE_FROZEN, instance)
	if (instance->reg_set == NULL) {
		return ret;
	}
	ioc_state = instance->ioc_adpter->ioc_state_get(instance);
	ps3_mutex_lock(&state_machine->lock);
	cur_state = ps3_atomic_read(&state_machine->state);

	LOG_INFO("hno:%u  hard recovery request:%s, %s\n",
		PS3_HOST(instance),
		ps3_ioc_state_print(ioc_state), namePS3InstanceState(cur_state));

	if (instance->peer_instance != NULL) {
		state_machine_peer = &instance->peer_instance->state_machine;
		INJECT_START(PS3_ERR_IJ_PEER_PCIE_FROZEN, instance->peer_instance)
		if (instance->peer_instance->reg_set == NULL) {
			goto l_clean;
		}
		ioc_state = instance->peer_instance->ioc_adpter->ioc_state_get(instance->peer_instance);
		ps3_mutex_lock(&state_machine_peer->lock);
		peer_cur_state = ps3_atomic_read(&state_machine_peer->state);

		LOG_INFO("hno:%u  hard recovery request:%s, %s\n",
			PS3_HOST(instance->peer_instance),
			ps3_ioc_state_print(ioc_state), namePS3InstanceState(peer_cur_state));
	}

	if (instance->recovery_context->heartbeat_recovery == PS3_HEARTBEAT_HARDRESET_DECIDE
		|| instance->recovery_context->heartbeat_recovery == PS3_HEARTBEAT_HARDRESET_RECOVERY) {
		ret = ps3_hard_recovery_request_decide(instance, cur_state, peer_cur_state);
		if (ret == PS3_RECOVERY_INTER_ERR_NEED_RECOVERY) {
			LOG_INFO("hno:%u heartbeat hard recovery start\n",
				PS3_HOST(instance));
			ps3_mutex_lock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
			instance->recovery_context->heartbeat_recovery = PS3_HEARTBEAT_HARDRESET_RECOVERY;
			ps3_mutex_unlock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
		}
		goto l_out;
	}

	ret = ps3_hard_recovery_request_decide(instance, cur_state, peer_cur_state);

l_out:
	if (instance->peer_instance != NULL) {
		ps3_mutex_unlock(&state_machine_peer->lock);
	}
l_clean:
	ps3_mutex_unlock(&state_machine->lock);
	return ret;
}

S32 ps3_hard_recovery_request(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_instance *instance_recovery = NULL;
	ps3_atomic_inc(&instance->recovery_context->hardreset_ref);
	mb();
	if (instance->recovery_context->instance_change) {
		LOG_INFO("hno:%u peer instance is change\n", PS3_HOST(instance));
		ret = -PS3_RETRY;
		goto l_out;
	}

	INJECT_START(PS3_ERR_IJ_RECOVERY_REQ_WAIT_FUNC1_PROBE, instance)
	INJECT_START(PS3_ERR_IJ_RECOVERY_REQ_WAIT_FUNC1_REMOVE, instance)
	while (instance->peer_instance != NULL &&
		PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance->peer_instance)) {
		if (instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_SHALLOW || \
			instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE) {
			instance->recovery_context->parall_hardreset_state = PS3_PARALLEL_HARDRESET_STATE_INIT;
			goto l_out;
		}
		if (instance->recovery_context->instance_change) {
			LOG_INFO("hno:%u peer instance is change\n", PS3_HOST(instance));
			ret = -PS3_RETRY;
			goto l_out;
		}

		if (instance->recovery_context->parall_hardreset_state == PS3_PARALLEL_HARDRESET_STATE_INIT) {
			instance->recovery_context->parall_hardreset_state = PS3_PARALLEL_HARDRESET_STATE_PENDING;
		}

		ps3_atomic_dec(&instance->recovery_context->hardreset_ref);
		ps3_msleep(10);
		if (instance->recovery_context->parall_hardreset_state == PS3_PARALLEL_HARDRESET_STATE_CONTINUE) {
			ps3_atomic_inc(&instance->recovery_context->hardreset_ref);
			mb();
			instance->recovery_context->parall_hardreset_state = PS3_PARALLEL_HARDRESET_STATE_INIT;
			break;
		}
		ps3_atomic_inc(&instance->recovery_context->hardreset_ref);
		mb();
	}

	if (instance->recovery_context->parall_hardreset_state == PS3_PARALLEL_HARDRESET_STATE_PENDING) {
		instance->recovery_context->parall_hardreset_state = PS3_PARALLEL_HARDRESET_STATE_INIT;
	}
	LOG_INFO("hno:%u peer_instance[%p], hard recovery request Function[%d]\n",
		PS3_HOST(instance), instance->peer_instance, ps3_get_pci_function(instance->pdev));
	if (ps3_get_pci_function(instance->pdev) == PS3_FUNC_ID_0 ||
		(ps3_get_pci_function(instance->pdev) == PS3_FUNC_ID_1 && instance->peer_instance == NULL)) {
		instance_recovery = instance;
	} else {
		instance_recovery = instance->peer_instance;
	}

	if (ps3_recovery_is_state_halt(instance_recovery)) {
		LOG_WARN("hno:%u  driver_state:DEAD or HALT now !!!\n",
			PS3_HOST(instance_recovery));
		ret = -PS3_FAILED;
		goto l_out;
	}

	if ((!PS3_IOC_HARD_RECOVERY_SUPPORT(instance_recovery)) || \
		 (!ps3_hard_reset_enable_query())) {
		LOG_ERROR("hno:%u  soc feature unsupport Hard reset[%d] or unable[%d]!!\n",
			PS3_HOST(instance_recovery),
			PS3_IOC_HARD_RECOVERY_SUPPORT(instance_recovery),
			ps3_hard_reset_enable_query());
		ret = -PS3_FAILED;
		goto l_out;
	}

	ret = ps3_hard_recovery_request_prepare(instance_recovery);
	if (ret == PS3_RECOVERY_INTER_ERR_SUCCESS) {
		ret = PS3_SUCCESS;
	} else if(ret == PS3_RECOVERY_INTER_ERR_NEED_RECOVERY) {
		ret = ps3_recovery_start(instance_recovery);
	} else {
		ret = -PS3_FAILED;
	}
	INJECT_START(PS3_ERR_IJ_WATCHDOG_IRQ_QUEUE_1, &ret)
l_out:
	LOG_INFO("hno:%u  hard recovery request:%d!\n",
		PS3_HOST(instance), ret);

	ps3_atomic_dec(&instance->recovery_context->hardreset_ref);
	return ret;
}

static inline S32 ps3_ioc_soft_recovery_request_decide(
	struct ps3_instance *instance, U32 ioc_state)
{
	U32 ioc_recovery_count = 0;
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
	struct ps3_instance_state_machine *state_machine
		= &instance->state_machine;
	S32 recovery_state = 0;

	recovery_state = instance->recovery_context->recovery_state;
	if(recovery_state == PS3_HARD_RECOVERY_SHALLOW||
		recovery_state == PS3_HARD_RECOVERY_DECIDE){
		LOG_INFO("hno:%u  instance state %s,recovery_state %d,hard reset doing, IOC soft return!\n",
			PS3_HOST(instance),
			namePS3InstanceState(ps3_atomic_read(&state_machine->state)),
			recovery_state);
		ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
		goto l_out;
	}else if(recovery_state == PS3_SOFT_RECOVERY_SHALLOW ||
		recovery_state == PS3_SOFT_RECOVERY_DEEP ||
		recovery_state == PS3_SOFT_RECOVERY_IOC_RECOVERY){
		LOG_INFO("hno:%u  instance state %s,recovery_state %d,repeat request!\n",
			PS3_HOST(instance),
			namePS3InstanceState(ps3_atomic_read(&state_machine->state)),
			recovery_state);
		goto l_out;
	}
	if (!ps3_ioc_recovery_count_get(instance, &ioc_recovery_count)) {
		LOG_INFO("hno:%u  entry IOC soft recovery request decide!,ioc_state:%s,"
			" save_recovery_count:%d, ioc_recovery_count:%d\n",
			PS3_HOST(instance),
			ps3_ioc_state_print(ioc_state),
			instance->recovery_context->ioc_recovery_count, ioc_recovery_count);
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}

	LOG_INFO("hno:%u  entry IOC soft recovery request decide!,ioc_state:%s,"
		" save_recovery_count:%d, ioc_recovery_count:%d\n",
		PS3_HOST(instance),
		ps3_ioc_state_print(ioc_state),
		instance->recovery_context->ioc_recovery_count, ioc_recovery_count);

	if (instance->recovery_context->ioc_recovery_count == ioc_recovery_count) {
		goto l_out;
	}

	if(ps3_instance_no_lock_state_transfer(instance, PS3_INSTANCE_STATE_SOFT_RECOVERY) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  transfer to ready failed!\n",
			PS3_HOST(instance));
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}

	if (instance->peer_instance != NULL) {
		if(ps3_instance_no_lock_state_transfer(instance->peer_instance, PS3_INSTANCE_STATE_SOFT_RECOVERY) != PS3_SUCCESS) {
			LOG_ERROR("hno:%u  transfer to ready failed!\n",
				PS3_HOST(instance->peer_instance));
			ret = PS3_RECOVERY_INTER_ERR_FAILED;
			goto l_out;
		}
	}
	instance->recovery_context->recovery_state
		= PS3_SOFT_RECOVERY_IOC_RECOVERY;

	ret = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
l_out:
	return ret;
}

static inline S32 ps3_soft_recovery_request_decide(
	struct ps3_instance *instance, U32 ioc_state)
{
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
	struct ps3_instance_state_machine *state_machine
		= &instance->state_machine;
	S32 recovery_state = instance->recovery_context->recovery_state;
	if(	recovery_state == PS3_HARD_RECOVERY_SHALLOW ||
			recovery_state == PS3_HARD_RECOVERY_DECIDE){
		LOG_INFO("hno:%u  instance state %s,recovery_state %d,repeat request!\n",
			PS3_HOST(instance),
			namePS3InstanceState(ps3_atomic_read(&state_machine->state)),
			recovery_state);
		ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
		goto l_out;
	}else if(recovery_state == PS3_SOFT_RECOVERY_SHALLOW ||
		recovery_state == PS3_SOFT_RECOVERY_DEEP ||
		recovery_state == PS3_SOFT_RECOVERY_IOC_RECOVERY){
		LOG_INFO("hno:%u  instance state %s,recovery_state %d,repeat request!\n",
			PS3_HOST(instance),
			namePS3InstanceState(ps3_atomic_read(&state_machine->state)),
			recovery_state);
		goto l_out;
	}

	if (ioc_state == PS3_FW_STATE_RUNNING) {
		if(ps3_instance_no_lock_state_transfer(instance, PS3_INSTANCE_STATE_SOFT_RECOVERY) != PS3_SUCCESS) {
			LOG_ERROR("hno:%u  transfer to ready failed!\n",
				PS3_HOST(instance));
			ret = PS3_RECOVERY_INTER_ERR_FAILED;
			goto l_out;
		}

		if (instance->peer_instance != NULL) {
			if(ps3_instance_no_lock_state_transfer(instance->peer_instance, PS3_INSTANCE_STATE_SOFT_RECOVERY) != PS3_SUCCESS) {
				LOG_ERROR("hno:%u  transfer to ready failed!\n",
					PS3_HOST(instance->peer_instance));
				ret = PS3_RECOVERY_INTER_ERR_FAILED;
				goto l_out;
			}
		}
		instance->recovery_context->recovery_state
			= PS3_SOFT_RECOVERY_SHALLOW;

		LOG_INFO("hno:%u  instance state to soft recovery shallow!\n",
			PS3_HOST(instance));

		ret = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
	}
l_out:
	return ret;
}

static S32 ps3_recovery_request_prepare(struct ps3_instance *instance)
{
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
	U32 ioc_state = 0;
	struct ps3_instance_state_machine *state_machine
		= &instance->state_machine;
	U32 cur_state = 0;
	struct ps3_instance_state_machine *state_machine_peer = NULL;
	U32 peer_cur_state = PS3_INSTANCE_STATE_OPERATIONAL;

	ps3_mutex_lock(&state_machine->lock);
	ioc_state = instance->ioc_adpter->ioc_state_get(instance);
	cur_state = ps3_atomic_read(&state_machine->state);

	LOG_INFO("hno:%u  recovery request:%s, %s\n",
		PS3_HOST(instance),
		ps3_ioc_state_print(ioc_state), namePS3InstanceState(cur_state));

	if (ps3_pci_err_recovery_get(instance) ||
		(instance->peer_instance != NULL && ps3_pci_err_recovery_get(instance->peer_instance))) {
		LOG_WARN("hno:%u  pci recovery resetting\n",
			PS3_HOST(instance->peer_instance));
		ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
		goto l_out;
	}

	if (instance->recovery_context->heartbeat_recovery == PS3_HEARTBEAT_HARDRESET_DECIDE
		|| instance->recovery_context->heartbeat_recovery == PS3_HEARTBEAT_HARDRESET_RECOVERY) {
		ret = ps3_hard_recovery_request_decide(instance, cur_state, peer_cur_state);
		if (ret == PS3_RECOVERY_INTER_ERR_NEED_RECOVERY) {
			LOG_INFO("hno:%u heartbeat recovery start\n",
				PS3_HOST(instance));
			ps3_mutex_lock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
			instance->recovery_context->heartbeat_recovery = PS3_HEARTBEAT_HARDRESET_RECOVERY;
			ps3_mutex_unlock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
		}
		goto l_out;
	}

	if (ioc_state == PS3_FW_STATE_HALT) {
		ps3_instance_state_transfer_to_dead(instance);
		LOG_ERROR("hno:%u  IOC state has halt, instance state to dead!\n",
			PS3_HOST(instance));
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}

	if (cur_state == PS3_INSTANCE_STATE_DEAD ||
		cur_state == PS3_INSTANCE_STATE_QUIT ||
		cur_state == PS3_INSTANCE_STATE_SUSPEND) {
		LOG_ERROR("hno:%u  instance state is %s, and ioc_state is %s!\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state),
			ps3_ioc_state_print(ioc_state));
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}

	if (instance->peer_instance != NULL) {
		state_machine_peer = &instance->peer_instance->state_machine;
		if (ps3_pci_err_recovery_get(instance->peer_instance)) {
			LOG_WARN("hno:%u  pci recovery resetting\n",
				PS3_HOST(instance->peer_instance));
			ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
			goto l_out;
		}
		ps3_mutex_lock(&state_machine_peer->lock);
		ioc_state = instance->peer_instance->ioc_adpter->ioc_state_get(instance);
		peer_cur_state = ps3_atomic_read(&state_machine->state);
		LOG_INFO("hno:%u  recovery request:%s, %s\n",
			PS3_HOST(instance->peer_instance),
			ps3_ioc_state_print(ioc_state), namePS3InstanceState(peer_cur_state));

		if (ioc_state == PS3_FW_STATE_HALT) {
			ps3_instance_state_transfer_to_dead(instance->peer_instance);
			LOG_ERROR("hno:%u  ioc state has halt, instance state to dead!\n",
				PS3_HOST(instance->peer_instance));
			ret = PS3_RECOVERY_INTER_ERR_FAILED;
			goto l_out_peer;
		}

		if (peer_cur_state == PS3_INSTANCE_STATE_DEAD ||
			peer_cur_state == PS3_INSTANCE_STATE_QUIT ||
			peer_cur_state == PS3_INSTANCE_STATE_SUSPEND) {
			LOG_ERROR("hno:%u  instance state is %s, and ioc_state is %s!\n",
				PS3_HOST(instance->peer_instance), namePS3InstanceState(peer_cur_state),
				ps3_ioc_state_print(ioc_state));
			ret = PS3_RECOVERY_INTER_ERR_FAILED;
			goto l_out_peer;
		}
	}

	if (ioc_state == PS3_FW_STATE_FAULT ||
		ioc_state == PS3_FW_STATE_CRITICAL) {
		LOG_ERROR("hno:%u  instance state is %s, and ioc_state is %s!\n",
			PS3_HOST(instance), namePS3InstanceState(peer_cur_state),
			ps3_ioc_state_print(ioc_state));
		ret = ps3_hard_recovery_request_decide(instance, cur_state, peer_cur_state);
		goto l_out_peer;
	}

	if(cur_state != PS3_INSTANCE_STATE_OPERATIONAL ||
		peer_cur_state != PS3_INSTANCE_STATE_OPERATIONAL) {
		LOG_INFO("hno:%u  instance state is %s:%s! no need repeat recovery requeset\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state), namePS3InstanceState(cur_state));
		ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
		goto l_out_peer;
	}

	ret = ps3_ioc_soft_recovery_request_decide(instance, ioc_state);
	if (ret == PS3_RECOVERY_INTER_ERR_NEED_RECOVERY ||
		ret == PS3_RECOVERY_INTER_ERR_INTERRUPT) {
		goto l_out_peer;
	}else if (ret == PS3_RECOVERY_INTER_ERR_FAILED) {
		goto l_hardreset;
	}

	ret = ps3_soft_recovery_request_decide(instance, ioc_state);
	if (ret == PS3_RECOVERY_INTER_ERR_NEED_RECOVERY ||
		ret == PS3_RECOVERY_INTER_ERR_SUCCESS ||
        ret == PS3_RECOVERY_INTER_ERR_INTERRUPT) {
		goto l_out_peer;
	}else if (ret == PS3_RECOVERY_INTER_ERR_FAILED) {
		goto l_hardreset;
	}

	LOG_ERROR("hno:%u  UNEXPECT!!! hard recovery!,ioc_state:%s, %s:%s\n",
		PS3_HOST(instance),
		ps3_ioc_state_print(ioc_state), namePS3InstanceState(cur_state), namePS3InstanceState(peer_cur_state));

l_hardreset:
	ret = ps3_hard_recovery_request_decide(instance, cur_state, peer_cur_state);
l_out_peer:
	if (instance->peer_instance != NULL) {
		ps3_mutex_unlock(&state_machine_peer->lock);
	}
l_out:
	ps3_mutex_unlock(&state_machine->lock);
	return ret;
}

S32 ps3_recovery_request(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_instance *instance_recovery = NULL;
	ps3_atomic_inc(&instance->recovery_context->hardreset_ref);
	mb();
	if (instance->recovery_context->instance_change) {
		LOG_INFO("hno:%u peer instance is change\n", PS3_HOST(instance));
		ret = -PS3_RETRY;
		goto l_out;
	}
	while (instance->peer_instance != NULL &&
		PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance->peer_instance)) {
		if (instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_SHALLOW || \
			instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE) {
			instance->recovery_context->parall_hardreset_state = PS3_PARALLEL_HARDRESET_STATE_INIT;
			goto l_out;
		}

		if (instance->recovery_context->parall_hardreset_state == PS3_PARALLEL_HARDRESET_STATE_INIT) {
			instance->recovery_context->parall_hardreset_state = PS3_PARALLEL_HARDRESET_STATE_PENDING;
		}

		if (instance->recovery_context->instance_change) {
			LOG_INFO("hno:%u peer instance is change\n", PS3_HOST(instance));
			ret = -PS3_RETRY;
			goto l_out;
		}

		ps3_atomic_dec(&instance->recovery_context->hardreset_ref);
		ps3_msleep(10);
		if (instance->recovery_context->parall_hardreset_state == PS3_PARALLEL_HARDRESET_STATE_CONTINUE) {
			ps3_atomic_inc(&instance->recovery_context->hardreset_ref);
			mb();
			instance->recovery_context->parall_hardreset_state = PS3_PARALLEL_HARDRESET_STATE_INIT;
			break;
		}
		ps3_atomic_inc(&instance->recovery_context->hardreset_ref);
		mb();
	}

	if (instance->recovery_context->parall_hardreset_state == PS3_PARALLEL_HARDRESET_STATE_PENDING) {
		instance->recovery_context->parall_hardreset_state = PS3_PARALLEL_HARDRESET_STATE_INIT;
	}
	LOG_INFO("hno:%u peer_instance[%p], hard recovery request Function[%d]\n",
		PS3_HOST(instance), instance->peer_instance, ps3_get_pci_function(instance->pdev));
	if (ps3_get_pci_function(instance->pdev) == PS3_FUNC_ID_0 ||
		(ps3_get_pci_function(instance->pdev) == PS3_FUNC_ID_1 && instance->peer_instance == NULL)) {
		instance_recovery = instance;
	} else {
		instance_recovery = instance->peer_instance;
	}

	if (ps3_pci_err_recovery_get(instance_recovery)) {
		LOG_WARN("hno:%u  pci recovery resetting\n",
			PS3_HOST(instance_recovery));
		ret = PS3_SUCCESS;
		goto l_out;
	}

	if (ps3_recovery_is_state_halt(instance_recovery)) {
		LOG_INFO_LIM("hno:%u  driver_state:DEAD or HALT now !!!\n",
			PS3_HOST(instance_recovery));
		ret = -PS3_FAILED;
		goto l_out;
	}

	if (instance_recovery->peer_instance != NULL &&
		instance_recovery->peer_instance->recovery_context != NULL) {
		if (ps3_pci_err_recovery_get(instance_recovery->peer_instance)) {
			LOG_WARN("hno:%u  pci recovery resetting\n",
				PS3_HOST(instance_recovery->peer_instance));
			ret = PS3_SUCCESS;
			goto l_out;
		}

		if (instance_recovery->peer_instance->reg_set == NULL ||
			ps3_recovery_is_state_halt(instance_recovery->peer_instance)) {
			LOG_WARN("hno:%u  driver_state:DEAD or HALT now !!!\n",
				PS3_HOST(instance_recovery->peer_instance));
			ret = -PS3_FAILED;
			goto l_out;
		}
	}

	ret = ps3_recovery_request_prepare(instance_recovery);
	if (ret == PS3_RECOVERY_INTER_ERR_SUCCESS ||
		ret == PS3_RECOVERY_INTER_ERR_INTERRUPT) {
		ret = PS3_SUCCESS;
	} else if(ret == PS3_RECOVERY_INTER_ERR_NEED_RECOVERY) {
		ret = ps3_recovery_start(instance_recovery);
	} else {
		ret = -PS3_FAILED;
	}

l_out:
	LOG_INFO("hno:%u  recovery request:%d!\n",
		PS3_HOST(instance), ret);
	ps3_atomic_dec(&instance->recovery_context->hardreset_ref);
	return ret;
}

static Bool ps3_recovery_reg(struct ps3_instance *instance)
{
	Bool ret = PS3_FALSE;
	HilReg0Ps3RegisterFPs3FeatureSupport_u *ps3_feature_support = NULL;
	HilReg0Ps3RegisterFPs3FirmwareVersion_u *pver = NULL;
	U32 cur_max_fw_cmds = 0;
	U32 fw_cur_state = PS3_FW_STATE_UNDEFINED;
	U64 value = 0;
	U64 ver = 0;

	PS3_IOC_REG_READ_WITH_CHECK(instance, reg_f.Excl_reg, ps3FeatureSupport, value);
	if (value == U64_MAX) {
		goto l_out;
	}
	ps3_feature_support = (HilReg0Ps3RegisterFPs3FeatureSupport_u *)&value;
	PS3_IOC_REG_READ_WITH_CHECK(instance, reg_f.Excl_reg, ps3FirmwareVersion, ver);
	if (ver == U64_MAX) {
		goto l_out;
	}
	pver = (HilReg0Ps3RegisterFPs3FirmwareVersion_u *)&ver;
	if (!ps3_ioc_mgr_max_fw_cmd_get(instance, &cur_max_fw_cmds)) {
		goto l_out;
	}
	fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
	if (fw_cur_state != PS3_FW_STATE_RUNNING) {
		goto l_out;
	}
	instance->is_ioc_halt_support = (ps3_feature_support->reg.fwHaltSupport == 1);
	instance->dump_context.is_dump_support = (ps3_feature_support->reg.dumpCrashSupport == 1);
	instance->is_shallow_soft_recovery_support = (ps3_feature_support->reg.shallowSoftRecoverySupport == 1);
	instance->is_deep_soft_recovery_support = (ps3_feature_support->reg.deepSoftRecoverySupport == 1);
	instance->is_hard_recovery_support = (ps3_feature_support->reg.hardRecoverySupport == 1);
	instance->ioc_fw_version = (U64)pver->reg.ps3FmVer;
	ps3_can_queue_reset(instance, cur_max_fw_cmds);
	ret = PS3_TRUE;
l_out:
	return ret;
}

static S32 ps3_recovery_finish(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	if (!ps3_recovery_reg(instance)) {
		LOG_WARN("hno:%u recovery reg NOK\n",PS3_HOST(instance));
	}
	return ret;
}

static S32 ps3_recovery_complete(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_13, instance)
	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN("hno:%u  pci recovery resetting\n", PS3_HOST(instance));
		ret = -PS3_IN_PCIE_ERR;
		goto l_out;
	}

	if (PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance)) {
		goto l_out;
	}

	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_14, instance)
	if (ps3_recovery_finish(instance) != PS3_SUCCESS) {
		ps3_instance_state_transfer_to_dead(instance);
		LOG_ERROR("hno:%u  recovery finish process failed, to DEAD\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
	}

l_out:
    return ret;
}
static void ps3_hard_recovery(struct ps3_instance *instance)
{
	struct ps3_recovery_context *context = instance->recovery_context;
	if (ps3_hard_recovery_handle(instance) != PS3_SUCCESS) {
		context->recovery_result = -PS3_FAILED;
		LOG_ERROR("hno:%u  hard recovery failed!\n",
			PS3_HOST(instance));
	}else{
		context->recovery_result = PS3_SUCCESS;
		LOG_INFO("hno:%u  hard recovery success!\n",
			PS3_HOST(instance));
	}

	if (instance->recovery_context->heartbeat_recovery == PS3_HEARTBEAT_HARDRESET_RECOVERY) {
		ps3_mutex_lock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
		instance->recovery_context->heartbeat_recovery = PS3_HEARTBEAT_NULL;
		ps3_mutex_unlock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
	}

	return;
}

#ifndef _WINDOWS
static void ps3_recovery_work(struct work_struct* work)
{
	struct ps3_recovery_context *context =
		ps3_container_of(work, struct ps3_recovery_context, recovery_work);
	struct ps3_instance *instance = context->work_instance;
#else
static void ps3_recovery_work(void *ins)
{
	struct ps3_instance *instance = (struct ps3_instance*)ins;
	struct ps3_recovery_context *context = instance->recovery_context;
#endif
	U32 cur_state = ps3_atomic_read(&instance->state_machine.state);
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;

	LOG_INFO("hno:%u  recovery work start, %s recovery state[%d]\n",
		PS3_HOST(instance), namePS3InstanceState(cur_state), context->recovery_state);

	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN("hno:%u  pci recovery resetting\n",
			PS3_HOST(instance));

		if(instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE){
			ps3_recovery_state_transfer(instance, PS3_HARD_RECOVERY_FINISH);
		}else{
			ps3_recovery_state_transfer(instance, PS3_SOFT_RECOVERY_FINISH);
		}

		ps3_mutex_lock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
		instance->recovery_context->heartbeat_recovery =
				PS3_HEARTBEAT_NULL;
		ps3_mutex_unlock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
		goto l_out;
	}

	context->recovery_result = PS3_SUCCESS;
	if (cur_state == PS3_INSTANCE_STATE_SOFT_RECOVERY) {
		ps3_mutex_lock(&instance->state_machine.lock);
		if (context->recovery_state == PS3_SOFT_RECOVERY_IOC_RECOVERY){
			ps3_mutex_unlock(&instance->state_machine.lock);
			goto l_soft_reset;
		}

		if (context->recovery_state == PS3_SOFT_RECOVERY_SHALLOW) {
			ps3_mutex_unlock(&instance->state_machine.lock);
			goto l_soft_reset;
		}

		if (context->recovery_state == PS3_SOFT_RECOVERY_DEEP) {
			ps3_mutex_unlock(&instance->state_machine.lock);
			goto l_soft_reset;
		}

		LOG_ERROR("hno:%u nothing to do %s,recovery_state %d!\n",
			PS3_HOST(instance),namePS3InstanceState(cur_state),context->recovery_state);
		ps3_mutex_unlock(&instance->state_machine.lock);
		if (ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_SOFT_RECOVERY,
			PS3_INSTANCE_STATE_RECOVERY) != PS3_SUCCESS) {
			if (instance->peer_instance == NULL ||
				instance->peer_instance->recovery_context == NULL) {
				context->recovery_result = -PS3_FAILED;
				LOG_ERROR("hno:%u  hard recovery NOK,recovery_state %d!\n",
					PS3_HOST(instance),context->recovery_state);
				ps3_recovery_state_transfer(instance, PS3_HARD_RECOVERY_FINISH);
				goto l_out;
			}
		}

		if (instance->peer_instance != NULL &&
			ps3_instance_state_transfer(instance->peer_instance, PS3_INSTANCE_STATE_SOFT_RECOVERY,
			PS3_INSTANCE_STATE_RECOVERY) != PS3_SUCCESS) {
			context->recovery_result = -PS3_FAILED;
			LOG_ERROR("hno:%u  hard recovery NOK,recovery_state %d!\n",
				PS3_HOST(instance->peer_instance),context->recovery_state);
			ps3_recovery_state_transfer(instance->peer_instance, PS3_HARD_RECOVERY_FINISH);
			goto l_out;
		}
	} else if (cur_state == PS3_INSTANCE_STATE_RECOVERY) {
		goto l_hard_reset;
	} else {
		LOG_ERROR("hno:%u nothing to do %s,recovery_state %d, cur_state:%s!\n",
			PS3_HOST(instance),namePS3InstanceState(cur_state),context->recovery_state,
			namePS3InstanceState(ps3_atomic_read(&instance->state_machine.state)));
		if(instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE){
			ps3_recovery_state_transfer(instance, PS3_HARD_RECOVERY_FINISH);
		}else{
			ps3_recovery_state_transfer(instance, PS3_SOFT_RECOVERY_FINISH);
		}
		goto l_out;
	}

l_soft_reset:
	ret = ps3_soft_recovery_handle(instance);
	if (ret == PS3_RECOVERY_INTER_ERR_INTERRUPT) {
		context->recovery_result = PS3_SUCCESS;
		LOG_INFO("hno:%u  soft recovery interrupt!\n",
			PS3_HOST(instance));
		ps3_recovery_state_transfer(instance, PS3_SOFT_RECOVERY_FINISH);
	} else if(ret == PS3_RECOVERY_INTER_ERR_SUCCESS){
		context->recovery_result = PS3_SUCCESS;
		ps3_recovery_state_transfer(instance, PS3_SOFT_RECOVERY_FINISH);
		LOG_WARN("hno:%u  soft recovery success!\n",
			PS3_HOST(instance));
	} else {
		LOG_INFO("hno:%u  soft recovery to hard recovery!\n",
			PS3_HOST(instance));
		if(ps3_soft_recovery_fail_to_hard_recovery(instance) != PS3_SUCCESS){
			context->recovery_result = -PS3_FAILED;
			LOG_ERROR("hno:%u  soft to hard recovery failed!\n",
				PS3_HOST(instance));
		}else{
			context->recovery_result = PS3_SUCCESS;
			LOG_INFO("hno:%u  soft to hard recovery success!\n",
				PS3_HOST(instance));
		}
	}
	goto l_out;

l_hard_reset:
	ps3_hard_recovery(instance);

l_out:
	if (ps3_pci_err_recovery_get(instance)) {
		LOG_INFO("hno:%u  recovery is interrupted by pci err recovery.\n",
			PS3_HOST(instance));
		ps3_instance_state_transfer_to_pcie_recovery(instance);
		if (instance->peer_instance != NULL) {
			ps3_instance_state_transfer_to_pcie_recovery(instance->peer_instance);
		}
	}
	return;
}

static S32 ps3_soft_recovery_cmd_reply_check(struct ps3_instance *instance)
{
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
	U32 i = 0;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd_context *context = &instance->cmd_context;
	U32 no_reply_count = 0;

	for (i = context->max_scsi_cmd_count; i < context->max_cmd_count; i++) {
		cmd = context->cmd_buf[i];

		ps3_mutex_lock(&instance->state_machine.lock);
		if(PS3_IS_INTERRUPT_SOFT_RECOVERY(instance)){
    		LOG_WARN("hno:%u  soft reset proc is interrupt!\n",
    			PS3_HOST(instance));
			ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
			ps3_mutex_unlock(&instance->state_machine.lock);
			goto l_out;
		}
		ps3_mutex_unlock(&instance->state_machine.lock);

		if (cmd->cmd_state.reset_flag != PS3_CMD_FLAG_SOFTRESET) {
			continue;
		}

		if (cmd->cmd_state.state == PS3_CMD_STATE_INIT) {
			continue;
		}

		if (cmd->req_frame->mgrReq.reqHead.noReplyWord == PS3_CMD_WORD_NO_REPLY_WORD &&
			ps3_cmd_resp_status(cmd) != U32_MAX) {
			continue;
		}
		if (cmd == instance->event_context.event_cmd ||
			cmd == instance->dev_context.vd_pending_cmd ||
			cmd == instance->webSubscribe_context.webSubscribe_cmd) {
			continue;
		}

		no_reply_count++;
		LOG_DEBUG("hno:%u no reply cmd,CFID[%d], %s,max_scsi_cmd_count %u, max_cmd_count %u\n",
			PS3_HOST(instance), i, namePS3CmdState(cmd->cmd_state.state),
			context->max_scsi_cmd_count, context->max_cmd_count);
	}

	if (no_reply_count > 0) {
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
	}
l_out:
	return ret;
}

static S32 ps3_soft_recovery_cmd_reply_polling_check(
	struct ps3_instance *instance, U32 wait_seconds)
{
	S32 ret = PS3_RECOVERY_INTER_ERR_FAILED;
	const U32 interval_check_ms = 20; 
	U32 msecs = U32_MAX;
	U32 i = 0;

	if (wait_seconds != 0) {
#ifndef _WINDOWS
		msecs = wait_seconds * HZ;
#else
		msecs = wait_seconds * 1000;
#endif
	}

	for (i = 0; (i < msecs); i += interval_check_ms) {
		if (!ps3_is_instance_state_normal(instance)) {
			ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
			break;
		}
		ps3_mutex_lock(&instance->state_machine.lock);

		if(PS3_IS_INTERRUPT_SOFT_RECOVERY(instance)){
			LOG_WARN("hno:%u  soft reset proc is interrupt!\n",
				PS3_HOST(instance));
			ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
			ps3_mutex_unlock(&instance->state_machine.lock);
			break;
		}
		ps3_mutex_unlock(&instance->state_machine.lock);

		if (ps3_pci_err_recovery_get(instance)) {
			LOG_WARN("hno:%u host in pci err recovery\n",
				PS3_HOST(instance));
			ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
			break;
		}

		ret = ps3_soft_recovery_cmd_reply_check(instance);
		if (ret == PS3_RECOVERY_INTER_ERR_SUCCESS ||
			ret == PS3_RECOVERY_INTER_ERR_INTERRUPT) {
			LOG_WARN("hno:%u no mgr cmd pending\n",
				PS3_HOST(instance));
			break;
		}

		ps3_msleep(interval_check_ms);
	}

	return ret;
}

static S32 ps3_soft_recovery_to_pre_operational(struct ps3_instance *instance)
{
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;

	ps3_mutex_lock(&instance->state_machine.lock);
	if(PS3_IS_INTERRUPT_SOFT_RECOVERY(instance)){
		ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
		LOG_WARN("hno:%u  soft reset proc is interrupt!\n",
			PS3_HOST(instance));
		ps3_mutex_unlock(&instance->state_machine.lock);
		goto l_out;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);

	ps3_cmd_reset_flag_set(instance, PS3_CMD_FLAG_SOFTRESET);

	ret = ps3_ioc_state_transfer_wait_to_running(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  soft recovery to running failed!\n",
			PS3_HOST(instance));
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}
	ret = ps3_soft_recovery_cmd_reply_polling_check(instance, PS3_SOFT_RESET_WAIT_TIMEOUT);
	if (ret != PS3_RECOVERY_INTER_ERR_SUCCESS) {
		LOG_ERROR("hno:%u  pending mgr cmd no reply all!\n",
			PS3_HOST(instance));
		goto l_out;
	}

    ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
	LOG_INFO("hno:%u  soft recovery to pre-operational success!\n",
		PS3_HOST(instance));
l_out:
	return ret;
}

static S32 ps3_hard_recovery_pre_operational_to_operational(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	if (!ps3_ioc_recovery_count_get(instance,
		&instance->recovery_context->ioc_recovery_count)) {
		LOG_ERROR("hno:%u get recovery count NOK\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	LOG_INFO("hno:%u  pre-operational to operational success!\n",
		PS3_HOST(instance));
l_out:
	return ret;
}

static S32 ps3_soft_recovery_pre_operational_to_operational(struct ps3_instance *instance)
{
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;

	ps3_mutex_lock(&instance->state_machine.lock);
	if(PS3_IS_INTERRUPT_SOFT_RECOVERY(instance)){
		ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
		LOG_WARN("hno:%u  soft reset proc is interrupt!\n",
			PS3_HOST(instance));
		ps3_mutex_unlock(&instance->state_machine.lock);
		goto l_out;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);
	if (!ps3_ioc_recovery_count_get(instance,
		&instance->recovery_context->ioc_recovery_count)) {
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}

	LOG_INFO("hno:%u  pre-operational to operational success!,reset count:%d\n",
		PS3_HOST(instance), instance->recovery_context->ioc_recovery_count);
l_out:
	return ret;
}
static S32 ps3_cmd_resubscribe(struct ps3_instance *instance)
{
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;

	ps3_mutex_lock(&instance->state_machine.lock);
	if(PS3_IS_INTERRUPT_SOFT_RECOVERY(instance)){
		LOG_WARN("hno:%u  soft reset proc is interrupt!\n",
			PS3_HOST(instance));
		ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
		ps3_mutex_unlock(&instance->state_machine.lock);
		goto l_out;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);
	ret = ps3_soft_reset_event_resubscribe(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  event unsubscribe failed!\n",
			PS3_HOST(instance));
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}

	ps3_mutex_lock(&instance->state_machine.lock);
	if(PS3_IS_INTERRUPT_SOFT_RECOVERY(instance)){
		LOG_WARN("hno:%u  soft reset proc is interrupt!\n",
			PS3_HOST(instance));
		ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
		ps3_mutex_unlock(&instance->state_machine.lock);
		goto l_out;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);

	ret = ps3_dev_mgr_vd_info_resubscribe(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  vd pending unsubscribe failed!\n",
			PS3_HOST(instance));
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}

    ps3_mutex_lock(&instance->state_machine.lock);
    if(PS3_IS_INTERRUPT_SOFT_RECOVERY(instance)){
        LOG_WARN("hno:%u  soft reset proc is interrupt!\n",
            PS3_HOST(instance));
        ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
        ps3_mutex_unlock(&instance->state_machine.lock);
        goto l_out;
    }
    ps3_mutex_unlock(&instance->state_machine.lock);

    if (ps3_atomic_read(&instance->webSubscribe_context.is_subscribe) == 1) {
        ret = ps3_soft_reset_web_resubscribe(instance);
        if (ret != PS3_SUCCESS) {
            LOG_ERROR("hno:%u  web unsubscribe failed!\n",
                PS3_HOST(instance));
            ret = PS3_RECOVERY_INTER_ERR_FAILED;
            goto l_out;
        }
    }

    ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
l_out:
    return ret;
}
static S32 ps3_soft_recovery_complete(struct ps3_instance *instance)
{
	S32 ret = ps3_soft_recovery_to_pre_operational(instance);
	if (ret != PS3_RECOVERY_INTER_ERR_SUCCESS) {
		LOG_WARN("hno:%u  ioc soft reset failed!\n",
		PS3_HOST(instance));
		goto l_out;
	}

	if (ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_SOFT_RECOVERY,
		PS3_INSTANCE_STATE_PRE_OPERATIONAL) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  transfer to pre operational failed!\n",
			PS3_HOST(instance));
		ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
		goto l_out;
	}

	if (instance->peer_instance != NULL && \
			instance->peer_instance->recovery_function.softreset_handle_pre_cb != NULL) {
		ret = instance->peer_instance->recovery_function.softreset_handle_pre_cb(instance->peer_instance);
		if (ret != PS3_RECOVERY_INTER_ERR_SUCCESS) {
			LOG_WARN("hno:%u  softreset handle pre cb failed\n",
				PS3_HOST(instance->peer_instance));
			goto l_out;
		}

		if (ps3_instance_state_transfer(instance->peer_instance, PS3_INSTANCE_STATE_SOFT_RECOVERY,
			PS3_INSTANCE_STATE_PRE_OPERATIONAL) != PS3_SUCCESS) {
			LOG_ERROR("hno:%u  transfer to pre operational failed!\n",
				PS3_HOST(instance->peer_instance));
			ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
			goto l_out;
		}
	}

    ret = ps3_soft_recovery_pre_operational_to_operational(instance);
	if (ret != PS3_RECOVERY_INTER_ERR_SUCCESS) {
		LOG_WARN("hno:%u  ioc soft reset failed!\n",
		PS3_HOST(instance));
		goto l_out;
	}

	if (instance->peer_instance != NULL && \
			instance->peer_instance->recovery_function.softreset_handle_post_cb != NULL) {
		ret = instance->peer_instance->recovery_function.softreset_handle_post_cb(instance->peer_instance);
		if (ret != PS3_RECOVERY_INTER_ERR_SUCCESS) {
			LOG_WARN("hno:%u  softreset handle post cb failed\n",
				PS3_HOST(instance->peer_instance));
			goto l_out;
		}
	}

l_out:
	return ret;
}

static S32 ps3_soft_recovery_fail_to_hard_recovery(struct ps3_instance *instance)
{
	S32 ret = -PS3_FAILED;
	U32 cur_state = ps3_atomic_read(&instance->state_machine.state);
	S32 ret_peer = PS3_SUCCESS;
	U32 peer_cur_state = PS3_INSTANCE_STATE_OPERATIONAL;

	PS3_IJ_SLEEP(10000, PS3_ERR_IJ_SOFT_TO_HARD);
	ps3_mutex_lock(&instance->state_machine.lock);

	if (instance->peer_instance != NULL) {
		ps3_mutex_lock(&instance->peer_instance->state_machine.lock);
		peer_cur_state = ps3_atomic_read(&instance->peer_instance->state_machine.state);
	}

	ret = ps3_hard_recovery_request_decide(instance, cur_state, peer_cur_state);
	if (instance->peer_instance != NULL) {
		ps3_mutex_unlock(&instance->peer_instance->state_machine.lock);
	}
	ps3_mutex_unlock(&instance->state_machine.lock);
	if (ret != PS3_RECOVERY_INTER_ERR_NEED_RECOVERY) {
		LOG_WARN("hno:%u  decide no need to hard recovery\n",
			PS3_HOST(instance));
		ret = PS3_SUCCESS;
		goto l_out;
	}

	ret = ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_SOFT_RECOVERY,
		PS3_INSTANCE_STATE_RECOVERY);
	if (ret != PS3_SUCCESS) {
		ret = ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_PRE_OPERATIONAL,
			PS3_INSTANCE_STATE_RECOVERY);
	}
	if (ret != PS3_SUCCESS) {
		ret = ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_OPERATIONAL,
			PS3_INSTANCE_STATE_RECOVERY);
	}

	if (instance->peer_instance != NULL) {
		ret_peer = ps3_instance_state_transfer(instance->peer_instance, PS3_INSTANCE_STATE_SOFT_RECOVERY,
			PS3_INSTANCE_STATE_RECOVERY);
		if (ret_peer != PS3_SUCCESS) {
			ret_peer = ps3_instance_state_transfer(instance->peer_instance, PS3_INSTANCE_STATE_PRE_OPERATIONAL,
				PS3_INSTANCE_STATE_RECOVERY);
		}
		if (ret_peer != PS3_SUCCESS) {
			ret_peer = ps3_instance_state_transfer(instance->peer_instance, PS3_INSTANCE_STATE_OPERATIONAL,
				PS3_INSTANCE_STATE_RECOVERY);
		}
	}

	if (ret == PS3_SUCCESS && ret_peer == PS3_SUCCESS) {
		ps3_need_wait_hard_reset_request(instance);
		ret = ps3_hard_recovery_handle(instance);
	} else {
		LOG_ERROR("hno:%u  transfer to recovery failed!，cur_state:%s\n",
			PS3_HOST(instance),
			namePS3InstanceState(ps3_atomic_read(&instance->state_machine.state)));
		ps3_recovery_state_transfer(instance, PS3_SOFT_RECOVERY_FINISH);
	}

	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  hard reset failed!\n",
			PS3_HOST(instance));
	} else {
		LOG_INFO("hno:%u  hard reset success and finished!\n",
			PS3_HOST(instance));
	}
l_out:
	return ret;
}

static S32 ps3_ioc_soft_recovery(struct ps3_instance *instance)
{
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;

	ret = ps3_soft_recovery_complete(instance);
	if (ret == PS3_RECOVERY_INTER_ERR_INTERRUPT) {
		LOG_WARN("hno:%u  IOC soft reset proc is interrupt!\n",
			PS3_HOST(instance));
		goto l_out;
	} else if (ret == PS3_RECOVERY_INTER_ERR_SUCCESS) {
		LOG_INFO("hno:%u  IOC soft reset success!\n",
			PS3_HOST(instance));
		goto l_reset_sucess;
	} else {
		LOG_ERROR("hno:%u  IOC self soft reset failed! need to hard reset\n",
			PS3_HOST(instance));
		goto l_out;
	}

l_reset_sucess:
	ret = ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_PRE_OPERATIONAL,
		PS3_INSTANCE_STATE_OPERATIONAL);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  transfer to operational failed!\n",
			PS3_HOST(instance));
		ret = PS3_RECOVERY_INTER_ERR_INTERRUPT;
		goto l_out;
	}
	ret = ps3_cmd_resubscribe(instance);
	if (ret != PS3_RECOVERY_INTER_ERR_SUCCESS) {
		LOG_ERROR("hno:%u  cmd unsubscribe failed!\n",
			PS3_HOST(instance));
		goto l_out;
	}
	ret = ps3_recovery_complete(instance);
	if(ret != PS3_SUCCESS){
		LOG_INFO("hno:%u  IOC soft reset opeational but complete failed! cur_state:%s,ret %d\n",
			PS3_HOST(instance), namePS3InstanceState(ps3_atomic_read(&instance->state_machine.state)),ret);
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}
	if (instance->peer_instance != NULL) {
		ret = ps3_instance_state_transfer(instance->peer_instance, PS3_INSTANCE_STATE_PRE_OPERATIONAL,
			PS3_INSTANCE_STATE_OPERATIONAL);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u  transfer to operational failed!\n",
				PS3_HOST(instance->peer_instance));
			ret = PS3_RECOVERY_INTER_ERR_FAILED;
			goto l_out;
		}
		LOG_INFO("hno:%u  soft reset success and finished!\n",
			PS3_HOST(instance->peer_instance));
		ret = ps3_cmd_resubscribe(instance->peer_instance);
		if (ret != PS3_RECOVERY_INTER_ERR_SUCCESS) {
			LOG_ERROR("hno:%u  cmd unsubscribe failed!\n",
				PS3_HOST(instance->peer_instance));
			goto l_out;
		}
		ret = ps3_recovery_complete(instance->peer_instance);
		if(ret != PS3_SUCCESS){
			LOG_INFO("hno:%u  soft reset opeational but complete failed! cur_state:%s,ret %d\n",
				PS3_HOST(instance->peer_instance),
				namePS3InstanceState(ps3_atomic_read(&instance->peer_instance->state_machine.state)),ret);
			ret = PS3_RECOVERY_INTER_ERR_FAILED;
			goto l_out;
		}
	}
	ps3_recovery_state_transfer(instance, PS3_SOFT_RECOVERY_FINISH);

    ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
	LOG_INFO("hno:%u  soft reset success and finished!\n",
		PS3_HOST(instance));
l_out:
	return ret;
}

static S32 ps3_host_shallow_soft_recovery(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	S32 err_code = PS3_RECOVERY_INTER_ERR_SUCCESS;
	if (PS3_FALSE == PS3_IOC_SHALLOW_SOFT_RECOVERY_SUPPORT(instance)) {
		LOG_ERROR("hno:%u  soc feature unsupport soft reset Shallow!\n",
			PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
		goto l_out;
	}

	if (instance->peer_instance != NULL) {
		if (PS3_FALSE == PS3_IOC_SHALLOW_SOFT_RECOVERY_SUPPORT(instance->peer_instance)) {
			LOG_ERROR("hno:%u  soc feature unsupport soft reset Shallow!\n",
				PS3_HOST(instance->peer_instance));
			err_code = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
			goto l_out;
		}
	}
	ps3_mutex_lock(&instance->state_machine.lock);
	if(PS3_IS_INTERRUPT_SOFT_RECOVERY(instance)){
		LOG_WARN("hno:%u  shallow soft reset proc is interrupt!\n",
			PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_INTERRUPT;
		ps3_mutex_unlock(&instance->state_machine.lock);
		goto l_out;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);
	if (instance->ioc_adpter->ioc_shallow_soft_reset != NULL) {
		ps3_recovery_state_transfer(instance, PS3_SOFT_RECOVERY_SHALLOW);
		ret = instance->ioc_adpter->ioc_shallow_soft_reset(instance);
	} else {
		LOG_ERROR("hno:%u  driver unsupport soft reset shallow!\n",
			PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
		goto l_out;
	}

	if (ret == PS3_SUCCESS) {
		LOG_INFO("hno:%u  shallow soft reset entry complete prcoess!\n",
			PS3_HOST(instance));
		err_code = ps3_soft_recovery_complete(instance);
	} else {
		LOG_ERROR("hno:%u  shallow soft reset fail!\n",
			PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
	}

	LOG_INFO("hno:%u  shallow soft reset finished!:ret:%d, err_code:%d\n",
		PS3_HOST(instance), ret, err_code);

l_out:
	return err_code;
}

static S32 ps3_host_deep_soft_recovery(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	S32 err_code = PS3_RECOVERY_INTER_ERR_SUCCESS;
	if (PS3_FALSE == PS3_IOC_DEEP_SOFT_RECOVERY_SUPPORT(instance)) {
			LOG_ERROR("hno:%u  soc feature unsupport soft reset Deep!\n",
				PS3_HOST(instance));
			err_code = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
			goto l_out;
	}

	if (instance->peer_instance != NULL) {
		if (PS3_FALSE == PS3_IOC_DEEP_SOFT_RECOVERY_SUPPORT(instance->peer_instance)) {
			LOG_ERROR("hno:%u  soc feature unsupport soft reset Deep!\n",
				PS3_HOST(instance->peer_instance));
			err_code = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
			goto l_out;
		}
	}

	ps3_mutex_lock(&instance->state_machine.lock);
    if(PS3_IS_INTERRUPT_SOFT_RECOVERY(instance)){
		LOG_WARN("hno:%u  deep soft reset proc is interrupt!\n",
			PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_INTERRUPT;
		ps3_mutex_unlock(&instance->state_machine.lock);
		goto l_out;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);
	ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_PRE_OPERATIONAL,
		PS3_INSTANCE_STATE_SOFT_RECOVERY);

	if (ps3_atomic_read(&instance->state_machine.state) !=
		PS3_INSTANCE_STATE_SOFT_RECOVERY) {
		LOG_ERROR("hno:%u  soft reovery has been interrupt!\n",
			PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_INTERRUPT;
		goto l_out;
	}
	if (instance->peer_instance != NULL) {
		ps3_instance_state_transfer(instance->peer_instance, PS3_INSTANCE_STATE_PRE_OPERATIONAL,
			PS3_INSTANCE_STATE_SOFT_RECOVERY);

		if (ps3_atomic_read(&instance->peer_instance->state_machine.state) !=
			PS3_INSTANCE_STATE_SOFT_RECOVERY) {
			LOG_ERROR("hno:%u  soft reovery has been interrupt!\n",
				PS3_HOST(instance->peer_instance));
			err_code = PS3_RECOVERY_INTER_ERR_INTERRUPT;
			goto l_out;
		}
	}

	if (instance->ioc_adpter->ioc_deep_soft_reset != NULL) {
		ps3_recovery_state_transfer(instance, PS3_SOFT_RECOVERY_DEEP);
		ret = instance->ioc_adpter->ioc_deep_soft_reset(instance);
	} else {
		LOG_ERROR("hno:%u  driver unsupport soft reset deep!\n",
			PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
		goto l_out;
	}

	if (ret == PS3_SUCCESS) {
		LOG_INFO("hno:%u  deep soft reset entry complete prcoess!\n",
			PS3_HOST(instance));
		err_code = ps3_soft_recovery_complete(instance);
	} else {
		LOG_ERROR("hno:%u  deep soft reset fail!\n",
			PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
	}

	LOG_INFO("hno:%u  deep soft reset finished!:ret:%d, err_code:%d\n",
		PS3_HOST(instance), ret, err_code);

l_out:
	return err_code;
}
static inline S32 ps3_wait_event_vdpending_cmd_complete(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 count = 0;

	const U32 retry_max  = PS3_WAIT_EVENT_CMD_LOOP_COUNT;

	while(instance->event_context.abort_eventcmd != 0 ||
		instance->dev_context.abort_vdpending_cmd != 0 ||
        instance->webSubscribe_context.abort_webcmd != 0){
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);

		if(count++ > retry_max){
			LOG_INFO("hno:%u  wait event proc over:%d ms,failed\n",
				PS3_HOST(instance),
				retry_max*PS3_LOOP_TIME_INTERVAL_100MS);
		    ret = -PS3_FAILED;
			break;
		}
	}
	return ret;
}
static S32 ps3_soft_recovery_handle(struct ps3_instance *instance)
{
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
	struct ps3_recovery_context *context = instance->recovery_context;

	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN("hno:%u  pci error recovery resetting\n",
			PS3_HOST(instance));
		goto l_out;
	}

	if(ps3_wait_event_vdpending_cmd_complete(instance) != PS3_SUCCESS){
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}
	if (instance->peer_instance != NULL) {
		if (ps3_pci_err_recovery_get(instance->peer_instance)) {
			LOG_WARN("hno:%u  pci error recovery resetting\n",
				PS3_HOST(instance->peer_instance));
			goto l_out;
		}

		if(ps3_wait_event_vdpending_cmd_complete(instance->peer_instance) != PS3_SUCCESS){
			ret = PS3_RECOVERY_INTER_ERR_FAILED;
			goto l_out;
		}
	}
	ps3_mutex_lock(&instance->state_machine.lock);
	if (context->recovery_state == PS3_SOFT_RECOVERY_IOC_RECOVERY) {
		ps3_mutex_unlock(&instance->state_machine.lock);
		ret = ps3_ioc_soft_recovery(instance);
		goto l_out;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);

	ret = ps3_host_shallow_soft_recovery(instance);
	if (ret == PS3_RECOVERY_INTER_ERR_INTERRUPT) {
		LOG_WARN("hno:%u  shallow soft reset proc is interrupt!\n",
			PS3_HOST(instance));
		goto l_out;
	} else if (ret == PS3_RECOVERY_INTER_ERR_SUCCESS) {
		LOG_WARN("hno:%u  shallow soft reset success!\n",
			PS3_HOST(instance));
		goto l_reset_sucess;
	} else {
		LOG_WARN("hno:%u  shallow soft reset fail, need to deep recovery!\n",
			PS3_HOST(instance));
	}

	ret = ps3_host_deep_soft_recovery(instance);
	if (ret == PS3_RECOVERY_INTER_ERR_INTERRUPT) {
		LOG_WARN("hno:%u  deep soft reset proc is interrupt!\n",
			PS3_HOST(instance));
		goto l_out;
	} else if (ret == PS3_RECOVERY_INTER_ERR_SUCCESS) {
		LOG_INFO("hno:%u  deep soft reset success!\n",
			PS3_HOST(instance));
		goto l_reset_sucess;
	} else {
		LOG_INFO("hno:%u  deep soft reset fail, need to hard recovery!\n",
			PS3_HOST(instance));
		goto l_out;
	}

l_reset_sucess:
	ret = ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_PRE_OPERATIONAL,
		PS3_INSTANCE_STATE_OPERATIONAL);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  transfer to operational failed!\n",
			PS3_HOST(instance));
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}
	LOG_INFO("hno:%u  soft reset success and finished!\n",
		PS3_HOST(instance));
	ret = ps3_cmd_resubscribe(instance);
	if (ret != PS3_RECOVERY_INTER_ERR_SUCCESS) {
		LOG_ERROR("hno:%u  cmd unsubscribe failed!\n",
			PS3_HOST(instance));
		goto l_out;
	}
	ret = ps3_recovery_complete(instance);
	if(ret != PS3_SUCCESS){
		LOG_INFO("hno:%u  soft reset opeational but complete failed! cur_state:%s,ret %d\n",
			PS3_HOST(instance), namePS3InstanceState(ps3_atomic_read(&instance->state_machine.state)),ret);
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}

	if (instance->peer_instance != NULL) {
		ret = ps3_instance_state_transfer(instance->peer_instance, PS3_INSTANCE_STATE_PRE_OPERATIONAL,
			PS3_INSTANCE_STATE_OPERATIONAL);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u  transfer to operational failed!\n",
				PS3_HOST(instance->peer_instance));
			ret = PS3_RECOVERY_INTER_ERR_FAILED;
			goto l_out;
		}
		LOG_INFO("hno:%u  soft reset success and finished!\n",
			PS3_HOST(instance->peer_instance));
		ret = ps3_cmd_resubscribe(instance->peer_instance);
		if (ret != PS3_RECOVERY_INTER_ERR_SUCCESS) {
			LOG_ERROR("hno:%u  cmd unsubscribe failed!\n",
				PS3_HOST(instance->peer_instance));
			goto l_out;
		}
		ret = ps3_recovery_complete(instance->peer_instance);
		if(ret != PS3_SUCCESS){
			LOG_INFO("hno:%u  soft reset opeational but complete failed! cur_state:%s,ret %d\n",
				PS3_HOST(instance->peer_instance),
				namePS3InstanceState(ps3_atomic_read(&instance->peer_instance->state_machine.state)),ret);
			ret = PS3_RECOVERY_INTER_ERR_FAILED;
			goto l_out;
		}
	}

	LOG_INFO("hno:%u  soft reset success and finished!\n",
		PS3_HOST(instance));
	ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
	return ret;

l_out:
	return ret;
}

static S32 ps3_hard_recovery_to_ready(struct ps3_instance *instance)
{
	S32 err_code = PS3_RECOVERY_INTER_ERR_SUCCESS;
	U32 ioc_state = 0;
	S32 ret = PS3_SUCCESS;

	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_3, instance)
	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN("hno:%u  pci recovery resetting\n", PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_PCIE_ERR;
		goto l_out;
	}

	if ((ret = ps3_ioc_hard_reset_to_ready(instance)) != PS3_SUCCESS) {
		ioc_state = instance->ioc_adpter->ioc_state_get(instance);
		LOG_ERROR("hno:%u  hard reset to ready NOK,%s!\n",
			PS3_HOST(instance),
			ps3_ioc_state_print(ioc_state));
		if (ret == -PS3_IN_PCIE_ERR) {
			err_code = PS3_RECOVERY_INTER_ERR_PCIE_ERR;
		} else {
			err_code = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
		}
		goto l_out;
	}

	if (ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_RECOVERY,
		PS3_INSTANCE_STATE_READY) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  transfer to ready NOK!\n",
			PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_INTERRUPT;
	}
l_out:
	return err_code;
}

static S32 ps3_hard_recovery_to_pre_operational(struct ps3_instance *instance)
{
	S32 err_code = PS3_RECOVERY_INTER_ERR_SUCCESS;
	S32 ret = PS3_SUCCESS;

	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_7, instance)
	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN("hno:%u  pci recovery resetting\n", PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_PCIE_ERR;
		goto l_out;
	}

	if (PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance)) {
		goto l_out;
	}

	if ((ret = ps3_recovery_ready_to_running(instance)) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  hard reset to ready NOK!\n",
			PS3_HOST(instance));

		if (ret == -PS3_IN_PCIE_ERR) {
			err_code = PS3_RECOVERY_INTER_ERR_PCIE_ERR;
			goto l_out;
		}
		if (ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_READY,
			PS3_INSTANCE_STATE_RECOVERY) != PS3_SUCCESS) {
			LOG_ERROR("hno:%u  transfer to recovery NOK!\n",
				PS3_HOST(instance));
			err_code = PS3_RECOVERY_INTER_ERR_INTERRUPT;
			goto l_out;
		}

		err_code = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
		goto l_out;
	}

	instance->ioc_adpter->irq_enable(instance);
#ifndef _WINDOWS
	ps3_irqpolls_enable(instance);
#endif
    if (instance->is_need_event && ps3_atomic_read(&instance->webSubscribe_context.is_subscribe) == 1) {
       ps3_web_cmd_clear(instance);
    }
	if (instance->is_need_event) {
		ps3_dev_mgr_vd_info_clear(instance);
	}

	if (ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_READY,
		PS3_INSTANCE_STATE_PRE_OPERATIONAL) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  transfer to pre-operational NOK!\n",
			PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_INTERRUPT;
		goto l_out;
	}
	ps3_dump_ctrl_set_int_ready(instance);

l_out:
	return err_code;
}

static S32 ps3_hard_recovery_to_operational(struct ps3_instance *instance)
{
	S32 err_code = PS3_RECOVERY_INTER_ERR_SUCCESS;
	S32 ret;
	ULong flags = 0;
	struct ps3_cmd *cmd = NULL;	  
	Bool need_subscribe = PS3_FALSE;

	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_10, instance)
	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN("hno:%u  pci recovery resetting\n", PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_PCIE_ERR;
		goto l_out;
	}

	if (PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance)) {
		goto l_out;
	}

	if (ps3_hard_recovery_pre_operational_to_operational(instance) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  pre operational NOK!\n",
			PS3_HOST(instance));
		if (ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_PRE_OPERATIONAL,
			PS3_INSTANCE_STATE_RECOVERY) != PS3_SUCCESS) {
			LOG_ERROR("hno:%u  transfer to recovery NOK!\n",
				PS3_HOST(instance));
			err_code = PS3_RECOVERY_INTER_ERR_INTERRUPT;
			goto l_out;
		}

		err_code = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
		goto l_out;
	}

	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_OPE_FAIL, instance)
	if (ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_PRE_OPERATIONAL,
		PS3_INSTANCE_STATE_OPERATIONAL) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  transfer to operational NOK!\n",
			PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_INTERRUPT;
		goto l_out;
	}
	INJECT_START(PS3_ERR_IJ_HARD_SUBC_WAIT_EVENT_INT, instance)
	INJECT_START(PS3_ERR_IJ_HARD_SUBC_WAIT_EVENT_PROC, instance)
	INJECT_START(PS3_ERR_IJ_HARD_SUBC_WAIT_EVENT_SUBCRIBE, instance)

	if (!instance->is_need_event) {
		LOG_INFO("hno:%u  not need event\n",
			PS3_HOST(instance));
		goto l_out;
	}

	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags);
	cmd = instance->event_context.event_cmd;
	if (cmd == NULL) {
		LOG_WARN_IN_IRQ(instance, "hno:%u  Event is not register yet\n",
			PS3_HOST(instance));
		need_subscribe = PS3_TRUE;
		goto l_subscribe;
	}
	if (instance->event_context.subwork == 0) {
		if (instance->hardreset_event != 0) {
			instance->hardreset_event = 0;
			LOG_INFO_IN_IRQ(instance, "hno:%u  event cmd free, CFID:%d\n",
				PS3_HOST(instance), cmd->index);
			instance->event_context.event_cmd = NULL;
			ps3_mgr_cmd_free(instance, cmd);
			need_subscribe = PS3_TRUE;
		}
	}

l_subscribe:
	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_11, instance)
	if(need_subscribe == PS3_TRUE) {
		instance->event_req_info.eventTypeMapProcResult = instance->event_req_info.eventTypeMap;
		ret = ps3_event_subscribe(instance);
		if (ret != PS3_SUCCESS && ret != -PS3_IN_UNLOAD) {
			err_code = PS3_RECOVERY_INTER_ERR_SUCCESS;
			ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags);
			LOG_WARN("hno:%u  IOC event subscribe NOK ret[%d]\n",
				PS3_HOST(instance), ret);
			goto l_out;
		}
	}
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags);

	ret = ps3_dev_mgr_vd_info_subscribe(instance);
	if (ret != PS3_SUCCESS && ret != -PS3_IN_UNLOAD) {
		err_code = PS3_RECOVERY_INTER_ERR_SUCCESS;
		LOG_WARN("hno:%u  vd info subscribe NOK ret[%d]\n",
			PS3_HOST(instance), ret);
		goto l_out;
	}

	if (ps3_atomic_read(&instance->webSubscribe_context.is_subscribe) == 1) {
		ret = ps3_web_subscribe(instance);
		if (ret != PS3_SUCCESS && ret != -PS3_IN_UNLOAD) {
			err_code = PS3_RECOVERY_INTER_ERR_SUCCESS;
			LOG_WARN("hno:%u  web subscribe NOK ret[%d]\n",
				PS3_HOST(instance), ret);
			goto l_out;
		}
	}
l_out:
	return err_code;
}

static inline void ps3_hard_reset_unnormal_handle(struct ps3_instance *instance)
{
	ps3_recovery_ready_to_force_cmd_stop(instance);

	ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_READY,
		PS3_INSTANCE_STATE_RECOVERY);
	instance ->is_half_hard_reset = PS3_TRUE;
	if (!instance->state_machine.is_load ||
		instance->state_machine.is_suspend) {
		ps3_ioc_notify_unload(instance);
	}
}

static inline S32 ps3_hard_reset_error_code_decide(struct ps3_instance *instance)
{
	S32 err_code = PS3_RECOVERY_INTER_ERR_SUCCESS;
	if (PS3_IS_INSTANCE_SUSPEND_OR_RESUME(instance)) {
		err_code = PS3_RECOVERY_INTER_ERR_SUSPEND_RESUME;
		goto l_out;
	}

	if (PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance) && \
			instance->peer_instance == NULL) {
		err_code = PS3_RECOVERY_INTER_ERR_FAILED;
	}

l_out:
	return err_code;
}

static inline S32 ps3_hard_reset_multi_unnormal_handle(struct ps3_instance *instance)
{

	if (PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance)) {
		LOG_WARN("host_no[%u], reset while probe or in shutdown/remove,"
			" finish[%d], flag[%d], is_load[%d]\n",
			PS3_HOST(instance), instance->is_probe_finish,
			instance->is_probe_failed,
			instance->state_machine.is_load);
		if (((PS3_IS_INSTANCE_PROBE(instance) || PS3_IS_INSTANCE_RESUME(instance)) && \
			instance->state_machine.is_load) || \
			PS3_IS_INSTANCE_REMOVE(instance)) {
			ps3_hard_reset_unnormal_handle(instance);
			ps3_instance_state_transfer_to_dead(instance);
		} else if (PS3_IS_INSTANCE_SUSPEND(instance)) {
			ps3_hard_reset_unnormal_handle(instance);
		}
	}
	if (instance->peer_instance != NULL && PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance->peer_instance)) {
		LOG_WARN("host_no[%u], reset while probe or in shutdown/remove,"
			" finish[%d], flag[%d], is_load[%d]\n",
			PS3_HOST(instance->peer_instance), instance->peer_instance->is_probe_finish,
			instance->peer_instance->is_probe_failed,
			instance->peer_instance->state_machine.is_load);
		if (((PS3_IS_INSTANCE_PROBE(instance->peer_instance) || PS3_IS_INSTANCE_RESUME(instance->peer_instance)) && \
			instance->peer_instance->state_machine.is_load) || \
			PS3_IS_INSTANCE_REMOVE(instance->peer_instance)) {
			ps3_hard_reset_unnormal_handle(instance->peer_instance);
			ps3_instance_state_transfer_to_dead(instance->peer_instance);
		} else if (PS3_IS_INSTANCE_SUSPEND(instance->peer_instance)) {
			ps3_hard_reset_unnormal_handle(instance->peer_instance);
		}
	}
	return ps3_hard_reset_error_code_decide(instance);
}

static void ps3_recovery_wait_reg_access_done(struct ps3_instance *instance)
{
	LOG_DEBUG("hno:%u recovery wait reg access done start\n",
			PS3_HOST(instance));
	ps3_wait_scsi_cmd_done(instance, PS3_FALSE);
	while (ps3_atomic_read(&instance->reg_op_count) != 0){
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_20MS);
	}
	LOG_DEBUG("hno:%u recovery wait reg access done end\n",
			PS3_HOST(instance));
}

void ps3_hard_recovery_state_finish(struct ps3_instance *instance) {
	ps3_mutex_lock(&instance->state_machine.lock);
	if (instance->recovery_context->host_reset_state == PS3_HOST_RESET_START) {
		instance->recovery_context->host_reset_state = PS3_HOST_RESET_HARD_RESET_DONE;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);
	ps3_recovery_state_transfer(instance, PS3_HARD_RECOVERY_FINISH);
}

static S32 ps3_hard_recovery_handle(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 retries = 0;
	U32 cur_state = 0;
	U32 ioc_state = 0;
	S32 err_code = PS3_RECOVERY_INTER_ERR_SUCCESS;
	U32 wait_count = 0;
	ULong flags = 0;

	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_0, instance)
	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN("hno:%u  pci recovery resetting\n",
			PS3_HOST(instance));
		goto l_out;
	}

	if (((!PS3_IOC_HARD_RECOVERY_SUPPORT(instance))
		|| (!ps3_hard_reset_enable_query()))
		&& (instance->recovery_context->heartbeat_recovery != PS3_HEARTBEAT_HARDRESET_RECOVERY)) {
		LOG_ERROR("hno:%u  soc feature unsupport Hard reset! need to be offline!\n",
			PS3_HOST(instance));
		goto l_offline;
	}

	INJECT_START(PS3_ERR_IJ_FLAG_WAIT_EVENT_INT, instance)
	INJECT_START(PS3_ERR_IJ_FLAG_WAIT_EVENT_PROC, instance)
	INJECT_START(PS3_ERR_IJ_FLAG_WAIT_EVENT_SUBCRIBE, instance)
	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags);
	if (instance->event_context.abort_eventcmd != 0) {
		instance->event_context.abort_eventcmd = 0;
	}

	if (instance->hardreset_event == 0) {
		instance->hardreset_event = 1;
	}
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags);
	INJECT_START(PS3_ERR_IJ_HOST_RESET_WAIT_DECIDE, instance)
	ps3_recovery_state_transfer(instance, PS3_HARD_RECOVERY_SHALLOW);
	INJECT_START(PS3_ERR_IJ_RECOVERY_WAIT_FUNC1_PROBE, instance)
	INJECT_START(PS3_ERR_IJ_RECOVERY_WAIT_FUNC1_REMOVE, instance)
	if (instance->is_scan_host_finish == PS3_TRUE
			&& instance->is_probe_finish == PS3_FALSE) {
		LOG_INFO("hno:%u  recovery wait until probe finish/failed\n",
				PS3_HOST(instance));
		while (wait_count < PS3_RECOVERY_WAIT_PROBE_FINISH_LOOP_COUNT) {
			if (instance->is_probe_finish || instance->is_probe_failed) {
				break;
			}

			wait_count++;
			ps3_msleep(PS3_RECOVERY_WAIT_LOOP_TIME_INTERVAL_20MS);
		}

		LOG_INFO("hno:%u  probe finish(%d)/failed(%d), continue recovery\n",
				PS3_HOST(instance), instance->is_probe_finish, instance->is_probe_failed);
	}

	ps3_wait_watchdog_dect_recovery(instance);

	if (instance->peer_instance != NULL && \
		instance->peer_instance->recovery_function.hardreset_handle_pre_cb != NULL) {
		ret = instance->peer_instance->recovery_function.hardreset_handle_pre_cb(instance->peer_instance);
		if (ret != PS3_SUCCESS) {
			LOG_WARN("hno:%u  hardreset handle pre cb NOK\n",
				PS3_HOST(instance));
			if (ret == -PS3_IN_PCIE_ERR) {
				goto l_out;
			} else {
				goto l_offline;
			}
		}
	}

	while(ps3_use_hard_reset_max_retry() > retries) {
		LOG_DEBUG("hno:%u  request IOC state to fault! retries:%d\n",
			PS3_HOST(instance), retries);

		instance->dump_context.is_hard_recovered = PS3_TRUE;
		if (instance->recovery_context->heartbeat_recovery != PS3_HEARTBEAT_HARDRESET_RECOVERY) {
			ret = instance->ioc_adpter->ioc_force_to_fault(instance);
			if (ret != PS3_SUCCESS) {
				LOG_WARN("hno:%u  hardreset handle force to fault NOK\n",
					PS3_HOST(instance));
				if (ret == -PS3_IN_PCIE_ERR) {
					goto l_out;
				}
			}
		} else {
			ps3_recovery_wait_reg_access_done(instance);
			if (instance->peer_instance != NULL) {
				ps3_recovery_wait_reg_access_done(instance->peer_instance);
			}
		}

		ioc_state = instance->ioc_adpter->ioc_state_get(instance);
		LOG_DEBUG("hno:%u  IOC state is %s! retries:%d\n",
			PS3_HOST(instance), ps3_ioc_state_print(ioc_state), retries);

		while(ps3_atomic_read(&instance->is_err_scsi_processing) > 0) {
			ps3_msleep(10);
		}
		instance->ioc_adpter->irq_disable(instance);
		INJECT_START(PS3_ERR_IJ_WAIT_IOCTL_IN_RECOVERY, instance)
#ifndef _WINDOWS
		ps3_irqs_sync(instance);
#endif

		INJECT_START(PS3_ERR_IJ_WAIT_READY, instance);
		err_code = ps3_hard_recovery_to_ready(instance);
		if (err_code != PS3_RECOVERY_INTER_ERR_SUCCESS) {
			LOG_INFO("hno:%u  IOC state to ready failed! retries:%d\n",
				PS3_HOST(instance), retries);
			if (err_code == PS3_RECOVERY_INTER_ERR_NEED_RECOVERY) {
				retries++;
				continue;
			} else if (err_code == PS3_RECOVERY_INTER_ERR_PCIE_ERR) {
				goto l_out;
			} else {
				goto l_offline;
			}
		}
		LOG_INFO("hno:%u  IOC state to ready success! retries:%d\n",
			PS3_HOST(instance), retries);

		if (instance->peer_instance != NULL && \
			instance->peer_instance->recovery_function.hardreset_handle_wait_ready_cb != NULL) {
			err_code = instance->peer_instance->recovery_function.hardreset_handle_wait_ready_cb( \
				instance->peer_instance);
			if (err_code != PS3_RECOVERY_INTER_ERR_SUCCESS) {
				LOG_INFO("hno:%u  ioc state to ready failed! retries:%d\n",
					PS3_HOST(instance), retries);
				if (err_code == PS3_RECOVERY_INTER_ERR_NEED_RECOVERY) {
					retries++;
					continue;
				} else if (err_code == PS3_RECOVERY_INTER_ERR_PCIE_ERR) {
					goto l_out;
				} else {
					goto l_offline;
				}
			}
			LOG_INFO("hno:%u  ioc state to ready success! retries:%d\n",
				PS3_HOST(instance), retries);
		}

		INJECT_START(PS3_ERR_IJ_FWSTATE_REMOVE, instance)
		err_code = ps3_hard_reset_multi_unnormal_handle(instance);
		if (err_code != PS3_RECOVERY_INTER_ERR_SUCCESS) {
			LOG_INFO("driver loading or unloading err_code:%d\n",
				err_code);
			if (err_code == PS3_RECOVERY_INTER_ERR_SUSPEND_RESUME) {
				goto l_out;
			} else {
				goto l_offline;
			}
		}

		INJECT_START(PS3_ERR_IJ_WATCHDOG_WAIT_RUNNING, instance);
		err_code = ps3_hard_recovery_to_pre_operational(instance);
		if (err_code != PS3_RECOVERY_INTER_ERR_SUCCESS) {
			LOG_INFO("hno:%u  IOC state to pre-operatioal failed! retries:%d\n",
				PS3_HOST(instance), retries);
			if (err_code == PS3_RECOVERY_INTER_ERR_NEED_RECOVERY) {
				if(instance->peer_instance != NULL) {
					if (ps3_instance_state_transfer(instance->peer_instance, PS3_INSTANCE_STATE_READY,
						PS3_INSTANCE_STATE_RECOVERY) != PS3_SUCCESS) {
						LOG_WARN("hno:%u  transfer to recovery NOK!\n",
							PS3_HOST(instance->peer_instance));
						goto l_offline;
					}
				}
				retries++;
				continue;
			} else if (err_code == PS3_RECOVERY_INTER_ERR_PCIE_ERR) {
				goto l_out;
			} else {
				goto l_offline;
			}
		}
		LOG_INFO("hno:%u  IOC state to pre-operatioal success! retries:%d\n",
			PS3_HOST(instance), retries);

		INJECT_START(PS3_ERR_IJ_RECOVERY_WAIT_FUNC0_RUNNING_1, instance)
		if (instance->peer_instance != NULL && (PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance->peer_instance) == PS3_FALSE) && \
			instance->peer_instance->recovery_function.hardreset_handle_init_running_cb != NULL) {
			if(ps3_atomic_read(&instance->peer_instance->state_machine.state) != PS3_INSTANCE_STATE_DEAD){
				err_code = instance->peer_instance->recovery_function.hardreset_handle_init_running_cb( \
					instance->peer_instance);
				if (err_code != PS3_RECOVERY_INTER_ERR_SUCCESS) {
					LOG_INFO("hno:%u  IOC state to pre-operatioal failed! retries:%d\n",
						PS3_HOST(instance), retries);
					if (err_code == PS3_RECOVERY_INTER_ERR_NEED_RECOVERY) {
						if (ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_PRE_OPERATIONAL,
							PS3_INSTANCE_STATE_RECOVERY) != PS3_SUCCESS) {
							LOG_WARN("hno:%u  transfer to recovery NOK!\n",
								PS3_HOST(instance->peer_instance));
							goto l_offline;
						}
						retries++;
						continue;
					} else if (err_code == PS3_RECOVERY_INTER_ERR_PCIE_ERR) {
						goto l_out;
					} else {
						goto l_offline;
					}
				}
				LOG_INFO("hno:%u  IOC state to pre-operatioal success! retries:%d\n",
					PS3_HOST(instance), retries);
			}
		}

		err_code = ps3_hard_recovery_to_operational(instance);
		if (err_code != PS3_RECOVERY_INTER_ERR_SUCCESS) {
			LOG_INFO("hno:%u  IOC state to operatioal failed! retries:%d\n",
				PS3_HOST(instance), retries);
			if (err_code == PS3_RECOVERY_INTER_ERR_NEED_RECOVERY) {
				retries++;
				continue;
			} else if (err_code == PS3_RECOVERY_INTER_ERR_PCIE_ERR) {
				goto l_out;
			} else {
				goto l_offline;
			}
		}
		LOG_INFO("hno:%u  IOC state to operatioal success! retries:%d\n",
			PS3_HOST(instance), retries);

		if (instance->peer_instance != NULL && \
			instance->peer_instance->recovery_function.hardreset_handle_post_cb != NULL) {
			if (PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance->peer_instance) == PS3_FALSE) {
				if(ps3_atomic_read(&instance->peer_instance->state_machine.state) != PS3_INSTANCE_STATE_DEAD){
					err_code = instance->peer_instance->recovery_function.hardreset_handle_post_cb( \
						instance->peer_instance);
					if (err_code != PS3_RECOVERY_INTER_ERR_SUCCESS) {
						LOG_INFO("hno:%u  IOC state to operatioal failed! retries:%d\n",
							PS3_HOST(instance), retries);
						if (err_code == PS3_RECOVERY_INTER_ERR_NEED_RECOVERY) {
							retries++;
							continue;
						} else if (err_code == PS3_RECOVERY_INTER_ERR_PCIE_ERR) {
							goto l_out;
						} else {
							goto l_offline;
						}
					}
					LOG_INFO("hno:%u  IOC state to operatioal success! retries:%d\n",
						PS3_HOST(instance), retries);
				}
			}
		}
		break;
	}

	cur_state = ps3_atomic_read(&instance->state_machine.state);
	if ((cur_state != PS3_INSTANCE_STATE_OPERATIONAL ||
		ps3_use_hard_reset_max_retry() == retries) && !PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance)) {
		LOG_ERROR("hno:%u  hard reset failed! cur_state:%s\n",
			PS3_HOST(instance),
			namePS3InstanceState(cur_state));
		goto l_offline;
	}

	ret = ps3_recovery_complete(instance);
	if(ret != PS3_SUCCESS){
		LOG_INFO("hno:%u  hard reset opeational but complete failed! cur_state:%s,ret %d\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state),ret);
		if (ret == -PS3_IN_PCIE_ERR) {
			goto l_out;
		} else {
			goto l_offline;
		}
	}

	if (instance->peer_instance == NULL || \
			instance->peer_instance->recovery_function.hardreset_handle_finish_cb == NULL) {
		ps3_hard_recovery_state_finish(instance);
		LOG_INFO("hno:%u  hard reset finish! cur_state:%s\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state));
	} else {
		if(PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance->peer_instance) == PS3_FALSE) {
			if(ps3_atomic_read(&instance->peer_instance->state_machine.state) != PS3_INSTANCE_STATE_DEAD){
				ret = instance->peer_instance->recovery_function.hardreset_handle_finish_cb( \
					instance->peer_instance);
			}
			if (ret != PS3_SUCCESS) {
				LOG_WARN("hno:%u  hardreset handle running cb NOK\n",
					PS3_HOST(instance->peer_instance));
				if (ret == -PS3_IN_PCIE_ERR) {
					goto l_out;
				} else {
					goto l_offline;
				}
			}
		} else {
			ps3_hard_recovery_state_finish(instance);
		}
	}
	ps3_ioc_can_hardreset_set(instance, PS3_IOC_CAN_HARDRESET);
	if (instance->peer_instance != NULL) {
		ps3_ioc_can_hardreset_set(instance->peer_instance, PS3_IOC_CAN_HARDRESET);
	}
	return ret;
l_offline:
	ps3_instance_state_transfer_to_dead(instance);
	if (instance->peer_instance != NULL && \
		instance->peer_instance->recovery_function.hardreset_handle_offline_cb != NULL) {
		instance->peer_instance->recovery_function.hardreset_handle_offline_cb( \
			instance->peer_instance);
	}
l_out:
	ps3_hard_recovery_state_finish(instance);
	if (ps3_pci_err_recovery_get(instance)) {
		ps3_mutex_lock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
		instance->recovery_context->heartbeat_recovery =
				PS3_HEARTBEAT_NULL;
		ps3_mutex_unlock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
	}
	LOG_WARN("hno:%u  hard reset finish! cur_state:%s\n",
		PS3_HOST(instance), namePS3InstanceState(cur_state));
	ret = -PS3_FAILED;
	return ret;
}

static S32 ps3_recovery_ready_to_force_cmd_stop(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	ps3_cmd_reset_flag_set(instance, PS3_CMD_FLAG_HARDRESET);

	while(ps3_atomic_read(&instance->is_err_scsi_processing) > 0) {
		ps3_msleep(10);
	}

	ps3_all_reply_fifo_complete(instance);

	ps3_cmd_force_stop(instance);

	LOG2_DEBUG("hno:%u force cmd stop end\n",
		PS3_HOST(instance));
	return ret;
}

static S32 ps3_recovery_ready_to_running(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	ps3_cmd_reset_flag_set(instance, PS3_CMD_FLAG_HARDRESET);

	while(ps3_atomic_read(&instance->is_err_scsi_processing) > 0) {
		ps3_msleep(10);
	}

	ps3_all_reply_fifo_complete(instance);

	ps3_cmd_force_stop(instance);
	if (!ps3_bit_pos_update(instance)) {
		LOG_ERROR("hno:%u update bit pos NOK\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}
	if ((ret = instance->ioc_adpter->ioc_init_proc(instance)) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u init IOC frame NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

l_out:
	LOG_DEBUG("hno:%u ready to running :%d\n",
		PS3_HOST(instance), ret);
	return ret;
}

static void ps3_can_queue_reset(struct ps3_instance *instance, U32 cur_max_fw_cmds)
{
	ULong flag = 0;
	LOG_INFO("hno:%u  IOC cur max fw cmd is %d!\n",
		PS3_HOST(instance), cur_max_fw_cmds);

	if (cur_max_fw_cmds <= instance->cmd_context.max_cmd_count) {
		instance->cmd_attr.cur_can_que = cur_max_fw_cmds -
			instance->cmd_context.max_mgr_cmd_count -
			instance->cmd_context.max_r1x_cmd_count;
		spin_lock_irqsave(instance->host->host_lock, flag);
		instance->host->can_queue = instance->cmd_attr.cur_can_que;
		LOG_INFO_IN_IRQ(instance,
			"hno:%u  IOC cur max fw cmd %d is less than %d, can queue is updated to %d!\n",
			PS3_HOST(instance), cur_max_fw_cmds,
			instance->cmd_context.max_cmd_count,
			instance->host->can_queue);
		spin_unlock_irqrestore(instance->host->host_lock, flag);
	}
}

static void ps3_cmd_reset_flag_set(struct ps3_instance *instance, U8 reset_flag)
{
	U32 i = 0;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd_context *context = &instance->cmd_context;
	ULong flags = 0;

	if (context->cmd_buf == NULL) {
		return;
	}

	for (i = 0; i < context->max_cmd_count; i++) {
		cmd = context->cmd_buf[i];
		ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
		if (cmd->cmd_state.state != PS3_CMD_STATE_INIT) {
			cmd->cmd_state.reset_flag = reset_flag;
		}
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	}
}

static Bool ps3_is_scsi_task_cmd(struct ps3_cmd *cmd)
{
	U8 cmd_type = cmd->req_frame->word[0];

	if (cmd_type == PS3_CMD_SCSI_TASK_MANAGEMENT) {
		return PS3_TRUE;
	} else {
		return PS3_FALSE;
	}
}

void ps3_scsi_cmd_force_stop(struct ps3_instance *instance)
{
	S32 i = 0;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *peer_cmd = NULL;
	struct ps3_cmd_context *context = &instance->cmd_context;
	struct scsi_cmnd *s_cmd = NULL;
	struct ps3_scsi_priv_data *data = NULL;
	U16 count = 0;
#ifndef _WINDOWS
	S32 ret_code = 0 ;

	if (ps3_pci_err_recovery_get(instance)) {
		ret_code = PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT);
	} else {
		ret_code = ps3_get_requeue_or_reset();
	}
#endif
	ps3_mutex_lock(&instance->recovery_context->free_cmd_lock);
	for (i = 0; i < instance->cmd_attr.cur_can_que; i++) {
		cmd = context->cmd_buf[i];
		if ((cmd->cmd_state.state == PS3_CMD_STATE_INIT) ||
			(cmd->cmd_state.state == PS3_CMD_STATE_COMPLETE)) {
			continue;
		}

		LOG_INFO("hno:%u force stop scsi CFID[%d] [%u]\n",
			PS3_HOST(instance), i, cmd->cmd_state.state);

		s_cmd = cmd->scmd;
		count++;

		PS3_IO_OUTSTAND_DEC(instance, s_cmd);
		PS3_VD_OUTSTAND_DEC(instance, s_cmd);
		PS3_DEV_IO_OUTSTAND_DEC(instance, cmd);
		PS3_IOC_DRV2IOC_BACK_INC(cmd->instance, cmd, PS3_REPLY_WORD_FLAG_FAIL);
		ps3_qos_cmd_update(instance, cmd);

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
				PS3_HOST(instance), i);
		}
		peer_cmd = cmd->r1x_peer_cmd;
		ps3_scsi_cmd_free(cmd);
		if (peer_cmd != NULL) {
			LOG_DEBUG("hno:%u force stop r1x scsi CFID[%d] and CFID[%d]\n",
				PS3_HOST(instance), i, peer_cmd->index);
			PS3_IOC_DRV2IOC_BACK_INC(peer_cmd->instance, peer_cmd, PS3_REPLY_WORD_FLAG_FAIL);
			ps3_r1x_peer_cmd_free_nolock(peer_cmd);
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
	}
	ps3_mutex_unlock(&instance->recovery_context->free_cmd_lock);
	LOG_WARN("hno:%u force stop scsi cmd count[%u]\n",
		PS3_HOST(instance), count);

	return;
}

void ps3_mgr_cmd_force_stop(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *abort_cmd = NULL;
	struct ps3_cmd_context *context = &instance->cmd_context;
	ULong cmd_lock_flags = 0;

	for (i = context->max_scsi_cmd_count; i < context->max_cmd_count; i++) {
		cmd = context->cmd_buf[i];
		ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &cmd_lock_flags);
		if ((cmd->cmd_state.state == PS3_CMD_STATE_INIT) ||
			(cmd->cmd_state.state == PS3_CMD_STATE_COMPLETE)) {
			LOG_FILE_WARN("hno:%u init or complete CFID:%d\n", PS3_HOST(instance), i);
			ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, cmd_lock_flags);
			continue;
		}

		if (cmd->cmd_state.state == PS3_CMD_STATE_DEAD) {
			LOG_FILE_WARN("hno:%u force free CFID:%d\n", PS3_HOST(instance), i);
			PS3_MGR_CMD_BACK_INC(instance, cmd, PS3_REPLY_WORD_FLAG_FAIL);

			if (PS3_MGR_CMD_TYPE(cmd) == PS3_CMD_IOCTL){
				LOG_FILE_WARN("hno:%u force free CFID:%d ioctl buff\n", PS3_HOST(instance), i);
				ps3_ioctl_buff_release(cmd);
			}
			if (cmd == instance->event_context.event_cmd) {
				instance->event_context.event_cmd = NULL;
			}
			ps3_mgr_cmd_free_nolock(instance, cmd);

			ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, cmd_lock_flags);
			continue;
		}

		if (cmd->req_frame->mgrReq.reqHead.noReplyWord == PS3_CMD_WORD_NO_REPLY_WORD) {
			LOG_FILE_WARN("hno:%u force complete no reply word CFID:%d\n",
				PS3_HOST(instance), i);
			if (ps3_is_scsi_task_cmd(cmd)) {
				cmd->resp_frame->normalRespFrame.respStatus = SCSI_STATUS_GOOD;
			} else {
				cmd->resp_frame->normalRespFrame.respStatus = PS3_DRV_MGR_BUSY;
			}
			PS3_MGR_CMD_BACK_INC(instance, cmd, PS3_REPLY_WORD_FLAG_FAIL);
			ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, cmd_lock_flags);
			continue;
		}

		if (ps3_is_scsi_task_cmd(cmd)) {
			cmd->resp_frame->normalRespFrame.respStatus = SCSI_STATUS_GOOD;
			PS3_MGR_CMD_BACK_INC(instance, cmd, PS3_REPLY_WORD_FLAG_FAIL);
			cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
			complete(&cmd->sync_done);
			LOG_FILE_WARN("hno:%u force complete task mgr CFID:%d\n",
				PS3_HOST(instance), i);
			ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, cmd_lock_flags);
			continue;
		}

		if (cmd->req_frame->mgrReq.syncFlag == 1) {
			PS3_MGR_CMD_BACK_INC(instance, cmd, PS3_REPLY_WORD_FLAG_FAIL);
			cmd->resp_frame->normalRespFrame.respStatus = PS3_DRV_MGR_BUSY;
			cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
			LOG_FILE_WARN("hno:%u force complete sync mgr CFID:%d\n",
				PS3_HOST(instance), i);
			complete(&cmd->sync_done);
		} else {
			if (instance->is_need_event) {
			    if (cmd == instance->event_context.event_cmd ||
				    cmd == instance->dev_context.vd_pending_cmd ||
				    cmd == instance->webSubscribe_context.webSubscribe_cmd) {
				    ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, cmd_lock_flags);
				    LOG_INFO("trace_id[0x%llx], hno:%u Event cmd:%d ignore!\n",
					    cmd->trace_id, PS3_HOST(instance), cmd->index);
				    continue;
			    }
            }
			LOG_FILE_WARN("hno:%u force dead CFID:%d\n",
				PS3_HOST(instance), i);
			cmd->cmd_state.state = PS3_CMD_STATE_DEAD;
		}
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, cmd_lock_flags);
	}
	if (instance->is_need_event) {
		if(instance->event_context.event_abort_cmd != NULL){
			abort_cmd = instance->event_context.event_abort_cmd;
			instance->event_context.event_abort_cmd = NULL;
		}
		if(abort_cmd != NULL){
			ps3_task_cmd_free(instance, abort_cmd);
		}

		if(instance->dev_context.vdpending_abort_cmd != NULL){
			abort_cmd = instance->dev_context.vdpending_abort_cmd;
			instance->dev_context.vdpending_abort_cmd = NULL;
		}
		if(abort_cmd != NULL){
			ps3_task_cmd_free(instance, abort_cmd);
		}

        if(instance->webSubscribe_context.web_abort_cmd != NULL){
            abort_cmd = instance->webSubscribe_context.web_abort_cmd;
            instance->webSubscribe_context.web_abort_cmd = NULL;
        }
        if(abort_cmd != NULL){
            ps3_task_cmd_free(instance, abort_cmd);
        }
	}
}

static void ps3_cmd_force_stop(struct ps3_instance *instance)
{
	S32 ret_code = 0;
	struct ps3_cmd_context *context = &instance->cmd_context;
	if (context->cmd_buf == NULL) {
		return;
	}

	if (ps3_pci_err_recovery_get(instance)) {
		ret_code = PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT);
	} else {
		ret_code = ps3_get_requeue_or_reset();
	}

	ps3_r1x_conflict_queue_clean_all(instance, ret_code, PS3_FALSE);

    ps3_qos_hard_reset(instance);
#ifdef PS3_SUPPORT_INJECT
	ps3_inject_clear();
#endif
	ps3_scsi_cmd_force_stop(instance);
	ps3_mgr_cmd_force_stop(instance);
}

Bool ps3_is_need_hard_reset(struct ps3_instance *instance)
{
	Bool need_hardreset = PS3_FALSE;
	Bool is_support_halt = PS3_FALSE;
	U32 hard_reset_enable = 0;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;
	if (!ps3_feature_support_reg_get(instance)) {
		goto l_out;
	}

	is_support_halt = PS3_IOC_STATE_HALT_SUPPORT(instance);

	LOG_INFO("hno:%u  instance state not support hardreset!\n",
		PS3_HOST(instance));
	if(is_support_halt && (cur_state != PS3_INSTANCE_STATE_DEAD)){
		ps3_atomic_set(&instance->state_machine.state,
			PS3_INSTANCE_STATE_DEAD);
		instance->ioc_adpter->ioc_force_to_halt(instance);
		LOG_INFO("hno:%u  instance state while support halt!\n",
			PS3_HOST(instance));
		while (1) {
			ps3_msleep(10);
		}
	}

	if(PS3_INSTANCE_ABNORMAL_FORCE_HARD_RECOVERY(instance)) {
		need_hardreset = PS3_TRUE;
		goto l_out;
	}

	hard_reset_enable = ps3_hard_reset_enable_query();
	cur_state = ps3_atomic_read(&instance->state_machine.state);

	LOG_WARN("hno[%u], ready to hard reset,instance state: %s\n",
		PS3_HOST(instance), namePS3InstanceState(cur_state));
	if((!PS3_IOC_HARD_RECOVERY_SUPPORT(instance))
		|| (!hard_reset_enable)){
		LOG_INFO("hno:%u  instance state not support hardreset!\n",
			PS3_HOST(instance));
		ps3_instance_state_transfer_to_dead(instance);
	} else {
		need_hardreset = PS3_TRUE;
	}

l_out:
	return need_hardreset;
}

S32 ps3_hard_reset_to_ready_with_doorbell(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	if (instance->peer_instance != NULL &&
		instance->recovery_context->parall_hardreset_state == PS3_PARALLEL_HARDRESET_STATE_PENDING) {
		instance->recovery_context->parall_hardreset_state = PS3_PARALLEL_HARDRESET_STATE_CONTINUE;
		while (instance->recovery_context->parall_hardreset_state != PS3_PARALLEL_HARDRESET_STATE_INIT) {
			ps3_msleep(PS3_PARALLEL_HARDRESET_STATE_WAIT_INIT_INTERVAL);
		}
	} else {
		ret = ps3_hard_recovery_request(instance);
		if (ret != PS3_SUCCESS){
			LOG_WARN("hno:%u  hard recovery request NOK\n",
				PS3_HOST(instance));
			goto l_out;
		}
	}
	ps3_recovery_cancel_work_sync(instance);

l_out:
	return ret;
}
S32 ps3_init_fail_hard_reset_with_doorbell(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	LOG_WARN("hno[%u], ready to hard reset\n", PS3_HOST(instance));
	if (instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_SHALLOW){
		goto l_out;
	}

	if (instance->peer_instance != NULL &&
		instance->recovery_context->parall_hardreset_state == PS3_PARALLEL_HARDRESET_STATE_PENDING) {
		instance->recovery_context->parall_hardreset_state = PS3_PARALLEL_HARDRESET_STATE_CONTINUE;
		while (instance->recovery_context->parall_hardreset_state != PS3_PARALLEL_HARDRESET_STATE_INIT) {
			ps3_msleep(PS3_PARALLEL_HARDRESET_STATE_WAIT_INIT_INTERVAL);
		}
	} else {
		ret = ps3_hard_recovery_request_with_retry(instance);
		if (ret != PS3_SUCCESS){
			LOG_WARN("hno:%u  hard recovery request NOK\n",
				PS3_HOST(instance));
			goto l_out;
		}
	}
	ps3_recovery_cancel_work_sync(instance);

	if (instance->ioc_adpter->ioc_state_get(instance) != PS3_FW_STATE_READY) {
		ret = -PS3_FAILED;
	}

	LOG_WARN("device[%d] hard reset success,exit init proc %d.\n", instance->pdev->dev.id, ret);
	return ret;
l_out:
	 if (ps3_recovery_is_state_halt(instance)) {
		LOG_WARN("hno:%u  driver_state:DEAD or HALT now !!!\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_end;
	}

	if ((!PS3_IOC_HARD_RECOVERY_SUPPORT(instance))
		|| (!ps3_hard_reset_enable_query())) {
		LOG_ERROR("hno:%u  soc feature unsupport Hard reset! need to be offline!\n",
			PS3_HOST(instance));

		ret = -PS3_FAILED;
		ps3_msleep(PS3_DEFAULT_MGR_CMD_TIMEOUT * 1000);
		goto l_end;
	}

	ret = instance->ioc_adpter->ioc_force_to_fault(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("device[%d] doorbell fault NOK.\n", instance->pdev->dev.id);
	}

	if (!instance->ioc_adpter->ioc_hard_reset) {
		ret = -PS3_FAILED;
		LOG_ERROR("device[%d] ioc_hard_reset is null.\n", instance->pdev->dev.id);
		goto l_end;
	}
	ret = instance->ioc_adpter->ioc_hard_reset(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("device[%d] hard reset NOK,exit init proc.\n", instance->pdev->dev.id);
		goto l_end;
	}
	LOG_WARN("device[%d] hard reset success,exit init proc.\n", instance->pdev->dev.id);
	ret = -PS3_FAILED;
l_end:
	return ret;
}

S32 ps3_hardreset_handle_pre(struct ps3_instance *instance)
{
	U32 wait_count = 0;
	ULong flags = 0;

	LOG_DEBUG("hno:%u functoin[%d], hardreset handle pre start\n",
			PS3_HOST(instance), ps3_get_pci_function(instance->pdev));

	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_1, instance)
	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN("hno:%u  pci recovery resetting\n",
			PS3_HOST(instance));
		return -PS3_IN_PCIE_ERR;
	}
	if(!PS3_IS_INSTANCE_PROBE_INIT(instance)) {

		if ((!PS3_IOC_HARD_RECOVERY_SUPPORT(instance))
			|| (!ps3_hard_reset_enable_query())) {
			LOG_ERROR("hno:%u  soc feature unsupport Hard reset! need to be offline!\n",
				PS3_HOST(instance));
			return -PS3_FAILED;
		}
	}
	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags);
	if (instance->event_context.abort_eventcmd != 0) {
		instance->event_context.abort_eventcmd = 0;
	}

	if (instance->hardreset_event == 0) {
		instance->hardreset_event = 1;
	}
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags);

	if (PS3_IS_INSTANCE_PROBE(instance) && instance->is_scan_host_finish == PS3_TRUE
			&& instance->is_probe_finish == PS3_FALSE) {
		LOG_INFO("hno:%u  recovery wait until probe finish/failed\n",
				PS3_HOST(instance));
		while (wait_count < PS3_RECOVERY_WAIT_PROBE_FINISH_LOOP_COUNT) {
			if (instance->is_probe_finish || instance->is_probe_failed) {
				break;
			}

			wait_count++;
			ps3_msleep(PS3_RECOVERY_WAIT_LOOP_TIME_INTERVAL_20MS);
		}

		LOG_INFO("hno:%u  probe finish(%d)/failed(%d), continue recovery\n",
				PS3_HOST(instance), instance->is_probe_finish, instance->is_probe_failed);
	}

	ps3_wait_watchdog_dect_recovery(instance);

	LOG_DEBUG("hno:%u functoin[%d], hardreset handle pre end\n",
			PS3_HOST(instance), ps3_get_pci_function(instance->pdev));

	return PS3_SUCCESS;
}

S32 ps3_hardreset_handle_wait_ready(struct ps3_instance *instance)
{
	S32 err_code = PS3_RECOVERY_INTER_ERR_SUCCESS;
	U32 ioc_state = 0;
	S32 ret = PS3_SUCCESS;

	LOG_DEBUG("hno:%u function[%d], hardreset handle wait ready start.\n",
		PS3_HOST(instance), ps3_get_pci_function(instance->pdev));

	instance->dump_context.is_hard_recovered = PS3_TRUE;
	ioc_state = instance->ioc_adpter->ioc_state_get(instance);
	LOG_DEBUG("hno:%u  ioc state is %s! function:%d\n",
		PS3_HOST(instance), ps3_ioc_state_print(ioc_state), ps3_get_pci_function(instance->pdev));

	while(ps3_atomic_read(&instance->is_err_scsi_processing) > 0) {
		ps3_msleep(10);
	}
	instance->ioc_adpter->irq_disable(instance);
#ifndef _WINDOWS
	ps3_irqs_sync(instance);
#endif

	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_6, instance)
	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN("hno:%u  pci recovery resetting\n", PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_PCIE_ERR;
		goto l_out;
	}

	INJECT_START(PS3_ERR_IJ_HARD_WAIT_READY_FAILED_F1, instance);
	if ((ret = ps3_ioc_state_transfer_to_ready(instance)) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  hard reset to ready NOK,%s!\n",
			PS3_HOST(instance),
			ps3_ioc_state_print(ioc_state));
		if (ret == -PS3_IN_PCIE_ERR) {
			err_code = PS3_RECOVERY_INTER_ERR_PCIE_ERR;
		} else {
			if (ps3_instance_state_transfer(instance->peer_instance, PS3_INSTANCE_STATE_READY,
				PS3_INSTANCE_STATE_RECOVERY) != PS3_SUCCESS) {
				LOG_WARN("hno:%u  transfer to recovery NOK!\n",
					PS3_HOST(instance->peer_instance));
				err_code = PS3_RECOVERY_INTER_ERR_INTERRUPT;
				goto l_out;
			}
			err_code = PS3_RECOVERY_INTER_ERR_NEED_RECOVERY;
		}
		goto l_out;
	}
	if (instance->state_machine.is_load && ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_RECOVERY,
		PS3_INSTANCE_STATE_READY) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  transfer to ready NOK!\n",
			PS3_HOST(instance));
		err_code = PS3_RECOVERY_INTER_ERR_INTERRUPT;
	}

l_out:
	LOG_DEBUG("hno:%u function[%d], hardreset handle wait ready end.\n",
		PS3_HOST(instance), ps3_get_pci_function(instance->pdev));

	return err_code;
}

S32 ps3_hardreset_handle_init_running(struct ps3_instance *instance)
{
	U32 err_code = PS3_RECOVERY_INTER_ERR_SUCCESS;

	if (!instance->is_probe_finish || instance->is_probe_failed ||
			!instance->state_machine.is_load) {
		ps3_recovery_ready_to_force_cmd_stop(instance);
		LOG_ERROR("hno:%u  reset while probe or in shutdown/remove,"
			" finish[%d], failed[%d], is_load[%d]\n",
			PS3_HOST(instance), instance->is_probe_finish,
			instance->is_probe_failed,
			instance->state_machine.is_load);
		ps3_instance_state_transfer(instance, PS3_INSTANCE_STATE_READY,
			PS3_INSTANCE_STATE_RECOVERY);
		instance ->is_half_hard_reset = PS3_TRUE;
		if (!instance->state_machine.is_load) {
			ps3_ioc_notify_unload(instance);
		}
		goto l_out;
	}

	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_9, instance)
	err_code = ps3_hard_recovery_to_pre_operational(instance);
l_out:
	return err_code;
}

S32 ps3_hardreset_handle_post(struct ps3_instance *instance)
{
	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_12, instance)
	return ps3_hard_recovery_to_operational(instance);
}

S32 ps3_hardreset_handle_finish(struct ps3_instance *instance)
{
	U32 cur_state = 0;
	S32 ret = PS3_SUCCESS;

	cur_state = ps3_atomic_read(&instance->state_machine.state);
	if (cur_state != PS3_INSTANCE_STATE_OPERATIONAL) {
		ps3_instance_state_transfer_to_dead(instance);
		LOG_ERROR("hno:%u  hard reset NOK! cur_state:%s\n",
			PS3_HOST(instance),
			namePS3InstanceState(cur_state));
		return -PS3_FAILED;
	}

	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_15, instance)
	ret = ps3_recovery_complete(instance);
	if(ret != PS3_SUCCESS){
		LOG_INFO("hno:%u  hard reset opeational but complete failed! cur_state:%s,ret %d\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state),ret);
		return ret;
	}

	ps3_hard_recovery_state_finish(instance);
	LOG_INFO("hno:%u  hard reset finish! cur_state:%s\n",
		PS3_HOST(instance), namePS3InstanceState(cur_state));

	return PS3_SUCCESS;
}

S32 ps3_hardreset_handle_offline(struct ps3_instance *instance)
{
	U32 cur_state = 0;
	cur_state = ps3_atomic_read(&instance->state_machine.state);
	ps3_instance_state_transfer_to_dead(instance);
	LOG_ERROR("hno:%u  hard reset finish! cur_state:%s\n",
		PS3_HOST(instance), namePS3InstanceState(cur_state));
	return PS3_SUCCESS;
}

S32 ps3_softreset_handle_pre(struct ps3_instance *instance)
{
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;

	ps3_cmd_reset_flag_set(instance, PS3_CMD_FLAG_SOFTRESET);

	ret = ps3_ioc_state_transfer_wait_to_running(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  soft recovery to running failed!\n",
			PS3_HOST(instance));
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}
	ret = ps3_soft_recovery_cmd_reply_polling_check(instance, PS3_SOFT_RESET_WAIT_TIMEOUT);
	if (ret != PS3_RECOVERY_INTER_ERR_SUCCESS) {
		LOG_ERROR("hno:%u  pending mgr cmd no reply all!\n",
			PS3_HOST(instance));
		goto l_out;
	}

	ret = PS3_RECOVERY_INTER_ERR_SUCCESS;
	LOG_INFO("hno:%u  soft recovery to pre-operational success!\n",
		PS3_HOST(instance));
l_out:
	return ret;
}

S32 ps3_softreset_handle_post(struct ps3_instance *instance)
{
	S32 ret = PS3_RECOVERY_INTER_ERR_SUCCESS;

	if (!ps3_ioc_recovery_count_get(instance,
		&instance->recovery_context->ioc_recovery_count)) {
		ret = PS3_RECOVERY_INTER_ERR_FAILED;
		goto l_out;
	}

	LOG_INFO("hno:%u  pre-operational to operational success!,reset count:%d\n",
		PS3_HOST(instance), instance->recovery_context->ioc_recovery_count);
l_out:
	return ret;
}

S32 ps3_hard_recovery_request_with_retry(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 count = 0;

	while ((ret = ps3_hard_recovery_request(instance)) == -PS3_RETRY) {
		if (count++ == ps3_use_hard_reset_max_retry()) {
			ret = -PS3_FAILED;
			break;
		}
		ps3_mdelay(10);
	}

	return ret;
}
S32 ps3_recovery_request_with_retry(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 count = 0;

	while ((ret = ps3_recovery_request(instance)) == -PS3_RETRY) {
		if (count++ == ps3_use_hard_reset_max_retry()) {
			ret = -PS3_FAILED;
			break;
		}
		ps3_mdelay(10);
	}

	return ret;
}