// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium Soc PCIe performance monitoring unit support
 *
 * Copyright (c) 2023, Phytium Technology Co., Ltd.
 */

#include <linux/acpi.h>
#include <linux/bitfield.h>
#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/bug.h>
#include <linux/cpuhotplug.h>
#include <linux/cpumask.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/perf_event.h>
#include <linux/platform_device.h>
#include <linux/smp.h>
#include <linux/types.h>

#include <asm/cputype.h>
#include <asm/local64.h>

#undef pr_fmt
#define pr_fmt(fmt) "phytium_pcie_pmu: " fmt

#define PHYTIUM_PCIE_MAX_COUNTERS 18

#define PCIE_START_TIMER 0x000
#define PCIE_STOP_TIMER 0x004
#define PCIE_CLEAR_EVENT 0x008
#define PCIE_SET_TIMER_L 0x00c
#define PCIE_SET_TIMER_H 0x010
#define PCIE_TRIG_MODE 0x014

#define PCIE_NOW_STATE 0x0e0
#define PCIE_EVENT_CYCLES 0x0e4
#define PCIE_TPOINT_END_L 0x0e4
#define PCIE_TPOINT_END_H 0x0e8
#define PCIE_STATE_STOP 0x0ec

#define PCIE_EVENT_AW 0x100
#define PCIE_EVENT_W_LAST 0x104
#define PCIE_EVENT_B 0x108
#define PCIE_EVENT_AR 0x10c
#define PCIE_EVENT_R_LAST 0x110
#define PCIE_EVENT_R_FULL 0x114
#define PCIE_EVENT_R_ERR 0x118
#define PCIE_EVENT_W_ERR 0x11c
#define PCIE_EVENT_DELAY_RD 0x120
#define PCIE_EVENT_DELAY_WR 0x124
#define PCIE_EVENT_RD_MAX 0x128
#define PCIE_EVENT_RD_MIN 0x12c
#define PCIE_EVENT_WR_MAX 0x130
#define PCIE_EVENT_WR_MIN 0x134

#define PCIE_EVENT_W_DATA 0x200
#define PCIE_W_DATA_BASE 0x200

#define PCIE_EVENT_RDELAY_TIME 0x300
#define PCIE_RDELAY_TIME_BASE 0x300

#define PCIE_EVENT_WDELAY_TIME 0x700
#define PCIE_WDELAY_TIME_BASE 0x700

#define PCIE_CLK_FRE 0xe00
#define PCIE_DATA_WIDTH 0xe04

#define to_phytium_pcie_pmu(p) (container_of(p, struct phytium_pcie_pmu, pmu))

static int phytium_pcie_pmu_hp_state;

struct phytium_pcie_pmu_hwevents {
	struct perf_event *hw_events[PHYTIUM_PCIE_MAX_COUNTERS];
	DECLARE_BITMAP(used_mask, PHYTIUM_PCIE_MAX_COUNTERS);
};

struct phytium_pcie_pmu {
	struct device *dev;
	void __iomem *base;
	void __iomem *csr_base;
	void __iomem *irq_reg;
	struct pmu pmu;
	struct phytium_pcie_pmu_hwevents pmu_events;
	u32 die_id;
	u32 pmu_id;
	int on_cpu;
	int irq;
	struct hlist_node node;
	int ctrler_id;
	int real_ctrler;
	u32 clk_bits;
};

#define GET_PCIE_EVENTID(hwc) (hwc->config_base & 0x1F)

#define EVENT_VALID(idx) ((idx >= 0) && (idx < PHYTIUM_PCIE_MAX_COUNTERS))

static const u32 pcie_counter_reg_offset[] = {
	PCIE_EVENT_CYCLES,   PCIE_EVENT_AW,	     PCIE_EVENT_W_LAST,
	PCIE_EVENT_B,	     PCIE_EVENT_AR,	     PCIE_EVENT_R_LAST,
	PCIE_EVENT_R_FULL,   PCIE_EVENT_R_ERR,	     PCIE_EVENT_W_ERR,
	PCIE_EVENT_DELAY_RD, PCIE_EVENT_DELAY_WR,    PCIE_EVENT_RD_MAX,
	PCIE_EVENT_RD_MIN,   PCIE_EVENT_WR_MAX,	     PCIE_EVENT_WR_MIN,
	PCIE_EVENT_W_DATA,   PCIE_EVENT_RDELAY_TIME, PCIE_EVENT_WDELAY_TIME
};

