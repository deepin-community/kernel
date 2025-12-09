// SPDX-License-Identifier: GPL-2.0
/*
 * fs/deepin_ro_fs_err_notify.c - Deepin read-only filesystem error notification
 *
 * This module provides notification functionality for read-only filesystem
 * errors, specifically targeting overlay filesystems mounted on /usr.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/path.h>
#include <linux/dcache.h>
#include <linux/sched.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/limits.h>
#include <linux/sysctl.h>
#include <linux/ratelimit.h>
#include <linux/build_bug.h>
#include <net/netlink.h>
#include <net/genetlink.h>

#include "internal.h"
#include "mount.h"

/* Family name (max GENL_NAMSIZ characters, including null terminator) */
#define DEEPIN_ERR_NOTIFY_FAMILY_NAME "DEEPIN_ENOTIFY"

/* Define netlink message types and attributes */
enum {
	DEEPIN_ERR_NOTIFY_ATTR_UNSPEC,
	DEEPIN_ERR_NOTIFY_ATTR_FILENAME, /* Filename */
	DEEPIN_ERR_NOTIFY_ATTR_PID, /* Process ID */
	DEEPIN_ERR_NOTIFY_ATTR_COMM, /* Process Name */
	DEEPIN_ERR_NOTIFY_ATTR_FUNC_NAME, /* Function Name */
	__DEEPIN_ERR_NOTIFY_ATTR_MAX,
};

#define DEEPIN_ERR_NOTIFY_ATTR_MAX (__DEEPIN_ERR_NOTIFY_ATTR_MAX - 1)

enum {
	DEEPIN_ERR_NOTIFY_CMD_UNSPEC,
	DEEPIN_ERR_NOTIFY_CMD_NOTIFY, /* Error Notify Command */
	__DEEPIN_ERR_NOTIFY_CMD_MAX,
};

#define DEEPIN_ERR_NOTIFY_CMD_MAX (__DEEPIN_ERR_NOTIFY_CMD_MAX - 1)

/* Track deepin error notification initialization status */
static bool deepin_err_notify_initialized __read_mostly;

/* Runtime control variable for deepin error notification */
static int deepin_err_notify_enable __read_mostly = 0;

int deepin_err_notify_enabled(void)
{
	return deepin_err_notify_initialized && deepin_err_notify_enable;
}

/**
 * deepin_err_notify_should_send - Check if error notification should be sent
 *
 * This function checks both the enable status and rate limiting to determine
 * whether an error notification should be sent.
 *
 * Return: 1 if notification should be sent, 0 otherwise
 */
int deepin_err_notify_should_send(void)
{

	/*
	 * Rate limiting: Allow 20 calls per 5 seconds.
	 * Rationale: Prevent excessive error notifications under high load,
	 * which could overwhelm the monitoring system or cause log flooding.
	 * 20 notifications per 5 seconds is sufficient to capture relevant
	 * filesystem errors without missing critical events.
	 */
	static DEFINE_RATELIMIT_STATE(deepin_err_notify_ratelimit, 5 * HZ, 20);

	if (!deepin_err_notify_enabled())
		return 0;

	return __ratelimit(&deepin_err_notify_ratelimit);
}

static int
prepare_and_notify_fs_error(const struct deepin_path_last *path_lasts,
			    int path_last_count)
{
	/* TODO: Implement in next commit */
	return -EOPNOTSUPP;
}

/* Check if overlay filesystem is mounted on /usr and send read only error notification */
void deepin_check_and_notify_ro_fs_err(const struct deepin_path_last *path_last,
				       const char *func_name)
{
	int ret;

	/* Early return if path_last is invalid */
	if (!path_last)
		return;
	ret = prepare_and_notify_fs_error(path_last, 1);

	if (ret < 0) {
		pr_err(
			"deepin_err_notify: Failed to send notification to userspace: %d\n",
			ret);
	}
}

/* Define multicast group */
static const struct genl_multicast_group deepin_err_notify_nl_mcgrps[] = {
	{
		.name = "ro_fs_events",
		.flags = GENL_UNS_ADMIN_PERM,
	},
};

/* Define Generic Netlink family */
static struct genl_family deepin_err_notify_genl_family __ro_after_init = {
	.module = THIS_MODULE,
	.hdrsize = 0,
	.name = DEEPIN_ERR_NOTIFY_FAMILY_NAME,
	.version = 1,
	.maxattr = DEEPIN_ERR_NOTIFY_ATTR_MAX,
	.mcgrps = deepin_err_notify_nl_mcgrps,
	.n_mcgrps = ARRAY_SIZE(deepin_err_notify_nl_mcgrps),
};

