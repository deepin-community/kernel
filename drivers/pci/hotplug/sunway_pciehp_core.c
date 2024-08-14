// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunway PCI Express Hot Plug Controller Driver
 */

#define pr_fmt(fmt) "sunway_pciehp: " fmt
#define dev_fmt pr_fmt

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/pci-ecam.h>
#include <linux/acpi.h>

#include "../pci.h"
#include "sunway_pciehp.h"

/* Global variables */
bool sunway_pciehp_poll_mode;
int  sunway_pciehp_poll_time;

#define DRIVER_VERSION  "0.1"
#define DRIVER_DESC     "Sunway PCI Express Hot Plug Controller Driver"

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

/*
 * not really modular, but the easiest way to keep compat with existing
 * bootargs behaviour is to continue using module_param here.
 */
module_param(sunway_pciehp_poll_mode, bool, 0644);
module_param(sunway_pciehp_poll_time, int, 0644);
MODULE_PARM_DESC(sunway_pciehp_poll_mode, "Using polling mechanism for hot-plug events or not");
MODULE_PARM_DESC(sunway_pciehp_poll_time, "Polling mechanism frequency, in seconds");

#define PCIE_MODULE_NAME "sunway_pciehp"

static int set_attention_status(struct hotplug_slot *slot, u8 value);
static int get_power_status(struct hotplug_slot *slot, u8 *value);
static int get_latch_status(struct hotplug_slot *slot, u8 *value);
static int get_adapter_status(struct hotplug_slot *slot, u8 *value);

static int init_slot(struct controller *ctrl)
{
	struct hotplug_slot_ops *ops;
	char name[SLOT_NAME_SIZE];
	int retval;

	/* Setup hotplug slot ops */
	ops = kzalloc(sizeof(*ops), GFP_KERNEL);
	if (!ops)
		return -ENOMEM;

	ops->enable_slot = sunway_pciehp_sysfs_enable_slot;
	ops->disable_slot = sunway_pciehp_sysfs_disable_slot;
	ops->get_power_status = get_power_status;
	ops->get_adapter_status = get_adapter_status;
	ops->reset_slot = sunway_pciehp_reset_slot;
	if (MRL_SENS(ctrl))
		ops->get_latch_status = get_latch_status;
	if (ATTN_LED(ctrl)) {
		ops->get_attention_status = sunway_pciehp_get_attention_status;
		ops->set_attention_status = set_attention_status;
	} else if (ctrl->pci_dev->hotplug_user_indicators) {
		ops->get_attention_status = sunway_pciehp_get_raw_indicator_status;
		ops->set_attention_status = sunway_pciehp_set_raw_indicator_status;
	}

	/* register this slot with the hotplug pci core */
	ctrl->hotplug_slot.ops = ops;
	snprintf(name, SLOT_NAME_SIZE, "%u", PSN(ctrl));

	retval = pci_hp_initialize(&ctrl->hotplug_slot,
				   ctrl->pci_dev->subordinate, 0, name);
	if (retval) {
		ctrl_err(ctrl, "pci_hp_initialize failed: error %d\n", retval);
		kfree(ops);
	}
	return retval;
}

static void cleanup_slot(struct controller *ctrl)
{
	struct hotplug_slot *hotplug_slot = &ctrl->hotplug_slot;

	pci_hp_destroy(hotplug_slot);
	kfree(hotplug_slot->ops);
}

/*
 * set_attention_status - Turns the Attention Indicator on, off or blinking
 */
static int set_attention_status(struct hotplug_slot *hotplug_slot, u8 status)
{
	struct controller *ctrl = to_ctrl(hotplug_slot);
	struct pci_dev *pdev = ctrl->pci_dev;

	if (status)
		status <<= PCI_EXP_SLTCTL_ATTN_IND_SHIFT;
	else
		status = PCI_EXP_SLTCTL_ATTN_IND_OFF;

	pci_config_pm_runtime_get(pdev);
	sunway_pciehp_set_indicators(ctrl, INDICATOR_NOOP, status);
	pci_config_pm_runtime_put(pdev);
	return 0;
}

static int get_power_status(struct hotplug_slot *hotplug_slot, u8 *value)
{
	struct controller *ctrl = to_ctrl(hotplug_slot);
	struct pci_dev *pdev = ctrl->pci_dev;

	pci_config_pm_runtime_get(pdev);
	sunway_pciehp_get_power_status(ctrl, value);
	pci_config_pm_runtime_put(pdev);
	return 0;
}

static int get_latch_status(struct hotplug_slot *hotplug_slot, u8 *value)
{
	struct controller *ctrl = to_ctrl(hotplug_slot);
	struct pci_dev *pdev = ctrl->pci_dev;

	pci_config_pm_runtime_get(pdev);
	sunway_pciehp_get_latch_status(ctrl, value);
	pci_config_pm_runtime_put(pdev);
	return 0;
}

static int get_adapter_status(struct hotplug_slot *hotplug_slot, u8 *value)
{
	struct controller *ctrl = to_ctrl(hotplug_slot);
	struct pci_dev *pdev = ctrl->pci_dev;
	int ret;

	pci_config_pm_runtime_get(pdev);
	ret = sunway_pciehp_card_present_or_link_active(ctrl);
	pci_config_pm_runtime_put(pdev);

	if (ret < 0)
		return ret;

	*value = ret;
	return 0;
}

/**
 * sunway_pciehp_check_presence() - synthesize event if presence has changed
 *
 * On probe and resume, an explicit presence check is necessary to bring up an
 * occupied slot or bring down an unoccupied slot.  This can't be triggered by
 * events in the Slot Status register, they may be stale and are therefore
 * cleared.  Secondly, sending an interrupt for "events that occur while
 * interrupt generation is disabled [when] interrupt generation is subsequently
 * enabled" is optional per PCIe r4.0, sec 6.7.3.4.
 */
