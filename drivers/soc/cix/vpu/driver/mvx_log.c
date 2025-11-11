// SPDX-License-Identifier: GPL-2.0-only
/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/******************************************************************************
 * Includes
 ******************************************************************************/

#include "mvx_log.h"
#include "mvx_log_ram.h"

#include <linux/uaccess.h>
#include <linux/aio.h>
#include <linux/debugfs.h>
#include <linux/dcache.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/namei.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/un.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

/******************************************************************************
 * External functions
 ******************************************************************************/
void mvx_sched_get_realtime_fps(struct list_head *sessions);

/******************************************************************************
 * Defines
 ******************************************************************************/

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif /* UNUSED */

/******************************************************************************
 * Types
 ******************************************************************************/

/******************************************************************************
 * Variables
 ******************************************************************************/

#ifdef MVX_LOG_FTRACE_ENABLE

/**
 * Map severity to string.
 */
static const char *const severity_to_name[] = {
	"Panic",
	"Error",
	"Warning",
	"Info",
	"Debug",
	"Verbose"
};
#endif /* MVX_LOG_FTRACE_ENABLE */

/**
 * Map severity to kernel log level.
 */
static const char *const severity_to_kern_level[] = {
	KERN_EMERG,
	KERN_ERR,
	KERN_WARNING,
	KERN_NOTICE,
	KERN_INFO,
	KERN_DEBUG
};

void mvx_log_get_util(struct timer_list *timer);

/******************************************************************************
 * Static functions
 ******************************************************************************/

/******************************************************************************
 * Log
 *
 * Directory                    i_node->i_private
 * --------------------------------------------------------
 * mvx                          struct mvx_log *
 * +-- group
 * |   +-- <group>              struct mvx_log_group *
 * |       +-- severity
 * |       +-- drain
 * +-- drain
 *     +-- <drain>              struct mvx_log_drain *
 *
 ******************************************************************************/

/**
 * trim() - Trim of trailing new line.
 * @str:    Pointer to string.
 */
static void trim(char *str)
{
	size_t len = strlen(str);

	while (len-- > 0) {
		if (str[len] != '\n')
			break;

		str[len] = '\0';
	}
}

/**
 * lookup() - Search for child dentry with matching name.
 * @parent:    Pointer to parent dentry.
 * @name:    Name of dentry to look for.
 *
 * Return: Pointer to dentry, NULL if not found.
 */
static struct dentry *lookup(struct dentry *parent, const char *name)
{
	struct dentry *child;

	/* Loop over directory entries in mvx/drain/. */
	list_for_each_entry(child, &parent->d_subdirs, d_child) {
		if (strcmp(name, child->d_name.name) == 0)
			return child;
	}

	return NULL;
}

/**
 * get_inode_private() - Get inode private member of parent directory.
 * @file:        File pointer.
 * @parent:        Number of parent directories.
 *
 * Return: Inode private member, or NULL on error.
 */
static void *get_inode_private(struct file *file, int parent)
{
	struct dentry *d = file->f_path.dentry;

	while (d != NULL && parent-- > 0)
		d = d->d_parent;

	if (d == NULL || d->d_inode == NULL)
		return NULL;

	return d->d_inode->i_private;
}

/**
 * readme_read() - Read handle function for mvx/group/<group>/drain. The
 *           function returns the usage instruction message.
 * @file:        File pointer.
 * @user_buffer:    The user space buffer that is read to.
 * @count:        The maximum number of bytes to read.
 * @position:        The current position in the buffer.
 */
static ssize_t readme_read(struct file *file,
			   char __user *user_buffer,
			   size_t count, loff_t *position)
{
	static const char msg[] =
	    "LOG GROUPS\n"
	    "\n"
	    "The available log groups can be found under 'group'.\n"
	    "$ ls group\n"
	    "\n"
	    "SEVERITY LEVELS\n"
	    "    0 - Panic\n"
	    "    1 - Error\n"
	    "    2 - Warning\n"
	    "    3 - Info\n"
	    "    4 - Debug\n"
	    "    5 - Verbose\n"
	    "\n"
	    "The severity level for a log group can be read and set at runtime.\n"
	    "$ cat group/general/severity\n"
	    "$ echo 3 > group/general/severity\n";

	return simple_read_from_buffer(user_buffer, count, position, msg,
				       sizeof(msg));
}

