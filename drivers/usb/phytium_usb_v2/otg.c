// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium USB DRD Driver.
 *
 * Copyright (C) 2023 - 2024 Phytium.
 */

#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/module.h>

#include "core.h"
#include "otg.h"

#define OTG_NRDY	BIT(11)
#define OTG_HOST_MODE	BIT(12)
#define OTG_DEVICE_MODE	BIT(13)

#define OVERRIDE_IDPULLUP  BIT(0)
#define OVERRIDE_SESS_VLD_SEL	BIT(10)

#define OTGIEN_ID_CHANGE	BIT(0)
#define OTGIEN_VBUSVALID_RISE	BIT(4)
#define OTGIEN_VBUSVALID_FALL	BIT(5)

#define OTGIEN_ID_CHANGE_INT		BIT(0)
#define OTGIEN_VBUSVALID_RISE_INT	BIT(4)
#define OTGIEN_VBUSVALID_FALL_INT	BIT(5)

#define OTGCMD_DEV_BUS_REQ	BIT(0)
#define OTGCMD_HOST_BUS_REQ	BIT(1)
#define OTGCMD_OTG_DIS		BIT(3)
#define OTGCMD_DEV_BUS_DROP	BIT(8)
#define OTGCMD_HOST_BUS_DROP	BIT(9)

#define OTGSTS_HOST_READY	BIT(27)
#define OTGSTS_DEV_READY	BIT(26)
#define OTGSTS_VBUS_VALID	BIT(1)
#define OTGSTS_ID_VALUE		BIT(0)

#define OTGSTATE_HOST_STATE_MASK	GENMASK(5, 3)
#define OTGSTATE_DEV_STATE_MASK		GENMASK(2, 0)

#define ID_HOST			0
#define ID_PERIPHERAL	1
//0 NONE  1:HOST 2:DEVICE

static int switch_role(struct phytium_usb *phytium_usb, enum usb_role role)
{
	int ret = 0;

	if (phytium_usb->dr_mode == USB_DR_MODE_HOST) {
		switch (role) {
		case USB_ROLE_NONE:
		case USB_ROLE_HOST:
			break;
		default:
			return ret;
		}
	}

	if (phytium_usb->dr_mode == USB_DR_MODE_PERIPHERAL) {
		switch (role) {
		case USB_ROLE_NONE:
		case USB_ROLE_DEVICE:
			break;
		default:
			return ret;
		}
	}

	phytium_usb_role_stop(phytium_usb);
	ret = phytium_usb_role_start(phytium_usb, role);
	if (ret)
		dev_err(phytium_usb->dev, "set role %d has failed\n", role);

	return ret;
}

static ssize_t role_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct phytium_usb *phytium_usb = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", phytium_usb->role);
}

static ssize_t role_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct phytium_usb *phytium_usb = dev_get_drvdata(dev);
	unsigned int role;
	int ret;

	ret = kstrtouint(buf, 10, &role);
	if (ret)
		return ret;

	if (role != phytium_usb->role) {
		dev_err(phytium_usb->dev, "role switch %d -> %d\n", phytium_usb->role, role);
		switch_role(phytium_usb, (enum usb_role)role);
	}

	return count;
}

static DEVICE_ATTR_RW(role);

static struct attribute *phytium_attributes[] = {
	&dev_attr_role.attr,
	NULL
};

static const struct attribute_group phytium_attr_group = {
	.attrs = phytium_attributes,
};

bool phytium_usb_otg_power_is_lost(void *data)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb) {
		if (!(readl(&phytium_usb->otg_regs->simulate) & BIT(0)))
			return true;
	}

	return false;
}

int phytium_usb_otg_is_host(void *data)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb) {
		if (phytium_usb->dr_mode == USB_DR_MODE_HOST)
			return true;
		else if (phytium_usb_otg_get_id(data) == ID_HOST)
			return true;
	}

	return false;
}

int phytium_usb_otg_is_device(void *data)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb) {
		if (phytium_usb->dr_mode == USB_DR_MODE_PERIPHERAL)
			return true;
		else if (phytium_usb->dr_mode == USB_DR_MODE_OTG)
			if (phytium_usb_otg_get_id(data) == ID_PERIPHERAL)
				return true;
	}

	return false;
}

int phytium_usb_otg_get_id(void *data)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;
	int id = 0;

	if (phytium_usb)
		id = !!(readl(&phytium_usb->otg_regs->sts) & OTGSTS_ID_VALUE);

	dev_err(phytium_usb->dev, "OTG ID: %d\n", id);

	return id;
}

int phytium_usb_otg_get_vbus(void *data)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;
	int vbus = 0;

	if (phytium_usb)
		vbus = !!(readl(&phytium_usb->otg_regs->sts) & OTGSTS_VBUS_VALID);

	dev_err(phytium_usb->dev, "OTG VBUS: %d\n", vbus);

	return vbus;
}

static irqreturn_t phytium_usb_otg_thread_irq(int irq, void *data)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb)
		phytium_usb_hw_role_switch(phytium_usb);

	return IRQ_HANDLED;
}

