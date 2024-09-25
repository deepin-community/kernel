
#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/delay.h>
#endif

#include "ps3_ioc_state.h"
#include "ps3_driver_log.h"
#include "ps3_ioc_manager.h"
#include "ps3_module_para.h"

#define PS3_SOFTRESET_MASK (0xFF)	
#define PS3_HARDRESET_MASK (0xFF)	

enum ps3_reset_type {
	PS3_FW_HARD_RESET = 0,
	PS3_FW_SHALLOW_SOFT_RESET = 1,
	PS3_FW_DEEP_SOFT_RESET = 2,
};

static inline const S8 *namePS3ResetType(S32 s)
{
	static const S8 *myNames[] = {
		[PS3_FW_HARD_RESET]		= "PS3_FW_HARD_RESET",
		[PS3_FW_SHALLOW_SOFT_RESET]	= "PS3_FW_SHALLOW_SOFT_RESET",
		[PS3_FW_DEEP_SOFT_RESET]	= "PS3_FW_DEEP_SOFT_RESET"
	};

	if (s > PS3_FW_DEEP_SOFT_RESET) {
		return "PS3_RESET_TYPE_INVALID";
	}

	return myNames[s];
}

struct ps3_reset_key_map {
	U32 reset_key_offset;		
	U32 reset_state_offset;		
	U32 reset_offset;		
	U32 reset_type;			
	U32 reset_status_offset;	
	U32 reset_status_mask;		
};

struct ps3_state_desc_map {
	U32 state;			
	const char *state_desc;		
};

static struct ps3_reset_key_map g_reset_key_table[] = {
	[PS3_FW_HARD_RESET] = {
			offsetof(HilReg0Ps3RegisterF_s, ps3HardresetKey),
			offsetof(HilReg0Ps3RegisterF_s, ps3HardresetState),
			offsetof(HilReg0Ps3RegisterF_s, ps3Hardreset),
		PS3_FW_HARD_RESET_ACT,
			offsetof(HilReg0Ps3RegisterF_s, ps3Hardreset),
		PS3_HARDRESET_MASK
	},
	[PS3_FW_SHALLOW_SOFT_RESET] = {
			offsetof(HilReg0Ps3RegisterF_s, ps3SoftresetKey),
			offsetof(HilReg0Ps3RegisterF_s, ps3SoftresetState),
			offsetof(HilReg0Ps3RegisterF_s, ps3Softreset),
		PS3_FW_STATE_ACT_SHALLOW_SOFT_RESET,
			offsetof(HilReg0Ps3RegisterF_s, ps3Softreset),
		PS3_SOFTRESET_MASK
	},
	[PS3_FW_DEEP_SOFT_RESET] = {
			offsetof(HilReg0Ps3RegisterF_s, ps3SoftresetKey),
			offsetof(HilReg0Ps3RegisterF_s, ps3SoftresetState),
			offsetof(HilReg0Ps3RegisterF_s, ps3Softreset),
		PS3_FW_STATE_ACT_DEEP_SOFT_RESET,
			offsetof(HilReg0Ps3RegisterF_s, ps3Softreset),
		PS3_SOFTRESET_MASK
	},
};

static struct ps3_state_desc_map g_state_desc[] = {
	{PS3_FW_STATE_UNDEFINED, "PS3_FW_STATE_UNDEFINED"},
	{PS3_FW_STATE_START, "PS3_FW_STATE_START"},
	{PS3_FW_STATE_READY, "PS3_FW_STATE_READY"},
	{PS3_FW_STATE_WAIT, "PS3_FW_STATE_WAIT"},
	{PS3_FW_STATE_RUNNING, "PS3_FW_STATE_RUNNING"},
	{PS3_FW_STATE_FLUSHING, "PS3_FW_STATE_FLUSHING"},
	{PS3_FW_STATE_RESET, "PS3_FW_STATE_RESET"},
	{PS3_FW_STATE_FAULT, "PS3_FW_STATE_FAULT"},
	{PS3_FW_STATE_CRITICAL, "PS3_FW_STATE_CRITICAL"},
	{PS3_FW_STATE_HALT, "PS3_FW_STATE_HALT"}
};

