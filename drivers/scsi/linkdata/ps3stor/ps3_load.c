// SPDX-License-Identifier: GPL-2.0
#include "ps3_load.h"
#include "ps3_instance_manager.h"
#include "ps3_ioc_manager.h"
#include "ps3_mgr_cmd_err.h"
#include "ps3_mgr_cmd.h"
#include "ps3_cmd_channel.h"
#include "ps3_event.h"
#include "ps3_cmd_statistics.h"
#include "ps3_ioctl.h"

#ifdef _WINDOWS

static int ps3_init_ioc_prepare(struct ps3_instance *instance);
static void ps3_init_ioc_prepare_exit(struct ps3_instance *instance);
static int ps3_init_ioc_complete(struct ps3_instance *instance);
static void ps3_init_ioc_complete_exit(struct ps3_instance *instance);
static int ps3_pci_init_complete(struct ps3_instance *instance);
static void ps3_pci_init_complete_exit(struct ps3_instance *instance);

int ps3_firmware_init(struct ps3_instance *instance)
{
	ps3_ioc_adp_init(instance);

	if (instance->ioc_adpter->ioc_init_state_to_ready(instance) !=
	    PS3_SUCCESS) {
		goto l_failed;
	}
	ps3_atomic_set(&instance->state_machine.state,
		       PS3_INSTANCE_STATE_READY);

	if (ps3_pci_init_complete(instance) != PS3_SUCCESS)
		goto l_failed;

	if (ps3_init_ioc_prepare(instance) != PS3_SUCCESS)
		goto l_failed;

	if (instance->ioc_adpter->ioc_init_proc(instance) != PS3_SUCCESS)
		goto l_failed;
	ps3_atomic_set(&instance->state_machine.state,
		       PS3_INSTANCE_STATE_PRE_OPERATIONAL);

	if (ps3_ctrl_info_get(instance) != PS3_SUCCESS)
		goto l_failed;

	if (ps3_init_ioc_complete(instance) != PS3_SUCCESS)
		goto l_failed;

	ps3_ioctl_init(instance, PS3_MAX_IOCTL_CMDS);

	return PS3_SUCCESS;

l_failed:
	DbgPrint("fireware init failed\n");
	ps3_firmware_exit(instance);
	return -PS3_FAILED;
}

void ps3_firmware_exit(struct ps3_instance *instance)
{
	ps3_pci_init_complete_exit(instance);
	ps3_init_ioc_complete_exit(instance);
	ps3_init_ioc_prepare_exit(instance);
}

void ps3_remove(struct ps3_instance *instance)
{
	unsigned long flags = 0;

	LOG_INFO("hno:%u %s\n", PS3_HOST(instance), __func__);
	instance->state_machine.is_load = PS3_FALSE;
	instance->ioc_adpter->irq_disable(instance);

	ps3_watchdog_stop(instance);
	ps3_recovery_context_exit(instance);

	ps3_event_unsubscribe(instance);
	ps3_spin_lock_irqsave(&instance->recov	ery_context->recovery_lock,
			      &flags);
	ps3_atomic_set(&instance->event_context.abort_eventcmd, 0);
	ps3_atomic_set(&instance->dev_context.abort_vdpending_cmd, 0);
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock,
				   flags);
	ps3_dev_mgr_vd_info_unsubscribe(instance);

#ifndef _WINDOWS
	ps3_sas_device_data_exit(instance);
#endif
	ps3_device_mgr_data_exit(instance);
#ifndef _WINDOWS
	if (ps3_sas_is_support_smp(instance))
		sas_remove_host(instance->host);
	else
		scsi_remove_host(instance->host);
#else
	ps3_device_unload_done(instance);
#endif
	if (ps3_soc_unload(instance, PS3_FALSE, PS3_UNLOAD_SUB_TYPE_REMOVE,
			   PS3_SUSPEND_TYPE_NONE) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u unload failed.\n", PS3_HOST(instance));
		if (ps3_ioc_hard_reset_to_ready(instance) != PS3_SUCCESS) {
			LOG_ERROR("hno:%u hard reset failed.\n",
				  PS3_HOST(instance));
		}
	}

#ifndef _WINDOWS
	ps3_irqs_sync(instance);
	ps3_irqpolls_enable(instance);
#endif
	ps3_instance_state_transfer_to_quit(instance);

	ps3_firmware_exit(instance);
	ps3_pci_exit(instance);
#ifndef _WINDOWS
	pci_set_drvdata(instance->pdev, NULL);
	ps3_instance_put(instance);
	scsi_host_put(instance->host);

	ps3_dma_dump_mapping(pdev);
#endif
}

