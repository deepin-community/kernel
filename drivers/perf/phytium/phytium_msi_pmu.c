// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium SoC MSI performance monitoring unit support
 *
 * Copyright (c) 2025, Phytium Technology Co., Ltd.
 */

#include <linux/acpi.h>
#include <linux/arm-smccc.h>
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
#include <linux/version.h>
#include <asm/cputype.h>
#include <asm/local64.h>

#undef pr_fmt
#define pr_fmt(fmt) "phytium_msi_pmu: " fmt

#define MISC_PERF_DRIVER_VERSION "1.0.0"

#define PHYTIUM_MISC_MAX_COUNTERS   15
#define PHYTIUM_MSI_PMU_EVENT_MASK	0xF

#define MISC_MSI_MON_CLR_REG			0x1F8
#define MISC_MSI_MON_CLKEN_REG			0x1FC
#define MISC_MSI_MON_STOP_REG			0x200
#define MISC_MSI_MON_START_REG			0x204
#define MISC_MSI_MON_OPT_MASK			GENMASK(2, 0)
#define MISC_PCIE0_MSI_MON_OPT_BIT		BIT(2)
#define MISC_PCIE1_MSI_MON_OPT_BIT		BIT(1)
#define MISC_PCIE2_MSI_MON_OPT_BIT		BIT(0)

#define MISC_PCIE0_MON_SET_TIME_H			0x208
#define MISC_PCIE0_MON_SET_TIME_L			0x20C
#define MISC_PCIE1_MON_SET_TIME_H		    0x210
#define MISC_PCIE1_MON_SET_TIME_L		    0x214
#define MISC_PCIE2_MON_SET_TIME_H		    0x218
#define MISC_PCIE2_MON_SET_TIME_L		    0x21C

#define MISC_MSI_MON_MODE                   0x220
#define MISC_MSI_MON_MODE_MASK				0x15
#define MISC_PCIE0_MSI_BYREQID_MODE_BIT		BIT(0)
#define MISC_PCIE1_MSI_BYREQID_MODE_BIT		BIT(2)
#define MISC_PCIE2_MSI_BYREQID_MODE_BIT		BIT(4)
#define MISC_MSI_MON_TRIG_MODE_MASK         0x2A
#define MISC_PCIE0_MSI_TRIG_MODE_BIT		BIT(1)
#define MISC_PCIE1_MSI_TRIG_MODE_BIT		BIT(3)
#define MISC_PCIE2_MSI_TRIG_MODE_BIT		BIT(5)

#define MISC_PCIE0_MSI_REG_ID_CFG_REG_23		0x250
#define MISC_PCIE0_MSI_REG_ID_CFG_REG_01		0x254
#define MISC_PCIE1_MSI_REG_ID_CFG_REG_23		0x258
#define MISC_PCIE1_MSI_REG_ID_CFG_REG_01		0x25C
#define MISC_PCIE2_MSI_REG_ID_CFG_REG_23		0x260
#define MISC_PCIE2_MSI_REG_ID_CFG_REG_01		0x264
#define MISC_MSI_MON_STATE						0x268

#define MISC_MSI_MON_STATE_STOP					0x284
#define MISC_PCIE0_MON_STATE_STOP_MASK			GENMASK(13, 12)
#define MISC_PCIE1_MON_STATE_STOP_MASK			GENMASK(5, 4)
#define MISC_PCIE2_MON_STATE_STOP_MASK			GENMASK(1, 0)
#define MISC_MSI_MON_OVFL_STATE_MASK			0x2
#define MISC_MSI_MON_COUNT_FULL_MASK			0x2022

#define MISC_PCIE0_REQ_ID_RECORD_23			0x224
#define MISC_PCIE0_REQ_ID_RECORD_01			0x228
#define MISC_PCIE0_REQ_ID_CNT				0x22C

#define MISC_PCIE1_REQ_ID_RECORD_23			0x230
#define MISC_PCIE1_REQ_ID_RECORD_01			0x234
#define MISC_PCIE1_REQ_ID_CNT				0x238

#define MISC_PCIE2_REQ_ID_RECORD_23			0x23C
#define MISC_PCIE2_REQ_ID_RECORD_01			0x240
#define MISC_PCIE2_REQ_ID_CNT				0x24C

#define MISC_PCIE0_MON_TPOINT_END_H      0x26C
#define MISC_PCIE0_MON_TPOINT_END_L      0x270
#define MISC_PCIE1_MON_TPOINT_END_H      0x274
#define MISC_PCIE1_MON_TPOINT_END_L      0x278
#define MISC_PCIE2_MON_TPOINT_END_H      0x27C
#define MISC_PCIE2_MON_TPOINT_END_L      0x280

#define MISC_MON_PIDR0		0xFE0
#define MISC_PMU_VER_BIT	GENMASK(7, 0)
#define MISC_PMU_PART_BIT	GENMASK(11, 8)

#define to_phytium_msi_pmu(p) (container_of(p, struct phytium_msi_pmu, pmu))

static int phytium_msi_pmu_hp_state;

enum {
	MISCV1P0 = 0x01,
};

struct phytium_msi_pmu_hwevents {
	struct perf_event *hw_events[PHYTIUM_MISC_MAX_COUNTERS];
	DECLARE_BITMAP(used_mask, PHYTIUM_MISC_MAX_COUNTERS);
	DECLARE_BITMAP(dev_mask, 3);
};

struct phytium_msi_pmu_event_cfg {
	int by_timer;
	int trig_mode;
	int pcie0_by_reqid;
	int pcie1_by_reqid;
	int pcie2_by_reqid;
	int pcie0_reqid0;
	int pcie0_reqid1;
	int pcie0_reqid2;
	int pcie0_reqid3;
	int pcie1_reqid0;
	int pcie1_reqid1;
	int pcie1_reqid2;
	int pcie1_reqid3;
	int pcie2_reqid0;
	int pcie2_reqid1;
	int pcie2_reqid2;
	int pcie2_reqid3;
	u64 timer;
};

struct phytium_msi_pmu {
	struct device *dev;
	void __iomem *base;
	struct pmu pmu;
	struct phytium_msi_pmu_hwevents pmu_events;
	struct phytium_msi_pmu_event_cfg event_cfg;
	struct hlist_node node;
	u32 die_id;
	u32 ver;
	int on_cpu;
	int irq;
};

#define GET_MISC_EVENTID(hwc) (hwc->config_base & PHYTIUM_MSI_PMU_EVENT_MASK)
#define EVENT_VALID(idx) ((idx >= 0) && (idx < PHYTIUM_MISC_MAX_COUNTERS))