static ssize_t phytium_pcie_pmu_format_sysfs_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct dev_ext_attribute *eattr;

	eattr = container_of(attr, struct dev_ext_attribute, attr);

	return sprintf(buf, "%s\n", (char *)eattr->var);
}

static ssize_t phytium_pcie_pmu_event_sysfs_show(struct device *dev,
					  struct device_attribute *attr,
					  char *page)
{
	struct dev_ext_attribute *eattr;

	eattr = container_of(attr, struct dev_ext_attribute, attr);

	return sprintf(page, "config=0x%lx\n", (unsigned long)eattr->var);
}

static ssize_t cpumask_show(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	struct phytium_pcie_pmu *pcie_pmu =
		to_phytium_pcie_pmu(dev_get_drvdata(dev));

	return cpumap_print_to_pagebuf(true, buf, cpumask_of(pcie_pmu->on_cpu));
}

#define PHYTIUM_PMU_ATTR(_name, _func, _config)                             \
	(&((struct dev_ext_attribute[]){                                    \
		{ __ATTR(_name, 0444, _func, NULL), (void *)_config } })[0] \
		  .attr.attr)

#define PHYTIUM_PCIE_PMU_FORMAT_ATTR(_name, _config)                \
	PHYTIUM_PMU_ATTR(_name, phytium_pcie_pmu_format_sysfs_show, \
			 (void *)_config)

#define PHYTIUM_PCIE_PMU_EVENT_ATTR(_name, _config)                \
	PHYTIUM_PMU_ATTR(_name, phytium_pcie_pmu_event_sysfs_show, \
			 (unsigned long)_config)

static struct attribute *phytium_pcie_pmu_format_attr[] = {
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(event, "config:0-4"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(ctrler, "config:8-10"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(timer, "config1:0-31"),
	NULL,
};

static const struct attribute_group phytium_pcie_pmu_format_group = {
	.name = "format",
	.attrs = phytium_pcie_pmu_format_attr,
};

static struct attribute *phytium_pcie_pmu_events_attr[] = {
	PHYTIUM_PCIE_PMU_EVENT_ATTR(cycles, 0x00),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(aw, 0x01),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_last, 0x02),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(b, 0x03),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(ar, 0x04),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_last, 0x05),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_full, 0x06),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_err, 0x07),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_err, 0x08),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd, 0x09),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr, 0x0a),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(rd_max, 0x0b),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(rd_min, 0x0c),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(wr_max, 0x0d),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(wr_min, 0x0e),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_data, 0x0f),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(rdelay_time, 0x10),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(wdelay_time, 0x11),
	NULL,
};

static const struct attribute_group phytium_pcie_pmu_events_group = {
	.name = "events",
	.attrs = phytium_pcie_pmu_events_attr,
};

static DEVICE_ATTR_RO(cpumask);

static struct attribute *phytium_pcie_pmu_cpumask_attrs[] = {
	&dev_attr_cpumask.attr,
	NULL,
};

static const struct attribute_group phytium_pcie_pmu_cpumask_attr_group = {
	.attrs = phytium_pcie_pmu_cpumask_attrs,
};

static const struct attribute_group *phytium_pcie_pmu_attr_groups[] = {
	&phytium_pcie_pmu_format_group,
	&phytium_pcie_pmu_events_group,
	&phytium_pcie_pmu_cpumask_attr_group,
	NULL,
};

static u32 phytium_pcie_pmu_get_event_ctrler(struct perf_event *event)
{
	return FIELD_GET(GENMASK(10, 8), event->attr.config);
}

static u32 phytium_pcie_pmu_get_event_timer(struct perf_event *event)
{
	return FIELD_GET(GENMASK(31, 0), event->attr.config1);
}

static u64 phytium_pcie_pmu_read_counter(struct phytium_pcie_pmu *pcie_pmu,
					 struct hw_perf_event *hwc)
{
	u32 idx = GET_PCIE_EVENTID(hwc);
	u32 cycle_l, cycle_h, rdelay_l, rdelay_h, w_data, wdelay_l, wdelay_h,
		pcie_data_width;
	u64 val64 = 0;
	int i;
	u32 counter_offset = pcie_counter_reg_offset[idx];

