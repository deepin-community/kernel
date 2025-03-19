// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Loongson Technology Corporation Limited
 */

#include <linux/acpi.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <soc/loongson/se.h>

#define SE_SDF_BUFSIZE			(PAGE_SIZE * 2)
#define SDF_OPENSESSION			(0x204)
#define SDF_CLOSESESSION		(0x205)

struct sdf_dev {
	struct miscdevice miscdev;
	struct lsse_ch *se_ch;
	struct mutex data_lock;
	bool processing_cmd;

	/* Synchronous CMD */
	wait_queue_head_t wq;
};

struct se_sdf_msg {
	u32 cmd;
	u32 data_off;
	u32 data_len;
	u32 info[5];
};

struct sdf_command_header {
	int command;
	union {
		int param_cnt;
		int ret;
	} u;
	int param_len[14];
};

struct sdf_kernel_command {
	struct sdf_command_header header;
	void *handle;
};

struct sdf_handle {
	struct list_head handle_list;
	void *handle;
};

struct sdf_file_pvt_data {
	struct sdf_dev *se;
	struct list_head handle_list;
	struct sdf_kernel_command skc;
	struct sdf_handle *ph;
};

static void sdf_complete(struct lsse_ch *ch)
{
	struct sdf_dev *se = (struct sdf_dev *)ch->priv;

	se->processing_cmd = false;
	wake_up(&se->wq);
}

static int se_send_sdf_cmd(struct sdf_dev *se, int len, int retry)
{
	struct se_sdf_msg *smsg = (struct se_sdf_msg *)se->se_ch->smsg;
	int err;

	spin_lock_irq(&se->se_ch->ch_lock);

	smsg->cmd = SE_CMD_SDF;
	/* One time one cmd */
	smsg->data_off = se->se_ch->data_buffer - se->se_ch->se->mem_base;
	smsg->data_len = len;

try_again:
	if (!retry--)
		goto out;

	err = se_send_ch_request(se->se_ch);
	if (err) {
		udelay(5);
		goto try_again;
	}

out:
	spin_unlock_irq(&se->se_ch->ch_lock);

	return err;
}

static int sdf_recvu(struct sdf_file_pvt_data *pvt, char __user *buf, int *se_ret)
{
	struct sdf_dev *se = pvt->se;
	struct sdf_kernel_command *skc;
	struct se_sdf_msg *rmsg;
	struct sdf_handle *ph;
	int ret;

	if (!wait_event_timeout(se->wq, !se->processing_cmd, HZ*10))
		return -ETIME;

	rmsg = (struct se_sdf_msg *)se->se_ch->rmsg;
	if (rmsg->cmd != SE_CMD_SDF) {
		pr_err("se get wrong response\n");
		return -EIO;
	}

	ret = copy_to_user((char __user *)buf,
			   se->se_ch->data_buffer + rmsg->data_off, rmsg->data_len);

	skc = (struct sdf_kernel_command *)(se->se_ch->data_buffer + rmsg->data_off);
	*se_ret = skc->header.u.ret;
	if (skc->header.command == SDF_OPENSESSION && !*se_ret) {
		ph = kmalloc(sizeof(*ph), GFP_KERNEL);
		if (!ph)
			return -ENOMEM;
		ph->handle = skc->handle;
		list_add(&ph->handle_list, &pvt->handle_list);
	}

	return ret;
}

static struct sdf_handle *find_sdf_handle(void *handle,
					  struct sdf_file_pvt_data *pvt)
{
	struct sdf_handle *ph;

	list_for_each_entry(ph, &pvt->handle_list, handle_list) {
		if (ph->handle == handle)
			return ph;
	}

	return NULL;
}

static int sdf_sendu(struct sdf_file_pvt_data *pvt,
		     const char __user *buf, size_t count)
{
	struct sdf_dev *se = pvt->se;
	struct sdf_kernel_command *skc;
	struct sdf_handle *ph = NULL;
	int ret, se_ret;

	mutex_lock(&se->data_lock);

	if (copy_from_user(se->se_ch->data_buffer, buf, count)) {
		ret = -EFAULT;
		goto out_unlock;
	}
	skc = (struct sdf_kernel_command *)se->se_ch->data_buffer;
	if (skc->header.command == SDF_CLOSESESSION)
		ph = find_sdf_handle(skc->handle, pvt);

	se->processing_cmd = true;
	ret = se_send_sdf_cmd(se, count, 5);
	if (ret) {
		pr_err("se_send_sdf_cmd failed\n");
		goto out_unlock;
	}

	ret = sdf_recvu(pvt, (char __user *)buf, &se_ret);
	if (ret) {
		pr_err("recv failed ret: %x\n", ret);
		goto out_unlock;
	}

	if (ph && !se_ret) {
		list_del(&ph->handle_list);
		kfree(ph);
	}

out_unlock:
	mutex_unlock(&se->data_lock);

	return ret;
}

static ssize_t sdf_write(struct file *filp, const char __user *buf,
			 size_t cnt, loff_t *offt)
{
	struct sdf_file_pvt_data *pvt = filp->private_data;

	if (cnt > SE_SDF_BUFSIZE)
		return -E2BIG;

	if (sdf_sendu(pvt, buf, cnt))
		return -EFAULT;

	return cnt;
}