static void ps3_reset_key_write(struct ps3_instance *instance,
	U32 offset)
{
	static U32 reset_key_array[] = {
		PS3_FW_DIAG_1ST_KEY,
		PS3_FW_DIAG_2ND_KEY,
		PS3_FW_DIAG_3RD_KEY,
		PS3_FW_DIAG_4TH_KEY,
		PS3_FW_DIAG_5TH_KEY,
		PS3_FW_DIAG_6TH_KEY,
		PS3_FW_DIAG_7TH_KEY,
		PS3_FW_DIAG_8TH_KEY,
		PS3_FW_DIAG_9TH_KEY
	};
	U32 idx = 0;

	for(idx = 0;idx < ARRAY_SIZE(reset_key_array);idx++) {
		PS3_IOC_REG_WRITE_OFFSET(instance, offset, (U64)reset_key_array[idx]);
	}
}
static void ps3_hardreset_key_write(struct ps3_instance *instance,
	U8 *reset_key_vir_addr, U64 *timeval)
{
	static U32 reset_key_array[] = {
		PS3_FW_DIAG_1ST_KEY,
		PS3_FW_DIAG_2ND_KEY,
		PS3_FW_DIAG_3RD_KEY,
		PS3_FW_DIAG_4TH_KEY,
		PS3_FW_DIAG_5TH_KEY,
		PS3_FW_DIAG_6TH_KEY,
		PS3_FW_DIAG_7TH_KEY,
		PS3_FW_DIAG_8TH_KEY,
		PS3_FW_DIAG_9TH_KEY
	};
	U32 idx = 0;
	timeval[PS3_START_WRITE_KEY_REG] = ps3_1970_now_ms_get();
	for(idx = 0;idx < ARRAY_SIZE(reset_key_array);idx++) {
		ps3_ioc_hardreset_reg_write(instance,
			(U64)reset_key_array[idx], reset_key_vir_addr, 0);
	}
	timeval[PS3_END_WRITE_KEY_REG] = ps3_1970_now_ms_get();
}
static S32 ps3_reset_key_state_check(struct ps3_instance *instance,
	U32 offset)
{
	U32 reset_key_state = 0;
	U32 read_count = 0;
	const U32 retry_max  = 50;
	S32 ret = PS3_SUCCESS;
	U64 value = 0;

	do {
		if (read_count >= retry_max) {
			LOG_ERROR("hno:%u  PS3 reset key state is still "
				"disabled after 5 sec \n", PS3_HOST(instance));
			ret = -PS3_FAILED;
			break;
		}

		if (read_count != 0) {
			ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
		}

		PS3_IOC_REG_READ_OFFSET(instance, offset, value);
		reset_key_state = (U32)value;

		read_count++;
	} while (!(reset_key_state & PS3_FW_DIAG_ENABLE) ||
		(reset_key_state == U32_MAX));

	if (ret == PS3_SUCCESS) {
		LOG_INFO("hno:%u  PS3 reset key state is enabled after %d msecs\n",
			PS3_HOST(instance),
			(read_count - 1) * PS3_LOOP_TIME_INTERVAL_100MS);
	}

	return ret;
}
static S32 ps3_hardreset_key_state_check(struct ps3_instance *instance,
	U8 *reset_key_state_vir_addr, U64 *timeval)
{
	U32 reset_key_state = 0;
	U32 read_count = 0;
	const U32 retry_max  = 900;
	S32 ret = PS3_SUCCESS;

	timeval[PS3_START_WAIT_KEY_READY_REG] = ps3_1970_now_ms_get();
	do {
		if (read_count >= retry_max) {
			ret = -PS3_FAILED;
			break;
		}

		if (read_count != 0) {
			ps3_mdelay(PS3_LOOP_TIME_INTERVAL_1MS);
		}

		reset_key_state = (U32)ps3_ioc_hardreset_reg_read(instance,
			reset_key_state_vir_addr);

		read_count++;
	} while (!(reset_key_state & PS3_FW_DIAG_ENABLE) ||
		((U32_MAX & reset_key_state) == U32_MAX));
	timeval[PS3_END_WAIT_KEY_READY_REG] = ps3_1970_now_ms_get();

	return ret;
}