	if (!EVENT_VALID(idx)) {
		dev_err(pcie_pmu->dev, "Unsupported event index:%d!\n", idx);
		return 0;
	}

	switch (idx) {
	case 0:
		cycle_l = readl(pcie_pmu->base + counter_offset);
		cycle_h = readl(pcie_pmu->base + counter_offset + 4);
		val64 = (u64)cycle_h << 32 | (u64)cycle_l;
		break;
	case 15:
		pcie_data_width = readl(pcie_pmu->base + PCIE_DATA_WIDTH);
		for (i = 0; i < (pcie_data_width / 8); i++) {
			w_data = readl(pcie_pmu->base + counter_offset + 4 * i);
			val64 += w_data;
		}
		break;
	case 16:
		for (i = 0; i <= 127; i = i + 2) {
			rdelay_l =
				readl(pcie_pmu->base + counter_offset + 4 * i);
			rdelay_h = readl(pcie_pmu->base + counter_offset +
					 4 * i + 4);
			val64 += (u64)rdelay_h << 32 | (u64)rdelay_l;
		}
		break;
	case 17:
		for (i = 0; i <= 63; i++) {
			wdelay_l =
				readl(pcie_pmu->base + counter_offset + 4 * i);
			wdelay_h = readl(pcie_pmu->base + counter_offset +
					4 * i + 4);
			val64 += (u64)wdelay_h << 32 | (u64)wdelay_l;
		}
		break;
	default:
		val64 = readl(pcie_pmu->base + counter_offset);
		break;
	}
	return val64;
}

static void phytium_pcie_pmu_enable_clk(struct phytium_pcie_pmu *pcie_pmu)
{
	u32 val;

	val = readl(pcie_pmu->csr_base);
	val |= (pcie_pmu->clk_bits);
	writel(val, pcie_pmu->csr_base);
}

static void phytium_pcie_pmu_disable_clk(struct phytium_pcie_pmu *pcie_pmu)
{
	u32 val;

	val = readl(pcie_pmu->csr_base);
	val &= ~(pcie_pmu->clk_bits);
	writel(val, pcie_pmu->csr_base);
}

static void phytium_pcie_pmu_select_ctrler(struct phytium_pcie_pmu *pcie_pmu)
{
	u32 val, offset = 0;

	if (pcie_pmu->pmu_id != 2)
		offset = 0xc;

	val = readl(pcie_pmu->csr_base + offset);

	if (pcie_pmu->pmu_id == 2) {
		val &= 0xffffffcf;
		val |= pcie_pmu->real_ctrler;
	} else {
		val &= 0xfffffffc;
		val |= pcie_pmu->real_ctrler;
	}

	writel(val, pcie_pmu->csr_base + offset);
}

static void
phytium_pcie_pmu_clear_all_counters(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(0x1, pcie_pmu->base + PCIE_CLEAR_EVENT);
}

static void
phytium_pcie_pmu_start_all_counters(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(0x1, pcie_pmu->base + PCIE_START_TIMER);
}

static void
phytium_pcie_pmu_stop_all_counters(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(0x1, pcie_pmu->base + PCIE_STOP_TIMER);
}

static void phytium_pcie_pmu_set_timer(struct phytium_pcie_pmu *pcie_pmu,
					u32 th_val)
{
	u32 val;

	val = readl(pcie_pmu->base + PCIE_SET_TIMER_L);
	val = readl(pcie_pmu->base + PCIE_SET_TIMER_H);

	writel(th_val, pcie_pmu->base + PCIE_SET_TIMER_L);
	writel(0, pcie_pmu->base + PCIE_SET_TIMER_H);
}

static void phytium_pcie_pmu_reset_timer(struct phytium_pcie_pmu *pcie_pmu)
{
	u32 val;

	val = readl(pcie_pmu->base + PCIE_SET_TIMER_L);
	val = readl(pcie_pmu->base + PCIE_SET_TIMER_H);

	writel(0xFFFFFFFF, pcie_pmu->base + PCIE_SET_TIMER_L);
	writel(0xFFFFFFFF, pcie_pmu->base + PCIE_SET_TIMER_H);
}

static unsigned long
phytium_pcie_pmu_get_stop_state(struct phytium_pcie_pmu *pcie_pmu)
{
	unsigned long val;

	val = (unsigned long)readl(pcie_pmu->base + PCIE_STATE_STOP);
	return val;
}

