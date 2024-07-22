// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunway PCI Express PCI Hot Plug Driver
 */

#define dev_fmt(fmt) "sunway_pciehp: " fmt

#include <linux/dmi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/signal.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/pci.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "../pci.h"
#include "sunway_pciehp.h"

#include <asm/sw64io.h>
#include <asm/sw64_init.h>

static const struct dmi_system_id inband_presence_disabled_dmi_table[] = {
	/*
	 * Match all Dell systems, as some Dell systems have inband
	 * presence disabled on NVMe slots (but don't support the bit to
	 * report it). Setting inband presence disabled should have no
	 * negative effect, except on broken hotplug slots that never
	 * assert presence detect--and those will still work, they will
	 * just have a bit of extra delay before being probed.
	 */
	{
		.ident = "Dell System",
		.matches = {
			DMI_MATCH(DMI_OEM_STRING, "Dell System"),
		},
	},
	{}
};

static inline struct pci_dev *ctrl_dev(struct controller *ctrl)
{
	return ctrl->pci_dev;
}

static irqreturn_t sunway_pciehp_isr(int irq, void *dev_id);
static irqreturn_t sunway_pciehp_ist(int irq, void *dev_id);
static int sunway_pciehp_poll_start(void *data);

static inline int sunway_pciehp_request_irq(struct controller *ctrl)
{
	int retval, irq = ctrl->pci_dev->irq;

	if (sunway_pciehp_poll_mode) {
		ctrl->poll_thread = kthread_run(&sunway_pciehp_poll_start, ctrl,
						"sunway_pciehp_poll_start-%s",
						slot_name(ctrl));
		return PTR_ERR_OR_ZERO(ctrl->poll_thread);
	}

	/* Installs the interrupt handler */
	retval = request_threaded_irq(irq, sunway_pciehp_isr, sunway_pciehp_ist,
				      IRQF_SHARED, MY_NAME, ctrl);
	if (retval)
		ctrl_err(ctrl, "Cannot get irq %d for the hotplug controller\n",
			 irq);
	return retval;
}

static inline void sunway_pciehp_free_irq(struct controller *ctrl)
{
	if (sunway_pciehp_poll_mode)
		kthread_stop(ctrl->poll_thread);
	else
		free_irq(ctrl->pci_dev->irq, ctrl);
}

static int pcie_poll_cmd(struct controller *ctrl, int timeout)
{
	struct pci_dev *pdev = ctrl_dev(ctrl);
	u16 slot_status;

	do {
		pcie_capability_read_word(pdev, PCI_EXP_SLTSTA, &slot_status);
		if (PCI_POSSIBLE_ERROR(slot_status)) {
			ctrl_info(ctrl, "%s: no response from device\n",
				  __func__);
			return 0;
		}

		if (slot_status & PCI_EXP_SLTSTA_CC) {
			pcie_capability_write_word(pdev, PCI_EXP_SLTSTA,
						   PCI_EXP_SLTSTA_CC);
			ctrl->cmd_busy = 0;
			smp_mb();
			return 1;
		}
		msleep(10);
		timeout -= 10;
	} while (timeout >= 0);
	return 0;	/* timeout */
}

static void pcie_wait_cmd(struct controller *ctrl)
{
	unsigned int msecs = sunway_pciehp_poll_mode ? 2500 : 1000;
	unsigned long duration = msecs_to_jiffies(msecs);
	unsigned long cmd_timeout = ctrl->cmd_started + duration;
	unsigned long now, timeout;
	int rc;

	/*
	 * If the controller does not generate notifications for command
	 * completions, we never need to wait between writes.
	 */
	if (NO_CMD_CMPL(ctrl))
		return;

	if (!ctrl->cmd_busy)
		return;

	/*
	 * Even if the command has already timed out, we want to call
	 * pcie_poll_cmd() so it can clear PCI_EXP_SLTSTA_CC.
	 */
	now = jiffies;
	if (time_before_eq(cmd_timeout, now))
		timeout = 1;
	else
		timeout = cmd_timeout - now;

	if (ctrl->slot_ctrl & PCI_EXP_SLTCTL_HPIE &&
	    ctrl->slot_ctrl & PCI_EXP_SLTCTL_CCIE)
		rc = wait_event_timeout(ctrl->queue, !ctrl->cmd_busy, timeout);
	else
		rc = pcie_poll_cmd(ctrl, jiffies_to_msecs(timeout));

	if (!rc)
		ctrl_info(ctrl, "Timeout on hotplug command %#06x (issued %u msec ago)\n",
			  ctrl->slot_ctrl,
			  jiffies_to_msecs(jiffies - ctrl->cmd_started));
}

#define CC_ERRATUM_MASK		(PCI_EXP_SLTCTL_PCC |	\
				 PCI_EXP_SLTCTL_PIC |	\
				 PCI_EXP_SLTCTL_AIC |	\
				 PCI_EXP_SLTCTL_EIC)