static const u32 misc_counter_reg_offset[] = {
	MISC_PCIE0_REQ_ID_CNT, MISC_PCIE0_REQ_ID_CNT, MISC_PCIE0_REQ_ID_CNT,
	MISC_PCIE0_REQ_ID_CNT, MISC_PCIE0_MON_TPOINT_END_H,
	MISC_PCIE1_REQ_ID_CNT, MISC_PCIE1_REQ_ID_CNT, MISC_PCIE1_REQ_ID_CNT,
	MISC_PCIE1_REQ_ID_CNT, MISC_PCIE1_MON_TPOINT_END_H,
	MISC_PCIE2_REQ_ID_CNT, MISC_PCIE2_REQ_ID_CNT, MISC_PCIE2_REQ_ID_CNT,
	MISC_PCIE2_REQ_ID_CNT, MISC_PCIE2_MON_TPOINT_END_H
};

static const u32 misc_reqid_record_reg_offset[] = {
	MISC_PCIE0_REQ_ID_RECORD_01, MISC_PCIE0_REQ_ID_RECORD_01,
	MISC_PCIE0_REQ_ID_RECORD_23, MISC_PCIE0_REQ_ID_RECORD_23, 0,
	MISC_PCIE1_REQ_ID_RECORD_01, MISC_PCIE1_REQ_ID_RECORD_01,
	MISC_PCIE2_REQ_ID_RECORD_01, MISC_PCIE2_REQ_ID_RECORD_01, 0,
	MISC_PCIE1_REQ_ID_RECORD_23, MISC_PCIE1_REQ_ID_RECORD_23,
	MISC_PCIE2_REQ_ID_RECORD_23, MISC_PCIE2_REQ_ID_RECORD_23, 0
};

static const unsigned long pcie_dev_msi_mon_stop_mask[] = {
	MISC_PCIE0_MON_STATE_STOP_MASK,
	MISC_PCIE1_MON_STATE_STOP_MASK,
	MISC_PCIE2_MON_STATE_STOP_MASK
};

static const unsigned long pcie_dev_msi_mon_opt_bits[] = {
	MISC_PCIE0_MSI_MON_OPT_BIT,
	MISC_PCIE1_MSI_MON_OPT_BIT,
	MISC_PCIE2_MSI_MON_OPT_BIT
};

ssize_t phytium_msi_pmu_format_sysfs_show(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	struct dev_ext_attribute *eattr;

	eattr = container_of(attr, struct dev_ext_attribute, attr);
	return sprintf(buf, "%s\n", (char *)eattr->var);
}

ssize_t phytium_msi_pmu_event_sysfs_show(struct device *dev, struct device_attribute *attr,
					char *page)
{
	struct dev_ext_attribute *eattr;

	eattr = container_of(attr, struct dev_ext_attribute, attr);
	return sprintf(page, "config=0x%lx\n", (unsigned long)eattr->var);
}

static ssize_t cpumask_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct phytium_msi_pmu *misc_pmu =
		to_phytium_msi_pmu(dev_get_drvdata(dev));

	return cpumap_print_to_pagebuf(true, buf, cpumask_of(misc_pmu->on_cpu));
}

#define PHYTIUM_PMU_ATTR(_name, _func, _config)     \
	(&((struct dev_ext_attribute[])             \
	{{__ATTR(_name, 0444, _func, NULL), (void *)_config}})[0]   \
	.attr.attr)

#define PHYTIUM_MSI_PMU_FORMAT_ATTR(_name, _config)                            \
	PHYTIUM_PMU_ATTR(_name, phytium_msi_pmu_format_sysfs_show, (void *)_config)

#define PHYTIUM_MSI_PMU_EVENT_ATTR(_name, _config)                                 \
	PHYTIUM_PMU_ATTR(_name, phytium_msi_pmu_event_sysfs_show, (unsigned long)_config)

#define MISC_PMU_EVENT_ATTR_EXTRACTOR(_name, _config, _start, _end)        \
	static inline int misc_pmu_get_##_name(struct perf_event *event)       \
	{                                                              \
		return FIELD_GET(GENMASK_ULL(_end, _start),                \
				event->attr._config);                     \
	}

MISC_PMU_EVENT_ATTR_EXTRACTOR(event, config, 0, 3);
MISC_PMU_EVENT_ATTR_EXTRACTOR(trig_mode, config, 4, 4);
MISC_PMU_EVENT_ATTR_EXTRACTOR(by_reqid, config, 5, 5);
MISC_PMU_EVENT_ATTR_EXTRACTOR(timer, config2, 0, 63);
MISC_PMU_EVENT_ATTR_EXTRACTOR(reqid0, config1, 0, 15);
MISC_PMU_EVENT_ATTR_EXTRACTOR(reqid1, config1, 16, 31);
MISC_PMU_EVENT_ATTR_EXTRACTOR(reqid2, config1, 32, 47);
MISC_PMU_EVENT_ATTR_EXTRACTOR(reqid3, config1, 48, 63);

static struct attribute *PHYTIUM_MSI_PMU_FORMAT_ATTR[] = {
	PHYTIUM_MSI_PMU_FORMAT_ATTR(event, "config:0-3"),
	PHYTIUM_MSI_PMU_FORMAT_ATTR(trig_mode, "config:4-4"),
	PHYTIUM_MSI_PMU_FORMAT_ATTR(by_reqid, "config:5-5"),
	PHYTIUM_MSI_PMU_FORMAT_ATTR(timer, "config2:0-63"),
	PHYTIUM_MSI_PMU_FORMAT_ATTR(reqid0, "config1:0-15"),
	PHYTIUM_MSI_PMU_FORMAT_ATTR(reqid1, "config1:16-31"),
	PHYTIUM_MSI_PMU_FORMAT_ATTR(reqid2, "config1:32-47"),
	PHYTIUM_MSI_PMU_FORMAT_ATTR(reqid3, "config1:48-63"),
	NULL,
};

static const struct attribute_group phytium_msi_pmu_format_group = {
	.name = "format",
	.attrs = PHYTIUM_MSI_PMU_FORMAT_ATTR,
};

