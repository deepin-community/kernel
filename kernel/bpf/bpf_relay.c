// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2024 Alibaba Cloud
 *
 * relay interface only used by bpf. To use it, please
 * echo following cmds to /sys/kernel/debug/relay_ebpf:
 * - Create:
 * create <dir_name> <file_name> bufnum <n> bufsize <n(k/m/...)> percpu <on/off>
 * - Remove:
 * remove <dir_name> <file_name>
 *
 * Also `cat` can be used to show the current relay files, one entry each line
 * - Show: cat /sys/kernel/debug/relay_ebpf
 * => id dir_name file_name bufnum bufsize percpu
 *
 * The field "id" is a unique identifier for each relay channel, which is
 * needed by bpf helper to write into the channel.
 */

#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/dcache.h>
#include <linux/filter.h>
#include <linux/relay.h>
#include <linux/init.h>
#include <linux/bpf.h>
#include <linux/err.h>
#include <linux/fs.h>

/* preserved index used for tcprt */
enum reserve_relay_idex {
	RELAY_INDEX_TCPRT_LOG = 0,
	RELAY_INDEX_TCPRT_STAT,
	RELAY_INDEX_BEGIN = 4,
};

#define DIR_NAME_TCPRT		"tcp-rt"
#define FILE_NAME_TCPRT_LOG	"rt-network-log"
#define FILE_NAME_TCPRT_STAT	"rt-network-stats"

/* dynamic array to maintain relay channels, with number limit RCHAN_NUM_MAX */
static struct rchan **rchan_array;
static size_t array_capacity;
#define RCHAN_NUM_MAX 32

/* use to protect relay_ebpf, which makes sure that relay_ebpf process one
 * command at a time.
 */
static DEFINE_MUTEX(relay_file_lock);

/* handle the extension of relay array */
static int relay_array_extend(size_t new_size)
{
	struct rchan **new_array, **old;
	size_t new_capacity;

	/* Calculate new capacity with a simple growth strategy */
	new_capacity = (new_size > array_capacity * 2) ? new_size : (array_capacity * 2);

	/* Compare with RCHAN_NUM_MAX, the max capacity */
	if (new_capacity > RCHAN_NUM_MAX)
		new_capacity = RCHAN_NUM_MAX;

	/* Do nothing if new capacity is not larger than old */
	if (new_capacity <= array_capacity)
		return -EINVAL;

	/* Allocate and init new array with new capacity */
	new_array = kcalloc(new_capacity, sizeof(*rchan_array), GFP_KERNEL);
	if (!new_array)
		return -ENOMEM;

	if (rchan_array)
		memcpy(new_array, rchan_array,
		       array_capacity * sizeof(*rchan_array));

	/* update rchan_array with rcu */
	old = rcu_dereference_protected(rchan_array,
					lockdep_is_held(&relay_file_lock));
	rcu_assign_pointer(rchan_array, new_array);
	synchronize_rcu();

	array_capacity = new_capacity;
	kfree(old);

	pr_info("bpf-relay: rchan_array extend to size %zu\n", new_capacity);
	return 0;
}

/* return the idx of target relay channel if exists, return -1 if not */
static int relay_array_lookup(const char *dirname, const char *filename)
{
	const char *fname, *dname;
	int i;

	for (i = 0; i < array_capacity; ++i) {
		if (!rchan_array[i])
			continue;

		fname = rchan_array[i]->base_filename;
		dname = rchan_array[i]->parent->d_name.name;

		if (strcmp(dname, dirname) == 0 &&
		    strcmp(fname, filename) == 0)
			return i;
	}

	return -1;
}

/* return first index if found, else return -1 */
static int relay_array_lookup_dir(struct dentry *dir)
{
	int i;

	for (i = 0; i < array_capacity; ++i) {
		if (!rchan_array[i])
			continue;

		if (rchan_array[i]->parent == dir)
			return i;
	}

	return -1;
}

/* must ensure the index is valid */
static void relay_array_delete(int index)
{
	struct dentry *dir = rchan_array[index]->parent;
	struct rchan *rch = rchan_array[index];

	/* remove targe relay channel */
	rcu_assign_pointer(rchan_array[index], NULL);
	synchronize_rcu();

	relay_close(rch);

	/* check if the parent dir is still in use */
	if (relay_array_lookup_dir(dir) == -1) {
		debugfs_remove_recursive(dir);
		pr_info("bpf-relay: directory deleted\n");
	}
}