static void pcie_do_write_cmd(struct controller *ctrl, u16 cmd,
			      u16 mask, bool wait)
{
	struct pci_dev *pdev = ctrl_dev(ctrl);
	u16 slot_ctrl_orig, slot_ctrl;

	mutex_lock(&ctrl->ctrl_lock);

	/*
	 * Always wait for any previous command that might still be in progress
	 */
	pcie_wait_cmd(ctrl);

	pcie_capability_read_word(pdev, PCI_EXP_SLTCTL, &slot_ctrl);
	if (PCI_POSSIBLE_ERROR(slot_ctrl)) {
		ctrl_info(ctrl, "%s: no response from device\n", __func__);
		goto out;
	}

	slot_ctrl_orig = slot_ctrl;
	slot_ctrl &= ~mask;
	slot_ctrl |= (cmd & mask);
	ctrl->cmd_busy = 1;
	smp_mb();
	ctrl->slot_ctrl = slot_ctrl;
	pcie_capability_write_word(pdev, PCI_EXP_SLTCTL, slot_ctrl);
	ctrl->cmd_started = jiffies;

	/*
	 * Controllers with the Intel CF118 and similar errata advertise
	 * Command Completed support, but they only set Command Completed
	 * if we change the "Control" bits for power, power indicator,
	 * attention indicator, or interlock.  If we only change the
	 * "Enable" bits, they never set the Command Completed bit.
	 */
	if ((slot_ctrl_orig & CC_ERRATUM_MASK) == (slot_ctrl & CC_ERRATUM_MASK))
		ctrl->cmd_busy = 0;

	/*
	 * Optionally wait for the hardware to be ready for a new command,
	 * indicating completion of the above issued command.
	 */
	if (wait)
		pcie_wait_cmd(ctrl);

out:
	mutex_unlock(&ctrl->ctrl_lock);
}

/**
 * pcie_write_cmd - Issue controller command
 * @ctrl: controller to which the command is issued
 * @cmd:  command value written to slot control register
 * @mask: bitmask of slot control register to be modified
 */
static void pcie_write_cmd(struct controller *ctrl, u16 cmd, u16 mask)
{
	pcie_do_write_cmd(ctrl, cmd, mask, true);
}

/* Same as above without waiting for the hardware to latch */
static void pcie_write_cmd_nowait(struct controller *ctrl, u16 cmd, u16 mask)
{
	pcie_do_write_cmd(ctrl, cmd, mask, false);
}

/**
 * sunway_pciehp_check_link_active() - Is the link active
 * @ctrl: PCIe hotplug controller
 *
 * Check whether the downstream link is currently active. Note it is
 * possible that the card is removed immediately after this so the
 * caller may need to take it into account.
 *
 * If the hotplug controller itself is not available anymore returns
 * %-ENODEV.
 */
int sunway_pciehp_check_link_active(struct controller *ctrl)
{
	struct pci_dev *pdev = ctrl_dev(ctrl);
	u16 lnk_status;
	int ret;

	ret = pcie_capability_read_word(pdev, PCI_EXP_LNKSTA, &lnk_status);
	if (ret == PCIBIOS_DEVICE_NOT_FOUND || PCI_POSSIBLE_ERROR(lnk_status))
		return -ENODEV;

	ret = !!(lnk_status & PCI_EXP_LNKSTA_DLLLA);
	ctrl_dbg(ctrl, "%s: lnk_status = %x\n", __func__, lnk_status);

	return ret;
}

static bool pcie_wait_link_active(struct controller *ctrl)
{
	struct pci_dev *pdev = ctrl_dev(ctrl);
	struct pci_bus *bus = pdev->bus;
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);

	if (pcie_wait_for_link(pdev, true)) {
		pci_mark_rc_linkup(hose);
		sunway_pciehp_restore_rc_piu(ctrl);
		return true;
	}

	return false;
}

static bool pci_bus_check_dev(struct pci_bus *bus, int devfn)
{
	u32 l;
	int count = 0;
	int delay = 1000, step = 20;
	bool found = false;

	do {
		found = pci_bus_read_dev_vendor_id(bus, devfn, &l, 0);
		count++;

		if (found)
			break;

		msleep(step);
		delay -= step;
	} while (delay > 0);

	if (count > 1)
		pr_debug("pci %04x:%02x:%02x.%d id reading try %d times with interval %d ms to get %08x\n",
			pci_domain_nr(bus), bus->number, PCI_SLOT(devfn),
			PCI_FUNC(devfn), count, step, l);

	return found;
}

static void pcie_wait_for_presence(struct pci_dev *pdev)
{
	int timeout = 1250;
	u16 slot_status;

	do {
		pcie_capability_read_word(pdev, PCI_EXP_SLTSTA, &slot_status);
		if (slot_status & PCI_EXP_SLTSTA_PDS)
			return;
		msleep(10);
		timeout -= 10;
	} while (timeout > 0);
}

static bool pci_bus_check_linkup(struct pci_controller *hose)
{
	int count = 0;
	int delay = 1000, step = 20;
	bool linkup = false;

	do {
		if (sw64_chip_init->pci_init.check_pci_linkup(hose) == 0) {
			udelay(10);
			if (sw64_chip_init->pci_init.check_pci_linkup(hose) == 0)
				linkup = true;
		}
		count++;

		if (linkup)
			break;

		msleep(step);
		delay -= step;
	} while (delay > 0);

	if (count > 1)
		pr_debug("Node %ld RC %ld linkup reading try %d times with interval %d ms\n",
				hose->node, hose->index, count, step);

	return linkup;
}

