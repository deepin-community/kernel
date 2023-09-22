// SPDX-License-Identifier: GPL-2.0+
#ifndef __GSGPU_BACKLIGHT_H__
#define __GSGPU_BACKLIGHT_H__

#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/pwm.h>

#define BL_MAX_LEVEL 100
#define BL_MIN_LEVEL 1
#define BL_DEF_LEVEL 60
#define GPIO_LCD_EN 62
#define GPIO_LCD_VDD 63

struct gsgpu_backlight {
	void *driver_private;
	struct backlight_device *device;
	struct pwm_device *pwm;
	int display_pipe_index;
	u32 pwm_id;
	u32 pwm_polarity;
	u32 pwm_period;
	bool hw_enabled;
	u32 level;
	u32 max;
	u32 min;

	unsigned int (*get_brightness)(struct gsgpu_backlight *ls_bl);
	void (*set_brightness)(struct gsgpu_backlight *ls_bl,
			       unsigned int level);
	void (*enable)(struct gsgpu_backlight *ls_bl);
	void (*disable)(struct gsgpu_backlight *ls_bl);
	void (*power)(struct gsgpu_backlight *ls_bl, bool enable);
};

#define BACKLIGHT_DEFAULT_METHOD_CLOSE(ls_bl)\
	do { \
		ls_bl->hw_enabled = false;\
		gpio_set_value(GPIO_LCD_EN, 0);\
		msleep(10);\
		pwm_disable(ls_bl->pwm); \
	} while (0)

#define BACKLIGHT_DEFAULT_METHOD_FORCE_CLOSE(ls_bl)\
	do { \
		ls_bl->hw_enabled = false;\
		BACKLIGHT_DEFAULT_METHOD_CLOSE(ls_bl);\
		msleep(160);\
		gpio_set_value(GPIO_LCD_VDD, 0); \
	} while (0)

#define BACKLIGHT_DEFAULT_METHOD_OPEN(ls_bl)\
	do { \
		pwm_enable(ls_bl->pwm);\
		msleep(10);\
		gpio_set_value(GPIO_LCD_EN, 1);\
		ls_bl->hw_enabled = true; \
	} while (0)

#define BACKLIGHT_DEFAULT_METHOD_FORCE_OPEN(ls_bl)\
	do {\
		gpio_set_value(GPIO_LCD_VDD, 1);\
		msleep(160);\
		BACKLIGHT_DEFAULT_METHOD_OPEN(ls_bl);\
		ls_bl->hw_enabled = true; \
	} while (0)

int gsgpu_backlight_register(struct drm_connector *connector);

#endif /* __GSGPU_BACKLIGHT_H__ */
