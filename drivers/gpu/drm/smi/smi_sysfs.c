// SPDX-License-Identifier: GPL-2.0-only
/*
 * smi_sysfs.c
 *
 * Copyright (c) 2020 
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/pci.h>
#include <drm/drm_print.h>
#include "smi_drv.h"
#include "ddk768/ddk768_pwm.h"

extern int pwm_ctrl;
extern int smi_debug;
extern int force_connect;

static ssize_t pwm_ctrl_store(__always_unused struct device *dev,
			      __always_unused struct device_attribute *attr,
			      const char *buf, size_t count)
{
	unsigned int ctrl = pwm_ctrl;
	unsigned int val;


	if (kstrtouint(buf, 10, &val)) {
		pr_err("Unable to parse %u\n", val);
		return -EINVAL;
	}

	if(val == ctrl) {
		pr_info("Nothing to do with %u\n", val);
		return count;
	} 

	if(ctrl) {
		if((val & 0xf) != (ctrl & 0xf)) {
			ddk768_pwmClose(ctrl & 0xf);
		} 
	
		if((val & ~0xf) != (ctrl & ~0xf)) {
			ddk768_pwmStop(ctrl & 0xf);
		}
	}

	if (val) {
		unsigned long pwm, divider, highCounter, lowCounter;

		pwm = val & 0xf;
		divider = (val & 0xf0) >> 4;
		highCounter = (val & 0xfff00) >> 8;
		lowCounter = (val & 0xfff00000) >> 20;

		if((val & 0xf) != (ctrl & 0xf)) {
			ddk768_pwmOpen(pwm);
		}

		if((val & ~0xf) != (ctrl & ~0xf)) {
			ddk768_pwmStart(pwm, divider, highCounter, lowCounter, 0);
		}
	}

	pwm_ctrl = val;
	pr_info("Setting pwm_ctrl to %u\n", val);
	return count;
}

static ssize_t pwm_ctrl_show(__always_unused struct device *dev,
			     __always_unused struct device_attribute *attr,
			     char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", pwm_ctrl);
}

static ssize_t smi_debug_store(__always_unused struct device *dev,
			      __always_unused struct device_attribute *attr,
			      const char *buf, size_t count) 
{
	unsigned int val;

	if (kstrtouint(buf, 10, &val)) {
		pr_err("Unable to parse %u\n", val);
		return -EINVAL;
	}
	

	pr_info("Setting smi_debug to %u\n", val);
	smi_debug = val;
	return count;
}

static ssize_t smi_debug_show(__always_unused struct device *dev,
			     __always_unused struct device_attribute *attr,
			     char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", smi_debug);
}

static ssize_t nopnp_store(__always_unused struct device *dev,
			     __always_unused struct device_attribute *attr,
			     const char *buf, size_t count)
{
	unsigned int val;

	if (kstrtouint(buf, 10, &val)) {
		pr_err("Unable to parse %u\n", val);
		return -EINVAL;
	}
	

	pr_info("Setting force_connect to %u\n", val);
	force_connect = val;
	return count;
}

static ssize_t nopnp_show(__always_unused struct device *dev,
			     __always_unused struct device_attribute *attr,
			     char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", force_connect);
}

static ssize_t version_show(__always_unused struct device *dev,
			     __always_unused struct device_attribute *attr,
			     char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u.%u.%u\n", DRIVER_MAJOR,
				DRIVER_MINOR, DRIVER_PATCHLEVEL);
}

// pwm: pwm_ctrl
// debug: smi_debug
// nopnp: force_connect

static struct device_attribute smi_device_attributes[] = {
	__ATTR_RW(pwm_ctrl),
	__ATTR_RW(smi_debug),
	__ATTR_RW(nopnp),
	__ATTR_RO(version)
};

void smi_sysfs_init(struct kobject *obj)
{
	int i;
	int ret;

	if (PTR_ERR_OR_ZERO(obj)) {
		pr_err("SMI obj is NULL\n");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(smi_device_attributes); i++) {
		if((ret = sysfs_create_file(obj, &smi_device_attributes[i].attr)) < 0) {
			pr_err("Create SMI sysfs failed: %d\n", ret);
		}
	}

}

void smi_sysfs_deinit(struct kobject *obj)
{
        int i;
        if (PTR_ERR_OR_ZERO(obj))
                return;

        for (i = 0; i < ARRAY_SIZE(smi_device_attributes); i++)
                sysfs_remove_file(obj, &smi_device_attributes[i].attr);

}

