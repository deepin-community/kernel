// SPDX-License-Identifier: GPL-2.0

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/iopoll.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <asm/se.h>

static int se_mem_size = 0x800000;
module_param(se_mem_size, int, 0444);
MODULE_PARM_DESC(se_mem_size, "LOONGSON SE shared memory size");

static int se_mem_page = PAGE_SIZE;
module_param(se_mem_page, int, 0444);
MODULE_PARM_DESC(se_mem_page, "LOONGSON SE shared memory page size");

static struct loongson_se se_dev;

static int lsse_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t lsse_write(struct file *filp, const char __user *buf,
		size_t cnt, loff_t *offt)
{
	return 0;
}

static const struct file_operations lsse_fops = {
	.owner = THIS_MODULE,
	.open = lsse_open,
	.write = lsse_write,
};

static struct miscdevice lsse_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "loongson-se",
	.fops = &lsse_fops,
};

static inline u32 se_readl(u64 addr)
{
	return readl(se_dev.base + addr);
}

static inline void se_writel(u32 val, u64 addr)
{
	writel(val, se_dev.base + addr);
}

static inline bool se_ch_status(struct loongson_se *se, u32 int_bit)
{
	return !!(se->ch_status & int_bit) == 1;
}

static void se_enable_int(struct loongson_se *se, u32 int_bit)
{
	unsigned long flag;
	u32 tmp;

	if (!int_bit)
		return;

	spin_lock_irqsave(&se->dev_lock, flag);

	tmp = se_readl(SE_S2LINT_EN);
	tmp |= int_bit;
	se_writel(tmp, SE_S2LINT_EN);

	spin_unlock_irqrestore(&se->dev_lock, flag);
}

static void se_disable_int(struct loongson_se *se, u32 int_bit)
{
	unsigned long flag;
	u32 tmp;

	if (!int_bit)
		return;

	spin_lock_irqsave(&se->dev_lock, flag);

	tmp = se_readl(SE_S2LINT_EN);
	tmp &= ~(int_bit);
	se_writel(tmp, SE_S2LINT_EN);

	spin_unlock_irqrestore(&se->dev_lock, flag);
}

static int se_send_request(struct loongson_se *se,
		struct se_mailbox_data *req)
{
	unsigned long flag;
	u32 status;
	int err = 0;
	int i;

	if (!se || !req)
		return -EINVAL;

	if (se_readl(SE_L2SINT_STAT) ||
			!(se_readl(SE_L2SINT_EN) & req->int_bit))
		return -EBUSY;

	spin_lock_irqsave(&se->cmd_lock, flag);

	for (i = 0; i < ARRAY_SIZE(req->u.mailbox); i++)
		se_writel(req->u.mailbox[i], SE_MAILBOX_S + i * 4);

	se_writel(req->int_bit, SE_L2SINT_SET);

	err = readl_relaxed_poll_timeout_atomic(se->base + SE_L2SINT_STAT, status,
				!(status & req->int_bit), 10, 10000);

	spin_unlock_irqrestore(&se->cmd_lock, flag);

	return err;
}

static int se_get_response(struct loongson_se *se,
		struct se_mailbox_data *res)
{
	unsigned long flag;
	int i;

	if (!se || !res)
		return -EINVAL;

	if ((se_readl(SE_S2LINT_STAT) & res->int_bit) == 0)
		return -EBUSY;

	spin_lock_irqsave(&se->cmd_lock, flag);

	for (i = 0; i < ARRAY_SIZE(res->u.mailbox); i++)
		res->u.mailbox[i] = se_readl(SE_MAILBOX_L + i * 4);

	se_writel(res->int_bit, SE_S2LINT_CL);

	spin_unlock_irqrestore(&se->cmd_lock, flag);

	return 0;
}

static int loongson_se_get_res(struct loongson_se *se, u32 int_bit, u32 cmd,
		struct se_mailbox_data *res)
{
	int err = 0;

	res->int_bit = int_bit;

	if (se_get_response(se, res)) {
		dev_err(se->dev, "Int 0x%x get response fail.\n", int_bit);
		return -EFAULT;
	}

	/* Check response */
	if (res->u.res.cmd == cmd)
		err = 0;
	else {
		dev_err(se->dev, "Response cmd is 0x%x, not expect cmd 0x%x.\n",
				res->u.res.cmd, cmd);
		err = -EFAULT;
	}

	return err;
}

