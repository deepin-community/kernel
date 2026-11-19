// SPDX-License-Identifier: GPL-2.0
#include "ps3_mgr_cmd_err.h"

#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#include <scsi/scsi.h>
#endif

#include "ps3_htp.h"

#include "ps3_mgr_channel.h"
#include "ps3_mgr_cmd.h"
#include "ps3_driver_log.h"
#include "ps3_recovery.h"

enum ps3_fault_action {
	PS3_FAULT_STRATEGY_NONE = 0,
	PS3_FAULT_STRATEGY_RETRY,
};

enum ps3_reset_type {
	PS3_RESET_TYPE_SOFT,
	PS3_RESET_TYPE_HARD,
};

struct ps3_fault_strategy {
	unsigned int resp_status;
	enum ps3_fault_action fault_action;
};

static struct ps3_fault_strategy ps3_fault_strategy_desc[] = {
	{ PS3_DRV_MGR_UNRUNING, PS3_FAULT_STRATEGY_NONE },
	{ PS3_DRV_MGR_INVAL_CMD, PS3_FAULT_STRATEGY_NONE },
	{ PS3_DRV_MGR_NORESOURCE, PS3_FAULT_STRATEGY_RETRY },
	{ PS3_DRV_MGR_INVAL_PARAM, PS3_FAULT_STRATEGY_NONE },
	{ PS3_DRV_MGR_DEV_NOEXIST, PS3_FAULT_STRATEGY_NONE },
	{ PS3_DRV_MGR_DEV_DATA_ERR, PS3_FAULT_STRATEGY_NONE },
	{ PS3_DRV_MGR_EVT_REPEAT, PS3_FAULT_STRATEGY_NONE },
	{ PS3_DRV_MGR_EVT_CANCEL_ERR, PS3_FAULT_STRATEGY_NONE },
	{ PS3_DRV_MGR_FLUSH_FAILED, PS3_FAULT_STRATEGY_RETRY },
	{ PS3_DRV_MGR_BUSY, PS3_FAULT_STRATEGY_RETRY },
	{ PS3_DRV_MGR_TIMEOUT, PS3_FAULT_STRATEGY_RETRY },
	{ PS3_DRV_MGR_SMP_BACKEND_ERR, PS3_FAULT_STRATEGY_NONE },
	{ PS3_DRV_MGR_LINK_GET_BACKEND_ERR, PS3_FAULT_STRATEGY_NONE },
	{ PS3_DRV_MGR_PHY_CTL_BACKEND_ERR, PS3_FAULT_STRATEGY_NONE },
	{ PS3_DRV_MGR_RESTART_COMMAND_RSP, PS3_FAULT_STRATEGY_NONE },
	{ PS3_DRV_MGR_TM_FAILED, PS3_FAULT_STRATEGY_NONE },
	{ U32_MAX, PS3_FAULT_STRATEGY_NONE }
};

static inline const char *ps3_err_action_print(enum ps3_fault_action action)
{
	static const char * const action_string[] = {
		[PS3_FAULT_STRATEGY_NONE] = "PS3_FAULT_STRATEGY_NONE",
		[PS3_FAULT_STRATEGY_RETRY] = "PS3_FAULT_STRATEGY_RETRY"
	};

	return action_string[action];
}

const char *ps3_err_mgr_fault_proc_result_print(int result)
{
	const char *proc_result_string = NULL;

	switch (result) {
	case PS3_SUCCESS:
		proc_result_string = "SUCCESS";
		break;
	case -PS3_RETRY:
		proc_result_string = "NEED RETRY";
		break;
	case -PS3_FAILED:
		proc_result_string = "FAILED";
		break;
	case -PS3_TIMEOUT:
		proc_result_string = "TIMEOUT";
		break;
	case -PS3_RESP_ERR:
		proc_result_string = "RESPONSE ERROR";
		break;
	case -PS3_RESP_INT:
		proc_result_string = "RESPONSE INTERRUPTED";
		break;
	case -PS3_CMD_NO_RESP:
		proc_result_string = "PS3_CMD_NO_RESP";
		break;
	default:
		proc_result_string = "INVALID RESULT";
		break;
	}

	return proc_result_string;
}

static enum ps3_fault_action
ps3_err_fault_strategy_lookup(struct ps3_instance *instance,
			      unsigned int resp_status, unsigned int retry_cnt)
{
	unsigned int idx = 0;
	enum ps3_fault_action fault_action = PS3_FAULT_STRATEGY_RETRY;

	for (idx = 0; idx < ARRAY_SIZE(ps3_fault_strategy_desc); idx++) {
		if (ps3_fault_strategy_desc[idx].resp_status == resp_status) {
			fault_action =
				ps3_fault_strategy_desc[idx].fault_action;
			break;
		}
	}

	if ((fault_action == PS3_FAULT_STRATEGY_RETRY) && (retry_cnt > 0))
		fault_action = PS3_FAULT_STRATEGY_NONE;

	LOG_INFO("host_no:%u error_no:%d fault_action is %s!\n",
		 PS3_HOST(instance), resp_status,
		 ps3_err_action_print(fault_action));
	return fault_action;
}

static int ps3_err_fault_strategy_exec(struct ps3_instance *instance,
				       enum ps3_fault_action fault_action)
{
	int proc_result = -PS3_FAILED;

