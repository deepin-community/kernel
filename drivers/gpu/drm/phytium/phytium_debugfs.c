// SPDX-License-Identifier: GPL-2.0
/* Phytium X100 display drm driver
 *
 * Copyright (c) 2021 Phytium Limited.
 *
 * Author:
 *	Yang Xun <yangxun@phytium.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/debugfs.h>
#include <linux/fs.h>

#include "phytium_display_drv.h"
#include "phytium_dp.h"
#include "phytium_reg.h"

static ssize_t
phytium_dp_register_write(struct file *filp,
				 const char __user *ubuf,
				 size_t len,
				 loff_t *ppos)
{
	char tmp[16];

	if (len >= sizeof(tmp))
		return -EINVAL;

	memset(tmp, 0, sizeof(tmp));
	if (copy_from_user(tmp, ubuf, len))
		return -EFAULT;
	tmp[len] = '\0';

	return len;
}

static int phytium_dp_register_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct phytium_dp_device *phytium_dp = connector_to_dp_device(connector);
	struct drm_device *dev =  phytium_dp->dev;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int port = phytium_dp->port;

	seq_printf(m, "addr:h0x%08x	h0x%08x\n", PHYTIUM_DP_M_VID(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_M_VID(port)));
	seq_printf(m, "addr:h0x%08x	h0x%08x\n", PHYTIUM_DP_N_VID(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_N_VID(port)));
	seq_printf(m, "addr:h0x%08x	h0x%08x\n", PHYTIUM_DP_TRANSFER_UNIT_SIZE(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_TRANSFER_UNIT_SIZE(port)));
	seq_printf(m, "addr:h0x%08x	h0x%08x\n", PHYTIUM_DP_DATA_COUNT(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_DATA_COUNT(port)));
	seq_printf(m, "addr:h0x%08x	h0x%08x\n", PHYTIUM_DP_MAIN_LINK_HTOTAL(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_MAIN_LINK_HTOTAL(port)));
	seq_printf(m, "addr:h0x%08x	h0x%08x\n", PHYTIUM_DP_MAIN_LINK_HRES(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_MAIN_LINK_HRES(port)));
	seq_printf(m, "addr:h0x%08x	h0x%08x\n", PHYTIUM_DP_MAIN_LINK_HSWIDTH(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_MAIN_LINK_HSWIDTH(port)));
	seq_printf(m, "addr:h0x%08x	h0x%08x\n", PHYTIUM_DP_MAIN_LINK_HSTART(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_MAIN_LINK_HSTART(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_MAIN_LINK_VTOTAL(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_MAIN_LINK_VTOTAL(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_MAIN_LINK_VRES(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_MAIN_LINK_VRES(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_MAIN_LINK_VSWIDTH(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_MAIN_LINK_VSWIDTH(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_MAIN_LINK_VSTART(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_MAIN_LINK_VSTART(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_MAIN_LINK_POLARITY(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_MAIN_LINK_POLARITY(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_MAIN_LINK_MISC0(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_MAIN_LINK_MISC0(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_MAIN_LINK_MISC1(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_MAIN_LINK_MISC1(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_USER_SYNC_POLARITY(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_USER_SYNC_POLARITY(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_VIDEO_STREAM_ENABLE(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_VIDEO_STREAM_ENABLE(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SECONDARY_STREAM_ENABLE(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SECONDARY_STREAM_ENABLE(port)));
	seq_puts(m, "audio:\n");
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SEC_INPUT_SELECT(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SEC_INPUT_SELECT(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SEC_DIRECT_CLKDIV(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SEC_DIRECT_CLKDIV(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SEC_CHANNEL_COUNT(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SEC_CHANNEL_COUNT(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SEC_CHANNEL_MAP(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SEC_CHANNEL_MAP(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SEC_DATA_WINDOW(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SEC_DATA_WINDOW(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SEC_CS_CATEGORY_CODE(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SEC_CS_CATEGORY_CODE(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SEC_MAUD(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SEC_MAUD(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SEC_NAUD(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SEC_NAUD(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SEC_CLOCK_MODE(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SEC_CLOCK_MODE(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SEC_CS_SOURCE_FORMAT(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SEC_CS_SOURCE_FORMAT(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SEC_CS_LENGTH_ORIG_FREQ(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SEC_CS_LENGTH_ORIG_FREQ(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SEC_CS_FREQ_CLOCK_ACCURACY(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SEC_CS_FREQ_CLOCK_ACCURACY(port)));
	seq_printf(m, "addr:h'0x%08x	h'0x%08x\n", PHYTIUM_DP_SEC_AUDIO_ENABLE(port),
		   phytium_readl_reg(priv, PHYTIUM_DP_SEC_AUDIO_ENABLE(port)));

	return 0;
}

static int phytium_dp_register_open(struct inode *inode, struct file *file)
{
	return single_open(file, phytium_dp_register_show, inode->i_private);
}

static const struct file_operations phytium_dp_register_fops = {
	.owner = THIS_MODULE,
	.open = phytium_dp_register_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = phytium_dp_register_write,
};

static ssize_t
phytium_dp_trigger_train_fail_write(struct file *filp,
				 const char __user *ubuf,
				 size_t len,
				 loff_t *ppos)
{
	struct seq_file *m = filp->private_data;
	struct drm_connector *connector = m->private;
	struct phytium_dp_device *phytium_dp = connector_to_dp_device(connector);
	char tmp[16];

	if (len >= sizeof(tmp))
		return -EINVAL;

	memset(tmp, 0, sizeof(tmp));
	if (copy_from_user(tmp, ubuf, len))
		return -EFAULT;
	tmp[len] = '\0';

	if (kstrtouint(tmp, 10, &phytium_dp->trigger_train_fail) != 0)
		return -EINVAL;

	return len;
}

static int phytium_dp_trigger_train_fail_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct phytium_dp_device *phytium_dp = connector_to_dp_device(connector);

	seq_printf(m, "trigger_train_fail: %d\n", phytium_dp->trigger_train_fail);
	seq_printf(m, "train_retry_count: %d\n", phytium_dp->train_retry_count);

	return 0;
}

static int phytium_dp_trigger_train_fail_open(struct inode *inode, struct file *file)
{
	return single_open(file, phytium_dp_trigger_train_fail_show, inode->i_private);
}

static const struct file_operations phytium_dp_trigger_train_fail_fops = {
	.owner = THIS_MODULE,
	.open = phytium_dp_trigger_train_fail_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = phytium_dp_trigger_train_fail_write,
};

static int phytium_edp_backlight_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct phytium_dp_device *phytium_dp = connector_to_dp_device(connector);

	if (!phytium_dp->is_edp)
		return -ENODEV;

	mutex_lock(&phytium_dp->panel.panel_lock);
	seq_printf(m, "backlight: %s\n", phytium_dp->panel.backlight_enabled?"enabled":"disabled");
	mutex_unlock(&phytium_dp->panel.panel_lock);

	return 0;
}

static int phytium_edp_backlight_open(struct inode *inode, struct file *file)
{
	return single_open(file, phytium_edp_backlight_show, inode->i_private);
}

static const struct file_operations phytium_edp_backlight_fops = {
	.owner = THIS_MODULE,
	.open = phytium_edp_backlight_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int phytium_edp_power_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct phytium_dp_device *phytium_dp = connector_to_dp_device(connector);

	if (!phytium_dp->is_edp)
		return -ENODEV;

	mutex_lock(&phytium_dp->panel.panel_lock);
	seq_printf(m, "power: %s\n", phytium_dp->panel.power_enabled?"enabled":"disabled");
	mutex_unlock(&phytium_dp->panel.panel_lock);

	return 0;
}

static int phytium_edp_power_open(struct inode *inode, struct file *file)
{
	return single_open(file, phytium_edp_power_show, inode->i_private);
}

static const struct file_operations phytium_edp_power_fops = {
	.owner = THIS_MODULE,
	.open = phytium_edp_power_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

struct dpcd_block {
	/* DPCD dump start address. */
	unsigned int offset;
	/* DPCD dump end address, inclusive. If unset, .size will be used. */
	unsigned int end;
	/* DPCD dump size. Used if .end is unset. If unset, defaults to 1. */
	size_t size;
	/* Only valid for eDP. */
	bool edp;
};