static unsigned long
phytium_pcie_pmu_get_irq_flag(struct phytium_pcie_pmu *pcie_pmu)
{
	unsigned long val;

	val = (unsigned long)readl(pcie_pmu->irq_reg);
	return val;
}

static int phytium_pcie_pmu_mark_event(struct perf_event *event)
{
	struct phytium_pcie_pmu *pcie_pmu = to_phytium_pcie_pmu(event->pmu);
	unsigned long *used_mask = pcie_pmu->pmu_events.used_mask;
	struct hw_perf_event *hwc = &event->hw;

	int idx = GET_PCIE_EVENTID(hwc);

	if (test_bit(idx, used_mask))
		return -EAGAIN;

	set_bit(idx, used_mask);

	return idx;
}

static void phytium_pcie_pmu_unmark_event(struct phytium_pcie_pmu *pcie_pmu,
					  int idx)
{
	if (!EVENT_VALID(idx)) {
		dev_err(pcie_pmu->dev, "Unsupported event index:%d!\n", idx);
		return;
	}

	clear_bit(idx, pcie_pmu->pmu_events.used_mask);
}

static int phytium_pcie_pmu_event_init(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	struct phytium_pcie_pmu *pcie_pmu;
	u32 event_ctrler, event_timer;

	if (event->attr.type != event->pmu->type)
		return -ENOENT;

	if (is_sampling_event(event) || event->attach_state & PERF_ATTACH_TASK)
		return -EOPNOTSUPP;

	pcie_pmu = to_phytium_pcie_pmu(event->pmu);

	if (event->cpu < 0) {
		dev_warn(pcie_pmu->dev, "Can't provide per-task data!\n");
		return -EINVAL;
	}

	if ((event->attr.config & 0x1F) > PHYTIUM_PCIE_MAX_COUNTERS)
		return -EINVAL;

	if (pcie_pmu->on_cpu == -1)
		return -EINVAL;

	event_timer = phytium_pcie_pmu_get_event_timer(event);
	if (event_timer != 0)
		phytium_pcie_pmu_set_timer(pcie_pmu, event_timer);

	event_ctrler = phytium_pcie_pmu_get_event_ctrler(event);
	switch (pcie_pmu->pmu_id) {
	case 0:
		if (event_ctrler != 0) {
			dev_warn(pcie_pmu->dev,
				 "Wrong ctrler id(%d) for pcie-pmu0!\n",
				 event_ctrler);
			return -EINVAL;
		}
		break;
	case 1:
		if ((event_ctrler < 1) || (event_ctrler > 3)) {
			dev_warn(pcie_pmu->dev,
				 "Wrong ctrler id(%d) for pcie-pmu1!\n",
				 event_ctrler);
			return -EINVAL;
		}
		break;
	case 2:
		if ((event_ctrler < 4) || (event_ctrler > 7)) {
			dev_warn(pcie_pmu->dev,
				 "Wrong ctrler id(%d) for pcie-pmu2!\n",
				 event_ctrler);
			return -EINVAL;
		}
		break;
	default:
		dev_err(pcie_pmu->dev, "Unsupported pmu id:%d!\n",
			pcie_pmu->pmu_id);
		return -EINVAL;
	}

	pcie_pmu->ctrler_id = event_ctrler;
	switch (pcie_pmu->pmu_id) {
	case 0:
	case 1:
		pcie_pmu->real_ctrler = pcie_pmu->ctrler_id;
		break;
	case 2:
		pcie_pmu->real_ctrler = (pcie_pmu->ctrler_id - 4) * 16;
		break;
	default:
		dev_err(pcie_pmu->dev, "Unsupported pmu id:%d!\n",
			pcie_pmu->pmu_id);
		return -EINVAL;
	}
	phytium_pcie_pmu_select_ctrler(pcie_pmu);

	hwc->idx = -1;
	hwc->config_base = event->attr.config;

	event->cpu = pcie_pmu->on_cpu;
	return 0;
}

static void phytium_pcie_pmu_event_update(struct perf_event *event)
{
	struct phytium_pcie_pmu *pcie_pmu = to_phytium_pcie_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	u64 delta;

	delta = phytium_pcie_pmu_read_counter(pcie_pmu, hwc);
	local64_add(delta, &event->count);
}

static void phytium_pcie_pmu_event_start(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->state = 0;
	perf_event_update_userpage(event);
}

