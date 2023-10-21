// SPDX-License-Identifier: GPL-2.0+
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include "gsgpu.h"
#include "gsgpu_dc_vbios.h"
#include "gsgpu_backlight.h"
#include "bridge_phy.h"

MODULE_SOFTDEP("post: loongson-laptop");
MODULE_SOFTDEP("post: pwm-ls");

static bool gsgpu_backlight_get_hw_status(struct gsgpu_backlight *ls_bl)
{
	return pwm_is_enabled(ls_bl->pwm);
}

static void gsgpu_backlight_enable(struct gsgpu_backlight *ls_bl)
{
	struct gsgpu_device *adev = ls_bl->driver_private;
	struct gsgpu_bridge_phy *phy =
		adev->mode_info.encoders[ls_bl->display_pipe_index]->bridge;

	if (IS_ERR(ls_bl->pwm))
		return;

	if (phy && phy->cfg_funcs && phy->cfg_funcs->backlight_ctrl)
		phy->cfg_funcs->backlight_ctrl(phy, DRM_MODE_DPMS_ON);
	else {
		BACKLIGHT_DEFAULT_METHOD_OPEN(ls_bl);
		DRM_DEBUG("%s set backlight enable.\n",
			 phy ? phy->res->chip_name : "");
	}

	ls_bl->device->props.power = FB_BLANK_UNBLANK;
}

static void gsgpu_backlight_disable(struct gsgpu_backlight *ls_bl)
{
	struct gsgpu_device *adev = ls_bl->driver_private;
	struct gsgpu_bridge_phy *phy =
		adev->mode_info.encoders[ls_bl->display_pipe_index]->bridge;

	if (IS_ERR(ls_bl->pwm))
		return;

	if (phy && phy->cfg_funcs && phy->cfg_funcs->backlight_ctrl)
		phy->cfg_funcs->backlight_ctrl(phy, DRM_MODE_DPMS_OFF);
	else {
		BACKLIGHT_DEFAULT_METHOD_CLOSE(ls_bl);
		DRM_DEBUG("%s set backlight disable.\n",
			 phy ? phy->res->chip_name : "");
	}

	ls_bl->device->props.power = !FB_BLANK_UNBLANK;
}

static void gsgpu_backlight_power(struct gsgpu_backlight *ls_bl, bool enable)
{
	DRM_DEBUG("Request backlight power: %s->%s.\n",
		  ls_bl->hw_enabled ? "open" : "close",
		  enable ? "open" : "close");

	if (enable && !ls_bl->hw_enabled) {
		if (ls_bl->enable)
			ls_bl->enable(ls_bl);
	} else if (!enable && ls_bl->hw_enabled) {
		if (ls_bl->disable)
			ls_bl->disable(ls_bl);
	}
}

static int gsgpu_backlight_update(struct backlight_device *bd)
{
	struct gsgpu_backlight *ls_bl = bl_get_data(bd);

	DRM_DEBUG("Request bl update: %s->%s, level:%d->%d.\n",
		  ls_bl->hw_enabled ? "open" : "close",
		  bd->props.power == FB_BLANK_UNBLANK ? "open" : "close",
		  ls_bl->level, bd->props.brightness);

	if (ls_bl->hw_enabled != (bd->props.power == FB_BLANK_UNBLANK))
		ls_bl->power(ls_bl, bd->props.power == FB_BLANK_UNBLANK);

	if (ls_bl->level != bd->props.brightness) {
		ls_bl->level = bd->props.brightness;
		ls_bl->set_brightness(ls_bl, ls_bl->level);
	}

	return 0;
}

static int gsgpu_backlight_get_brightness(struct backlight_device *bd)
{
	struct gsgpu_backlight *ls_bl = bl_get_data(bd);

	if (ls_bl->get_brightness)
		return ls_bl->get_brightness(ls_bl);

	return -ENOEXEC;
}

static const struct backlight_ops gsgpu_backlight_ops = {
	.update_status  = gsgpu_backlight_update,
	.get_brightness = gsgpu_backlight_get_brightness,
};

static unsigned int gsgpu_backlight_get(struct gsgpu_backlight *ls_bl)
{
	u16 duty_ns, period_ns;
	u32 level;

	if (IS_ERR(ls_bl->pwm))
		return 0;

	period_ns = ls_bl->pwm_period;
	duty_ns = pwm_get_duty_cycle(ls_bl->pwm);

	level = DIV_ROUND_UP((duty_ns * ls_bl->max), period_ns);
	level = clamp(level, ls_bl->min, ls_bl->max);

	return level;
}

static void gsgpu_backlight_set(struct gsgpu_backlight *ls_bl,
		unsigned int level)
{
	unsigned int period_ns;
	unsigned int duty_ns;

	if (IS_ERR(ls_bl->pwm))
		return;

	level = clamp(level, ls_bl->min, ls_bl->max);
	period_ns = ls_bl->pwm_period;
	duty_ns = DIV_ROUND_UP((level * period_ns), ls_bl->max);

	DRM_DEBUG("Set backlight: level=%d, 0x%x/0x%x ns.\n",
		  level, duty_ns, period_ns);

	pwm_config(ls_bl->pwm, duty_ns, period_ns);
}