int sunway_pciehp_check_link_status(struct controller *ctrl)
{
	struct pci_dev *pdev = ctrl_dev(ctrl);
	struct pci_bus *bus = pdev->bus;
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);
	bool found, linkup;
	u16 lnk_status;

	if (!pcie_wait_link_active(ctrl)) {
		ctrl_info(ctrl, "Slot(%s): No link\n", slot_name(ctrl));
		return -1;
	}

	if (ctrl->inband_presence_disabled)
		pcie_wait_for_presence(pdev);

	/* wait 1000ms and check RC linkup before read pci conf */
	msleep(1000);
	linkup = pci_bus_check_linkup(hose);

	if (!linkup)
		return -1;

	/* try read device id in 1s */
	found = pci_bus_check_dev(ctrl->pci_dev->subordinate,
					PCI_DEVFN(0, 0));

	/* ignore link or presence changes up to this point */
	if (found)
		atomic_and(~(PCI_EXP_SLTSTA_DLLSC | PCI_EXP_SLTSTA_PDC),
			   &ctrl->pending_events);

	pcie_capability_read_word(pdev, PCI_EXP_LNKSTA, &lnk_status);
	ctrl_dbg(ctrl, "%s: lnk_status = %x\n", __func__, lnk_status);
	if ((lnk_status & PCI_EXP_LNKSTA_LT) ||
	    !(lnk_status & PCI_EXP_LNKSTA_NLW)) {
		ctrl_info(ctrl, "Slot(%s): Cannot train link: status %#06x\n",
			 slot_name(ctrl), lnk_status);
		return -1;
	}

	pcie_update_link_speed(ctrl->pci_dev->subordinate, lnk_status);

	if (!found) {
		ctrl_info(ctrl, "Slot(%s): No device found\n",
				slot_name(ctrl));
		return -1;
	}

	return 0;
}

static int __sunway_pciehp_link_set(struct controller *ctrl, bool enable)
{
	struct pci_dev *pdev = ctrl_dev(ctrl);

	pcie_capability_clear_and_set_word(pdev, PCI_EXP_LNKCTL,
			PCI_EXP_LNKCTL_LD,
			enable ? 0 : PCI_EXP_LNKCTL_LD);

	return 0;
}

int sunway_pciehp_link_enable(struct controller *ctrl)
{
	return __sunway_pciehp_link_set(ctrl, true);
}

int sunway_pciehp_link_disable(struct controller *ctrl)
{
	struct pci_dev *pdev = ctrl_dev(ctrl);
	struct pci_bus *bus = pdev->bus;
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);
	int ret;

	ret =  __sunway_pciehp_link_set(ctrl, false);
	pci_clear_rc_linkup(hose);

	return ret;
}

int sunway_pciehp_get_raw_indicator_status(struct hotplug_slot *hotplug_slot,
				    u8 *status)
{
	struct controller *ctrl = to_ctrl(hotplug_slot);
	struct pci_dev *pdev = ctrl_dev(ctrl);
	u16 slot_ctrl;

	pci_config_pm_runtime_get(pdev);
	pcie_capability_read_word(pdev, PCI_EXP_SLTCTL, &slot_ctrl);
	pci_config_pm_runtime_put(pdev);
	*status = (slot_ctrl & (PCI_EXP_SLTCTL_AIC | PCI_EXP_SLTCTL_PIC)) >> 6;
	return 0;
}

int sunway_pciehp_get_attention_status(struct hotplug_slot *hotplug_slot, u8 *status)
{
	struct controller *ctrl = to_ctrl(hotplug_slot);
	struct pci_dev *pdev = ctrl_dev(ctrl);
	u16 slot_ctrl;

	pci_config_pm_runtime_get(pdev);
	pcie_capability_read_word(pdev, PCI_EXP_SLTCTL, &slot_ctrl);
	pci_config_pm_runtime_put(pdev);
	ctrl_dbg(ctrl, "%s: SLOTCTRL %x, value read %x\n", __func__,
		 pci_pcie_cap(ctrl->pci_dev) + PCI_EXP_SLTCTL, slot_ctrl);

	switch (slot_ctrl & PCI_EXP_SLTCTL_AIC) {
	case PCI_EXP_SLTCTL_ATTN_IND_ON:
		*status = 1;	/* On */
		break;
	case PCI_EXP_SLTCTL_ATTN_IND_BLINK:
		*status = 2;	/* Blink */
		break;
	case PCI_EXP_SLTCTL_ATTN_IND_OFF:
		*status = 0;	/* Off */
		break;
	default:
		*status = 0xFF;
		break;
	}

	return 0;
}

void sunway_pciehp_get_power_status(struct controller *ctrl, u8 *status)
{
	struct pci_dev *pdev = ctrl_dev(ctrl);
	u16 slot_ctrl;

	pcie_capability_read_word(pdev, PCI_EXP_SLTCTL, &slot_ctrl);
	ctrl_dbg(ctrl, "%s: SLOTCTRL %x value read %x\n", __func__,
		 pci_pcie_cap(ctrl->pci_dev) + PCI_EXP_SLTCTL, slot_ctrl);

	switch (slot_ctrl & PCI_EXP_SLTCTL_PCC) {
	case PCI_EXP_SLTCTL_PWR_ON:
		*status = 1;	/* On */
		break;
	case PCI_EXP_SLTCTL_PWR_OFF:
		*status = 0;	/* Off */
		break;
	default:
		*status = 0xFF;
		break;
	}
}

void sunway_pciehp_get_latch_status(struct controller *ctrl, u8 *status)
{
	struct pci_dev *pdev = ctrl_dev(ctrl);
	u16 slot_status;

	pcie_capability_read_word(pdev, PCI_EXP_SLTSTA, &slot_status);
	*status = !!(slot_status & PCI_EXP_SLTSTA_MRLSS);
}

/**
 * sunway_pciehp_card_present() - Is the card present
 * @ctrl: PCIe hotplug controller
 *
 * Function checks whether the card is currently present in the slot and
 * in that case returns true. Note it is possible that the card is
 * removed immediately after the check so the caller may need to take
 * this into account.
 *
 * It the hotplug controller itself is not available anymore returns
 * %-ENODEV.
 */