static int se_send_genl_cmd(struct loongson_se *se, struct se_mailbox_data *req,
		struct se_mailbox_data *res, int retry)
{
	int err = 0, cnt = 0;

	for (cnt = 0; cnt < retry; cnt++) {
		dev_dbg(se->dev, "%d time send cmd 0x%x\n", cnt + 1, req->u.gcmd.cmd);

		err = se_send_request(se, req);
		if (err)
			continue;

		if (!wait_for_completion_timeout(&se->cmd_completion,
					msecs_to_jiffies(0x1000))) {
			se_enable_int(se, req->int_bit);
			continue;
		}

		err = loongson_se_get_res(se, req->int_bit, req->u.gcmd.cmd, res);
		if (err || res->u.res.cmd_ret) {
			se_enable_int(se, req->int_bit);
			continue;
		}
		break;
	}

	if (cnt >= retry)
		err = -ETIMEDOUT;

	se_enable_int(se, req->int_bit);
	return err;
}

static int loongson_se_set_msg(struct lsse_ch *ch)
{
	struct loongson_se *se = ch->se;
	struct se_mailbox_data req = {0};
	struct se_mailbox_data res = {0};
	int err;

	req.int_bit = SE_INT_SETUP;
	req.u.gcmd.cmd = SE_CMD_SETMSG;
	/* MSG off */
	req.u.gcmd.info[0] = ch->id;
	req.u.gcmd.info[1] = ch->smsg - se->mem_base;
	req.u.gcmd.info[2] = ch->msg_size;

	dev_dbg(se->dev, "Set Channel %d msg off 0x%x, msg size %d\n", ch->id,
			req.u.gcmd.info[1], req.u.gcmd.info[2]);

	err = se_send_genl_cmd(se, &req, &res, 5);
	if (res.u.res.cmd_ret)
		return res.u.res.cmd_ret;

	return err;
}

static irqreturn_t loongson_se_irq(int irq, void *dev_id)
{
	struct loongson_se *se = (struct loongson_se *)dev_id;
	struct lsse_ch *ch;
	u32 int_status;

	int_status = se_readl(SE_S2LINT_STAT);

	dev_dbg(se->dev, "%s int status is 0x%x\n", __func__, int_status);

	se_disable_int(se, int_status);

	if (int_status & SE_INT_SETUP) {
		complete(&se->cmd_completion);
		int_status &= ~SE_INT_SETUP;
	}

	while (int_status) {
		int id = __ffs(int_status);

		ch = &se->chs[id];
		if (ch->complete)
			ch->complete(ch);
		int_status &= ~BIT(id);
		se_writel(BIT(id), SE_S2LINT_CL);
	}

	return IRQ_HANDLED;
}

static int se_init_hw(struct loongson_se *se)
{
	struct se_mailbox_data req = {0};
	struct se_mailbox_data res = {0};
	struct device *dev = se->dev;
	int err, retry = 5;
	u64 size;

	size = se_mem_size;

	if (size & (size - 1)) {
		size = roundup_pow_of_two(size);
		se_mem_size = size;
	}

	se_enable_int(se, SE_INT_SETUP);

	/* Start engine */
	memset(&req, 0, sizeof(struct se_mailbox_data));
	memset(&res, 0, sizeof(struct se_mailbox_data));
	req.int_bit = SE_INT_SETUP;
	req.u.gcmd.cmd = SE_CMD_START;
	err = se_send_genl_cmd(se, &req, &res, retry);
	if (err)
		return err;

	/* Get Version */
	memset(&req, 0, sizeof(struct se_mailbox_data));
	memset(&res, 0, sizeof(struct se_mailbox_data));
	req.int_bit = SE_INT_SETUP;
	req.u.gcmd.cmd = SE_CMD_GETVER;
	err = se_send_genl_cmd(se, &req, &res, retry);
	if (err)
		return err;

	se->version = res.u.res.info[0];

	/* Setup data buffer */
	se->mem_base = dmam_alloc_coherent(dev, size,
			&se->mem_addr, GFP_KERNEL);
	if (!se->mem_base)
		return -ENOMEM;

	memset(se->mem_base, 0, size);

	memset(&req, 0, sizeof(struct se_mailbox_data));
	memset(&res, 0, sizeof(struct se_mailbox_data));
	req.int_bit = SE_INT_SETUP;
	req.u.gcmd.cmd = SE_CMD_SETBUF;
	/* MMAP */
	req.u.gcmd.info[0] = (se->mem_addr & 0xffffffff) | 0x80;
	req.u.gcmd.info[1] = se->mem_addr >> 32;
	/* MASK */
	req.u.gcmd.info[2] = ~(size - 1);
	req.u.gcmd.info[3] = 0xffffffff;

	pr_debug("Set win mmap 0x%llx, mask 0x%llx\n",
			((u64)req.u.gcmd.info[1] << 32) | req.u.gcmd.info[0],
			((u64)req.u.gcmd.info[3] << 32) | req.u.gcmd.info[2]);

	err = se_send_genl_cmd(se, &req, &res, retry);
	if (err)
		return err;

	se->mem_map_size = size / se_mem_page;
	se->mem_map = bitmap_zalloc(se->mem_map_size, GFP_KERNEL);
	if (!se->mem_map)
		return -ENOMEM;

	dev_info(se->dev, "SE module setup down, shared memory size is 0x%x bytes, memory page size is 0x%x bytes\n",
					se_mem_size, se_mem_page);

	return err;
}

