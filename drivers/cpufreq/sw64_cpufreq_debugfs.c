// SPDX-License-Identifier: GPL-2.0

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include <asm/sw64io.h>
#include <asm/debug.h>
#include <asm/cpufreq.h>

static int cpufreq_show(struct seq_file *m, void *v)
{
	int i;
	u64 val;
	int freq;

	val = sw64_io_read(0, CLK_CTL);
	val = val >> CORE_PLL2_CFG_SHIFT;

	for (i = 0; freq_table[i].frequency != CPUFREQ_TABLE_END; i++) {
		if (freq_table[i].frequency != CPUFREQ_ENTRY_INVALID)
			freq = freq_table[i].frequency;
		else
			freq = freq_table[i].driver_data;

		if (val == i)
			seq_printf(m, "[%d] ", freq);
		else
			seq_printf(m, "%d ", freq);
	}
	seq_puts(m, "\n");

	return 0;
}

static int cpufreq_open(struct inode *inode, struct file *file)
{
	return single_open(file, cpufreq_show, NULL);
}

static ssize_t cpufreq_set(struct file *file, const char __user *user_buf,
			size_t len, loff_t *ppos)
{
	char buf[5];
	size_t size;
	int cf, i, err, index, freq;

	size = min(sizeof(buf) - 1, len);
	if (copy_from_user(buf, user_buf, size))
		return -EFAULT;
	buf[size] = '\0';

	err = kstrtoint(buf, 10, &cf);
	if (err)
		return err;

	index = -1;
	for (i = 0; freq_table[i].frequency != CPUFREQ_TABLE_END; i++) {
		if (freq_table[i].frequency != CPUFREQ_ENTRY_INVALID)
			freq = freq_table[i].frequency;
		else
			freq = freq_table[i].driver_data;

		if (cf == freq) {
			index = i;
			break;
		}
	}

	if (index < 0)
		return -EINVAL;

	sw64_set_rate(index);
	update_cpu_freq(freq);
	return len;
}

static const struct file_operations set_cpufreq_fops = {
	.open		= cpufreq_open,
	.read		= seq_read,
	.write		= cpufreq_set,
	.llseek         = seq_lseek,
	.release        = single_release,
};

static int __init cpufreq_debugfs_init(void)
{
	struct dentry *cpufreq_entry;

	if (!sw64_debugfs_dir)
		return -ENODEV;

	cpufreq_entry = debugfs_create_file("cpufreq", 0600,
				       sw64_debugfs_dir, NULL,
				       &set_cpufreq_fops);
	if (!cpufreq_entry)
		return -ENOMEM;

	return 0;
}
late_initcall(cpufreq_debugfs_init);