static void sunway_pciehp_check_presence(struct controller *ctrl)
{
	int occupied;

	down_read_nested(&ctrl->reset_lock, ctrl->depth);
	mutex_lock(&ctrl->state_lock);

	occupied = sunway_pciehp_card_present_or_link_active(ctrl);
	if ((occupied > 0 && (ctrl->state == OFF_STATE ||
					ctrl->state == BLINKINGON_STATE)) ||
			(!occupied && (ctrl->state == ON_STATE ||
				       ctrl->state == BLINKINGOFF_STATE)))
		sunway_pciehp_request(ctrl, PCI_EXP_SLTSTA_PDC);

	mutex_unlock(&ctrl->state_lock);
	up_read(&ctrl->reset_lock);
}

static int sunwayhp_init(struct pci_dev *dev)
{
	int rc;
	struct controller *ctrl;

	if (!dev->subordinate) {
		/* Can happen if we run out of bus numbers during probe */
		dev_err(&dev->dev,
			"Hotplug bridge without secondary bus, ignoring\n");
		return -ENODEV;
	}

	ctrl = sunwayhpc_init(dev);
	if (!ctrl) {
		dev_err(&dev->dev, "Controller initialization failed\n");
		return -ENODEV;
	}
	pci_set_drvdata(dev, ctrl);

	/* Setup the slot information structures */
	rc = init_slot(ctrl);
	if (rc) {
		if (rc == -EBUSY)
			ctrl_warn(ctrl, "Slot already registered by another hotplug driver\n");
		else
			ctrl_err(ctrl, "Slot initialization failed (%d)\n", rc);
		goto err_out_release_ctlr;
	}

	/* Enable events after we have setup the data structures */
	rc = sunway_pcie_init_notification(ctrl);
	if (rc) {
		ctrl_err(ctrl, "Notification initialization failed (%d)\n", rc);
		goto err_out_free_ctrl_slot;
	}

	/* Publish to user space */
	rc = pci_hp_add(&ctrl->hotplug_slot);
	if (rc) {
		ctrl_err(ctrl, "Publication to user space failed (%d)\n", rc);
		goto err_out_shutdown_notification;
	}

	sunway_pciehp_check_presence(ctrl);

	return 0;

err_out_shutdown_notification:
	sunway_pcie_shutdown_notification(ctrl);
err_out_free_ctrl_slot:
	cleanup_slot(ctrl);
err_out_release_ctlr:
	sunway_pciehp_release_ctrl(ctrl);
	return -ENODEV;
}

static void sunwayhp_remove(struct pci_dev *dev)
{
	struct controller *ctrl = pci_get_drvdata(dev);

	pci_hp_del(&ctrl->hotplug_slot);
	sunway_pcie_shutdown_notification(ctrl);
	cleanup_slot(ctrl);
	sunway_pciehp_release_ctrl(ctrl);
}

static void sunway_hose_hotplug_init(void)
{
	int ret;
	struct pci_dev *pdev = NULL;
	struct pci_controller *hose;
	struct pci_config_window *cfg;
	struct device *dev;
	struct fwnode_handle *fwnode;
	u64 hotplug_enable;

	while ((pdev = pci_get_device(PCI_VENDOR_ID_JN,
				PCI_DEVICE_ID_SW64_ROOT_BRIDGE, pdev))) {
		hose = pci_bus_to_pci_controller(pdev->bus);

		/* disable by default */
		hose->hotplug_enable = false;

		if (sunway_legacy_pci)
			continue;

		cfg = (struct pci_config_window *)pdev->bus->sysdata;
		dev = cfg->parent;

		if (acpi_disabled)
			fwnode = dev->fwnode;
		else
			fwnode = acpi_fwnode_handle(to_acpi_device(dev));

		ret = fwnode_property_read_u64(fwnode,
			"sunway,hotplug-enable", &hotplug_enable);

		/* Fallback to legacy prop name */
		if (ret)
			ret = fwnode_property_read_u64(fwnode,
				"sw64,hot_plug_slot_enable", &hotplug_enable);

		if (!ret)
			hose->hotplug_enable = hotplug_enable;
	}
}

static int __init sunway_pciehp_init(void)
{
	int retval;
	struct pci_dev *pdev = NULL;
	struct pci_controller *hose;

	if (is_guest_or_emul()) {
		pr_info(DRIVER_DESC " does not support for VM and emulator.\n");
		return -ENODEV;
	}

	pr_info(DRIVER_DESC " version: " DRIVER_VERSION "\n");

	sunway_hose_hotplug_init();

	while ((pdev = pci_get_device(PCI_VENDOR_ID_JN, PCI_DEVICE_ID_SW64_ROOT_BRIDGE, pdev))) {
		hose = pci_bus_to_pci_controller(pdev->bus);

		if (!hose->hotplug_enable)
			continue;

		retval = sunwayhp_init(pdev);
	}

	return retval;
}

static void __exit sunway_pciehp_exit(void)
{
	struct pci_dev *pdev = NULL;
	struct pci_controller *hose;

	pr_info(DRIVER_DESC " version: " DRIVER_VERSION " unloaded\n");

	while ((pdev = pci_get_device(PCI_VENDOR_ID_JN, PCI_DEVICE_ID_SW64_ROOT_BRIDGE, pdev))) {
		hose = pci_bus_to_pci_controller(pdev->bus);

		if (!hose->hotplug_enable)
			continue;

		sunwayhp_remove(pdev);
	}

}

module_init(sunway_pciehp_init);
module_exit(sunway_pciehp_exit);
