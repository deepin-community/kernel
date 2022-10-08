// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2022 Hisilicon Limited.
#include <linux/mm.h>
#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/net_tstamp.h>
#include <linux/debugfs.h>

#ifndef PTP_CLOCK_NAME_LEN
#define PTP_CLOCK_NAME_LEN	32
#endif

#define HISI_PTP_VERSION	"22.10.2"

#define HISI_PTP_NAME		"hisi_ptp"
#define HISI_PTP_INT_NAME_LEN	32

#define HISI_PTP_DBGFS_STS_LEN	2048
#define HISI_PTP_DBGFS_REG_LEN	0x10000

#define HISI_RES_T_PERI_SC	0
#define HISI_RES_N_NET_SC	0
#define HISI_RES_N_IO_SC	1

#define HISI_PTP_INIT_DONE	0

/* peri subctrl reg offset */
#define PERI_SC_PTP_RESET_REQ			0xE18
#define PERI_SC_PTP_RESET_DREQ			0xE1C
#define PERI_SC_LOCAL_TIMER_COMP_HIGH_ADDR	0x5000
#define PERI_SC_LOCAL_TIMER_COMP_LOW_ADDR	0x5004
#define PERI_SC_BAUD_VALUE_ADDR			0x5008
#define PERI_SC_LOCAL_CNT_EN_ADDR		0x500C
#define PERI_SC_SYNC_ERR_COMP_HIGH_ADDR		0x5010
#define PERI_SC_SYNC_ERR_COMP_LOW_ADDR		0x5014
#define PERI_SC_CRC_EN_ADDR			0x5018
#define PERI_SC_ONE_CYCLE_NUM_ADDR		0x5020
#define PERI_SC_SYNC_ERR_CLR_ADDR		0x5024
#define PERI_SC_RX_SHIFT_EN_ADDR		0x5028
#define PERI_SC_TIMEL_CY_NUM_ADDR		0x502C
#define PERI_SC_INT_PTP_SYNC_ERR_ADDR		0x5044
#define PERI_SC_INT_PTP_SYNC_ERR_MASK_ADDR	0x5048
#define PERI_SC_INT_ORIGIN			0x504C
#define PERI_SC_CRC_ERR_COUNT			0x5050
#define PERI_SC_CRC_INT_CONTRL_ADDR		0x5054
#define PERI_SC_CAPTURE_PTP_TIME_COMP_HIGH	0x5058
#define PERI_SC_CAPTURE_PTP_TIME_COMP_LOW	0x505C
#define PERI_SC_CAPTURE_SYSTEM_COUNTER_BIN_HIGH	0x5060
#define PERI_SC_CAPTURE_SYSTEM_COUNTER_BIN_LOW	0x5064
#define PERI_SC_CAPTURE_VLD			0x5068
#define PERI_SC_LOCAL_TIME_LOW_ADDR		0x5070
#define PERI_SC_LOCAL_TIME_HIGH_ADDR		0x5074

/* net subctrl reg offset */
#define NET_SC_PTP_BAUD_VALUE_ADDR		0x2008
#define NET_SC_PTP_COUNTER_EN_ADDR		0x200C
#define NET_SC_PTP_NORMAL_MODE_EN		0x2010
#define NET_SC_PTP_WIRE_DELAY_CAL_EN		0x2014
#define NET_SC_SAMPLE_DELAY_CFG_ADDR		0x2018
#define NET_SC_PTP_TX_DFXBUS0_ADDR		0x201C
#define NET_SC_PTP_TX_DFXBUS1_ADDR		0x2020
#define NET_SC_PTP_TX_DFXBUS2_ADDR		0x2024
#define NET_SC_PTP_TX_DFXBUS3_ADDR		0x2028

/* io subctrl reg offset */
#define IO_SC_PTP_BAUD_VALUE_ADDR		0x2008
#define IO_SC_PTP_COUNTER_EN_ADDR		0x200C
#define IO_SC_PTP_NORMAL_MODE_EN		0x2010
#define IO_SC_PTP_WIRE_DELAY_CAL_EN		0x2014
#define IO_SC_SAMPLE_DELAY_CFG_ADDR		0x2018
#define IO_SC_PTP_TX_DFXBUS0_ADDR		0x201C
#define IO_SC_PTP_TX_DFXBUS1_ADDR		0x2020
#define IO_SC_PTP_TX_DFXBUS2_ADDR		0x2024
#define IO_SC_PTP_TX_DFXBUS3_ADDR		0x2028

/* default values */
#define HISI_DEF_BAUD				0x1388
#define HISI_DEF_TIME_COMP			0xB2432
#define HISI_DEF_ERR_COMP			0xFFFFFFFF
#define HISI_DEF_ONE_CYCLE_NUM			0x50

#define HISI_PTP_TX_IDLE_MASK			GENMASK(26, 23)

#define HISI_PTP_RX_CRC_INT_EN			BIT(0)
#define HISI_PTP_RX_CRC_CLR			BIT(1)
#define HISI_PTP_RX_CRC_CLR_AND_EN \
	(HISI_PTP_RX_CRC_INT_EN | HISI_PTP_RX_CRC_INT_EN)
#define HISI_PTP_RX_CRC_CLR_AND_DISABLE		HISI_PTP_RX_CRC_CLR