static const struct dpcd_block phytium_dpcd_debug[] = {
	{ .offset = DP_DPCD_REV, .size = DP_RECEIVER_CAP_SIZE },
	{ .offset = DP_PSR_SUPPORT, .end = DP_PSR_CAPS },
	{ .offset = DP_DOWNSTREAM_PORT_0, .size = 16 },
	{ .offset = DP_LINK_BW_SET, .end = DP_EDP_CONFIGURATION_SET },
	{ .offset = DP_SINK_COUNT, .end = DP_ADJUST_REQUEST_LANE2_3 },
	{ .offset = DP_SET_POWER },
	{ .offset = DP_EDP_DPCD_REV },
	{ .offset = DP_EDP_GENERAL_CAP_1, .end = DP_EDP_GENERAL_CAP_3 },
	{ .offset = DP_EDP_DISPLAY_CONTROL_REGISTER, .end = DP_EDP_BACKLIGHT_FREQ_CAP_MAX_LSB },
	{ .offset = DP_EDP_DBC_MINIMUM_BRIGHTNESS_SET, .end = DP_EDP_DBC_MAXIMUM_BRIGHTNESS_SET },
	{ .offset = DP_DEVICE_SERVICE_IRQ_VECTOR, .size = 1 },
	{ .offset = DP_TEST_REQUEST, .end = DP_TEST_PATTERN },
};