static void loongson_se_disable_hw(struct loongson_se *se)
{
	struct se_mailbox_data req = {0};
	struct se_mailbox_data res = {0};
	int retry = 5;

	/* Stop engine */
	req.int_bit = SE_INT_SETUP;
	req.u.gcmd.cmd = SE_CMD_STOP;
	se_send_genl_cmd(se, &req, &res, retry);

	se_disable_int(se, SE_INT_ALL);
	kfree(se->mem_map);
}

int se_send_ch_request(struct lsse_ch *ch)
{
	struct loongson_se *se;
	u32 status, int_bit;
	int err = 0;

	if (!ch)
		return -EINVAL;

	se = ch->se;
	int_bit = ch->int_bit;

	if ((se_readl(SE_L2SINT_STAT) & int_bit) ||
			!(se_readl(SE_L2SINT_EN) & int_bit))
		return -EBUSY;

	se_enable_int(se, int_bit);
	se_writel(int_bit, SE_L2SINT_SET);

	err = readl_relaxed_poll_timeout_atomic(se->base + SE_L2SINT_STAT, status,
				!(status & int_bit), 10, 10000);

	return err;
}
EXPORT_SYMBOL_GPL(se_send_ch_request);

struct lsse_ch *se_init_ch(int id, int data_size, int msg_size, void *priv,
		void (*complete)(struct lsse_ch *se_ch))
{
	struct loongson_se *se = &se_dev;
	struct lsse_ch *ch;
	unsigned long flag;
	int data_first, data_nr;
	int msg_first, msg_nr;

	if (!se) {
		pr_err("SE has not been initialized\n");
		return NULL;
	}

	if (id == 0 || id > SE_CH_MAX) {
		dev_err(se->dev, "Channel number %d is invalid\n", id);
		return NULL;
	}

	if (se_ch_status(se, BIT(id))) {
		dev_err(se->dev, "Channel number %d has been initialized\n", id);
		return NULL;
	}

	spin_lock_irqsave(&se->dev_lock, flag);

	ch = &se_dev.chs[id];
	ch->se = se;
	ch->id = id;
	ch->int_bit = BIT(id);
	se->ch_status |= BIT(id);

	data_nr = round_up(data_size, se_mem_page) / se_mem_page;
	data_first = bitmap_find_next_zero_area(se->mem_map, se->mem_map_size,
			0, data_nr, 0);
	if (data_first >= se->mem_map_size) {
		dev_err(se->dev, "Insufficient memory space\n");
		spin_unlock_irqrestore(&se->dev_lock, flag);
		return NULL;
	}

	bitmap_set(se->mem_map, data_first, data_nr);
	ch->data_buffer = se->mem_base + data_first * se_mem_page;
	ch->data_addr = se->mem_addr + data_first * se_mem_page;
	ch->data_size = data_size;

	msg_nr = round_up(msg_size, se_mem_page) / se_mem_page;
	msg_first = bitmap_find_next_zero_area(se->mem_map, se->mem_map_size,
			0, msg_nr, 0);
	if (msg_first >= se->mem_map_size) {
		dev_err(se->dev, "Insufficient memory space\n");
		bitmap_clear(se->mem_map, data_first, data_nr);
		spin_unlock_irqrestore(&se->dev_lock, flag);
		return NULL;
	}

	bitmap_set(se->mem_map, msg_first, msg_nr);
	ch->smsg = se->mem_base + msg_first * se_mem_page;
	ch->rmsg = ch->smsg + msg_size / 2;
	ch->msg_size = msg_size;

	ch->complete = complete;
	ch->priv = priv;

	spin_lock_init(&ch->ch_lock);

	spin_unlock_irqrestore(&se->dev_lock, flag);

	if (loongson_se_set_msg(ch)) {
		dev_err(se->dev, "Channel %d setup message address failed\n", id);
		return NULL;
	}

	se_enable_int(se, ch->int_bit);

	return ch;
}
EXPORT_SYMBOL_GPL(se_init_ch);

