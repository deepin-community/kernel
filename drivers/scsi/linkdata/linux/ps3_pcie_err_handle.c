
#include <scsi/scsi_host.h>

#include "ps3_pcie_err_handle.h"
#include "ps3_driver_log.h"
#include "ps3_recovery.h"
#include "ps3_ioc_state.h"
#include "ps3_module_para.h"

static pci_ers_result_t ps3_pci_err_detected(struct pci_dev *pdev,
	pci_channel_state_t state);
static pci_ers_result_t ps3_pci_mmio_enabled(struct pci_dev *pdev);
static pci_ers_result_t ps3_pci_slot_reset(struct pci_dev *pdev);
static void ps3_pci_resume(struct pci_dev *pdev);

extern S32 ps3_pci_init(struct pci_dev *pdev, struct ps3_instance *instance);
extern S32 ps3_pci_init_complete(struct ps3_instance *instance);
extern void ps3_pci_init_complete_exit(struct ps3_instance *instance);

static struct pci_error_handlers ps3_err_handlers = {
	.error_detected	= ps3_pci_err_detected,
	.mmio_enabled	= ps3_pci_mmio_enabled,
	.slot_reset	= ps3_pci_slot_reset,
	.resume		= ps3_pci_resume
};

void ps3_pci_err_handler_init(struct pci_driver *drv)
{
	drv->err_handler = &ps3_err_handlers;
	return;
}

int ps3_base_init_resources(struct ps3_instance *instance)
{
	int ret = PS3_SUCCESS;
	struct pci_dev *pdev = instance->pdev;

	ret = ps3_pci_init(pdev, instance);
	INJECT_START(PS3_ERR_IJ_FORCE_PCIE_ERR_PCI_INIT_FAILED, &ret)
	if (ret) {
		LOG_ERROR("hno:%u pci init failed, ret: %d\n", PS3_HOST(instance), ret);
		goto l_out;
	}

	instance->is_pci_reset = PS3_FALSE;

	ret = ps3_pci_init_complete(instance);
	INJECT_START(PS3_ERR_IJ_FORCE_PCIE_ERR_PCI_INIT_COMP_FAILED, &ret)
	if (ret) {
		LOG_ERROR("hno:%u pci init complete failed, ret: %d\n", PS3_HOST(instance), ret);
		goto l_out;
	}

	pci_save_state(pdev);

l_out:

	return ret;
}

void ps3_base_free_resources(struct ps3_instance *instance)
{
	instance->ioc_adpter->irq_disable(instance);
	ps3_irqs_sync(instance);

	ps3_irqpolls_enable(instance);
	ps3_dump_work_stop(instance);
	ps3_pci_init_complete_exit(instance);
}

static pci_ers_result_t ps3_pci_err_detected(struct pci_dev *pdev, pci_channel_state_t state)
{
	pci_ers_result_t ret = PCI_ERS_RESULT_NEED_RESET;
	struct ps3_instance *instance = (struct ps3_instance *)pci_get_drvdata(pdev);
	if (instance == NULL) {
		LOG_INFO("get instance failed\n");
		dev_info(&pdev->dev, "[PS3]%s():%d; get instance failed\n", __FUNCTION__, __LINE__);
		ret = PCI_ERS_RESULT_DISCONNECT;
		goto l_out;
	}

	LOG_INFO("[%04x:%02x:%02x:%x]:PCIe err detected state:%u\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
			ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev), state);
	dev_info(&pdev->dev, "[PS3]%s():%d;PCIe err detected state:%u\n",
			__FUNCTION__, __LINE__, state);
	
	switch (state) {
	case pci_channel_io_normal:
		ret = PCI_ERS_RESULT_CAN_RECOVER;
		break;
	case pci_channel_io_frozen:
		ps3_pci_err_recovery_set(instance, PS3_TRUE);
		scsi_block_requests(instance->host);
		ps3_watchdog_stop(instance);
		if (ps3_recovery_cancel_work_sync(instance) != PS3_SUCCESS) {
			LOG_ERROR("hno:%u work sync failed, state: %u\n", PS3_HOST(instance), state);
		}
		instance->pci_err_handle_state = PS3_DEVICE_ERR_STATE_CLEAN;
		ps3_base_free_resources(instance);
		ret = PCI_ERS_RESULT_NEED_RESET;
		break;
	case pci_channel_io_perm_failure:
		ps3_pci_err_recovery_set(instance, PS3_TRUE);
		ps3_watchdog_stop(instance);
		if (ps3_recovery_cancel_work_sync(instance) != PS3_SUCCESS) {
			LOG_ERROR("hno:%u work sync failed, state: %u\n", PS3_HOST(instance), state);
		}
		ps3_scsi_cmd_force_stop(instance);
		ps3_mgr_cmd_force_stop(instance);
		ps3_pci_err_recovery_set(instance, PS3_FALSE);
		ps3_instance_state_transfer_to_dead(instance);
		ret = PCI_ERS_RESULT_DISCONNECT;
		break;
	default:
		ret = PCI_ERS_RESULT_RECOVERED;
		break;
	}

l_out:
	LOG_INFO("[%04x:%02x:%02x:%x]:PCIe err detect state:%u ret:%u\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
			ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev), state, ret);
	dev_info(&pdev->dev, "[PS3]%s():%d;PCIe err detect state:%u ret:%u\n",
			__FUNCTION__, __LINE__, state, ret);
	return ret;
}