	switch (fault_action) {
	case PS3_FAULT_STRATEGY_RETRY:
		ps3_msleep(PS3_ERR_MGR_CMD_DELAY_TIME_BEFORE_RERTY);
		proc_result = -PS3_RETRY;
		break;
	default:
		break;
	}

	LOG_INFO("host_no:%u proc result for mgr cmd fault is %s!\n",
		 PS3_HOST(instance),
		 ps3_err_mgr_fault_proc_result_print(proc_result));
	return proc_result;
}

int ps3_err_mgr_cmd_failed_check(struct ps3_instance *instance,
				 struct ps3_cmd *cmd)
{
	int ret = PS3_SUCCESS;
	enum ps3_fault_action fault_action = PS3_FAULT_STRATEGY_NONE;
	unsigned int resp_status = cmd->resp_frame->normalRespFrame.respStatus;

	if (!ps3_is_instance_state_allow_cmd_execute(instance)) {
		ret = -PS3_FAILED;
		goto l_out;
	}

	fault_action = ps3_err_fault_strategy_lookup(instance, resp_status,
						     cmd->retry_cnt);
	ret = ps3_err_fault_strategy_exec(instance, fault_action);
l_out:
	return ret;
}

void ps3_err_fault_context_init(struct ps3_instance *instance)
{
	struct ps3_fault_context *fault_context = &instance->fault_context;

	memset(fault_context, 0, sizeof(*fault_context));
	ps3_atomic_set(&instance->is_err_scsi_processing, 0);
}

void ps3_err_fault_context_exit(struct ps3_instance *instance)
{
	struct ps3_fault_context *fault_context = &instance->fault_context;

	memset(fault_context, 0, sizeof(*fault_context));
}

int ps3_err_mgr_cmd_proc(struct ps3_instance *instance, int fault_type,
			 struct ps3_cmd *cmd)
{
	int proc_result = PS3_SUCCESS;
	int cur_state = PS3_INSTANCE_STATE_INIT;

	LOG_INFO("host_no:%u fault_type: %s, retries:%d!\n", PS3_HOST(instance),
		 ps3_err_mgr_fault_proc_result_print(fault_type),
		 cmd->retry_cnt);

	switch (fault_type) {
	case PS3_SUCCESS:
		break;
	case -PS3_RESP_ERR:
		if (PS3_MGR_CMD_TYPE(cmd) == PS3_CMD_IOCTL) {
			LOG_INFO(
				"host_no:%u CFID:%d trace_id:0x%llx cli return err:%u\n",
				PS3_HOST(instance), cmd->cmd_word.cmdFrameID,
				cmd->trace_id, ps3_cmd_resp_status(cmd));
			if (instance->is_probe_finish) {
				if (ps3_cmd_resp_status(cmd) ==
				    PS3_DRV_MGR_BUSY) {
					LOG_INFO(
						"host_no:%u CFID:%d trace_id:0x%llx cli return busy\n",
						PS3_HOST(instance),
						cmd->cmd_word.cmdFrameID,
						cmd->trace_id);
					if (ps3_instance_wait_for_normal(
						    instance) == PS3_SUCCESS) {
						cmd->req_frame->mgrReq.value
							.isRetry = PS3_TRUE;
						proc_result = -PS3_RETRY;
					} else {
						proc_result = -PS3_FAILED;
					}
				} else if (ps3_cmd_resp_status(cmd) ==
					   PS3_DRV_MGR_RESTART_COMMAND_RSP) {
					LOG_WARN(
						"host_no:%u CFID:%d trace_id:0x%llx cli cx restart return success\n",
						PS3_HOST(instance),
						cmd->cmd_word.cmdFrameID,
						cmd->trace_id);
					ps3_need_wait_hard_reset_request(
						instance);
					if (ps3_recovery_state_wait_for_normal(
						    instance) != PS3_SUCCESS) {
						proc_result = -PS3_FAILED;
					} else if (ps3_hard_recovery_request_with_retry(
						instance, PS3_FALSE) != PS3_SUCCESS) {
						LOG_WARN(
						"host_no:%u hard recovery request NOK\n",
						PS3_HOST(instance));
					}
				} else {
					cur_state = ps3_atomic_read(
						&instance->state_machine.state);
					if (cur_state ==
						    PS3_INSTANCE_STATE_QUIT ||
					    cur_state ==
						    PS3_INSTANCE_STATE_DEAD) {
						proc_result = -PS3_ENODEV;
					} else {
						proc_result = -PS3_FAILED;
					}
				}
			} else {
				proc_result = -PS3_FAILED;
			}
		} else {
			proc_result =
				ps3_err_mgr_cmd_failed_check(instance, cmd);
		}
		break;
	case -PS3_TIMEOUT:
		proc_result = -PS3_CMD_NO_RESP;
		break;
	default:
		proc_result = -PS3_FAILED;
		break;
	}

	if (proc_result != -PS3_CMD_NO_RESP)
		cmd->cmd_state.reset_flag = 0;

	LOG_INFO("host_no:%u proc_result: %s, retries:%d!\n",
		 PS3_HOST(instance),
		 ps3_err_mgr_fault_proc_result_print(proc_result),
		 cmd->retry_cnt);

	return proc_result;
}
