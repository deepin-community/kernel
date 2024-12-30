#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#include <scsi/scsi_host.h>
#endif

#include "ps3_instance_manager.h"
#include "ps3_ioc_state.h"
#include "ps3_mgr_cmd_err.h"
#include "ps3_watchdog.h"
#include "ps3_driver_log.h"
#include "ps3_dump.h"
#include "ps3_module_para.h"
#include "ps3_ioc_manager.h"
#include "ps3_err_inject.h"

#define PS3_WATCHDOG_NAME_MAX_LENGTH		(48)	

static S32 ps3_watchdog_fault_detect_and_recovery(struct ps3_instance *instance)
{
	U32 ioc_recovery_count = 0;
	U32 ioc_state = PS3_FW_STATE_UNDEFINED;
	S32 ret = PS3_SUCCESS;

	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN_LIM("hno:%u  pci recovery resetting\n",
			PS3_HOST(instance));
		goto l_out;
	}

	if (!ps3_ioc_recovery_count_get(instance, &ioc_recovery_count)) {
		ps3_atomic_inc(&instance->watchdog_reg_read_fail_count);
		LOG_ERROR_LIM("hno:%u  get recovery count NOK, cnt: %d\n",
			PS3_HOST(instance), ps3_atomic_read(&instance->watchdog_reg_read_fail_count));
		goto l_out;
	}

	if (!ps3_ioc_state_get_with_check(instance, &ioc_state)) {
		ps3_atomic_inc(&instance->watchdog_reg_read_fail_count);
		LOG_ERROR_LIM("hno:%u  get ioc state NOK, cnt: %u\n",
			PS3_HOST(instance), ps3_atomic_read(&instance->watchdog_reg_read_fail_count));
		goto l_out;
	}

	ps3_atomic_set(&instance->watchdog_reg_read_fail_count, 0);

	if (instance->ioc_adpter->ioc_heartbeat_detect) {
		if ((instance->recovery_context->heartbeat_recovery == PS3_HEARTBEAT_HARDRESET_DECIDE ||
			instance->recovery_context->heartbeat_recovery == PS3_HEARTBEAT_HARDRESET_RECOVERY)
			&&(instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_SHALLOW ||
			instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE)) {
			goto l_out;
		}

		if (instance->ioc_adpter->ioc_heartbeat_detect(instance) == PS3_TRUE
				&& !ps3_pci_err_recovery_get(instance)) {
			LOG_INFO("hno:%u heartbeat recovery triggered\n",
				PS3_HOST(instance));
			ps3_mutex_lock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
			if (instance->recovery_context->heartbeat_recovery == PS3_HEARTBEAT_NULL) {
				instance->recovery_context->heartbeat_recovery =
					PS3_HEARTBEAT_HARDRESET_DECIDE;
			}
			ps3_mutex_unlock(&instance->recovery_context->ps3_watchdog_recovery_mutex);
			goto l_recovery;
		}
	}

	if (ioc_state == PS3_FW_STATE_FAULT ||
		ioc_state == PS3_FW_STATE_CRITICAL) {
		goto l_recovery;
	}

	if ((ioc_state == PS3_FW_STATE_RUNNING) &&
		(instance->recovery_context->ioc_recovery_count ==
			ioc_recovery_count)) {
		goto l_out;
	}

	if (ioc_state == PS3_FW_STATE_RESET) {
		goto l_out;
	}

	if (ioc_recovery_count == PS3_IOC_RECOVERY_COUNT_MASK) {
		goto l_out;
	}

	if (ioc_state == PS3_FW_STATE_UNDEFINED) {
		goto l_out;
	}

	if (ioc_state == PS3_FW_STATE_MASK) {
		goto l_out;
	}

l_recovery:
	LOG_INFO("hno:%u  watchdog recovery request, ioc_state[0x%x],"
		" ioc_reco_cnt[%u], local_reco_cnt[%u]\n",
		PS3_HOST(instance), ioc_state, ioc_recovery_count,
		instance->recovery_context->ioc_recovery_count);

	PS3_IJ_SLEEP(200000, PS3_ERR_IJ_WATCHDOG_DELAY);

	if (ps3_atomic_read(&instance->state_machine.state) != PS3_INSTANCE_STATE_DEAD) {
		if (ps3_need_block_hard_reset_request(instance) || PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance) || \
			instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_SHALLOW || \
			instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE) {
			LOG_WARN_LIM("hno:%u  can not start reset request recovery_state:%d\n",
				PS3_HOST(instance), instance->recovery_context->recovery_state);
		} else {
			if(ps3_recovery_request(instance) == PS3_SUCCESS){
				LOG_INFO("hno[%u], recovery success!\n",
					PS3_HOST(instance));
				ret = PS3_SUCCESS;
			}else{
				LOG_ERROR("hno[%u], recovery request NOK, %s!\n",
					PS3_HOST(instance),
					namePS3InstanceState(ps3_atomic_read(&instance->state_machine.state)));
				return -PS3_FAILED;
			}
		}
	}