static void phytium_pcie_pmu_event_stop(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->state |= PERF_HES_STOPPED;

	if (flags & PERF_EF_UPDATE)
		phytium_pcie_pmu_event_update(event);
}

static int phytium_pcie_pmu_event_add(struct perf_event *event, int flags)
{
	struct phytium_pcie_pmu *pcie_pmu = to_phytium_pcie_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int idx;

	hwc->state |= PERF_HES_STOPPED;

	idx = phytium_pcie_pmu_mark_event(event);
	if (idx < 0)
		return idx;

	event->hw.idx = idx;
	pcie_pmu->pmu_events.hw_events[idx] = event;

	return 0;
}

static void phytium_pcie_pmu_event_del(struct perf_event *event, int flags)
{
	struct phytium_pcie_pmu *pcie_pmu = to_phytium_pcie_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	unsigned long val;
	u32 event_timer;

	phytium_pcie_pmu_event_stop(event, PERF_EF_UPDATE);
	val = phytium_pcie_pmu_get_irq_flag(pcie_pmu);
	val = phytium_pcie_pmu_get_stop_state(pcie_pmu);
	phytium_pcie_pmu_unmark_event(pcie_pmu, hwc->idx);

	event_timer = phytium_pcie_pmu_get_event_timer(event);
	if (event_timer != 0)
		phytium_pcie_pmu_reset_timer(pcie_pmu);

	perf_event_update_userpage(event);
	pcie_pmu->pmu_events.hw_events[hwc->idx] = NULL;
}

static void phytium_pcie_pmu_enable(struct pmu *pmu)
{
	struct phytium_pcie_pmu *pcie_pmu = to_phytium_pcie_pmu(pmu);
	int event_added = bitmap_weight(pcie_pmu->pmu_events.used_mask,
					PHYTIUM_PCIE_MAX_COUNTERS);

	if (event_added) {
		phytium_pcie_pmu_clear_all_counters(pcie_pmu);
		phytium_pcie_pmu_start_all_counters(pcie_pmu);
	}
}

static void phytium_pcie_pmu_disable(struct pmu *pmu)
{
	struct phytium_pcie_pmu *pcie_pmu = to_phytium_pcie_pmu(pmu);
	int event_added = bitmap_weight(pcie_pmu->pmu_events.used_mask,
					PHYTIUM_PCIE_MAX_COUNTERS);

	if (event_added)
		phytium_pcie_pmu_stop_all_counters(pcie_pmu);
}

static const struct acpi_device_id phytium_pcie_pmu_acpi_match[] = {
	{
		"PHYT0044",
	},
	{},
};
MODULE_DEVICE_TABLE(acpi, phytium_pcie_pmu_acpi_match);

