#include <linux/lsm_hooks.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/buffer_head.h>
#include <linux/security.h>
#include <linux/init.h>
#include <linux/rculist.h>
#include <linux/lsm_uos_hook_manager.h>
#include <linux/module.h>

#include "hook.h"

// only use consecutive letters and numbers, otherwise
// 'security_add_hooks' will fail
#define UOS_HOOK_MANAGER_LSM_NAME "uosmanager"

extern struct uos_hook_entry_list **hooked_entries;

// TODO(jouyouyun): check cb addr validity
// TODO(jouyouyun): copy entries
#define IMPL_HOOK_FUNC_REAL(index_real, hook_cb_real, return_real)             \
        struct module *mod;                                                    \
        struct list_head *uos_p = NULL, *uos_next = NULL;                      \
        struct uos_hook_entry *uos_entry = NULL;                               \
        struct uos_hook_entry_list *uos_entries = hooked_entries[index_real];  \
        if (unlikely(!uos_entries)) {                                          \
                pr_info("[%s] Not found valid hook entries: %d\n", __func__,   \
                        index_real);                                           \
                return_real;                                                   \
        }                                                                      \
        int idx;                                                                \
        idx=srcu_read_lock(&uos_entries->uksi_srcu);                            \
        list_for_each_entry_rcu (uos_entry,&uos_entries->entries,link) {          \
                if (unlikely(!uos_entry) || unlikely(!uos_entry->cb) ||        \
                    unlikely(!uos_entry->cb->owner) ||                         \
                    unlikely(uos_entry->cb->cb_addr == 0)) {                   \
                        pr_info("[%s] the hook callback invalid\n", __func__);          \
                        continue;                                              \
                }                                                              \
                mod = __module_address(uos_entry->cb->cb_addr);                \
                if (!mod || strcmp(mod->name, uos_entry->cb->owner) != 0) {    \
                        pr_err("[%s] terrible error happens,check your UKSI code!\n", uos_entry->cb->owner);    \
                        continue;                                              \
                }                                                              \
                hook_cb_real;                                                  \
        }                                                                      \
        srcu_read_unlock(&uos_entries->uksi_srcu,idx);                                                  \
        return_real;

#define IMPL_HOOK_FUNC_INT(func_name,_ret_init, index, hook_cb, ...)           \
        static int hook_##func_name(__VA_ARGS__)                               \
        {                                                                      \
                int uos_ret = _ret_init;                                       \
                IMPL_HOOK_FUNC_REAL(index, uos_ret = hook_cb;                  \
                                    if (uos_ret != _ret_init){break;}, return uos_ret); \
        }

#define IMPL_HOOK_FUNC_VOID(func_name, index, hook_cb, ...)                    \
	static void hook_##func_name(__VA_ARGS__)                              \
	{                                                                      \
		IMPL_HOOK_FUNC_REAL(index, hook_cb, return );                  \
	}

IMPL_HOOK_FUNC_INT(binder_set_context_mgr, 0, UOS_BINDER_SET_CONTEXT_MGR,
		   ((int (*)(const struct cred *))(uos_entry->cb->cb_addr))
		   (mgr), const struct cred *mgr);

IMPL_HOOK_FUNC_INT(binder_transaction, 0, UOS_BINDER_TRANSACTION,
		   ((int (*)(const struct cred *,
			     const struct cred *))(uos_entry->cb->
						   cb_addr)) (from, to),
		   const struct cred *from, const struct cred *to);

IMPL_HOOK_FUNC_INT(binder_transfer_binder, 0, UOS_BINDER_TRANSFER_BINDER,
		   ((int (*)(const struct cred *,
			     const struct cred *))(uos_entry->cb->
						   cb_addr)) (from, to),
		   const struct cred *from, const struct cred *to);

IMPL_HOOK_FUNC_INT(binder_transfer_file, 0, UOS_BINDER_TRANSFER_FILE,
		   ((int (*)(const struct cred *, const struct cred *,
			     struct file *))(uos_entry->cb->cb_addr)) (from, to,
								       file),
		   const struct cred *from, const struct cred *to,
		   struct file *file);

IMPL_HOOK_FUNC_INT(ptrace_access_check, 0, UOS_PTRACE_ACCESS_CHECK,
		   ((int (*)(struct task_struct *,
			     unsigned int))(uos_entry->cb->cb_addr)) (child,
								      mode),
		   struct task_struct *child, unsigned int mode);

IMPL_HOOK_FUNC_INT(ptrace_traceme, 0, UOS_PTRACE_TRACEME,
		   ((int (*)(struct task_struct *))(uos_entry->cb->cb_addr))
		   (parent), struct task_struct *parent);

IMPL_HOOK_FUNC_INT(capget, 0, UOS_CAPGET,
		   ((int (*)(struct task_struct *, kernel_cap_t *,
			     kernel_cap_t *,
			     kernel_cap_t *))(uos_entry->cb->cb_addr)) (target,
									effective,
									inheritable,
									permitted),
		   struct task_struct *target, kernel_cap_t * effective,
		   kernel_cap_t * inheritable, kernel_cap_t * permitted);

IMPL_HOOK_FUNC_INT(capset, 0, UOS_CAPSET,
		   ((int (*)(struct cred *, const struct cred *,
			     const kernel_cap_t *, const kernel_cap_t *,
			     const kernel_cap_t *))(uos_entry->cb->
						    cb_addr)) (new, old,
							       effective,
							       inheritable,
							       permitted),
		   struct cred *new, const struct cred *old,
		   const kernel_cap_t * effective,
		   const kernel_cap_t * inheritable,
		   const kernel_cap_t * permitted);

IMPL_HOOK_FUNC_INT(capable, 0, UOS_CAPABLE,
		   ((int (*)(const struct cred *, struct user_namespace *, int,
			     unsigned int))(uos_entry->cb->cb_addr)) (cred, ns,
								      cap,
								      opts),
		   const struct cred *cred, struct user_namespace *ns, int cap,
		   unsigned int opts);

IMPL_HOOK_FUNC_INT(quotactl, 0, UOS_QUOTACTL,
		   ((int (*)(int, int,
			     int,
			     struct super_block *))(uos_entry->cb->
						    cb_addr)) (cmds, type, id,
							       sb), int cmds,
		   int type, int id, struct super_block *sb);

IMPL_HOOK_FUNC_INT(quota_on, 0, UOS_QUOTA_ON,
		   ((int (*)(struct dentry *))(uos_entry->cb->cb_addr))
		   (dentry), struct dentry *dentry);

IMPL_HOOK_FUNC_INT(syslog, 0, UOS_SYSLOG,
		   ((int (*)(int))(uos_entry->cb->cb_addr)) (type), int type);

IMPL_HOOK_FUNC_INT(settime, 0, UOS_SETTIME,
		   ((int (*)(const struct timespec64 *,
			     const struct timezone *))(uos_entry->cb->
						       cb_addr)) (ts, tz),
		   const struct timespec64 *ts, const struct timezone *tz);

IMPL_HOOK_FUNC_INT(vm_enough_memory, 0, UOS_VM_ENOUGH_MEMORY,
		   ((int (*)(struct mm_struct *, long))(uos_entry->cb->cb_addr))
		   (mm, pages), struct mm_struct *mm, long pages);

IMPL_HOOK_FUNC_INT(bprm_check_security, 0, UOS_BPRM_CHECK_SECURITY,
		   ((int (*)(struct linux_binprm *))(uos_entry->cb->cb_addr))
		   (bprm), struct linux_binprm *bprm);

