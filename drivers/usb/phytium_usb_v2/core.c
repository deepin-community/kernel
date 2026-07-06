// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium USB DRD Driver.
 *
 * Copyright (C) 2023 - 2024 Phytium.
 */

#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>

#include "core.h"
#include "otg.h"
#include "host.h"
#include "gadget.h"

static enum usb_role hw_role_state_machine(struct phytium_usb *phytium_usb)
{
	enum usb_role role = USB_ROLE_NONE;
	int id, vbus;

	if (phytium_usb->dr_mode != USB_DR_MODE_OTG) {
		if (phytium_usb_otg_is_host(phytium_usb))
			role = USB_ROLE_HOST;

		if (phytium_usb_otg_is_device(phytium_usb))
			role = USB_ROLE_DEVICE;

		return role;
	}

	id = phytium_usb_otg_get_id(phytium_usb);
	vbus = phytium_usb_otg_get_vbus(phytium_usb);

	role = phytium_usb->role;

	switch (role) {
	case USB_ROLE_NONE:
		if (!id)
			role = USB_ROLE_HOST;
		else if (vbus)
			role = USB_ROLE_DEVICE;
		break;
	case USB_ROLE_HOST:
		if (id)
			role = USB_ROLE_NONE;
		break;
	case USB_ROLE_DEVICE:
		if (!vbus)
			role = USB_ROLE_NONE;
		break;
	}

	dev_info(phytium_usb->dev, "role %d -> %d\n", phytium_usb->role, role);

	return role;
}

int phytium_usb_role_start(struct phytium_usb *phytium_usb, enum usb_role role)
{
	int ret;

	mutex_lock(&phytium_usb->mutex);
	phytium_usb->role = role;
	mutex_unlock(&phytium_usb->mutex);

	if (!phytium_usb->roles[role])
		return -ENXIO;

	if (phytium_usb->roles[role]->state == ROLE_STATE_ACTIVE)
		return 0;

	mutex_lock(&phytium_usb->mutex);
	ret = phytium_usb->roles[role]->start((void *)phytium_usb);
	if (!ret)
		phytium_usb->roles[role]->state = ROLE_STATE_ACTIVE;
	mutex_unlock(&phytium_usb->mutex);

	return ret;
}

void phytium_usb_role_stop(struct phytium_usb *phytium_usb)
{
	enum usb_role role;

	if (phytium_usb) {
		role = phytium_usb->role;

		if (phytium_usb->roles[role]->state == ROLE_STATE_INACTIVE)
			return;

		mutex_lock(&phytium_usb->mutex);
		phytium_usb->roles[role]->stop((void *)phytium_usb);
		phytium_usb->roles[role]->state = ROLE_STATE_INACTIVE;
		mutex_unlock(&phytium_usb->mutex);
	}
}

static enum usb_role role_get(struct usb_role_switch *sw)
{
	struct phytium_usb *phytium_usb = usb_role_switch_get_drvdata(sw);

	return phytium_usb->role;
}

