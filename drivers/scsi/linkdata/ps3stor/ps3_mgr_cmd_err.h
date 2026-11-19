/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_MGR_CMD_ERR_H_
#define _PS3_MGR_CMD_ERR_H_

#ifndef _WINDOWS
#include <linux/mutex.h>
#include <linux/atomic.h>
#include "linux/kernel.h"
#endif

#include "ps3_htp_def.h"
#include "ps3_instance_manager.h"

#define PS3_ERR_MGR_CMD_FAULT_RETRY_MAX (3)
#define PS3_ERR_MGR_CMD_DELAY_TIME_BEFORE_RERTY (500)

void ps3_err_fault_context_init(struct ps3_instance *instance);
void ps3_err_fault_context_exit(struct ps3_instance *instance);

int ps3_err_mgr_cmd_proc(struct ps3_instance *instance, int fault_type,
			 struct ps3_cmd *cmd);

int ps3_err_mgr_cmd_failed_check(struct ps3_instance *instance,
				 struct ps3_cmd *cmd);

const char *ps3_err_mgr_fault_proc_result_print(int result);

#endif