/**
 * group_util_read() - Read handle function for mvx/group/<group>/utilization. The
 *            function returns current VPU utilization.
 * @file:        File pointer.
 * @user_buffer:    The user space buffer that is read to.
 * @count:        The maximum number of bytes to read.
 * @position:        The current position in the buffer.
 */
static ssize_t group_util_read(struct file *file,
			       char __user *user_buffer,
			       size_t count, loff_t *position)
{
	/* File path mvx/group/<group>/drain. */
	struct mvx_log_group *group = get_inode_private(file, 1);
	char name[64];
	size_t len;

#if !MVX_USE_UTILIZATION_TIMER
	mvx_log_get_util(NULL);
#endif
	if (group->enabled & MVX_LOG_PERF_UTILIZATION)
		len =
		    scnprintf(name, sizeof(name),
			      "VPU Utilization: %d.%02d%%\n",
			      group->utilization / 100,
			      group->utilization % 100);
	else
		len =
		    scnprintf(name, sizeof(name),
			      "VPU Performance Monitor is OFF\n");

	return simple_read_from_buffer(user_buffer, count, position, name, len);
}

/**
 * group_avgfps_read() - Read handle function for mvx/group/<group>/avgfps. The
 *            function returns average fps of sessions.
 * @file:        File pointer.
 * @user_buffer:    The user space buffer that is read to.
 * @count:        The maximum number of bytes to read.
 * @position:        The current position in the buffer.
 */
static ssize_t group_avgfps_read(struct file *file,
				 char __user *user_buffer,
				 size_t count, loff_t *position)
{
	/* File path mvx/group/<group>/average_fps. */
	struct mvx_log_group *group = get_inode_private(file, 1);
	char *cache = group->avgfps + MVX_LOG_FPS_MSG_BUF_SIZE;
	size_t len = 0;

	mutex_lock(&group->mutex);
	if (group->enabled & MVX_LOG_PERF_FPS) {
		if (group->has_update) {
			int i;
			int num = MVX_LOG_FPS_MSG_UNITS;
			int offset =
			    group->fps_msg_w * MVX_LOG_FPS_MSG_UNIT_SIZE;
			char *start = group->avgfps + offset;

			if (start[0] == 0) {
				num = group->fps_msg_w;
				offset = 0;
			}
			for (i = 0; i < num; i++) {
				len +=
				    scnprintf(cache + len,
					      MVX_LOG_FPS_MSG_UNIT_SIZE, "%s",
					      group->avgfps + offset);
				offset += MVX_LOG_FPS_MSG_UNIT_SIZE;
				if (offset == MVX_LOG_FPS_MSG_BUF_SIZE)
					offset = 0;
			}
			group->has_update = false;
		} else {
			len = MVX_LOG_FPS_MSG_BUF_SIZE;
		}
	} else {
		len =
		    scnprintf(cache, MVX_LOG_FPS_MSG_UNIT_SIZE,
			      "VPU fps stats is OFF\n");
	}
	mutex_unlock(&group->mutex);

	return simple_read_from_buffer(user_buffer, count, position, cache,
				       len);
}

/**
 * group_avgfps_read() - Read handle function for mvx/group/<group>/rtfps. The
 *            function returns realtime fps of sessions.
 * @file:        File pointer.
 * @user_buffer:    The user space buffer that is read to.
 * @count:        The maximum number of bytes to read.
 * @position:        The current position in the buffer.
 */
static ssize_t group_rtfps_read(struct file *file,
				char __user *user_buffer,
				size_t count, loff_t *position)
{
	/* File path mvx/group/<group>/realtime_fps. */
	struct mvx_log_group *group = get_inode_private(file, 1);
	char *cache = group->rtfps + MVX_LOG_FPS_MSG_BUF_SIZE;
	size_t len = 0;

	if (group->enabled & MVX_LOG_PERF_FPS) {
		int i;
		int offset = 0;

		group->rtfps_num = 0;
		mvx_sched_get_realtime_fps(group->sessions);
		group->rtfps_num = min(group->rtfps_num, MVX_LOG_FPS_MSG_UNITS);
		for (i = 0; i < group->rtfps_num; i++) {
			len += scnprintf(cache + len, MVX_LOG_FPS_MSG_UNIT_SIZE,
					 "%s", group->rtfps + offset);
			offset += MVX_LOG_FPS_MSG_UNIT_SIZE;
		}
	} else {
		len =
		    scnprintf(cache, MVX_LOG_FPS_MSG_UNIT_SIZE,
			      "VPU fps stats is OFF\n");
	}

	return simple_read_from_buffer(user_buffer, count, position, cache,
				       len);
}

