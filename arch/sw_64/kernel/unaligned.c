// SPDX-License-Identifier: GPL-2.0
#include <asm/debug.h>

#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

unsigned long unaligned_count;
char unaligned_task[TASK_COMM_LEN];
struct unaligned_stat unaligned[UNA_MAX_ENTRIES];

static ssize_t unaligned_set(struct file *file, const char __user *user_buf,
			size_t len, loff_t *ppos)
{
	size_t size;

	unaligned_count = 0;
	size = min(sizeof(unaligned_task), len);
	if (copy_from_user(unaligned_task, user_buf, size))
		return -EFAULT;
	unaligned_task[size - 1] = '\0';

	return len;
}

static int unaligned_show(struct seq_file *m, void *v)
{
	int i, idx, nr;

	if (!unaligned_task[0]) {
		seq_puts(m, "No task traced\n");
		return 0;
	}
	seq_printf(m, "Task command:\t\t%s\n", unaligned_task);
	seq_printf(m, "Unaligned count:\t%ld\n", unaligned_count);
	if (!unaligned_count)
		return 0;
	nr = 0;
	idx = unaligned_count % UNA_MAX_ENTRIES;
	seq_printf(m, "Latest %d unaligned stat:\nNo.\tVA\t\tPC\n", UNA_MAX_ENTRIES);
	if (unaligned_count >= UNA_MAX_ENTRIES) {
		for (i = idx; i < UNA_MAX_ENTRIES; i++)
			seq_printf(m, "%d\t%#lx\t%#lx\n",
					nr++, unaligned[i].va, unaligned[i].pc);
	}
	for (i = 0; i < idx; i++)
		seq_printf(m, "%d\t%#lx\t%#lx\n",
				nr++, unaligned[i].va, unaligned[i].pc);
	return 0;
}

static int unaligned_open(struct inode *inode, struct file *file)
{
	return single_open(file, unaligned_show, NULL);
}

static const struct file_operations unaligned_fops = {
	.read		= seq_read,
	.write		= unaligned_set,
	.open		= unaligned_open,
	.llseek		= default_llseek,
};

static int __init unaligned_init(void)
{
	struct dentry *unaligned;

	if (!sw64_debugfs_dir)
		return -ENODEV;

	unaligned = debugfs_create_file("unaligned", 0644,
			sw64_debugfs_dir, NULL,
			&unaligned_fops);
	if (!unaligned)
		return -ENOMEM;

	return 0;
}

late_initcall(unaligned_init);