IMPL_HOOK_FUNC_VOID(bprm_committing_creds, UOS_BPRM_COMMITTING_CREDS,
		    ((void (*)(struct linux_binprm *))(uos_entry->cb->cb_addr))
		    (bprm), struct linux_binprm *bprm);

IMPL_HOOK_FUNC_VOID(bprm_committed_creds, UOS_BPRM_COMMITTED_CREDS,
		    ((void (*)(struct linux_binprm *))(uos_entry->cb->cb_addr))
		    (bprm), struct linux_binprm *bprm);

IMPL_HOOK_FUNC_INT(sb_remount, 0, UOS_SB_REMOUNT,
		   ((int (*)(struct super_block *,
			     void *))(uos_entry->cb->cb_addr)) (sb, data),
		   struct super_block *sb, void *data);

IMPL_HOOK_FUNC_INT(sb_show_options, 0, UOS_SB_SHOW_OPTIONS,
		   ((int (*)(struct seq_file *, struct super_block *))
		    (uos_entry->cb->cb_addr)) (m, sb), struct seq_file *m,
		   struct super_block *sb);

IMPL_HOOK_FUNC_INT(sb_statfs, 0, UOS_SB_STATFS,
		   ((int (*)(struct dentry *))(uos_entry->cb->cb_addr))
		   (dentry), struct dentry *dentry);

IMPL_HOOK_FUNC_INT(sb_mount, 0, UOS_SB_MOUNT,
		   ((int (*)(const char *, const struct path *, const char *,
			     unsigned long,
			     void *))(uos_entry->cb->cb_addr)) (dev_name, path,
								type, flags,
								data),
		   const char *dev_name, const struct path *path,
		   const char *type, unsigned long flags, void *data);

IMPL_HOOK_FUNC_INT(sb_umount, 0, UOS_SB_UMOUNT,
		   ((int (*)(struct vfsmount *,
			     int))(uos_entry->cb->cb_addr)) (mnt, flags),
		   struct vfsmount *mnt, int flags);

IMPL_HOOK_FUNC_INT(sb_clone_mnt_opts, 0, UOS_SB_CLONE_MNT_OPTS,
		   ((int (*)(const struct super_block *, struct super_block *,
			     unsigned long,
			     unsigned long *))(uos_entry->cb->cb_addr)) (oldsb,
									 newsb,
									 kern_flags,
									 set_kern_flags),
		   const struct super_block *oldsb, struct super_block *newsb,
		   unsigned long kern_flags, unsigned long *set_kern_flags);

IMPL_HOOK_FUNC_INT(sb_kern_mount, 0, UOS_SB_KERN_MOUNT,
		   ((int (*)(struct super_block *))(uos_entry->cb->cb_addr))
		   (sb), struct super_block *sb);

IMPL_HOOK_FUNC_INT(sb_pivotroot, 0, UOS_SB_PIVOTROOT,
		   ((int (*)(const struct path *, const struct path *))
		    (uos_entry->cb->cb_addr)) (old_path, new_path),
		   const struct path *old_path, const struct path *new_path);

IMPL_HOOK_FUNC_INT(dentry_create_files_as, 0, UOS_DENTRY_CREATE_FILES_AS,
		   ((int (*)(const struct dentry *, int, struct qstr *,
			     const struct cred *,
			     struct cred *))(uos_entry->cb->cb_addr)) (dentry,
								       mode,
								       name,
								       old,
								       new),
		   struct dentry *dentry, int mode, struct qstr *name,
		   const struct cred *old, struct cred *new);

#ifdef CONFIG_SECURITY_PATH
IMPL_HOOK_FUNC_INT(path_unlink, 0, UOS_PATH_UNLINK,
		   ((int (*)(const struct path *,
			     struct dentry *))(uos_entry->cb->cb_addr)) (dir,
									 dentry),
		   const struct path *dir, struct dentry *dentry);

IMPL_HOOK_FUNC_INT(path_mkdir, 0, UOS_PATH_MKDIR,
		   ((int (*)(const struct path *, struct dentry *,
			     umode_t))(uos_entry->cb->cb_addr)) (dir, dentry,
								 mode),
		   const struct path *dir, struct dentry *dentry, umode_t mode);

IMPL_HOOK_FUNC_INT(path_rmdir, 0, UOS_PATH_RMDIR,
		   ((int (*)(const struct path *,
			     struct dentry *))(uos_entry->cb->cb_addr)) (dir,
									 dentry),
		   const struct path *dir, struct dentry *dentry);

IMPL_HOOK_FUNC_INT(path_mknod, 0, UOS_PATH_MKNOD,
		   ((int (*)(const struct path *, struct dentry *, umode_t,
			     unsigned int))(uos_entry->cb->cb_addr)) (dir,
								      dentry,
								      mode,
								      dev),
		   const struct path *dir, struct dentry *dentry, umode_t mode,
		   unsigned int dev);

IMPL_HOOK_FUNC_INT(path_truncate, 0, UOS_PATH_TRUNCATE,
		   ((int (*)(const struct path *))(uos_entry->cb->cb_addr))
		   (path), const struct path *path);

IMPL_HOOK_FUNC_INT(path_symlink, 0, UOS_PATH_SYMLINK,
		   ((int (*)(const struct path *, struct dentry *,
			     const char *))(uos_entry->cb->cb_addr)) (dir,
								      dentry,
								      old_name),
		   const struct path *dir, struct dentry *dentry,
		   const char *old_name);

IMPL_HOOK_FUNC_INT(path_link, 0, UOS_PATH_LINK,
		   ((int (*)(struct dentry *, const struct path *,
			     struct dentry *))(uos_entry->cb->
					       cb_addr)) (old_dentry, new_dir,
							  new_dentry),
		   struct dentry *old_dentry, const struct path *new_dir,
		   struct dentry *new_dentry);

IMPL_HOOK_FUNC_INT(path_rename, 0, UOS_PATH_RENAME,
		   ((int (*)(const struct path *, struct dentry *,
			     const struct path *,
			     struct dentry *))(uos_entry->cb->
					       cb_addr)) (old_dir, old_dentry,
							  new_dir, new_dentry),
		   const struct path *old_dir, struct dentry *old_dentry,
		   const struct path *new_dir, struct dentry *new_dentry);

IMPL_HOOK_FUNC_INT(path_chmod, 0, UOS_PATH_CHMOD,
		   ((int (*)(const struct path *,
			     umode_t))(uos_entry->cb->cb_addr)) (path, mode),
		   const struct path *path, umode_t mode);

IMPL_HOOK_FUNC_INT(path_chown, 0, UOS_PATH_CHOWN,
		   ((int (*)(const struct path *, kuid_t,
			     kgid_t))(uos_entry->cb->cb_addr)) (path, uid, gid),
		   const struct path *path, kuid_t uid, kgid_t gid);

IMPL_HOOK_FUNC_INT(path_chroot, 0, UOS_PATH_CHROOT,
		   ((int (*)(const struct path *))(uos_entry->cb->cb_addr))
		   (path), const struct path *path);
#endif /* CONFIG_SECURITY_PATH */

IMPL_HOOK_FUNC_INT(inode_create, 0, UOS_INODE_CREATE,
		   ((int (*)(struct inode *, struct dentry *,
			     umode_t))(uos_entry->cb->cb_addr)) (dir, dentry,
								 mode),
		   struct inode *dir, struct dentry *dentry, umode_t mode);

IMPL_HOOK_FUNC_VOID(inode_free_security, UOS_INODE_FREE_SECURITY,
		    ((void (*)(struct inode *))(uos_entry->cb->cb_addr))
		    (inode), struct inode *inode);

