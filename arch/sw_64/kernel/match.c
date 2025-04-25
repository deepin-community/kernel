// SPDX-License-Identifier: GPL-2.0

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include <asm/sw64io.h>
#include <asm/debug.h>
#include <asm/csr.h>


char da_match_buf[1024], dv_match_buf[1024], dav_match_buf[1024];
char ia_match_buf[1024], iv_match_buf[1024], ida_match_buf[1024];

unsigned long da_match_cf1, da_match_cf2, da_match_cf3;
unsigned long dv_match_cf1, dv_match_cf2, dv_match_cf3;
unsigned long dav_match_cf1, dav_match_cf2, dav_match_cf3,
	      dav_match_cf4, dav_match_cf5;
unsigned long ia_match_cf1, ia_match_cf2, ia_match_cf3, ia_match_cf4;
unsigned long iv_match_cf1, iv_match_cf2;
unsigned long ida_match_cf1, ida_match_cf2;

static int da_match_show(struct seq_file *m, void *v)
{

	seq_printf(m, "%s", da_match_buf);
	return 0;
}

static int dv_match_show(struct seq_file *m, void *v)
{

	seq_printf(m, "%s", dv_match_buf);
	return 0;
}

static int dav_match_show(struct seq_file *m, void *v)
{

	seq_printf(m, "%s", dav_match_buf);
	return 0;
}

static int ia_match_show(struct seq_file *m, void *v)
{

	seq_printf(m, "%s", ia_match_buf);
	return 0;
}

static int iv_match_show(struct seq_file *m, void *v)
{

	seq_printf(m, "%s", iv_match_buf);
	return 0;
}

static int ida_match_show(struct seq_file *m, void *v)
{

	seq_printf(m, "%s", ida_match_buf);
	return 0;
}

static int da_match_open(struct inode *inode, struct file *file)
{
	return single_open(file, da_match_show, NULL);
}

static int dv_match_open(struct inode *inode, struct file *file)
{
	return single_open(file, dv_match_show, NULL);
}

static int dav_match_open(struct inode *inode, struct file *file)
{
	return single_open(file, dav_match_show, NULL);
}

static int ia_match_open(struct inode *inode, struct file *file)
{
	return single_open(file, ia_match_show, NULL);
}

static int iv_match_open(struct inode *inode, struct file *file)
{
	return single_open(file, iv_match_show, NULL);
}

static int ida_match_open(struct inode *inode, struct file *file)
{
	return single_open(file, ida_match_show, NULL);
}

static void
write_da_match(void *i)
{
	unsigned long dc_ctl;

	sw64_write_csr(da_match_cf1, CSR_DA_MATCH);
	sw64_write_csr(da_match_cf2, CSR_DA_MASK);
	dc_ctl = sw64_read_csr(CSR_DC_CTLP);
	dc_ctl &= ~((0x1UL << 3) | (0x3UL << DA_MATCH_EN_S)
			| (0x1UL << DAV_MATCH_EN_S) | (0x1UL << DPM_MATCH_EN_S)
			| (0x3UL << DPM_MATCH));
	dc_ctl |= da_match_cf3;
	sw64_write_csr(dc_ctl, CSR_DC_CTLP);
}

static void
write_dv_match(void *i)
{
	unsigned long dc_ctl;

	sw64_write_csr(dv_match_cf1, CSR_DV_MATCH);
	sw64_write_csr(dv_match_cf2, CSR_DV_MASK);
	dc_ctl = sw64_read_csr(CSR_DC_CTLP);
	dc_ctl &= ~((0x1UL << DAV_MATCH_EN_S) | (0x1UL << DPM_MATCH_EN_S)
			| (0x3UL << DPM_MATCH));
	dc_ctl |= ((0x1UL << DV_MATCH_EN_S) | dv_match_cf3);
	sw64_write_csr(dc_ctl, CSR_DC_CTLP);
}