static S32 ps3_after_reset_request_check(
	struct ps3_instance *instance, enum ps3_reset_type reset_type)
{
	U32 fw_state  = instance->ioc_adpter->ioc_state_get(instance);
	U32 read_count = 0;
#ifdef PS3_HARDWARE_HAPS_V200
	const U32 retry_max  = 3600;
#else
	const U32 retry_max  = 1800;
#endif
	S32 ret = -PS3_FAILED;

	while(read_count < retry_max) {
		if ( (fw_state == PS3_FW_STATE_START) ||
			(fw_state == PS3_FW_STATE_READY)) {
			ret = PS3_SUCCESS;
			break;
		}
		if ((reset_type == PS3_FW_SHALLOW_SOFT_RESET) ||
			(reset_type == PS3_FW_DEEP_SOFT_RESET)) {
			if (fw_state == PS3_FW_STATE_RUNNING) {
				ret = PS3_SUCCESS;
				break;
			}
		}

		if (ps3_pci_err_recovery_get(instance)) {
			LOG_WARN("hno:%u  pci recovery resetting\n", PS3_HOST(instance));
			ret = -PS3_IN_PCIE_ERR;
			break;
		}
#ifdef PS3_HARDWARE_HAPS_V200
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_3000MS);
#else
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
#endif
		fw_state  = instance->ioc_adpter->ioc_state_get(instance);
		read_count++;
	}

	if ( ret != PS3_SUCCESS ) {
		LOG_ERROR("hno:%u  PS3 IOC state is not valid 180 secs after IOC reset, fw state is %s\n",
			PS3_HOST(instance),ps3_ioc_state_print(fw_state));
	}

	LOG_INFO("hno:%u  fw state is %s\n", PS3_HOST(instance),
		ps3_ioc_state_print(fw_state));

	return ret;

}

static S32 ps3_reset_request_completion_check(
	struct ps3_instance *instance, U32 offset, U32 completion_mask)
{
	U32 read_count = 0;
	const U32 retry_max  = 1000;
	S32 ret = PS3_SUCCESS;
	U64 value = 0;
	U32 completion = 0;

	PS3_IOC_REG_READ_OFFSET(instance, offset, value);
	completion = (U32)value;
	while (completion & completion_mask ||
		completion == U32_MAX) {
		if (read_count > retry_max) {
			LOG_ERROR("hno:%u  PS3 reset flag is not "
				"clear 100 secs after reset request \n",
				PS3_HOST(instance));
			ret = -PS3_FAILED;
			break;
		}

		if (ps3_pci_err_recovery_get(instance)) {
			LOG_WARN("hno:%u  pci recovery resetting\n", PS3_HOST(instance));
			ret = -PS3_FAILED;
			break;
		}

		ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
		PS3_IOC_REG_READ_OFFSET(instance, offset, value);
		completion = (U32)value;
		read_count++;
	}

	if (ret == PS3_SUCCESS) {
		LOG_INFO("hno:%u  PS3 reset complete %d msecs after reset complete \n",
			PS3_HOST(instance), read_count * PS3_LOOP_TIME_INTERVAL_100MS);
	}

	return ret;
}

const char *ps3_ioc_state_print(U32 state)
{
	U32 idx = 0;
	U32 fw_state = state & PS3_FW_STATE_MASK;
	const char *ps3_state_name = "invalid state";

	for (idx = 0;idx < ARRAY_SIZE(g_state_desc);idx++) {
		if (g_state_desc[idx].state == fw_state) {
			ps3_state_name = g_state_desc[idx].state_desc;
			break;
		}
	}

	return ps3_state_name;
}

static inline void ps3_ioc_state_trigger_transition(struct ps3_instance *instance, U32 action)
{
	PS3_IOC_REG_WRITE(instance, reg_f.Excl_reg, ps3Doorbell, (U64)action);
}

static inline void ps3_ioc_debug0_trigger(struct ps3_instance *instance, U32 mask_value)
{
	PS3_IOC_REG_WRITE(instance, reg_f.Excl_reg, ps3CmdTrigger, (U64)mask_value);
}