static irqreturn_t phytium_pcie_pmu_overflow_handler(int irq, void *dev_id)
{
	struct phytium_pcie_pmu *pcie_pmu = dev_id;
	struct perf_event *event;
	unsigned long overflown, stop_state;
	int idx;
	unsigned long *used_mask = pcie_pmu->pmu_events.used_mask;
	int event_added = bitmap_weight(used_mask, PHYTIUM_PCIE_MAX_COUNTERS);

	overflown = phytium_pcie_pmu_get_irq_flag(pcie_pmu);

	if (!test_bit(pcie_pmu->pmu_id + 4, &overflown))
		return IRQ_NONE;

	stop_state = phytium_pcie_pmu_get_stop_state(pcie_pmu);

	if (bitmap_weight(&stop_state, 6)) {
		for_each_set_bit(idx, used_mask, PHYTIUM_PCIE_MAX_COUNTERS) {
			event = pcie_pmu->pmu_events.hw_events[idx];
			if (!event)
				continue;
			phytium_pcie_pmu_event_update(event);
		}
		phytium_pcie_pmu_clear_all_counters(pcie_pmu);
		phytium_pcie_pmu_start_all_counters(pcie_pmu);

		return IRQ_HANDLED;
	}
	if (!event_added) {
		phytium_pcie_pmu_clear_all_counters(pcie_pmu);
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}

static int phytium_pcie_pmu_init_irq(struct phytium_pcie_pmu *pcie_pmu,
				     struct platform_device *pdev)
{
	int irq, ret;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	ret = devm_request_irq(&pdev->dev, irq,
				phytium_pcie_pmu_overflow_handler,
				IRQF_NOBALANCING | IRQF_NO_THREAD | IRQF_SHARED,
				dev_name(&pdev->dev), pcie_pmu);
	if (ret < 0) {
		dev_err(&pdev->dev, "Fail to request IRQ:%d ret:%d\n", irq,
			ret);
		return ret;
	}

	pcie_pmu->irq = irq;

	return 0;
}

static int phytium_pcie_pmu_init_data(struct platform_device *pdev,
				      struct phytium_pcie_pmu *pcie_pmu)
{
	struct resource *res, *clkres, *irqres;

	if (device_property_read_u32(&pdev->dev, "phytium,die-id",
				     &pcie_pmu->die_id)) {
		dev_err(&pdev->dev, "Can not read phytium,die-id!\n");
		return -EINVAL;
	}

	if (device_property_read_u32(&pdev->dev, "phytium,pmu-id",
				     &pcie_pmu->pmu_id)) {
		dev_err(&pdev->dev, "Can not read phytium,pmu-id!\n");
		return -EINVAL;
	}

	switch (pcie_pmu->pmu_id) {
	case 0:
		pcie_pmu->clk_bits = 0x1;
		break;
	case 1:
		pcie_pmu->clk_bits = 0xe;
		break;
	case 2:
		pcie_pmu->clk_bits = 0xf;
		break;
	default:
		dev_err(&pdev->dev, "Unsupported pmu id:%d!\n",
			pcie_pmu->pmu_id);
		break;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pcie_pmu->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pcie_pmu->base)) {
		dev_err(&pdev->dev, "ioremap failed for pcie_pmu resource\n");
		return PTR_ERR(pcie_pmu->base);
	}

	clkres = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!clkres) {
		dev_err(&pdev->dev, "failed for get pcie_pmu clk resource.\n");
		return -EINVAL;
	}

	pcie_pmu->csr_base =
		devm_ioremap(&pdev->dev, clkres->start, resource_size(clkres));
	if (IS_ERR(pcie_pmu->csr_base)) {
		dev_err(&pdev->dev,
			"ioremap failed for pcie_pmu csr resource\n");
		return PTR_ERR(pcie_pmu->csr_base);
	}

	irqres = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!irqres) {
		dev_err(&pdev->dev,
			"failed for get pcie_pmu irq reg resource.\n");
		return -EINVAL;
	}

	pcie_pmu->irq_reg =
		devm_ioremap(&pdev->dev, irqres->start, resource_size(irqres));
	if (IS_ERR(pcie_pmu->irq_reg)) {
		dev_err(&pdev->dev,
			"ioremap failed for pcie_pmu irq resource\n");
		return PTR_ERR(pcie_pmu->irq_reg);
	}

	return 0;
}

static int phytium_pcie_pmu_dev_probe(struct platform_device *pdev,
				      struct phytium_pcie_pmu *pcie_pmu)
{
	int ret;

	ret = phytium_pcie_pmu_init_data(pdev, pcie_pmu);
	if (ret)
		return ret;

	ret = phytium_pcie_pmu_init_irq(pcie_pmu, pdev);
	if (ret)
		return ret;
	pcie_pmu->dev = &pdev->dev;
	pcie_pmu->on_cpu = raw_smp_processor_id();
	WARN_ON(irq_set_affinity(pcie_pmu->irq, cpumask_of(pcie_pmu->on_cpu)));
	pcie_pmu->ctrler_id = -1;

	return 0;
}

