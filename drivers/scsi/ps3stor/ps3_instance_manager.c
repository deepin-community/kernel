
#include "ps3_instance_manager.h"
#include "ps3_driver_log.h"
#include "ps3_ioc_manager.h"

#ifndef  _WINDOWS
#include "ps3_debug.h"
#include <linux/cpu.h>
#endif
#include "ps3_cli_debug.h"

#ifdef PS3_HARDWARE_HAPS_V200
#define PS3_INSTANCE_WAIT_TO_OPERATIONAL_MAX_SECS	(180) 
#define PS3_INSTANCE_WAIT_TO_NORMAL_MAX_SECS		(3600) 
#define PS3_INSTANCE_STATE_CHECK_INTERVAL_MS		(1000)	
#define PS3_INSTANCE_STATE_CHECK_NORMAL_INTERVAL_MS		(3000)	
#else
#define PS3_INSTANCE_WAIT_TO_OPERATIONAL_MAX_SECS	(180) 
#define PS3_INSTANCE_WAIT_TO_NORMAL_MAX_SECS	        (420) 
#define PS3_INSTANCE_STATE_CHECK_INTERVAL_MS		(1000)	
#endif

struct ps3_host_info g_ps3_host_info;

#ifndef  _WINDOWS
struct ps3_mgmt_info *ps3_mgmt_info_get(void)
{
	static struct ps3_mgmt_info g_mgmt_info;

	return &g_mgmt_info;
}

struct ps3_instance *ps3_instance_lookup(U16 host_no)
{
	struct ps3_instance *instance = NULL;
	struct list_head *pitem = NULL;
	list_for_each(pitem, &ps3_mgmt_info_get()->instance_list_head) {
		instance = list_entry(pitem, struct ps3_instance, list_item);
		if (instance->host->host_no == host_no) {
			return instance;
		}
	}

	return NULL;
}

S32 ps3_instance_add(struct ps3_instance *instance)
{
	S32 ret = -PS3_FAILED;
	struct list_head *pitem = NULL;
	struct ps3_instance *peer_instance = NULL;

	if (instance == NULL) {
		return ret;
	}

	spin_lock(&ps3_mgmt_info_get()->spin_lock_obj);
	list_for_each(pitem, &ps3_mgmt_info_get()->instance_list_head) {
		if (pitem == &instance->list_item) {
			pitem = NULL;
			goto l_repeat_add;
		}
	}

	list_for_each_entry(peer_instance, &ps3_mgmt_info_get()->instance_list_head, list_item) {
		if (peer_instance != NULL &&
			ps3_get_pci_domain(peer_instance->pdev) == ps3_get_pci_domain(instance->pdev) &&
			peer_instance->pdev->bus->number == instance->pdev->bus->number &&
			PCI_SLOT(peer_instance->pdev->devfn) == PCI_SLOT(instance->pdev->devfn)) {
			INJECT_START(PS3_ERR_IJ_ASYNC_HARDRESET_PROBE, peer_instance)
			peer_instance->recovery_context->instance_change = 1;
			mb();
			ps3_recovery_cancel_work_sync(peer_instance);
			instance->peer_instance = peer_instance;
			peer_instance->peer_instance = instance;
			mb();
			peer_instance->recovery_context->instance_change = 0;
		}
	}

	list_add(&instance->list_item, &ps3_mgmt_info_get()->instance_list_head);
	mutex_init(&instance->state_machine.lock);
	atomic_set(&instance->state_machine.state, PS3_INSTANCE_STATE_INIT);
	instance->state_machine.is_load = PS3_FALSE;
	instance->state_machine.is_pci_err_recovery = PS3_FALSE;

	ret = PS3_SUCCESS;
l_repeat_add:
	spin_unlock(&ps3_mgmt_info_get()->spin_lock_obj);
	if (pitem == NULL) {
		LOG_WARN("hno:%u  repeat add instance!\n",
			PS3_HOST(instance));
	}
	return ret;
}

