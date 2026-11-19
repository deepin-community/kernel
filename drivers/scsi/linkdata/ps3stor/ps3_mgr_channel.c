// SPDX-License-Identifier: GPL-2.0
#ifdef _WINDOWS
#include "ps3_def.h"

#else

#define WINDOWS_DELAY

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/kernel.h>

#include "ps3_scsih.h"
#include "ps3_scsih_cmd_parse.h"
#endif

#include "ps3_htp.h"
#include "ps3_mgr_channel.h"
#include "ps3_cmd_complete.h"
#include "ps3_platform_utils.h"
#include "ps3_util.h"
#include "ps3_mgr_cmd_err.h"
#include "ps3_mgr_cmd.h"
#include "ps3_cmd_complete.h"
#include "ps3_ioc_manager.h"
#include "ps3_driver_log.h"
#include "ps3_instance_manager.h"
#include "ps3_cmd_statistics.h"
#include "ps3_kernel_version.h"

#ifndef _WINDOWS
#if defined(PS3_LINUX_SIGNAL)
#include <linux/sched/signal.h>
#else
#include <linux/signal.h>
#endif
#endif

#define CMD_MAX_RETRY_COUNT (10)

#define SEND_IOCTL_CMD_CHECK(instance, cmd)                                    \
	((PS3_MGR_CMD_TYPE(cmd) == PS3_CMD_IOCTL) && (instance)->is_support_irq)

static inline unsigned char is_interrupt_signal(struct task_struct *p)
{
	if (sigismember(&p->pending.signal, SIGINT) ||
	    sigismember(&p->pending.signal, SIGKILL) ||
	    sigismember(&p->pending.signal, SIGQUIT)) {
		return PS3_TRUE;
	}
	return PS3_FALSE;
}

void ps3_wait_cmd_for_completion_interrupt(struct ps3_instance *instance,
					   struct ps3_cmd *cmd)
{
	int ret = PS3_SUCCESS;
	unsigned long flags = 0;
	unsigned char is_send_cancel = PS3_FALSE;
	int cur_state = PS3_INSTANCE_STATE_INIT;
	int retry_count = 0;

	while (1) {
		ret = wait_for_completion_interruptible(&cmd->sync_done);
		if (ret == PS3_SUCCESS) {
			LOG_DEBUG(
				"host_no:%u wait_for_completion_interrupted success\n",
				PS3_HOST(instance));
			if ((cmd->resp_frame->normalRespFrame.respStatus ==
			     PS3_DRV_MGR_BUSY) &&
			    is_send_cancel) {
				cmd->resp_frame->normalRespFrame.respStatus =
					PS3_MGR_REC_FORCE;
			}
			break;
		}

		ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);

		ret = signal_pending(current);
		if (!ret) {
			ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
			ps3_msleep(100);
			continue;
		}
		ret = is_interrupt_signal(current);
		if (ret) {
			cur_state =
				ps3_atomic_read(&instance->state_machine.state);
			if (cur_state == PS3_INSTANCE_STATE_QUIT ||
			    cur_state == PS3_INSTANCE_STATE_DEAD) {
				LOG_INFO_IN_IRQ(
					instance, "host_no:%u cur_state:%s\n",
					PS3_HOST(instance),
					namePS3InstanceState(cur_state));
				ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock,
							   flags);
				break;
			}

			ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);

			LOG_INFO_LIM(
				"host_no:%u wait_for_completion_interrupted by SIG INT OR KILL, QUIT\n",
				PS3_HOST(instance));
			if (is_send_cancel) {
				ps3_msleep(100);
				continue;
			}
			is_send_cancel = PS3_TRUE;
cancel:
			ret = ps3_mgr_cmd_cancel(instance, cmd->index);
			if (ret != PS3_SUCCESS) {
				if (ret == -PS3_RECOVERED) {
					LOG_INFO_LIM(
						"host_no:%u cancel cmd %u send failed, wait\n",
						PS3_HOST(instance), cmd->index);
					ps3_msleep(100);
					continue;
				} else if (ret == -PS3_EBUSY) {
					ps3_msleep(100);
					if (retry_count++ <
					    CMD_MAX_RETRY_COUNT) {
						goto cancel;
					}
					continue;
				}
				LOG_INFO(
					"host_no:%u cancel cmd %u failed, QUIT, ret:%d\n",
					PS3_HOST(instance), cmd->index, ret);
			} else {
				ps3_spin_lock_irqsave(&cmd->cmd_state.lock,
						      &flags);
				cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
				ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock,
							   flags);
			}
			break;
		}
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
		ps3_msleep(100);
	}
}