static int phytium_pcie_pmu_probe(struct platform_device *pdev)
{
	struct phytium_pcie_pmu *pcie_pmu;
	char *name;
	int ret;

	pcie_pmu = devm_kzalloc(&pdev->dev, sizeof(*pcie_pmu), GFP_KERNEL);
	if (!pcie_pmu)
		return -ENOMEM;

	platform_set_drvdata(pdev, pcie_pmu);

	ret = phytium_pcie_pmu_dev_probe(pdev, pcie_pmu);
	if (ret)
		return ret;

	ret = cpuhp_state_add_instance_nocalls(
		phytium_pcie_pmu_hp_state, &pcie_pmu->node);
	if (ret) {
		dev_err(&pdev->dev, "Error %d registering hotplug;\n", ret);
		return ret;
	}

	name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "phyt%u_pcie_pmu%u",
			      pcie_pmu->die_id, pcie_pmu->pmu_id);
	pcie_pmu->pmu = (struct pmu){
		.name = name,
		.module = THIS_MODULE,
		.task_ctx_nr = perf_invalid_context,
		.event_init = phytium_pcie_pmu_event_init,
		.pmu_enable = phytium_pcie_pmu_enable,
		.pmu_disable = phytium_pcie_pmu_disable,
		.add = phytium_pcie_pmu_event_add,
		.del = phytium_pcie_pmu_event_del,
		.start = phytium_pcie_pmu_event_start,
		.stop = phytium_pcie_pmu_event_stop,
		.read = phytium_pcie_pmu_event_update,
		.attr_groups = phytium_pcie_pmu_attr_groups,
		.capabilities = PERF_PMU_CAP_NO_EXCLUDE,
	};

	ret = perf_pmu_register(&pcie_pmu->pmu, name, -1);
	if (ret) {
		dev_err(pcie_pmu->dev, "PCIE PMU register failed!\n");
		cpuhp_state_remove_instance_nocalls(
			phytium_pcie_pmu_hp_state,
			&pcie_pmu->node);
	}

	phytium_pcie_pmu_enable_clk(pcie_pmu);

	pr_info("Phytium PCIe PMU: ");
	pr_info("die_id = %d pmu_id = %d.\n", pcie_pmu->die_id,
		pcie_pmu->pmu_id);

	return ret;
}

static int phytium_pcie_pmu_remove(struct platform_device *pdev)
{
	struct phytium_pcie_pmu *pcie_pmu = platform_get_drvdata(pdev);

	phytium_pcie_pmu_disable_clk(pcie_pmu);

	perf_pmu_unregister(&pcie_pmu->pmu);
	cpuhp_state_remove_instance_nocalls(
		phytium_pcie_pmu_hp_state, &pcie_pmu->node);

	return 0;
}

static struct platform_driver phytium_pcie_pmu_driver = {
	.driver = {
			.name = "phytium_pcie_pmu",
			.acpi_match_table = ACPI_PTR(phytium_pcie_pmu_acpi_match),
			.suppress_bind_attrs = true,
	},
	.probe = phytium_pcie_pmu_probe,
	.remove = phytium_pcie_pmu_remove,
};

static int phytium_pcie_pmu_offline_cpu(unsigned int cpu, struct hlist_node *node)
{
	struct phytium_pcie_pmu *pcie_pmu =
		hlist_entry_safe(node, struct phytium_pcie_pmu, node);
	unsigned int target;
	cpumask_t available_cpus;

	if (pcie_pmu->on_cpu != cpu)
		return 0;

	cpumask_and(&available_cpus,
			cpumask_of_node(pcie_pmu->die_id), cpu_online_mask);

	target = cpumask_any_but(&available_cpus, cpu);
	if (target >= nr_cpu_ids) {
		target = cpumask_any_but(cpu_online_mask, cpu);
		if (target >= nr_cpu_ids)
			return 0;
	}

	perf_pmu_migrate_context(&pcie_pmu->pmu, cpu, target);
	WARN_ON(irq_set_affinity(pcie_pmu->irq, cpumask_of(target)));
	pcie_pmu->on_cpu = target;

	return 0;
}

static int __init phytium_pcie_pmu_module_init(void)
{
	int ret;

	phytium_pcie_pmu_hp_state =
		cpuhp_setup_state_multi(CPUHP_AP_ONLINE_DYN,
					"perf/phytium/pciepmu:offline", NULL,
					phytium_pcie_pmu_offline_cpu);
	if (phytium_pcie_pmu_hp_state < 0) {
		pr_err("PCIE PMU: setup hotplug, ret = %d\n",
			phytium_pcie_pmu_hp_state);
		return phytium_pcie_pmu_hp_state;
	}

	ret = platform_driver_register(&phytium_pcie_pmu_driver);
	if (ret)
		cpuhp_remove_multi_state(
			phytium_pcie_pmu_hp_state);

	return ret;
}
module_init(phytium_pcie_pmu_module_init);

static void __exit phytium_pcie_pmu_module_exit(void)
{
	platform_driver_unregister(&phytium_pcie_pmu_driver);
	cpuhp_remove_multi_state(phytium_pcie_pmu_hp_state);
}
module_exit(phytium_pcie_pmu_module_exit);

MODULE_DESCRIPTION("Phytium PCIe PMU driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hu Xianghua <huxianghua@phytium.com.cn>");