void se_deinit_ch(struct lsse_ch *ch)
{
	struct loongson_se *se = &se_dev;
	unsigned long flag;
	int first, nr;
	int id = ch->id;

	if (!se) {
		pr_err("SE has bot been initialized\n");
		return;
	}

	if (id == 0 || id > SE_CH_MAX) {
		dev_err(se->dev, "Channel number %d is invalid\n", id);
		return;
	}

	if (!se_ch_status(se, BIT(id))) {
		dev_err(se->dev, "Channel number %d has not been initialized\n", id);
		return;
	}

	spin_lock_irqsave(&se->dev_lock, flag);

	se->ch_status &= ~BIT(ch->id);

	first = (ch->data_buffer - se->mem_base) / se_mem_page;
	nr = round_up(ch->data_size, se_mem_page) / se_mem_page;
	bitmap_clear(se->mem_map, first, nr);

	first = (ch->smsg - se->mem_base) / se_mem_page;
	nr = round_up(ch->msg_size, se_mem_page) / se_mem_page;
	bitmap_clear(se->mem_map, first, nr);

	spin_unlock_irqrestore(&se->dev_lock, flag);

	se_disable_int(se, ch->int_bit);
}
EXPORT_SYMBOL_GPL(se_deinit_ch);

static struct platform_device lsse_sdf_pdev = {
	.name	= "loongson-sdf",
	.id	= -1,
};

static const struct of_device_id loongson_se_of_match[] = {
	{ .compatible = "loongson,ls3c6000se", },
	{}
};
MODULE_DEVICE_TABLE(of, loongson_se_of_match);

static int loongson_se_probe(struct platform_device *pdev)
{
	struct loongson_se *se = &se_dev;
	struct resource *res;
	struct device *dev = &pdev->dev;
	int nr_irq, err, i;
	int irq[8];

	nr_irq = platform_irq_count(pdev);
	if (nr_irq < 0)
		return -ENODEV;

	for (i = 0; i < nr_irq; i++) {
		irq[i] = platform_get_irq(pdev, i);
		if (irq[i] < 0)
			return -ENODEV;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	se->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(se->base))
		return PTR_ERR(se->base);

	se->dev = &pdev->dev;
	platform_set_drvdata(pdev, se);
	init_completion(&se->cmd_completion);
	spin_lock_init(&se->cmd_lock);
	spin_lock_init(&se->dev_lock);

	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (err)
		return err;

	for (i = 0; i < nr_irq; i++) {
		err = devm_request_irq(dev, irq[i], loongson_se_irq, 0,
				"loongson-se", se);
		if (err)
			goto out;
	}

	err = se_init_hw(se);
	if (err)
		goto disable_hw;

	err = misc_register(&lsse_miscdev);
	if (err)
		goto disable_hw;

	err = platform_device_register(&lsse_sdf_pdev);
	if (err)
		pr_err("register sdf device failed\n");

	return 0;

disable_hw:
	loongson_se_disable_hw(se);
out:
	for ( ; i >= 0; i--)
		devm_free_irq(dev, irq[i], se);

	return err;
}

static int loongson_se_remove(struct platform_device *pdev)
{
	struct loongson_se *se = platform_get_drvdata(pdev);

	misc_deregister(&lsse_miscdev);
	loongson_se_disable_hw(se);
	platform_device_unregister(&lsse_sdf_pdev);

	return 0;
}

static struct platform_driver loongson_se_driver = {
	.probe   = loongson_se_probe,
	.remove  = loongson_se_remove,
	.driver  = {
		.name  = "loongson-se",
		.of_match_table = loongson_se_of_match,
	},
};

module_platform_driver(loongson_se_driver);

MODULE_AUTHOR("Yinggang Gu");
MODULE_DESCRIPTION("Loongson SE driver");
MODULE_LICENSE("GPL");