/* get the next usable id, return -1 if there is no id left */
static int relay_array_usable_id(const char *dir_name, const char *file_name)
{
	int i;

	/* firstly check preserved special ids */
	if (strcmp(dir_name, DIR_NAME_TCPRT) == 0 &&
	    strcmp(file_name, FILE_NAME_TCPRT_LOG) == 0) {
		pr_info("bpf-relay: prepare to create tcprt log\n");
		i = RELAY_INDEX_TCPRT_LOG;
		goto check;
	}
	if (strcmp(dir_name, DIR_NAME_TCPRT) == 0 &&
	    strcmp(file_name, FILE_NAME_TCPRT_STAT) == 0) {
		pr_info("bpf-relay: prepare to create stats\n");
		i = RELAY_INDEX_TCPRT_STAT;
		goto check;
	}

	/* not special relay, find the minimal usable id */
	for (i = RELAY_INDEX_BEGIN; i < array_capacity; ++i) {
		if (!rchan_array[i])
			return i;
	}

check:
	/* if extend needed but fails, return -1 */
	if (i >= array_capacity) {
		if (relay_array_extend(i + 1))
			return -1;
	}

	return i;
}

/* relay callbacks used by all relay files */
static struct dentry *create_buf_file_handler(const char *filename,
					      struct dentry *parent,
					      umode_t mode,
					      struct rchan_buf *buf,
					      int *is_global)
{
	char final_fname[NAME_MAX];

	strscpy(final_fname, filename, sizeof(final_fname));
	if (buf->chan->private_data) {
		*is_global = 1;

		/* if it is global, remove the last cpu_id 0 */
		final_fname[strlen(filename) - 1] = '\0';
	}

	return debugfs_create_file(final_fname, mode, parent, buf,
				   &relay_file_operations);
}

static int remove_buf_file_handler(struct dentry *dentry)
{
	debugfs_remove(dentry);
	return 0;
}

static int subbuf_start(struct rchan_buf *buf,
			void *subbuf,
			void *prev_subbuf,
			size_t prev_padding)
{
	return 1;
}

static struct rchan_callbacks relay_callbacks = {
	.create_buf_file = create_buf_file_handler,
	.remove_buf_file = remove_buf_file_handler,
	.subbuf_start    = subbuf_start,
};

/* each time print a line */
static ssize_t relay_ebpf_read(struct file *file,
			       char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	/* use relay_array index as ppos, each read() process an element and
	 * *ppos increases by 1, until it reaches array_capacity.
	 */
	struct rchan *rch;
	char buf[128];
	int size, ret;
	loff_t index;

	if (count < sizeof(buf))
		return -EACCES;

	mutex_lock(&relay_file_lock);
	index = *ppos;
	if (index < 0) {
		ret = -EINVAL;
		goto out;
	}

	/* find the first non-null rchan from ppos */
	while (index < array_capacity && !rchan_array[index])
		index++;

	if (index >= array_capacity) {
		ret = 0;
		goto out;
	}

	/* find a valid entry, rch->is_global==0 means percpu is on */
	rch = rchan_array[index];
	size = snprintf(buf, sizeof(buf), "%lld %s %s %lu %lu %d\n", index,
			rch->parent->d_name.name, rch->base_filename,
			rch->n_subbufs, rch->subbuf_size, !rch->is_global);

	ret = copy_to_user(user_buf, buf, size);
	if (ret) {
		ret = -EFAULT;
		goto out;
	}

	ret = size;
	/* current element is processed, increase index */
	*ppos = index + 1;
out:
	mutex_unlock(&relay_file_lock);
	return ret;
}

static int bpf_relay_create(const char *dir_name, const char *file_name,
			    unsigned long bufnum, unsigned long bufsize,
			    void *is_global, int index)
{
	struct dentry *dir;
	struct rchan *rch;
	int dir_create = 0;

	if (index >= array_capacity || index < 0) {
		pr_info("bpf-relay: create fail, index %d out of range\n",
			index);
		return -EINVAL;
	}

	/* check if this relay channel already exists */
	if (relay_array_lookup(dir_name, file_name) != -1) {
		pr_info("bpf-relay: create fail, channel already exists\n");
		return -EEXIST;
	}

	/* find if the dir already exists, if not, create it */
	dir = debugfs_lookup(dir_name, NULL);
	if (!dir) {
		dir = debugfs_create_dir(dir_name, NULL);
		if (IS_ERR(dir))
			return PTR_ERR(dir);
		dir_create = 1;

	} else if (!S_ISDIR(dir->d_inode->i_mode)) {
		pr_info("bpf-relay: create fail, %s is not a directory\n",
			dir_name);
		return -EINVAL;
	}

	rch = relay_open(file_name, dir, bufsize, bufnum,
			 &relay_callbacks, is_global);
	if (!rch) {
		if (dir_create)
			debugfs_remove_recursive(dir);
		pr_info("bpf-relay: create fail, relay_open fail\n");
		return -ENOMEM;
	}

	rcu_assign_pointer(rchan_array[index], rch);
	pr_info("bpf-relay: create finished, id=%d\n", index);
	return 0;
}

