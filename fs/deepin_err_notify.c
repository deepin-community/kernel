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
static int deepin_err_notify_enable __read_mostly = 1;

int deepin_err_notify_enabled(void)
{
	return deepin_err_notify_initialized && deepin_err_notify_enable;
}

/* Check if overlay filesystem is mounted on /usr and send read only error notification */
void deepin_check_and_notify_ro_fs_err(const struct path *path,
				       const char *func_name)
{
	char *path_buf = NULL;
	char *full_path = "";
	/* Rate limiting: allow 100 calls per 5 seconds */
	static DEFINE_RATELIMIT_STATE(deepin_ro_fs_err_ratelimit,
				      5 * HZ, /* 5 seconds interval */
				      100); /* 100 calls per interval */

	/* Check rate limit before proceeding */
	if (!__ratelimit(&deepin_ro_fs_err_ratelimit))
		return;

	/* Early return if path or path->mnt is invalid */
	if (!path || !path->mnt || !path->mnt->mnt_sb)
		return;

	/* Use filesystem callback to decide if notification should be sent.
	 * If filesystem implements the callback, use it.
	 */
	if (path->mnt->mnt_sb->s_op &&
	    path->mnt->mnt_sb->s_op->deepin_should_notify_error) {
		if (!path->mnt->mnt_sb->s_op->deepin_should_notify_error(path->mnt->mnt_sb))
			return;
	} else {
		/* If filesystem does not implement the callback, return immediately. */
		return;
	}

	/* Attempt to get the full path.
	 * Dynamic allocation is used to avoid excessive frame size.
	 */
	if (path->dentry) {
		path_buf = kmalloc(PATH_MAX, GFP_KERNEL);
		if (path_buf) {
			char *p = NULL;

			p = d_path(path, path_buf, PATH_MAX);
			if (!IS_ERR(p))
				full_path = p;
		}
	}

	deepin_send_ro_fs_err_notification(full_path, func_name);

	kfree(path_buf);
}

/* Define multicast group */
static const struct genl_multicast_group deepin_err_notify_nl_mcgrps[] = {
	{
		.name = "ro_fs_events",
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

/* Use fs_initcall to ensure initialization before file system operations */
fs_initcall(deepin_err_notify_init);