static void
write_dav_match(void *i)
{
	unsigned long dc_ctl;

	sw64_write_csr(dav_match_cf1, CSR_DA_MATCH);
	sw64_write_csr(dav_match_cf2, CSR_DA_MASK);
	sw64_write_csr(dav_match_cf3, CSR_DV_MATCH);
	sw64_write_csr(dav_match_cf4, CSR_DV_MASK);
	dc_ctl = sw64_read_csr(CSR_DC_CTLP);
	dc_ctl &= ~((0x1UL << 3) | (0x3UL << DA_MATCH_EN_S)
			| (0x1UL << DPM_MATCH_EN_S) | (0x3UL << DPM_MATCH));
	dc_ctl |= ((0x1UL << DV_MATCH_EN_S) | (0x1UL << DAV_MATCH_EN_S)
			| dav_match_cf5);
	sw64_write_csr(dc_ctl, CSR_DC_CTLP);
}

static void
write_ia_match(void *i)
{
	ia_match_cf1 |= (0x1UL << IA_MATCH_EN_S);
	sw64_write_csr_imb(ia_match_cf1, CSR_IA_MATCH);
	sw64_write_csr_imb(ia_match_cf2, CSR_IA_MASK);
	sw64_write_csr(((0x3ffUL << 18) | ia_match_cf3), CSR_IA_VPNMATCH);
	sw64_write_csr(((0x3ffUL << 18) | ia_match_cf4), CSR_IA_UPNMATCH);
}

static void
write_iv_match(void *i)
{
	unsigned long ia_match_tmp;

	ia_match_tmp = sw64_read_csr(CSR_IA_MATCH);
	ia_match_tmp &= ~(0x1UL << IV_PM_EN_S);
	ia_match_tmp |= ((((iv_match_cf2 >> IV_PM_EN_S) & 0x1) << IV_PM_EN_S)
			| (iv_match_cf2 & 0x3) | (0x1UL << IV_MATCH_EN_S));
	sw64_write_csr_imb(iv_match_cf1, CSR_IV_MATCH);
	sw64_write_csr_imb(ia_match_tmp, CSR_IA_MATCH);
}

static void
write_ida_match(void *i)
{

	ida_match_cf1 |= (0x1UL << IDA_MATCH_EN_S);
	sw64_write_csr(ida_match_cf1, CSR_IDA_MATCH);
	sw64_write_csr(ida_match_cf2, CSR_IDA_MASK);
}

static ssize_t da_match_set(struct file *file, const char __user *user_buf,
			size_t len, loff_t *ppos)
{
	size_t size;
	char tmp[400];
	char *p;
	int i, m;
	const char *sep = " ";
	char tmp1[400];
	int err;
	char *ret = NULL;

	size = min(sizeof(da_match_buf) - 1, len);
	if (copy_from_user(da_match_buf, user_buf, size))
		return -EFAULT;

	da_match_buf[size] = '\0';
	strcpy(tmp, da_match_buf);
	p = tmp;

	for (i = 0 ; i < 4; i++) {
		m = i*100;
		ret = strsep(&p, sep);
		if (ret != NULL)
			strcpy(&tmp1[m], ret);

	}
	tmp1[400] = '\0';

	err = kstrtoul(&tmp1[0], 0, &da_match_cf1);
	if (err)
		return err;

	err = kstrtoul(&tmp1[100], 0, &da_match_cf2);
	if (err)
		return err;

	err = kstrtoul(&tmp1[200], 0, &da_match_cf3);
	if (err)
		return err;

	on_each_cpu(write_da_match, NULL, 1);

	return len;
}

