// SPDX-License-Identifier: GPL-2.0
/*
 * Deepin filesystem error notification
 *
 * This module provides notification functionality for filesystem errors,
 * especially for read-only filesystem errors.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/stat.h>
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

/* Multicast group name for error events (used in Netlink communication) */
#define MULTICAST_GROUP_ERR_EVENTS "err_events"

/* Maximum number of path_last structures that can be processed */
#define MAX_PATH_LASTS 2

/* Define netlink message types and attributes */
enum {
	DEEPIN_ERR_NOTIFY_ATTR_UNSPEC,
	DEEPIN_ERR_NOTIFY_ATTR_FILENAME = 1, /* Filename (string) */
	DEEPIN_ERR_NOTIFY_ATTR_INODE = 2, /* Inode (u64) */
	DEEPIN_ERR_NOTIFY_ATTR_INODE_PARENT =
		3, /* Bool: inode from parent (u8) */
	DEEPIN_ERR_NOTIFY_ATTR_PID = 4, /* Process ID (u32) */
	DEEPIN_ERR_NOTIFY_ATTR_PPID = 5, /* Parent Process ID (u32) */
	DEEPIN_ERR_NOTIFY_ATTR_COMM =
		6, /* Process Short Name (string, 15 chars max) */
	DEEPIN_ERR_NOTIFY_ATTR_UID = 7, /* User ID (u32) */
	DEEPIN_ERR_NOTIFY_ATTR_GID = 8, /* Group ID (u32) */
	DEEPIN_ERR_NOTIFY_ATTR_STATUS = 9, /* Enable/Disable Status (u8) */
	__DEEPIN_ERR_NOTIFY_ATTR_MAX,
};

#define DEEPIN_ERR_NOTIFY_ATTR_MAX (__DEEPIN_ERR_NOTIFY_ATTR_MAX - 1)

enum {
	DEEPIN_ERR_NOTIFY_CMD_UNSPEC,
	DEEPIN_ERR_NOTIFY_CMD_ERR_ROFS =
		1, /* Read Only Filesystem Error Notify Command */
	DEEPIN_ERR_NOTIFY_CMD_ENABLE = 2, /* Enable error notification */
	DEEPIN_ERR_NOTIFY_CMD_DISABLE = 3, /* Disable error notification */
	DEEPIN_ERR_NOTIFY_CMD_GET_STATUS = 4, /* Get enable/disable status */
	__DEEPIN_ERR_NOTIFY_CMD_MAX,
};

#define DEEPIN_ERR_NOTIFY_CMD_MAX (__DEEPIN_ERR_NOTIFY_CMD_MAX - 1)

/**
 * struct deepin_fs_err_event_data - Filesystem error event data for netlink
 * @filenames: Array of filenames involved in the error
 * @count: Number of filenames in the array (also used for inode count)
 * @inodes: Array of inode numbers corresponding to the filenames
 * @inode_is_parent: Array of bool flags indicating if inode is from parent dentry
 * @pid: Process ID that triggered the error
 * @ppid: Parent Process ID
 * @comm: Process short name (15 chars max)
 * @uid: User ID
 * @gid: Group ID
 *
 * This structure consolidates all the parameters needed for sending
 * filesystem error notifications via netlink.
 */
struct deepin_fs_err_event_data {
	const char **filenames;
	int count;
	const ino_t *inodes;
	const bool *inode_is_parent;
	pid_t pid;
	pid_t ppid;
	const char *comm;
	uid_t uid;
	gid_t gid;
};

/* Track deepin error notification initialization status */
static bool deepin_err_notify_initialized __read_mostly;

/* Runtime control variable for deepin error notification */
static int deepin_err_notify_enable __read_mostly;

/* Forward declaration of Generic Netlink family */
static struct genl_family deepin_err_notify_genl_family;

static int
prepare_and_notify_fs_error(const struct deepin_path_last *path_lasts,
			    int path_last_count);