#define HISI_PTP_SUP_CHK_CNT			32
/* suppress check window and suppress time, unit: ms */
#define HISI_PTP_SUP_CHK_THR			10
#define HISI_PTP_SUP_TIME			100

enum HISI_PTP_TX_MODE {
	HISI_PTP_CAL_MODE,
	HISI_PTP_NORMAL_MODE,
};

struct hisi_ptp_rx {
	struct list_head node;
	char name[HISI_PTP_INT_NAME_LEN];
	struct device *dev;
	u64 time_comp; /* internal wire time compensation value */
	int irq;
	void __iomem *base;
};

struct hisi_ptp_tx {
	struct device *dev;
	void __iomem *base;
	void __iomem *io_sc_base;
};

struct hisi_ptp_pdev {
	struct list_head ptp_rx_list;
	struct hisi_ptp_tx *ptp_tx;
	u32 tx_cnt;
	u32 rx_total;
	u32 rx_cnt;
	unsigned long flag;
	void __iomem *rx_base; /* peri subctl base of chip 0 */
	u32 irq_cnt;
	unsigned long last_jiffies; /* record last irq jiffies */
	struct timer_list suppress_timer;
	struct ptp_clock *clock;
	struct ptp_clock_info info;
	rwlock_t rw_lock;
	struct dentry *dbgfs_root;
};

struct hisi_ptp_reg {
	const char *name;
	u32 offset;
};

static struct hisi_ptp_pdev g_ptpdev;

static uint err_threshold = HISI_DEF_ERR_COMP;
module_param(err_threshold, uint, 0644);
MODULE_PARM_DESC(err_threshold, "PTP time sync error threshold");

static struct hisi_ptp_pdev *hisi_ptp_get_pdev(struct ptp_clock_info *info)
{
	struct hisi_ptp_pdev *ptp =
		container_of(info, struct hisi_ptp_pdev, info);
	return ptp;
}

/* This function should call under rw_lock */
static void hisi_ptp_disable(struct hisi_ptp_pdev *ptp)
{
	struct hisi_ptp_rx *rx;
	void __iomem *base;

	/* disable tx */
	if (ptp->ptp_tx && ptp->ptp_tx->base) {
		base = ptp->ptp_tx->base;
		writel(0, base + NET_SC_PTP_COUNTER_EN_ADDR);
	}

	/* disable all totem rx and interrupt */
	list_for_each_entry(rx, &ptp->ptp_rx_list, node) {
		base = rx->base;
		writel(0, base + PERI_SC_RX_SHIFT_EN_ADDR);
		writel(1, base + PERI_SC_INT_PTP_SYNC_ERR_MASK_ADDR);
		writel(HISI_PTP_RX_CRC_CLR_AND_DISABLE,
		       base + PERI_SC_CRC_INT_CONTRL_ADDR);
	}
}

/* This function should call under rw_lock */
static void hisi_ptp_unmask_irq(struct hisi_ptp_pdev *ptp)
{
	struct hisi_ptp_rx *rx;
	void __iomem *base;

	/* clear CRC errors and unmask all totem interrupt */
	list_for_each_entry(rx, &ptp->ptp_rx_list, node) {
		base = rx->base;
		writel(HISI_PTP_RX_CRC_INT_EN,
		       base + PERI_SC_CRC_INT_CONTRL_ADDR);
		writel(0, base + PERI_SC_INT_PTP_SYNC_ERR_MASK_ADDR);
	}
}

/* This function should call under rw_lock */
static void hisi_ptp_wait_and_enable(struct hisi_ptp_pdev *ptp)
{
#define HISI_PTP_TX_IDLE_WAIT_CNT 20
	void __iomem *nimbus_base;
	struct hisi_ptp_rx *rx;
	void __iomem *base;
	int delay_cnt = 0;

	if (!ptp->ptp_tx || !ptp->ptp_tx->base)
		return;

	/* wait for tx idle */
	nimbus_base = ptp->ptp_tx->base;
	while (delay_cnt++ < HISI_PTP_TX_IDLE_WAIT_CNT) {
		u32 dfx_bus0 = readl(nimbus_base + NET_SC_PTP_TX_DFXBUS0_ADDR);

		/* wait bit26:23 to 0 */
		if ((dfx_bus0 & HISI_PTP_TX_IDLE_MASK) == 0)
			break;

		udelay(1);
	}

	/* enable all totem interrupt and rx */
	list_for_each_entry(rx, &ptp->ptp_rx_list, node) {
		base = rx->base;
		writel(1, base + PERI_SC_SYNC_ERR_CLR_ADDR);
		writel(0, base + PERI_SC_SYNC_ERR_CLR_ADDR);
		writel(1, base + PERI_SC_INT_PTP_SYNC_ERR_ADDR);
		writel(1, base + PERI_SC_RX_SHIFT_EN_ADDR);
	}

	/* enable tx */
	writel(1, nimbus_base + NET_SC_PTP_COUNTER_EN_ADDR);

	hisi_ptp_unmask_irq(ptp);
}

/* This function should call under rw_lock */
static bool hisi_ptp_need_suppress(struct hisi_ptp_pdev *ptp)
{
	if (time_is_before_jiffies(ptp->last_jiffies +
				   msecs_to_jiffies(HISI_PTP_SUP_CHK_THR))) {
		ptp->last_jiffies = jiffies;
		ptp->irq_cnt = 0;
		return false;
	}

	if (ptp->irq_cnt++ < HISI_PTP_SUP_CHK_CNT)
		return false;

	return true;
}

