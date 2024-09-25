
#ifndef _PS3_MGR_CMD_ERR_H_
#define _PS3_MGR_CMD_ERR_H_

#ifndef _WINDOWS
#include <linux/mutex.h>
#include <linux/atomic.h>
#endif

#include "ps3_htp_def.h"
#include "ps3_instance_manager.h"

#define PS3_ERR_MGR_CMD_FAULT_RETRY_MAX		(3)	
#define PS3_ERR_MGR_CMD_DELAY_TIME_BEFORE_RERTY (500)	

void ps3_err_fault_context_init(struct ps3_instance *instance);
void ps3_err_fault_context_exit(struct ps3_instance *instance);

S32 ps3_err_mgr_cmd_proc(struct ps3_instance *instance,
	S32 fault_type, struct ps3_cmd *cmd);

S32 ps3_err_mgr_cmd_failed_check(struct ps3_instance *instance,
	struct ps3_cmd *cmd);

const S8 *ps3_err_mgr_fault_proc_result_print(S32 result);

#endif
