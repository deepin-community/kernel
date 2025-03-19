// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Loongson Technology Corporation Limited
 */

#include <linux/acpi.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <soc/loongson/se.h>

/*
 * The Loongson Security Module provides the control for hardware
 * encryption acceleration child devices. The SE framework is
 * shown as follows:
 *
 *                   +------------+
 *                   |    CPU     |
 *                   +------------+
 *			^	^
 *	            DMA |	| IRQ
 *			v	v
 *        +-----------------------------------+
 *        |     Loongson Security Module      |
 *        +-----------------------------------+
 *             ^                ^
 *    chnnel0  |       channel1 |
 *             v                v
 *        +-----------+    +----------+
 *        | sub-dev0  |    | sub-dev1 |  ..... Max sub-dev31
 *        +-----------+    +----------+
 *
 * The CPU cannot directly communicate with SE's sub devices,
 * but sends commands to SE, which processes the commands and
 * sends them to the corresponding sub devices.
 */

static inline u32 se_readl(struct loongson_se *se, u32 off)
{
	return readl(se->base + off);
}

static inline void se_writel(struct loongson_se *se, u32 val, u32 off)
{
	writel(val, se->base + off);
}

static inline bool se_ch_status(struct loongson_se *se, u32 int_bit)
{
	return !!(se->ch_status & int_bit);
}

static void se_enable_int(struct loongson_se *se, u32 int_bit)
{
	unsigned long flag;
	u32 tmp;

	spin_lock_irqsave(&se->dev_lock, flag);

	tmp = se_readl(se, SE_S2LINT_EN);
	tmp |= int_bit;
	se_writel(se, tmp, SE_S2LINT_EN);

	spin_unlock_irqrestore(&se->dev_lock, flag);
}

static void se_disable_int(struct loongson_se *se, u32 int_bit)
{
	unsigned long flag;
	u32 tmp;

	spin_lock_irqsave(&se->dev_lock, flag);

	tmp = se_readl(se, SE_S2LINT_EN);
	tmp &= ~(int_bit);
	se_writel(se, tmp, SE_S2LINT_EN);

	spin_unlock_irqrestore(&se->dev_lock, flag);
}

static int se_send_request(struct loongson_se *se, struct se_data *req)
{
	unsigned long flag;
	u32 status;
	int err;
	int i;

	if (!se || !req)
		return -EINVAL;

	if (se_readl(se, SE_L2SINT_STAT) ||
	    !(se_readl(se, SE_L2SINT_EN) & req->int_bit))
		return -EBUSY;

	spin_lock_irqsave(&se->cmd_lock, flag);

	for (i = 0; i < ARRAY_SIZE(req->u.data); i++)
		se_writel(se, req->u.data[i], SE_DATA_S + i * 4);
	se_writel(se, req->int_bit, SE_L2SINT_SET);
	err = readl_relaxed_poll_timeout_atomic(se->base + SE_L2SINT_STAT, status,
						!(status & req->int_bit), 10, 10000);

	spin_unlock_irqrestore(&se->cmd_lock, flag);

	return err;
}

static int se_get_response(struct loongson_se *se, struct se_data *res)
{
	unsigned long flag;
	int i;

	if (!se || !res)
		return -EINVAL;

	if ((se_readl(se, SE_S2LINT_STAT) & res->int_bit) == 0)
		return -EBUSY;

	spin_lock_irqsave(&se->cmd_lock, flag);

	for (i = 0; i < ARRAY_SIZE(res->u.data); i++)
		res->u.data[i] = se_readl(se, SE_DATA_L + i * 4);
	se_writel(se, res->int_bit, SE_S2LINT_CL);

	spin_unlock_irqrestore(&se->cmd_lock, flag);

	return 0;
}

static int loongson_se_get_res(struct loongson_se *se, u32 int_bit, u32 cmd,
			       struct se_data *res)
{
	res->int_bit = int_bit;