static int ps3_mgr_cmd_send(struct ps3_instance *instance, struct ps3_cmd *cmd)
{
	int ret = PS3_SUCCESS;
	int cur_state = PS3_INSTANCE_STATE_INIT;

	if (!instance->state_machine.is_load ||
		instance->is_suspend) {
		if (PS3_MGR_CMD_TYPE(cmd) != PS3_CMD_MANAGEMENT) {
			LOG_WARN(
				"host_no:%u instance state is unloading or suspend\n",
				PS3_HOST(instance));
			ret = -PS3_FAILED;
			goto l_out;
		}
	}
	if (!ps3_is_instance_state_allow_cmd_execute(instance)) {
		LOG_WARN("host_no:%u cannot send block cmd\n",
			 PS3_HOST(instance));
		ret = -PS3_RECOVERED;
		if (PS3_MGR_CMD_TYPE(cmd) == PS3_CMD_IOCTL) {
			cur_state =
				ps3_atomic_read(&instance->state_machine.state);
			if (cur_state == PS3_INSTANCE_STATE_QUIT ||
			    cur_state == PS3_INSTANCE_STATE_DEAD) {
				LOG_WARN("host_no:%u cur_state:%s\n",
					 PS3_HOST(instance),
					 namePS3InstanceState(cur_state));
				goto l_out;
			}
			cmd->resp_frame->normalRespFrame.respStatus =
				PS3_DRV_MGR_BUSY;
			LOG_WARN(
				"host_no:%u ioctl cannot send block cmd:%u,resp:%d, will retry\n",
				PS3_HOST(instance), cmd->index,
				cmd->resp_frame->normalRespFrame.respStatus);
			ret = -PS3_RESP_ERR;
		}
		goto l_out;
	}

	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN(
			"host_no:%u cannot send block cmd due to pci recovery\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	PS3_MGR_CMD_STAT_INC(instance, cmd);

	LOG_FILE_INFO("host_no:%u CFID:%d trace_id:0x%llx ready send\n",
		      PS3_HOST(instance), cmd->cmd_word.cmdFrameID,
		      cmd->trace_id);
	LOG_DEBUG("host_no:%u req cmd info:\n"
		  "\t reqFrameBufBase = 0x%llx,  function = %d\n",
		  PS3_HOST(instance),
		  cpu_to_le64(instance->cmd_context.req_frame_buf_phys),
		  ps3_get_pci_function(instance->pdev));

	instance->ioc_adpter->cmd_send(instance, &cmd->cmd_word);
l_out:
	return ret;
}
static int ps3_blocked_unload_cmd_send(struct ps3_instance *instance,
				       struct ps3_cmd *cmd)
{
	int ret = PS3_SUCCESS;

	PS3_MGR_CMD_STAT_INC(instance, cmd);

	instance->ioc_adpter->cmd_send(instance, &cmd->cmd_word);
	ret = ps3_block_cmd_wait(instance, cmd, 0);
	LOG_INFO("host_no:%u CFID:%d trace_id:0x%llx ret:%d\n",
		 PS3_HOST(instance), cmd->cmd_word.cmdFrameID, cmd->trace_id,
		 ret);
	return ret;
}

static int ps3_blocked_cmd_wake(struct ps3_cmd *cmd)
{
	unsigned long flags = 0;

	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	if (cmd->cmd_state.state == PS3_CMD_STATE_DEAD) {
		LOG_WARN_IN_IRQ(
			cmd->instance,
			"host_no:%u CFID:%d trace_id:0x%llx dead free\n",
			PS3_HOST(cmd->instance), cmd->index, cmd->trace_id);

		PS3_MGR_CMD_BACK_INC(cmd->instance, cmd, 0);
		ps3_mgr_cmd_free_nolock(cmd->instance, cmd);
		goto l_out;
	}

	if (cmd->cmd_state.state == PS3_CMD_STATE_PROCESS) {
		cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;

		complete(&cmd->sync_done);

		PS3_MGR_CMD_BACK_INC(cmd->instance, cmd, 0);
	} else {
		LOG_ERROR_IN_IRQ(
			cmd->instance,
			"host_no:%u CFID:%d trace_id:0x%llx repeat reply\n",
			PS3_HOST(cmd->instance), cmd->index, cmd->trace_id);
	}

l_out:
	ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	return PS3_SUCCESS;
}

static int ps3_polled_cmd_send(struct ps3_instance *instance,
			       struct ps3_cmd *cmd)
{
	int ret = PS3_SUCCESS;

	if (!instance->state_machine.is_load ||
		instance->is_suspend) {
		if (PS3_MGR_CMD_TYPE(cmd) != PS3_CMD_MANAGEMENT) {
			LOG_WARN(
				"host_no:%u instance state is unloading or suspend\n",
				PS3_HOST(instance));
			ret = -PS3_FAILED;
			goto l_out;
		}
	}
	if (!ps3_is_instance_state_allow_cmd_execute(instance)) {
		ret = -PS3_RECOVERED;
		goto l_out;
	}

	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN(
			"host_no:%u cannot send block cmd due to pci recovery\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	PS3_MGR_CMD_STAT_INC(instance, cmd);
	instance->ioc_adpter->cmd_send(instance, &cmd->cmd_word);
l_out:
	return ret;
}
static int ps3_polled_unload_cmd_send(struct ps3_instance *instance,
				      struct ps3_cmd *cmd)
{
	int ret = -PS3_FAILED;

	PS3_MGR_CMD_STAT_INC(instance, cmd);
	instance->ioc_adpter->cmd_send(instance, &cmd->cmd_word);

	ret = ps3_cmd_reply_polling(instance, cmd, 0, PS3_TRUE);
	if (ret != -PS3_TIMEOUT) {
		PS3_MGR_CMD_BACK_INC(instance, cmd,
				     (ret == PS3_SUCCESS) ?
					     PS3_REPLY_WORD_FLAG_SUCCESS :
					     PS3_REPLY_WORD_FLAG_FAIL);
	}
	return ret;
}

static int ps3_blocked_cmd_cb(struct ps3_cmd *cmd, unsigned short reply_flags)
{
	if ((cmd->req_frame->mgrReq.reqHead.noReplyWord ==
	     PS3_CMD_WORD_NEED_REPLY_WORD) &&
	    (reply_flags == PS3_REPLY_WORD_FLAG_SUCCESS)) {
		cmd->resp_frame->normalRespFrame.respStatus = SCSI_STATUS_GOOD;
	}

	return ps3_blocked_cmd_wake(cmd);
}

int ps3_cmd_send_sync(struct ps3_instance *instance, struct ps3_cmd *cmd)
{
	if ((cmd->is_force_polling == 0 &&
	     ps3_irq_is_enable(&cmd->instance->irq_context)) ||
	    SEND_IOCTL_CMD_CHECK(instance, cmd)) {
		cmd->cmd_receive_cb = ps3_blocked_cmd_cb;
		cmd->req_frame->mgrReq.reqHead.noReplyWord =
			PS3_CMD_WORD_NEED_REPLY_WORD;
		return ps3_mgr_cmd_send(instance, cmd);
	}
	cmd->cmd_receive_cb = NULL;
	cmd->req_frame->mgrReq.reqHead.noReplyWord =
		PS3_CMD_WORD_NO_REPLY_WORD;
	return ps3_polled_cmd_send(instance, cmd);
}

int ps3_cmd_wait_sync(struct ps3_instance *instance, struct ps3_cmd *cmd)
{
	int ret = PS3_SUCCESS;

	if (cmd->req_frame->mgrReq.reqHead.noReplyWord ==
	    PS3_CMD_WORD_NEED_REPLY_WORD) {
		ret = ps3_block_cmd_wait(instance, cmd, 0);
	} else {
		ret = ps3_cmd_reply_polling(instance, cmd, 0, PS3_FALSE);
		if (ret != -PS3_TIMEOUT) {
			PS3_MGR_CMD_BACK_INC(
				instance, cmd,
				(ret == PS3_SUCCESS) ?
					PS3_REPLY_WORD_FLAG_SUCCESS :
					PS3_REPLY_WORD_FLAG_FAIL);
		}
	}
	LOG_FILE_INFO("host_no:%u CFID:%d trace_id:0x%llx recv ret:%d\n",
		      PS3_HOST(instance), cmd->cmd_word.cmdFrameID,
		      cmd->trace_id, ret);
	return ret;
}

int ps3_unload_cmd_send_sync(struct ps3_instance *instance, struct ps3_cmd *cmd)
{
	if (cmd->is_force_polling == 0 &&
	    ps3_irq_is_enable(&cmd->instance->irq_context)) {
		cmd->cmd_receive_cb = ps3_blocked_cmd_cb;
		cmd->req_frame->mgrReq.reqHead.noReplyWord =
			PS3_CMD_WORD_NEED_REPLY_WORD;
		return ps3_blocked_unload_cmd_send(instance, cmd);
	}
	cmd->cmd_receive_cb = NULL;
	cmd->req_frame->mgrReq.reqHead.noReplyWord = PS3_CMD_WORD_NO_REPLY_WORD;
	return ps3_polled_unload_cmd_send(instance, cmd);
}

int ps3_cmd_no_block_send(struct ps3_instance *instance, struct ps3_cmd *cmd)
{
	int ret = PS3_SUCCESS;

	cmd->cmd_receive_cb = ps3_blocked_cmd_cb;
	cmd->req_frame->mgrReq.reqHead.noReplyWord =
		PS3_CMD_WORD_NEED_REPLY_WORD;

	if (!ps3_is_instance_state_allow_cmd_execute(instance)) {
		ret = -PS3_FAILED;
		goto l_out;
	}

	PS3_MGR_CMD_STAT_INC(instance, cmd);

	instance->ioc_adpter->cmd_send(instance, &cmd->cmd_word);
l_out:
	return ret;
}

int ps3_block_cmd_wait(struct ps3_instance *instance, struct ps3_cmd *cmd,
		       unsigned long timeout)
{
	int ret = PS3_SUCCESS;

	if (cmd->is_interrupt) {
#ifdef WINDOWS_DELAY
		ps3_wait_cmd_for_completion_interrupt(instance, cmd);
#endif
	} else {
		ret = ps3_wait_cmd_for_completion_timeout(instance, cmd,
							  timeout);
		if (ret == -PS3_TIMEOUT) {
			LOG_ERROR(
				"host_no:%u CFID:%d trace_id:0x%llx time out\n",
				PS3_HOST(instance), cmd->cmd_word.cmdFrameID,
				cmd->trace_id);
			goto l_out;
		}
	}

	if (cmd->cmd_state.state != PS3_CMD_STATE_COMPLETE) {
		LOG_INFO(
			"host_no:%u CFID:%d trace_id:0x%llx not complete,state:%u\n",
			PS3_HOST(instance), cmd->cmd_word.cmdFrameID,
			cmd->trace_id, cmd->cmd_state.state);
		ret = -PS3_TIMEOUT;
		goto l_out;
	}

	if (ps3_cmd_resp_status(cmd) != SCSI_STATUS_GOOD) {
		ret = -PS3_RESP_ERR;
		LOG_INFO("host_no:%u CFID:%d trace_id:0x%llx resp err:0x%x\n",
			 PS3_HOST(instance), cmd->cmd_word.cmdFrameID,
			 cmd->trace_id, ps3_cmd_resp_status(cmd));
	}
l_out:
	return ret;
}

int ps3_cmd_send_async(struct ps3_instance *instance, struct ps3_cmd *cmd,
		       int (*cmd_receive_cb)(struct ps3_cmd *, unsigned short))
{
	cmd->cmd_receive_cb = cmd_receive_cb;
	PS3_MGR_CMD_STAT_INC(instance, cmd);
	return ps3_async_cmd_send(instance, cmd);
}