S32 ps3_ioc_state_fault_wait(struct ps3_instance *instance)
{
	U32 fw_cur_state = PS3_FW_STATE_UNDEFINED;
	S32 ret = PS3_SUCCESS;
	U32 count = 0;

	fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
	while (count < PS3_FW_STATE_TO_FAULT_TMO_LOOP_COUNT) {
		if (fw_cur_state == PS3_FW_STATE_FAULT ||
			fw_cur_state == PS3_FW_STATE_CRITICAL) {
			LOG_INFO("hno:%u  within 180s fw transfer to %s\n",
				PS3_HOST(instance),
				ps3_ioc_state_print(fw_cur_state));
			break;
		}

		ps3_msleep(PS3_LOOP_TIME_INTERVAL_20MS);
		fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
		count++;
	}

	if (fw_cur_state != PS3_FW_STATE_FAULT) {
		LOG_ERROR("hno:%u  fw state[%s] is not fault\n",
			PS3_HOST(instance), ps3_ioc_state_print(fw_cur_state));
		ret = -PS3_FAILED;
	} else {
		LOG_INFO("hno:%u  fw state transition to %s\n",
			PS3_HOST(instance),
			ps3_ioc_state_print(fw_cur_state));
	}

	return ret;
}

S32 ps3_ioc_state_ready_wait(struct ps3_instance *instance)
{
	U32 fw_cur_state = PS3_FW_STATE_UNDEFINED;
	S32 ret = PS3_SUCCESS;
	U32 count = 0;
	Bool is_unload_valid = PS3_FALSE;

	ps3_check_debug0_valid_with_check(instance, &is_unload_valid,
		PS3_CMD_TRIGGER_UNLOAD);
	fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
	while (count < instance->wait_ready_timeout) {
#if 0
		if ( (fw_cur_state == PS3_FW_STATE_READY)
			|| (fw_cur_state == PS3_FW_STATE_FAULT)
			|| (fw_cur_state == PS3_FW_STATE_HALT) ){
			break;
		}
#else
		if (ps3_pci_err_recovery_get(instance)) {
			LOG_WARN("hno:%u  pci recovery resetting\n", PS3_HOST(instance));
			ret = -PS3_IN_PCIE_ERR;
			goto l_out;
		}

		if (fw_cur_state == PS3_FW_STATE_READY && !is_unload_valid) {
			LOG_INFO("hno:%u  within 180s fw transfer to %s\n",
				PS3_HOST(instance),
				ps3_ioc_state_print(fw_cur_state));
			break;
		}
#endif
#ifdef PS3_HARDWARE_HAPS_V200
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_3000MS);
#else
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_20MS);
#endif
		ps3_check_debug0_valid_with_check(instance, &is_unload_valid,
			PS3_CMD_TRIGGER_UNLOAD);
		fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
		count++;
	}

	if (fw_cur_state != PS3_FW_STATE_READY || is_unload_valid) {
		LOG_ERROR("hno:%u  fw state[%s] is not ready, or unload[%d] not done\n",
			PS3_HOST(instance),
			ps3_ioc_state_print(fw_cur_state), is_unload_valid);
		ret = -PS3_FAILED;
	} else {
		LOG_INFO("hno:%u  fw state transition to %s\n",
			PS3_HOST(instance),
			ps3_ioc_state_print(fw_cur_state));
	}

l_out:
	return ret;
}

