
#ifdef _WINDOWS

#else
#include <linux/irqreturn.h>
#include <linux/types.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/limits.h>
#include <linux/delay.h>
#include <linux/compiler.h>

#endif
#include "ps3_cmd_complete.h"
#include "ps3_cmd_channel.h"
#include "ps3_driver_log.h"
#include "ps3_err_def.h"
#include "ps3_ioc_state.h"
#include "ps3_ioc_manager.h"
#include "ps3_scsi_cmd_err.h"

static inline Bool ps3_reply_word_is_valid(U64 val);
static inline void ps3_reply_word_next(struct ps3_irq *irq,
	struct PS3ReplyWord **reply_word);
static S32 ps3_reply_fifo_traverse(struct ps3_irq *irq,
	struct ps3_instance *instance, struct PS3ReplyWord* reply_word,
	S32 *completed_count);
static inline struct PS3ReplyWord* ps3_reply_word_query(
	struct ps3_irq *irq, U16 idx);

S32 ps3_cmd_complete(struct ps3_irq *irq)
{
	S32 ret = PS3_SUCCESS;
	S32 completed_count = 0;
	struct PS3ReplyWord *reply_word = NULL;
	struct ps3_instance *instance = irq->instance;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;
#ifndef _WINDOWS
	LOG_DEBUG("hno:%u start isr_os:%d, name:%s isr_sn:%d, "
		"irq_enable:%d, irq_poll:%d, last_reply_idx:%d, is_busy:%d\n",
		PS3_HOST(irq->instance), irq->irqNo, irq->name,
		irq->isrSN, irq->is_enable_irq, irq->is_sched_irq_poll,
		irq->last_reply_idx, atomic_read(&irq->is_busy));
#else
	LOG_DEBUG("hno:%u start isr_os:%d, name:%s isr_sn:%d, "
		"last_reply_idx:%d, is_busy:%d\n",
		PS3_HOST(irq->instance), irq->irqNo, irq->name,
		irq->isrSN, irq->last_reply_idx, ps3_atomic_read(&irq->is_busy));
#endif
	cur_state = ps3_atomic_read(&instance->state_machine.state);
	if (unlikely (cur_state == PS3_INSTANCE_STATE_QUIT) ) {
		LOG_WARN_IN_IRQ(instance, "hno:%u instance state is quit\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	if (!ps3_irq_busy_add(irq)) {
		LOG_INFO_IN_IRQ(instance, "hno:%u irq is_busy:%d\n", PS3_HOST(instance),
			ps3_atomic_read(&irq->is_busy));
		goto l_out;
	}

	reply_word = ps3_reply_word_query(irq, irq->last_reply_idx);
	if (!ps3_reply_word_is_valid(*(U64*)reply_word)) {
		ps3_irq_busy_dec(irq);
		LOG_DEBUG("hno:%u reply_w:0x%llx last_reply_idx:%d\n",
			PS3_HOST(instance), *(U64*)reply_word,
			irq->last_reply_idx);
		goto l_out;
	}

	ret = ps3_reply_fifo_traverse(irq, instance,
		reply_word, &completed_count);
	if (ret == -PS3_IN_IRQ_POLLING) {
		ps3_irq_busy_dec(irq);
		LOG_DEBUG("hno:%u reply in irq polling:%d\n",
			PS3_HOST(instance), ret);
		goto l_out;
	}

	if (completed_count != 0) {
		wmb();
#ifndef _WINDOWS
		ps3_can_queue_depth_update(instance);
#endif
	}

	ps3_irq_busy_dec(irq);
#ifndef _WINDOWS
	LOG_DEBUG("hno:%u end isr_os:%d, name:%s isr_sn:%d, "
		"irq_enable:%d, irq_poll:%d, last_reply_idx:%d, complete_count:%d, is_busy:%d\n",
		PS3_HOST(instance), irq->irqNo, irq->name, irq->isrSN,
		irq->is_enable_irq, irq->is_sched_irq_poll, irq->last_reply_idx,
		completed_count, atomic_read(&irq->is_busy));
#else
	LOG_DEBUG("hno:%u end isr_os:%d, name:%s isr_sn:%d, "
		"irq_enable:%d, complete_count:%d, is_busy:%d\n",
		PS3_HOST(instance), irq->irqNo, irq->name, irq->isrSN,
		irq->last_reply_idx, completed_count, ps3_atomic_read(&irq->is_busy));
#endif

l_out:
	return completed_count;
}

static inline struct PS3ReplyWord *ps3_reply_word_query(
	struct ps3_irq *irq, U16 idx)
{
	return irq->reply_fifo_virt_base_addr + idx;
}

static inline Bool ps3_reply_word_is_valid(U64 val)
{
	U64 type = val & U64_MAX;
	return (type != U64_MAX);
}

static inline void ps3_reply_word_next(struct ps3_irq *irq,
	struct PS3ReplyWord **reply_word)
{
	if (!irq->last_reply_idx) {
		*reply_word = ps3_reply_word_query(irq, irq->last_reply_idx);
	} else {
		++(*reply_word);
	}
	return;
}
#ifndef _WINDOWS
void ps3_trigger_irq_poll(struct ps3_irq *irq)
{
	LOG_DEBUG("host_no:%u trigger irq_poll isrSN:%d\n",
		PS3_HOST(irq->instance), irq->isrSN);

	if (irq->is_sched_irq_poll) {
		LOG_DEBUG("host_no:%u irq_poll_is_processing is PS3_TRUE\n",
			PS3_HOST(irq->instance));
		goto l_out;
	}

	irq->is_sched_irq_poll = PS3_TRUE;
	irq->is_enable_irq = PS3_TRUE;
	ps3_irq_poll_sched(&irq->irqpoll);

l_out:
	return;
}

void ps3_can_queue_depth_update(struct ps3_instance *instance)
{
	ULong flag = 0;
	ULong time_threahold = 5;
	S32 old_host_can_que = 0;

	if (instance->fault_context.ioc_busy &&
		time_after(jiffies, instance->fault_context.last_time +
		time_threahold * HZ) &&
		(atomic_read(&instance->cmd_statistics.io_outstanding) <
		(instance->cmd_attr.throttle_que_depth + 1))) {
		instance->fault_context.ioc_busy = PS3_FALSE;

		spin_lock_irqsave(instance->host->host_lock, flag);
		old_host_can_que = instance->host->can_queue;
		instance->host->can_queue = instance->cmd_attr.cur_can_que;
		spin_unlock_irqrestore(instance->host->host_lock, flag);
		LOG_INFO("hno:%u old_can_queue:%d, cur_can_que:%d, ioc_busy:%d.\n",
			PS3_HOST(instance), old_host_can_que,
			instance->cmd_attr.cur_can_que,
			instance->fault_context.ioc_busy);
	}
	return ;
}
#endif
static S32 ps3_reply_fifo_traverse(struct ps3_irq *irq,
	struct ps3_instance *instance, struct PS3ReplyWord *reply_word,
	S32 *completed_count)
{
	S32 ret = PS3_SUCCESS;
	U32 reply_threshold_count = 0;
	U64 *r_word = NULL;
	static U16 reply_word_size = sizeof(struct PS3ReplyWord);

	while (ps3_reply_word_is_valid(*(U64*)reply_word)) {
		r_word = (U64 *)reply_word;
		LOG_DEBUG("hno:%u CFID:%d reply_word:0x%llx reply_f:%d reply_mode:%d rettype:%d\n",
			PS3_HOST(instance), le16_to_cpu(reply_word->cmdFrameID), *r_word,
			reply_word->retStatus, reply_word->mode, reply_word->retType);
		ret = ps3_cmd_dispatch(instance,
			le16_to_cpu(reply_word->cmdFrameID),
			reply_word);
		if (unlikely(ret != PS3_SUCCESS)) {
			LOG_ERROR_IN_IRQ(instance, "host_no:%u CFID:%d dispatch cmd fail\n",
				PS3_HOST(instance),
				le16_to_cpu(reply_word->cmdFrameID));
		}

		memset(reply_word, 0xff, reply_word_size);

		++(irq->last_reply_idx);
		if (irq->last_reply_idx >=
			instance->irq_context.reply_fifo_depth) {
			LOG_DEBUG("last_reply_idx = %d, depth=%d\n", irq->last_reply_idx,
				instance->irq_context.reply_fifo_depth);
			irq->last_reply_idx = 0;
		}

		if (((++(*completed_count)) & (PS3_QOS_NOTIFY_CMD_COUNT - 1)) == 0) {
            ps3_qos_waitq_notify(instance);
        }

		++reply_threshold_count;

		ps3_reply_word_next(irq, &reply_word);
		if (!ps3_reply_word_is_valid(*(U64*)reply_word)) {
			break;
		}
#ifndef _WINDOWS
#ifdef CONFIG_IRQ_POLL
		if (reply_threshold_count >= irq->irq_poll_sched_threshold) {
			reply_threshold_count = 0;
			wmb();
			ps3_trigger_irq_poll(irq);
			ret = -PS3_IN_IRQ_POLLING;
			break;
		}
#endif
#endif
	}

	if (((*completed_count) & (PS3_QOS_NOTIFY_CMD_COUNT - 1)) > 0) {
		ps3_qos_waitq_notify(instance);
	}

	return ret;
}

S32 ps3_resp_status_convert(U32 resp_status)
{
	S32 ret = PS3_SUCCESS;

	switch (resp_status) {
	case U8_MAX:
		ret = -PS3_TIMEOUT;
		break;
	case SCSI_STATUS_GOOD:
		ret = PS3_SUCCESS;
		break;
	default:
		ret = -PS3_RESP_ERR;
		break;
	}

	return ret;
}

S32 ps3_cmd_reply_polling(struct ps3_instance *instance,
	struct ps3_cmd *cmd, ULong timeout, Bool ignore)
{
	const U32 seconds_to_msecs_unit = 1000;
	const U32 step_size             = 20; 
	const U32 read_fw_satus_period  = 5000;
	U32 local_resp_status           = U8_MAX;
	U32 msecs = U32_MAX;
	U32 i = 0;
	ULong flags = 0;
	U32 time_out;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;
	
	time_out = max((ULong)cmd->time_out, timeout);
	if (time_out != 0) {
		msecs = time_out * seconds_to_msecs_unit;
	}

	for (i = 0; (i < msecs) &&
		(cmd->resp_frame->normalRespFrame.respStatus == PS3_SCSI_STATUS_MASK);
		i += step_size) {
		rmb();
		ps3_msleep(step_size);
		if (i % read_fw_satus_period) {
			continue;
		}

		if (ignore) {
			cur_state = ps3_atomic_read(&instance->state_machine.state);
			if (cur_state == PS3_INSTANCE_STATE_DEAD) {
				break;
			}
		} else {
			if (!ps3_is_instance_state_allow_cmd_execute(instance)) {
				break;
			}
		}
	}

	local_resp_status = le32_to_cpu(cmd->resp_frame->normalRespFrame.respStatus);
	LOG_DEBUG("host_no:%u CFID:%d, respStatus:0x%x\n",
		PS3_HOST(instance), cmd->index, local_resp_status);

	if (local_resp_status != U8_MAX) {
		ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
		cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	}

	return ps3_resp_status_convert(local_resp_status);
}
S32 ps3_cmd_reply_polling_when_recovery(struct ps3_instance *instance,
	struct ps3_cmd *cmd, ULong timeout)
{
	const U32 seconds_to_msecs_unit = 1000;
	const U32 step_size             = 20; 
	const U32 read_fw_satus_period  = 5000;
	U32 local_resp_status           = U8_MAX;
	U32 msecs = U32_MAX;
	U32 i = 0;
	ULong flags = 0;
	U32 time_out;

	time_out = max((ULong)cmd->time_out, timeout);
	if (time_out != 0) {
		msecs = time_out * seconds_to_msecs_unit;
	}

	for (i = 0; (i < msecs) &&
		(cmd->resp_frame->normalRespFrame.respStatus == PS3_SCSI_STATUS_MASK);
		i += step_size) {
		rmb();
		ps3_msleep(step_size);
		if (i % read_fw_satus_period) {
			continue;
		}
	}

	local_resp_status = le32_to_cpu(cmd->resp_frame->normalRespFrame.respStatus);
	LOG_DEBUG("host_no:%u CFID:%d, respStatus:0x%x\n",
		PS3_HOST(instance), cmd->index, local_resp_status);

	if (local_resp_status != U8_MAX) {
		ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
		cmd->cmd_state.state = PS3_CMD_STATE_COMPLETE;
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	}

	return ps3_resp_status_convert(local_resp_status);
}

void ps3_all_reply_fifo_complete(struct ps3_instance *instance)
{
	U32 i = 0;
	for (; i < instance->irq_context.valid_msix_vector_count; ++i) {
		(void)ps3_cmd_complete(instance->irq_context.irqs + i);
	}

	return;
}