static inline int deepin_err_notify_enabled(void)
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

/**
 * combine_path_and_last - Combine base path with last component
 * @buffer: Pre-allocated buffer to store the combined path (size PATH_MAX)
 * @path: The base path to combine
 * @last: The last component to append
 *
 * This function combines a base path with a last component, constructing
 * the full path: base_path + "/" + last. The result is stored in buffer.
 *
 * Returns: Pointer to the combined path string on success, NULL on failure
 */
static char *combine_path_and_last(char *buffer, const struct path *path,
				   const char *last)
{
	char *base_path;
	size_t base_len, last_len, total_len;

	base_path = d_absolute_path(path, buffer, PATH_MAX);
	if (IS_ERR(base_path))
		return NULL;

	/* Construct full path: base_path + "/" + last */
	base_len = strlen(base_path);
	last_len = strlen(last);
	total_len = base_len + 1 + last_len + 1;

	if (total_len > PATH_MAX)
		return NULL;

	/*
	 * Move base_path to start of buffer if needed.
	 * d_absolute_path() may return a pointer to the middle of the buffer,
	 * not the start. We need the path at the beginning so we can
	 * append "/" + last to it.
	 */
	if (base_path != buffer)
		memmove(buffer, base_path, base_len);

	/* Avoid appending an extra "/" after root directory "/" which would
	 * result in "//filename".
	 */
	if (base_len > 0 && buffer[base_len - 1] != '/') {
		buffer[base_len] = '/';
		memcpy(buffer + base_len + 1, last, last_len + 1);
	} else {
		/* base_path is already "/" or ends with "/" (rare case) */
		memcpy(buffer + base_len, last, last_len + 1);
	}

	return buffer;
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
		pr_debug(
			"deepin_err_notify: Failed to send notification to userspace: %d\n",
			ret);
	}
}

/* Define multicast group */
static const struct genl_multicast_group deepin_err_notify_nl_mcgrps[] = {
	{
		.name = MULTICAST_GROUP_ERR_EVENTS,
		.flags = GENL_UNS_ADMIN_PERM, /* Require CAP_NET_ADMIN */
	},
};

/* Netlink attribute policy */
static const struct nla_policy
	deepin_err_notify_genl_policy[DEEPIN_ERR_NOTIFY_ATTR_MAX + 1] = {
		[DEEPIN_ERR_NOTIFY_ATTR_FILENAME] = { .type = NLA_STRING },
		[DEEPIN_ERR_NOTIFY_ATTR_INODE] = { .type = NLA_U64 },
		[DEEPIN_ERR_NOTIFY_ATTR_INODE_PARENT] = { .type = NLA_U8 },
		[DEEPIN_ERR_NOTIFY_ATTR_PID] = { .type = NLA_U32 },
		[DEEPIN_ERR_NOTIFY_ATTR_PPID] = { .type = NLA_U32 },
		[DEEPIN_ERR_NOTIFY_ATTR_COMM] = { .type = NLA_STRING },
		[DEEPIN_ERR_NOTIFY_ATTR_UID] = { .type = NLA_U32 },
		[DEEPIN_ERR_NOTIFY_ATTR_GID] = { .type = NLA_U32 },
		[DEEPIN_ERR_NOTIFY_ATTR_STATUS] = { .type = NLA_U8 },
	};

/**
 * deepin_err_notify_cmd_enable - Enable error notification via netlink
 * @skb: Socket buffer (unused)
 * @info: Generic netlink info structure
 *
 * Returns: 0 on success
 */
static int deepin_err_notify_cmd_enable(struct sk_buff *skb,
					struct genl_info *info)
{
	deepin_err_notify_enable = 1;
	pr_debug("deepin_err_notify: Error notification enabled via netlink\n");
	return 0;
}

/**
 * deepin_err_notify_cmd_disable - Disable error notification via netlink
 * @skb: Socket buffer (unused)
 * @info: Generic netlink info structure
 *
 * Returns: 0 on success
 */