S32 ps3_ioc_state_transfer_to_ready(struct ps3_instance *instance)
{
	U32 fw_cur_state = PS3_FW_STATE_UNDEFINED;
	S32 ret = PS3_SUCCESS;

	if (!ps3_ioc_state_get_with_check(instance, &fw_cur_state)) {
		ret = -PS3_FAILED;
		goto l_out;
	}

	LOG_INFO("hno:%u  fw state is %s(0x%x)\n",
		PS3_HOST(instance), ps3_ioc_state_print(fw_cur_state), fw_cur_state);

	switch (fw_cur_state) {
	case PS3_FW_STATE_UNDEFINED:
	case PS3_FW_STATE_RESET:
	case PS3_FW_STATE_START:
	case PS3_FW_STATE_FLUSHING:
		ret = ps3_ioc_state_ready_wait(instance);
                if (ret != PS3_SUCCESS) {
					if (ret != -PS3_IN_PCIE_ERR) {
	                	ret = -PS3_NO_RECOVERED;
					}
	                LOG_ERROR("hno:%u  fw state to ready NOK \n",
			        PS3_HOST(instance));
                }
		break;
	case PS3_FW_STATE_READY:
		break;
	case PS3_FW_STATE_WAIT:
	case PS3_FW_STATE_RUNNING:
		LOG_ERROR("hno:%u  fw state is wait/running, to ready NOK \n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		break;
	case PS3_FW_STATE_FAULT:
	case PS3_FW_STATE_CRITICAL:
	case PS3_FW_STATE_HALT:
		LOG_ERROR("hno:%u  fw state is fault/halt, to ready NOK \n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		break;

	default:
		ret = ps3_ioc_state_ready_wait(instance);
		break;
	}
l_out:
	return ret;
}

S32 ps3_ioc_state_transfer_wait_to_running(struct ps3_instance *instance)
{
	U32 fw_cur_state = PS3_FW_STATE_UNDEFINED;
	S32 ret = PS3_SUCCESS;
	U32 count = 0;

	fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
	LOG_INFO("hno:%u  fw state is %s(0x%x)\n",
		PS3_HOST(instance),
		ps3_ioc_state_print(fw_cur_state), fw_cur_state);

	while (count < PS3_FW_STATE_TO_RUNNING_TMO_LOOP_COUNT) {
		if ( (fw_cur_state == PS3_FW_STATE_RUNNING)
			|| (fw_cur_state == PS3_FW_STATE_FAULT)
			|| (fw_cur_state == PS3_FW_STATE_HALT)
			|| (fw_cur_state == PS3_FW_STATE_CRITICAL)){
			break;
		}

		if (ps3_pci_err_recovery_get(instance)) {
			LOG_WARN("hno:%u  pci recovery resetting\n", PS3_HOST(instance));
			break;
		}

		ps3_mutex_lock(&instance->state_machine.lock);
		if(PS3_IS_INTERRUPT_SOFT_RECOVERY(instance)){
			LOG_WARN("hno:%u  soft reset proc is interrupt!\n",
				PS3_HOST(instance));
			ret = -PS3_FAILED;
			ps3_mutex_unlock(&instance->state_machine.lock);
			break;
		}
		ps3_mutex_unlock(&instance->state_machine.lock);

		ps3_msleep(PS3_LOOP_TIME_INTERVAL_20MS);
		fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
		count++;
	}

	if (fw_cur_state != PS3_FW_STATE_RUNNING) {
		LOG_ERROR("hno:%u  fw state transition NOK, state is %s\n",
			PS3_HOST(instance),
			ps3_ioc_state_print(fw_cur_state));
		if (ps3_pci_err_recovery_get(instance)) {
			LOG_WARN("hno:%u  pci recovery resetting\n", PS3_HOST(instance));
			ret = -PS3_IN_PCIE_ERR;
		} else {
			ret = -PS3_FAILED;
		}
	}

	LOG_INFO("hno:%u  fw state transit to %s\n",
		PS3_HOST(instance), ps3_ioc_state_print(fw_cur_state));

	return ret;
}

static S32 ps3_ioc_state_reset_request(struct ps3_instance *instance,
	enum ps3_reset_type reset_type)
{
	S32 ret = -PS3_FAILED;
	U32 ioc_reset_type = 0;
	U32 reset_status_mask = 0;
	U32 cur_state = 0;

	ioc_reset_type = g_reset_key_table[reset_type].reset_type;
	reset_status_mask = g_reset_key_table[reset_type].reset_status_mask;

	cur_state = ps3_atomic_read(&instance->state_machine.state);

	LOG_INFO("hno:%u  %s, key_offset: 0x%x, state_offset: 0x%x,"
		" reset_offset: 0x%x, status_offset: 0x%x, IOC reset_type: 0x%8x"
		" status mask: 0x%8x cur_state[%d]\n",
		PS3_HOST(instance), namePS3ResetType(reset_type),
		g_reset_key_table[reset_type].reset_key_offset,
		g_reset_key_table[reset_type].reset_state_offset,
		g_reset_key_table[reset_type].reset_offset,
		g_reset_key_table[reset_type].reset_status_offset,
		g_reset_key_table[reset_type].reset_type,
		g_reset_key_table[reset_type].reset_status_mask,
		cur_state);
	preempt_disable();
	ps3_reset_key_write(instance, g_reset_key_table[reset_type].reset_key_offset);
	preempt_enable();

	ps3_reset_key_state_check(instance, g_reset_key_table[reset_type].reset_state_offset);

	PS3_IOC_REG_WRITE_OFFSET(instance, g_reset_key_table[reset_type].reset_offset, (U64)ioc_reset_type);

	if (ps3_hard_reset_waiting_query()) {
		ps3_msleep(ps3_hard_reset_waiting_query());
	}

	if (ps3_reset_request_completion_check(instance, g_reset_key_table[reset_type].reset_status_offset,
		reset_status_mask) != PS3_SUCCESS) {
		goto l_out;
	}

	if (ps3_after_reset_request_check(instance, reset_type) !=
		PS3_SUCCESS) {
		goto l_out;
	}

	ret = PS3_SUCCESS;

l_out:
	LOG_INFO("hno:%u  PS3 %s complete, ret:%d\n",
		PS3_HOST(instance), namePS3ResetType(reset_type), ret);
	return ret;
}

static S32 ps3_ioc_state_hardreset_request(struct ps3_instance *instance,
	enum ps3_reset_type reset_type)
{
	S32 ret = -PS3_FAILED;
	U8 *reset_key_addr = NULL;
	U8 *reset_state_addr = NULL;
	U8 *reset_addr = NULL;
	U32 ioc_reset_type = 0;
	U32 cur_state = 0;
	U8 *reg_start = (U8*)instance->reg_set;
	U32 read_count = 0;
	const U32 retry_max  = 180;
	ULong flags;
	U64 timeval[PS3_RESET_MAX_COUNT] = {0};

	reset_key_addr = reg_start +
		g_reset_key_table[reset_type].reset_key_offset;
	reset_state_addr = reg_start +
		g_reset_key_table[reset_type].reset_state_offset;
	reset_addr = reg_start +
		g_reset_key_table[reset_type].reset_offset;
	ioc_reset_type = g_reset_key_table[reset_type].reset_type;

	cur_state = ps3_atomic_read(&instance->state_machine.state);

	LOG_INFO("hno:%u  %s, key_offset: 0x%x, state_offset: 0x%x,"
		" reset_offset: 0x%x, status_offset: 0x%x, IOC reset_type: 0x%8x"
		" status mask: 0x%8x cur_state[%d]\n",
		PS3_HOST(instance), namePS3ResetType(reset_type),
		g_reset_key_table[reset_type].reset_key_offset,
		g_reset_key_table[reset_type].reset_state_offset,
		g_reset_key_table[reset_type].reset_offset,
		g_reset_key_table[reset_type].reset_status_offset,
		g_reset_key_table[reset_type].reset_type,
		g_reset_key_table[reset_type].reset_status_mask,
		cur_state);
	instance->is_hard_reset = PS3_TRUE;
	mb();
	while(ps3_atomic_read(&instance->reg_op_count) != 0){
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);

		if(read_count++ > retry_max){
			LOG_INFO("hno:%u  %s, wait reg op over:%d ms,failed\n",
				PS3_HOST(instance), namePS3ResetType(reset_type),
				read_count*PS3_LOOP_TIME_INTERVAL_100MS);
			ret = -PS3_FAILED;
			goto l_out;
		}
	}
	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_4, instance)
	if (ps3_pci_err_recovery_get(instance)) {
		LOG_INFO("hno:%u pcie recovery proceess\n",
				PS3_HOST(instance));
		ret = -PS3_IN_PCIE_ERR;
		goto l_out;
	}	
	spin_lock_irqsave(&instance->recovery_context->ps3_hardreset_lock, flags);
	ps3_hardreset_key_write(instance, reset_key_addr, timeval);

	ret = ps3_hardreset_key_state_check(instance, reset_state_addr, timeval);
	if(ret != PS3_SUCCESS){
		spin_unlock_irqrestore(&instance->recovery_context->ps3_hardreset_lock, flags);
		LOG_INFO("hno:%u  %s, key check failed, ret:%d\n",
			PS3_HOST(instance), namePS3ResetType(reset_type),ret);
		goto l_out;
	}

	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_5, instance)
	if (ps3_pci_err_recovery_get(instance)) {
		spin_unlock_irqrestore(&instance->recovery_context->ps3_hardreset_lock, flags);
		LOG_WARN("hno:%u pcie recovery proceess\n",
				PS3_HOST(instance));
		ret = -PS3_IN_PCIE_ERR;
		goto l_out;
	}
	
	timeval[PS3_START_WRITE_HARDRESET_REG] = ps3_1970_now_ms_get();
	ps3_ioc_hardreset_reg_write(instance, (U64)ioc_reset_type, reset_addr, PS3_FALSE);
	timeval[PS3_END_WRITE_HARDRESET_REG] = ps3_1970_now_ms_get();
	spin_unlock_irqrestore(&instance->recovery_context->ps3_hardreset_lock, flags);
	LOG_INFO("hno:%u  %s, key_offset: key state success,"
	"then write hardreset ioc_reset_type:%u, reset_addr:%p\n",
		PS3_HOST(instance), namePS3ResetType(reset_type), ioc_reset_type, reset_addr);
	LOG_INFO("hno:%u time:%lld-%lld-%lld-%lld-%lld-%lld\n",
		PS3_HOST(instance), timeval[PS3_START_WRITE_KEY_REG], timeval[PS3_END_WRITE_KEY_REG], 
		timeval[PS3_START_WAIT_KEY_READY_REG], timeval[PS3_END_WAIT_KEY_READY_REG], 
		timeval[PS3_START_WRITE_HARDRESET_REG], timeval[PS3_END_WRITE_HARDRESET_REG]);

	instance->recovery_context->hardreset_count++;
	if (ps3_hard_reset_waiting_query()) {
		ps3_msleep(ps3_hard_reset_waiting_query());
	}
	LOG_INFO("hno:%u  %s, after sleep:%d ms\n",
		PS3_HOST(instance), namePS3ResetType(reset_type),ps3_hard_reset_waiting_query());

	instance->is_hard_reset = PS3_FALSE;
	if ((ret = ps3_after_reset_request_check(instance, reset_type)) !=
		PS3_SUCCESS) {
		goto l_out;
	}

	ret = PS3_SUCCESS;

