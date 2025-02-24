// SPDX-License-Identifier: GPL-2.0

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/of.h>
#include <linux/iopoll.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <asm/se.h>
#include <linux/list.h>

#define SE_SDF_BUFSIZE			(PAGE_SIZE * 2)
#define SDF_OPENSESSION			0x204
#define SDF_CLOSESESSION		0x205

struct lsse_sdf_dev {
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

#define KERNEL_COMMAND_SIZE	(sizeof(struct sdf_kernel_command))

struct sdf_handle {
	struct list_head handle_list;
	void *handle;
};

struct sdf_file_pvt_data {
	struct lsse_sdf_dev *se;
	struct list_head handle_list;
	struct sdf_kernel_command skc;
	struct sdf_handle *ph;
};

static struct lsse_sdf_dev *se_sdf_dev;

static void lsse_sdf_complete(struct lsse_ch *ch)
{
	struct lsse_sdf_dev *se = (struct lsse_sdf_dev *)ch->priv;

	se->processing_cmd = false;
	wake_up(&se->wq);
}

static int se_send_sdf_cmd(struct lsse_sdf_dev *se, int len, int retry)
{
	struct se_sdf_msg *smsg = (struct se_sdf_msg *)se->se_ch->smsg;
	unsigned long flag;
	int err;

	spin_lock_irqsave(&se->se_ch->ch_lock, flag);

	smsg->cmd = SE_CMD_SDF;
	/* One time one cmd */
	smsg->data_off = se->se_ch->data_buffer - se->se_ch->se->mem_base;
	smsg->data_len = len;

try_again:
	if (!retry--)
		goto out;

	pr_debug("Send sdf cmd, last retry %d times\n", retry);

	err = se_send_ch_request(se->se_ch);
	if (err) {
		udelay(5);
		goto try_again;
	}

out:
	spin_unlock_irqrestore(&se->se_ch->ch_lock, flag);

	return err;
}

static int lsse_sdf_recv(struct sdf_file_pvt_data *pvt, char *buf,
		size_t size, int user, int *se_ret)
{
	int len, time, ret = 0;
	struct se_sdf_msg *rmsg;
	struct sdf_kernel_command *skc;
	struct sdf_handle *ph;
	struct lsse_sdf_dev *se = pvt->se;

	if (!se->se_ch->rmsg) {
		pr_err("se device is not ready\n");
		return -EBUSY;
	}

	time = wait_event_timeout(se->wq, !se->processing_cmd, HZ*30);
	if (!time)
		return -ETIME;

	rmsg = (struct se_sdf_msg *)se->se_ch->rmsg;
	if (rmsg->cmd != SE_CMD_SDF) {
		pr_err("se get wrong response\n");
		return -EIO;
	}
	len = rmsg->data_len;

	if ((!user && len > KERNEL_COMMAND_SIZE) || len > SE_SDF_BUFSIZE
			|| (size && len > size))
		return -E2BIG;

	if (user) {
		ret = copy_to_user((char __user *)buf,
				se->se_ch->data_buffer + rmsg->data_off, len);
		if (!se_ret)
			return ret;

		skc = (struct sdf_kernel_command *)
			(se->se_ch->data_buffer + rmsg->data_off);
		*se_ret = skc->header.u.ret;
		if (skc->header.command == SDF_OPENSESSION && !*se_ret) {
			ph = kmalloc(sizeof(*ph), GFP_KERNEL);
			if (!ph)
				return -ENOMEM;
			ph->handle = skc->handle;
			list_add(&ph->handle_list, &pvt->handle_list);
		}
	} else
		memcpy(buf, se->se_ch->data_buffer + rmsg->data_off, len);

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

static int lsse_sdf_send(struct sdf_file_pvt_data *pvt, const char *buf,
		size_t count, int user)
{
	int ret, se_ret;
	struct sdf_handle *ph = NULL;
	struct sdf_kernel_command *skc;
	struct lsse_sdf_dev *se = pvt->se;

	if (!se->se_ch->smsg) {
		pr_err("se device is not ready\n");
		return 0;
	}

	if (count > se->se_ch->data_size) {
		pr_err("Invalid size in send: count=%zd, size=%d\n",
			count, se->se_ch->data_size);
		return -EIO;
	}

	if (user) {
		ret = mutex_lock_interruptible(&se->data_lock);
		if (ret)
			goto out;
	} else
		mutex_lock(&se->data_lock);

	if (user) {
		ret = copy_from_user(se->se_ch->data_buffer, buf, count);
		if (ret) {
			ret = -EFAULT;
			goto out_unlock;
		}
		skc = (struct sdf_kernel_command *)se->se_ch->data_buffer;
		if (skc->header.command == SDF_CLOSESESSION)
			ph = find_sdf_handle(skc->handle, pvt);
	} else
		memcpy(se->se_ch->data_buffer, buf, count);

	se->processing_cmd = true;
	ret = se_send_sdf_cmd(se, count, 5);
	if (ret) {
		pr_err("se_send_sdf_cmd failed\n");
		goto out_unlock;
	}

	ret = lsse_sdf_recv(pvt, (char *)buf, 0, user, &se_ret);
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
out:
	return ret;
}

static ssize_t lsse_sdf_write(struct file *filp, const char __user *buf,
		size_t cnt, loff_t *offt)
{
	struct sdf_file_pvt_data *pvt = filp->private_data;

	if (cnt > SE_SDF_BUFSIZE)
		return -E2BIG;

	if (lsse_sdf_send(pvt, buf, cnt, 1))
		return -EFAULT;

	return cnt;
}

static ssize_t lsse_sdf_read(struct file *filp, char __user *buf,
			size_t size, loff_t *off)
{
	return lsse_sdf_recv(filp->private_data, buf, size, 1, NULL);
}

static int close_one_handle(struct sdf_file_pvt_data *pvt, struct sdf_handle *ph)
{
	struct sdf_kernel_command *skc = &pvt->skc;

	skc->header.command = 0x205;
	skc->header.u.param_cnt = 1;
	skc->header.param_len[0] = 8;
	skc->handle = ph->handle;
	/* close one session */
	lsse_sdf_send(pvt, (char *)&pvt->skc, KERNEL_COMMAND_SIZE, 0);
	if (skc->header.u.ret) {
		pr_err("Auto Close Session failed, session handle: %llx, ret: %d\n",
				(u64)ph->handle, skc->header.u.ret);
		return skc->header.u.ret;
	}
	kfree(ph);

	return 0;
}

static int close_all_handle(struct sdf_file_pvt_data *pvt)
{
	int ret = 0;
	struct sdf_handle *ph, *tmp;

	list_for_each_entry_safe(ph, tmp, &pvt->handle_list, handle_list) {
		list_del(&ph->handle_list);
		ret = close_one_handle(pvt, ph);
		if (ret)
			return ret;
	}

	return 0;
}

static int lsse_sdf_release(struct inode *inode, struct file *filp)
{
	int ret;
	struct sdf_file_pvt_data *pvt = filp->private_data;

	ret = close_all_handle(pvt);
	filp->private_data = NULL;
	kfree(pvt);

	if (ret)
		ret = -EFAULT;
	return ret;
}

static int lsse_sdf_open(struct inode *inode, struct file *filp)
{
	struct sdf_file_pvt_data *pvt = kmalloc(sizeof(*pvt), GFP_KERNEL);

	if (!pvt)
		return -ENOMEM;

	INIT_LIST_HEAD(&pvt->handle_list);
	pvt->se = se_sdf_dev;
	filp->private_data = pvt;

	return 0;
}

static const struct file_operations lsse_sdf_fops = {
	.owner = THIS_MODULE,
	.open = lsse_sdf_open,
	.write = lsse_sdf_write,
	.read = lsse_sdf_read,
	.release = lsse_sdf_release,
};

static struct miscdevice lsse_sdf_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lsse_sdf",
	.fops = &lsse_sdf_fops,
};

static int lsse_sdf_probe(struct platform_device *pdev)
{
	int msg_size;
	int ret;

	se_sdf_dev = kzalloc(sizeof(*se_sdf_dev), GFP_KERNEL);
	if (IS_ERR_OR_NULL(se_sdf_dev))
		return PTR_ERR(se_sdf_dev);

	mutex_init(&se_sdf_dev->data_lock);
	init_waitqueue_head(&se_sdf_dev->wq);
	se_sdf_dev->processing_cmd = false;

	msg_size = 2 * sizeof(struct se_sdf_msg);
	se_sdf_dev->se_ch = se_init_ch(SE_CH_SDF, SE_SDF_BUFSIZE, msg_size,
			se_sdf_dev, lsse_sdf_complete);

	ret = misc_register(&lsse_sdf_miscdev);
	if (ret < 0) {
		pr_err("register sdf dev failed!\n");
		goto out;
	}

	return 0;

out:
	kfree(se_sdf_dev);

	return ret;
}

static int lsse_sdf_remove(struct platform_device *pdev)
{
	misc_deregister(&lsse_sdf_miscdev);
	se_deinit_ch(se_sdf_dev->se_ch);
	kfree(se_sdf_dev);

	return 0;
}

static struct platform_driver loongson_sdf_driver = {
	.probe	= lsse_sdf_probe,
	.remove	= lsse_sdf_remove,
	.driver  = {
		.name  = "loongson-sdf",
	},
};
module_platform_driver(loongson_sdf_driver);

MODULE_ALIAS("platform:loongson-sdf");
MODULE_AUTHOR("Yinggang Gu");
MODULE_DESCRIPTION("Loongson SE sdf driver");
MODULE_LICENSE("GPL");
