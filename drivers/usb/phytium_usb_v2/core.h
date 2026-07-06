/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __LINUX_PHYTIUM_USB_CORE_H__
#define __LINUX_PHYTIUM_USB_CORE_H__

#include <linux/usb/otg.h>
#include <linux/platform_device.h>
#include <linux/usb/role.h>

#include "otg.h"

#define ROLE_STATE_INACTIVE	0
#define ROLE_STATE_ACTIVE	1
#define PHYTIUM_USB_DRIVER_V2_VERSION "1.0.1"

struct phytium_usb_platform_data {
	int (*platform_suspend)(struct device *dev, bool suspend, bool wakeup);
	unsigned long quirks;
};

struct phytium_usb_role_driver {
	int (*start)(void *data);
	void (*stop)(void *data);
	int (*suspend)(void *data, bool do_wakeup);
	int (*resume)(void *data, bool hibernated);
	const char *name;
	int state;
};

struct phytium_usb {
	struct device *dev;
	void __iomem *host_regs;
	struct resource	host_res[2];
	struct phytium_usb_platform_data	*platdata;
	int device_irq;
	void __iomem *device_regs;
	int otg_irq;
	struct resource otg_res;
	struct phytium_usb_otg_regs __iomem *otg_regs;
	enum usb_dr_mode dr_mode;
	enum usb_role  role;
	struct phytium_usb_role_driver *roles[USB_ROLE_DEVICE + 1];
	struct platform_device *host_dev;
	struct xhci_plat_priv *host_plat_data;
	spinlock_t lock;
	void *gadget_dev;
	struct mutex mutex;
	bool in_lpm;
	struct usb_role_switch *role_sw;
};

int phytium_usb_init(struct phytium_usb *phytium_usb);
int phytium_usb_uninit(struct phytium_usb *phytium_usb);
int phytium_usb_hw_role_switch(struct phytium_usb *phytium_usb);
int phytium_usb_resume(struct phytium_usb *phytium_usb, u8 set_active);
int phytium_usb_suspend(struct phytium_usb *phytium_usb);
int phytium_usb_role_start(struct phytium_usb *phytium_usb, enum usb_role role);
void phytium_usb_role_stop(struct phytium_usb *phytium_usb);

#endif
