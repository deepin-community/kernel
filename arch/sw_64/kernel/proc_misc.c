// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

extern const struct seq_operations cpu_active_mask_op;
static int cpu_active_mask_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &cpu_active_mask_op);
}

static const struct file_operations proc_cpu_active_mask_operations = {
	.open		= cpu_active_mask_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int __init proc_cpu_active_mask_init(void)
{
	proc_create("cpu_active_mask", 0, NULL, &proc_cpu_active_mask_operations);
	return 0;
}
fs_initcall(proc_cpu_active_mask_init);