static int role_set(struct usb_role_switch *sw, enum usb_role role)
{
	struct phytium_usb *phytium_usb = usb_role_switch_get_drvdata(sw);
	int ret = 0;

	if (phytium_usb->role == role)
		return ret;

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

int phytium_usb_hw_role_switch(struct phytium_usb *phytium_usb)
{
	enum usb_role real_role, current_role;
	int ret = 0;

	if (phytium_usb) {
		if (phytium_usb->role_sw)
			return ret;

		current_role = phytium_usb->role;
		real_role = hw_role_state_machine(phytium_usb);

		if (current_role == real_role)
			goto exit;

		if (current_role != 0 && real_role == 0)
			goto exit;

		phytium_usb_role_stop(phytium_usb);

		dev_info(phytium_usb->dev, "Switching role %d -> %d\n", current_role, real_role);

		ret = phytium_usb_role_start(phytium_usb, real_role);
		if (ret) {
			dev_err(phytium_usb->dev, "set %d has failed, back to %d\n",
					real_role, current_role);
			ret = phytium_usb_role_start(phytium_usb, current_role);
			if (ret)
				dev_err(phytium_usb->dev, "Back to %d failed too\n", current_role);
		}
	}

exit:
	return ret;
}


static int phytium_usb_set_mode(struct phytium_usb *phytium_usb)
{
	if (phytium_usb) {
		if (phytium_usb->dr_mode == USB_DR_MODE_OTG) {
			phytium_usb_otg_disable_irq((void *)phytium_usb);
			phytium_usb_otg_clear_irq((void *)phytium_usb);
		}
	}

	return 0;
}

static int phytium_usb_start_mode(struct phytium_usb *phytium_usb)
{
	int ret = 0;

	if (phytium_usb) {
		switch (phytium_usb->dr_mode) {
		case USB_DR_MODE_HOST:
			ret = phytium_usb_role_start(phytium_usb, USB_ROLE_HOST);
			break;
		case USB_DR_MODE_PERIPHERAL:
			ret = phytium_usb_role_start(phytium_usb, USB_ROLE_DEVICE);
			break;
		case USB_DR_MODE_OTG:
			ret = phytium_usb_hw_role_switch(phytium_usb);
			break;
		default:
			dev_warn(phytium_usb->dev, "unknown dr_mode:%d\n", phytium_usb->dr_mode);
			ret = -EINVAL;
			goto err;
		}
	}

	return ret;
err:
	phytium_usb_role_stop(phytium_usb);
	phytium_usb_otg_disable_irq((void *)phytium_usb);

	return ret;
}

static int idle_role_start(void *data)
{
	return 0;
}

static void idle_role_stop(void *data)
{
}

static int phytium_usb_idle_init(struct phytium_usb *phytium_usb)
{
	struct phytium_usb_role_driver *rdrv;

	rdrv = devm_kzalloc(phytium_usb->dev, sizeof(*rdrv), GFP_KERNEL);
	if (!rdrv)
		return -ENOMEM;

	rdrv->start = idle_role_start;
	rdrv->stop = idle_role_stop;
	rdrv->state = ROLE_STATE_INACTIVE;
	rdrv->suspend = NULL;
	rdrv->resume = NULL;
	rdrv->name = "idle";

	phytium_usb->roles[USB_ROLE_NONE] = rdrv;

	return 0;
}

static int phytium_usb_init_role(struct phytium_usb *phytium_usb)
{
	int ret = 0;
	enum usb_dr_mode dr_mode;

	phytium_usb->role = USB_ROLE_NONE;

	ret = phytium_usb_idle_init(phytium_usb);
	if (ret)
		return ret;

	dr_mode = usb_get_dr_mode(phytium_usb->dev);
	if (dr_mode == USB_DR_MODE_UNKNOWN)
		dr_mode = USB_DR_MODE_OTG;

	phytium_usb_host_init((void *)phytium_usb);

	phytium_usb_gadget_init((void *)phytium_usb);


	phytium_usb_set_mode(phytium_usb);

	phytium_usb->dr_mode = dr_mode;

	ret = phytium_usb_start_mode(phytium_usb);

	return ret;
}

int phytium_usb_init(struct phytium_usb *phytium_usb)
{
	int ret;

	if (!phytium_usb)
		return 0;

	mutex_init(&phytium_usb->mutex);

	ret = dma_set_mask_and_coherent(phytium_usb->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_warn(phytium_usb->dev, "set dma mask failed with:%d\n", ret);
		return ret;
	}

	if (device_property_read_bool(phytium_usb->dev, "usb-role-switch")) {
		struct usb_role_switch_desc sw_desc = { };

		sw_desc.get = role_get;
		sw_desc.set = role_set;
		sw_desc.allow_userspace_control = true;
		sw_desc.driver_data = phytium_usb;
		sw_desc.fwnode = phytium_usb->dev->fwnode;

		phytium_usb->role_sw = usb_role_switch_register(phytium_usb->dev, &sw_desc);
		if (IS_ERR(phytium_usb->role_sw)) {
			dev_warn(phytium_usb->dev, "Unalbe to register Role Switch\n");
			return PTR_ERR(phytium_usb->role_sw);
		}
	}

	ret = phytium_usb_otg_init((void *)phytium_usb);
	if (ret)
		goto init_failed;

	ret = phytium_usb_init_role(phytium_usb);
	if (ret)
		goto init_failed;

	spin_lock_init(&phytium_usb->lock);

	return 0;

init_failed:
	phytium_usb_otg_disable_irq((void *)phytium_usb);

	if (phytium_usb->role_sw)
		usb_role_switch_unregister(phytium_usb->role_sw);

	return ret;
}

int phytium_usb_uninit(struct phytium_usb *phytium_usb)
{
	if (phytium_usb->role_sw)
		usb_role_switch_unregister(phytium_usb->role_sw);

	phytium_usb_role_stop(phytium_usb);
	phytium_usb_otg_uninit((void *)phytium_usb);

	kfree(phytium_usb);

	return 0;
}

int phytium_usb_resume(struct phytium_usb *phytium_usb, u8 set_active)
{
	enum usb_role real_role;
	bool role_changed = false;
	int ret = 0;

	if (phytium_usb_otg_power_is_lost((void *)phytium_usb)) {
		if (!phytium_usb->role_sw) {
			real_role = hw_role_state_machine(phytium_usb);
			if (real_role != phytium_usb->role) {
				ret = phytium_usb_hw_role_switch(phytium_usb);
				if (ret)
					return ret;

				role_changed = true;
			}
		}

		if (!role_changed) {
			if (phytium_usb->role == USB_ROLE_HOST)
				ret = phytium_usb_otg_host_on((void *)phytium_usb);
			else if (phytium_usb->role == USB_ROLE_DEVICE)
				ret = phytium_usb_otg_gadget_on((void *)phytium_usb);

			if (ret)
				return ret;
		}
	}

	if (phytium_usb->roles[phytium_usb->role]->resume)
		phytium_usb->roles[phytium_usb->role]->resume(phytium_usb,
				phytium_usb_otg_power_is_lost(phytium_usb));

	return 0;
}

int phytium_usb_suspend(struct phytium_usb *phytium_usb)
{
	unsigned long flags;

	if (phytium_usb->roles[phytium_usb->role]->suspend) {
		spin_lock_irqsave(&phytium_usb->lock, flags);
		phytium_usb->roles[phytium_usb->role]->suspend(phytium_usb, false);
		spin_unlock_irqrestore(&phytium_usb->lock, flags);
	}

	return 0;
}

MODULE_AUTHOR("Zhenhua Chen <chenzhenhua@phytium.com.cn>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Phytium USB2 DRD Controller Driver");