int sunway_pciehp_card_present(struct controller *ctrl)
{
	struct pci_dev *pdev = ctrl_dev(ctrl);
	u16 slot_status;
	int ret;

	ret = pcie_capability_read_word(pdev, PCI_EXP_SLTSTA, &slot_status);
	if (ret == PCIBIOS_DEVICE_NOT_FOUND || PCI_POSSIBLE_ERROR(slot_status))
		return -ENODEV;

	return !!(slot_status & PCI_EXP_SLTSTA_PDS);
}

/**
 * sunway_pciehp_card_present_or_link_active() - whether given slot is occupied
 * @ctrl: PCIe hotplug controller
 *
 * Unlike pciehp_card_present(), which determines presence solely from the
 * Presence Detect State bit, this helper also returns true if the Link Active
 * bit is set.	This is a concession to broken hotplug ports which hardwire
 * Presence Detect State to zero, such as Wilocity's [1ae9:0200].
 *
 * Returns: %1 if the slot is occupied and %0 if it is not. If the hotplug
 *	    port is not present anymore returns %-ENODEV.
 */
int sunway_pciehp_card_present_or_link_active(struct controller *ctrl)
{
	int ret;

	ret = sunway_pciehp_card_present(ctrl);
	if (ret)
		return ret;

	return sunway_pciehp_check_link_active(ctrl);
}

int sunway_pciehp_query_power_fault(struct controller *ctrl)
{
	struct pci_dev *pdev = ctrl_dev(ctrl);
	u16 slot_status;

	pcie_capability_read_word(pdev, PCI_EXP_SLTSTA, &slot_status);
	return !!(slot_status & PCI_EXP_SLTSTA_PFD);
}

int sunway_pciehp_set_raw_indicator_status(struct hotplug_slot *hotplug_slot,
				    u8 status)
{
	struct controller *ctrl = to_ctrl(hotplug_slot);
	struct pci_dev *pdev = ctrl_dev(ctrl);

	pci_config_pm_runtime_get(pdev);
	pcie_write_cmd_nowait(ctrl, status << 6,
			      PCI_EXP_SLTCTL_AIC | PCI_EXP_SLTCTL_PIC);
	pci_config_pm_runtime_put(pdev);
	return 0;
}

/**
 * sunway_pciehp_set_indicators() - set attention indicator, power indicator, or both
 * @ctrl: PCIe hotplug controller
 * @pwr: one of:
 *	PCI_EXP_SLTCTL_PWR_IND_ON
 *	PCI_EXP_SLTCTL_PWR_IND_BLINK
 *	PCI_EXP_SLTCTL_PWR_IND_OFF
 * @attn: one of:
 *	PCI_EXP_SLTCTL_ATTN_IND_ON
 *	PCI_EXP_SLTCTL_ATTN_IND_BLINK
 *	PCI_EXP_SLTCTL_ATTN_IND_OFF
 *
 * Either @pwr or @attn can also be INDICATOR_NOOP to leave that indicator
 * unchanged.
 */
void sunway_pciehp_set_indicators(struct controller *ctrl, int pwr, int attn)
{
	u16 cmd = 0, mask = 0;

	if (PWR_LED(ctrl) && pwr != INDICATOR_NOOP) {
		cmd |= (pwr & PCI_EXP_SLTCTL_PIC);
		mask |= PCI_EXP_SLTCTL_PIC;
	}

	if (ATTN_LED(ctrl) && attn != INDICATOR_NOOP) {
		cmd |= (attn & PCI_EXP_SLTCTL_AIC);
		mask |= PCI_EXP_SLTCTL_AIC;
	}

	if (cmd) {
		pcie_write_cmd_nowait(ctrl, cmd, mask);
		ctrl_dbg(ctrl, "%s: SLOTCTRL %x write cmd %x\n", __func__,
			 pci_pcie_cap(ctrl->pci_dev) + PCI_EXP_SLTCTL, cmd);
	}
}

int sunway_pciehp_power_on_slot(struct controller *ctrl)
{
	struct pci_dev *pdev = ctrl_dev(ctrl);
	u16 slot_status;
	int retval;

	/* Clear power-fault bit from previous power failures */
	pcie_capability_read_word(pdev, PCI_EXP_SLTSTA, &slot_status);
	if (slot_status & PCI_EXP_SLTSTA_PFD)
		pcie_capability_write_word(pdev, PCI_EXP_SLTSTA,
					   PCI_EXP_SLTSTA_PFD);
	ctrl->power_fault_detected = 0;

	pcie_write_cmd(ctrl, PCI_EXP_SLTCTL_PWR_ON, PCI_EXP_SLTCTL_PCC);
	ctrl_dbg(ctrl, "%s: SLOTCTRL %x write cmd %x\n", __func__,
		 pci_pcie_cap(ctrl->pci_dev) + PCI_EXP_SLTCTL,
		 PCI_EXP_SLTCTL_PWR_ON);

	retval = sunway_pciehp_link_enable(ctrl);
	if (retval)
		ctrl_err(ctrl, "%s: Can not enable the link!\n", __func__);

	return retval;
}

void sunway_pciehp_power_off_slot(struct controller *ctrl)
{
	pcie_write_cmd(ctrl, PCI_EXP_SLTCTL_PWR_OFF, PCI_EXP_SLTCTL_PCC);
	ctrl_dbg(ctrl, "%s: SLOTCTRL %x write cmd %x\n", __func__,
		 pci_pcie_cap(ctrl->pci_dev) + PCI_EXP_SLTCTL,
		 PCI_EXP_SLTCTL_PWR_OFF);
}