static irqreturn_t hisi_ptp_irq_handle(int irq, void *data)
{
	struct hisi_ptp_pdev *ptp = (struct hisi_ptp_pdev *)data;

	dev_dbg(ptp->ptp_tx->dev, "ptp time sync error, irq:%d\n", irq);

	write_lock(&ptp->rw_lock);

	hisi_ptp_disable(ptp);

	if (hisi_ptp_need_suppress(ptp)) {
		mod_timer(&ptp->suppress_timer,
			  jiffies + msecs_to_jiffies(HISI_PTP_SUP_TIME));
		write_unlock(&ptp->rw_lock);
		return IRQ_HANDLED;
	}

	hisi_ptp_wait_and_enable(ptp);

	write_unlock(&ptp->rw_lock);

	return IRQ_HANDLED;
}

static int hisi_ptp_get_rx_resource(struct platform_device *pdev,
				    struct hisi_ptp_pdev *ptp)
{
	struct hisi_ptp_rx *rx;
	struct resource *peri;
	unsigned long flags;
	u32 rx_total = 0;
	bool is_base_rx;
	int ret;

	ret = device_property_read_u32(&pdev->dev, "rx_num", &rx_total);
	if (ret) {
		dev_err(&pdev->dev, "failed to read rx total property\n");
		return ret;
	}

	rx = devm_kzalloc(&pdev->dev, sizeof(struct hisi_ptp_rx), GFP_KERNEL);
	if (!rx)
		return -ENOMEM;

	peri = platform_get_resource(pdev, IORESOURCE_MEM, HISI_RES_T_PERI_SC);
	if (!peri) {
		dev_err(&pdev->dev, "failed to get rx peri resource\n");
		return -EINVAL;
	}

	rx->base = devm_ioremap(&pdev->dev, peri->start, resource_size(peri));
	if (!rx->base) {
		dev_err(&pdev->dev, "failed to remap rx peri resource\n");
		return -ENOMEM;
	}

	rx->irq = platform_get_irq(pdev, 0);
	if (rx->irq < 0) {
		dev_err(&pdev->dev, "failed to get irq, ret = %d\n", rx->irq);
		return rx->irq;
	}
	snprintf(rx->name, HISI_PTP_INT_NAME_LEN, "%s-%d", HISI_PTP_NAME,
		 rx->irq);
	ret = devm_request_irq(&pdev->dev, rx->irq, hisi_ptp_irq_handle, 0,
			       rx->name, ptp);
	if (ret) {
		dev_err(&pdev->dev, "failed to request irq(%d), ret = %d\n",
			rx->irq, ret);
		return ret;
	}

	is_base_rx = device_property_present(&pdev->dev, "base_rx");

	rx->dev = &pdev->dev;

	write_lock_irqsave(&ptp->rw_lock, flags);

	if (is_base_rx)
		ptp->rx_base = rx->base;

	ptp->rx_cnt++;

	/* use the first rx device to init the global rx_total */
	if (ptp->rx_total == 0)
		ptp->rx_total = rx_total;

	if (ptp->rx_total != rx_total || ptp->rx_cnt > ptp->rx_total) {
		write_unlock_irqrestore(&ptp->rw_lock, flags);
		dev_err(&pdev->dev,
			"failed to probe rx device, please check the asl file!\n");
		dev_err(&pdev->dev,
			"rx_total:%u, current rx_total:%u, rx_cnt:%u\n",
			ptp->rx_total, rx_total, ptp->rx_cnt);

		return -EINVAL;
	}

	list_add_tail(&rx->node, &ptp->ptp_rx_list);

	write_unlock_irqrestore(&ptp->rw_lock, flags);

	return 0;
}

static int hisi_ptp_get_tx_resource(struct platform_device *pdev,
				    struct hisi_ptp_pdev *ptp)
{
	struct hisi_ptp_tx *tx;
	struct resource *mem;
	unsigned long flags;

	write_lock_irqsave(&ptp->rw_lock, flags);
	/* use have only one tx device */
	if (ptp->tx_cnt) {
		write_unlock_irqrestore(&ptp->rw_lock, flags);
		dev_err(&pdev->dev,
			"failed to probe tx device, more than one tx device found, please check the asl file!\n");
		return -EINVAL;
	}
	write_unlock_irqrestore(&ptp->rw_lock, flags);

	tx = devm_kzalloc(&pdev->dev, sizeof(struct hisi_ptp_tx), GFP_KERNEL);
	if (!tx)
		return -ENOMEM;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, HISI_RES_N_NET_SC);
	if (!mem) {
		dev_err(&pdev->dev, "failed to get tx net sc resource\n");
		return -EINVAL;
	}

	tx->base = devm_ioremap(&pdev->dev, mem->start, resource_size(mem));
	if (!tx->base) {
		dev_err(&pdev->dev, "failed to remap tx net sc resource\n");
		return -ENOMEM;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, HISI_RES_N_IO_SC);
	if (!mem) {
		dev_err(&pdev->dev, "failed to get tx nimbus io sc resource\n");
		return -EINVAL;
	}

	tx->io_sc_base = devm_ioremap(&pdev->dev, mem->start,
				      resource_size(mem));
	if (!tx->io_sc_base) {
		dev_err(&pdev->dev, "failed to remap tx nimbus io resource\n");
		return -ENOMEM;
	}

	tx->dev = &pdev->dev;

	write_lock_irqsave(&ptp->rw_lock, flags);
	ptp->tx_cnt++;
	ptp->ptp_tx = tx;
	write_unlock_irqrestore(&ptp->rw_lock, flags);

	return 0;
}