IMPL_HOOK_FUNC_INT(inode_link, 0, UOS_INODE_LINK,
		   ((int (*)(struct dentry *, struct inode *, struct dentry *))
		    (uos_entry->cb->cb_addr)) (old_dentry, dir, new_dentry),
		   struct dentry *old_dentry, struct inode *dir,
		   struct dentry *new_dentry);

IMPL_HOOK_FUNC_INT(inode_unlink, 0, UOS_INODE_UNLINK,
		   ((int (*)(struct inode *,
			     struct dentry *))(uos_entry->cb->cb_addr)) (dir,
									 dentry),
		   struct inode *dir, struct dentry *dentry);

IMPL_HOOK_FUNC_INT(inode_symlink, 0, UOS_INODE_SYMLINK,
		   ((int (*)(struct inode *, struct dentry *, const char *))
		    (uos_entry->cb->cb_addr)) (dir, dentry, old_name),
		   struct inode *dir, struct dentry *dentry,
		   const char *old_name);

IMPL_HOOK_FUNC_INT(inode_mkdir, 0, UOS_INODE_MKDIR,
		   ((int (*)(struct inode *, struct dentry *,
			     umode_t))(uos_entry->cb->cb_addr)) (dir, dentry,
								 mode),
		   struct inode *dir, struct dentry *dentry, umode_t mode);

IMPL_HOOK_FUNC_INT(inode_rmdir, 0, UOS_INODE_RMDIR,
		   ((int (*)(struct inode *,
			     struct dentry *))(uos_entry->cb->cb_addr)) (dir,
									 dentry),
		   struct inode *dir, struct dentry *dentry);

IMPL_HOOK_FUNC_INT(inode_mknod, 0, UOS_INODE_MKNOD,
		   ((int (*)(struct inode *, struct dentry *, umode_t,
			     dev_t))(uos_entry->cb->cb_addr)) (dir, dentry,
							       mode, dev),
		   struct inode *dir, struct dentry *dentry, umode_t mode,
		   dev_t dev);

IMPL_HOOK_FUNC_INT(inode_rename, 0, UOS_INODE_RENAME,
		   ((int (*)(struct inode *, struct dentry *, struct inode *,
			     struct dentry *))(uos_entry->cb->
					       cb_addr)) (old_dir, old_dentry,
							  new_dir, new_dentry),
		   struct inode *old_dir, struct dentry *old_dentry,
		   struct inode *new_dir, struct dentry *new_dentry);

IMPL_HOOK_FUNC_INT(inode_readlink, 0, UOS_INODE_READLINK,
		   ((int (*)(struct dentry *))(uos_entry->cb->cb_addr))
		   (dentry), struct dentry *dentry);

IMPL_HOOK_FUNC_INT(inode_follow_link, 0, UOS_INODE_FOLLOW_LINK,
		   ((int (*)(struct dentry *, struct inode *,
			     bool))(uos_entry->cb->cb_addr)) (dentry, inode,
							      rcu),
		   struct dentry *dentry, struct inode *inode, bool rcu);

IMPL_HOOK_FUNC_INT(inode_permission, 0, UOS_INODE_PERMISSION,
		   ((int (*)(struct inode *,
			     int))(uos_entry->cb->cb_addr)) (inode, mask),
		   struct inode *inode, int mask);

IMPL_HOOK_FUNC_INT(inode_setattr, 0, UOS_INODE_SETATTR,
		   ((int (*)(struct dentry *,
			     struct iattr *))(uos_entry->cb->cb_addr)) (dentry,
									attr),
		   struct dentry *dentry, struct iattr *attr);

IMPL_HOOK_FUNC_INT(inode_getsecurity, -EOPNOTSUPP, UOS_INODE_GETSECURITY,
		   ((int (*)(struct inode *, const char *,
			     void **, bool))(uos_entry->cb->cb_addr)) (inode,
								       name,
								       buffer,
								       alloc),
		   struct inode *inode, const char *name, void **buffer,
		   bool alloc);

IMPL_HOOK_FUNC_INT(inode_setsecurity, -EOPNOTSUPP, UOS_INODE_SETSECURITY,
		   ((int (*)(struct inode *, const char *,
			     const void *, size_t,
			     int))(uos_entry->cb->cb_addr)) (inode, name, value,
							     size, flags),
		   struct inode *inode, const char *name, const void *value,
		   size_t size, int flags);

IMPL_HOOK_FUNC_INT(inode_copy_up_xattr, -EOPNOTSUPP, UOS_INODE_COPY_UP_XATTR,
		   ((int (*)(const char *))(uos_entry->cb->cb_addr)) (name),
		   const char *name);

IMPL_HOOK_FUNC_INT(file_permission, 0, UOS_FILE_PERMISSION,
		   ((int (*)(struct file *,
			     int))(uos_entry->cb->cb_addr)) (file, mask),
		   struct file *file, int mask);

IMPL_HOOK_FUNC_INT(file_ioctl, 0, UOS_FILE_IOCTL,
		   ((int (*)(struct file *, unsigned int,
			     unsigned long))(uos_entry->cb->cb_addr)) (file,
								       cmd,
								       arg),
		   struct file *file, unsigned int cmd, unsigned long arg);

IMPL_HOOK_FUNC_INT(mmap_addr, 0, UOS_MMAP_ADDR,
		   ((int (*)(unsigned long))(uos_entry->cb->cb_addr)) (addr),
		   unsigned long addr);

IMPL_HOOK_FUNC_INT(mmap_file, 0, UOS_MMAP_FILE,
		   ((int (*)(struct file *, unsigned long, unsigned long,
			     unsigned long))(uos_entry->cb->cb_addr)) (file,
								       reqprot,
								       prot,
								       flags),
		   struct file *file, unsigned long reqprot, unsigned long prot,
		   unsigned long flags);

IMPL_HOOK_FUNC_INT(file_lock, 0, UOS_FILE_LOCK,
		   ((int (*)(struct file *,
			     unsigned int))(uos_entry->cb->cb_addr)) (file,
								      cmd),
		   struct file *file, unsigned int cmd);

IMPL_HOOK_FUNC_INT(file_fcntl, 0, UOS_FILE_FCNTL,
		   ((int (*)(struct file *, unsigned int,
			     unsigned long))(uos_entry->cb->cb_addr)) (file,
								       cmd,
								       arg),
		   struct file *file, unsigned int cmd, unsigned long arg);

IMPL_HOOK_FUNC_INT(file_receive, 0, UOS_FILE_RECEIVE,
		   ((int (*)(struct file *))(uos_entry->cb->cb_addr)) (file),
		   struct file *file);

IMPL_HOOK_FUNC_INT(file_open, 0, UOS_FILE_OPEN,
		   ((int (*)(struct file *))(uos_entry->cb->cb_addr)) (file),
		   struct file *file);

IMPL_HOOK_FUNC_VOID(file_free_security, UOS_FILE_FREE_SECURITY,
		    ((void (*)(struct file *))(uos_entry->cb->cb_addr)) (file),
		    struct file *file);

IMPL_HOOK_FUNC_VOID(uos_file_close, UOS_UFILE_CLOSE,
                       ((void (*)(struct file *))(uos_entry->cb->cb_addr))(file),
                       struct file *file);

IMPL_HOOK_FUNC_INT(cred_alloc_blank, 0, UOS_CRED_ALLOC_BLANK,
		   ((int (*)(struct cred *, gfp_t))(uos_entry->cb->cb_addr))
		   (cred, gfp), struct cred *cred, gfp_t gfp);