	if (se_get_response(se, res)) {
		dev_err(se->dev, "Int 0x%x get response fail.\n", int_bit);
		return -EFAULT;
	}

	/* Check response */
	if (res->u.res.cmd != cmd) {
		dev_err(se->dev, "Response cmd is 0x%x, not expect cmd 0x%x.\n",
			res->u.res.cmd, cmd);
		return -EFAULT;
	}

	return 0;
}

static int se_send_genl_cmd(struct loongson_se *se, struct se_data *req,
			    struct se_data *res, int retry)
{
	int err, cnt = 0;

try_again:
	if (cnt++ >= retry) {
		err = -ETIMEDOUT;
		goto out;
	}

	dev_dbg(se->dev, "%d time send cmd 0x%x\n", cnt, req->u.gcmd.cmd);

	err = se_send_request(se, req);
	if (err)
		goto try_again;

	if (!wait_for_completion_timeout(&se->cmd_completion, HZ)) {
		se_enable_int(se, req->int_bit);
		goto try_again;
	}
	err = loongson_se_get_res(se, req->int_bit, req->u.gcmd.cmd, res);
	if (err || res->u.res.cmd_ret) {
		se_enable_int(se, req->int_bit);
		goto try_again;
	}

out:
	se_enable_int(se, req->int_bit);

	return err;
}

static int loongson_se_set_msg(struct lsse_ch *ch)
{
	struct loongson_se *se = ch->se;
	struct se_data req = {0};
	struct se_data res = {0};
	int err;

	req.int_bit = SE_INT_SETUP;
	req.u.gcmd.cmd = SE_CMD_SETMSG;
	/* MSG off */
	req.u.gcmd.info[0] = ch->id;
	req.u.gcmd.info[1] = ch->smsg - se->mem_base;
	req.u.gcmd.info[2] = ch->msg_size;

	dev_dbg(se->dev, "Set Channel %d msg off 0x%x, msg size %d\n",
		ch->id, req.u.gcmd.info[1], req.u.gcmd.info[2]);

	err = se_send_genl_cmd(se, &req, &res, 5);
	if (res.u.res.cmd_ret)
		return res.u.res.cmd_ret;

	return err;
}

static irqreturn_t se_irq(int irq, void *dev_id)
{
	struct loongson_se *se = (struct loongson_se *)dev_id;
	struct lsse_ch *ch;
	u32 int_status;

	int_status = se_readl(se, SE_S2LINT_STAT);

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
		se_writel(se, BIT(id), SE_S2LINT_CL);
	}

	return IRQ_HANDLED;
}

static int se_init_hw(struct loongson_se *se, dma_addr_t addr, int size)
{
	struct se_data req;
	struct se_data res;
	int err, retry = 5;

	se_enable_int(se, SE_INT_SETUP);

	/* Start engine */
	memset(&req, 0, sizeof(struct se_data));
	memset(&res, 0, sizeof(struct se_data));
	req.int_bit = SE_INT_SETUP;
	req.u.gcmd.cmd = SE_CMD_START;
	err = se_send_genl_cmd(se, &req, &res, retry);
	if (err)
		return err;

	/* Get Version */
	memset(&req, 0, sizeof(struct se_data));
	memset(&res, 0, sizeof(struct se_data));
	req.int_bit = SE_INT_SETUP;
	req.u.gcmd.cmd = SE_CMD_GETVER;
	err = se_send_genl_cmd(se, &req, &res, retry);
	if (err)
		return err;
	se->version = res.u.res.info[0];

	/* Set shared mem */
	memset(&req, 0, sizeof(struct se_data));
	memset(&res, 0, sizeof(struct se_data));
	req.int_bit = SE_INT_SETUP;
	req.u.gcmd.cmd = SE_CMD_SETBUF;
	/* MMAP */
	req.u.gcmd.info[0] = addr & 0xffffffff;
	req.u.gcmd.info[1] = addr >> 32;
	/* MASK */
	req.u.gcmd.info[2] = ~(size - 1);
	req.u.gcmd.info[3] = 0xffffffff;
	err = se_send_genl_cmd(se, &req, &res, retry);
	if (err)
		return err;
	pr_debug("Set win mmap 0x%llx, mask 0x%llx\n",
		 ((u64)req.u.gcmd.info[1] << 32) | req.u.gcmd.info[0],
		 ((u64)req.u.gcmd.info[3] << 32) | req.u.gcmd.info[2]);

	return err;
}