static int gsgpu_backlight_hw_request_init(struct gsgpu_device *adev,
					   struct gsgpu_backlight *ls_bl)
{
	bool pwm_enable_default;
	char pwm_consumer_name[32];

	snprintf(pwm_consumer_name, sizeof(pwm_consumer_name), "ls-pwm-%02d", ls_bl->pwm_id);
	ls_bl->pwm = pwm_get(&adev->loongson_dc->dev, pwm_consumer_name);
	if (IS_ERR(ls_bl->pwm)) {
		DRM_ERROR("Failed to get the pwm chip (%s)\n", pwm_consumer_name);
		ls_bl->pwm = NULL;
		return -ENODEV;
	}

	/* Disable PWM, configure its polarity and period, and then re-enable it.*/
	pwm_enable_default = pwm_is_enabled(ls_bl->pwm);
	struct pwm_state state;
	pwm_get_state(ls_bl->pwm, &state);
	state.polarity = ls_bl->pwm_polarity;
	state.period = ls_bl->pwm_period;
	state.enabled = false;
	pwm_apply_state(ls_bl->pwm, &state);
	gsgpu_backlight_set(ls_bl, ls_bl->level);
	if (pwm_enable_default)
		pwm_enable(ls_bl->pwm);

	/* Turn the LCD on, just in case the HW for some reason did not.
	 * We don't bother to check the return values as some HW may not
	 * expose the same LCD GPIO pins. */
	gpio_request(GPIO_LCD_VDD, "GPIO_VDD");
	gpio_request(GPIO_LCD_EN, "GPIO_EN");
	gpio_direction_output(GPIO_LCD_VDD, 1);
	gpio_direction_output(GPIO_LCD_EN, 1);

	return 0;
}

static struct gsgpu_backlight
*gsgpu_backlight_init(struct gsgpu_device *adev, int index)
{
	struct gsgpu_backlight *ls_bl = NULL;
	struct backlight_properties props;
	struct pwm_resource *pwm_res;
	int ret = 0;

	ls_bl = kzalloc(sizeof(struct gsgpu_backlight), GFP_KERNEL);
	if (IS_ERR(ls_bl)) {
		DRM_ERROR("Failed to alloc backlight.\n");
		return NULL;
	}

	ls_bl->min = BL_MIN_LEVEL;
	ls_bl->max = BL_MAX_LEVEL;
	ls_bl->level = BL_DEF_LEVEL;
	ls_bl->driver_private = adev;
	ls_bl->display_pipe_index = index;
	ls_bl->get_brightness = gsgpu_backlight_get;
	ls_bl->set_brightness = gsgpu_backlight_set;
	ls_bl->enable = gsgpu_backlight_enable;
	ls_bl->disable = gsgpu_backlight_disable;
	ls_bl->power = gsgpu_backlight_power;

	pwm_res = dc_get_vbios_resource(adev->dc->vbios,
					index, GSGPU_RESOURCE_PWM);
	ls_bl->pwm_id = pwm_res->pwm;
	/* 0:low start, 1:high start */
	ls_bl->pwm_polarity = pwm_res->polarity;
	ls_bl->pwm_period = pwm_res->period;

	DRM_INFO("pwm: id=%d, period=%dns, polarity=%d.\n",
		 ls_bl->pwm_id, ls_bl->pwm_period, ls_bl->pwm_polarity);

	ret = gsgpu_backlight_hw_request_init(adev, ls_bl);
	if (ret)
		goto ERROR_HW;

	memset(&props, 0, sizeof(props));
	props.type = BACKLIGHT_RAW;
	props.power = FB_BLANK_UNBLANK;
	props.max_brightness = ls_bl->max;
	props.brightness = ls_bl->level;

	ls_bl->device = backlight_device_register("gsgpu-bl",
			adev->mode_info.connectors[index]->base.kdev,
			ls_bl, &gsgpu_backlight_ops, &props);
	if (IS_ERR(ls_bl->device)) {
		DRM_ERROR("Failed to register backlight.\n");
		goto ERROR_REG;
	}

	DRM_INFO("register gsgpu backlight_%d completed.\n", index);

	adev->mode_info.backlights[index] = ls_bl;

	return ls_bl;

ERROR_HW:
ERROR_REG:
	kfree(ls_bl);

	return NULL;
}

int gsgpu_backlight_register(struct drm_connector *connector)
{
	struct gsgpu_backlight *ls_bl;
	struct gsgpu_device *adev = connector->dev->dev_private;
	int ret = 0;
	bool ls_bl_status = false;

	switch (connector->connector_type) {
	case DRM_MODE_CONNECTOR_eDP:
	case DRM_MODE_CONNECTOR_LVDS:
		break;
	default:
		return ret;
	}

	ls_bl = gsgpu_backlight_init(adev, connector->index);
	if (!ls_bl)
		return -ENXIO;

	ls_bl_status = gsgpu_backlight_get_hw_status(ls_bl);
	if (ls_bl_status) {
		ls_bl->hw_enabled = true;
		ls_bl->power(ls_bl, true);
	} else {
		ls_bl->hw_enabled = false;
		ls_bl->power(ls_bl, false);
	}

	DRM_INFO("backlight power status: %s->%s.\n",
		 ls_bl_status ? "on" : "off",
		 ls_bl->hw_enabled ? "on" : "off");

	return ret;
}