IMPL_HOOK_FUNC_VOID(cred_free, UOS_CRED_FREE,
		    ((void (*)(struct cred *))(uos_entry->cb->cb_addr)) (cred),
		    struct cred *cred);

IMPL_HOOK_FUNC_INT(cred_prepare, 0, UOS_CRED_PREPARE,
		   ((int (*)(struct cred *, const struct cred *,
			     gfp_t))(uos_entry->cb->cb_addr)) (new, old, gfp),
		   struct cred *new, const struct cred *old, gfp_t gfp);

IMPL_HOOK_FUNC_VOID(cred_transfer, UOS_CRED_TRANSFER,
		    ((void (*)(struct cred *, const struct cred *))
		     (uos_entry->cb->cb_addr)) (new, old), struct cred *new,
		    const struct cred *old);

IMPL_HOOK_FUNC_VOID(cred_getsecid, UOS_CRED_GETSECID,
		    ((void (*)(const struct cred *, u32 *))
		     (uos_entry->cb->cb_addr)) (c, secid), const struct cred *c,
		    u32 * secid);

IMPL_HOOK_FUNC_INT(kernel_module_request, 0, UOS_KERNEL_MODULE_REQUEST,
		   ((int (*)(char *))(uos_entry->cb->cb_addr)) (kmod_name),
		   char *kmod_name);

IMPL_HOOK_FUNC_INT(kernel_read_file, 0, UOS_KERNEL_READ_FILE,
		   ((int (*)(struct file *,
			     enum kernel_read_file_id,
			     bool))(uos_entry->cb->cb_addr)) (file, id,
							      contents),
		   struct file *file, enum kernel_read_file_id id,
		   bool contents);

IMPL_HOOK_FUNC_INT(kernel_post_read_file, 0, UOS_KERNEL_POST_READ_FILE,
		   ((int (*)(struct file *,
			     char *, loff_t,
			     enum kernel_read_file_id))(uos_entry->cb->
							cb_addr)) (file, buf,
								   size, id),
		   struct file *file, char *buf, loff_t size,
		   enum kernel_read_file_id id);

IMPL_HOOK_FUNC_INT(kernel_act_as, 0, UOS_KERNEL_ACT_AS,
		   ((int (*)(struct cred *, u32))(uos_entry->cb->cb_addr)) (new,
									    secid),
		   struct cred *new, u32 secid);

IMPL_HOOK_FUNC_INT(kernel_create_files_as, 0, UOS_KERNEL_CREATE_FILES_AS,
		   ((int (*)(struct cred *, struct inode *))
		    (uos_entry->cb->cb_addr)) (new, inode), struct cred *new,
		   struct inode *inode);

IMPL_HOOK_FUNC_INT(kernel_load_data, 0, UOS_KERNEL_LOAD_DATA,
		   ((int (*)(enum kernel_load_data_id, bool))
		    (uos_entry->cb->cb_addr)) (id, contents),
		   enum kernel_load_data_id id, bool contents);

IMPL_HOOK_FUNC_INT(task_alloc, 0, UOS_TASK_ALLOC,
		   ((int (*)(struct task_struct * task, unsigned long))
		    (uos_entry->cb->cb_addr)) (task, clone_flags),
		   struct task_struct *task, unsigned long clone_flags);

IMPL_HOOK_FUNC_VOID(task_free, UOS_TASK_FREE,
		    ((void (*)(struct task_struct * task))
		     (uos_entry->cb->cb_addr)) (task),
		    struct task_struct *task);

IMPL_HOOK_FUNC_INT(task_fix_setuid, 0, UOS_TASK_FIX_SETUID,
		   ((int (*)(struct cred *, const struct cred *,
			     int))(uos_entry->cb->cb_addr)) (new, old, flags),
		   struct cred *new, const struct cred *old, int flags);

IMPL_HOOK_FUNC_INT(task_setpgid, 0, UOS_TASK_SETPGID,
		   ((int (*)(struct task_struct *,
			     pid_t))(uos_entry->cb->cb_addr)) (p, pgid),
		   struct task_struct *p, pid_t pgid);

IMPL_HOOK_FUNC_INT(task_getpgid, 0, UOS_TASK_GETPGID,
		   ((int (*)(struct task_struct *))(uos_entry->cb->cb_addr))
		   (p), struct task_struct *p);

IMPL_HOOK_FUNC_INT(task_getsid, 0, UOS_TASK_GETSID,
		   ((int (*)(struct task_struct *))(uos_entry->cb->cb_addr))
		   (p), struct task_struct *p);

IMPL_HOOK_FUNC_INT(task_setnice, 0, UOS_TASK_SETNICE,
		   ((int (*)(struct task_struct *,
			     int))(uos_entry->cb->cb_addr)) (p, nice),
		   struct task_struct *p, int nice);

IMPL_HOOK_FUNC_INT(task_setioprio, 0, UOS_TASK_SETIOPRIO,
		   ((int (*)(struct task_struct *,
			     int))(uos_entry->cb->cb_addr)) (p, ioprio),
		   struct task_struct *p, int ioprio);

IMPL_HOOK_FUNC_INT(task_getioprio, 0, UOS_TASK_GETIOPRIO,
		   ((int (*)(struct task_struct *))(uos_entry->cb->cb_addr))
		   (p), struct task_struct *p);

IMPL_HOOK_FUNC_INT(task_prlimit, 0, UOS_TASK_PRLIMIT,
		   ((int (*)(const struct cred *, const struct cred *,
			     unsigned int))(uos_entry->cb->cb_addr)) (cred,
								      tcred,
								      flags),
		   const struct cred *cred, const struct cred *tcred,
		   unsigned int flags);

IMPL_HOOK_FUNC_INT(task_setrlimit, 0, UOS_TASK_SETRLIMIT,
		   ((int (*)(struct task_struct *, unsigned int,
			     struct rlimit *))(uos_entry->cb->cb_addr)) (p,
									 resource,
									 new_rlim),
		   struct task_struct *p, unsigned int resource,
		   struct rlimit *new_rlim);

IMPL_HOOK_FUNC_INT(task_setscheduler, 0, UOS_TASK_SETSCHEDULER,
		   ((int (*)(struct task_struct *))(uos_entry->cb->cb_addr))
		   (p), struct task_struct *p);

IMPL_HOOK_FUNC_INT(task_getscheduler, 0, UOS_TASK_GETSCHEDULER,
		   ((int (*)(struct task_struct *))(uos_entry->cb->cb_addr))
		   (p), struct task_struct *p);

IMPL_HOOK_FUNC_INT(task_movememory, 0, UOS_TASK_MOVEMEMORY,
		   ((int (*)(struct task_struct *))(uos_entry->cb->cb_addr))
		   (p), struct task_struct *p);

IMPL_HOOK_FUNC_INT(task_kill, 0, UOS_TASK_KILL,
		   ((int (*)(struct task_struct *, struct kernel_siginfo *, int,
			     const struct cred *))(uos_entry->cb->cb_addr)) (p,
									     info,
									     sig,
									     cred),
		   struct task_struct *p, struct kernel_siginfo *info, int sig,
		   const struct cred *cred);

IMPL_HOOK_FUNC_INT(task_prctl, -ENOSYS, UOS_TASK_PRCTL,
		   ((int (*)(int, unsigned long, unsigned long, unsigned long,
			     unsigned long))(uos_entry->cb->cb_addr)) (option,
								       arg2,
								       arg3,
								       arg4,
								       arg5),
		   int option, unsigned long arg2, unsigned long arg3,
		   unsigned long arg4, unsigned long arg5);