/**
 * group_status_read() - Read handle function for mvx/group/<group>/enable. The
 *            function returns VPU performance monitor status.
 * @file:        File pointer.
 * @user_buffer:    The user space buffer that is read to.
 * @count:        The maximum number of bytes to read.
 * @position:        The current position in the buffer.
 */
static ssize_t group_status_read(struct file *file,
				 char __user *user_buffer,
				 size_t count, loff_t *position)
{
	/* File path mvx/group/<group>/enable. */
	struct mvx_log_group *group = get_inode_private(file, 1);
	char buf[16];
	size_t len;

	len = scnprintf(buf, sizeof(buf), "%d\n", group->enabled);
	return simple_read_from_buffer(user_buffer, count, position, buf, len);
}

/**
 * group_status_write() - Write handle function for mvx/group/<group>/enable. The
 *            function returns VPU performance monitor status.
 * @file:        File pointer.
 * @user_buffer:    The user space buffer that is written to.
 * @count:        The maximum number of bytes to write.
 * @position:        The current position in the buffer.
 */
static ssize_t group_status_write(struct file *file,
				const char __user *user_buffer,
				size_t count, loff_t *position)
{
	/* File path mvx/group/<group>/enable. */
	struct mvx_log_group *group = get_inode_private(file, 1);
	char enable_str[4];
	int enabled;
	ssize_t size;

	/* Check that input is not larger that path buffer. */
	if (count < 1) {
		pr_err("MVX: Invalid data.\n");
		return -EINVAL;
	}

	size = simple_write_to_buffer(enable_str, sizeof(enable_str) - 1,
				      position, user_buffer, count);
	enabled = simple_strtol(enable_str, NULL, 0) & MVX_LOG_PERF_ALL;
	if ((enabled & MVX_LOG_PERF_UTILIZATION) &&
	    !(group->enabled & MVX_LOG_PERF_UTILIZATION)) {
		if (group->drain->reset)
			group->drain->reset(group->drain);
	}

	if ((enabled & MVX_LOG_PERF_FPS)
	    && !(group->enabled & MVX_LOG_PERF_FPS)) {
		int size = MVX_LOG_FPS_MSG_BUF_SIZE * 2;

		if (group->avgfps)
			memset(group->avgfps, 0, size);
		if (group->rtfps)
			memset(group->rtfps, 0, size);
		group->fps_msg_w = 0;
	}

	group->enabled = enabled;

	return size;
}

/**
 * group_drain_read() - Read handle function for mvx/group/<group>/drain. The
 *            function returns the name of the currently configured
 *            drain.
 * @file:        File pointer.
 * @user_buffer:    The user space buffer that is read to.
 * @count:        The maximum number of bytes to read.
 * @position:        The current position in the buffer.
 */
static ssize_t group_drain_read(struct file *file,
				char __user *user_buffer,
				size_t count, loff_t *position)
{
	/* File path mvx/group/<group>/drain. */
	struct mvx_log_group *group = get_inode_private(file, 1);
	struct mvx_log_drain *drain = group->drain;
	char name[100];
	size_t len;

	if (drain == NULL || drain->dentry == NULL) {
		pr_err("MVX: No drain assigned to log group.\n");
		return -EINVAL;
	}

	len = scnprintf(name, sizeof(name), "%s\n", drain->dentry->d_name.name);

	return simple_read_from_buffer(user_buffer, count, position, name, len);
}

/**
 * group_drain_write() - Write handle function for mvx/group/<group>/drain. The
 *             function sets the drain for the group. If the drain
 *             does not match any registered drain, then error is
 *             returned to user space.
 * @file:        File pointer.
 * @user_buffer:    The user space buffer that is written to.
 * @count:        The maximum number of bytes to write.
 * @position:        The current position in the buffer.
 */