static int handle_create(const char *buf)
{
	char dir_name[NAME_MAX], file_name[NAME_MAX], bsize_str[20], percpu[4];
	unsigned long bufnum, bufsize;
	static unsigned char global_flag;
	unsigned char *is_global;
	int ret;

	ret = sscanf(buf, " create %s %s bufnum %lu bufsize %s percpu %4s",
		     dir_name, file_name, &bufnum, bsize_str, percpu);
	if (ret != 5) {
		pr_info("bpf-relay: create fail, get args failed\n");
		return -EINVAL;
	}

	/* parse arguments */
	bufsize = (unsigned long)memparse(bsize_str, NULL);

	/* by passing a valid pointer as private_data for relay channel,
	 * we mark the channel as global, see create_buf_file_handler()
	 */
	is_global = NULL;
	if (strcmp(percpu, "off") == 0)
		is_global = &global_flag;

	ret = relay_array_usable_id(dir_name, file_name);
	if (ret < 0) {
		pr_info("bpf-relay: create fail, no id left\n");
		return -ENOMEM;
	}

	/* create common relay chan according to args */
	return bpf_relay_create(dir_name, file_name, bufnum, bufsize,
				is_global, ret);
}

static int handle_remove(const char *buf)
{
	char dir_name[NAME_MAX], file_name[NAME_MAX];
	int ret;

	ret = sscanf(buf, " remove %s %s", dir_name, file_name);
	if (ret != 2) {
		pr_info("bpf-relay: remove fail, get args failed\n");
		return -EINVAL;
	}

	ret = relay_array_lookup(dir_name, file_name);
	if (ret >= 0) {
		relay_array_delete(ret);
		pr_info("bpf-relay: remove finished, id=%d\n", ret);
	} else {
		pr_info("bpf-relay: remove finished, not exists\n");
	}

	return 0;
}

static ssize_t relay_ebpf_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	char cmd[10], buf[128];
	int ret;

	if (!count || count >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, user_buf, count))
		return -EFAULT;

	/* parse cmd */
	buf[count] = '\0';
	ret = sscanf(buf, " %s", cmd);
	if (ret != 1) {
		pr_info("bpf-relay: write fail, get cmd failed\n");
		return -EINVAL;
	}

	mutex_lock(&relay_file_lock);
	if (strcmp(cmd, "create") == 0) {
		ret = handle_create(buf);
	} else if (strcmp(cmd, "remove") == 0) {
		ret = handle_remove(buf);
	} else {
		pr_info("bpf-relay: write fail, invalid cmd\n");
		ret = -EINVAL;
	}

	/* create or remove succ */
	if (!ret)
		ret = count;

	mutex_unlock(&relay_file_lock);
	return ret;
}

static const struct file_operations relay_ebpf_fops = {
	.write = relay_ebpf_write,
	.read  = relay_ebpf_read,
};

__bpf_kfunc_start_defs();

/* Write data of size size__sz to relay channel of index.
 * WARNING: This kfunc can be deprecated at ANY time in the future.
 */
__bpf_kfunc int bpf_anolis_relay_write(void *data, size_t size__sz, int index)
{
	struct rchan *rch, **rch_arr;
	int ret = 0;

	/* capacity does not need to be protected because it is always updated
	 * after rchan_array and will not decrease. It is safe to use a newer
	 * rchan_array is with older (as well as smaller) capacity.
	 */
	if (index >= array_capacity)
		return -EINVAL;

	rcu_read_lock();

	/* rch_arr will not be NULL, because if it is NULL, array_capacity must
	 * be 0 and then the above index checking would not pass.
	 */
	rch_arr = rcu_dereference(rchan_array);
	rch = rcu_dereference(rch_arr[index]);
	if (!rch) {
		ret = -ENOENT;
		goto out;
	}

	relay_write(rch, data, size__sz);
out:
	rcu_read_unlock();
	return ret;
}

__bpf_kfunc_end_defs();

BTF_SET8_START(bpf_relay_kfunc_ids)
BTF_ID_FLAGS(func, bpf_anolis_relay_write, KF_TRUSTED_ARGS)
BTF_SET8_END(bpf_relay_kfunc_ids)

static const struct btf_kfunc_id_set bpf_relay_kfunc_set = {
	.owner = THIS_MODULE,
	.set   = &bpf_relay_kfunc_ids,
};

/* create relay-ebpf file, rchan_array is created with "create" cmd */
static int __init bpf_relay_init(void)
{
	int ret;

	ret = register_btf_kfunc_id_set(BPF_PROG_TYPE_UNSPEC,
					&bpf_relay_kfunc_set);
	if (ret) {
		pr_err("bpf-relay: register kfunc fail\n");
		return ret;
	}

	if (!debugfs_create_file("relay_ebpf", 0644, NULL, NULL,
				 &relay_ebpf_fops)) {
		pr_err("bpf-relay: debugfs create relay_ebpf fail\n");
		return -ENOMEM;
	}

	return 0;
}
late_initcall(bpf_relay_init);
