
#ifndef _PS3_RECOVERY_H_
#define _PS3_RECOVERY_H_

#ifndef _WINDOWS
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/workqueue.h>
#include <linux/irqreturn.h>
#else
#include "ps3_worker.h"
#endif

#include "ps3_platform_utils.h"
#include "ps3_inner_data.h"
#include "ps3_driver_log.h"
enum {
	PS3_SOFT_RECOVERY_PROBE_PROCESS = 0,
	PS3_SOFT_RECOVERY_SHALLOW = 1,
	PS3_SOFT_RECOVERY_DEEP = 2,
	PS3_SOFT_RECOVERY_IOC_RECOVERY = 3,
	PS3_SOFT_RECOVERY_FINISH = 4,
	PS3_HARD_RECOVERY_DECIDE = 5,
	PS3_HARD_RECOVERY_SHALLOW = 6,
	PS3_HARD_RECOVERY_FINISH = 7,
	PS3_RESET_LOG_INTERVAL = 8,
};

enum {
	PS3_HOST_RESET_INIT       = 0,
	PS3_HOST_RESET_START      = 1,
	PS3_HOST_RESET_HARD_RESET_DONE = 2, 
};
enum {
	PS3_PARALLEL_HARDRESET_STATE_INIT = 0,
	PS3_PARALLEL_HARDRESET_STATE_PENDING,
	PS3_PARALLEL_HARDRESET_STATE_CONTINUE,
};
#define PS3_PARALLEL_HARDRESET_STATE_WAIT_INIT_INTERVAL (5)
#define PS3_RECOVERY_CONTEXT_MAX_NUM (8)
#define PS3_RECOVERY_IRQ_NAME_MAX_LENTH (48)

enum {
	PS3_HEARTBEAT_NULL      = 0,		
	PS3_HEARTBEAT_HARDRESET_DECIDE = 1,	
	PS3_HEARTBEAT_HARDRESET_RECOVERY =2,
};

enum {
	PS3_IOC_CANNOT_HARDRESET = 0,   
	PS3_IOC_CAN_HARDRESET = 1,      
};

struct ps3_recovery_context
{
	ps3_mutex free_cmd_lock; 
	ps3_spinlock recovery_lock; 
	ps3_spinlock ps3_hardreset_lock;
	U32 ioc_recovery_count;  
	U8 recovery_state;
	U8 reserved1;
	U8 host_reset_state;
	U8 heartbeat_recovery; 
#ifndef _WINDOWS
	struct workqueue_struct *recovery_wq;
	S8 recovery_wq_name[20];
	struct work_struct recovery_work;
#else
	struct ps3_worker recovery_work;
#endif
	struct ps3_instance *instance[2];
	struct ps3_instance *work_instance;	
	S32 recovery_result; 
	U32 hardreset_count;		
	U8 parall_hardreset_state;
	U8 instance_change;
	U8 reserved[6];
	ps3_atomic32 hardreset_ref;
	ps3_mutex ps3_watchdog_recovery_mutex;	
};
#define PS3_IS_INTERRUPT_SOFT_RECOVERY(instance) \
	(instance->recovery_context->recovery_state == PS3_HARD_RECOVERY_DECIDE)

#define PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance) (PS3_IS_INSTANCE_PROBE(instance) || \
	PS3_IS_INSTANCE_REMOVE(instance) || PS3_IS_INSTANCE_SUSPEND_OR_RESUME(instance))

#define PS3_IS_INSTANCE_PROBE(instance) (!(instance)->state_machine.is_suspend && !((instance)->is_probe_finish || \
	(instance)->is_probe_failed))

#define PS3_IS_INSTANCE_REMOVE(instance) (!(instance)->state_machine.is_suspend && ((instance)->is_probe_finish || \
	(instance)->is_probe_failed) && !(instance)->state_machine.is_load)

#define PS3_INSTANCE_ABNORMAL_FORCE_HARD_RECOVERY(instance) (PS3_IS_INSTANCE_NOT_LOAD_NORMAL(instance) || \
		((instance)->peer_instance != NULL && PS3_IS_INSTANCE_NOT_LOAD_NORMAL((instance)->peer_instance)))

#define PS3_IS_INSTANCE_PROBE_INIT(instance) (!((instance)->is_probe_finish || (instance)->is_probe_failed) &&	\
	!(instance)->state_machine.is_load)

#define PS3_IS_INSTANCE_SUSPEND_OR_RESUME(instance) (PS3_IS_INSTANCE_SUSPEND(instance) || \
		PS3_IS_INSTANCE_RESUME(instance))

#define PS3_IS_INSTANCE_SUSPEND(instance) ((instance)->state_machine.is_suspend && \
	(instance)->is_suspend)
		
#define PS3_IS_INSTANCE_RESUME(instance) ((instance)->state_machine.is_suspend && \
	(instance)->is_resume)

S32 ps3_recovery_context_init(struct ps3_instance *instance);

void ps3_recovery_function_init(struct ps3_instance *instance);

void ps3_recovery_context_exit(struct ps3_instance *instance);

void ps3_recovery_clean(struct ps3_instance *instance);

void ps3_recovery_destory(struct ps3_instance *instance);

S32 ps3_recovery_request(struct ps3_instance *instance);

S32 ps3_hard_recovery_request(struct ps3_instance *instance);

void ps3_scsi_cmd_force_stop(struct ps3_instance *instance);
void ps3_mgr_cmd_force_stop(struct ps3_instance *instance);

S32 ps3_recovery_cancel_work_sync(struct ps3_instance *instance);

#ifndef _WINDOWS
S32 ps3_hard_reset_to_ready_with_doorbell(struct ps3_instance *instance);
S32 ps3_init_fail_hard_reset_with_doorbell(struct ps3_instance *instance);
Bool ps3_is_need_hard_reset(struct ps3_instance *instance);

void ps3_recovery_work_queue_destory(struct ps3_instance *instance);
#endif
Bool ps3_no_need_recovery(struct ps3_instance *instance);

S32 ps3_hardreset_handle_pre(struct ps3_instance *instance);

S32 ps3_hardreset_handle_wait_ready(struct ps3_instance *instance);

S32 ps3_hardreset_handle_init_running(struct ps3_instance *instance);

S32 ps3_hardreset_handle_post(struct ps3_instance *instance);

S32 ps3_hardreset_handle_finish(struct ps3_instance *instance);

S32 ps3_hardreset_handle_offline(struct ps3_instance *instance);

S32 ps3_softreset_handle_pre(struct ps3_instance *instance);

S32 ps3_softreset_handle_post(struct ps3_instance *instance);
S32 ps3_hard_recovery_request_with_retry(struct ps3_instance *instance);
S32 ps3_recovery_request_with_retry(struct ps3_instance *instance);
#ifndef _WINDOWS
irqreturn_t ps3_recovery_irq_handler(S32 virq, void *dev_id);
 #endif
S32 ps3_recovery_irq_start(struct ps3_instance *instance);
#endif 