static void se_disable_hw(struct loongson_se *se)
{
	struct se_data req = {0};
	struct se_data res = {0};

	/* Stop engine */
	req.int_bit = SE_INT_SETUP;
	req.u.gcmd.cmd = SE_CMD_STOP;
	se_send_genl_cmd(se, &req, &res, 5);
	se_disable_int(se, SE_INT_ALL);
}

/*
 * Called by SE's child device driver.
 */
int se_send_ch_request(struct lsse_ch *ch)
{
	struct loongson_se *se;
	u32 status, int_bit;

	se = ch->se;
	int_bit = ch->int_bit;
	if ((se_readl(se, SE_L2SINT_STAT) & int_bit) ||
	    !(se_readl(se, SE_L2SINT_EN) & int_bit))
		return -EBUSY;

	se_enable_int(se, int_bit);
	se_writel(se, int_bit, SE_L2SINT_SET);

	return readl_relaxed_poll_timeout_atomic(se->base + SE_L2SINT_STAT, status,
						 !(status & int_bit), 10, 10000);

}
EXPORT_SYMBOL_GPL(se_send_ch_request);

/*
 * se_init_ch() - Init the channel used by child device.
 *
 * Allocate the shared memory agreed upon with SE on SE probe,
 * and register the callback function when the data processing
 * in this channel is completed.
 */
struct lsse_ch *se_init_ch(struct device *dev, int id, int data_size, int msg_size,
			   void *priv, void (*complete)(struct lsse_ch *se_ch))
{
	struct loongson_se *se = dev_get_drvdata(dev);
	struct lsse_ch *ch;
	unsigned long flag;
	int data_first, data_nr;
	int msg_first, msg_nr;

	if (!se) {
		pr_err("SE has not been initialized\n");
		return NULL;
	}

	if (id > SE_CH_MAX) {
		dev_err(se->dev, "Channel number %d is invalid\n", id);
		return NULL;
	}

	if (se_ch_status(se, BIT(id))) {
		dev_err(se->dev, "Channel number %d has been initialized\n", id);
		return NULL;
	}

	spin_lock_irqsave(&se->dev_lock, flag);

	ch = &se->chs[id];
	ch->se = se;
	ch->id = id;
	ch->int_bit = BIT(id);
	se->ch_status |= BIT(id);

	data_nr = round_up(data_size, PAGE_SIZE) / PAGE_SIZE;
	data_first = bitmap_find_next_zero_area(se->mem_map, se->mem_map_pages,
						0, data_nr, 0);
	if (data_first >= se->mem_map_pages) {
		dev_err(se->dev, "Insufficient memory space\n");
		spin_unlock_irqrestore(&se->dev_lock, flag);
		return NULL;
	}

	bitmap_set(se->mem_map, data_first, data_nr);
	ch->data_buffer = se->mem_base + data_first * PAGE_SIZE;
	ch->data_addr = se->mem_addr + data_first * PAGE_SIZE;
	ch->data_size = data_size;

	msg_nr = round_up(msg_size, PAGE_SIZE) / PAGE_SIZE;
	msg_first = bitmap_find_next_zero_area(se->mem_map, se->mem_map_pages,
					       0, msg_nr, 0);
	if (msg_first >= se->mem_map_pages) {
		dev_err(se->dev, "Insufficient memory space\n");
		bitmap_clear(se->mem_map, data_first, data_nr);
		spin_unlock_irqrestore(&se->dev_lock, flag);
		return NULL;
	}

	bitmap_set(se->mem_map, msg_first, msg_nr);
	ch->smsg = se->mem_base + msg_first * PAGE_SIZE;
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
	struct loongson_se *se = ch->se;
	unsigned long flag;
	int first, nr;
	int id = ch->id;

	if (!se) {
		pr_err("SE has bot been initialized\n");
		return;
	}

	if (id > SE_CH_MAX) {
		dev_err(se->dev, "Channel number %d is invalid\n", id);
		return;
	}

	if (!se_ch_status(se, BIT(id))) {
		dev_err(se->dev, "Channel number %d has not been initialized\n", id);
		return;
	}

	spin_lock_irqsave(&se->dev_lock, flag);
	se->ch_status &= ~BIT(ch->id);

	first = (ch->data_buffer - se->mem_base) / PAGE_SIZE;
	nr = round_up(ch->data_size, PAGE_SIZE) / PAGE_SIZE;
	bitmap_clear(se->mem_map, first, nr);

	first = (ch->smsg - se->mem_base) / PAGE_SIZE;
	nr = round_up(ch->msg_size, PAGE_SIZE) / PAGE_SIZE;
	bitmap_clear(se->mem_map, first, nr);

	se_disable_int(se, ch->int_bit);
	spin_unlock_irqrestore(&se->dev_lock, flag);

}
EXPORT_SYMBOL_GPL(se_deinit_ch);

