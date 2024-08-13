// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include <linux/proc_fs.h>

#include "ne6x.h"
#include "ne6x_reg.h"
#include "ne6x_dev.h"

static struct proc_dir_entry *ne6x_proc_root;

ssize_t ne6x_proc_tps_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	struct ne6x_soc_temperature temp = {0};
	struct ne6x_soc_power power = {0};
	struct device *dev = NULL;
	struct ne6x_pf *pf = NULL;
	char *info = NULL;
	ssize_t len = 0;
	int err;

	if (*ppos > 0 || count < PAGE_SIZE)
		return 0;

	info = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	pf = filp->private_data;
	dev = &pf->pdev->dev;
	err = ne6x_dev_get_temperature_info(pf, &temp);
	if (err) {
		dev_err(dev, "get device temperature failed\n");
	} else {
		len += sprintf(info, "Chip temperature  (°C)  %d\n", temp.chip_temerature);
		len += sprintf(info + len, "Nic temerature    (°C)  %d\n", temp.board_temperature);
	}

	err = ne6x_dev_get_power_consum(pf, &power);
	if (err) {
		dev_err(dev, "get device power failed\n");
	} else {
		len += sprintf(info + len, "Current           (A)   %d.%03d\n",
			       power.cur / 1000, power.cur % 1000);
		len += sprintf(info + len, "Voltage           (V)   %d.%03d\n",
			       power.vol / 1000, power.vol % 1000);
		len += sprintf(info + len, "Power             (W)   %d.%03d\n",
			       power.power / 1000, power.power % 1000);
	}

	if (!len) {
		kfree(info);
		return len;
	}

	if (copy_to_user(buf, info, len)) {
		kfree(info);
		return -EFAULT;
	}

	*ppos = len;
	kfree(info);
	return len;
}

ssize_t ne6x_proc_i2c_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	struct device *dev = NULL;
	struct ne6x_pf *pf = NULL;
	char info[512] = {0};
	ssize_t len = 0;
	u32 id = 0;
	int err;

	if (*ppos > 0 || count < 512)
		return 0;

	pf = filp->private_data;
	dev = &pf->pdev->dev;
	err = ne6x_dev_i2c3_signal_test(pf, &id);
	if (err)
		dev_err(dev, "get device i2c external info failed\n");
	else
		len += sprintf(info, "I2c external sig test    %d\n", id & 0xff);

	if (!len)
		return len;

	if (copy_to_user(buf, info, len))
		return -EFAULT;

	*ppos = len;
	return len;
}

static int ne6x_tps_open(struct inode *inode, struct file *file)
{
	file->private_data = pde_data(inode);

	return 0;
}

static int ne6x_i2c_open(struct inode *inode, struct file *file)
{
	file->private_data = pde_data(inode);

	return 0;
}

static const struct proc_ops ne6x_proc_tps_fops = {
	.proc_open = ne6x_tps_open,
	.proc_read = ne6x_proc_tps_read,
};

static const struct proc_ops ne6x_proc_i2c_fops = {
	.proc_open = ne6x_i2c_open,
	.proc_read = ne6x_proc_i2c_read,
};

void ne6x_proc_pf_init(struct ne6x_pf *pf)
{
	struct proc_dir_entry *pfile = NULL;
	const struct device *dev = NULL;
	const char *name = NULL;

	name = pci_name(pf->pdev);
	dev = &pf->pdev->dev;
	pf->ne6x_proc_pf = proc_mkdir(name, ne6x_proc_root);
	if (!pf->ne6x_proc_pf) {
		dev_err(dev, "proc dir %s create failed\n", name);
		return;
	}

	pfile = proc_create_data("temperature_power_state", 0600, pf->ne6x_proc_pf,
				 &ne6x_proc_tps_fops, pf);
	if (!pfile) {
		dev_err(dev, "proc file temperature_power_state create failed\n");
		goto create_failed;
	}

	pfile = proc_create_data("i2c_test", 0600, pf->ne6x_proc_pf, &ne6x_proc_i2c_fops, pf);
	if (!pfile) {
		dev_err(dev, "proc file i2c_test create failed\n");
		goto create_failed;
	}

	return;

create_failed:
	proc_remove(pf->ne6x_proc_pf);
}

void ne6x_proc_pf_exit(struct ne6x_pf *pf)
{
	proc_remove(pf->ne6x_proc_pf);
	pf->ne6x_proc_pf = NULL;
}

extern char ne6x_driver_name[];
void ne6x_proc_init(void)
{
	ne6x_proc_root = proc_mkdir(ne6x_driver_name, NULL);
	if (!ne6x_proc_root)
		pr_info("init of proc failed\n");
}

void ne6x_proc_exit(void)
{
	proc_remove(ne6x_proc_root);
	ne6x_proc_root = NULL;
}