void sunway_pciehp_poll(struct controller *ctrl, u32 events)
{
	struct pci_dev *pdev = ctrl_dev(ctrl);
	struct pci_bus *bus = pdev->bus;
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);
	unsigned long piu_value;
	u16 slot_ctrl;
	int i;
	void __iomem *piu_ior0_base;
	void __iomem *piu_ior1_base;

	piu_ior0_base = hose->piu_ior0_base;
	piu_ior1_base = hose->piu_ior1_base;

	if (events & SW64_POLL_DISABLE_SLOT) {
		while (1) {
			piu_value = readq(piu_ior1_base + NEWLTSSMSTATE0);
			piu_value &= 0xff;

			if (piu_value == 0x19)
				break;

			udelay(10);
		}

		writeq(HP_CTRL_REMOVE, (piu_ior0_base + HP_CTRL));

		while (1) {
			piu_value = readq(piu_ior0_base + HP_WATCHOUT);
			piu_value >>= 24;

			if (piu_value == 0x19)
				break;

			udelay(10);
		}

		sunway_pciehp_link_enable(ctrl);

		mdelay(100);
		while (1) {
			piu_value = readq(piu_ior1_base + NEWLTSSMSTATE0);
			piu_value &= 0xff;

			if (piu_value == 0x0)
				break;

			udelay(10);
		}

		pcie_capability_read_word(pdev, PCI_EXP_SLTCTL, &slot_ctrl);
		slot_ctrl &= ~(PCI_EXP_SLTCTL_ABPE |
				PCI_EXP_SLTCTL_PFDE |
				PCI_EXP_SLTCTL_MRLSCE |
				PCI_EXP_SLTCTL_PDCE |
				PCI_EXP_SLTCTL_CCIE |
				PCI_EXP_SLTCTL_HPIE);
		slot_ctrl |= (PCI_EXP_SLTCTL_ATTN_IND_OFF |
				PCI_EXP_SLTCTL_PWR_IND_OFF |
				PCI_EXP_SLTCTL_DLLSCE);
		pcie_capability_write_word(pdev, PCI_EXP_SLTCTL, slot_ctrl);

		writeq(HP_CTRL_FINISH, (piu_ior0_base + HP_CTRL));

		ctrl->state = OFF_STATE;
	}

	if (events & SW64_POLL_ENABLE_SLOT) {
		writeq(HP_CTRL_INSERT, (piu_ior0_base + HP_CTRL));

		for (i = 0; i < 30; i++) {
			if (pcie_wait_for_link(pdev, true)) {
				pci_mark_rc_linkup(hose);
				sunway_pciehp_restore_rc_piu(ctrl);

				writeq(HP_CTRL_FINISH, (piu_ior0_base + HP_CTRL));

				pcie_capability_read_word(pdev, PCI_EXP_SLTCTL, &slot_ctrl);
				slot_ctrl &= ~PCI_EXP_SLTCTL_PWR_OFF;
				pcie_capability_write_word(pdev, PCI_EXP_SLTCTL, slot_ctrl);

				ctrl->state = ON_STATE;
				return;
			}

			if (sunway_pciehp_query_power_fault(ctrl)) {
				pcie_capability_write_word(pdev, PCI_EXP_SLTSTA,
						PCI_EXP_SLTSTA_PFD);
				udelay(10);
				if (sunway_pciehp_query_power_fault(ctrl)) {
					ctrl_err(ctrl, "power fault\n");
					return;
				}
			}
		}

		ctrl_err(ctrl, "insert failed\n");
	}
}

static irqreturn_t sunway_pciehp_isr(int irq, void *dev_id)
{
	struct controller *ctrl = (struct controller *)dev_id;
	struct pci_dev *pdev = ctrl_dev(ctrl);
	struct device *parent = pdev->dev.parent;
	u16 status, events = 0;

	/*
	 * Interrupts only occur in D3hot or shallower and only if enabled
	 * in the Slot Control register (PCIe r4.0, sec 6.7.3.4).
	 */
	if (pdev->current_state == PCI_D3cold ||
	    (!(ctrl->slot_ctrl & PCI_EXP_SLTCTL_HPIE) && !sunway_pciehp_poll_mode))
		return IRQ_NONE;

	/*
	 * Keep the port accessible by holding a runtime PM ref on its parent.
	 * Defer resume of the parent to the IRQ thread if it's suspended.
	 * Mask the interrupt until then.
	 */
	if (parent) {
		pm_runtime_get_noresume(parent);
		if (!pm_runtime_active(parent)) {
			pm_runtime_put(parent);
			disable_irq_nosync(irq);
			atomic_or(RERUN_ISR, &ctrl->pending_events);
			return IRQ_WAKE_THREAD;
		}
	}

	if (sunway_pciehp_poll_mode) {
		if (atomic_read(&ctrl->pending_events) & SW64_POLL_DISABLE_SLOT) {
			sunway_pciehp_link_disable(ctrl);
			goto out;
		} else if (atomic_read(&ctrl->pending_events) & SW64_POLL_ENABLE_SLOT)
			goto out;

		return IRQ_NONE;
	}

read_status:
	pcie_capability_read_word(pdev, PCI_EXP_SLTSTA, &status);
	if (PCI_POSSIBLE_ERROR(status)) {
		ctrl_info(ctrl, "%s: no response from device\n", __func__);
		if (parent)
			pm_runtime_put(parent);
		return IRQ_NONE;
	}

	/*
	 * Slot Status contains plain status bits as well as event
	 * notification bits; right now we only want the event bits.
	 */
	status &= PCI_EXP_SLTSTA_ABP | PCI_EXP_SLTSTA_PFD |
		  PCI_EXP_SLTSTA_PDC | PCI_EXP_SLTSTA_CC |
		  PCI_EXP_SLTSTA_DLLSC;

	/*
	 * If we've already reported a power fault, don't report it again
	 * until we've done something to handle it.
	 */
	if (ctrl->power_fault_detected)
		status &= ~PCI_EXP_SLTSTA_PFD;
	else if (status & PCI_EXP_SLTSTA_PFD)
		ctrl->power_fault_detected = true;

	events |= status;
	if (!events) {
		if (parent)
			pm_runtime_put(parent);
		return IRQ_NONE;
	}

	if (status) {
		pcie_capability_write_word(pdev, PCI_EXP_SLTSTA, status);

		/*
		 * In MSI mode, all event bits must be zero before the port
		 * will send a new interrupt (PCIe Base Spec r5.0 sec 6.7.3.4).
		 * So re-read the Slot Status register in case a bit was set
		 * between read and write.
		 */
		if (pci_dev_msi_enabled(pdev) && !sunway_pciehp_poll_mode)
			goto read_status;
	}

	ctrl_dbg(ctrl, "pending interrupts %#06x from Slot Status\n", events);
	if (parent)
		pm_runtime_put(parent);

	/*
	 * Command Completed notifications are not deferred to the
	 * IRQ thread because it may be waiting for their arrival.
	 */
	if (events & PCI_EXP_SLTSTA_CC) {
		ctrl->cmd_busy = 0;
		smp_mb();
		wake_up(&ctrl->queue);

		if (events == PCI_EXP_SLTSTA_CC)
			return IRQ_HANDLED;

		events &= ~PCI_EXP_SLTSTA_CC;
	}

out:
	if (pdev->ignore_hotplug) {
		ctrl_dbg(ctrl, "ignoring hotplug event %#06x\n", events);
		return IRQ_HANDLED;
	}

	/* Save pending events for consumption by IRQ thread. */
	atomic_or(events, &ctrl->pending_events);
	return IRQ_WAKE_THREAD;
}