/* Send read only filesystem error notification */
void deepin_send_ro_fs_err_notification(const char *filename,
					const char *func_name)
{
	pid_t pid = 0;
	const char *comm = NULL;
	int msg_size;
	struct sk_buff *skb = NULL;
	void *msg_head = NULL;
	int error;

	pid = current->pid;
	comm = current->comm;

	msg_size = nla_total_size(strlen(filename) + 1) +
		   nla_total_size(sizeof(u32)) +
		   nla_total_size(strlen(comm) + 1) +
		   nla_total_size(strlen(func_name) + 1);

	/* Use GFP_NOFS to avoid recursion in file system operations. */
	skb = genlmsg_new(msg_size, GFP_NOFS);
	if (!skb) {
		pr_err("deepin_err_notify: Failed to allocate netlink message\n");
		return;
	}

	msg_head = genlmsg_put(skb, 0, 0, &deepin_err_notify_genl_family, 0,
			       DEEPIN_ERR_NOTIFY_CMD_NOTIFY);
	if (!msg_head) {
		pr_err("deepin_err_notify: Failed to put netlink header\n");
		goto err_out;
	}

	error = nla_put_string(skb, DEEPIN_ERR_NOTIFY_ATTR_FILENAME, filename);
	if (error)
		goto attr_err_out;

	error = nla_put_u32(skb, DEEPIN_ERR_NOTIFY_ATTR_PID, pid);
	if (error)
		goto attr_err_out;

	error = nla_put_string(skb, DEEPIN_ERR_NOTIFY_ATTR_COMM, comm);
	if (error)
		goto attr_err_out;

	error = nla_put_string(skb, DEEPIN_ERR_NOTIFY_ATTR_FUNC_NAME,
			       func_name);
	if (error)
		goto attr_err_out;

	genlmsg_end(skb, msg_head);

	/* Send multicast message. */
	genlmsg_multicast(&deepin_err_notify_genl_family, skb, 0, 0, GFP_NOFS);
	return;

attr_err_out:
	pr_err("deepin_err_notify: Failed to add netlink attributes\n");
err_out:
	kfree_skb(skb);
}

/**
 * deepin_put_path_last - Release resources held by deepin_path_last structure
 * @path_last: Pointer to the deepin_path_last structure to release
 *
 * This function releases the path reference and frees the allocated filename
 * string in the deepin_path_last structure.
 */
void deepin_put_path_last(struct deepin_path_last *path_last)
{
	if (path_last) {
		path_put(&path_last->path);
		kfree(path_last->last);
		path_last->last = NULL;
	}
}

/* sysctl table and initialization */
#ifdef CONFIG_SYSCTL
static struct ctl_table deepin_err_notify_sysctls[] = {
	{
		.procname = "deepin-err-notify-enable",
		.data = &deepin_err_notify_enable,
		.maxlen = sizeof(int),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
};

static void __init deepin_err_notify_sysctl_init(void)
{
	register_sysctl_init("fs", deepin_err_notify_sysctls);
}
#else
static void __init deepin_err_notify_sysctl_init(void)
{
}
#endif /* CONFIG_SYSCTL */

/* Deepin error notify initialization */
static int __init deepin_err_notify_init(void)
{
	int error;

	/* Compile-time check for family name length */
	BUILD_BUG_ON(sizeof(DEEPIN_ERR_NOTIFY_FAMILY_NAME) > GENL_NAMSIZ);

	error = genl_register_family(&deepin_err_notify_genl_family);
	if (error) {
		pr_err("deepin_err_notify: Failed to register Generic Netlink family: %d\n",
		       error);
		return error;
	}

	/* Set initialization success flag */
	deepin_err_notify_initialized = true;

	/* Initialize sysctl interface */
	deepin_err_notify_sysctl_init();

	pr_info("deepin_err_notify: Generic Netlink family registered successfully\n");
	return 0;
}

/**
 * deepin_notify_rename_ro_fs_err - Notify read-only filesystem errors during rename
 * @old_last: qstr for old filename
 * @new_last: qstr for new filename
 * @old_path: path of old parent directory
 * @new_path: path of new parent directory
 *
 * This function is called when a rename operation fails with EROFS error.
 */
void deepin_notify_rename_ro_fs_err(const struct qstr *old_last,
				    const struct qstr *new_last,
				    const struct path *old_path,
				    const struct path *new_path)
{
	/* Simplified implementation - will be enhanced in next commit */
}

/* Use fs_initcall to ensure initialization before file system operations */
fs_initcall(deepin_err_notify_init);