static void hisi_ptp_cal_time_start(struct hisi_ptp_pdev *ptp)
{
	void __iomem *io_sc_base;
	struct hisi_ptp_rx *rx;
	void __iomem *base;

	/* config all rx to enter calculation mode. */
	list_for_each_entry(rx, &ptp->ptp_rx_list, node) {
		base = rx->base;
		writel(1, base + PERI_SC_PTP_RESET_REQ);
		writel(1, base + PERI_SC_PTP_RESET_DREQ);
		writel(0, base + PERI_SC_LOCAL_TIMER_COMP_HIGH_ADDR);
		writel(0, base + PERI_SC_LOCAL_TIMER_COMP_LOW_ADDR);
		writel(1, base + PERI_SC_CRC_EN_ADDR);
		writel(0, base + PERI_SC_LOCAL_CNT_EN_ADDR);
		writel(1, base + PERI_SC_RX_SHIFT_EN_ADDR);
	}

	/* config tx to enter calculation mode. */
	base = ptp->ptp_tx->base;
	io_sc_base = ptp->ptp_tx->io_sc_base;
	writel(HISI_PTP_CAL_MODE, io_sc_base + IO_SC_PTP_NORMAL_MODE_EN);
	writel(HISI_PTP_CAL_MODE, base + NET_SC_PTP_NORMAL_MODE_EN);

	writel(HISI_DEF_BAUD, io_sc_base + IO_SC_PTP_BAUD_VALUE_ADDR);
	writel(1, io_sc_base + IO_SC_PTP_COUNTER_EN_ADDR);
	writel(0, io_sc_base + IO_SC_PTP_WIRE_DELAY_CAL_EN);
	writel(1, io_sc_base + IO_SC_PTP_WIRE_DELAY_CAL_EN);
}

static void hisi_ptp_cal_time_get(struct hisi_ptp_pdev *ptp)
{
#define HISI_PTP_MAX_WAIT_CNT 60
	struct hisi_ptp_rx *rx;
	void __iomem *base;
	int cnt;
	u32 rd_l;
	u32 rd_h;
	u32 td_l;
	u32 td_h;
	u64 rd;
	u64 td;

	list_for_each_entry(rx, &ptp->ptp_rx_list, node) {
		base = rx->base;
		rx->time_comp = HISI_DEF_TIME_COMP;

		cnt = 0;
		do {
			if (readl(base + PERI_SC_CAPTURE_VLD) == 0) {
				mdelay(1);
				continue;
			}

			rd_h = readl(base +
				     PERI_SC_CAPTURE_SYSTEM_COUNTER_BIN_HIGH);
			rd_l = readl(base +
				     PERI_SC_CAPTURE_SYSTEM_COUNTER_BIN_LOW);
			td_h = readl(base + PERI_SC_CAPTURE_PTP_TIME_COMP_HIGH);
			td_l = readl(base + PERI_SC_CAPTURE_PTP_TIME_COMP_LOW);

			rd = (u64)rd_h << 32 | rd_l;
			td = (u64)td_h << 32 | td_l;

			if (!rd || !td || rd < td) {
				mdelay(1);
				continue;
			}

			rx->time_comp = rd - td;
			break;
		} while (cnt++ <= HISI_PTP_MAX_WAIT_CNT);
	}
}

static void hisi_ptp_cal_time_end(struct hisi_ptp_pdev *ptp)
{
	void __iomem *io_sc_base;
	struct hisi_ptp_rx *rx;
	void __iomem *base;

	/* config all rx to exit calculation mode. */
	list_for_each_entry(rx, &ptp->ptp_rx_list, node) {
		base = rx->base;
		writel(0, base + PERI_SC_RX_SHIFT_EN_ADDR);
	}

	/* config tx to exit calculation mode. */
	base = ptp->ptp_tx->base;
	io_sc_base = ptp->ptp_tx->io_sc_base;

	writel(0, io_sc_base + IO_SC_PTP_COUNTER_EN_ADDR);
	writel(HISI_PTP_NORMAL_MODE, io_sc_base + IO_SC_PTP_NORMAL_MODE_EN);
	writel(HISI_PTP_NORMAL_MODE, base + NET_SC_PTP_NORMAL_MODE_EN);
}

/* This function should call under rw_lock */
static void hisi_ptp_cal_time_comp(struct hisi_ptp_pdev *ptp)
{
	hisi_ptp_cal_time_start(ptp);
	hisi_ptp_cal_time_get(ptp);
	hisi_ptp_cal_time_end(ptp);
}

/* This function should call under rw_lock */
static void hisi_ptp_peri_rx_init(struct hisi_ptp_pdev *ptp)
{
	struct hisi_ptp_rx *rx;
	void __iomem *base;

	list_for_each_entry(rx, &ptp->ptp_rx_list, node) {
		base = rx->base;
		writel(1, base + PERI_SC_CRC_EN_ADDR);
		writel(upper_32_bits(rx->time_comp),
		       base + PERI_SC_LOCAL_TIMER_COMP_HIGH_ADDR);
		writel(lower_32_bits(rx->time_comp),
		       base + PERI_SC_LOCAL_TIMER_COMP_LOW_ADDR);
		writel(err_threshold,
		       base + PERI_SC_SYNC_ERR_COMP_LOW_ADDR);
		writel(1, base + PERI_SC_CRC_INT_CONTRL_ADDR);
		writel(0, base + PERI_SC_SYNC_ERR_CLR_ADDR);
		writel(1, base + PERI_SC_LOCAL_CNT_EN_ADDR);
		writel(1, base + PERI_SC_RX_SHIFT_EN_ADDR);
	}
}

