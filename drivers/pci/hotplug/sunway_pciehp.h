/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunway PCI Express Hot Plug Controller Driver
 */
#ifndef _SUNWAYPCIEHP_H
#define _SUNWAYPCIEHP_H

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/pci_hotplug.h>
#include <linux/delay.h>
#include <linux/sched/signal.h>		/* signal_pending() */
#include <linux/mutex.h>
#include <linux/rwsem.h>
#include <linux/workqueue.h>

#include "../pcie/portdrv.h"

#define MY_NAME	"sunway_pciehp"

extern bool sunway_pciehp_poll_mode;
extern int  sunway_pciehp_poll_time;

/*
 * Set CONFIG_DYNAMIC_DEBUG=y and boot with 'dyndbg="file sunway_pciehp* +p"' to
 * enable debug messages.
 */
#define ctrl_dbg(ctrl, format, arg...)                                  \
	pci_dbg(ctrl->pci_dev, format, ## arg)
#define ctrl_err(ctrl, format, arg...)                                  \
	pci_err(ctrl->pci_dev, format, ## arg)
#define ctrl_info(ctrl, format, arg...)                                 \
	pci_info(ctrl->pci_dev, format, ## arg)
#define ctrl_warn(ctrl, format, arg...)                                 \
	pci_warn(ctrl->pci_dev, format, ## arg)

#define SLOT_NAME_SIZE 10

struct saved_piu_space {
	unsigned long epdmabar;
	unsigned long msiaddr;
	unsigned long iommuexcpt_ctrl;
	unsigned long dtbaseaddr;
	unsigned long intaconfig;
	unsigned long intbconfig;
	unsigned long intcconfig;
	unsigned long intdconfig;
	unsigned long pmeintconfig;
	unsigned long aererrintconfig;
	unsigned long hpintconfig;
	unsigned int state_saved:1;
};

/**
 * struct controller - PCIe hotplug controller
 * @slot_cap: cached copy of the Slot Capabilities register
 * @inband_presence_disabled: In-Band Presence Detect Disable supported by
 *      controller and disabled per spec recommendation (PCIe r5.0, appendix I
 *      implementation note)
 * @slot_ctrl: cached copy of the Slot Control register
 * @ctrl_lock: serializes writes to the Slot Control register
 * @cmd_started: jiffies when the Slot Control register was last written;
 *      the next write is allowed 1 second later, absent a Command Completed
 *      interrupt (PCIe r4.0, sec 6.7.3.2)
 * @cmd_busy: flag set on Slot Control register write, cleared by IRQ handler
 *      on reception of a Command Completed event
 * @queue: wait queue to wake up on reception of a Command Completed event,
 *      used for synchronous writes to the Slot Control register
 * @pending_events: used by the IRQ handler to save events retrieved from the
 *      Slot Status register for later consumption by the IRQ thread
 * @notification_enabled: whether the IRQ was requested successfully
 * @power_fault_detected: whether a power fault was detected by the hardware
 *      that has not yet been cleared by the user
 * @poll_thread: thread to poll for slot events if no IRQ is available,
 *      enabled with pciehp_poll_mode module parameter
 * @state: current state machine position
 * @state_lock: protects reads and writes of @state;
 *      protects scheduling, execution and cancellation of @button_work
 * @button_work: work item to turn the slot on or off after 5 seconds
 *      in response to an Attention Button press
 * @hotplug_slot: structure registered with the PCI hotplug core
 * @reset_lock: prevents access to the Data Link Layer Link Active bit in the
 *      Link Status register and to the Presence Detect State bit in the Slot
 *      Status register during a slot reset which may cause them to flap
 * @depth: Number of additional hotplug ports in the path to the root bus,
 *      used as lock subclass for @reset_lock
 * @ist_running: flag to keep user request waiting while IRQ thread is running
 * @request_result: result of last user request submitted to the IRQ thread
 * @requester: wait queue to wake up on completion of user request,
 *      used for synchronous slot enable/disable request via sysfs
 *
 * PCIe hotplug has a 1:1 relationship between controller and slot, hence
 * unlike other drivers, the two aren't represented by separate structures.
 */
struct controller {
	struct pci_dev *pci_dev;

	u32 slot_cap;                           /* capabilities and quirks */
	unsigned int inband_presence_disabled:1;
	u16 slot_ctrl;                          /* control register access */
	struct mutex ctrl_lock;
	unsigned long cmd_started;
	unsigned int cmd_busy:1;
	wait_queue_head_t queue;

	atomic_t pending_events;                /* event handling */
	unsigned int notification_enabled:1;
	unsigned int power_fault_detected;
	struct task_struct *poll_thread;

	u8 state;                               /* state machine */
	struct mutex state_lock;
	struct delayed_work button_work;

	struct hotplug_slot hotplug_slot;       /* hotplug core interface */
	struct rw_semaphore reset_lock;
	unsigned int depth;
	unsigned int ist_running;
	int request_result;
	wait_queue_head_t requester;

	struct saved_piu_space saved_piu;
};

/**
 * DOC: Slot state
 *
 * @OFF_STATE: slot is powered off, no subordinate devices are enumerated
 * @BLINKINGON_STATE: slot will be powered on after the 5 second delay,
 *	Power Indicator is blinking
 * @BLINKINGOFF_STATE: slot will be powered off after the 5 second delay,
 *	Power Indicator is blinking
 * @POWERON_STATE: slot is currently powering on
 * @POWEROFF_STATE: slot is currently powering off
 * @ON_STATE: slot is powered on, subordinate devices have been enumerated
 */
#define OFF_STATE			0
#define BLINKINGON_STATE		1
#define BLINKINGOFF_STATE		2
#define POWERON_STATE			3
#define POWEROFF_STATE			4
#define ON_STATE			5

/**
 * DOC: Flags to request an action from the IRQ thread
 *
 * These are stored together with events read from the Slot Status register,
 * hence must be greater than its 16-bit width.
 *
 * %DISABLE_SLOT: Disable the slot in response to a user request via sysfs or
 *	an Attention Button press after the 5 second delay
 * %RERUN_ISR: Used by the IRQ handler to inform the IRQ thread that the
 *	hotplug port was inaccessible when the interrupt occurred, requiring
 *	that the IRQ handler is rerun by the IRQ thread after it has made the
 *	hotplug port accessible by runtime resuming its parents to D0
 */
#define DISABLE_SLOT		(1 << 16)
#define RERUN_ISR		(1 << 17)
#define SW64_POLL_DISABLE_SLOT	(1 << 18)
#define SW64_POLL_ENABLE_SLOT	(1 << 19)

#define ATTN_BUTTN(ctrl)	((ctrl)->slot_cap & PCI_EXP_SLTCAP_ABP)
#define POWER_CTRL(ctrl)	((ctrl)->slot_cap & PCI_EXP_SLTCAP_PCP)
#define MRL_SENS(ctrl)		((ctrl)->slot_cap & PCI_EXP_SLTCAP_MRLSP)
#define ATTN_LED(ctrl)		((ctrl)->slot_cap & PCI_EXP_SLTCAP_AIP)
#define PWR_LED(ctrl)		((ctrl)->slot_cap & PCI_EXP_SLTCAP_PIP)
#define NO_CMD_CMPL(ctrl)	((ctrl)->slot_cap & PCI_EXP_SLTCAP_NCCS)
#define PSN(ctrl)		(((ctrl)->slot_cap & PCI_EXP_SLTCAP_PSN) >> 19)

#define HP_CTRL_FINISH  0x0
#define HP_CTRL_INSERT  0x1
#define HP_CTRL_REMOVE  0x2

void sunway_pciehp_request(struct controller *ctrl, int action);
void sunway_pciehp_handle_button_press(struct controller *ctrl);
void sunway_pciehp_handle_disable_request(struct controller *ctrl);
void sunway_pciehp_handle_presence_or_link_change(struct controller *ctrl, u32 events);
int sunway_pciehp_configure_device(struct controller *ctrl);
void sunway_pciehp_unconfigure_device(struct controller *ctrl, bool presence);
void sunway_pciehp_queue_pushbutton_work(struct work_struct *work);
struct controller *sunwayhpc_init(struct pci_dev *dev);
int sunway_pcie_init_notification(struct controller *ctrl);
void sunway_pcie_shutdown_notification(struct controller *ctrl);
void sunway_pcie_clear_hotplug_events(struct controller *ctrl);
int sunway_pciehp_power_on_slot(struct controller *ctrl);
void sunway_pciehp_power_off_slot(struct controller *ctrl);
void sunway_pciehp_get_power_status(struct controller *ctrl, u8 *status);

#define INDICATOR_NOOP -1       /* Leave indicator unchanged */
void sunway_pciehp_set_indicators(struct controller *ctrl, int pwr, int attn);

void sunway_pciehp_get_latch_status(struct controller *ctrl, u8 *status);
int sunway_pciehp_query_power_fault(struct controller *ctrl);
int sunway_pciehp_card_present(struct controller *ctrl);
int sunway_pciehp_card_present_or_link_active(struct controller *ctrl);
int sunway_pciehp_check_link_status(struct controller *ctrl);
int sunway_pciehp_check_link_active(struct controller *ctrl);
void sunway_pciehp_release_ctrl(struct controller *ctrl);

int sunway_pciehp_sysfs_enable_slot(struct hotplug_slot *hotplug_slot);
int sunway_pciehp_sysfs_disable_slot(struct hotplug_slot *hotplug_slot);
int sunway_pciehp_reset_slot(struct hotplug_slot *hotplug_slot, bool probe);
int sunway_pciehp_get_attention_status(struct hotplug_slot *hotplug_slot, u8 *status);
int sunway_pciehp_set_raw_indicator_status(struct hotplug_slot *hotplug_slot, u8 status);
int sunway_pciehp_get_raw_indicator_status(struct hotplug_slot *hotplug_slot, u8 *status);

int sunway_pciehp_link_enable(struct controller *ctrl);
int sunway_pciehp_link_disable(struct controller *ctrl);
void sunway_pciehp_restore_rc_piu(struct controller *ctrl);

int sunway_pciehp_slot_reset(struct pci_dev *dev);

static inline const char *slot_name(struct controller *ctrl)
{
	return hotplug_slot_name(&ctrl->hotplug_slot);
}

static inline struct controller *to_ctrl(struct hotplug_slot *hotplug_slot)
{
	return container_of(hotplug_slot, struct controller, hotplug_slot);
}

#endif				/* _SUNWAYPCIEHP_H */