static irqreturn_t sunway_pciehp_ist(int irq, void *dev_id)
{
	struct controller *ctrl = (struct controller *)dev_id;
	struct pci_dev *pdev = ctrl_dev(ctrl);
	irqreturn_t ret;
	u32 events;

	ctrl->ist_running = true;
	pci_config_pm_runtime_get(pdev);

	/* rerun sunway_pciehp_isr() if the port was inaccessible on interrupt */
	if (atomic_fetch_and(~RERUN_ISR, &ctrl->pending_events) & RERUN_ISR) {
		ret = sunway_pciehp_isr(irq, dev_id);
		enable_irq(irq);
		if (ret != IRQ_WAKE_THREAD)
			goto out;
	}

	synchronize_hardirq(irq);
	events = atomic_xchg(&ctrl->pending_events, 0);
	if (!events) {
		ret = IRQ_NONE;
		goto out;
	}

	if (sunway_pciehp_poll_mode) {
		sunway_pciehp_poll(ctrl, events);
		ret = IRQ_HANDLED;
		goto out;
	}

	/* Check Attention Button Pressed */
	if (events & PCI_EXP_SLTSTA_ABP)
		sunway_pciehp_handle_button_press(ctrl);

	/* Check Power Fault Detected */
	if (events & PCI_EXP_SLTSTA_PFD) {
		ctrl_err(ctrl, "Slot(%s): Power fault\n", slot_name(ctrl));
		sunway_pciehp_set_indicators(ctrl, PCI_EXP_SLTCTL_PWR_IND_OFF,
				PCI_EXP_SLTCTL_ATTN_IND_ON);
	}

	/*
	 * Disable requests have higher priority than Presence Detect Changed
	 * or Data Link Layer State Changed events.
	 */
	down_read_nested(&ctrl->reset_lock, ctrl->depth);
	if (events & DISABLE_SLOT)
		sunway_pciehp_handle_disable_request(ctrl);
	else if (events & (PCI_EXP_SLTSTA_PDC | PCI_EXP_SLTSTA_DLLSC))
		sunway_pciehp_handle_presence_or_link_change(ctrl, events);
	up_read(&ctrl->reset_lock);

	ret = IRQ_HANDLED;
out:
	pci_config_pm_runtime_put(pdev);
	ctrl->ist_running = false;
	wake_up(&ctrl->requester);
	return ret;
}

static int sunway_pciehp_poll_start(void *data)
{
	struct controller *ctrl = data;

	schedule_timeout_idle(10 * HZ); /* start with 10 sec delay */

	while (!kthread_should_stop()) {
		/* poll for interrupt events or user requests */
		while (sunway_pciehp_isr(IRQ_NOTCONNECTED, ctrl) == IRQ_WAKE_THREAD ||
		       atomic_read(&ctrl->pending_events))
			sunway_pciehp_ist(IRQ_NOTCONNECTED, ctrl);

		if (sunway_pciehp_poll_time <= 0 || sunway_pciehp_poll_time > 60)
			sunway_pciehp_poll_time = 2; /* clamp to sane value */

		schedule_timeout_idle(sunway_pciehp_poll_time * HZ);
	}

	return 0;
}