l_out:
	instance->is_hard_reset = PS3_FALSE;
	LOG_INFO("hno:%u  PS3 %s complete, ret:%d\n",
		PS3_HOST(instance), namePS3ResetType(reset_type), ret);
	return ret;
}

S32 ps3_ioc_state_hard_reset(struct ps3_instance *instance)
{
	if (ps3_use_hard_reset_reg_query()) {
		return ps3_ioc_state_hardreset_request(instance, PS3_FW_HARD_RESET);
	} else {
		return ps3_ioc_state_reset_request(instance, PS3_FW_SHALLOW_SOFT_RESET);
	}
}

S32 ps3_ioc_state_shallow_soft_reset(struct ps3_instance *instance)
{
	return ps3_ioc_state_reset_request(instance, PS3_FW_SHALLOW_SOFT_RESET);
}

S32 ps3_ioc_state_deep_soft_reset(struct ps3_instance *instance)
{
	return ps3_ioc_state_reset_request(instance, PS3_FW_DEEP_SOFT_RESET);
}

static S32 ps3_trigger_ioc_state_change_by_doorbell(struct ps3_instance *instance,
	U32 expect_fw_state, U32 doorbell_trigger, u32 time_out)
{
	U32 fw_cur_state = PS3_FW_STATE_UNDEFINED;
	Bool is_doorbell_done = PS3_TRUE;
	S32 ret = PS3_SUCCESS;
	U32 count = 0;

	LOG2_WARN("hno:%u  expect fw state:%s, doorbell_trigger is %d\n",
		PS3_HOST(instance),
		ps3_ioc_state_print(expect_fw_state), doorbell_trigger);

	fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
	if (fw_cur_state == expect_fw_state) {
		goto l_out;
	}
	
	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_2, instance)
	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN("hno:%u pci recovery resetting\n", PS3_HOST(instance));
		ret = -PS3_IN_PCIE_ERR;
		goto l_out;
	}	
	ps3_ioc_state_trigger_transition(instance, doorbell_trigger);

	fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
	if (doorbell_trigger == PS3_REG_DOORBELL_STATE_TO_FAULT) {
		ps3_get_doorbell_done_with_check(instance, &is_doorbell_done);
	}
	while (count < time_out) {
		if (fw_cur_state == expect_fw_state && is_doorbell_done) {
			break;
		}

		ps3_msleep(PS3_LOOP_TIME_INTERVAL_20MS);
		fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
		if (fw_cur_state == PS3_FW_STATE_MASK) {
			LOG_ERROR("hno:%u  break because get fw_cur_state NOK.\n",
				PS3_HOST(instance));
			break;
		}

		if (doorbell_trigger == PS3_REG_DOORBELL_STATE_TO_FAULT && !is_doorbell_done) {
			if (ps3_get_doorbell_done_with_check(instance, &is_doorbell_done) == PS3_FALSE) {
				LOG_ERROR("hno:%u  break because get doorbell_done NOK.\n",
					PS3_HOST(instance));
				break;
			}
		}

		count++;
	}
	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_DOORBELL_FAILED, &fw_cur_state)
	if (fw_cur_state != expect_fw_state) {
		LOG_ERROR("hno:%u  fw state transition NOK, is_doorbell_done %d state is %s\n",
			PS3_HOST(instance),
			is_doorbell_done,
			ps3_ioc_state_print(fw_cur_state));
		ret = -PS3_FAILED;
	}