static irqreturn_t phytium_usb_otg_irq(int irq, void *data)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;
	irqreturn_t ret = IRQ_NONE;
	u32 reg;

	if (phytium_usb) {
		if (phytium_usb->dr_mode != USB_DR_MODE_OTG)
			return IRQ_NONE;

		if (phytium_usb->in_lpm)
			return ret;

		reg = readl(&phytium_usb->otg_regs->ivect);
		if (!reg)
			return ret;

		if (reg & OTGIEN_ID_CHANGE_INT) {
			dev_err(phytium_usb->dev, "OTG IRQ: new ID:%d\n",
					phytium_usb_otg_get_id(phytium_usb));
			ret = IRQ_WAKE_THREAD;
		}

		if (reg & (OTGIEN_VBUSVALID_RISE_INT | OTGIEN_VBUSVALID_FALL_INT)) {
			dev_err(phytium_usb->dev, "OTG IRQ: new VBUS: %d\n",
					phytium_usb_otg_get_vbus(phytium_usb));
			ret = IRQ_WAKE_THREAD;
		}

		writel(~0, &phytium_usb->otg_regs->ivect);
	}

	return ret;
}

int phytium_usb_otg_init(void *data)
{
	u32 state;
	int ret;
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb) {
		phytium_usb->otg_regs = devm_ioremap_resource(phytium_usb->dev,
				&phytium_usb->otg_res);
		if (IS_ERR(phytium_usb->otg_regs))
			return PTR_ERR(phytium_usb->otg_regs);

		state = readl(&phytium_usb->otg_regs->sts);
		if (state & OTG_NRDY) {
			dev_warn(phytium_usb->dev, "controller is not ready yet\n");
			return -ENODEV;
		}

		phytium_usb->dr_mode = USB_DR_MODE_OTG;//USB_DR_MODE_PERIPHERAL;
		if (OTG_HOST_MODE & state)
			phytium_usb->dr_mode = USB_DR_MODE_HOST;
		else if (OTG_DEVICE_MODE & state)
			phytium_usb->dr_mode = USB_DR_MODE_PERIPHERAL;

		ret = devm_request_threaded_irq(phytium_usb->dev, phytium_usb->otg_irq,
			phytium_usb_otg_irq, phytium_usb_otg_thread_irq,
			IRQF_SHARED, "phytium_usb_otg", phytium_usb);
		if (ret) {
			dev_warn(phytium_usb->dev, "request otg irq failed with:%d\n", ret);
			return ret;
		}

		ret = devm_device_add_group(phytium_usb->dev, &phytium_attr_group);
		if (ret)
			dev_err(phytium_usb->dev, "Create sysfs attributes, err: %d\n", ret);
	}

	return 0;
}

int phytium_usb_otg_enable_iddig(void *data)
{
	u32 reg;
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb) {
		reg = readl(&phytium_usb->otg_regs->override);
		reg |= OVERRIDE_IDPULLUP;
		writel(reg, &phytium_usb->otg_regs->override);
		usleep_range(50000, 70000);
	}

	return 0;
}

int phytium_usb_otg_disable_irq(void *data)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb)
		writel(0, &phytium_usb->otg_regs->ien);

	return 0;
}

int phytium_usb_otg_clear_irq(void *data)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb)
		writel(~0, &phytium_usb->otg_regs->ivect);

	return 0;
}

int phytium_usb_otg_enable_irq(void *data)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb)
		writel(OTGIEN_ID_CHANGE | OTGIEN_VBUSVALID_RISE |
				OTGIEN_VBUSVALID_FALL,
				&phytium_usb->otg_regs->ien);

	return 0;
}

int phytium_usb_otg_host_on(void *data)
{
	int ret = 0;
	u32 val;
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb) {
		writel(OTGCMD_HOST_BUS_REQ | OTGCMD_OTG_DIS,
				&phytium_usb->otg_regs->cmd);
		ret = readl_poll_timeout_atomic(&phytium_usb->otg_regs->sts,
				val, val & OTGSTS_HOST_READY, 1, 100000);
		if (ret)
			dev_err(phytium_usb->dev, "wait for host_ready fail\n");
		phytium_usb->dr_mode = USB_DR_MODE_HOST;
	}

	return ret;
}

int phytium_usb_otg_host_off(void *data)
{
	u32 val;
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb) {
		writel(OTGCMD_HOST_BUS_DROP | OTGCMD_DEV_BUS_DROP,
			&phytium_usb->otg_regs->cmd);

		readl_poll_timeout_atomic(&phytium_usb->otg_regs->state, val,
				!(val & OTGSTATE_HOST_STATE_MASK), 1, 2000000);
		phytium_usb->dr_mode = USB_DR_MODE_OTG;
	}

	return 0;
}

int phytium_usb_otg_gadget_on(void *data)
{
	int ret = 0;
	u32 val;
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb) {
		writel(OTGCMD_DEV_BUS_REQ | OTGCMD_OTG_DIS,
				&phytium_usb->otg_regs->cmd);
		ret = readl_poll_timeout_atomic(&phytium_usb->otg_regs->sts,
				val, val & OTGSTS_DEV_READY, 1, 100000);
		if (ret)
			dev_err(phytium_usb->dev, "wait for host_ready fail\n");
		phytium_usb->dr_mode = USB_DR_MODE_PERIPHERAL;
	}

	return ret;
}

int phytium_usb_otg_gadget_off(void *data)
{
	u32 val;
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb) {
		writel(OTGCMD_HOST_BUS_DROP | OTGCMD_DEV_BUS_DROP,
			&phytium_usb->otg_regs->cmd);

		readl_poll_timeout_atomic(&phytium_usb->otg_regs->state, val,
				!(val & OTGSTATE_DEV_STATE_MASK), 1, 2000000);
		phytium_usb->dr_mode = USB_DR_MODE_OTG;
	}

	return 0;
}

void phytium_usb_otg_uninit(void *data)
{
	phytium_usb_otg_host_off(data);
	phytium_usb_otg_disable_irq(data);
}

MODULE_AUTHOR("Zhenhua Chen <chenzhenhua@phytium.com.cn>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Phytium USB2 DRD Controller Driver");
