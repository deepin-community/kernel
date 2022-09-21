#include <linux/slab.h>
#include <linux/lsm_uos_hook_manager.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/security.h>

#include <linux/module.h>
#include "hook.h"

static struct uos_hook_cb_entry *uos_hook_entry_dup(const struct
						    uos_hook_cb_entry *entry);

struct uos_hook_entry_list **hooked_entries = NULL;

#define APPEND_HOOK_CB(id, cb)                                          \
        {                                                               \
                struct uos_hook_entry_list *uos_entries = hooked_entries[id]; \
                if(unlikely(!uos_entries)){                             \
                        pr_err("terrible error happens,check your uksi code!\r\n");\
                        return -1;                                      \
                }                                                       \
                mutex_lock(&uos_entries->lock);                         \
                list_add_tail(&cb->link, &uos_entries->entries);        \
                mutex_unlock(&uos_entries->lock);                       \
        }                                                               \

#define QUERY_HOOK_CB(id, mname, found)                                 \
        {                                                               \
                struct uos_hook_entry *uos_entry = NULL;                \
                struct list_head *p = NULL, *next = NULL;               \
                struct uos_hook_entry_list *uos_entries = hooked_entries[id]; \
                if(unlikely(!uos_entries)){                             \
                        pr_err("terrible error happens,check your uksi code!\r\n");\
                        goto out_query_hook_cb;                                 \
                }                                                       \
                int idx;                                                \
                idx=srcu_read_lock(&uos_entries->uksi_srcu);            \
                list_for_each_entry_rcu (uos_entry,&uos_entries->entries,link) {          \
                        if (unlikely(!uos_entry) || unlikely(!uos_entry->cb) ||          \
                            unlikely(!uos_entry->cb->owner)) {          \
                                pr_err("terrible error happens,check your uksi code!\r\n");     \
                                continue;                               \
                        }                                               \
                        if(strcmp(uos_entry->cb->owner,mname) != 0 ){   \
                                continue;                               \
                        }                                               \
                        found = 1;                                      \
                        break;                                          \
                }                                                       \
                srcu_read_unlock(&uos_entries->uksi_srcu,idx);          \
        }                                                               \
        out_query_hook_cb:                                                      \



#define DEL_HOOK_CB(id, mname, found)                                   \
        {                                                               \
                struct uos_hook_entry *uos_entry = NULL;                \
                struct list_head *p = NULL, *next = NULL;               \
                struct uos_hook_entry_list * uos_entries = hooked_entries[id]; \
                if(unlikely(uos_entries==NULL)){                                \
                        pr_err("terrible error happens,check your uksi code!\r\n");\
                        goto out_del_hook_cb;                                   \
                }                                                       \
                mutex_lock(&uos_entries->lock);                         \
                list_for_each_entry_rcu(uos_entry, &uos_entries->entries,link) {        \
                        if (unlikely(!uos_entry) || unlikely(!uos_entry->cb) ||          \
                            unlikely(!uos_entry->cb->owner)) {          \
                                pr_err("terrible error happens,check your uksi code!\r\n");     \
                                continue;                               \
                        }                                               \
                        if(strcmp(uos_entry->cb->owner,mname) != 0 ){   \
                                continue;                               \
                        }                                               \
                        found=1;                                        \
                        list_del_rcu(&uos_entry->link);                 \
                        synchronize_srcu(&uos_entries->uksi_srcu);      \
                        uos_hook_entry_free(uos_entry);                 \
                        break;                                          \
                }                                                       \
                mutex_unlock(&uos_entries->lock);                       \
        }                                                               \
                out_del_hook_cb:                                                        \