/* This function should call under rw_lock */
static void hisi_ptp_net_tx_init(struct hisi_ptp_pdev *ptp)
{
	void __iomem *base;

	base = ptp->ptp_tx->base;
	writel(1, base + NET_SC_PTP_COUNTER_EN_ADDR);
}

static int hisi_ptp_adjfine(struct ptp_clock_info *ptp_info, long delta)
{
	return -EOPNOTSUPP;
}

static int hisi_ptp_adjtime(struct ptp_clock_info *ptp_info, s64 delta)
{
	return -EOPNOTSUPP;
}

static int hisi_ptp_settime(struct ptp_clock_info *ptp_info,
			    const struct timespec64 *ts)
{
	return -EOPNOTSUPP;
}

static int hisi_ptp_gettime(struct ptp_clock_info *ptp_info,
			     struct timespec64 *ts,
			     struct ptp_system_timestamp *sts)
{
	struct hisi_ptp_pdev *ptp = hisi_ptp_get_pdev(ptp_info);
	unsigned long flags;
	u32 hi = UINT_MAX;
	u32 lo = UINT_MAX;
	u64 ns;

	read_lock_irqsave(&ptp->rw_lock, flags);

	if (ptp->rx_base) {
		hi = readl(ptp->rx_base + PERI_SC_LOCAL_TIME_HIGH_ADDR);
		lo = readl(ptp->rx_base + PERI_SC_LOCAL_TIME_LOW_ADDR);
	}

	read_unlock_irqrestore(&ptp->rw_lock, flags);

	ns = (u64)hi * NSEC_PER_SEC + lo;
	*ts = ns_to_timespec64(ns);

	return 0;
}

static int hisi_ptp_create_clock(struct hisi_ptp_pdev *ptp)
{
	dev_info(ptp->ptp_tx->dev, "register ptp clock\n");

	snprintf(ptp->info.name, PTP_CLOCK_NAME_LEN, "%s", HISI_PTP_NAME);
	ptp->info.owner = THIS_MODULE;
	ptp->info.adjfine = hisi_ptp_adjfine;
	ptp->info.adjtime = hisi_ptp_adjtime;
	ptp->info.settime64 = hisi_ptp_settime;
	ptp->info.gettimex64 = hisi_ptp_gettime;
	ptp->clock = ptp_clock_register(&ptp->info, ptp->ptp_tx->dev);
	if (IS_ERR(ptp->clock)) {
		dev_err(ptp->ptp_tx->dev,
			"failed to register ptp clock, ret = %ld\n",
			PTR_ERR(ptp->clock));
		return PTR_ERR(ptp->clock);
	}

	return 0;
}

static void hisi_ptp_timer(struct timer_list *t)
{
	struct hisi_ptp_pdev *ptp = from_timer(ptp, t, suppress_timer);
	unsigned long flags;

	write_lock_irqsave(&ptp->rw_lock, flags);

	dev_dbg(ptp->ptp_tx->dev, "ptp timer timeout handler.\n");

	ptp->last_jiffies = jiffies;
	ptp->irq_cnt = 0;

	hisi_ptp_wait_and_enable(ptp);

	write_unlock_irqrestore(&ptp->rw_lock, flags);
}

static int hisi_ptp_probe(struct platform_device *pdev)
{
	struct hisi_ptp_pdev *ptp = &g_ptpdev;
	unsigned long flags;
	const char *type;
	int ret;

	dev_info(&pdev->dev, "ptp probe start\n");

	ret = device_property_read_string(&pdev->dev, "type", &type);
	if (ret) {
		dev_err(&pdev->dev, "failed to read device type, ret = %d\n",
			ret);
		return ret;
	}

	if (!memcmp(type, "rx", strlen("rx"))) {
		ret = hisi_ptp_get_rx_resource(pdev, ptp);
	} else if (!memcmp(type, "tx", strlen("tx"))) {
		ret = hisi_ptp_get_tx_resource(pdev, ptp);
	} else {
		dev_err(&pdev->dev,
			"failed to probe unknown device, type: %s\n",
			type);
		ret = -EINVAL;
	}
	if (ret)
		return ret;

	write_lock_irqsave(&ptp->rw_lock, flags);

	if (ptp->rx_total == 0 || ptp->rx_total != ptp->rx_cnt ||
	    ptp->tx_cnt != 1) {
		write_unlock_irqrestore(&ptp->rw_lock, flags);
		dev_info(&pdev->dev,
			 "waiting for devices...rx total:%u, now:%u. tx total:1, now:%u\n",
			 ptp->rx_total, ptp->rx_cnt, ptp->tx_cnt);
		return 0;
	}

	if (!ptp->rx_base) {
		write_unlock_irqrestore(&ptp->rw_lock, flags);
		dev_err(&pdev->dev,
			"failed to probe, no base rx device, please check the asl file!\n");
		return -EINVAL;
	}

	hisi_ptp_disable(ptp);
	hisi_ptp_cal_time_comp(ptp);
	hisi_ptp_peri_rx_init(ptp);
	hisi_ptp_net_tx_init(ptp);
	hisi_ptp_unmask_irq(ptp);

	write_unlock_irqrestore(&ptp->rw_lock, flags);

	ret = hisi_ptp_create_clock(ptp);
	if (ret) {
		write_lock_irqsave(&ptp->rw_lock, flags);
		hisi_ptp_disable(ptp);
		write_unlock_irqrestore(&ptp->rw_lock, flags);
		return ret;
	}

	set_bit(HISI_PTP_INIT_DONE, &ptp->flag);

	dev_info(&pdev->dev, "ptp probe end\n");
	return 0;
}