static int deepin_err_notify_cmd_disable(struct sk_buff *skb,
					 struct genl_info *info)
{
	deepin_err_notify_enable = 0;
	pr_debug(
		"deepin_err_notify: Error notification disabled via netlink\n");
	return 0;
}

/**
 * deepin_err_notify_cmd_get_status - Get error notification status via netlink
 * @skb: Socket buffer (unused)
 * @info: Generic netlink info structure
 *
 * Returns: 0 on success, negative error code on failure
 */
static int deepin_err_notify_cmd_get_status(struct sk_buff *skb,
					    struct genl_info *info)
{
	struct sk_buff *msg;
	void *hdr;
	int ret;

	/* Allocate a new message */
	msg = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	/* Add Generic Netlink header */
	hdr = genlmsg_put(msg, info->snd_portid, info->snd_seq,
			  &deepin_err_notify_genl_family, 0,
			  DEEPIN_ERR_NOTIFY_CMD_GET_STATUS);
	if (!hdr) {
		ret = -EMSGSIZE;
		goto err_free_msg;
	}

	/* Add status attribute */
	ret = nla_put_u8(msg, DEEPIN_ERR_NOTIFY_ATTR_STATUS,
			 deepin_err_notify_enable);
	if (ret < 0)
		goto err_free_msg;

	genlmsg_end(msg, hdr);

	/* Send unicast reply to the requester */
	return genlmsg_reply(msg, info);

err_free_msg:
	nlmsg_free(msg);
	return ret;
}

/* Define Generic Netlink operations */
static const struct genl_ops deepin_err_notify_genl_ops[] = {
	{
		.cmd = DEEPIN_ERR_NOTIFY_CMD_ENABLE,
		.doit = deepin_err_notify_cmd_enable,
		.flags = GENL_ADMIN_PERM, /* Require CAP_NET_ADMIN */
	},
	{
		.cmd = DEEPIN_ERR_NOTIFY_CMD_DISABLE,
		.doit = deepin_err_notify_cmd_disable,
		.flags = GENL_ADMIN_PERM, /* Require CAP_NET_ADMIN */
	},
	{
		.cmd = DEEPIN_ERR_NOTIFY_CMD_GET_STATUS,
		.doit = deepin_err_notify_cmd_get_status,
		.flags = 0, /* Allow any user to query status */
	},
};

/* Define Generic Netlink family */
static struct genl_family deepin_err_notify_genl_family __ro_after_init = {
	.module = THIS_MODULE,
	.hdrsize = 0,
	.name = DEEPIN_ERR_NOTIFY_FAMILY_NAME,
	.version = 1,
	.maxattr = DEEPIN_ERR_NOTIFY_ATTR_MAX,
	.policy = deepin_err_notify_genl_policy,
	.ops = deepin_err_notify_genl_ops,
	.n_ops = ARRAY_SIZE(deepin_err_notify_genl_ops),
	.mcgrps = deepin_err_notify_nl_mcgrps,
	.n_mcgrps = ARRAY_SIZE(deepin_err_notify_nl_mcgrps),
};

/**
 * get_inode_from_path - Get inode number from path using vfs_getattr
 * @path: The path to get inode from
 * @fallback_dentry: Fallback dentry if vfs_getattr fails
 * @inode_out: Output parameter for inode number
 *
 * Use vfs_getattr to ensure we get the current inode information.
 * This is critical for overlay filesystems where the dentry may be stale
 * after copy-up. vfs_getattr forces the filesystem to return the current
 * inode information.
 *
 * If vfs_getattr fails, falls back to using the dentry's inode directly.
 *
 * Return: 0 on success, negative error code on failure
 */