static int check_uos_hook_params(int id, int tr, int inum)
{
	int type = 0;
	int in_num = 0;

	if (id == UOS_INODE_CREATE || id == UOS_INODE_LINK
	    || id == UOS_INODE_SYMLINK || id == UOS_INODE_MKDIR
	    || id == UOS_INODE_FOLLOW_LINK || id == UOS_FILE_IOCTL
	    || id == UOS_FILE_FCNTL || id == UOS_TASK_FIX_SETUID
	    || id == UOS_TASK_PRLIMIT || id == UOS_TASK_SETRLIMIT
	    || id == UOS_SHM_SHMAT || id == UOS_CRED_PREPARE
	    || id == UOS_MSG_QUEUE_MSGSND || id == UOS_GETPROCATTR
	    || id == UOS_SETPROCATTR || id == UOS_SECID_TO_SECCTX
	    || id == UOS_SECCTX_TO_SECID || id == UOS_INODE_NOTIFYSECCTX
	    || id == UOS_INODE_SETSECCTX || id == UOS_KERNEL_READ_FILE
#ifdef CONFIG_BPF_SYSCALL
	    || id == UOS_BPF
#endif
#ifdef CONFIG_KEYS
	    || id == UOS_KEY_ALLOC || id == UOS_KEY_PERMISSION
#endif
	    ) {
		type = 1;
		in_num = 3;
#ifdef CONFIG_SECURITY_PATH
	} else if (id == UOS_PATH_UNLINK || id == UOS_PATH_RMDIR
		   || id == UOS_PATH_CHMOD) {
		type = 1;
		in_num = 2;
	} else if (id == UOS_PATH_MKNOD || id == UOS_PATH_RENAME) {
		type = 1;
		in_num = 4;
	} else if (id == UOS_PATH_TRUNCATE || id == UOS_PATH_CHROOT) {
		type = 1;
		in_num = 1;
	} else if (id == UOS_PATH_SYMLINK || id == UOS_PATH_LINK ||
		   id == UOS_PATH_CHOWN || id == UOS_PATH_MKDIR) {
		type = 1;
		in_num = 3;
#endif
#ifdef CONFIG_SECURITY_NETWORK
	} else if (id == UOS_SOCKET_CREATE || id == UOS_SOCKET_RECVMSG
		   || id == UOS_SCTP_BIND_CONNECT) {
		type = 1;
		in_num = 4;
	} else if (id == UOS_SOCKET_POST_CREATE) {
		type = 1;
		in_num = 5;
	} else if (id == UOS_SOCKET_BIND || id == UOS_SOCKET_CONNECT
		   || id == UOS_SOCKET_SENDMSG || id == UOS_SOCKET_GETSOCKOPT
		   || id == UOS_SOCKET_SETSOCKOPT) {
		type = 1;
		in_num = 3;
	} else if (id == UOS_SOCKET_SOCKETPAIR || id == UOS_SOCKET_LISTEN ||
		   id == UOS_SOCKET_ACCEPT || id == UOS_SOCKET_SHUTDOWN
		   || id == UOS_TUN_DEV_ATTACH
		   || id == UOS_SCTP_ASSOC_REQUEST) {
		type = 1;
		in_num = 2;
	} else if (id == UOS_SOCKET_GETSOCKNAME || id == UOS_SOCKET_GETPEERNAME
		   || id == UOS_TUN_DEV_ATTACH_QUEUE
		   || id == UOS_TUN_DEV_OPEN) {
		type = 1;
		in_num = 1;
	} else if (id == UOS_TUN_DEV_CREATE) {
		type = 1;
		in_num = 0;
#endif
#ifdef CONFIG_AUDIT
	} else if (id == UOS_AUDIT_RULE_INIT) {
		type = 1;
		in_num = 4;
	} else if (id == UOS_AUDIT_RULE_KNOWN) {
		type = 1;
		in_num = 1;
	} else if (id == UOS_AUDIT_RULE_MATCH) {
		type = 1;
		in_num = 4;
#endif
	} else if (id == UOS_INODE_UNLINK || id == UOS_INODE_RMDIR ||
		   id == UOS_INODE_PERMISSION || id == UOS_INODE_SETATTR ||
		   id == UOS_FILE_PERMISSION ||
		   id == UOS_FILE_LOCK || id == UOS_TASK_ALLOC
		   || id == UOS_TASK_SETPGID || id == UOS_TASK_SETNICE
		   || id == UOS_TASK_SETIOPRIO || id == UOS_NETLINK_SEND
		   || id == UOS_SB_REMOUNT || id == UOS_SB_UMOUNT
		   || id == UOS_SB_SHOW_OPTIONS || id == UOS_SHM_ASSOCIATE
		   || id == UOS_SHM_SHMCTL || id == UOS_SEM_ASSOCIATE
		   || id == UOS_SEM_SEMCTL || id == UOS_PTRACE_ACCESS_CHECK
		   || id == UOS_SETTIME || id == UOS_KERNEL_ACT_AS
		   || id == UOS_KERNEL_CREATE_FILES_AS
		   || id == UOS_IPC_PERMISSION || id == UOS_BINDER_TRANSACTION
		   || id == UOS_BINDER_TRANSFER_BINDER
		   || id == UOS_BINDER_TRANSFER_FILE
		   || id == UOS_VM_ENOUGH_MEMORY || id == UOS_SB_PIVOTROOT
		   || id == UOS_CRED_ALLOC_BLANK
		   || id == UOS_MSG_QUEUE_ASSOCIATE
		   || id == UOS_MSG_QUEUE_MSGCTL || id == UOS_KERNEL_LOAD_DATA
#ifdef CONFIG_BPF_SYSCALL
		   || id == UOS_BPF_MAP
#endif
	    ) {
		type = 1;
		in_num = 2;
	} else if (id == UOS_INODE_MKNOD || id == UOS_INODE_RENAME ||
		   id == UOS_MMAP_FILE || id == UOS_TASK_KILL ||
		   id == UOS_SB_CLONE_MNT_OPTS || id == UOS_SEM_SEMOP
		   || id == UOS_CAPGET || id == UOS_CAPABLE
		   || id == UOS_KERNEL_POST_READ_FILE || id == UOS_QUOTACTL
		   || id == UOS_INODE_GETSECURITY) {
		type = 1;
		in_num = 4;
	} else if (id == UOS_INODE_READLINK || id == UOS_MMAP_ADDR
		   || id == UOS_FILE_RECEIVE || id == UOS_FILE_OPEN
		   || id == UOS_BPRM_CHECK_SECURITY || id == UOS_TASK_GETPGID
		   || id == UOS_TASK_GETSID || id == UOS_TASK_GETIOPRIO
		   || id == UOS_TASK_SETSCHEDULER || id == UOS_TASK_GETSCHEDULER
		   || id == UOS_TASK_MOVEMEMORY || id == UOS_SB_STATFS
		   || id == UOS_PTRACE_TRACEME || id == UOS_SYSLOG
		   || id == UOS_KERNEL_MODULE_REQUEST
		   || id == UOS_BINDER_SET_CONTEXT_MGR || id == UOS_QUOTA_ON
		   || id == UOS_ISMACLABEL || id == UOS_INODE_COPY_UP_XATTR
		   || id == UOS_SB_KERN_MOUNT
#ifdef CONFIG_BPF_SYSCALL
		   || id == UOS_BPF_PROG
#endif
	    ) {
		type = 1;
		in_num = 1;
	} else if (id == UOS_INODE_FREE_SECURITY || id == UOS_FILE_FREE_SECURITY
		   || id == UOS_BPRM_COMMITTING_CREDS
		   || id == UOS_BPRM_COMMITTED_CREDS || id == UOS_TASK_FREE || id == UOS_UFILE_CLOSE
		   || id == UOS_CRED_FREE || id == UOS_INODE_INVALIDATE_SECCTX
#ifdef CONFIG_KEYS
		   || id == UOS_KEY_FREE
#endif
	    ) {
		type = 0;
		in_num = 1;
	} else if (id == UOS_TASK_PRCTL || id == UOS_SB_MOUNT
		   || id == UOS_CAPSET || id == UOS_DENTRY_CREATE_FILES_AS
		   || id == UOS_MSG_QUEUE_MSGRCV
		   || id == UOS_INODE_SETSECURITY) {
		type = 1;
		in_num = 5;
	} else if (id == UOS_CRED_TRANSFER || id == UOS_CRED_GETSECID
		   || id == UOS_D_INSTANTIATE || id == UOS_RELEASE_SECCTX
		   || id == UOS_IPC_GETSECID) {
		type = 0;
		in_num = 2;
	}

	if (type != tr) {
		pr_info("[%s] hookid:%d entry->ret_type is wrong \n", __func__,
			id);
		return 1;
	}
	if (in_num != inum) {
		pr_info("[%s] hookid:%d entry->arg_len is wrong \n", __func__,
			id);
		return 1;
	}
	return 0;
}