static ssize_t group_drain_write(struct file *file,
				 const char __user *user_buffer,
				 size_t count, loff_t *position)
{
	/* File path mvx/group/<group>/drain. */
	struct mvx_log_group *group = get_inode_private(file, 1);
	struct mvx_log *log = get_inode_private(file, 3);
	struct dentry *dentry;
	char drain_str[100];
	ssize_t size;

	/* Check that input is not larger that path buffer. */
	if (count > (sizeof(drain_str) - 1)) {
		pr_err("MVX: Input overflow.\n");

		return -EINVAL;
	}

	/* Append input to path. */
	size = simple_write_to_buffer(drain_str, sizeof(drain_str) - 1,
				      position, user_buffer, count);
	drain_str[count] = '\0';
	trim(drain_str);

	dentry = lookup(log->drain_dir, drain_str);

	if (IS_ERR_OR_NULL(dentry)) {
		pr_warn("MVX: No drain matching '%s'.\n", drain_str);
		return -EINVAL;
	}

	/* Assign drain to log group. */
	group->drain = dentry->d_inode->i_private;

	return size;
}

/**
 * drain_ram_read() - Read the RAM buffer.
 * @drain:        The RAM buffer drain.
 * @user_buffer:    The user space buffer that is read to.
 * @count:        The maximum number of bytes to read.
 * @position:        The current position in the buffer.
 * @pos:        The last used position of the drain buffer
 */
static ssize_t drain_ram_read(struct mvx_log_drain_ram *drain,
			      char __user *user_buffer,
			      size_t count, loff_t *position, size_t pos)
{
	ssize_t n = 0;

	/* Make sure position is not beyond end of file. */
	if (*position > pos)
		return -EINVAL;

	/* If position is more than BUFFER_SIZE bytes behind, then fast forward
	 * to current position minus BUFFER_SIZE.
	 */
	if ((pos - *position) > drain->buffer_size)
		*position = pos - drain->buffer_size;

	/* Copy data to user space. */
	while ((n < count) && (*position < pos)) {
		size_t offset;
		size_t length;

		/* Offset in circular buffer. */
		offset = *position & (drain->buffer_size - 1);

		/* Available number of bytes. */
		length = min((size_t)(pos - *position), count - n);

		/* Make sure length does not go beyond end of circular buffer.
		 */
		length = min(length, drain->buffer_size - offset);

		/* Copy data from kernel- to user space. */
		length -= copy_to_user(&user_buffer[n], &drain->buf[offset],
				       length);

		/* No bytes were copied. Return error. */
		if (length == 0)
			return -EINVAL;

		*position += length;
		n += length;
	}

	return n;
}

/**
 * drain_ram_read_msg() - Read of the RAM file.
 * @file:        File pointer.
 * @user_buffer:    The user space buffer that is read to.
 * @count:        The maximum number of bytes to read.
 * @position:        The current position in the buffer.
 */
static ssize_t drain_ram_read_msg(struct file *file,
				  char __user *user_buffer,
				  size_t count, loff_t *position)
{
	struct mvx_log_drain_ram *drain = get_inode_private(file, 1);

	while (*position == drain->write_pos) {
		int ret;

		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		/* Block until there is data available. */
		ret = wait_event_interruptible(drain->queue,
					       *position < drain->write_pos);
		if (ret != 0)
			return -EINTR;
	}

	return drain_ram_read(drain, user_buffer, count, position,
			      drain->write_pos);
}

/**
 * drain_ram_msg_poll() - Handle poll.
 * @file:        File pointer.
 * @wait:        The poll table to which the wait queue is added.
 */
static unsigned int drain_ram_msg_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	struct mvx_log_drain_ram *drain = get_inode_private(file, 1);

	poll_wait(file, &drain->queue, wait);

	if (file->f_pos < drain->write_pos)
		mask |= POLLIN | POLLRDNORM;
	else if (file->f_pos > drain->write_pos)
		mask |= POLLERR;

	return mask;
}