static struct attribute *phytium_msi_pmu_events_attr[] = {
	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie0_msi_cnt0, 0x00),
	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie0_msi_cnt1, 0x01),
	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie0_msi_cnt2, 0x02),
	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie0_msi_cnt3, 0x03),
	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie0_msi_cycles, 0x4),

	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie1_msi_cnt0, 0x05),
	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie1_msi_cnt1, 0x06),
	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie1_msi_cnt2, 0x07),
	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie1_msi_cnt3, 0x08),
	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie1_msi_cycles, 0x09),

	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie2_msi_cnt0, 0x0a),
	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie2_msi_cnt1, 0x0b),
	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie2_msi_cnt2, 0x0c),
	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie2_msi_cnt3, 0x0d),
	PHYTIUM_MSI_PMU_EVENT_ATTR(pcie2_msi_cycles, 0x0e),

	NULL,
};

static const struct attribute_group phytium_msi_pmu_events_group = {
	.name = "events",
	.attrs = phytium_msi_pmu_events_attr,
};

static DEVICE_ATTR_RO(cpumask);

static struct attribute *phytium_msi_pmu_cpumask_attrs[] = {
	&dev_attr_cpumask.attr,
	NULL,
};

static const struct attribute_group phytium_msi_pmu_cpumask_attr_group = {
	.attrs = phytium_msi_pmu_cpumask_attrs,
};

static const struct attribute_group *phytium_msi_pmu_attr_groups[] = {
	&phytium_msi_pmu_format_group,
	&phytium_msi_pmu_events_group,
	&phytium_msi_pmu_cpumask_attr_group,
	NULL,
};

int phytium_msi_pmu_get_event_type(struct perf_event *event)
{
	int idx, event_type;

	idx = (int)misc_pmu_get_event(event);
	event_type = (int)(idx / 5);

	return event_type;
}

static void phytium_msi_pmu_enable_clk(struct phytium_msi_pmu *misc_pmu)
{
	u32 val;

	val = readl(misc_pmu->base + MISC_MSI_MON_CLKEN_REG);
	val |= MISC_MSI_MON_OPT_MASK;
	writel(val, misc_pmu->base + MISC_MSI_MON_CLKEN_REG);
}

static void phytium_msi_pmu_disable_clk(struct phytium_msi_pmu *misc_pmu)
{
	writel(0, misc_pmu->base + MISC_MSI_MON_CLKEN_REG);
}

static void phytium_msi_pmu_start_counters(struct phytium_msi_pmu *misc_pmu, u32 st_val)
{
	writel(st_val, misc_pmu->base + MISC_MSI_MON_START_REG);
}

static void phytium_msi_pmu_start_all_counters(struct phytium_msi_pmu *misc_pmu)
{
	u32 val;

	val = MISC_MSI_MON_OPT_MASK;
	writel(val, misc_pmu->base + MISC_MSI_MON_START_REG);
}

static void phytium_msi_pmu_stop_all_counters(struct phytium_msi_pmu *misc_pmu)
{
	u32 val;

	val = MISC_PCIE0_MSI_MON_OPT_BIT | MISC_PCIE1_MSI_MON_OPT_BIT | MISC_PCIE2_MSI_MON_OPT_BIT;
	writel(val, misc_pmu->base + MISC_MSI_MON_STOP_REG);
}

static void phytium_msi_pmu_clean_mon_mode(struct phytium_msi_pmu *misc_pmu)
{
	writel(0, misc_pmu->base  + MISC_MSI_MON_MODE);
}

static void phytium_msi_pmu_set_trig_mode(struct phytium_msi_pmu *misc_pmu,
						u32 trig_mode)
{
	u32 val;

	val = readl(misc_pmu->base + MISC_MSI_MON_MODE);
	if (trig_mode)
		val |= MISC_MSI_MON_TRIG_MODE_MASK;
	else
		val &= MISC_MSI_MON_MODE_MASK;

	writel(val, misc_pmu->base  + MISC_MSI_MON_MODE);
}

// config by_reqid
static void phytium_msi_pmu_by_reqid_mode(struct phytium_msi_pmu *misc_pmu,
						u32 opt_bit, u32 by_reqid)
{
	u32 val, mask;

	if (by_reqid) {
		mask = MISC_MSI_MON_MODE_MASK & opt_bit;
		val = readl(misc_pmu->base + MISC_MSI_MON_MODE);
		val |= mask;
		writel(val, misc_pmu->base  + MISC_MSI_MON_MODE);
	} else {
		val = readl(misc_pmu->base + MISC_MSI_MON_MODE);
		val &= ~opt_bit;
		writel(val, misc_pmu->base  + MISC_MSI_MON_MODE);
	}
}

// config 4 reqids
static void phytium_msi_pmu_set_req_id(struct phytium_msi_pmu *misc_pmu,
	u32 offset, u32 req_id_val)
{
	u32 val;

	val = readl(misc_pmu->base + offset);
	val |= req_id_val;

	writel(val, misc_pmu->base + offset);
}


// overflow stop status
static unsigned long phytium_msi_pmu_get_now_status(struct phytium_msi_pmu *misc_pmu)
{
	unsigned long val;

	val = (unsigned long)readl(misc_pmu->base + MISC_MSI_MON_STATE);
	return val;
}

// overflow stop reason
static unsigned long phytium_msi_pmu_get_stop_status(struct phytium_msi_pmu *misc_pmu)
{
	unsigned long val;

	val = (unsigned long)readl(misc_pmu->base + MISC_MSI_MON_STATE_STOP);
	return val;
}

static void phytium_msi_pmu_clear_all_counters(struct phytium_msi_pmu *misc_pmu)
{
	u32 val;

	val = MISC_MSI_MON_OPT_MASK;
	writel(val, misc_pmu->base + MISC_MSI_MON_CLR_REG);
}

static void phytium_msi_pmu_clear_counters(struct phytium_msi_pmu *misc_pmu,
						u32 clr_val)
{
	writel(clr_val, misc_pmu->base + MISC_MSI_MON_CLR_REG);
}

static u64 phytium_msi_pmu_read_counter(struct phytium_msi_pmu *misc_pmu,
	struct hw_perf_event *hwc)
{
	u32 cycles_l, cycles_h, req_cnt, reqid_val;
	u32 counter_offset, offset_id, mov_bits, cnt_mov_bits, record_mov_bits;
	u32 reqid[4], reqcnt[4];
	u64 val64;
	int idx, event_type, i, j;
	struct perf_event *event;

	idx = GET_MISC_EVENTID(hwc);
	event = misc_pmu->pmu_events.hw_events[idx];
	if (!EVENT_VALID(idx)) {
		dev_err(misc_pmu->dev, "Unsupported event index:%d!\n", idx);
		return 0;
	}