static void pcie_enable_notification(struct controller *ctrl)
{
	u16 cmd, mask;

	/*
	 * TBD: Power fault detected software notification support.
	 *
	 * Power fault detected software notification is not enabled
	 * now, because it caused power fault detected interrupt storm
	 * on some machines. On those machines, power fault detected
	 * bit in the slot status register was set again immediately
	 * when it is cleared in the interrupt service routine, and
	 * next power fault detected interrupt was notified again.
	 */

	/*
	 * Always enable link events: thus link-up and link-down shall
	 * always be treated as hotplug and unplug respectively. Enable
	 * presence detect only if Attention Button is not present.
	 */
	cmd = PCI_EXP_SLTCTL_DLLSCE;
	if (ATTN_BUTTN(ctrl))
		cmd |= PCI_EXP_SLTCTL_ABPE;
	else
		cmd |= PCI_EXP_SLTCTL_PDCE;
	if (!sunway_pciehp_poll_mode)
		cmd |= PCI_EXP_SLTCTL_HPIE;
	if (!sunway_pciehp_poll_mode && !NO_CMD_CMPL(ctrl))
		cmd |= PCI_EXP_SLTCTL_CCIE;

	mask = (PCI_EXP_SLTCTL_PDCE | PCI_EXP_SLTCTL_ABPE |
		PCI_EXP_SLTCTL_PFDE |
		PCI_EXP_SLTCTL_HPIE | PCI_EXP_SLTCTL_CCIE |
		PCI_EXP_SLTCTL_DLLSCE);

	pcie_write_cmd_nowait(ctrl, cmd, mask);
	ctrl_dbg(ctrl, "%s: SLOTCTRL %x write cmd %x\n", __func__,
		 pci_pcie_cap(ctrl->pci_dev) + PCI_EXP_SLTCTL, cmd);
}

static void pcie_disable_notification(struct controller *ctrl)
{
	u16 mask;

	mask = (PCI_EXP_SLTCTL_PDCE | PCI_EXP_SLTCTL_ABPE |
		PCI_EXP_SLTCTL_MRLSCE | PCI_EXP_SLTCTL_PFDE |
		PCI_EXP_SLTCTL_HPIE | PCI_EXP_SLTCTL_CCIE |
		PCI_EXP_SLTCTL_DLLSCE);
	pcie_write_cmd(ctrl, 0, mask);
	ctrl_dbg(ctrl, "%s: SLOTCTRL %x write cmd %x\n", __func__,
		 pci_pcie_cap(ctrl->pci_dev) + PCI_EXP_SLTCTL, 0);
}

void sunway_pcie_clear_hotplug_events(struct controller *ctrl)
{
	pcie_capability_write_word(ctrl_dev(ctrl), PCI_EXP_SLTSTA,
				   PCI_EXP_SLTSTA_PDC | PCI_EXP_SLTSTA_DLLSC);
}

/**
 * sunway_pciehp_slot_reset() - ignore link event caused by error-induced hot reset
 *
 * Called from pcie_portdrv_slot_reset() after AER or DPC initiated a reset
 * further up in the hierarchy to recover from an error.  The reset was
 * propagated down to this hotplug port.  Ignore the resulting link flap.
 * If the link failed to retrain successfully, synthesize the ignored event.
 * Surprise removal during reset is detected through Presence Detect Changed.
 */
int sunway_pciehp_slot_reset(struct pci_dev *dev)
{
	struct controller *ctrl = pci_get_drvdata(dev);

	if (ctrl->state != ON_STATE)
		return 0;

	pcie_capability_write_word(ctrl_dev(ctrl), PCI_EXP_SLTSTA,
			PCI_EXP_SLTSTA_DLLSC);

	if (!sunway_pciehp_check_link_active(ctrl))
		sunway_pciehp_request(ctrl, PCI_EXP_SLTSTA_DLLSC);

	return 0;
}

/*
 * sunway_pciehp has a 1:1 bus:slot relationship so we ultimately want a secondary
 * bus reset of the bridge, but at the same time we want to ensure that it is
 * not seen as a hot-unplug, followed by the hot-plug of the device. Thus,
 * disable link state notification and presence detection change notification
 * momentarily, if we see that they could interfere. Also, clear any spurious
 * events after.
 */
int sunway_pciehp_reset_slot(struct hotplug_slot *hotplug_slot, bool probe)
{
	struct controller *ctrl = to_ctrl(hotplug_slot);
	struct pci_dev *pdev = ctrl_dev(ctrl);
	u16 stat_mask = 0, ctrl_mask = 0;
	int rc;

	if (probe)
		return 0;

	down_write_nested(&ctrl->reset_lock, ctrl->depth);

	if (!ATTN_BUTTN(ctrl)) {
		ctrl_mask |= PCI_EXP_SLTCTL_PDCE;
		stat_mask |= PCI_EXP_SLTSTA_PDC;
	}
	ctrl_mask |= PCI_EXP_SLTCTL_DLLSCE;
	stat_mask |= PCI_EXP_SLTSTA_DLLSC;

	pcie_write_cmd(ctrl, 0, ctrl_mask);
	ctrl_dbg(ctrl, "%s: SLOTCTRL %x write cmd %x\n", __func__,
		 pci_pcie_cap(ctrl->pci_dev) + PCI_EXP_SLTCTL, 0);

	rc = pci_bridge_secondary_bus_reset(ctrl->pci_dev);

	pcie_capability_write_word(pdev, PCI_EXP_SLTSTA, stat_mask);
	pcie_write_cmd_nowait(ctrl, ctrl_mask, ctrl_mask);
	ctrl_dbg(ctrl, "%s: SLOTCTRL %x write cmd %x\n", __func__,
		 pci_pcie_cap(ctrl->pci_dev) + PCI_EXP_SLTCTL, ctrl_mask);

	up_write(&ctrl->reset_lock);
	return rc;
}

int sunway_pcie_init_notification(struct controller *ctrl)
{
	if (sunway_pciehp_request_irq(ctrl))
		return -1;
	pcie_enable_notification(ctrl);
	ctrl->notification_enabled = 1;
	return 0;
}

void sunway_pcie_shutdown_notification(struct controller *ctrl)
{
	if (ctrl->notification_enabled) {
		pcie_disable_notification(ctrl);
		sunway_pciehp_free_irq(ctrl);
		ctrl->notification_enabled = 0;
	}
}