static int sdf_recvk(struct sdf_file_pvt_data *pvt, char *buf)
{
	struct sdf_dev *se = pvt->se;
	struct se_sdf_msg *rmsg;
	int time;

	time = wait_event_timeout(se->wq, !se->processing_cmd, HZ*10);
	if (!time)
		return -ETIME;

	rmsg = (struct se_sdf_msg *)se->se_ch->rmsg;
	if (rmsg->cmd != SE_CMD_SDF) {
		pr_err("se get wrong response\n");
		return -EIO;
	}
	memcpy(buf, se->se_ch->data_buffer + rmsg->data_off, rmsg->data_len);

	return 0;
}

static int sdf_sendk(struct sdf_file_pvt_data *pvt, char *buf, size_t count)
{
	struct sdf_dev *se = pvt->se;
	int ret;

	mutex_lock(&se->data_lock);

	memcpy(se->se_ch->data_buffer, buf, count);
	se->processing_cmd = true;
	ret = se_send_sdf_cmd(se, count, 5);
	if (ret) {
		pr_err("se_send_sdf_cmd failed\n");
		goto out_unlock;
	}

	ret = sdf_recvk(pvt, buf);
	if (ret)
		pr_err("recv failed ret: %x\n", ret);

out_unlock:
	mutex_unlock(&se->data_lock);

	return ret;
}

static int close_one_handle(struct sdf_file_pvt_data *pvt, struct sdf_handle *ph)
{
	struct sdf_kernel_command *skc = &pvt->skc;
	int ret;

	skc->header.command = SDF_CLOSESESSION;
	skc->header.u.param_cnt = 1;
	skc->handle = ph->handle;
	skc->header.param_len[0] = sizeof(skc->handle);
	/* close one session */
	ret = sdf_sendk(pvt, (char *)&pvt->skc, sizeof(*skc));
	if (skc->header.u.ret) {
		pr_err("Auto Close Session failed, session handle: %llx, ret: %d\n",
		       (u64)ph->handle, skc->header.u.ret);
		return skc->header.u.ret;
	}
	kfree(ph);

	return ret;
}

static int close_all_handle(struct sdf_file_pvt_data *pvt)
{
	struct sdf_handle *ph, *tmp;
	int ret;

	list_for_each_entry_safe(ph, tmp, &pvt->handle_list, handle_list) {
		list_del(&ph->handle_list);
		ret = close_one_handle(pvt, ph);
		if (ret)
			return ret;
	}

	return 0;
}

static int sdf_release(struct inode *inode, struct file *filp)
{
	struct sdf_file_pvt_data *pvt = filp->private_data;
	int ret;

	ret = close_all_handle(pvt);
	kfree(pvt);

	return ret;
}

static int sdf_open(struct inode *inode, struct file *filp)
{
	struct sdf_file_pvt_data *pvt;

	pvt = kmalloc(sizeof(*pvt), GFP_KERNEL);
	if (!pvt)
		return -ENOMEM;

	INIT_LIST_HEAD(&pvt->handle_list);
	pvt->se = container_of(filp->private_data,
			       struct sdf_dev, miscdev);
	filp->private_data = pvt;

	return 0;
}

static const struct file_operations sdf_fops = {
	.owner = THIS_MODULE,
	.open = sdf_open,
	.write = sdf_write,
	.release = sdf_release,
};

static int sdf_probe(struct platform_device *pdev)
{
	struct sdf_dev *sdf;
	int msg_size, ret, ch;

	sdf = devm_kzalloc(&pdev->dev, sizeof(*sdf), GFP_KERNEL);
	if (!sdf)
		return -ENOMEM;
	mutex_init(&sdf->data_lock);
	init_waitqueue_head(&sdf->wq);
	sdf->processing_cmd = false;
	platform_set_drvdata(pdev, sdf);

	if (device_property_read_u32(&pdev->dev, "channel", &ch))
		return -ENODEV;
	msg_size = 2 * sizeof(struct se_sdf_msg);
	sdf->se_ch = se_init_ch(pdev->dev.parent, ch, SE_SDF_BUFSIZE,
				msg_size, sdf, sdf_complete);

	sdf->miscdev.minor = MISC_DYNAMIC_MINOR;
	sdf->miscdev.name = "lsse_sdf";
	sdf->miscdev.fops = &sdf_fops;
	ret = misc_register(&sdf->miscdev);
	if (ret)
		pr_err("register sdf dev failed!\n");

	return ret;
}

static int sdf_remove(struct platform_device *pdev)
{
	struct sdf_dev *sdf = platform_get_drvdata(pdev);

	misc_deregister(&sdf->miscdev);
	se_deinit_ch(sdf->se_ch);

	return 0;
}

static const struct acpi_device_id loongson_sdf_acpi_match[] = {
	{"LOON0012", 0},
	{}
};
MODULE_DEVICE_TABLE(acpi, loongson_sdf_acpi_match);

static struct platform_driver loongson_sdf_driver = {
	.probe	= sdf_probe,
	.remove	= sdf_remove,
	.driver  = {
		.name  = "loongson-sdf",
		.acpi_match_table = loongson_sdf_acpi_match,
	},
};
module_platform_driver(loongson_sdf_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Loongson Technology Corporation");
MODULE_DESCRIPTION("Loongson Secure Device Function driver");