	counter_offset = misc_counter_reg_offset[idx];
	event_type = phytium_msi_pmu_get_event_type(event);

	if ((idx % 5) == 4) {
		cycles_l = readl(misc_pmu->base + counter_offset + 4);
		cycles_h = readl(misc_pmu->base + counter_offset);
		val64 = (u64)cycles_h << 32 | (u64)cycles_l;
	} else {
		offset_id = (u32)(idx % 5);
		req_cnt = readl(misc_pmu->base + counter_offset);
		mov_bits = offset_id * 4;
		val64 = (u64)(0xF & (req_cnt >> mov_bits));

		j = event_type * 5;
		for (i = 0; i < 4; i++) {
			reqid_val = readl(misc_pmu->base + misc_reqid_record_reg_offset[j]);
			req_cnt = readl(misc_pmu->base + misc_counter_reg_offset[j]);
			record_mov_bits = (i % 2) * 16;
			reqid[i] = (u32)(0xFFFF & (reqid_val >> record_mov_bits));
			cnt_mov_bits = i * 4;
			reqcnt[i] = (u32)(0xF & (req_cnt >> cnt_mov_bits));
			dev_info(misc_pmu->dev, "reqid(%u),cnt=%u\n", reqid[i], reqcnt[i]);
			j += 1;
		}
	}
	return val64;
}

static void phytium_msi_pmu_set_timer(struct perf_event *event, u64 th_val)
{
	struct phytium_msi_pmu *misc_pmu = to_phytium_msi_pmu(event->pmu);
	u32 val_l, val_h;

	val_l = th_val & 0xFFFFFFFF;
	val_h = (th_val >> 32) & 0xFFFFFFFF;

	writel(val_l, misc_pmu->base + MISC_PCIE0_MON_SET_TIME_L);
	writel(val_h, misc_pmu->base + MISC_PCIE0_MON_SET_TIME_H);
	writel(val_l, misc_pmu->base + MISC_PCIE1_MON_SET_TIME_L);
	writel(val_h, misc_pmu->base + MISC_PCIE1_MON_SET_TIME_H);
	writel(val_l, misc_pmu->base + MISC_PCIE2_MON_SET_TIME_L);
	writel(val_h, misc_pmu->base + MISC_PCIE2_MON_SET_TIME_H);
}

static void phytium_msi_pmu_reset_timer(struct phytium_msi_pmu *misc_pmu)
{
	writel(0xFFFFFFFF, misc_pmu->base + MISC_PCIE0_MON_SET_TIME_L);
	writel(0xFFFFFFFF, misc_pmu->base + MISC_PCIE0_MON_SET_TIME_H);

	writel(0xFFFFFFFF, misc_pmu->base + MISC_PCIE1_MON_SET_TIME_L);
	writel(0xFFFFFFFF, misc_pmu->base + MISC_PCIE1_MON_SET_TIME_H);

	writel(0xFFFFFFFF, misc_pmu->base + MISC_PCIE2_MON_SET_TIME_L);
	writel(0xFFFFFFFF, misc_pmu->base + MISC_PCIE2_MON_SET_TIME_H);
}

void phytium_msi_pmu_clean_event_config(struct phytium_msi_pmu *misc_pmu)
{
	misc_pmu->event_cfg.pcie0_by_reqid = -1;
	misc_pmu->event_cfg.pcie1_by_reqid = -1;
	misc_pmu->event_cfg.pcie2_by_reqid = -1;

	misc_pmu->event_cfg.pcie0_reqid0 = -1;
	misc_pmu->event_cfg.pcie0_reqid1 = -1;
	misc_pmu->event_cfg.pcie0_reqid2 = -1;
	misc_pmu->event_cfg.pcie0_reqid3 = -1;

	misc_pmu->event_cfg.pcie1_reqid0 = -1;
	misc_pmu->event_cfg.pcie1_reqid1 = -1;
	misc_pmu->event_cfg.pcie1_reqid2 = -1;
	misc_pmu->event_cfg.pcie1_reqid3 = -1;

	misc_pmu->event_cfg.pcie2_reqid0 = -1;
	misc_pmu->event_cfg.pcie2_reqid1 = -1;
	misc_pmu->event_cfg.pcie2_reqid2 = -1;
	misc_pmu->event_cfg.pcie2_reqid3 = -1;

	misc_pmu->event_cfg.trig_mode = -1;

	phytium_msi_pmu_set_trig_mode(misc_pmu, 0);
	phytium_msi_pmu_clean_mon_mode(misc_pmu);
}

static int phytium_msi_pmu_mark_event(struct perf_event *event)
{
	struct phytium_msi_pmu *misc_pmu = to_phytium_msi_pmu(event->pmu);
	unsigned long *used_mask = misc_pmu->pmu_events.used_mask;
	unsigned long *dev_mask = misc_pmu->pmu_events.dev_mask;
	int idx = (int)misc_pmu_get_event(event);
	int event_type = phytium_msi_pmu_get_event_type(event);

	if (test_bit(idx, used_mask))
		return -EAGAIN;
	set_bit(idx, used_mask);

	if (!test_bit(event_type, dev_mask))
		set_bit(event_type, dev_mask);

	return idx;
}

static void phytium_msi_pmu_unmark_event(struct perf_event *event)
{
	struct phytium_msi_pmu *misc_pmu = to_phytium_msi_pmu(event->pmu);
	int idx = (int)misc_pmu_get_event(event);
	int event_type = phytium_msi_pmu_get_event_type(event);

	if (!EVENT_VALID(idx)) {
		dev_err(misc_pmu->dev, "Unsupported event index:%d!\n", idx);
		return;
	}

	clear_bit(idx, misc_pmu->pmu_events.used_mask);
	clear_bit(event_type, misc_pmu->pmu_events.dev_mask);
}

int phytium_msi_pmu_event_init(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	struct phytium_msi_pmu *misc_pmu;

	if (event->attr.type != event->pmu->type)
		return -ENOENT;

	if (is_sampling_event(event) || event->attach_state & PERF_ATTACH_TASK)
		return -EOPNOTSUPP;

	misc_pmu = to_phytium_msi_pmu(event->pmu);

	if (event->cpu < 0) {
		dev_warn(misc_pmu->dev, "Can't provide per-task data!\n");
		return -EINVAL;
	}

	if ((event->attr.config & PHYTIUM_MSI_PMU_EVENT_MASK) > PHYTIUM_MISC_MAX_COUNTERS)
		return -EINVAL;

	if (misc_pmu->on_cpu == -1)
		return -EINVAL;

	hwc->idx = -1;
	hwc->config_base = event->attr.config;

	event->cpu = misc_pmu->on_cpu;

	return 0;
}