static struct uos_hook_cb_entry *uos_hook_entry_dup(const struct
						    uos_hook_cb_entry *entry)
{
	struct uos_hook_cb_entry *real = NULL;

	// TODO(jouyouyun): check entry validity
	if (unlikely(!entry) || unlikely(!entry->owner))
		return NULL;

	real = kzalloc(sizeof(struct uos_hook_cb_entry), GFP_KERNEL);
	if (unlikely(!real)) {
		pr_info("[%s] failed to alloc mem for entry(%s)\n", __func__,
			entry->owner);
		return NULL;
	}

	real->owner = kzalloc(strlen(entry->owner) + 1, GFP_KERNEL);
	if (unlikely(!real->owner)) {
		kfree(real);
		pr_info("[%s] failed to alloc mem for owner(%s)\n", __func__,
			entry->owner);
		return NULL;
	}

	strcpy(real->owner, entry->owner);
	real->cb_addr = entry->cb_addr;
	real->ret_type = entry->ret_type;
	real->arg_len = entry->arg_len;

	return real;
}

static int hook_sys_show(struct seq_file *seq, void *v)
{
	seq_printf(seq, "1.0 \n");
	return 0;
}

static int hook_sys_open(struct inode *inode, struct file *file)
{
	return single_open(file, hook_sys_show, inode->i_private);
}