IMPL_HOOK_FUNC_INT(ipc_permission, 0, UOS_IPC_PERMISSION,
		   ((int (*)(struct kern_ipc_perm *, short))
		    (uos_entry->cb->cb_addr)) (ipcp, flag),
		   struct kern_ipc_perm *ipcp, short flag);

IMPL_HOOK_FUNC_VOID(ipc_getsecid, UOS_IPC_GETSECID,
		    ((void (*)(struct kern_ipc_perm *, u32 *))
		     (uos_entry->cb->cb_addr)) (ipcp, secid),
		    struct kern_ipc_perm *ipcp, u32 * secid);

IMPL_HOOK_FUNC_INT(msg_queue_associate, 0, UOS_MSG_QUEUE_ASSOCIATE,
		   ((int (*)(const struct kern_ipc_perm *, int))
		    (uos_entry->cb->cb_addr)) (msq, msqflg),
		   struct kern_ipc_perm *msq, int msqflg);

IMPL_HOOK_FUNC_INT(msg_queue_msgctl, 0, UOS_MSG_QUEUE_MSGCTL,
		   ((int (*)(const struct kern_ipc_perm *, int))
		    (uos_entry->cb->cb_addr)) (msq, cmd),
		   struct kern_ipc_perm *msq, int cmd);

IMPL_HOOK_FUNC_INT(msg_queue_msgsnd, 0, UOS_MSG_QUEUE_MSGSND,
		   ((int (*)
		     (const struct kern_ipc_perm *, struct msg_msg *,
		      int))(uos_entry->cb->cb_addr)) (msq, msg, msqflg),
		   struct kern_ipc_perm *msq, struct msg_msg *msg, int msqflg);

IMPL_HOOK_FUNC_INT(msg_queue_msgrcv, 0, UOS_MSG_QUEUE_MSGRCV,
		   ((int (*)
		     (const struct kern_ipc_perm *, struct msg_msg *,
		      struct task_struct *, long,
		      int))(uos_entry->cb->cb_addr)) (msq, msg, target, type,
						      mode),
		   struct kern_ipc_perm *msq, struct msg_msg *msg,
		   struct task_struct *target, long type, int mode);

IMPL_HOOK_FUNC_VOID(d_instantiate, UOS_D_INSTANTIATE,
		    ((void (*)(const struct dentry *, struct inode *))
		     (uos_entry->cb->cb_addr)) (dentry, inode),
		    struct dentry *dentry, struct inode *inode);

IMPL_HOOK_FUNC_INT(getprocattr, -EINVAL, UOS_GETPROCATTR,
		   ((int (*)(const struct task_struct *, char *, char **))
		    (uos_entry->cb->cb_addr)) (p, name, value),
		   struct task_struct *p, char *name, char **value);

IMPL_HOOK_FUNC_INT(setprocattr, -EINVAL, UOS_SETPROCATTR,
		   ((int (*)(const char *, void *, size_t))
		    (uos_entry->cb->cb_addr)) (name, value, size),
		   const char *name, void *value, size_t size);

IMPL_HOOK_FUNC_INT(ismaclabel, 0, UOS_ISMACLABEL,
		   ((int (*)(const char *))(uos_entry->cb->cb_addr)) (name),
		   const char *name);

IMPL_HOOK_FUNC_INT(secid_to_secctx, -EOPNOTSUPP, UOS_SECID_TO_SECCTX,
		   ((int (*)(u32, char **, u32 *))(uos_entry->cb->cb_addr))
		   (secid, secdata, seclen), u32 secid, char **secdata,
		   u32 * seclen);

IMPL_HOOK_FUNC_INT(secctx_to_secid, 0, UOS_SECCTX_TO_SECID,
		   ((int (*)(const char *, u32, u32 *))
		    (uos_entry->cb->cb_addr)) (secdata, seclen, secid),
		   const char *secdata, u32 seclen, u32 * secid);

IMPL_HOOK_FUNC_VOID(release_secctx, UOS_RELEASE_SECCTX, ((void (*)(char *, u32))
							 (uos_entry->cb->
							  cb_addr)) (secdata,
								     seclen),
		    char *secdata, u32 seclen);

IMPL_HOOK_FUNC_VOID(inode_invalidate_secctx, UOS_INODE_INVALIDATE_SECCTX,
		    ((void (*)(struct inode *))
		     (uos_entry->cb->cb_addr)) (inode), struct inode *inode);

IMPL_HOOK_FUNC_INT(inode_notifysecctx, 0, UOS_INODE_NOTIFYSECCTX,
		   ((int (*)(struct inode *, void *, u32))
		    (uos_entry->cb->cb_addr)) (inode, ctx, ctxlen),
		   struct inode *inode, void *ctx, u32 ctxlen);

IMPL_HOOK_FUNC_INT(inode_setsecctx, 0, UOS_INODE_SETSECCTX,
		   ((int (*)(struct dentry *, void *, u32))
		    (uos_entry->cb->cb_addr)) (dentry, ctx, ctxlen),
		   struct dentry *dentry, void *ctx, u32 ctxlen);

IMPL_HOOK_FUNC_INT(netlink_send, 0, UOS_NETLINK_SEND,
		   ((int (*)(struct sock *,
			     struct sk_buff *))(uos_entry->cb->cb_addr)) (sk,
									  skb),
		   struct sock *sk, struct sk_buff *skb);

IMPL_HOOK_FUNC_INT(shm_associate, 0, UOS_SHM_ASSOCIATE,
		   ((int (*)(struct kern_ipc_perm *,
			     int))(uos_entry->cb->cb_addr)) (shp, shmflg),
		   struct kern_ipc_perm *shp, int shmflg);

IMPL_HOOK_FUNC_INT(shm_shmctl, 0, UOS_SHM_SHMCTL,
		   ((int (*)(struct kern_ipc_perm *,
			     int))(uos_entry->cb->cb_addr)) (shp, cmd),
		   struct kern_ipc_perm *shp, int cmd);

IMPL_HOOK_FUNC_INT(shm_shmat, 0, UOS_SHM_SHMAT,
		   ((int (*)(struct kern_ipc_perm *, char __user *,
			     int))(uos_entry->cb->cb_addr)) (shp, shmaddr,
							     shmflg),
		   struct kern_ipc_perm *shp, char __user * shmaddr,
		   int shmflg);

IMPL_HOOK_FUNC_INT(sem_associate, 0, UOS_SEM_ASSOCIATE,
		   ((int (*)(struct kern_ipc_perm *,
			     int))(uos_entry->cb->cb_addr)) (sma, semflg),
		   struct kern_ipc_perm *sma, int semflg);

IMPL_HOOK_FUNC_INT(sem_semctl, 0, UOS_SEM_SEMCTL,
		   ((int (*)(struct kern_ipc_perm *,
			     int))(uos_entry->cb->cb_addr)) (sma, cmd),
		   struct kern_ipc_perm *sma, int cmd);

IMPL_HOOK_FUNC_INT(sem_semop, 0, UOS_SEM_SEMOP,
		   ((int (*)(struct kern_ipc_perm *, struct sembuf *, unsigned,
			     int))(uos_entry->cb->cb_addr)) (sma, sops, nsops,
							     alter),
		   struct kern_ipc_perm *sma, struct sembuf *sops,
		   unsigned nsops, int alter);

#ifdef CONFIG_SECURITY_NETWORK
IMPL_HOOK_FUNC_INT(socket_create, 0, UOS_SOCKET_CREATE,
		   ((int (*)(int, int, int, int))(uos_entry->cb->cb_addr))
		   (family, type, protocol, kern), int family, int type,
		   int protocol, int kern);