void phytium_msi_pmu_event_update(struct perf_event *event)
{
	struct phytium_msi_pmu *misc_pmu = to_phytium_msi_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	u64 delta;

	delta = phytium_msi_pmu_read_counter(misc_pmu, hwc);
	local64_add(delta, &event->count);
}

void phytium_msi_pmu_event_start(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->state = 0;
	perf_event_update_userpage(event);
}

void phytium_msi_pmu_event_stop(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->state |= PERF_HES_STOPPED;

	if (flags & PERF_EF_UPDATE)
		phytium_msi_pmu_event_update(event);
}

int phytium_msi_pmu_set_event_config(struct phytium_msi_pmu *misc_pmu, int idx)
{
	int event_type;
	int dec0, dec1, dec2, dec3;
	u32 trig_mode, by_reqid, reqid0, reqid1, reqid2, reqid3;
	u32 by_reqid_val, reqid_cfg_offset01, reqid_cfg_offset23;
	struct perf_event *event = misc_pmu->pmu_events.hw_events[idx];

	trig_mode = misc_pmu_get_trig_mode(event);
	if (misc_pmu->event_cfg.trig_mode < 0) {
		phytium_msi_pmu_set_trig_mode(misc_pmu, trig_mode);
		misc_pmu->event_cfg.trig_mode = trig_mode;
	} else if (misc_pmu->event_cfg.trig_mode != trig_mode) {
		dev_err(misc_pmu->dev,
		"Incorrect trig_mode parameter for the same PMU");
		dev_err(misc_pmu->dev, "The event parameters should be the same!");
		return -EINVAL;
	}

	event_type = phytium_msi_pmu_get_event_type(event);
	switch (event_type) {
	case 0:
		/* peu */
		by_reqid = misc_pmu_get_by_reqid(event);
		reqid0 = misc_pmu_get_reqid0(event);
		reqid1 = misc_pmu_get_reqid1(event);
		reqid2 = misc_pmu_get_reqid2(event);
		reqid3 = misc_pmu_get_reqid3(event);
		if (misc_pmu->event_cfg.pcie0_by_reqid < 0) {
			// first read
			misc_pmu->event_cfg.pcie0_by_reqid = by_reqid;
			if (by_reqid) {
				misc_pmu->event_cfg.pcie0_reqid0 = reqid0;
				misc_pmu->event_cfg.pcie0_reqid1 = reqid1;
				misc_pmu->event_cfg.pcie0_reqid2 = reqid2;
				misc_pmu->event_cfg.pcie0_reqid3 = reqid3;
			}
		} else if (misc_pmu->event_cfg.pcie0_by_reqid == 1) {
			dec0 = reqid0 - misc_pmu->event_cfg.pcie0_reqid0;
			dec1 = reqid1 - misc_pmu->event_cfg.pcie0_reqid1;
			dec2 = reqid2 - misc_pmu->event_cfg.pcie0_reqid2;
			dec3 = reqid3 - misc_pmu->event_cfg.pcie0_reqid3;
			if (dec0 || dec1 || dec2 || dec3 || (by_reqid == 0)) {
				dev_err(misc_pmu->dev,
				"Incorrect reqid parameter of pcie0 for the same PMU!");
				dev_err(misc_pmu->dev, "The event parameters should be the same!");
				return -EINVAL;
			}
		} else {
			if (by_reqid) {
				dev_err(misc_pmu->dev,
				"Incorrect by_reqid parameter of pcie0 for the same PMU");
				dev_err(misc_pmu->dev, "The event parameters should be the same!");
				return -EINVAL;
			}
		}
		by_reqid_val = MISC_PCIE0_MSI_BYREQID_MODE_BIT;
		reqid_cfg_offset01 = MISC_PCIE0_MSI_REG_ID_CFG_REG_01;
		reqid_cfg_offset23 = MISC_PCIE0_MSI_REG_ID_CFG_REG_23;
		break;
	case 1:
		/* pxu */
		by_reqid = misc_pmu_get_by_reqid(event);
		reqid0 = misc_pmu_get_reqid0(event);
		reqid1 = misc_pmu_get_reqid1(event);
		reqid2 = misc_pmu_get_reqid2(event);
		reqid3 = misc_pmu_get_reqid3(event);
		if (misc_pmu->event_cfg.pcie1_by_reqid < 0) {
			misc_pmu->event_cfg.pcie1_by_reqid = by_reqid;
			if (by_reqid) {
				misc_pmu->event_cfg.pcie1_reqid0 = reqid0;
				misc_pmu->event_cfg.pcie1_reqid1 = reqid1;
				misc_pmu->event_cfg.pcie1_reqid2 = reqid2;
				misc_pmu->event_cfg.pcie1_reqid3 = reqid3;
			}
		} else if (misc_pmu->event_cfg.pcie1_by_reqid == 1) {
			dec0 = reqid0 - misc_pmu->event_cfg.pcie1_reqid0;
			dec1 = reqid1 - misc_pmu->event_cfg.pcie1_reqid1;
			dec2 = reqid2 - misc_pmu->event_cfg.pcie1_reqid2;
			dec3 = reqid3 - misc_pmu->event_cfg.pcie1_reqid3;
			if (dec0 || dec1 || dec2 || dec3 || (by_reqid == 0)) {
				dev_err(misc_pmu->dev,
				"Incorrect reqid parameter of pcie1 for the same PMU!");
				dev_err(misc_pmu->dev, "The event parameters should be the same!");
				return -EINVAL;
			}
		} else {
			if (by_reqid) {
				dev_err(misc_pmu->dev,
				"Incorrect by_reqid parameter of pcie1 for the same PMU");
				dev_err(misc_pmu->dev, "The event parameters should be the same!");
				return -EINVAL;
			}
		}
		by_reqid_val = MISC_PCIE1_MSI_BYREQID_MODE_BIT;
		reqid_cfg_offset01 = MISC_PCIE1_MSI_REG_ID_CFG_REG_01;
		reqid_cfg_offset23 = MISC_PCIE1_MSI_REG_ID_CFG_REG_23;
		break;
	case 2:
		/* pcu */
		by_reqid = misc_pmu_get_by_reqid(event);
		reqid0 = misc_pmu_get_reqid0(event);
		reqid1 = misc_pmu_get_reqid1(event);
		reqid2 = misc_pmu_get_reqid2(event);
		reqid3 = misc_pmu_get_reqid3(event);
		if (misc_pmu->event_cfg.pcie2_by_reqid < 0) {
			misc_pmu->event_cfg.pcie2_by_reqid = by_reqid;
			if (by_reqid) {
				misc_pmu->event_cfg.pcie2_reqid0 = reqid0;
				misc_pmu->event_cfg.pcie2_reqid1 = reqid1;
				misc_pmu->event_cfg.pcie2_reqid2 = reqid2;
				misc_pmu->event_cfg.pcie2_reqid3 = reqid3;
			}
		} else if (misc_pmu->event_cfg.pcie2_by_reqid == 1) {
			dec0 = reqid0 - misc_pmu->event_cfg.pcie2_reqid0;
			dec1 = reqid1 - misc_pmu->event_cfg.pcie2_reqid1;
			dec2 = reqid2 - misc_pmu->event_cfg.pcie2_reqid2;
			dec3 = reqid3 - misc_pmu->event_cfg.pcie2_reqid3;
			if (dec0 || dec1 || dec2 || dec3 || (by_reqid == 0)) {
				dev_err(misc_pmu->dev,
				"Incorrect reqid parameter of pcie2 for the same PMU!");
				dev_err(misc_pmu->dev, "The event parameters should be the same!");
				return -EINVAL;
			}
		} else {
			if (by_reqid) {
				dev_err(misc_pmu->dev,
				"Incorrect by_reqid parameter of pcie2 for the same PMU");
				dev_err(misc_pmu->dev, "The event parameters should be the same!");
				return -EINVAL;
			}
		}
		by_reqid_val = MISC_PCIE2_MSI_BYREQID_MODE_BIT;
		reqid_cfg_offset01 = MISC_PCIE2_MSI_REG_ID_CFG_REG_01;
		reqid_cfg_offset23 = MISC_PCIE2_MSI_REG_ID_CFG_REG_23;
		break;
	default:
		return 0;
	}

	phytium_msi_pmu_by_reqid_mode(misc_pmu, by_reqid_val, by_reqid);
	if (by_reqid) {
		phytium_msi_pmu_set_req_id(misc_pmu, reqid_cfg_offset01,
						(reqid0 | (reqid1 << 16)));
		phytium_msi_pmu_set_req_id(misc_pmu, reqid_cfg_offset23,
						(reqid2 | (reqid3 << 16)));
	}

	return 0;
}