static int get_inode_from_path(const struct path *path,
			       struct dentry *fallback_dentry,
			       ino_t *inode_out)
{
	struct kstat stat;
	int ret = vfs_getattr(path, &stat, STATX_INO, AT_STATX_SYNC_AS_STAT);

	if (ret == 0) {
		*inode_out = stat.ino;
	} else if (fallback_dentry && fallback_dentry->d_inode) {
		/* Fallback to dentry inode if vfs_getattr fails */
		*inode_out = fallback_dentry->d_inode->i_ino;
	} else {
		return ret;
	}

	return 0;
}

/**
 * notify_fs_error - Send filesystem error notification via netlink
 * @event: Pointer to filesystem error event data structure
 *
 * This function constructs and sends a Generic Netlink message with
 * filesystem error information to userspace listeners.
 * Multiple filenames are sent as separate attributes.
 *
 * Returns: 0 on success (including when there are no listeners to receive
 * the notification, i.e., when -ESRCH occurs), negative error code on
 * other failures.
 */
static int notify_fs_error(const struct deepin_fs_err_event_data *event)
{
	int msg_size = 0;
	struct sk_buff *skb = NULL;
	void *msg_head = NULL;
	int error = 0;
	int i = 0;

	/* Validate input parameters */
	if (!event || !event->filenames || event->count <= 0 || !event->comm ||
	    !event->inodes || !event->inode_is_parent) {
		pr_warn("deepin_err_notify: Invalid event data (event=%p)\n",
			 event);
		return -EINVAL;
	}

	/* Calculate message size for all filenames and attributes */
	msg_size = nla_total_size(sizeof(u32)) /* PID */
		   + nla_total_size(sizeof(u32)) /* PPID */
		   + nla_total_size(strlen(event->comm) + 1) /* COMM */
		   + nla_total_size(sizeof(u32)) /* UID */
		   + nla_total_size(sizeof(u32)); /* GID */

	for (i = 0; i < event->count; i++) {
		if (event->filenames[i])
			msg_size +=
				nla_total_size(strlen(event->filenames[i]) + 1);
	}

	/* Add size for inode numbers and parent flags */
	for (i = 0; i < event->count; i++) {
		msg_size += nla_total_size(sizeof(u64)); /* INODE */
		msg_size += nla_total_size(sizeof(u8)); /* INODE_PARENT */
	}

	skb = genlmsg_new(msg_size, GFP_KERNEL);
	if (!skb) {
		pr_err(
			"deepin_err_notify: Failed to allocate netlink message\n");
		return -ENOMEM;
	}

	msg_head = genlmsg_put(skb, 0, 0, &deepin_err_notify_genl_family, 0,
			       DEEPIN_ERR_NOTIFY_CMD_ERR_ROFS);
	if (!msg_head) {
		error = -EMSGSIZE;
		pr_err("deepin_err_notify: Failed to put netlink header (error: %d)\n",
		       error);
		goto err_out;
	}

	/* Add all filenames as separate attributes */
	for (i = 0; i < event->count; i++) {
		if (event->filenames[i]) {
			error = nla_put_string(skb,
					       DEEPIN_ERR_NOTIFY_ATTR_FILENAME,
					       event->filenames[i]);
			if (error)
				goto attr_err_out;
		}
	}

	/* Add all inode numbers and parent flags as paired attributes */
	for (i = 0; i < event->count; i++) {
		error = nla_put_u64_64bit(skb, DEEPIN_ERR_NOTIFY_ATTR_INODE,
					  event->inodes[i], 0);
		if (error)
			goto attr_err_out;

		/* Add corresponding parent flag */
		error = nla_put_u8(skb, DEEPIN_ERR_NOTIFY_ATTR_INODE_PARENT,
				   event->inode_is_parent[i] ? 1 : 0);
		if (error)
			goto attr_err_out;
	}

	error = nla_put_u32(skb, DEEPIN_ERR_NOTIFY_ATTR_PID, event->pid);
	if (error)
		goto attr_err_out;

	error = nla_put_u32(skb, DEEPIN_ERR_NOTIFY_ATTR_PPID, event->ppid);
	if (error)
		goto attr_err_out;

	error = nla_put_string(skb, DEEPIN_ERR_NOTIFY_ATTR_COMM, event->comm);
	if (error)
		goto attr_err_out;

	error = nla_put_u32(skb, DEEPIN_ERR_NOTIFY_ATTR_UID, event->uid);
	if (error)
		goto attr_err_out;

	error = nla_put_u32(skb, DEEPIN_ERR_NOTIFY_ATTR_GID, event->gid);
	if (error)
		goto attr_err_out;

	genlmsg_end(skb, msg_head);

	/*
	 * Send multicast message
	 *
	 * IMPORTANT: genlmsg_multicast() consumes the skb on success or when
	 * returning -ESRCH (no listeners). The skb ownership is transferred
	 * to the network subsystem in these cases.
	 *
	 * For other error codes, the skb is NOT consumed and must be freed
	 * by the caller.
	 */
	error = genlmsg_multicast(&deepin_err_notify_genl_family, skb, 0, 0,
				  GFP_KERNEL);
	if (error) {
		if (error == -ESRCH) {
			/*
			 * -ESRCH means no listeners registered.
			 * The skb has been consumed, so we must NOT free it.
			 */
			return 0;
		}
		/*
		 * Other errors: skb was not consumed, need to free it.
		 * Jump to err_out for cleanup.
		 */
		pr_err("deepin_err_notify: Multicast failed: %d\n", error);
		goto err_out;
	}

	/*
	 * Success: skb has been consumed by genlmsg_multicast().
	 * Do NOT call kfree_skb() here, as it would cause a double free.
	 */
	pr_debug(
		"deepin_err_notify: Notification sent (files=%d, pid=%d, comm=%s)\n",
		event->count, event->pid, event->comm);
	return 0;

attr_err_out:
	pr_err("deepin_err_notify: Failed to add netlink attributes: %d\n",
		 error);
err_out:
	kfree_skb(skb);
	return error;
}

