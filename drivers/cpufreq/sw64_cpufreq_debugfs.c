// SPDX-License-Identifier: GPL-2.0

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include <asm/sw64io.h>
#include <asm/debug.h>
#include <asm/cpufreq.h>

/* Show cpufreq in Mhz */
static int cpufreq_show(struct seq_file *m, void *v)
{
	int i;
	u64 val;
	void __iomem *spbu_base = misc_platform_get_spbu_base(0);

	val = readq(spbu_base + OFFSET_CLK_CTL) >> CORE_PLL2_CFG_SHIFT;
	val &= CORE_PLL2_CFG_MASK;
	seq_puts(m, "CPU frequency in Mhz:\n");
	for (i = 0; freq_table[i].frequency != CPUFREQ_TABLE_END; i++) {
		if (freq_table[i].frequency == CPUFREQ_ENTRY_INVALID)
			continue;
		if (val == i)
			seq_printf(m, "[%d] ", freq_table[i].frequency / 1000);
		else
			seq_printf(m, "%d ", freq_table[i].frequency / 1000);
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
	cf *= 1000; /* convert Mhz to khz */
	index = -1;
	for (i = 0; freq_table[i].frequency != CPUFREQ_TABLE_END; i++) {
		if (cf == freq_table[i].frequency) {
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