void phytium_msi_pmu_reset_event_config(struct phytium_msi_pmu *misc_pmu,
							unsigned long dev_mask)
{
	int idx;
	u32 by_reqid, reqid_cfg_offset01, reqid_cfg_offset23, by_reqid_val;
	u32 reqid0, reqid1, reqid2, reqid3;

	for_each_set_bit(idx, &dev_mask, 3) {
		switch (idx) {
		case 0:
			by_reqid = misc_pmu->event_cfg.pcie0_by_reqid;
			reqid0 = misc_pmu->event_cfg.pcie0_reqid0;
			reqid1 = misc_pmu->event_cfg.pcie0_reqid1;
			reqid2 = misc_pmu->event_cfg.pcie0_reqid2;
			reqid3 = misc_pmu->event_cfg.pcie0_reqid3;
			by_reqid_val = MISC_PCIE0_MSI_BYREQID_MODE_BIT;
			reqid_cfg_offset01 = MISC_PCIE0_MSI_REG_ID_CFG_REG_01;
			reqid_cfg_offset23 = MISC_PCIE0_MSI_REG_ID_CFG_REG_23;
			break;
		case 1:
			by_reqid = misc_pmu->event_cfg.pcie1_by_reqid;
			reqid0 = misc_pmu->event_cfg.pcie1_reqid0;
			reqid1 = misc_pmu->event_cfg.pcie1_reqid1;
			reqid2 = misc_pmu->event_cfg.pcie1_reqid2;
			reqid3 = misc_pmu->event_cfg.pcie1_reqid3;
			by_reqid_val = MISC_PCIE1_MSI_BYREQID_MODE_BIT;
			reqid_cfg_offset01 = MISC_PCIE1_MSI_REG_ID_CFG_REG_01;
			reqid_cfg_offset23 = MISC_PCIE1_MSI_REG_ID_CFG_REG_23;
			break;
		case 2:
			by_reqid = misc_pmu->event_cfg.pcie2_by_reqid;
			reqid0 = misc_pmu->event_cfg.pcie2_reqid0;
			reqid1 = misc_pmu->event_cfg.pcie2_reqid1;
			reqid2 = misc_pmu->event_cfg.pcie2_reqid2;
			reqid3 = misc_pmu->event_cfg.pcie2_reqid3;
			by_reqid_val = MISC_PCIE2_MSI_BYREQID_MODE_BIT;
			reqid_cfg_offset01 = MISC_PCIE2_MSI_REG_ID_CFG_REG_01;
			reqid_cfg_offset23 = MISC_PCIE2_MSI_REG_ID_CFG_REG_23;
			break;
		default:
			return;
		}

		phytium_msi_pmu_by_reqid_mode(misc_pmu, by_reqid_val, by_reqid);
		if (by_reqid) {
			phytium_msi_pmu_set_req_id(misc_pmu, reqid_cfg_offset01,
							(reqid0 | (reqid1 << 16)));
			phytium_msi_pmu_set_req_id(misc_pmu, reqid_cfg_offset23,
							(reqid2 | (reqid3 << 16)));
		}
	}

	if (misc_pmu->event_cfg.trig_mode >= 0)
		phytium_msi_pmu_set_trig_mode(misc_pmu, misc_pmu->event_cfg.trig_mode);

}

int phytium_msi_pmu_event_add(struct perf_event *event, int flags)
{
	struct phytium_msi_pmu *misc_pmu = to_phytium_msi_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int idx, ret;
	u64 event_timer = misc_pmu_get_timer(event);

	phytium_msi_pmu_enable_clk(misc_pmu);

	hwc->state |= PERF_HES_STOPPED;

	idx = phytium_msi_pmu_mark_event(event);
	if (idx < 0)
		return idx;

	event->hw.idx = idx;
	misc_pmu->pmu_events.hw_events[idx] = event;

	ret = phytium_msi_pmu_set_event_config(misc_pmu, idx);
	if (ret)
		return ret;

	if (event_timer != 0)
		phytium_msi_pmu_set_timer(event, event_timer);
	return 0;
}