static int hisi_ptp_remove(struct platform_device *pdev)
{
	struct hisi_ptp_pdev *ptp = &g_ptpdev;
	struct hisi_ptp_rx *rx;
	unsigned long flags;

	if (test_and_clear_bit(HISI_PTP_INIT_DONE, &ptp->flag)) {
		ptp_clock_unregister(ptp->clock);
		ptp->clock = NULL;

		write_lock_irqsave(&ptp->rw_lock, flags);
		hisi_ptp_disable(ptp);
		write_unlock_irqrestore(&ptp->rw_lock, flags);

		dev_info(&pdev->dev, "unregister ptp clock\n");
	}

	write_lock_irqsave(&ptp->rw_lock, flags);
	if (ptp->ptp_tx && ptp->ptp_tx->dev == &pdev->dev) {
		ptp->tx_cnt--;
		ptp->ptp_tx = NULL;
		dev_info(&pdev->dev, "remove tx ptp device\n");
	} else {
		list_for_each_entry(rx, &ptp->ptp_rx_list, node) {
			if (rx->dev == &pdev->dev) {
				ptp->rx_cnt--;
				list_del(&rx->node);
				dev_info(&pdev->dev, "remove rx ptp device\n");
				break;
			}
		}
	}
	write_unlock_irqrestore(&ptp->rw_lock, flags);

	return 0;
}

static const struct acpi_device_id hisi_ptp_acpi_match[] = {
	{ "HISI0411", 0 },
	{ }
};
MODULE_DEVICE_TABLE(acpi, hisi_ptp_acpi_match);

static struct platform_driver hisi_ptp_driver = {
	.probe = hisi_ptp_probe,
	.remove = hisi_ptp_remove,
	.driver	= {
		.name = HISI_PTP_NAME,
		.acpi_match_table = ACPI_PTR(hisi_ptp_acpi_match),
	},
};

static ssize_t hisi_ptp_dbg_read_state(struct file *filp, char __user *buf,
				       size_t cnt, loff_t *ppos)
{
	struct hisi_ptp_pdev *ptp = filp->private_data;
	struct hisi_ptp_rx *rx;
	unsigned long flags;
	ssize_t size = 0;
	char *read_buf;
	int pos = 0;
	int len;

	if (*ppos < 0)
		return -EINVAL;
	if (cnt <= 0)
		return 0;
	if (!access_ok(buf, cnt))
		return -EFAULT;

	read_buf = kvzalloc(HISI_PTP_DBGFS_STS_LEN, GFP_KERNEL);
	if (!read_buf)
		return -ENOMEM;

	len = HISI_PTP_DBGFS_STS_LEN;

	write_lock_irqsave(&ptp->rw_lock, flags);
	pos += scnprintf(read_buf + pos, len - pos, "error threshold: %#x\n",
			 err_threshold);
	pos += scnprintf(read_buf + pos, len - pos, "tx count: %u\n",
			 ptp->tx_cnt);
	pos += scnprintf(read_buf + pos, len - pos, "rx total: %u\n",
			 ptp->rx_total);
	pos += scnprintf(read_buf + pos, len - pos, "rx count: %u\n",
			 ptp->rx_cnt);
	pos += scnprintf(read_buf + pos, len - pos, "irq count: %u\n",
			 ptp->irq_cnt);
	pos += scnprintf(read_buf + pos, len - pos, "irq last jiffies: %lu\n",
			 ptp->last_jiffies);

	list_for_each_entry(rx, &ptp->ptp_rx_list, node) {
		pos += scnprintf(read_buf + pos, len - pos, "name: %s\n",
				 rx->name);
		pos += scnprintf(read_buf + pos, len - pos, "time comp: %#llx\n",
				 rx->time_comp);
		pos += scnprintf(read_buf + pos, len - pos, "irq: %d\n",
				 rx->irq);
	}
	write_unlock_irqrestore(&ptp->rw_lock, flags);

	size = simple_read_from_buffer(buf, cnt, ppos, read_buf,
				       strlen(read_buf));

	kvfree(read_buf);

	return size;
}