IMPL_HOOK_FUNC_INT(socket_post_create, 0, UOS_SOCKET_POST_CREATE,
		   ((int (*)(struct socket *, int, int, int,
			     int))(uos_entry->cb->cb_addr)) (sock, family, type,
							     protocol, kern),
		   struct socket *sock, int family, int type, int protocol,
		   int kern);

IMPL_HOOK_FUNC_INT(socket_socketpair, 0, UOS_SOCKET_SOCKETPAIR,
		   ((int (*)(struct socket *,
			     struct socket *))(uos_entry->cb->cb_addr)) (socka,
									 sockb),
		   struct socket *socka, struct socket *sockb);

IMPL_HOOK_FUNC_INT(socket_bind, 0, UOS_SOCKET_BIND,
		   ((int (*)(struct socket *, struct sockaddr *,
			     int))(uos_entry->cb->cb_addr)) (sock, address,
							     addrlen),
		   struct socket *sock, struct sockaddr *address, int addrlen);

IMPL_HOOK_FUNC_INT(socket_connect, 0, UOS_SOCKET_CONNECT,
		   ((int (*)(struct socket *, struct sockaddr *,
			     int))(uos_entry->cb->cb_addr)) (sock, address,
							     addrlen),
		   struct socket *sock, struct sockaddr *address, int addrlen);

IMPL_HOOK_FUNC_INT(socket_listen, 0, UOS_SOCKET_LISTEN,
		   ((int (*)(struct socket *,
			     int))(uos_entry->cb->cb_addr)) (sock, backlog),
		   struct socket *sock, int backlog);

IMPL_HOOK_FUNC_INT(socket_accept, 0, UOS_SOCKET_ACCEPT,
		   ((int (*)(struct socket *, struct socket *))
		    (uos_entry->cb->cb_addr)) (sock, newsock),
		   struct socket *sock, struct socket *newsock);

IMPL_HOOK_FUNC_INT(socket_sendmsg, 0, UOS_SOCKET_SENDMSG,
		   ((int (*)(struct socket *, struct msghdr *,
			     int))(uos_entry->cb->cb_addr)) (sock, msg, size),
		   struct socket *sock, struct msghdr *msg, int size);

IMPL_HOOK_FUNC_INT(socket_recvmsg, 0, UOS_SOCKET_RECVMSG,
		   ((int (*)(struct socket *, struct msghdr *, int,
			     int))(uos_entry->cb->cb_addr)) (sock, msg, size,
							     flags),
		   struct socket *sock, struct msghdr *msg, int size,
		   int flags);

IMPL_HOOK_FUNC_INT(socket_getsockname, 0, UOS_SOCKET_GETSOCKNAME,
		   ((int (*)(struct socket *))(uos_entry->cb->cb_addr)) (sock),
		   struct socket *sock);

IMPL_HOOK_FUNC_INT(socket_getpeername, 0, UOS_SOCKET_GETPEERNAME,
		   ((int (*)(struct socket *))(uos_entry->cb->cb_addr)) (sock),
		   struct socket *sock);

IMPL_HOOK_FUNC_INT(socket_getsockopt, 0, UOS_SOCKET_GETSOCKOPT,
		   ((int (*)(struct socket *, int,
			     int))(uos_entry->cb->cb_addr)) (sock, level,
							     optname),
		   struct socket *sock, int level, int optname);

IMPL_HOOK_FUNC_INT(socket_setsockopt, 0, UOS_SOCKET_SETSOCKOPT,
		   ((int (*)(struct socket *, int,
			     int))(uos_entry->cb->cb_addr)) (sock, level,
							     optname),
		   struct socket *sock, int level, int optname);

IMPL_HOOK_FUNC_INT(socket_shutdown, 0, UOS_SOCKET_SHUTDOWN,
		   ((int (*)(struct socket *,
			     int))(uos_entry->cb->cb_addr)) (sock, how),
		   struct socket *sock, int how);

IMPL_HOOK_FUNC_INT(tun_dev_create, 0, UOS_TUN_DEV_CREATE,
		   ((int (*)(void))(uos_entry->cb->cb_addr)) (), void);

IMPL_HOOK_FUNC_INT(tun_dev_attach_queue, 0, UOS_TUN_DEV_ATTACH_QUEUE,
		   ((int (*)(void *))(uos_entry->cb->cb_addr)) (security),
		   void *security);

IMPL_HOOK_FUNC_INT(tun_dev_attach, 0, UOS_TUN_DEV_ATTACH,
		   ((int (*)(struct sock *,
			     void *))(uos_entry->cb->cb_addr)) (sk, security),
		   struct sock *sk, void *security);

IMPL_HOOK_FUNC_INT(tun_dev_open, 0, UOS_TUN_DEV_OPEN,
		   ((int (*)(void *))(uos_entry->cb->cb_addr)) (security),
		   void *security);

IMPL_HOOK_FUNC_INT(sctp_assoc_request, 0, UOS_SCTP_ASSOC_REQUEST,
		   ((int (*)(struct sctp_endpoint *,
			     struct sk_buff *))(uos_entry->cb->cb_addr)) (ep,
									  skb),
		   struct sctp_endpoint *ep, struct sk_buff *skb);

IMPL_HOOK_FUNC_INT(sctp_bind_connect, 0, UOS_SCTP_BIND_CONNECT,
		   ((int (*)(struct sock *, int, struct sockaddr *,
			     int))(uos_entry->cb->cb_addr)) (sk, optname,
							     address, addrlen),
		   struct sock *sk, int optname, struct sockaddr *address,
		   int addrlen);
#endif /* CONFIG_SECURITY_NETWORK */
#ifdef CONFIG_KEYS
IMPL_HOOK_FUNC_INT(key_alloc, 0, UOS_KEY_ALLOC,
		   ((int (*)(struct key *, const struct cred *, unsigned long))
		    (uos_entry->cb->cb_addr)) (key, cred, flags),
		   struct key *key, const struct cred *cred,
		   unsigned long flags);

IMPL_HOOK_FUNC_VOID(key_free, UOS_KEY_FREE,
		    ((void (*)(struct key *))(uos_entry->cb->cb_addr)) (key),
		    struct key *key);

IMPL_HOOK_FUNC_INT(key_permission, 0, UOS_KEY_PERMISSION,
		   ((int (*)
		     (key_ref_t, const struct cred *, enum key_need_perm))
		    (uos_entry->cb->cb_addr)) (key_ref, cred, perm),
		   key_ref_t key_ref, const struct cred *cred,
		   enum key_need_perm perm);
#endif
#ifdef CONFIG_AUDIT
IMPL_HOOK_FUNC_INT(audit_rule_init, 0, UOS_AUDIT_RULE_INIT,
		   ((int (*)(u32, u32, char *,
			     void **))(uos_entry->cb->cb_addr)) (field, op,
								 rulestr,
								 lsmrule),
		   u32 field, u32 op, char *rulestr, void **lsmrule);

IMPL_HOOK_FUNC_INT(audit_rule_known, 0, UOS_AUDIT_RULE_KNOWN,
		   ((int (*)(struct audit_krule *))(uos_entry->cb->cb_addr))
		   (krule), struct audit_krule *krule);

IMPL_HOOK_FUNC_INT(audit_rule_match, 0, UOS_AUDIT_RULE_MATCH,
		   ((int (*)(u32, u32, u32, void *))(uos_entry->cb->cb_addr))
		   (secid, field, op, lsmrule), u32 secid, u32 field, u32 op,
		   void *lsmrule);