/**
 * drain_ram_ioctl() - Handle IOCTL.
 * @file:        File pointer.
 * @cmd:        The value of the command to be handled.
 * @arg:        Extra argument.
 */
static long drain_ram_ioctl(struct file *file,
			    unsigned int cmd, unsigned long arg)
{
	struct mvx_log_drain_ram *drain_ram = get_inode_private(file, 1);

	switch (cmd) {
	case MVX_LOG_IOCTL_CLEAR:
		drain_ram->read_pos = drain_ram->write_pos;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * drain_ram_open() - Open file handle function.
 * @inode:        The inode associated with the file.
 * @file:        Pointer to the opened file.
 *
 * Return: 0 Always succeeds.
 */
static int drain_ram_open(struct inode *inode, struct file *file)
{
	struct mvx_log_drain_ram *drain_ram = get_inode_private(file, 1);

	file->f_pos = drain_ram->read_pos;

	return 0;
}

/******************************************************************************
 * External interface
 ******************************************************************************/

int mvx_log_construct(struct mvx_log *log, const char *entry_name)
{
	int ret;
	static const struct file_operations readme_fops = {
		.read = readme_read
	};
	struct dentry *dentry;

	if (!IS_ENABLED(CONFIG_DEBUG_FS)) {
		pr_info
		    ("MVX: Debugfs is not enabled. '%s' dir is not created.\n",
		     entry_name);
		return 0;
	}

	log->mvx_dir = debugfs_create_dir(entry_name, NULL);
	if (IS_ERR_OR_NULL(log->mvx_dir)) {
		pr_err("MVX: Failed to create '%s' dir.\n", entry_name);
		return -ENOMEM;
	}

	log->log_dir = debugfs_create_dir("log", log->mvx_dir);
	if (IS_ERR_OR_NULL(log->log_dir)) {
		pr_err("MVX: Failed to create 'log' dir.\n");
		ret = -ENOMEM;
		goto error;
	}

	log->log_dir->d_inode->i_private = log;

	log->drain_dir = debugfs_create_dir("drain", log->log_dir);
	if (IS_ERR_OR_NULL(log->drain_dir)) {
		pr_err("MVX: Failed to create 'drain' dir.\n");
		ret = -ENOMEM;
		goto error;
	}

	log->group_dir = debugfs_create_dir("group", log->log_dir);
	if (IS_ERR_OR_NULL(log->group_dir)) {
		pr_err("MVX: Failed to create 'group' dir.\n");
		ret = -ENOMEM;
		goto error;
	}

	/* Create <group>/drain. */
	dentry = debugfs_create_file("README", 0400, log->log_dir, NULL,
				     &readme_fops);
	if (IS_ERR_OR_NULL(dentry)) {
		pr_err("MVX: Failed to create 'README'.\n");
		ret = -ENOMEM;
		goto error;
	}

	return 0;

error:
	debugfs_remove_recursive(log->mvx_dir);
	return ret;
}

void mvx_log_destruct(struct mvx_log *log)
{
	if (IS_ENABLED(CONFIG_DEBUG_FS))
		debugfs_remove_recursive(log->mvx_dir);
}

/******************************************************************************
 * Log Drain
 ******************************************************************************/

static int drain_construct(struct mvx_log_drain *drain,
			   mvx_print_fptr print,
			   mvx_data_fptr data, mvx_reset_fptr reset)
{
	drain->print = print;
	drain->data = data;
	drain->reset = reset;

	return 0;
}

static void drain_destruct(struct mvx_log_drain *drain)
{
	UNUSED(drain);
}

static void drain_dmesg_print(struct mvx_log_drain *drain,
			      enum mvx_log_severity severity,
			      const char *tag,
			      const char *msg, const unsigned int n_args, ...)
{
	va_list args;
	char fmt[500];

	severity = min_t(int, severity, MVX_LOG_VERBOSE);

	snprintf(fmt, sizeof(fmt), "%s%s: %s\n",
		 severity_to_kern_level[severity], tag, msg);
	fmt[sizeof(fmt) - 1] = '\0';

	va_start(args, n_args);
	vprintk(fmt, args);
	va_end(args);
}

static void drain_dmesg_data(struct mvx_log_drain *drain,
			     enum mvx_log_severity severity,
			     struct iovec *vec, size_t count)
{
	size_t i;

	pr_info("count=%zu\n", count);

	for (i = 0; i < count; ++i) {
		const char *p = vec[i].iov_base;
		size_t length = vec[i].iov_len;

		pr_info("  length=%zu\n", length);

		while (length > 0) {
			size_t j = min_t(size_t, length, 32);
			char buf[3 + 32 * 3 + 1];
			size_t n = 0;

			length -= j;

			n += scnprintf(&buf[n], sizeof(buf) - n, "   ");

			while (j-- > 0)
				n += scnprintf(&buf[n], sizeof(buf) - n,
					       " %02x", *p++);

			pr_info("%s\n", buf);
		}
	}
}

int mvx_log_drain_dmesg_construct(struct mvx_log_drain *drain)
{
	return drain_construct(drain, drain_dmesg_print, drain_dmesg_data,
			       NULL);
}

void mvx_log_drain_dmesg_destruct(struct mvx_log_drain *drain)
{
	drain_destruct(drain);
}

int mvx_log_drain_add(struct mvx_log *log,
		      const char *name, struct mvx_log_drain *drain)
{
	if (!IS_ENABLED(CONFIG_DEBUG_FS)) {
		pr_info
		    ("MVX: Debugfs is not enabled. '%s' dir is not created.\n",
		     name);
		return 0;
	}

	/* Create <drain> directory. */
	drain->dentry = debugfs_create_dir(name, log->drain_dir);
	if (IS_ERR_OR_NULL(drain->dentry)) {
		pr_err("MVX: Failed to create '%s' dir.\n", name);
		return -ENOMEM;
	}

	/* Store pointer to drain object in inode private data. */
	drain->dentry->d_inode->i_private = drain;

	return 0;
}

static void drain_ram_data(struct mvx_log_drain *drain,
			   enum mvx_log_severity severity,
			   struct iovec *vec, size_t count)
{
	struct mvx_log_drain_ram *drain_ram = (struct mvx_log_drain_ram *)drain;
	size_t i;
	size_t length;
	size_t pos;
	int sem_taken;

	if (!IS_ENABLED(CONFIG_DEBUG_FS))
		return;

	/* Calculate the total length of the output. */
	for (i = 0, length = 0; i < count; ++i)
		length += vec[i].iov_len;

	/* Round up to next 32-bit boundary. */
	length = (length + 3) & ~3;

	if (length > drain_ram->buffer_size) {
		pr_err
		    ("MVX: Logged data larger than output buffer. length=%zu, buffer_length=%zu.\n",
		     length, (size_t)drain_ram->buffer_size);
		return;
	}

	sem_taken = down_interruptible(&drain_ram->sem);

	pos = drain_ram->write_pos & (drain_ram->buffer_size - 1);

	/* Loop over scatter input. */
	for (i = 0; i < count; ++i) {
		const char *buf = vec[i].iov_base;
		size_t len = vec[i].iov_len;

		/* Copy log message to output buffer. */
		while (len > 0) {
			size_t n = min(len, drain_ram->buffer_size - pos);

			memcpy(&drain_ram->buf[pos], buf, n);

			len -= n;
			buf += n;
			pos = (pos + n) & (drain_ram->buffer_size - 1);
		}
	}

	/* Update write_pos. Length has already been 4 byte aligned */
	drain_ram->write_pos += length;

	if (sem_taken == 0)
		up(&drain_ram->sem);

	wake_up_interruptible(&drain_ram->queue);
}

static void drain_ram_print(struct mvx_log_drain *drain,
			    enum mvx_log_severity severity,
			    const char *tag,
			    const char *msg, const unsigned int n_args, ...)
{
	char buf[500];
	va_list args;
	size_t n = 0;
	struct mvx_log_header header;
	struct iovec vec[2];
	struct timespec64 timespec;

	if (!IS_ENABLED(CONFIG_DEBUG_FS))
		return;

	/* Write the log message. */
	va_start(args, n_args);
	n += vscnprintf(buf, sizeof(buf), msg, args);
	va_end(args);

	ktime_get_real_ts64(&timespec);

	header.magic = MVX_LOG_MAGIC;
	header.length = n;
	header.type = MVX_LOG_TYPE_TEXT;
	header.severity = severity;
	header.timestamp.sec = timespec.tv_sec;
	header.timestamp.nsec = timespec.tv_nsec;

	vec[0].iov_base = &header;
	vec[0].iov_len = sizeof(header);

	vec[1].iov_base = buf;
	vec[1].iov_len = n;

	drain_ram_data(drain, severity, vec, 2);
}

static void drain_ram_reset(struct mvx_log_drain *drain)
{
	struct mvx_log_drain_ram *drain_ram = (struct mvx_log_drain_ram *)drain;
	int sem_taken = down_interruptible(&drain_ram->sem);

	memset(drain_ram->buf, 0, drain_ram->buffer_size);
	drain_ram->read_pos = 0;
	drain_ram->write_pos = 0;
	if (sem_taken == 0)
		up(&drain_ram->sem);
}

int mvx_log_drain_ram_construct(struct mvx_log_drain_ram *drain,
				size_t buffer_size)
{
	int ret;

	ret = drain_construct(&drain->base, drain_ram_print, drain_ram_data,
			      drain_ram_reset);
	if (ret != 0)
		return ret;

	if (!IS_ENABLED(CONFIG_DEBUG_FS)) {
		pr_info("MVX: No Debugfs no RAM drain.\n");
		return 0;
	}

	drain->buf = vmalloc(buffer_size);
	if (drain->buf == NULL)
		return -ENOMEM;

	*(size_t *)&drain->buffer_size = buffer_size;
	drain->read_pos = 0;
	drain->write_pos = 0;
	init_waitqueue_head(&drain->queue);
	sema_init(&drain->sem, 1);

	return 0;
}

void mvx_log_drain_ram_destruct(struct mvx_log_drain_ram *drain)
{
	if (IS_ENABLED(CONFIG_DEBUG_FS))
		vfree(drain->buf);

	drain_destruct(&drain->base);
}

int mvx_log_drain_ram_add(struct mvx_log *log,
			  const char *name, struct mvx_log_drain_ram *drain)
{
	static const struct file_operations drain_ram_msg = {
		.read = drain_ram_read_msg,
		.poll = drain_ram_msg_poll,
		.open = drain_ram_open,
		.unlocked_ioctl = drain_ram_ioctl
	};
	struct dentry *dentry;
	int ret;

	if (!IS_ENABLED(CONFIG_DEBUG_FS)) {
		pr_info
		    ("MVX: Debugfs is not enabled. RAM drain dirs are not created.\n");
		return 0;
	}

	ret = mvx_log_drain_add(log, name, &drain->base);
	if (ret != 0)
		return ret;

	/* Create dentry. */
	dentry = debugfs_create_file("msg", 0600, drain->base.dentry, NULL,
				     &drain_ram_msg);
	if (IS_ERR_OR_NULL(dentry)) {
		pr_err("MVX: Failed to create '%s/msg.\n", name);
		ret = -ENOMEM;
		goto error;
	}

	return 0;

error:
	debugfs_remove_recursive(drain->base.dentry);

	return ret;
}

#ifdef MVX_LOG_FTRACE_ENABLE
static void drain_ftrace_print(struct mvx_log_drain *drain,
			       enum mvx_log_severity severity,
			       const char *tag,
			       const char *msg, const unsigned int n_args, ...)
{
	va_list args;
	char fmt[500];

	severity = min_t(int, severity, MVX_LOG_VERBOSE);

	snprintf(fmt, sizeof(fmt), "%s %s: %s\n", severity_to_name[severity],
		 tag, msg);
	fmt[sizeof(fmt) - 1] = '\0';

	va_start(args, n_args);
	va_end(args);
}

static void drain_ftrace_data(struct mvx_log_drain *drain,
			      enum mvx_log_severity severity,
			      struct iovec *vec, size_t count)
{
	size_t i;

	for (i = 0; i < count; ++i) {
		const char *p = vec[i].iov_base;
		size_t length = vec[i].iov_len;

		while (length > 0) {
			size_t j = min_t(size_t, length, 32);
			char buf[3 + 32 * 3 + 1];
			size_t n = 0;

			length -= j;

			n += scnprintf(&buf[n], sizeof(buf) - n, "   ");

			while (j-- > 0)
				n += scnprintf(&buf[n], sizeof(buf) - n,
					       " %02x", *p++);
		}
	}
}

int mvx_log_drain_ftrace_construct(struct mvx_log_drain *drain)
{
	return drain_construct(drain, drain_ftrace_print, drain_ftrace_data,
			       NULL);
}

void mvx_log_drain_ftrace_destruct(struct mvx_log_drain *drain)
{
	drain_destruct(drain);
}

#endif /* MVX_LOG_FTRACE_ENABLE */

/******************************************************************************
 * Log Group
 ******************************************************************************/

void mvx_log_group_construct(struct mvx_log_group *group,
			     const char *tag,
			     const enum mvx_log_severity severity,
			     struct mvx_log_drain *drain)
{
	group->tag = tag;
	group->severity = severity;
	group->drain = drain;
	group->enabled = 0;
	group->utilization = -1;
	atomic_set(&group->freq, 300000000);
	group->ts.tv_sec = 0;
	group->ts.tv_nsec = 0;
	group->fps_msg_w = 0;
	mutex_init(&group->mutex);
	group->avgfps = NULL;
	group->rtfps = NULL;
	if (!strncmp(tag, "MVX perf", strlen("MVX perf"))) {
		int size = MVX_LOG_FPS_MSG_BUF_SIZE * 2;	// the 2nd half for msg cache

		group->avgfps = vmalloc(size);
		memset(group->avgfps, 0, size);
		group->rtfps = vmalloc(size);
		memset(group->rtfps, 0, size);
	}
}

int mvx_log_group_add(struct mvx_log *log,
		      const char *name, struct mvx_log_group *group)
{
	//struct dentry *dentry;
	int ret;

	if (!IS_ENABLED(CONFIG_DEBUG_FS)) {
		pr_info
		    ("MVX: Debugfs is not enabled. '%s' dir is not created.\n",
		     name);
		return 0;
	}

	/* Create <group> directory. */
	group->dentry = debugfs_create_dir(name, log->group_dir);
	if (IS_ERR_OR_NULL(group->dentry)) {
		pr_err("MVX: Failed to create '%s' dir.\n", name);
		ret = -ENOMEM;
		goto error;
	}

	/* Store reference to group object in inode private data. */
	group->dentry->d_inode->i_private = group;

	if (!strncmp(name, "perf", strlen("perf"))) {
		static const struct file_operations group_util_fops = {
			.read = group_util_read,
		};
		static const struct file_operations group_avgfps_fops = {
			.read = group_avgfps_read,
		};
		static const struct file_operations group_rtfps_fops = {
			.read = group_rtfps_read,
		};
		static const struct file_operations group_status_fops = {
			.read = group_status_read,
			.write = group_status_write,
		};
		/* Create <group>/utilization. */
		debugfs_create_file("utilization", 0400, group->dentry, NULL,
				    &group_util_fops);

		/* Create <group>/avgfps. */
		debugfs_create_file("average_fps", 0400, group->dentry, NULL,
				    &group_avgfps_fops);

		/* Create <group>/rtfps. */
		debugfs_create_file("realtime_fps", 0400, group->dentry, NULL,
				    &group_rtfps_fops);

		/* Create <group>/enable. */
		debugfs_create_file("enable", 0600, group->dentry, NULL,
				    &group_status_fops);
	} else {
		/* Create <group>/drain. */
		static const struct file_operations group_drain_fops = {
			.read = group_drain_read,
			.write = group_drain_write
		};
		debugfs_create_file("drain", 0600, group->dentry, NULL,
				    &group_drain_fops);

		/* Create <group>/severity. */
		debugfs_create_u32("severity", 0600, group->dentry,
				   &group->severity);
	}

	return 0;

error:
	mvx_log_group_destruct(group);
	return ret;
}

void mvx_log_group_destruct(struct mvx_log_group *group)
{
	if (group->avgfps)
		vfree(group->avgfps);
	if (group->rtfps)
		vfree(group->rtfps);
}

const char *mvx_log_strrchr(const char *s)
{
	const char *p = strrchr(s, '/');

	return (p == NULL) ? s : p + 1;
}