static ssize_t dv_match_set(struct file *file, const char __user *user_buf,
			size_t len, loff_t *ppos)
{
	size_t size;
	char tmp[400];
	char *p;
	int i, m;
	const char *sep = " ";
	char tmp1[400];
	int err;
	char *ret = NULL;

	size = min(sizeof(dv_match_buf) - 1, len);
	if (copy_from_user(dv_match_buf, user_buf, size))
		return -EFAULT;

	dv_match_buf[size] = '\0';
	strcpy(tmp, dv_match_buf);
	p = tmp;

	for (i = 0 ; i < 4; i++) {
		m = i*100;
		ret = strsep(&p, sep);
		if (ret != NULL)
			strcpy(&tmp1[m], ret);

	}
	tmp1[400] = '\0';

	err = kstrtoul(&tmp1[0], 0, &dv_match_cf1);
	if (err)
		return err;

	err = kstrtoul(&tmp1[100], 0, &dv_match_cf2);
	if (err)
		return err;

	err = kstrtoul(&tmp1[200], 0, &dv_match_cf3);
	if (err)
		return err;

	on_each_cpu(write_dv_match, NULL, 1);

	return len;
}

static ssize_t dav_match_set(struct file *file, const char __user *user_buf,
			size_t len, loff_t *ppos)
{
	size_t size;
	char tmp[500];
	char *p;
	int i, m;
	const char *sep = " ";
	char tmp1[500];
	int err;
	char *ret = NULL;

	size = min(sizeof(dav_match_buf) - 1, len);
	if (copy_from_user(dav_match_buf, user_buf, size))
		return -EFAULT;

	dav_match_buf[size] = '\0';
	strcpy(tmp, dav_match_buf);
	p = tmp;

	for (i = 0 ; i < 5; i++) {
		m = i*100;
		ret = strsep(&p, sep);
		if (ret != NULL)
			strcpy(&tmp1[m], ret);

	}
	tmp1[500] = '\0';

	err = kstrtoul(&tmp1[0], 0, &dav_match_cf1);
	if (err)
		return err;

	err = kstrtoul(&tmp1[100], 0, &dav_match_cf2);
	if (err)
		return err;

	err = kstrtoul(&tmp1[200], 0, &dav_match_cf3);
	if (err)
		return err;

	err = kstrtoul(&tmp1[300], 0, &dav_match_cf4);
	if (err)
		return err;

	err = kstrtoul(&tmp1[400], 0, &dav_match_cf5);
	if (err)
		return err;

	on_each_cpu(write_dav_match, NULL, 1);
	return len;
}

static ssize_t ia_match_set(struct file *file, const char __user *user_buf,
			size_t len, loff_t *ppos)
{
	size_t size;
	char tmp[400];
	char *p;
	int i, m;
	const char *sep = " ";
	char tmp1[400];
	int err;
	char *ret = NULL;

	size = min(sizeof(ia_match_buf) - 1, len);
	if (copy_from_user(ia_match_buf, user_buf, size))
		return -EFAULT;

	ia_match_buf[size] = '\0';
	strcpy(tmp, ia_match_buf);
	p = tmp;

	for (i = 0 ; i < 4; i++) {
		m = i*100;
		ret = strsep(&p, sep);
		if (ret != NULL)
			strcpy(&tmp1[m], ret);

	}
	tmp1[400] = '\0';

	err = kstrtoul(&tmp1[0], 0, &ia_match_cf1);
	if (err)
		return err;

	err = kstrtoul(&tmp1[100], 0, &ia_match_cf2);
	if (err)
		return err;

	err = kstrtoul(&tmp1[200], 0, &ia_match_cf3);
	if (err)
		return err;

	err = kstrtoul(&tmp1[300], 0, &ia_match_cf4);
	if (err)
		return err;

	on_each_cpu(write_ia_match, NULL, 1);
	return len;
}