#endif /* CONFIG_AUDIT */
#ifdef CONFIG_BPF_SYSCALL
IMPL_HOOK_FUNC_INT(bpf, 0, UOS_BPF,
		   ((int (*)(int, union bpf_attr *, unsigned int))
		    (uos_entry->cb->cb_addr)) (cmd, attr, size),
		   int cmd, union bpf_attr *attr, unsigned int size);

IMPL_HOOK_FUNC_INT(bpf_map, 0, UOS_BPF_MAP,
		   ((int (*)(struct bpf_map *, fmode_t))
		    (uos_entry->cb->cb_addr)) (map, fmode),
		   struct bpf_map *map, fmode_t fmode);

IMPL_HOOK_FUNC_INT(bpf_prog, 0, UOS_BPF_PROG, ((int (*)(struct bpf_prog *))
					       (uos_entry->cb->cb_addr)) (prog),
		   struct bpf_prog *prog);
#endif

static struct security_hook_list hooked_list[] __lsm_ro_after_init = {
	LSM_HOOK_INIT(binder_set_context_mgr, hook_binder_set_context_mgr),
	LSM_HOOK_INIT(binder_transaction, hook_binder_transaction),
	LSM_HOOK_INIT(binder_transfer_binder, hook_binder_transfer_binder),
	LSM_HOOK_INIT(binder_transfer_file, hook_binder_transfer_file),
	LSM_HOOK_INIT(ptrace_access_check, hook_ptrace_access_check),
	LSM_HOOK_INIT(ptrace_traceme, hook_ptrace_traceme),
	LSM_HOOK_INIT(capget, hook_capget),
	LSM_HOOK_INIT(capset, hook_capset),
	LSM_HOOK_INIT(capable, hook_capable),
	LSM_HOOK_INIT(quotactl, hook_quotactl),
	LSM_HOOK_INIT(quota_on, hook_quota_on),
	LSM_HOOK_INIT(syslog, hook_syslog),
	LSM_HOOK_INIT(settime, hook_settime),
	LSM_HOOK_INIT(vm_enough_memory, hook_vm_enough_memory),
	LSM_HOOK_INIT(bprm_check_security, hook_bprm_check_security),
	LSM_HOOK_INIT(bprm_committing_creds, hook_bprm_committing_creds),
	LSM_HOOK_INIT(bprm_committed_creds, hook_bprm_committed_creds),
	LSM_HOOK_INIT(sb_remount, hook_sb_remount),
	LSM_HOOK_INIT(sb_show_options, hook_sb_show_options),
	LSM_HOOK_INIT(sb_statfs, hook_sb_statfs),
	LSM_HOOK_INIT(sb_mount, hook_sb_mount),
	LSM_HOOK_INIT(sb_umount, hook_sb_umount),
	LSM_HOOK_INIT(sb_kern_mount, hook_sb_kern_mount),
	LSM_HOOK_INIT(sb_pivotroot, hook_sb_pivotroot),
	LSM_HOOK_INIT(sb_clone_mnt_opts, hook_sb_clone_mnt_opts),
	LSM_HOOK_INIT(dentry_create_files_as, hook_dentry_create_files_as),
#ifdef CONFIG_SECURITY_PATH
	LSM_HOOK_INIT(path_unlink, hook_path_unlink),
	LSM_HOOK_INIT(path_mkdir, hook_path_mkdir),
	LSM_HOOK_INIT(path_rmdir, hook_path_rmdir),
	LSM_HOOK_INIT(path_mknod, hook_path_mknod),
	LSM_HOOK_INIT(path_truncate, hook_path_truncate),
	LSM_HOOK_INIT(path_symlink, hook_path_symlink),
	LSM_HOOK_INIT(path_link, hook_path_link),
	LSM_HOOK_INIT(path_rename, hook_path_rename),
	LSM_HOOK_INIT(path_chmod, hook_path_chmod),
	LSM_HOOK_INIT(path_chown, hook_path_chown),
	LSM_HOOK_INIT(path_chroot, hook_path_chroot),
#endif /* CONFIG_SECURITY_PATH */
	LSM_HOOK_INIT(inode_create, hook_inode_create),
	LSM_HOOK_INIT(inode_free_security, hook_inode_free_security),
	LSM_HOOK_INIT(inode_link, hook_inode_link),
	LSM_HOOK_INIT(inode_unlink, hook_inode_unlink),
	LSM_HOOK_INIT(inode_symlink, hook_inode_symlink),
	LSM_HOOK_INIT(inode_mkdir, hook_inode_mkdir),
	LSM_HOOK_INIT(inode_rmdir, hook_inode_rmdir),
	LSM_HOOK_INIT(inode_mknod, hook_inode_mknod),
	LSM_HOOK_INIT(inode_rename, hook_inode_rename),
	LSM_HOOK_INIT(inode_readlink, hook_inode_readlink),
	LSM_HOOK_INIT(inode_follow_link, hook_inode_follow_link),
	LSM_HOOK_INIT(inode_permission, hook_inode_permission),
	LSM_HOOK_INIT(inode_setattr, hook_inode_setattr),
	LSM_HOOK_INIT(inode_copy_up_xattr, hook_inode_copy_up_xattr),
	LSM_HOOK_INIT(inode_getsecurity, hook_inode_getsecurity),
	LSM_HOOK_INIT(inode_setsecurity, hook_inode_setsecurity),
	LSM_HOOK_INIT(file_permission, hook_file_permission),
	LSM_HOOK_INIT(file_ioctl, hook_file_ioctl),
	LSM_HOOK_INIT(mmap_addr, hook_mmap_addr),
	LSM_HOOK_INIT(mmap_file, hook_mmap_file),
	LSM_HOOK_INIT(file_lock, hook_file_lock),
	LSM_HOOK_INIT(file_fcntl, hook_file_fcntl),
	LSM_HOOK_INIT(file_receive, hook_file_receive),
	LSM_HOOK_INIT(file_open, hook_file_open),
	LSM_HOOK_INIT(file_free_security, hook_file_free_security),
	LSM_HOOK_INIT(uos_file_close, hook_uos_file_close),
	LSM_HOOK_INIT(cred_alloc_blank, hook_cred_alloc_blank),
	LSM_HOOK_INIT(cred_free, hook_cred_free),
	LSM_HOOK_INIT(cred_prepare, hook_cred_prepare),
	LSM_HOOK_INIT(cred_transfer, hook_cred_transfer),
	LSM_HOOK_INIT(cred_getsecid, hook_cred_getsecid),
	LSM_HOOK_INIT(kernel_act_as, hook_kernel_act_as),
	LSM_HOOK_INIT(kernel_module_request, hook_kernel_module_request),
	LSM_HOOK_INIT(kernel_read_file, hook_kernel_read_file),
	LSM_HOOK_INIT(kernel_post_read_file, hook_kernel_post_read_file),
	LSM_HOOK_INIT(kernel_create_files_as, hook_kernel_create_files_as),
	LSM_HOOK_INIT(kernel_load_data, hook_kernel_load_data),
	LSM_HOOK_INIT(task_alloc, hook_task_alloc),
	LSM_HOOK_INIT(task_free, hook_task_free),
	LSM_HOOK_INIT(task_fix_setuid, hook_task_fix_setuid),
	LSM_HOOK_INIT(task_setpgid, hook_task_setpgid),
	LSM_HOOK_INIT(task_getpgid, hook_task_getpgid),
	LSM_HOOK_INIT(task_getsid, hook_task_getsid),
	LSM_HOOK_INIT(task_setnice, hook_task_setnice),
	LSM_HOOK_INIT(task_setioprio, hook_task_setioprio),
	LSM_HOOK_INIT(task_getioprio, hook_task_getioprio),
	LSM_HOOK_INIT(task_prlimit, hook_task_prlimit),
	LSM_HOOK_INIT(task_setrlimit, hook_task_setrlimit),
	LSM_HOOK_INIT(task_setscheduler, hook_task_setscheduler),
	LSM_HOOK_INIT(task_getscheduler, hook_task_getscheduler),
	LSM_HOOK_INIT(task_movememory, hook_task_movememory),
	LSM_HOOK_INIT(task_kill, hook_task_kill),
	LSM_HOOK_INIT(task_prctl, hook_task_prctl),
	LSM_HOOK_INIT(ipc_permission, hook_ipc_permission),
	LSM_HOOK_INIT(task_free, hook_task_free),
	LSM_HOOK_INIT(ipc_getsecid, hook_ipc_getsecid),
	LSM_HOOK_INIT(msg_queue_associate, hook_msg_queue_associate),
	LSM_HOOK_INIT(msg_queue_msgctl, hook_msg_queue_msgctl),
	LSM_HOOK_INIT(msg_queue_msgsnd, hook_msg_queue_msgsnd),
	LSM_HOOK_INIT(msg_queue_msgrcv, hook_msg_queue_msgrcv),
	LSM_HOOK_INIT(shm_associate, hook_shm_associate),
	LSM_HOOK_INIT(shm_shmctl, hook_shm_shmctl),
	LSM_HOOK_INIT(shm_shmat, hook_shm_shmat),
	LSM_HOOK_INIT(sem_associate, hook_sem_associate),
	LSM_HOOK_INIT(sem_semctl, hook_sem_semctl),
	LSM_HOOK_INIT(sem_semop, hook_sem_semop),
	LSM_HOOK_INIT(netlink_send, hook_netlink_send),
	LSM_HOOK_INIT(d_instantiate, hook_d_instantiate),
	LSM_HOOK_INIT(getprocattr, hook_getprocattr),
	LSM_HOOK_INIT(setprocattr, hook_setprocattr),
	LSM_HOOK_INIT(ismaclabel, hook_ismaclabel),
	LSM_HOOK_INIT(secid_to_secctx, hook_secid_to_secctx),
	LSM_HOOK_INIT(secctx_to_secid, hook_secctx_to_secid),
	LSM_HOOK_INIT(release_secctx, hook_release_secctx),
	LSM_HOOK_INIT(inode_invalidate_secctx, hook_inode_invalidate_secctx),
	LSM_HOOK_INIT(inode_notifysecctx, hook_inode_notifysecctx),
	LSM_HOOK_INIT(inode_setsecctx, hook_inode_setsecctx),
#ifdef CONFIG_SECURITY_NETWORK
	LSM_HOOK_INIT(socket_create, hook_socket_create),
	LSM_HOOK_INIT(socket_post_create, hook_socket_post_create),
	LSM_HOOK_INIT(socket_socketpair, hook_socket_socketpair),
	LSM_HOOK_INIT(socket_bind, hook_socket_bind),
	LSM_HOOK_INIT(socket_connect, hook_socket_connect),
	LSM_HOOK_INIT(socket_listen, hook_socket_listen),
	LSM_HOOK_INIT(socket_accept, hook_socket_accept),
	LSM_HOOK_INIT(socket_sendmsg, hook_socket_sendmsg),
	LSM_HOOK_INIT(socket_recvmsg, hook_socket_recvmsg),
	LSM_HOOK_INIT(socket_getsockname, hook_socket_getsockname),
	LSM_HOOK_INIT(socket_getpeername, hook_socket_getpeername),
	LSM_HOOK_INIT(socket_getsockopt, hook_socket_getsockopt),
	LSM_HOOK_INIT(socket_setsockopt, hook_socket_setsockopt),
	LSM_HOOK_INIT(socket_shutdown, hook_socket_shutdown),
	LSM_HOOK_INIT(tun_dev_create, hook_tun_dev_create),
	LSM_HOOK_INIT(tun_dev_attach_queue, hook_tun_dev_attach_queue),
	LSM_HOOK_INIT(tun_dev_attach, hook_tun_dev_attach),
	LSM_HOOK_INIT(tun_dev_open, hook_tun_dev_open),
	LSM_HOOK_INIT(sctp_assoc_request, hook_sctp_assoc_request),
	LSM_HOOK_INIT(sctp_bind_connect, hook_sctp_bind_connect),
#endif /* CONFIG_SECURITY_NETWORK */
#ifdef CONFIG_KEYS
	LSM_HOOK_INIT(key_alloc, hook_key_alloc),
	LSM_HOOK_INIT(key_free, hook_key_free),
	LSM_HOOK_INIT(key_permission, hook_key_permission),
#endif
#ifdef CONFIG_AUDIT
	LSM_HOOK_INIT(audit_rule_init, hook_audit_rule_init),
	LSM_HOOK_INIT(audit_rule_known, hook_audit_rule_known),
	LSM_HOOK_INIT(audit_rule_match, hook_audit_rule_match),
#endif /* CONFIG_AUDIT */
#ifdef CONFIG_BPF_SYSCALL
	LSM_HOOK_INIT(bpf, hook_bpf),
	LSM_HOOK_INIT(bpf_map, hook_bpf_map),
	LSM_HOOK_INIT(bpf_prog, hook_bpf_prog),
#endif
};