l_out:
	if (ps3_atomic_read(&instance->watchdog_reg_read_fail_count) >= PS3_TRANS_DEAD_IOC_FAILED_MAX_COUNT) {
#ifdef PS3_HARDWARE_SIM
		LOG_WARN("hno:%u  reg read failed 5 times consecutively, simu platform ignore\n",
			PS3_HOST(instance));
#else
		ps3_instance_state_transfer_to_dead(instance);
		instance->watchdog_context.is_stop = PS3_TRUE;
		LOG_ERROR("hno:%u  reg read failed 5 times consecutively, transfer to dead\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
#endif
	}

	return ret;
}

#ifndef _WINDOWS

static void ps3_watchdog_service(struct work_struct* work)
{
	struct ps3_watchdog_context* watchdog_ctx =
		ps3_container_of(work, struct ps3_watchdog_context,
			watchdog_work.work);
	struct ps3_instance *instance = watchdog_ctx->instance;
	struct delayed_work* delayed_work = &watchdog_ctx->watchdog_work;
	S32 ret = PS3_SUCCESS;

	if (instance->recovery_context->recovery_state ==
			PS3_HARD_RECOVERY_SHALLOW ||
		instance->recovery_context->recovery_state ==
			PS3_HARD_RECOVERY_DECIDE) {
		watchdog_ctx->is_halt = PS3_TRUE;
		goto l_next;
	}

	watchdog_ctx->is_halt = PS3_FALSE;
	if (instance->is_probe_finish && instance->state_machine.is_load) {
		ret = ps3_watchdog_fault_detect_and_recovery(instance);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u  watchdog fault detect and recovery \
				NOK watchdog service exit\n",
				PS3_HOST(instance));
		}
	}
l_next:

	if (watchdog_ctx->is_stop) {
		LOG_INFO("hno:%u  watchdog has stop\n",
			PS3_HOST(instance));
		return;
	}

    ps3_qos_waitq_poll(instance);

	if (watchdog_ctx->watchdog_queue != NULL) {
		queue_delayed_work(watchdog_ctx->watchdog_queue, delayed_work,
			msecs_to_jiffies(PS3_WATCHDOG_INTERVAL));
	}
}

S32 ps3_watchdog_start(struct ps3_instance *instance)
{
	struct ps3_watchdog_context* watchdog_ctx = &instance->watchdog_context;
	char watchdog_queue_name[PS3_WATCHDOG_NAME_MAX_LENGTH];
	struct delayed_work* delayed_work = &watchdog_ctx->watchdog_work;
	ps3_atomic_inc(&instance->watchdog_ref);
	mb();

	if (PS3_FALSE == watchdog_ctx->is_stop) {
		LOG_DEBUG("hno:%u watchdog running\n", PS3_HOST(instance));
		ps3_atomic_dec(&instance->watchdog_ref);
		return PS3_SUCCESS;
	}

	if (watchdog_ctx->watchdog_queue != NULL) {
		LOG_DEBUG("hno:%u  watchdog for is already started!\n",
			PS3_HOST(instance));
		ps3_atomic_dec(&instance->watchdog_ref);
		goto l_out;
	}

	memset(watchdog_queue_name, 0, PS3_WATCHDOG_NAME_MAX_LENGTH);
	memset(watchdog_ctx, 0, sizeof(struct ps3_watchdog_context));
	INIT_DELAYED_WORK(delayed_work, ps3_watchdog_service);

	snprintf(watchdog_queue_name, PS3_WATCHDOG_NAME_MAX_LENGTH,
		"ps3_watchdog_service_host%d", instance->host->host_no);

	watchdog_ctx->watchdog_queue =
		create_singlethread_workqueue(watchdog_queue_name);
	if (watchdog_ctx->watchdog_queue == NULL) {
		LOG_ERROR("hno:%u  watchdog work queue create failed\n",
			PS3_HOST(instance));
		ps3_atomic_dec(&instance->watchdog_ref);
		return -PS3_FAILED;
	}

	watchdog_ctx->instance = instance;
	watchdog_ctx->is_stop = PS3_FALSE;
	queue_delayed_work(watchdog_ctx->watchdog_queue, delayed_work,
		msecs_to_jiffies(PS3_WATCHDOG_INTERVAL));
	ps3_atomic_dec(&instance->watchdog_ref);

l_out:
	return PS3_SUCCESS;
}