static ssize_t iv_match_set(struct file *file, const char __user *user_buf,
			size_t len, loff_t *ppos)
{
	size_t size;
	char tmp[400];
	char *p;
	int i, m;
	const char *sep = " ";
	char tmp1[400];
	int err;
	char *ret = NULL;

	size = min(sizeof(ia_match_buf) - 1, len);
	if (copy_from_user(ia_match_buf, user_buf, size))
		return -EFAULT;

	ia_match_buf[size] = '\0';
	strcpy(tmp, ia_match_buf);
	p = tmp;

	for (i = 0 ; i < 4; i++) {
		m = i*100;
		ret = strsep(&p, sep);
		if (ret != NULL)
			strcpy(&tmp1[m], ret);

	}
	tmp1[400] = '\0';

	err = kstrtoul(&tmp1[0], 0, &iv_match_cf1);
	if (err)
		return err;

	err = kstrtoul(&tmp1[100], 0, &iv_match_cf2);
	if (err)
		return err;

	on_each_cpu(write_iv_match, NULL, 1);
	return len;
}


static ssize_t ida_match_set(struct file *file, const char __user *user_buf,
			size_t len, loff_t *ppos)
{
	size_t size;
	char tmp[400];
	char *p;
	int i, m;
	const char *sep = " ";
	char tmp1[400];
	int err;
	char *ret = NULL;

	size = min(sizeof(ida_match_buf) - 1, len);
	if (copy_from_user(ida_match_buf, user_buf, size))
		return -EFAULT;

	ida_match_buf[size] = '\0';
	strcpy(tmp, ida_match_buf);
	p = tmp;

	for (i = 0 ; i < 4; i++) {
		m = i*100;
		ret = strsep(&p, sep);
		if (ret != NULL)
			strcpy(&tmp1[m], ret);
	}
	tmp1[400] = '\0';

	err = kstrtoul(&tmp1[0], 0, &ida_match_cf1);
	if (err)
		return err;

	err = kstrtoul(&tmp1[100], 0, &ida_match_cf2);
	if (err)
		return err;

	on_each_cpu(write_ida_match, NULL, 1);

	return len;
}

static const struct file_operations set_da_match_fops = {
	.open		= da_match_open,
	.read		= seq_read,
	.write		= da_match_set,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations set_dv_match_fops = {
	.open		= dv_match_open,
	.read		= seq_read,
	.write		= dv_match_set,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations set_dav_match_fops = {
	.open		= dav_match_open,
	.read		= seq_read,
	.write		= dav_match_set,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations set_ia_match_fops = {
	.open		= ia_match_open,
	.read		= seq_read,
	.write		= ia_match_set,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations set_iv_match_fops = {
	.open		= iv_match_open,
	.read		= seq_read,
	.write		= iv_match_set,
	.llseek		= seq_lseek,
	.release	= single_release,
};


static const struct file_operations set_ida_match_fops = {
	.open		= ida_match_open,
	.read		= seq_read,
	.write		= ida_match_set,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init match_debugfs_init(void)
{
	struct dentry *match_entry;

	if (!sw64_debugfs_dir)
		return -ENODEV;

	match_entry = debugfs_create_file("da_match", 0600,
					sw64_debugfs_dir, NULL,
					&set_da_match_fops);
	if (!match_entry)
		return -ENOMEM;

	match_entry = debugfs_create_file("dv_match", 0600,
					sw64_debugfs_dir, NULL,
					&set_dv_match_fops);
	if (!match_entry)
		return -ENOMEM;

	match_entry = debugfs_create_file("dav_match", 0600,
					sw64_debugfs_dir, NULL,
					&set_dav_match_fops);
	if (!match_entry)
		return -ENOMEM;

	match_entry = debugfs_create_file("ia_match", 0600,
					sw64_debugfs_dir, NULL,
					&set_ia_match_fops);
	if (!match_entry)
		return -ENOMEM;

	match_entry = debugfs_create_file("iv_match", 0600,
					sw64_debugfs_dir, NULL,
					&set_iv_match_fops);
	if (!match_entry)
		return -ENOMEM;

	match_entry = debugfs_create_file("ida_match", 0600,
					sw64_debugfs_dir, NULL,
					&set_ida_match_fops);
	if (!match_entry)
		return -ENOMEM;

	return 0;
}
late_initcall(match_debugfs_init);