static inline void dbg_ctrl(struct controller *ctrl)
{
	struct pci_dev *pdev = ctrl->pci_dev;
	u16 reg16;

	ctrl_dbg(ctrl, "Slot Capabilities	: 0x%08x\n", ctrl->slot_cap);
	pcie_capability_read_word(pdev, PCI_EXP_SLTSTA, &reg16);
	ctrl_dbg(ctrl, "Slot Status		: 0x%04x\n", reg16);
	pcie_capability_read_word(pdev, PCI_EXP_SLTCTL, &reg16);
	ctrl_dbg(ctrl, "Slot Control		: 0x%04x\n", reg16);
}

#define FLAG(x, y)	(((x) & (y)) ? '+' : '-')

static inline int sunway_pcie_hotplug_depth(struct pci_dev *dev)
{
	struct pci_bus *bus = dev->bus;
	int depth = 0;

	while (bus->parent) {
		bus = bus->parent;
		if (bus->self && bus->self->is_hotplug_bridge)
			depth++;
	}

	return depth;
}

struct controller *sunwayhpc_init(struct pci_dev *dev)
{
	struct controller *ctrl;
	u32 slot_cap, slot_cap2;
	u8 poweron;
	struct pci_bus *subordinate = dev->subordinate;

	ctrl = kzalloc(sizeof(*ctrl), GFP_KERNEL);
	if (!ctrl)
		return NULL;

	ctrl->pci_dev = dev;
	ctrl->depth = sunway_pcie_hotplug_depth(dev);
	pcie_capability_read_dword(dev, PCI_EXP_SLTCAP, &slot_cap);

	if (dev->hotplug_user_indicators)
		slot_cap &= ~(PCI_EXP_SLTCAP_AIP | PCI_EXP_SLTCAP_PIP);

	/*
	 * We assume no Thunderbolt controllers support Command Complete events,
	 * but some controllers falsely claim they do.
	 */
	if (dev->is_thunderbolt)
		slot_cap |= PCI_EXP_SLTCAP_NCCS;

	ctrl->slot_cap = slot_cap;
	mutex_init(&ctrl->ctrl_lock);
	mutex_init(&ctrl->state_lock);
	init_rwsem(&ctrl->reset_lock);
	init_waitqueue_head(&ctrl->requester);
	init_waitqueue_head(&ctrl->queue);
	INIT_DELAYED_WORK(&ctrl->button_work, sunway_pciehp_queue_pushbutton_work);
	dbg_ctrl(ctrl);

	down_read(&pci_bus_sem);
	ctrl->state = list_empty(&subordinate->devices) ? OFF_STATE : ON_STATE;
	up_read(&pci_bus_sem);

	pcie_capability_read_dword(dev, PCI_EXP_SLTCAP2, &slot_cap2);
	if (slot_cap2 & PCI_EXP_SLTCAP2_IBPD) {
		pcie_write_cmd_nowait(ctrl, PCI_EXP_SLTCTL_IBPD_DISABLE,
				      PCI_EXP_SLTCTL_IBPD_DISABLE);
		ctrl->inband_presence_disabled = 1;
	}

	if (dmi_first_match(inband_presence_disabled_dmi_table))
		ctrl->inband_presence_disabled = 1;

	/* Clear all remaining event bits in Slot Status register. */
	pcie_capability_write_word(dev, PCI_EXP_SLTSTA,
		PCI_EXP_SLTSTA_ABP | PCI_EXP_SLTSTA_PFD |
		PCI_EXP_SLTSTA_MRLSC | PCI_EXP_SLTSTA_CC |
		PCI_EXP_SLTSTA_DLLSC | PCI_EXP_SLTSTA_PDC);

	ctrl_info(ctrl, "Slot #%d AttnBtn%c PwrCtrl%c MRL%c AttnInd%c PwrInd%c HotPlug%c Surprise%c Interlock%c NoCompl%c IbPresDis%c LLActRep%c\n",
		(slot_cap & PCI_EXP_SLTCAP_PSN) >> 19,
		FLAG(slot_cap, PCI_EXP_SLTCAP_ABP),
		FLAG(slot_cap, PCI_EXP_SLTCAP_PCP),
		FLAG(slot_cap, PCI_EXP_SLTCAP_MRLSP),
		FLAG(slot_cap, PCI_EXP_SLTCAP_AIP),
		FLAG(slot_cap, PCI_EXP_SLTCAP_PIP),
		FLAG(slot_cap, PCI_EXP_SLTCAP_HPC),
		FLAG(slot_cap, PCI_EXP_SLTCAP_HPS),
		FLAG(slot_cap, PCI_EXP_SLTCAP_EIP),
		FLAG(slot_cap, PCI_EXP_SLTCAP_NCCS),
		FLAG(slot_cap2, PCI_EXP_SLTCAP2_IBPD),
		FLAG(dev->link_active_reporting, true));

	/*
	 * If empty slot's power status is on, turn power off.	The IRQ isn't
	 * requested yet, so avoid triggering a notification with this command.
	 */
	if (POWER_CTRL(ctrl)) {
		sunway_pciehp_get_power_status(ctrl, &poweron);
		if (!sunway_pciehp_card_present_or_link_active(ctrl) && poweron) {
			pcie_disable_notification(ctrl);
			sunway_pciehp_power_off_slot(ctrl);
		}
	}

	return ctrl;
}

void sunway_pciehp_release_ctrl(struct controller *ctrl)
{
	cancel_delayed_work_sync(&ctrl->button_work);
	kfree(ctrl);
}