void ps3_watchdog_stop(struct ps3_instance *instance)
{
	struct ps3_watchdog_context* watchdog_ctx = &instance->watchdog_context;
	struct workqueue_struct* watchdog_queue;
	struct delayed_work* delayed_work = &watchdog_ctx->watchdog_work;

	if (PS3_TRUE == watchdog_ctx->is_stop) {
		return;
	}

	watchdog_ctx->is_stop = PS3_TRUE;
	mb(); 
	while(ps3_atomic_read(&instance->watchdog_ref) != 0) {
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
	}

	if (watchdog_ctx->watchdog_queue != NULL) {
		watchdog_queue = watchdog_ctx->watchdog_queue;
		if (!cancel_delayed_work_sync(delayed_work)) {
			flush_workqueue(watchdog_queue);
		}

		destroy_workqueue(watchdog_queue);
		watchdog_ctx->watchdog_queue = NULL;
		LOG_INFO("hno:%u  watchdog destory work and stop\n",
			PS3_HOST(instance));
	}
}
#else

static void ps3_watchdog_service(void *ins)
{
	struct ps3_instance *instance = (struct ps3_instance*)ins;
	struct ps3_watchdog_context* watchdog_ctx = &instance->watchdog_context;
	struct ps3_delay_worker* watchdog_work = &watchdog_ctx->watchdog_work;
	S32 ret = PS3_SUCCESS;

	if(ps3_atomic_read(&instance->state_machine.state) !=
		PS3_INSTANCE_STATE_OPERATIONAL &&
		(instance->recovery_context->recovery_state ==
			PS3_HARD_RECOVERY_SHALLOW ||
		instance->recovery_context->recovery_state ==
			PS3_HARD_RECOVERY_DECIDE)) {
		watchdog_ctx->is_halt = PS3_TRUE;
		goto l_next;
	}

	watchdog_ctx->is_halt = PS3_FALSE;

	if (instance->is_probe_finish) {
		ret = ps3_watchdog_fault_detect_and_recovery(instance);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u  watchdog fault detect and recovery \
				failed watchdog service exit\n",
				PS3_HOST(instance));
		}
	}
l_next:

	if (watchdog_ctx->is_stop) {
		LOG_DEBUG("hno:%u  watchdog has stop\n",
			PS3_HOST(instance));
		return;
	}

	ps3_dump_detect(instance);

	ps3_delay_worker_start(watchdog_work, PS3_WATCHDOG_INTERVAL);
}

S32 ps3_watchdog_start(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_watchdog_context* watchdog_ctx = &instance->watchdog_context;
	struct ps3_delay_worker* watchdog_work = &watchdog_ctx->watchdog_work;

	memset(watchdog_ctx, 0, sizeof(struct ps3_watchdog_context));

	if (ps3_delay_worker_init(instance, watchdog_work, "ps3_watchdog_service", ps3_watchdog_service) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u timer init failed\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	watchdog_ctx->is_stop = PS3_FALSE;
	if (ps3_delay_worker_start(watchdog_work, PS3_WATCHDOG_INTERVAL) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u timer request failed\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		watchdog_ctx->is_stop = PS3_TRUE;
		goto l_out;
	}

l_out:
	return PS3_SUCCESS;
}

void ps3_watchdog_stop(struct ps3_instance *instance)
{
	struct ps3_watchdog_context* watchdog_ctx = &instance->watchdog_context;
	struct ps3_delay_worker* watchdog_work = &watchdog_ctx->watchdog_work;

	watchdog_ctx->is_stop = PS3_TRUE;
	ps3_delay_worker_exit(watchdog_work);

	memset(watchdog_ctx, 0, sizeof(struct ps3_watchdog_context));
	LOG_WARN("hno:%u  watchdog destory work and stop\n",
		PS3_HOST(instance));
}
#endif