void phytium_msi_pmu_event_del(struct perf_event *event, int flags)
{
	struct phytium_msi_pmu *misc_pmu = to_phytium_msi_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	u32 event_timer;

	phytium_msi_pmu_event_stop(event, PERF_EF_UPDATE);

	event_timer = misc_pmu_get_timer(event);
	if (event_timer != 0)
		phytium_msi_pmu_reset_timer(misc_pmu);

	phytium_msi_pmu_unmark_event(event);

	perf_event_update_userpage(event);
	misc_pmu->pmu_events.hw_events[hwc->idx] = NULL;
}

void phytium_msi_pmu_enable(struct pmu *pmu)
{
	struct phytium_msi_pmu *misc_pmu = to_phytium_msi_pmu(pmu);
	int event_added = bitmap_weight(misc_pmu->pmu_events.used_mask, PHYTIUM_MISC_MAX_COUNTERS);

	if (event_added) {
		phytium_msi_pmu_clear_all_counters(misc_pmu);
		phytium_msi_pmu_start_all_counters(misc_pmu);
	}
}

void phytium_msi_pmu_disable(struct pmu *pmu)
{
	struct phytium_msi_pmu *misc_pmu = to_phytium_msi_pmu(pmu);
	int event_added = bitmap_weight(misc_pmu->pmu_events.used_mask, PHYTIUM_MISC_MAX_COUNTERS);

	if (event_added)
		phytium_msi_pmu_stop_all_counters(misc_pmu);
	else
		phytium_msi_pmu_clean_event_config(misc_pmu);
}

static const struct acpi_device_id phytium_msi_pmu_acpi_match[] = {
	{ "PHYT300D", },
	{},
};
MODULE_DEVICE_TABLE(acpi, phytium_msi_pmu_acpi_match);

static int phytium_msi_pmu_version(struct platform_device *pdev,
	struct phytium_msi_pmu *misc_pmu)
{
	u32 pidr;

	pidr = readl(misc_pmu->base + MISC_MON_PIDR0);
	dev_info(&pdev->dev, "PIDR=%#x,VER=%#lx.\n", pidr, (pidr & MISC_PMU_VER_BIT));
	pidr &= MISC_PMU_VER_BIT;
	if (pidr == 0x1) {
		misc_pmu->ver = MISCV1P0;
	} else {
		dev_err(&pdev->dev, "The current driver does not support this device.\n");
		return -ENODEV;
	}
	return 0;
}

bool is_interrupt_state(unsigned long now_state)
{
	const int shifts[] = {0, 4, 8};

	for (int i = 0; i < 3; i++) {
		unsigned int group_val = (now_state >> shifts[i]) & 0x0F;

		if (group_val == MISC_MSI_MON_OVFL_STATE_MASK)
			return true;
	}

	return false;
}

static irqreturn_t phytium_msi_pmu_overflow_handler(int irq, void *dev_id)
{
	struct phytium_msi_pmu *misc_pmu = dev_id;
	struct perf_event *event;
	unsigned long now_state, stop_state;
	int idx, event_type;
	unsigned long dev_stop_mask, dev_mask;
	unsigned long *used_mask = misc_pmu->pmu_events.used_mask;
	u32 opt_val = 0;
	int event_added = bitmap_weight(used_mask, PHYTIUM_MISC_MAX_COUNTERS);

	// 0:pcu 1:pxu 2:peu
	now_state = phytium_msi_pmu_get_now_status(misc_pmu);

	if (!is_interrupt_state(now_state))
		return IRQ_NONE;

	if (!event_added) {
		phytium_msi_pmu_clear_counters(misc_pmu, MISC_MSI_MON_OPT_MASK);
		return IRQ_HANDLED;
	}

	stop_state = phytium_msi_pmu_get_stop_status(misc_pmu);
	if (stop_state & MISC_MSI_MON_COUNT_FULL_MASK) {
		for_each_set_bit(idx, used_mask, PHYTIUM_MISC_MAX_COUNTERS) {
			event = misc_pmu->pmu_events.hw_events[idx];
			if (!event)
				continue;
			event_type = phytium_msi_pmu_get_event_type(event);
			dev_stop_mask = pcie_dev_msi_mon_stop_mask[event_type];

			if (stop_state & dev_stop_mask & MISC_MSI_MON_COUNT_FULL_MASK) {
				phytium_msi_pmu_event_update(event);
				opt_val |= pcie_dev_msi_mon_opt_bits[event_type];
				set_bit(event_type, &dev_mask);
			}
		}
		phytium_msi_pmu_clear_counters(misc_pmu, opt_val);
		phytium_msi_pmu_start_counters(misc_pmu, opt_val);
	} else {
		for_each_set_bit(idx, used_mask, PHYTIUM_MISC_MAX_COUNTERS) {
			event = misc_pmu->pmu_events.hw_events[idx];
			if (!event)
				continue;
			phytium_msi_pmu_event_update(event);
		}
		phytium_msi_pmu_clear_all_counters(misc_pmu);
	}

	return IRQ_HANDLED;
}

static int phytium_msi_pmu_init_irq(struct platform_device *pdev,
				struct phytium_msi_pmu *misc_pmu)
{
	int irq, ret;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	ret = devm_request_irq(&pdev->dev, irq, phytium_msi_pmu_overflow_handler,
				IRQF_NOBALANCING | IRQF_NO_THREAD | IRQF_SHARED,
				dev_name(&pdev->dev), misc_pmu);
	if (ret < 0) {
		dev_err(&pdev->dev, "Fail to request IRQ:%d ret:%d\n", irq, ret);
		return ret;
	}

	misc_pmu->irq = irq;

	return 0;
}

static int phytium_msi_pmu_init_data(struct platform_device *pdev,
					struct phytium_msi_pmu *misc_pmu)
{
	struct resource *res;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	misc_pmu->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(misc_pmu->base)) {
		dev_err(&pdev->dev, "ioremap failed for misc_pmu base resource\n");
		return PTR_ERR(misc_pmu->base);
	}

	ret = phytium_msi_pmu_version(pdev, misc_pmu);
	if (ret)
		return ret;
	if (device_property_read_u32(&pdev->dev, "phytium,die-id", &misc_pmu->die_id)) {
		dev_err(&pdev->dev, "Can not read phytium,die-id!\n");
		return -EINVAL;
	}

	return 0;
}

static int phytium_msi_pmu_dev_probe(struct platform_device *pdev,
					struct phytium_msi_pmu *misc_pmu)
{
	int ret;