static int init_hook_entries(void)
{
	int i = 0;
	int failed = 0;

	hooked_entries =
	    kzalloc(sizeof(struct uos_hook_entry_list *) * (UOS_HOOK_NONE + 1),
		    GFP_KERNEL);
	if (unlikely(!hooked_entries))
		return -1;

	for (; i < UOS_HOOK_NONE; i++) {
		struct uos_hook_entry_list *entries =
		    kzalloc(sizeof(struct uos_hook_entry_list), GFP_KERNEL);
		if (unlikely(!entries)) {
			pr_info("[%s] failed to alloc memory in init: %d\n",
				__func__, i);
			failed = 1;
			break;
		}
		mutex_init(&entries->lock);
		INIT_LIST_HEAD(&entries->entries);
#ifndef CONFIG_DEBUG_LOCK_ALLOC
		init_srcu_struct(&entries->uksi_srcu);
#endif
		hooked_entries[i] = entries;
	}

	if (failed) {
		// init hooked_entries failed
		int j = 0;
		for (; j < i; j++) {
			mutex_destroy(&hooked_entries[j]->lock);
			kfree(hooked_entries[j]);
			hooked_entries[j] = NULL;
		}
		kfree(hooked_entries);
		return -1;
	}

	return 0;
}

static int __init manager_init(void)
{
	int error = 0;

#ifdef CONFIG_DEBUG_LOCK_ALLOC
	pr_err("UKSI is disabled because CONFIG_DEBUG_LOCK_ALLOC is set");
	return -1;
#endif
	error = init_hook_entries();
	if (error) {
		pr_info("[%s] failed to init hook entries\n", __func__);
		return -1;
	}

	security_add_hooks(hooked_list, ARRAY_SIZE(hooked_list),
			   UOS_HOOK_MANAGER_LSM_NAME);
	pr_info("UOS Manager initialized: %s\n", UOS_HOOK_MANAGER_LSM_NAME);

	return 0;
}

int hookmanager_enabled_boot __initdata = 1;

DEFINE_LSM(hookmanager) = {
	.name = "hookmanager",
	.enabled = &hookmanager_enabled_boot,
	.init = manager_init,
};