static const struct hisi_ptp_reg hisi_ptp_tx_reg[] = {
	{"NET_SC_PTP_BAUD_VALUE_ADDR  ",
	 NET_SC_PTP_BAUD_VALUE_ADDR},
	{"NET_SC_PTP_COUNTER_EN_ADDR  ",
	 NET_SC_PTP_COUNTER_EN_ADDR},
	{"NET_SC_PTP_NORMAL_MODE_EN   ",
	 NET_SC_PTP_NORMAL_MODE_EN},
	{"NET_SC_PTP_WIRE_DELAY_CAL_EN",
	 NET_SC_PTP_WIRE_DELAY_CAL_EN},
	{"NET_SC_SAMPLE_DELAY_CFG_ADDR",
	 NET_SC_SAMPLE_DELAY_CFG_ADDR},
	{"NET_SC_PTP_TX_DFXBUS0_ADDR  ",
	 NET_SC_PTP_TX_DFXBUS0_ADDR},
	{"NET_SC_PTP_TX_DFXBUS1_ADDR  ",
	 NET_SC_PTP_TX_DFXBUS1_ADDR},
	{"NET_SC_PTP_TX_DFXBUS2_ADDR  ",
	 NET_SC_PTP_TX_DFXBUS2_ADDR},
	{"NET_SC_PTP_TX_DFXBUS3_ADDR  ",
	 NET_SC_PTP_TX_DFXBUS3_ADDR}
};

static const struct hisi_ptp_reg hisi_ptp_tx_io_reg[] = {
	{"IO_SC_PTP_BAUD_VALUE_ADDR  ",
	 IO_SC_PTP_BAUD_VALUE_ADDR},
	{"IO_SC_PTP_COUNTER_EN_ADDR  ",
	 IO_SC_PTP_COUNTER_EN_ADDR},
	{"IO_SC_PTP_NORMAL_MODE_EN   ",
	 IO_SC_PTP_NORMAL_MODE_EN},
	{"IO_SC_PTP_WIRE_DELAY_CAL_EN",
	 IO_SC_PTP_WIRE_DELAY_CAL_EN},
	{"IO_SC_SAMPLE_DELAY_CFG_ADDR",
	 IO_SC_SAMPLE_DELAY_CFG_ADDR},
	{"IO_SC_PTP_TX_DFXBUS0_ADDR  ",
	 IO_SC_PTP_TX_DFXBUS0_ADDR},
	{"IO_SC_PTP_TX_DFXBUS1_ADDR  ",
	 IO_SC_PTP_TX_DFXBUS1_ADDR},
	{"IO_SC_PTP_TX_DFXBUS2_ADDR  ",
	 IO_SC_PTP_TX_DFXBUS2_ADDR},
	{"IO_SC_PTP_TX_DFXBUS3_ADDR  ",
	 IO_SC_PTP_TX_DFXBUS3_ADDR}
};

static const struct hisi_ptp_reg hisi_ptp_rx_reg[] = {
	{"PERI_SC_LOCAL_TIMER_COMP_HIGH_ADDR     ",
	 PERI_SC_LOCAL_TIMER_COMP_HIGH_ADDR},
	{"PERI_SC_LOCAL_TIMER_COMP_LOW_ADDR      ",
	 PERI_SC_LOCAL_TIMER_COMP_LOW_ADDR},
	{"PERI_SC_BAUD_VALUE_ADDR                ",
	 PERI_SC_BAUD_VALUE_ADDR},
	{"PERI_SC_LOCAL_CNT_EN_ADDR              ",
	 PERI_SC_LOCAL_CNT_EN_ADDR},
	{"PERI_SC_SYNC_ERR_COMP_HIGH_ADDR        ",
	 PERI_SC_SYNC_ERR_COMP_HIGH_ADDR},
	{"PERI_SC_SYNC_ERR_COMP_LOW_ADDR         ",
	 PERI_SC_SYNC_ERR_COMP_LOW_ADDR},
	{"PERI_SC_CRC_EN_ADDR                    ",
	 PERI_SC_CRC_EN_ADDR},
	{"PERI_SC_ONE_CYCLE_NUM_ADDR             ",
	 PERI_SC_ONE_CYCLE_NUM_ADDR},
	{"PERI_SC_SYNC_ERR_CLR_ADDR              ",
	 PERI_SC_SYNC_ERR_CLR_ADDR},
	{"PERI_SC_RX_SHIFT_EN_ADDR               ",
	 PERI_SC_RX_SHIFT_EN_ADDR},
	{"PERI_SC_TIMEL_CY_NUM_ADDR              ",
	 PERI_SC_TIMEL_CY_NUM_ADDR},
	{"PERI_SC_INT_PTP_SYNC_ERR_ADDR          ",
	 PERI_SC_INT_PTP_SYNC_ERR_ADDR},
	{"PERI_SC_INT_PTP_SYNC_ERR_MASK_ADDR     ",
	 PERI_SC_INT_PTP_SYNC_ERR_MASK_ADDR},
	{"PERI_SC_INT_ORIGIN                     ",
	 PERI_SC_INT_ORIGIN},
	{"PERI_SC_CRC_ERR_COUNT                  ",
	 PERI_SC_CRC_ERR_COUNT},
	{"PERI_SC_CRC_INT_CONTRL_ADDR            ",
	 PERI_SC_CRC_INT_CONTRL_ADDR},
	{"PERI_SC_CAPTURE_PTP_TIME_COMP_HIGH     ",
	 PERI_SC_CAPTURE_PTP_TIME_COMP_HIGH},
	{"PERI_SC_CAPTURE_PTP_TIME_COMP_LOW      ",
	 PERI_SC_CAPTURE_PTP_TIME_COMP_LOW},
	{"PERI_SC_CAPTURE_SYSTEM_COUNTER_BIN_HIGH",
	 PERI_SC_CAPTURE_SYSTEM_COUNTER_BIN_HIGH},
	{"PERI_SC_CAPTURE_SYSTEM_COUNTER_BIN_LOW ",
	 PERI_SC_CAPTURE_SYSTEM_COUNTER_BIN_LOW},
	{"PERI_SC_CAPTURE_VLD                    ",
	 PERI_SC_CAPTURE_VLD},
	{"PERI_SC_LOCAL_TIME_LOW_ADDR            ",
	 PERI_SC_LOCAL_TIME_LOW_ADDR},
	{"PERI_SC_LOCAL_TIME_HIGH_ADDR           ",
	 PERI_SC_LOCAL_TIME_HIGH_ADDR}
};