	ret = phytium_msi_pmu_init_data(pdev, misc_pmu);
	if (ret)
		return ret;

	ret = phytium_msi_pmu_init_irq(pdev, misc_pmu);
	if (ret)
		return ret;

	misc_pmu->dev = &pdev->dev;
	misc_pmu->on_cpu = -1;

	return 0;
}

static int phytium_msi_pmu_probe(struct platform_device *pdev)
{
	struct phytium_msi_pmu *misc_pmu;
	char *name;
	int ret;

	misc_pmu = devm_kzalloc(&pdev->dev, sizeof(*misc_pmu), GFP_KERNEL);
	if (!misc_pmu)
		return -ENOMEM;

	platform_set_drvdata(pdev, misc_pmu);

	ret = phytium_msi_pmu_dev_probe(pdev, misc_pmu);
	if (ret)
		return ret;

	ret = cpuhp_state_add_instance(phytium_msi_pmu_hp_state, &misc_pmu->node);
	if (ret) {
		dev_err(&pdev->dev, "Error %d registering hotplug;\n", ret);
		return ret;
	}
	name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "phyt%u_msi_pmu", misc_pmu->die_id);

	misc_pmu->pmu = (struct pmu){
		.name = name,
		.module = THIS_MODULE,
		.task_ctx_nr = perf_invalid_context,
		.event_init = phytium_msi_pmu_event_init,
		.pmu_enable = phytium_msi_pmu_enable,
		.pmu_disable = phytium_msi_pmu_disable,
		.add = phytium_msi_pmu_event_add,
		.del = phytium_msi_pmu_event_del,
		.start = phytium_msi_pmu_event_start,
		.stop = phytium_msi_pmu_event_stop,
		.read = phytium_msi_pmu_event_update,
		.attr_groups = phytium_msi_pmu_attr_groups,
		.capabilities = PERF_PMU_CAP_NO_EXCLUDE,
	};

	ret = perf_pmu_register(&misc_pmu->pmu, name, -1);
	if (ret) {
		dev_err(misc_pmu->dev, "MISC PMU register failed!\n");
		cpuhp_state_remove_instance_nocalls(phytium_msi_pmu_hp_state, &misc_pmu->node);
	}

	phytium_msi_pmu_enable_clk(misc_pmu);

	pr_info("%s on cpu%d.\n", name, misc_pmu->on_cpu);

	return ret;
}

static int phytium_msi_pmu_remove(struct platform_device *pdev)
{
	struct phytium_msi_pmu *misc_pmu = platform_get_drvdata(pdev);

	phytium_msi_pmu_disable_clk(misc_pmu);
	perf_pmu_unregister(&misc_pmu->pmu);
	cpuhp_state_remove_instance_nocalls(phytium_msi_pmu_hp_state, &misc_pmu->node);

	return 0;
}

static struct platform_driver phytium_msi_pmu_driver = {
	.driver = {
			.name = "phytium_msi_pmu",
			.acpi_match_table = ACPI_PTR(phytium_msi_pmu_acpi_match),
			.suppress_bind_attrs = true,
	},
	.probe = phytium_msi_pmu_probe,
	.remove = phytium_msi_pmu_remove,
};

int phytium_msi_pmu_online_cpu(unsigned int cpu, struct hlist_node *node)
{
	struct phytium_msi_pmu *misc_pmu =
		hlist_entry_safe(node, struct phytium_msi_pmu, node);

	if (!cpumask_test_cpu(cpu, cpumask_of_node(misc_pmu->die_id)))
		return 0;

	if (misc_pmu->on_cpu != -1) {
		if (!cpumask_test_cpu(misc_pmu->on_cpu, cpumask_of_node(misc_pmu->die_id))) {
			perf_pmu_migrate_context(&misc_pmu->pmu, misc_pmu->on_cpu, cpu);
			misc_pmu->on_cpu = cpu;
			WARN_ON(irq_set_affinity_hint(misc_pmu->irq, cpumask_of(cpu)));
		}
		return 0;
	}

	misc_pmu->on_cpu = cpu;

	return 0;
}

int phytium_msi_pmu_offline_cpu(unsigned int cpu, struct hlist_node *node)
{
	struct phytium_msi_pmu *misc_pmu = hlist_entry_safe(node,
							struct phytium_msi_pmu, node);
	unsigned int target;
	cpumask_t available_cpus;

	if (misc_pmu->on_cpu != cpu)
		return 0;

	if (cpumask_and(&available_cpus, cpumask_of_node(misc_pmu->die_id), cpu_online_mask) &&
		cpumask_andnot(&available_cpus, &available_cpus, cpumask_of(cpu)))
		target = cpumask_last(&available_cpus);
	else {
		cpumask_andnot(&available_cpus, cpu_online_mask, cpumask_of(cpu));
		target = cpumask_last(&available_cpus);
	}

	if (target >= nr_cpu_ids) {
		dev_err(misc_pmu->dev, "offline cpu%d with no target to migrate.\n", cpu);
		return 0;
	}

	perf_pmu_migrate_context(&misc_pmu->pmu, cpu, target);
	misc_pmu->on_cpu = target;

	return 0;
}

static int __init phytium_msi_pmu_module_init(void)
{
	int ret;

	phytium_msi_pmu_hp_state =
		cpuhp_setup_state_multi(CPUHP_AP_ONLINE_DYN,
					"perf/phytium/miscpmu:online",
					phytium_msi_pmu_online_cpu,
					phytium_msi_pmu_offline_cpu);
	if (phytium_msi_pmu_hp_state < 0) {
		pr_err("MISC PMU: setup hotplug, ret = %d\n", phytium_msi_pmu_hp_state);
		return phytium_msi_pmu_hp_state;
	}

	ret = platform_driver_register(&phytium_msi_pmu_driver);
	if (ret)
		cpuhp_remove_multi_state(phytium_msi_pmu_hp_state);

	return ret;
}
module_init(phytium_msi_pmu_module_init);

static void __exit phytium_msi_pmu_module_exit(void)
{
	platform_driver_unregister(&phytium_msi_pmu_driver);
	cpuhp_remove_multi_state(phytium_msi_pmu_hp_state);
}
module_exit(phytium_msi_pmu_module_exit);

MODULE_DESCRIPTION("Phytium MISC PMU driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(MISC_PERF_DRIVER_VERSION);
MODULE_AUTHOR("Tan Rui <tanrui2142@phytium.com.cn>");
MODULE_AUTHOR("Fu Boyi <fuboyi2150@phytium.com.cn>");