l_out:
	return ret;
}

S32 ps3_ioc_notify_unload(struct ps3_instance *instance)
{
	Bool is_unload_valid = PS3_FALSE;
	S32 ret = PS3_SUCCESS;
	U32 count = 0;
	U8 unload_type = PS3_CMD_TRIGGER_UNLOAD;

	LOG_WARN("hno:%u  trigger ioc unload reg!!\n",
		PS3_HOST(instance));

	if (instance->state_machine.is_suspend) {
		unload_type = PS3_CMD_TRIGGER_UNLOAD_SUSPEND;
	}

	ps3_ioc_debug0_trigger(instance, unload_type);

	ps3_check_debug0_valid_with_check(instance, &is_unload_valid,
		unload_type);
	while (count < PS3_FW_STATE_TO_UNLOAD_TMO_LOOP_COUNT) {
		if (!is_unload_valid) {
			break;
		}

		ps3_msleep(PS3_LOOP_TIME_INTERVAL_20MS);
		ps3_check_debug0_valid_with_check(instance, &is_unload_valid,
			unload_type);
		count++;
	}

	if (is_unload_valid) {
		LOG_ERROR("hno:%u  do not wait unload done\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
	}

	return ret;
}

S32 ps3_ioc_state_force_to_fault(struct ps3_instance *instance)
{
	return ps3_trigger_ioc_state_change_by_doorbell(instance,
		PS3_FW_STATE_FAULT, PS3_REG_DOORBELL_STATE_TO_FAULT,
		PS3_FW_STATE_TO_FAULT_TMO_LOOP_COUNT);
}

S32 ps3_ioc_state_force_to_halt(struct ps3_instance *instance)
{
	return ps3_trigger_ioc_state_change_by_doorbell(instance,
		PS3_FW_STATE_HALT, PS3_REG_DOORBELL_STATE_TO_HALT,
		PS3_FW_STATE_TO_HALT_TMO_LOOP_COUNT);
}

#ifdef PS3_HARDWARE_ASIC
U32 ps3_ioc_heartbeat_detect(struct ps3_instance *instance)
{
	U32 ret = PS3_FALSE;
	U64 heartbeat_value = 0;

	(void) instance;

	if (!ps3_enable_heartbeat_query()) {
		return ret;
	}

	if (!ps3_ioc_heartbeat_get(instance, &heartbeat_value)) {
		LOG_DEBUG("hno:%u probably Linkdown\n", PS3_HOST(instance));
		return ret;
	}

	if (heartbeat_value & instance->hard_dog_mask) {
		LOG_DEBUG("hno:%u heartbeat detect success\n", PS3_HOST(instance));
		ret = PS3_TRUE;
	}

	return ret;
}
#endif