static const struct file_operations hook_ops = {
	.owner = THIS_MODULE,
	.open = hook_sys_open,
	.read = seq_read,
};

static int __init add_sys_file(void)
{
	struct dentry *dir;
	dir = securityfs_create_dir("hookmanager", NULL);
	securityfs_create_file("version", 0666, dir, NULL, &hook_ops);
	return 0;
}

fs_initcall(add_sys_file);

void uos_hook_entry_free(struct uos_hook_entry *uos_entry)
{
	kfree(uos_entry->cb->owner);
	kfree(uos_entry->cb);
	kfree(uos_entry);
}

int uos_hook_register(enum UOS_HOOK_LIST hook_id,
		      struct uos_hook_cb_entry *entry)
{
	struct module *mod;
	int found = 0;
	struct uos_hook_entry *target = NULL;
	struct uos_hook_cb_entry *real = NULL;

	if (unlikely(!hooked_entries)) {
		pr_err("UKSI has not been initialized yet!!!\n");
		return -1;
	};

	if (hook_id >= UOS_HOOK_NONE) {
		pr_info("[%s] invalid hook: %d\n", __func__, hook_id);
		return -1;
	}

       if (unlikely(!entry)) {
               pr_err("[%s] pointer to struct uos_hook_cb_entry should not be NULL for hook_id:%d\n", __func__, hook_id);
               return -1;
       }

       if (unlikely(!entry->owner)) {
               pr_err("[%s] owner should not be NULL for hook_id:%d\n", __func__, hook_id);
               return -1;
       }

       mod = __module_address(entry->cb_addr);

       if (!mod || strcmp(mod->name, entry->owner) != 0) {
               pr_err("[%s] invalid owner for hook_id:%d\n", __func__, hook_id);
               return -1;
       }

	QUERY_HOOK_CB(hook_id, entry->owner, found);
	if (found) {
		pr_info("[%s] the owner(%s) has registed the hook(%d)\n",
			__func__, entry->owner, hook_id);
		return -1;
	}

	int ret =
	    check_uos_hook_params(hook_id, entry->ret_type, entry->arg_len);
	if (ret)
		return -1;

	real = uos_hook_entry_dup(entry);
	if (unlikely(!real))
		return -1;

	target = kzalloc(sizeof(struct uos_hook_entry), GFP_KERNEL);
	if (unlikely(!target)) {
		kfree(real->owner);
		kfree(real);
		pr_info("[%s] failed to alloc mem for target\n", __func__);
		return -1;
	}

	target->cb = real;
	APPEND_HOOK_CB(hook_id, target);

	return 0;
}

EXPORT_SYMBOL(uos_hook_register);

int uos_hook_cancel(enum UOS_HOOK_LIST hook_id, char *owner)
{
	int found = 0;

	if (hook_id >= UOS_HOOK_NONE) {
		pr_info("[%s] invalid hook: %d\n", __func__, hook_id);
		return -1;
	}

	DEL_HOOK_CB(hook_id, owner, found);
	if (!found) {
		pr_info("[%s] the owner(%s) not registed the hook(%d)\n",
			__func__, owner, hook_id);
		return -1;
	}

	return 0;
}

EXPORT_SYMBOL(uos_hook_cancel);