S32 ps3_instance_remove(struct ps3_instance *instance)
{
	struct ps3_instance *peer_instance = NULL;

	if (instance == NULL) {
		return -PS3_FAILED;
	}

	spin_lock(&ps3_mgmt_info_get()->spin_lock_obj);
	list_del(&instance->list_item);
	instance->peer_instance = NULL;
	list_for_each_entry(peer_instance, &ps3_mgmt_info_get()->instance_list_head, list_item) {
		if (peer_instance != NULL &&
			ps3_get_pci_domain(peer_instance->pdev) == ps3_get_pci_domain(instance->pdev) &&
			peer_instance->pdev->bus->number == instance->pdev->bus->number &&
			PCI_SLOT(peer_instance->pdev->devfn) == PCI_SLOT(instance->pdev->devfn)) {
			INJECT_START(PS3_ERR_IJ_ASYNC_HARDRESET_REMOVE, peer_instance)
			peer_instance->recovery_context->instance_change = 1;
			mb();
			ps3_recovery_cancel_work_sync(peer_instance);
			peer_instance->peer_instance = NULL;
			mb();
			peer_instance->recovery_context->instance_change = 0;
		}
	}
	mutex_destroy(&instance->state_machine.lock);
	ps3_mutex_destroy(&instance->task_mgr_reset_lock);
	ps3_mutex_destroy(&instance->task_abort_lock);
	spin_unlock(&ps3_mgmt_info_get()->spin_lock_obj);

	return PS3_SUCCESS;
}

void ps3_mgmt_info_init(void)
{
	ps3_spin_lock_init(&ps3_mgmt_info_get()->spin_lock_obj);
	INIT_LIST_HEAD(&ps3_mgmt_info_get()->instance_list_head);

	ps3_cli_debug_init();

	return;
}
#endif
void ps3_instance_init(struct ps3_instance *instance)
{
	memset(instance, 0, sizeof(struct ps3_instance));
	ps3_mutex_init(&instance->state_machine.lock);
	ps3_atomic_set(&instance->state_machine.state, PS3_INSTANCE_STATE_INIT);
	instance->state_machine.is_load = PS3_FALSE;
	instance->state_machine.is_poweroff = PS3_FALSE;
	instance->state_machine.is_pci_err_recovery = PS3_FALSE;
}