static void hisi_ptp_dump_reg(void __iomem *base,
			      const struct hisi_ptp_reg *reg, int reg_len,
			      char *buf, int len, int *pos)
{
	int i;

	for (i = 0; i < reg_len; i++)
		*pos += scnprintf(buf + *pos, len - *pos, "%s : 0x%08x\n",
				  reg[i].name, readl(base + reg[i].offset));
}

static ssize_t hisi_ptp_dbg_read_reg(struct file *filp, char __user *buf,
				     size_t cnt, loff_t *ppos)
{
	struct hisi_ptp_pdev *ptp = filp->private_data;
	struct hisi_ptp_rx *rx;
	unsigned long flags;
	ssize_t size = 0;
	char *read_buf;
	int pos = 0;
	int len;

	if (*ppos < 0)
		return -EINVAL;
	if (cnt <= 0)
		return 0;
	if (!access_ok(buf, cnt))
		return -EFAULT;

	read_buf = kvzalloc(HISI_PTP_DBGFS_REG_LEN, GFP_KERNEL);
	if (!read_buf)
		return -ENOMEM;

	len = HISI_PTP_DBGFS_REG_LEN;

	write_lock_irqsave(&ptp->rw_lock, flags);
	if (ptp->ptp_tx && ptp->ptp_tx->base)
		hisi_ptp_dump_reg(ptp->ptp_tx->base, hisi_ptp_tx_reg,
				  ARRAY_SIZE(hisi_ptp_tx_reg),
				  read_buf, len, &pos);

	if (ptp->ptp_tx && ptp->ptp_tx->io_sc_base)
		hisi_ptp_dump_reg(ptp->ptp_tx->io_sc_base, hisi_ptp_tx_io_reg,
				  ARRAY_SIZE(hisi_ptp_tx_io_reg),
				  read_buf, len, &pos);

	list_for_each_entry(rx, &ptp->ptp_rx_list, node)
		hisi_ptp_dump_reg(rx->base, hisi_ptp_rx_reg,
				  ARRAY_SIZE(hisi_ptp_rx_reg),
				  read_buf, len, &pos);

	write_unlock_irqrestore(&ptp->rw_lock, flags);

	size = simple_read_from_buffer(buf, cnt, ppos, read_buf,
				       strlen(read_buf));

	kvfree(read_buf);

	return size;
}

static const struct file_operations hisi_ptp_dbg_state_ops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = hisi_ptp_dbg_read_state,
};

static const struct file_operations hisi_ptp_dbg_reg_ops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = hisi_ptp_dbg_read_reg,
};

static void hisi_ptp_dbgfs_init(struct hisi_ptp_pdev *ptp)
{
	ptp->dbgfs_root = debugfs_create_dir(HISI_PTP_NAME, NULL);
	debugfs_create_file("state", 0400, ptp->dbgfs_root, ptp,
			    &hisi_ptp_dbg_state_ops);
	debugfs_create_file("reg", 0400, ptp->dbgfs_root, ptp,
			    &hisi_ptp_dbg_reg_ops);
}

static void hisi_ptp_dbgfs_uninit(struct hisi_ptp_pdev *ptp)
{
	debugfs_remove_recursive(ptp->dbgfs_root);
}

static int __init hisi_ptp_module_init(void)
{
	struct hisi_ptp_pdev *ptp = &g_ptpdev;
	int ret;

	memset(ptp, 0, sizeof(struct hisi_ptp_pdev));
	rwlock_init(&ptp->rw_lock);
	INIT_LIST_HEAD(&ptp->ptp_rx_list);

	timer_setup(&ptp->suppress_timer, hisi_ptp_timer, 0);

	ret = platform_driver_register(&hisi_ptp_driver);
	if (ret) {
		del_timer_sync(&ptp->suppress_timer);
		pr_err("failed to register ptp platform driver, ret = %d\n",
		       ret);
		return ret;
	}

	hisi_ptp_dbgfs_init(ptp);

	pr_info("hisi ptp platform driver inited, version: %s\n",
		HISI_PTP_VERSION);

	return 0;
}
module_init(hisi_ptp_module_init);

static void __exit hisi_ptp_module_exit(void)
{
	struct hisi_ptp_pdev *ptp = &g_ptpdev;

	pr_info("hisi ptp platform driver exit\n");

	hisi_ptp_dbgfs_uninit(ptp);

	platform_driver_unregister(&hisi_ptp_driver);

	if (ptp->suppress_timer.function)
		del_timer_sync(&ptp->suppress_timer);

	memset(ptp, 0, sizeof(struct hisi_ptp_pdev));
}
module_exit(hisi_ptp_module_exit);

MODULE_DESCRIPTION("HiSilicon PTP driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(HISI_PTP_VERSION);