static pci_ers_result_t ps3_pci_mmio_enabled(struct pci_dev *pdev)
{
	pci_ers_result_t ret = PCI_ERS_RESULT_RECOVERED;

	LOG_INFO("[%04x:%02x:%02x:%x]: PCIe err mmio enabled\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
			ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));
	dev_info(&pdev->dev, "[PS3]%s():%d; PCIe err mmio enabled\n",
			__FUNCTION__, __LINE__);

	return ret;
}

static pci_ers_result_t ps3_pci_slot_reset(struct pci_dev *pdev)
{
	int ret;
	struct ps3_instance *instance = (struct ps3_instance *)pci_get_drvdata(pdev);

	LOG_INFO("hno:%u PCIe err slot reset begin.\n", PS3_HOST(instance));
	dev_info(&pdev->dev, "[PS3]%s():%d;hno:%u PCIe err slot reset begin.\n",
			__FUNCTION__, __LINE__, PS3_HOST(instance));

	instance->pdev = pdev;
	pci_restore_state(pdev);

	ret = ps3_base_init_resources(instance);
	if (ret) {
		LOG_ERROR("hno:%u base init resources failed, ret: %d\n", PS3_HOST(instance), ret);
		dev_info(&pdev->dev, "[PS3]hno:%u init resources failed, ret: %d\n", PS3_HOST(instance), ret);
		goto l_out;
	}

	LOG_INFO("hno:%u PCIe err slot reset succeed.\n", PS3_HOST(instance));
	dev_info(&pdev->dev, "[PS3]%s():%d;hno:%u PCIe err slot reset succeed.\n",
			__FUNCTION__, __LINE__, PS3_HOST(instance));
	ps3_pci_err_recovery_set(instance, PS3_FALSE);
	instance->pci_err_handle_state = PS3_DEVICE_ERR_STATE_INIT;
	return PCI_ERS_RESULT_RECOVERED;
l_out:
	if (instance) {
		ps3_instance_state_transfer_to_dead(instance);
	}
	ps3_pci_err_recovery_set(instance, PS3_FALSE);
	instance->pci_err_handle_state = PS3_DEVICE_ERR_STATE_INIT;

	return PCI_ERS_RESULT_DISCONNECT;
}

static void ps3_pci_resume(struct pci_dev *pdev)
{
	S32 ret = PS3_SUCCESS;
	U32 fw_cur_state = PS3_FW_STATE_UNDEFINED;
	struct ps3_instance *instance = (struct ps3_instance *)pci_get_drvdata(pdev);

	LOG_INFO("hno:%u PCIe err resume\n", PS3_HOST(instance));
	dev_info(&pdev->dev, "[PS3]%s():%d;hno:%u PCIe err resume\n",
			__FUNCTION__, __LINE__, PS3_HOST(instance));

	fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
	INJECT_START(PS3_ERR_IJ_FORCE_PCIE_ERR_IOC_RUNNING_INIT_FAILED, &fw_cur_state)
	if (fw_cur_state == PS3_FW_STATE_RUNNING) {
		LOG_INFO("hno:%u not need repeat recovery\n", PS3_HOST(instance));
		dev_info(&pdev->dev, "[PS3]hno:%u not need repeat recovery\n", PS3_HOST(instance));
		goto l_norecovery;
	}
	INJECT_START(PS3_ERR_IJ_PCI_FORCE_HARD_RECOVERY_REQ_FAILED, instance)
	ret = ps3_hard_recovery_request_with_retry(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u hard reset NOK, ret: %d\n", PS3_HOST(instance), ret);
		dev_info(&pdev->dev, "[PS3]hno:%u hard reset NOK, ret: %d\n", PS3_HOST(instance), ret);
		goto l_failed;
	}
	ret = ps3_instance_wait_for_operational(instance, PS3_TRUE);
	INJECT_START(PS3_ERR_IJ_PCI_FORCE_WAIT_OPERARIONAL_FAILED, &ret)
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u wait for opt NOK.\n", PS3_HOST(instance));
		dev_info(&pdev->dev, "[PS3]hno:%u wait for opt NOK.\n", PS3_HOST(instance));
		goto l_failed;
	}

l_norecovery:
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)) || \
		(defined(RHEL_MAJOR) && (RHEL_MAJOR == 8) && (RHEL_MINOR >= 3)) \
		|| (defined(CONFIG_SUSE_KERNEL) && ((CONFIG_SUSE_VERSION == 15) \
		&& (CONFIG_SUSE_PATCHLEVEL >= 2))))
		pci_aer_clear_nonfatal_status(pdev);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
		pci_cleanup_aer_uncorrect_error_status(pdev);
#endif
	ps3_watchdog_start(instance);
	scsi_unblock_requests(instance->host);
	instance->pci_err_handle_state = PS3_DEVICE_ERR_STATE_NORMAL;
	return;
l_failed:
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)) || \
		(defined(RHEL_MAJOR) && (RHEL_MAJOR == 8) && (RHEL_MINOR >= 3)) \
		|| (defined(CONFIG_SUSE_KERNEL) && ((CONFIG_SUSE_VERSION == 15) \
		&& (CONFIG_SUSE_PATCHLEVEL >= 2))))
		pci_aer_clear_nonfatal_status(pdev);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
		pci_cleanup_aer_uncorrect_error_status(pdev);
#endif
	if (instance) {
		ps3_instance_state_transfer_to_dead(instance);
	}
	instance->pci_err_handle_state = PS3_DEVICE_ERR_STATE_NORMAL;
	return;
}