S32 ps3_instance_state_transfer(struct ps3_instance *instance,
	U32 exp_cur_state, U32 dest_state)
{
	U32 cur_state = 0;
	ps3_mutex_lock(&instance->state_machine.lock);
#if 0
	由于当前硬件不支持拦截dma等通路，只能通过硬复位
	来防止片内踩主机内存，所以状态需要正常切换

	if (!instance->state_machine.is_load) {
		LOG_ERROR("hno:%u  driver is unload or suspend!\n",
			PS3_HOST(instance));
		goto l_fail;
	}
#endif
	cur_state = ps3_atomic_read(&instance->state_machine.state);
	if (cur_state != exp_cur_state) {
		goto l_fail;
	}

	switch(cur_state) {
	case PS3_INSTANCE_STATE_INIT:
		if ((dest_state == PS3_INSTANCE_STATE_READY) || 
				(dest_state == PS3_INSTANCE_STATE_RECOVERY)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_READY:
		if ((dest_state == PS3_INSTANCE_STATE_PRE_OPERATIONAL) ||
			(dest_state == PS3_INSTANCE_STATE_RECOVERY) ||
			(dest_state == PS3_INSTANCE_STATE_PCIE_RECOVERY)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_PRE_OPERATIONAL:
		if ((dest_state == PS3_INSTANCE_STATE_OPERATIONAL) ||
			(dest_state == PS3_INSTANCE_STATE_RECOVERY) ||
			(dest_state == PS3_INSTANCE_STATE_SOFT_RECOVERY)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_OPERATIONAL:
		if ((dest_state == PS3_INSTANCE_STATE_SOFT_RECOVERY) ||
			(dest_state == PS3_INSTANCE_STATE_RECOVERY) ||
			(dest_state == PS3_INSTANCE_STATE_SUSPEND) ||
			(dest_state == PS3_INSTANCE_STATE_QUIT)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_SOFT_RECOVERY:
		if ((dest_state == PS3_INSTANCE_STATE_PRE_OPERATIONAL)
			|| (dest_state == PS3_INSTANCE_STATE_RECOVERY)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_RECOVERY:
		if ((dest_state == PS3_INSTANCE_STATE_READY)
			|| (dest_state == PS3_INSTANCE_STATE_DEAD)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_SUSPEND:
		if (dest_state == PS3_INSTANCE_STATE_READY) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_DEAD:
		if (dest_state == PS3_INSTANCE_STATE_QUIT) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_QUIT:
		goto l_fail;
	case PS3_INSTANCE_STATE_PCIE_RECOVERY:
		if (dest_state == PS3_INSTANCE_STATE_RECOVERY) {
			goto l_success;
		} else {
			goto l_fail;
		}
	default:
		goto l_fail;
	}

l_fail:
	ps3_mutex_unlock(&instance->state_machine.lock);
	LOG_ERROR("hno:%u  state transfer NOK!"
		"[exp_cur_state:%s][cur_state:%s][dest_state:%s]\n",
		PS3_HOST(instance),
		namePS3InstanceState(exp_cur_state),
		namePS3InstanceState(cur_state),
		namePS3InstanceState(dest_state));
	return -PS3_FAILED;
l_success:
	ps3_atomic_set(&instance->state_machine.state, dest_state);
	ps3_mutex_unlock(&instance->state_machine.lock);
	LOG_WARN("hno:%u  state transfer from %s to %s!\n",
		PS3_HOST(instance),
		namePS3InstanceState(cur_state),
		namePS3InstanceState(dest_state));
	return PS3_SUCCESS;
}
S32 ps3_instance_no_lock_state_transfer(struct ps3_instance *instance,
	U32 dest_state)
{
	U32 cur_state = 0;
#if 0
	由于当前硬件不支持拦截dma等通路，只能通过硬复位
	来防止片内踩主机内存，所以状态需要正常切换
	if (!instance->state_machine.is_load) {
		LOG_ERROR("hno:%u  driver is unload or suspend!\n",
			PS3_HOST(instance));
		goto l_fail;
	}
#endif
	cur_state = ps3_atomic_read(&instance->state_machine.state);
	if(cur_state == dest_state){
		goto l_success;
	}
	switch(cur_state) {
	case PS3_INSTANCE_STATE_INIT:
		if ((dest_state == PS3_INSTANCE_STATE_READY) ||
			(dest_state == PS3_INSTANCE_STATE_RECOVERY)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_READY:
		if ((dest_state == PS3_INSTANCE_STATE_PRE_OPERATIONAL) ||
			(dest_state == PS3_INSTANCE_STATE_RECOVERY)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_PRE_OPERATIONAL:
		if ((dest_state == PS3_INSTANCE_STATE_OPERATIONAL) ||
			(dest_state == PS3_INSTANCE_STATE_RECOVERY) ||
			(dest_state == PS3_INSTANCE_STATE_SOFT_RECOVERY)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_OPERATIONAL:
		if ((dest_state == PS3_INSTANCE_STATE_SOFT_RECOVERY) ||
			(dest_state == PS3_INSTANCE_STATE_RECOVERY) ||
			(dest_state == PS3_INSTANCE_STATE_SUSPEND) ||
			(dest_state == PS3_INSTANCE_STATE_QUIT)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_SOFT_RECOVERY:
		if ((dest_state == PS3_INSTANCE_STATE_PRE_OPERATIONAL)
			|| (dest_state == PS3_INSTANCE_STATE_RECOVERY)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_RECOVERY:
		if ((dest_state == PS3_INSTANCE_STATE_READY)
			|| (dest_state == PS3_INSTANCE_STATE_DEAD)) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_SUSPEND:
		if (dest_state == PS3_INSTANCE_STATE_READY) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_DEAD:
		if (dest_state == PS3_INSTANCE_STATE_QUIT) {
			goto l_success;
		} else {
			goto l_fail;
		}
		break;
	case PS3_INSTANCE_STATE_QUIT:
		goto l_fail;
	case PS3_INSTANCE_STATE_PCIE_RECOVERY:
		if (dest_state == PS3_INSTANCE_STATE_RECOVERY) {
			goto l_success;
		} else {
			goto l_fail;
		}
	default:
		goto l_fail;
	}

l_fail:
	LOG_ERROR("hno:%u  state transfer NOK!"
		"[cur_state:%s][dest_state:%s]\n",
		PS3_HOST(instance),
		namePS3InstanceState(cur_state),
		namePS3InstanceState(dest_state));
	return -PS3_FAILED;
l_success:
	ps3_atomic_set(&instance->state_machine.state, dest_state);
	LOG_WARN("hno:%u  state transfer from %s to %s!\n",
		PS3_HOST(instance),
		namePS3InstanceState(cur_state),
		namePS3InstanceState(dest_state));
	return PS3_SUCCESS;
}

void ps3_instance_state_transfer_to_dead(struct ps3_instance *instance)
{
	ps3_mutex_lock(&instance->state_machine.lock);
	if(ps3_atomic_read(&instance->state_machine.state) != PS3_INSTANCE_STATE_DEAD){
		ps3_atomic_set(&instance->state_machine.state,
			PS3_INSTANCE_STATE_DEAD);
	}
	ps3_mutex_unlock(&instance->state_machine.lock);
}

void ps3_instance_state_transfer_to_pcie_recovery(struct ps3_instance *instance)
{
	ps3_mutex_lock(&instance->state_machine.lock);
	if(ps3_atomic_read(&instance->state_machine.state) != PS3_INSTANCE_STATE_PCIE_RECOVERY){
		ps3_atomic_set(&instance->state_machine.state,
			PS3_INSTANCE_STATE_PCIE_RECOVERY);
	}
	ps3_mutex_unlock(&instance->state_machine.lock);
}

void ps3_instance_state_transfer_to_quit(struct ps3_instance *instance)
{
	ps3_atomic_set(&instance->state_machine.state,
		PS3_INSTANCE_STATE_QUIT);
}

void ps3_instance_state_transfer_to_suspend(struct ps3_instance *instance)
{
	ps3_atomic_set(&instance->state_machine.state,
		PS3_INSTANCE_STATE_SUSPEND);
}

void ps3_instance_state_transition_to_recovery(struct ps3_instance *instance)
{
	ps3_atomic_set(&instance->state_machine.state,
		PS3_INSTANCE_STATE_RECOVERY);
}

S32 ps3_instance_wait_for_hard_reset_flag_done(struct ps3_instance *instance)
{
	U32 wait_cnt = PS3_INSTANCE_WAIT_TO_OPERATIONAL_MAX_SECS * 2;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;
	U32 idx = 0;
	S32 ret = PS3_SUCCESS;
	for (idx = 0;idx < wait_cnt;idx++) {
		cur_state = ps3_atomic_read(&instance->state_machine.state);
		if ((cur_state == PS3_INSTANCE_STATE_DEAD) ||
			(cur_state == PS3_INSTANCE_STATE_QUIT)){
			ret = -PS3_FAILED;
			goto l_out;
		}
		ps3_mutex_lock(&instance->state_machine.lock);
		if (instance->recovery_context->host_reset_state == PS3_HOST_RESET_HARD_RESET_DONE &&
			ps3_atomic_read(&instance->state_machine.state) ==  PS3_INSTANCE_STATE_OPERATIONAL){
			if(atomic_read(&instance->cmd_statistics.io_outstanding) != 0) {
				ret = -PS3_RETRY;
			}
		    ps3_mutex_unlock(&instance->state_machine.lock);
			goto l_out;
		}
		ps3_mutex_unlock(&instance->state_machine.lock);

		ps3_msleep(PS3_INSTANCE_STATE_CHECK_INTERVAL_MS);
	}

	if (idx >= wait_cnt) {
		LOG_WARN("hno:%u  wait pre_operational timeout!\n",
			PS3_HOST(instance));
	}
	ps3_mutex_lock(&instance->state_machine.lock);
	if (instance->recovery_context->host_reset_state != PS3_HOST_RESET_HARD_RESET_DONE) {
		LOG_WARN("hno:%u  wait host reset hard reset done NOK!\n",
			PS3_HOST(instance));
		ret = -PS3_RETRY;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);
l_out:
	LOG_INFO("hno:%u  wait hard reset flag %d!\n", PS3_HOST(instance), ret);
	return ret;
}

S32 ps3_instance_wait_for_dead_or_pre_operational(struct ps3_instance *instance)
{
	U32 wait_cnt = PS3_INSTANCE_WAIT_TO_OPERATIONAL_MAX_SECS * 3;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;
	U32 idx = 0;

	LOG_INFO("hno:%u  wait dead or pre_operational begin\n", PS3_HOST(instance));
	for (idx = 0; idx < wait_cnt; idx++) {
		cur_state = ps3_atomic_read(&instance->state_machine.state);
		if ((cur_state == PS3_INSTANCE_STATE_PRE_OPERATIONAL) ||
			(cur_state == PS3_INSTANCE_STATE_OPERATIONAL) ||
			(cur_state == PS3_INSTANCE_STATE_DEAD) ||
			(cur_state == PS3_INSTANCE_STATE_QUIT)) {
			break;
		}

		ps3_msleep(PS3_INSTANCE_STATE_CHECK_INTERVAL_MS);
	}

	if (idx >= wait_cnt) {
		LOG_WARN("hno:%u  wait dead or pre_operational timeout!\n",
			PS3_HOST(instance));
	}

	if ((cur_state != PS3_INSTANCE_STATE_PRE_OPERATIONAL) &&
		(cur_state != PS3_INSTANCE_STATE_OPERATIONAL) &&
		(cur_state != PS3_INSTANCE_STATE_DEAD)) {
		LOG_WARN("hno:%u  wait dead or pre_operational NOK!\n",
			PS3_HOST(instance));
		return -PS3_FAILED;

	}

	LOG_INFO("hno:%u  wait dead or pre_operational success!\n", PS3_HOST(instance));
	return PS3_SUCCESS;
}

S32 ps3_instance_wait_for_operational(struct ps3_instance *instance, Bool is_hardreset)
{
	U32 wait_cnt = PS3_INSTANCE_WAIT_TO_OPERATIONAL_MAX_SECS * 3;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;
	U32 idx = 0;
	S32 recovery_state = PS3_RESET_LOG_INTERVAL;

	for (idx = 0; idx < wait_cnt; idx++) {
		if (!instance->state_machine.is_load) {
			LOG_ERROR("hno:%u instance state not is_load\n", PS3_HOST(instance));
			break;
		}

		ps3_mutex_lock(&instance->state_machine.lock);
		cur_state = ps3_atomic_read(&instance->state_machine.state);
		recovery_state = instance->recovery_context->recovery_state;
		if (((cur_state == PS3_INSTANCE_STATE_OPERATIONAL) ||
			(cur_state == PS3_INSTANCE_STATE_DEAD) ||
			(cur_state == PS3_INSTANCE_STATE_QUIT)) &&
			(is_hardreset ? (recovery_state == PS3_HARD_RECOVERY_FINISH) : PS3_TRUE)) {
			LOG_INFO("hno:%u  wait break, cur_state: %s, recovery_state: %d, is_hardreset: %d\n",
				PS3_HOST(instance), namePS3InstanceState(cur_state), recovery_state, is_hardreset);
			ps3_mutex_unlock(&instance->state_machine.lock);
			break;
		}
		ps3_mutex_unlock(&instance->state_machine.lock);

		ps3_msleep(PS3_INSTANCE_STATE_CHECK_INTERVAL_MS);
	}

	if (idx >= wait_cnt) {
		LOG_WARN("hno:%u  wait operational timeout!\n",
			PS3_HOST(instance));
	}

	if (cur_state != PS3_INSTANCE_STATE_OPERATIONAL) {
		LOG_WARN("hno:%u  wait operational NOK, cur_state: %s, recovery_state: %d\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state), recovery_state);
		return -PS3_FAILED;

	}

	LOG_INFO("hno:%u  wait operational success!\n", PS3_HOST(instance));
	return PS3_SUCCESS;
}

S32 ps3_instance_wait_for_normal(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 wait_cnt = PS3_INSTANCE_WAIT_TO_NORMAL_MAX_SECS;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;
	U32 idx = 0;
	Bool is_pci_err_recovery = PS3_FALSE;

	for (idx = 0; idx < wait_cnt; idx++) {
		is_pci_err_recovery = ps3_pci_err_recovery_get(instance);
		cur_state = ps3_atomic_read(&instance->state_machine.state);
		if (cur_state == PS3_INSTANCE_STATE_DEAD &&
			PS3_IOC_STATE_HALT_SUPPORT(instance) &&
			PS3_HALT_CLI_SUPPORT(instance)) {
			goto l_out;
    	}

		if (ps3_state_is_normal(cur_state) ||
			cur_state == PS3_INSTANCE_STATE_QUIT ||
			cur_state == PS3_INSTANCE_STATE_DEAD ||
			is_pci_err_recovery) {
			LOG_DEBUG("hno:%u cur state: %d, pci err recovery: %d\n",
				PS3_HOST(instance), cur_state, is_pci_err_recovery);
			break;
		}

		if (!(idx % PS3_RESET_LOG_INTERVAL)) {
			LOG_DEBUG("hno:%u state[%s] waiting for reset to finish\n",
				PS3_HOST(instance), namePS3InstanceState(cur_state));
		}
#ifdef PS3_HARDWARE_HAPS_V200
		ps3_msleep(PS3_INSTANCE_STATE_CHECK_NORMAL_INTERVAL_MS);

		if (ps3_get_wait_cli_flag()) {
			LOG_DEBUG("hno:%u not wait cmd\n", PS3_HOST(instance));
			break;
		}
#else
		ps3_msleep(PS3_INSTANCE_STATE_CHECK_INTERVAL_MS);
#endif
	}

	if (!ps3_state_is_normal(cur_state) || is_pci_err_recovery) {
		LOG_WARN("hno:%u cannot handle cmd, driver state: %s, pci recovery: %d\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state), is_pci_err_recovery);
		ret = -PS3_FAILED;
		goto l_out;
	}

l_out:
	return ret;
}

S32 ps3_recovery_state_wait_for_normal(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 wait_cnt = PS3_INSTANCE_WAIT_TO_NORMAL_MAX_SECS;
	S32 recovery_state = PS3_RESET_LOG_INTERVAL;
	U32 idx = 0;
	Bool is_pci_err_recovery = PS3_FALSE;

	for (idx = 0; idx < wait_cnt; idx++) {
		recovery_state = instance->recovery_context->recovery_state;
		is_pci_err_recovery = ps3_pci_err_recovery_get(instance);

		if ( recovery_state == PS3_HARD_RECOVERY_FINISH ||
			 recovery_state == PS3_SOFT_RECOVERY_PROBE_PROCESS ||
			is_pci_err_recovery) {
			LOG_DEBUG("hno:%u recovery state: %d, pci err recovery: %d\n",
				PS3_HOST(instance), recovery_state, is_pci_err_recovery);
			goto l_out;
		}
#ifdef PS3_HARDWARE_HAPS_V200
		ps3_msleep(PS3_INSTANCE_STATE_CHECK_NORMAL_INTERVAL_MS);

		if (ps3_get_wait_cli_flag()) {
			LOG_DEBUG("hno:%u not wait cmd\n", PS3_HOST(instance));
			break;
		}
#else
		ps3_msleep(PS3_INSTANCE_STATE_CHECK_INTERVAL_MS);
#endif
	}

	ret = -PS3_FAILED;
	LOG_WARN("hno:%u wait recovery time out, recovery_state: %d\n",
		PS3_HOST(instance), recovery_state);
l_out:
	if (is_pci_err_recovery) {
		LOG_WARN("hno:%u cannot handle cmd, pci recovery: %d\n",
			PS3_HOST(instance), is_pci_err_recovery);
		ret = -PS3_FAILED;
	}
	return ret;
}

void ps3_host_info_get(void)
{
#if defined(CONFIG_X86) || defined(CONFIG_X86_64)
	struct cpuinfo_x86 *cpu = NULL;

	memset(&g_ps3_host_info, 0, sizeof(struct ps3_host_info));
	cpu = &cpu_data(0);
	g_ps3_host_info.machine = PS3_HOST_MACHINE_X86;
	if (strstr(cpu->x86_vendor_id, "Intel"))  {
		g_ps3_host_info.vendor = PS3_HOST_VENDOR_INTEL;
	} else if (strstr(cpu->x86_vendor_id, "Hygon")) {
		g_ps3_host_info.vendor = PS3_HOST_VENDOR_HYGON;
	} else if (strstr(cpu->x86_vendor_id, "AMD")) {
		g_ps3_host_info.vendor = PS3_HOST_VENDOR_AMD;
	}
	g_ps3_host_info.processor_cnt = num_online_cpus();
#else
	memset(&g_ps3_host_info, 0, sizeof(struct ps3_host_info));
#endif
	LOG_DEBUG("host info: machine=%u,vendor=%u,processor_cnt=%u\n", 
		g_ps3_host_info.machine, g_ps3_host_info.vendor, g_ps3_host_info.processor_cnt);
}

U16 ps3_host_vendor_get(void) 
{
	return g_ps3_host_info.vendor;
}