static int loongson_se_probe(struct platform_device *pdev)
{
	struct loongson_se *se;
	struct device *dev = &pdev->dev;
	int nr_irq, irq, err, size;

	se = devm_kmalloc(dev, sizeof(*se), GFP_KERNEL);
	if (!se)
		return -ENOMEM;
	se->dev = dev;
	dev_set_drvdata(dev, se);
	init_completion(&se->cmd_completion);
	spin_lock_init(&se->cmd_lock);
	spin_lock_init(&se->dev_lock);
	/* Setup DMA buffer */
	dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64));
	if (device_property_read_u32(dev, "dmam_size", &size))
		return -ENODEV;
	size = roundup_pow_of_two(size);
	se->mem_base = dmam_alloc_coherent(dev, size, &se->mem_addr, GFP_KERNEL);
	if (!se->mem_base)
		return -ENOMEM;
	se->mem_map_pages = size / PAGE_SIZE;
	se->mem_map = devm_bitmap_zalloc(dev, se->mem_map_pages, GFP_KERNEL);
	if (!se->mem_map)
		return -ENOMEM;

	se->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(se->base))
		return PTR_ERR(se->base);

	nr_irq = platform_irq_count(pdev);
	if (nr_irq <= 0)
		return -ENODEV;
	while (nr_irq) {
		irq = platform_get_irq(pdev, --nr_irq);
		if (irq < 0)
			return -ENODEV;
		/* Use the same interrupt handler address.
		 * Determine which irq it is accroding
		 * SE_S2LINT_STAT register.
		 */
		err = devm_request_irq(dev, irq, se_irq, 0,
				       "loongson-se", se);
		if (err)
			dev_err(dev, "failed to request irq: %d\n", err);
	}

	err = se_init_hw(se, se->mem_addr, size);
	if (err)
		se_disable_hw(se);

	return err;
}

static const struct acpi_device_id loongson_se_acpi_match[] = {
	{"LOON0011", 0},
	{}
};
MODULE_DEVICE_TABLE(acpi, loongson_se_acpi_match);

static struct platform_driver loongson_se_driver = {
	.probe   = loongson_se_probe,
	.driver  = {
		.name  = "loongson-se",
		.acpi_match_table = loongson_se_acpi_match,
	},
};
module_platform_driver(loongson_se_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Loongson Technology Corporation");
MODULE_DESCRIPTION("Loongson Security Module driver");