static void ps3_cmd_attr_context_init(struct ps3_instance *instance)
{
	unsigned int cmd_qdepth = 0;

	instance->cmd_attr.cur_can_que =
		instance->cmd_context.max_scsi_cmd_count -
		instance->cmd_context.max_r1x_cmd_count;

#ifndef _WINDOWS

	cmd_qdepth = ps3_throttle_qdepth_query();
	if ((cmd_qdepth != 0) && cmd_qdepth <= instance->cmd_attr.cur_can_que) {
		instance->cmd_attr.throttle_que_depth = cmd_qdepth;
	} else {
		instance->cmd_attr.throttle_que_depth =
			PS3_DEVICE_QDEPTH_DEFAULT_VALUE;
	}
#else
	instance->cmd_attr.throttle_que_depth = PS3_DEVICE_QDEPTH_DEFAULT_VALUE;
#endif

	instance->cmd_attr.vd_io_threshold = 0;
	instance->cmd_attr.is_support_direct_cmd = PS3_FALSE;
}

static int ps3_init_ioc_prepare(struct ps3_instance *instance)
{
	ps3_ioc_mgr_req_queue_lock_init(instance);
	ps3_err_fault_context_init(instance);

	if (!ps3_ioc_fw_version_get(instance))
		goto l_fail;

	if (!ps3_ioc_state_halt_support_get(instance))
		goto l_fail;

	if (!ps3_ioc_recovery_support_get(instance))
		goto l_fail;

	if (ps3_recovery_context_init(instance) != PS3_SUCCESS)
		goto l_fail;

	if (ps3_ioc_init_cmd_context_init(instance) != PS3_SUCCESS)
		goto l_fail;

	if (ps3_ctrl_info_buf_alloc(instance) != PS3_SUCCESS)
		goto l_fail;

	if (ps3_cmd_context_init(instance) != PS3_SUCCESS)
		goto l_fail;

	ps3_cmd_attr_context_init(instance);

	if (ps3_event_context_init(instance) != PS3_SUCCESS)
		goto l_fail;

	if (ps3_cmd_statistics_init(instance) != PS3_SUCCESS)
		goto l_fail;

	if (ps3_dump_init(instance) != PS3_SUCCESS)
		goto l_fail;

	return PS3_SUCCESS;
l_fail:
	ps3_init_ioc_prepare_exit(instance);
	return -PS3_FAILED;
}

static void ps3_init_ioc_prepare_exit(struct ps3_instance *instance)
{
	ps3_ioc_init_cmd_context_exit(instance);
	ps3_err_fault_context_exit(instance);
	ps3_ctrl_info_buf_free(instance);
	ps3_cmd_statistics_exit(instance);
	ps3_event_context_exit(instance);
	ps3_recovery_context_exit(instance);
	ps3_cmd_context_exit(instance);
	ps3_dump_dma_buf_free(instance);
	ps3_dump_exit(instance);
#ifndef _WINDOWS
	(void)ps3_debug_mem_free(instance);
#endif
}

static int ps3_init_ioc_complete(struct ps3_instance *instance)
{
	if (ps3_mgr_cmd_init(instance) != PS3_SUCCESS)
		goto l_failed;

	if (ps3_device_mgr_init(instance) != PS3_SUCCESS)
		goto l_failed;
#ifndef _WINDOWS
	if (ps3_sas_device_mgr_init(instance) != PS3_SUCCESS)
		goto l_failed;
#endif
	return PS3_SUCCESS;
l_failed:
	ps3_init_ioc_complete_exit(instance);
	return -PS3_FAILED;
}

static void ps3_init_ioc_complete_exit(struct ps3_instance *instance)
{
#ifndef _WINDOWS
	ps3_sas_device_mgr_exit(instance);
#endif
	ps3_device_mgr_exit(instance);
	ps3_mgr_cmd_exit(instance);
}

static int ps3_pci_init_complete(struct ps3_instance *instance)
{
	if (ps3_irq_context_init(instance) != PS3_SUCCESS)
		goto l_failed;
	if (instance->ioc_adpter->irq_init) {
		if (instance->ioc_adpter->irq_init(instance) != PS3_SUCCESS)
			goto l_failed;
	} else {
		if (ps3_irqs_init(instance) != PS3_SUCCESS)
			goto l_failed;
	}

	return PS3_SUCCESS;

l_failed:
	ps3_pci_init_complete_exit(instance);
	return -PS3_FAILED;
}

static void ps3_pci_init_complete_exit(struct ps3_instance *instance)
{
	ps3_irqs_exit(instance);
	ps3_irq_context_exit(instance);
}

#endif