static int phytium_dpcd_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct phytium_dp_device *phytium_dp = connector_to_dp_device(connector);
	uint8_t buf[16], i;
	ssize_t err;

	if (connector->status != connector_status_connected)
		return -ENODEV;

	for (i = 0; i < ARRAY_SIZE(phytium_dpcd_debug); i++) {
		const struct dpcd_block *b = &phytium_dpcd_debug[i];
		size_t size = b->end ? b->end - b->offset + 1 : (b->size ?: 1);

		if (WARN_ON(size > sizeof(buf)))
			continue;

		err = drm_dp_dpcd_read(&phytium_dp->aux, b->offset, buf, size);
		if (err <= 0) {
			DRM_ERROR("dpcd read (%zu bytes at %u) failed (%zd)\n",
				   size, b->offset, err);
			continue;
		}

		seq_printf(m, "%04x: %*ph\n", b->offset, (int) size, buf);
	}

	return 0;
}

static int phytium_dpcd_open(struct inode *inode, struct file *file)
{
	return single_open(file, phytium_dpcd_show, inode->i_private);
}

static const struct file_operations phytium_dpcd_fops = {
	.owner = THIS_MODULE,
	.open = phytium_dpcd_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t
phytium_dp_state_write(struct file *filp,
				 const char __user *ubuf,
				 size_t len,
				 loff_t *ppos)
{
	char tmp[16];

	if (len >= sizeof(tmp))
		return -EINVAL;

	memset(tmp, 0, sizeof(tmp));
	if (copy_from_user(tmp, ubuf, len))
		return -EFAULT;
	tmp[len] = '\0';

	return len;
}

static int phytium_dp_state_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct phytium_dp_device *phytium_dp = connector_to_dp_device(connector);

	seq_printf(m, "port number: %d\n", phytium_dp->port);
	seq_printf(m, "source_max_lane_count: %d\n", phytium_dp->source_max_lane_count);
	seq_printf(m, "max_source_rates: %d\n",
		   phytium_dp->source_rates[phytium_dp->num_source_rates-1]);
	if (connector->status == connector_status_connected) {
		seq_printf(m, "sink_max_lane_count: %d\n", phytium_dp->sink_max_lane_count);
		seq_printf(m, "max_sink_rates: %d\n",
			   phytium_dp->sink_rates[phytium_dp->num_sink_rates-1]);
		seq_printf(m, "link_rate: %d\n", phytium_dp->link_rate);
		seq_printf(m, "link_lane_count: %d\n", phytium_dp->link_lane_count);
		seq_printf(m, "train_set[0]: %d\n", phytium_dp->train_set[0]);
		seq_printf(m, "has_audio: %s\n", phytium_dp->has_audio?"yes":"no");
	}

	return 0;
}

static int phytium_dp_state_open(struct inode *inode, struct file *file)
{
	return single_open(file, phytium_dp_state_show, inode->i_private);
}

static const struct file_operations phytium_dp_state_fops = {
	.owner = THIS_MODULE,
	.open = phytium_dp_state_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = phytium_dp_state_write,
};

static const struct phytium_debugfs_connector_files {
	const char *name;
	const struct file_operations *fops;
} phytium_debugfs_connector_files[] = {
	{"dp_state", &phytium_dp_state_fops},
	{"dpcd", &phytium_dpcd_fops},
	{"dp_register", &phytium_dp_register_fops},
	{"dp_trigger_train_fail", &phytium_dp_trigger_train_fail_fops},
};

static const struct phytium_debugfs_connector_files phytium_edp_debugfs_connector_files[] = {
	{"edp_power", &phytium_edp_power_fops},
	{"edp_backlight", &phytium_edp_backlight_fops},
};

int phytium_debugfs_connector_add(struct drm_connector *connector)
{
	struct dentry *root = connector->debugfs_entry;
	struct dentry *ent;
	int i;
	struct phytium_dp_device *phytium_dp = connector_to_dp_device(connector);

	if (!root)
		return -ENODEV;

	for (i = 0; i < ARRAY_SIZE(phytium_debugfs_connector_files); i++) {
		ent = debugfs_create_file(phytium_debugfs_connector_files[i].name,
					  0644,
					  root,
					  connector,
					  phytium_debugfs_connector_files[i].fops);
		if (!ent)
			return -ENOMEM;
	}

	if (phytium_dp->is_edp)
		for (i = 0; i < ARRAY_SIZE(phytium_edp_debugfs_connector_files); i++) {
			ent = debugfs_create_file(phytium_edp_debugfs_connector_files[i].name,
						  0644,
						  root,
						  connector,
						  phytium_edp_debugfs_connector_files[i].fops);
			if (!ent)
				return -ENOMEM;
		}

	return 0;
}