static int
prepare_and_notify_fs_error(const struct deepin_path_last *path_lasts,
			    int path_last_count)
{
	/* Process information variables */
	pid_t pid = 0, ppid = 0;
	const char *comm = NULL;
	uid_t uid = 0;
	gid_t gid = 0;

	/* Path conversion variables */
	char *path_bufs[MAX_PATH_LASTS] = { NULL, NULL };
	const char *filenames[MAX_PATH_LASTS] = { "", "" };
	int count = 0;
	unsigned long inodes[MAX_PATH_LASTS] = { 0, 0 };
	bool inode_parent_flags[MAX_PATH_LASTS] = { false, false };
	int i = 0;
	int error = 0;

	/* Validate input parameters */
	if (!path_lasts || path_last_count <= 0) {
		pr_warn(
			"deepin_err_notify: Invalid parameters: path_lasts=%p, path_last_count=%d\n",
			path_lasts, path_last_count);
		return -EINVAL;
	}

	/* Only handle up to MAX_PATH_LASTS path_lasts */
	if (path_last_count > MAX_PATH_LASTS) {
		pr_warn(
			"deepin_err_notify: Invalid path_last_count=%d (must be 1-%d)\n",
			path_last_count, MAX_PATH_LASTS);
		return -EINVAL;
	}

	/* Get basic process information from current context */
	pid = current->pid;
	ppid = current->real_parent ? current->real_parent->pid : 0;
	comm = current->comm; /* Short name, limited to 15 characters */

	/* Get credentials */
	uid = from_kuid(&init_user_ns, current_uid());
	gid = from_kgid(&init_user_ns, current_gid());

	/* Convert each path_last to a filename string and extract inode */
	for (i = 0; i < path_last_count; i++) {
		if (path_lasts[i].path.dentry) {
			const struct deepin_path_last *pl = &path_lasts[i];
			struct dentry *target_dentry = pl->path.dentry;

			path_bufs[i] = __getname();
			if (!path_bufs[i])
				continue;

			/* Build the filename */
			if (pl->last) {
				/* Complete path not obtained, need to combine path + last */
				char *combined_path = combine_path_and_last(
					path_bufs[i], &pl->path, pl->last);
				if (!combined_path) {
					pr_warn(
						"deepin_err_notify: combine_path_and_last failed for path_last[%d]\n",
						i);
					continue;
				}

				filenames[count] = combined_path;

				/*
				 * Use parent directory's inode since target doesn't exist.
				 */
				get_inode_from_path(&pl->path, target_dentry,
						    &inodes[count]);
				inode_parent_flags[count] = true;
				count++;
			} else {
				/* Complete path obtained */
				char *p = d_absolute_path(
					&pl->path, path_bufs[i], PATH_MAX);
				if (!IS_ERR(p)) {
					filenames[count] = p;
				} else {
					pr_warn(
						"deepin_err_notify: d_absolute_path failed for path_last[%d]: %ld\n",
						i, PTR_ERR(p));
					continue;
				}

				/*
				 * File exists, get the real inode number.
				 */
				get_inode_from_path(&pl->path, target_dentry,
						    &inodes[count]);
				inode_parent_flags[count] = false;
				count++;
			}
		}
	}

	/*
	 * Send the notification if we got at least one valid filename.
	 * If no valid filenames were extracted, return success anyway
	 * since this is not a critical error (paths may be unmounted, etc).
	 */
	if (count > 0) {
		struct deepin_fs_err_event_data event = {
			.filenames = filenames,
			.count = count,
			.inodes = inodes,
			.inode_is_parent = inode_parent_flags,
			.pid = pid,
			.ppid = ppid,
			.comm = comm,
			.uid = uid,
			.gid = gid,
		};

		error = notify_fs_error(&event);
		if (error < 0) {
			pr_debug(
				"deepin_err_notify: notify_fs_error failed: %d\n",
				error);
			goto cleanup;
		}
	} else {
		pr_warn("deepin_err_notify: No valid filenames to send\n");
	}

	/* Success path: set return value to 0 */
	error = 0;

cleanup:
	/* Resource cleanup: free all allocated path buffers */
	for (i = 0; i < MAX_PATH_LASTS; i++) {
		if (path_bufs[i])
			__putname(path_bufs[i]);
	}

	return error;
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

/* Deepin error notify initialization */
static int __init deepin_err_notify_init(void)
{
	int error;

	/* Compile-time check for family name length */
	BUILD_BUG_ON(sizeof(DEEPIN_ERR_NOTIFY_FAMILY_NAME) > GENL_NAMSIZ);

	/* Register Generic Netlink family */
	error = genl_register_family(&deepin_err_notify_genl_family);
	if (error) {
		pr_err("deepin_err_notify: Failed to register Generic Netlink family: %d\n",
		       error);
		return error;
	}

	/* Set initialization success flag */
	deepin_err_notify_initialized = true;

	pr_debug(
		"deepin_err_notify: Generic Netlink family '%s' registered successfully\n",
		DEEPIN_ERR_NOTIFY_FAMILY_NAME);
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
 * It attempts to look up the old and new paths and send error notification
 * if both paths are valid.
 */
void deepin_notify_rename_ro_fs_err(const struct qstr *old_last,
				    const struct qstr *new_last,
				    const struct path *old_path,
				    const struct path *new_path)
{
	const struct deepin_path_last path_lasts[2] = {
		{ .path = *old_path, .last = old_last->name },
		{ .path = *new_path, .last = new_last->name }
	};
	int ret;

	ret = prepare_and_notify_fs_error(path_lasts, 2);
	if (ret < 0) {
		pr_debug(
			"deepin_err_notify: Failed to send notification to userspace: %d\n",
			ret);
	}
}

/* Use fs_initcall to ensure initialization before file system operations */
fs_initcall(deepin_err_notify_init);
