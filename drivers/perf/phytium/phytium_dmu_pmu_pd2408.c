// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium PD2408 DMU performance monitoring unit support
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
#define pr_fmt(fmt) "pd2408_dmu_pmu: " fmt

#define DMU_PERF_DRIVER_VERSION "1.0.2"

#define DMU_PMU_TIMER_START     0x0
#define DMU_PMU_TIMER_STOP      0x4
#define DMU_PMU_CLEAR_EVENT     0x8
#define DMU_PMU_SET_TIMER_L     0xc
#define DMU_PMU_SET_TIMER_H     0x10
#define DMU_PMU_AXI_MONITOR_EN			0x1c
#define DMU_PMU_TIMER_INT_CLEAR             0x2c
#define DMU_PMU_AXI_MONITOR_INT_CLEAR       0x30
#define DMU_PMU_TIMER_INT_STA               0x40
#define DMU_PMU_AXI_MONITOR_INT_STA         0x44
#define DMU_PMU_TIMER_INT_MASK              0x54
#define DMU_PMU_AXI_MONITOR_INT_MASK        0x58

#define DMU_PMU_EVENT_CYCLES 0x208

#define DMU_PMU_EVENT_AXI_READ_CMD_CNT              0x074
#define DMU_PMU_EVENT_AXI_WRITE_CMD_CNT             0x07c
#define DMU_PMU_EVENT_AXI_READ_FLUX_CNT             0x084
#define DMU_PMU_EVENT_AXI_WRITE_FLUX_CNT            0x08c

#define DMU_PMU_MAX_COUNTERS 5
#define DMU_PMU_MAX_COUNTERS_TIMER 1
#define DMU_PMU_MAX_COUNTERS_AXI 4

#define ALL_EVENT_CLEAR_BIT		0x1
#define DMU_PMU_TIMER_OPT_BIT		0x1
#define AXI_MONITOR_OPT_BIT		0x01010101

#define DMU_PMU_NOTICE_START  0x0
#define DMU_PMU_NOTICE_STOP   0x1

#define to_pd2408_dmu_pmu(p) (container_of(p, struct pd2408_dmu_pmu, pmu))

#define GET_DMU_EVENTID(hwc) (hwc->config_base & 0x7)
#define EVENT_VALID(idx) ((idx >= 0) && (idx < DMU_PMU_MAX_COUNTERS))

static int pd2408_dmu_pmu_hp_state;
int used_event;

struct pd2408_dmu_pmu_hwevents {
	struct perf_event *hw_events[DMU_PMU_MAX_COUNTERS];
	DECLARE_BITMAP(used_event_mask, DMU_PMU_MAX_COUNTERS);
};

struct pd2408_dmu_pmu {
	struct device *dev;
	void __iomem *base;
	struct pmu pmu;
	struct pd2408_dmu_pmu_hwevents pmu_events;
	struct hlist_node node;
	int on_cpu;
	int irq;
	u32 soc_version;
	u32 dmu_id;
	bool used_flag;
};

static const u32 dmu_counter_reg_offset[] = {
	DMU_PMU_EVENT_CYCLES,
	DMU_PMU_EVENT_AXI_WRITE_FLUX_CNT, DMU_PMU_EVENT_AXI_READ_FLUX_CNT,
	DMU_PMU_EVENT_AXI_WRITE_CMD_CNT, DMU_PMU_EVENT_AXI_READ_CMD_CNT
};

ssize_t pd2408_dmu_pmu_format_sysfs_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	struct dev_ext_attribute *eattr;

	eattr = container_of(attr, struct dev_ext_attribute, attr);

	return sprintf(buf, "%s\n", (char *)eattr->var);
}

ssize_t pd2408_dmu_pmu_event_sysfs_show(struct device *dev,
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
	struct pd2408_dmu_pmu *dmu_pmu =
		to_pd2408_dmu_pmu(dev_get_drvdata(dev));

	return cpumap_print_to_pagebuf(true, buf, cpumask_of(dmu_pmu->on_cpu));
}

#define PHYTIUM_PMU_ATTR(_name, _func, _config)                             \
	(&((struct dev_ext_attribute[]){                                    \
		{ __ATTR(_name, 0444, _func, NULL), (void *)_config } })[0] \
		  .attr.attr)

#define PHYTIUM_DMU_PMU_FORMAT_ATTR(_name, _config)                \
	PHYTIUM_PMU_ATTR(_name, pd2408_dmu_pmu_format_sysfs_show, \
			 (void *)_config)

#define PHYTIUM_DMU_PMU_EVENT_ATTR(_name, _config)                \
	PHYTIUM_PMU_ATTR(_name, pd2408_dmu_pmu_event_sysfs_show, \
			 (unsigned long)_config)

static struct attribute *pd2408_dmu_pmu_format_attr[] = {
	PHYTIUM_DMU_PMU_FORMAT_ATTR(event, "config:0-2"),
	NULL,
};

static const struct attribute_group pd2408_dmu_pmu_format_group = {
	.name = "format",
	.attrs = pd2408_dmu_pmu_format_attr,
};

static struct attribute *pd2408_dmu_pmu_events_attr[] = {
	PHYTIUM_DMU_PMU_EVENT_ATTR(dmu_axi_cycles, 0x00),
	PHYTIUM_DMU_PMU_EVENT_ATTR(axi_write_flux, 0x01),
	PHYTIUM_DMU_PMU_EVENT_ATTR(axi_read_flux, 0x02),
	PHYTIUM_DMU_PMU_EVENT_ATTR(axi_write_cmd, 0x03),
	PHYTIUM_DMU_PMU_EVENT_ATTR(axi_read_cmd, 0x04),
	NULL,
};

static const struct attribute_group pd2408_dmu_pmu_events_group = {
	.name = "events",
	.attrs = pd2408_dmu_pmu_events_attr,
};

static DEVICE_ATTR_RO(cpumask);

static struct attribute *pd2408_dmu_pmu_cpumask_attrs[] = {
	&dev_attr_cpumask.attr,
	NULL,
};

static const struct attribute_group pd2408_dmu_pmu_cpumask_attr_group = {
	.attrs = pd2408_dmu_pmu_cpumask_attrs,
};

static const struct attribute_group *pd2408_dmu_pmu_attr_groups[] = {
	&pd2408_dmu_pmu_format_group,
	&pd2408_dmu_pmu_events_group,
	&pd2408_dmu_pmu_cpumask_attr_group,
	NULL,
};

#if IS_ENABLED(CONFIG_ARM_PHYTIUM_DMU_DEVFREQ)
extern struct blocking_notifier_head dmu_pmu_notifier_chain;

void pd2408_dmu_pmu_notifier_chain_trigger(struct pd2408_dmu_pmu *dmu_pmu, int event)
{
	static bool start_flag;

	if ((event == DMU_PMU_NOTICE_START) && (start_flag == false)) {
		blocking_notifier_call_chain(&dmu_pmu_notifier_chain, event, NULL);
		start_flag = true;
		dmu_pmu->used_flag = true;
	} else if ((event == DMU_PMU_NOTICE_STOP) && (start_flag == true)) {
		blocking_notifier_call_chain(&dmu_pmu_notifier_chain, event, NULL);
		start_flag = false;
		dmu_pmu->used_flag = false;
	}
}
#endif

static u64 pd2408_dmu_pmu_read_counter(struct pd2408_dmu_pmu *dmu_pmu,
					   struct hw_perf_event *hwc)
{
	u32 val32_l, val32_h, idx, counter_offset;
	u64 val64;

	idx = GET_DMU_EVENTID(hwc);
	counter_offset = dmu_counter_reg_offset[idx];

	if (!EVENT_VALID(idx)) {
		dev_err(dmu_pmu->dev, "Unsupported event index:%d!\n", idx);
		return 0;
	}

	val32_l = readl(dmu_pmu->base + counter_offset);
	val32_h = readl(dmu_pmu->base + counter_offset + 4);
	val64 = (u64)val32_h << 32 | (u64)val32_l;

	return val64;
}

static void pd2408_dmu_pmu_clear_all_counters(struct pd2408_dmu_pmu *dmu_pmu)
{
	writel(BIT(0), dmu_pmu->base + DMU_PMU_CLEAR_EVENT);
}

static void pd2408_dmu_pmu_disable_axi_cmd_events(struct pd2408_dmu_pmu *dmu_pmu)
{
	writel(0x0, dmu_pmu->base + DMU_PMU_AXI_MONITOR_EN);
}

static void pd2408_dmu_pmu_mask_all_irq(struct pd2408_dmu_pmu *dmu_pmu)
{
	writel(DMU_PMU_TIMER_OPT_BIT, dmu_pmu->base + DMU_PMU_TIMER_INT_MASK);
	writel(AXI_MONITOR_OPT_BIT, dmu_pmu->base + DMU_PMU_AXI_MONITOR_INT_MASK);
}

static void pd2408_dmu_pmu_start_all_counters(struct pd2408_dmu_pmu *dmu_pmu)
{
	writel(BIT(0), dmu_pmu->base + DMU_PMU_TIMER_START);
}

static void pd2408_dmu_pmu_stop_all_counters(struct pd2408_dmu_pmu *dmu_pmu)
{
	writel(BIT(0), dmu_pmu->base + DMU_PMU_TIMER_STOP);
}

static void pd2408_dmu_pmu_reset_timer(struct pd2408_dmu_pmu *dmu_pmu)
{
	writel(0xFFFFFFFF, dmu_pmu->base + DMU_PMU_SET_TIMER_L);
	writel(0xFFFFFFFF, dmu_pmu->base + DMU_PMU_SET_TIMER_H);
}

static void pd2408_dmu_pmu_enable_events(struct pd2408_dmu_pmu *dmu_pmu, int idx)
{
	u8 en_bit;
	u32 en_offset, irq_offset, val;

	if (idx == 0) {
		en_bit = 0;
		en_offset = 0;
		irq_offset = DMU_PMU_TIMER_INT_MASK;
	} else {
		en_bit = (idx - 1) * 8;
		en_offset = DMU_PMU_AXI_MONITOR_EN;
		irq_offset = DMU_PMU_AXI_MONITOR_INT_MASK;
		}

	if (en_offset) {
		val = readl(dmu_pmu->base + en_offset);
		val |= BIT(en_bit);
		writel(val, dmu_pmu->base + en_offset);
	}

	val = readl(dmu_pmu->base + irq_offset);
	val &= ~BIT(en_bit);
	writel(val, dmu_pmu->base + irq_offset);
}

static int pd2408_dmu_pmu_mark_event(struct perf_event *event)
{
	struct pd2408_dmu_pmu *dmu_pmu = to_pd2408_dmu_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	unsigned long *used_mask = dmu_pmu->pmu_events.used_event_mask;

	int idx = GET_DMU_EVENTID(hwc);

	if (test_bit(idx, used_mask))
		return -EAGAIN;

	set_bit(idx, used_mask);

	return idx;
}

static void pd2408_dmu_pmu_unmark_event(struct pd2408_dmu_pmu *dmu_pmu,
					 int idx)
{
	if (!EVENT_VALID(idx)) {
		dev_err(dmu_pmu->dev, "Unsupported event index:%d!\n", idx);
		return;
	}

	clear_bit(idx, dmu_pmu->pmu_events.used_event_mask);
}

int pd2408_dmu_pmu_event_init(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	struct pd2408_dmu_pmu *dmu_pmu;

	if (event->attr.type != event->pmu->type)
		return -ENOENT;

	if (is_sampling_event(event) || event->attach_state & PERF_ATTACH_TASK)
		return -EOPNOTSUPP;

	dmu_pmu = to_pd2408_dmu_pmu(event->pmu);

	if (event->cpu < 0) {
		dev_warn(dmu_pmu->dev, "Can't provide per-task data!\n");
		return -EINVAL;
	}

	if (event->attr.config > DMU_PMU_MAX_COUNTERS)
		return -EINVAL;

	if (dmu_pmu->on_cpu == -1)
		return -EINVAL;

	hwc->idx = -1;
	hwc->config_base = event->attr.config;

	event->cpu = dmu_pmu->on_cpu;
	used_event = 0;

	return 0;
}

void pd2408_dmu_pmu_event_update(struct perf_event *event)
{
	struct pd2408_dmu_pmu *dmu_pmu = to_pd2408_dmu_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	u64 delta;

	pd2408_dmu_pmu_stop_all_counters(dmu_pmu);
	delta = pd2408_dmu_pmu_read_counter(dmu_pmu, hwc);
	local64_add(delta, &event->count);

}

void pd2408_dmu_pmu_event_start(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->state = 0;
	perf_event_update_userpage(event);
}

void pd2408_dmu_pmu_event_stop(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->state |= PERF_HES_STOPPED;

	if (flags & PERF_EF_UPDATE)
		pd2408_dmu_pmu_event_update(event);
}

int pd2408_dmu_pmu_event_add(struct perf_event *event, int flags)
{
	struct pd2408_dmu_pmu *dmu_pmu = to_pd2408_dmu_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int idx;

	hwc->state |= PERF_HES_STOPPED;

	idx = pd2408_dmu_pmu_mark_event(event);
	if (!EVENT_VALID(idx)) {
		dev_err(dmu_pmu->dev, "Unsupported event index:%d!\n", idx);
		return idx;
	}
#if IS_ENABLED(CONFIG_ARM_PHYTIUM_DMU_DEVFREQ)
	pd2408_dmu_pmu_notifier_chain_trigger(dmu_pmu, DMU_PMU_NOTICE_START);
#endif
	event->hw.idx = idx;
	dmu_pmu->pmu_events.hw_events[idx] = event;

	pd2408_dmu_pmu_enable_events(dmu_pmu, idx);
	used_event += 1;
	return 0;
}

void pd2408_dmu_pmu_event_del(struct perf_event *event, int flags)
{
	struct pd2408_dmu_pmu *dmu_pmu = to_pd2408_dmu_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;

	used_event -= 1;
	pd2408_dmu_pmu_event_stop(event, PERF_EF_UPDATE);

	pd2408_dmu_pmu_unmark_event(dmu_pmu, hwc->idx);

	perf_event_update_userpage(event);
	dmu_pmu->pmu_events.hw_events[hwc->idx] = NULL;
#if IS_ENABLED(CONFIG_ARM_PHYTIUM_DMU_DEVFREQ)
	if (used_event == 0)
		pd2408_dmu_pmu_notifier_chain_trigger(dmu_pmu, DMU_PMU_NOTICE_STOP);
#endif

}

void pd2408_dmu_pmu_enable(struct pmu *pmu)
{
	struct pd2408_dmu_pmu *dmu_pmu = to_pd2408_dmu_pmu(pmu);
	int event_added;

	event_added = bitmap_weight(dmu_pmu->pmu_events.used_event_mask,
					DMU_PMU_MAX_COUNTERS);

	if (event_added) {
		pd2408_dmu_pmu_stop_all_counters(dmu_pmu);
		pd2408_dmu_pmu_clear_all_counters(dmu_pmu);
		pd2408_dmu_pmu_reset_timer(dmu_pmu);
		pd2408_dmu_pmu_start_all_counters(dmu_pmu);
	}
}

void pd2408_dmu_pmu_disable(struct pmu *pmu)
{
	struct pd2408_dmu_pmu *dmu_pmu = to_pd2408_dmu_pmu(pmu);
	int event_added;

	event_added = bitmap_weight(dmu_pmu->pmu_events.used_event_mask,
					DMU_PMU_MAX_COUNTERS);
	if (event_added && dmu_pmu->used_flag) {
		pd2408_dmu_pmu_mask_all_irq(dmu_pmu);
		pd2408_dmu_pmu_disable_axi_cmd_events(dmu_pmu);
	}
}

static const struct acpi_device_id pd2408_dmu_pmu_acpi_match[] = {
	{ "PHYT0069", },
	{},
};
MODULE_DEVICE_TABLE(acpi, pd2408_dmu_pmu_acpi_match);

static irqreturn_t pd2408_dmu_pmu_overflow_handler(int irq, void *dev_id)
{
	struct pd2408_dmu_pmu *dmu_pmu = dev_id;
	struct perf_event *event;
	int idx;
	unsigned long *used_mask = dmu_pmu->pmu_events.used_event_mask;
	u32 timer_int_sta, axi_int_sta;

	timer_int_sta = readl(dmu_pmu->base + DMU_PMU_TIMER_INT_STA);
	axi_int_sta = readl(dmu_pmu->base + DMU_PMU_AXI_MONITOR_INT_STA);

	if ((timer_int_sta + axi_int_sta) == 0)
		return IRQ_NONE;

	if (timer_int_sta)
		writel(0x1, dmu_pmu->base + DMU_PMU_TIMER_INT_CLEAR);

	if (axi_int_sta)
		writel(axi_int_sta, dmu_pmu->base + DMU_PMU_AXI_MONITOR_INT_CLEAR);

	if (!dmu_pmu->used_flag) {
		pd2408_dmu_pmu_mask_all_irq(dmu_pmu);
		return IRQ_HANDLED;
	}

	for_each_set_bit(idx, used_mask, DMU_PMU_MAX_COUNTERS) {
		event = dmu_pmu->pmu_events.hw_events[idx];
		if (!event)
			continue;
		pd2408_dmu_pmu_event_update(event);
	}
	writel(ALL_EVENT_CLEAR_BIT, dmu_pmu->base + DMU_PMU_CLEAR_EVENT);
	pd2408_dmu_pmu_start_all_counters(dmu_pmu);

	return IRQ_HANDLED;
}

static int pd2408_dmu_pmu_init_irq(struct pd2408_dmu_pmu *dmu_pmu,
				       struct platform_device *pdev)
{
	int irq, ret;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	ret = devm_request_irq(&pdev->dev, irq,
				pd2408_dmu_pmu_overflow_handler,
				IRQF_NOBALANCING | IRQF_NO_THREAD | IRQF_SHARED,
				dev_name(&pdev->dev), dmu_pmu);
	if (ret < 0) {
		dev_err(&pdev->dev, "Fail to request IRQ:%d ret:%d\n", irq,
			ret);
		return ret;
	}

	dmu_pmu->irq = irq;

	return 0;
}

static int pd2408_dmu_pmu_init_data(struct platform_device *pdev,
					struct pd2408_dmu_pmu *dmu_pmu)
{
	struct resource *res;

	if (device_property_read_u32(&pdev->dev, "phytium,ddr-id",
				     &dmu_pmu->dmu_id)) {
		dev_err(&pdev->dev, "Can not read phytium,ddr-id!\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dmu_pmu->base = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(dmu_pmu->base)) {
		dev_err(&pdev->dev,
			"ioremap failed for dmu_pmu base resource\n");
		return PTR_ERR(dmu_pmu->base);
	}

	dmu_pmu->used_flag = true;
	pd2408_dmu_pmu_mask_all_irq(dmu_pmu);

	return 0;
}

static int pd2408_dmu_pmu_dev_probe(struct platform_device *pdev,
					struct pd2408_dmu_pmu *dmu_pmu)
{
	int ret;

	ret = pd2408_dmu_pmu_init_data(pdev, dmu_pmu);
	if (ret)
		return ret;

	ret = pd2408_dmu_pmu_init_irq(dmu_pmu, pdev);
	if (ret)
		return ret;

	dmu_pmu->dev = &pdev->dev;
	dmu_pmu->on_cpu = -1;

	return 0;
}

static int pd2408_dmu_pmu_probe(struct platform_device *pdev)
{
	struct pd2408_dmu_pmu *dmu_pmu;
	char *name;
	int ret;

	dmu_pmu = devm_kzalloc(&pdev->dev, sizeof(*dmu_pmu), GFP_KERNEL);
	if (!dmu_pmu)
		return -ENOMEM;

	platform_set_drvdata(pdev, dmu_pmu);

	ret = pd2408_dmu_pmu_dev_probe(pdev, dmu_pmu);
	if (ret)
		return ret;

	ret = cpuhp_state_add_instance(pd2408_dmu_pmu_hp_state,
					&dmu_pmu->node);
	if (ret) {
		dev_err(&pdev->dev, "Error %d registering hotplug;\n", ret);
		return ret;
	}

	name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "phyt_dmu%u_pmu", dmu_pmu->dmu_id);
	dmu_pmu->pmu = (struct pmu) {
		.name = name,
		.module = THIS_MODULE,
		.task_ctx_nr = perf_invalid_context,
		.event_init = pd2408_dmu_pmu_event_init,
		.pmu_enable = pd2408_dmu_pmu_enable,
		.pmu_disable = pd2408_dmu_pmu_disable,
		.add = pd2408_dmu_pmu_event_add,
		.del = pd2408_dmu_pmu_event_del,
		.start = pd2408_dmu_pmu_event_start,
		.stop = pd2408_dmu_pmu_event_stop,
		.read = pd2408_dmu_pmu_event_update,
		.attr_groups = pd2408_dmu_pmu_attr_groups,
	};

	ret = perf_pmu_register(&dmu_pmu->pmu, name, -1);
	if (ret) {
		dev_err(dmu_pmu->dev, "DMU PMU register failed!\n");
		cpuhp_state_remove_instance_nocalls(pd2408_dmu_pmu_hp_state,
			&dmu_pmu->node);
	}

	pr_info("die_dmu%d_pmu on cpu%d.\n", dmu_pmu->dmu_id, dmu_pmu->on_cpu);

	return ret;
}

static int pd2408_dmu_pmu_remove(struct platform_device *pdev)
{
	struct pd2408_dmu_pmu *dmu_pmu = platform_get_drvdata(pdev);

	pd2408_dmu_pmu_mask_all_irq(dmu_pmu);
	perf_pmu_unregister(&dmu_pmu->pmu);
	cpuhp_state_remove_instance_nocalls(pd2408_dmu_pmu_hp_state,
					&dmu_pmu->node);

	return 0;
}

static struct platform_driver pd2408_dmu_pmu_driver = {
	.driver = {
			.name = "pd2408_dmu_pmu",
			.acpi_match_table = ACPI_PTR(pd2408_dmu_pmu_acpi_match),
			.suppress_bind_attrs = true,
		},
	.probe = pd2408_dmu_pmu_probe,
	.remove = pd2408_dmu_pmu_remove,
};

int pd2408_dmu_pmu_online_cpu(unsigned int cpu, struct hlist_node *node)
{
	struct pd2408_dmu_pmu *dmu_pmu =
		hlist_entry_safe(node, struct pd2408_dmu_pmu, node);

	if (dmu_pmu->on_cpu != -1) {
		if (!cpumask_test_cpu(dmu_pmu->on_cpu, cpu_online_mask)) {
			perf_pmu_migrate_context(&dmu_pmu->pmu, dmu_pmu->on_cpu, cpu);
			dmu_pmu->on_cpu = cpu;
			WARN_ON(irq_set_affinity_hint(dmu_pmu->irq, cpumask_of(cpu)));
		}
		return 0;
	}

	dmu_pmu->on_cpu = cpu;
	WARN_ON(irq_set_affinity_hint(dmu_pmu->irq, cpumask_of(cpu)));

	return 0;
}

int pd2408_dmu_pmu_offline_cpu(unsigned int cpu, struct hlist_node *node)
{
	struct pd2408_dmu_pmu *dmu_pmu =
		hlist_entry_safe(node, struct pd2408_dmu_pmu, node);
	unsigned int target;
	cpumask_t available_cpus;

	if (dmu_pmu->on_cpu != cpu)
		return 0;

	cpumask_andnot(&available_cpus, cpu_online_mask, cpumask_of(cpu));
	target = cpumask_last(&available_cpus);

	if (target >= nr_cpu_ids) {
		dev_err(dmu_pmu->dev, "offline cpu%d with no target to migrate.\n",
			cpu);
		return 0;
	}

	perf_pmu_migrate_context(&dmu_pmu->pmu, cpu, target);
	WARN_ON(irq_set_affinity_hint(dmu_pmu->irq, cpumask_of(target)));
	dmu_pmu->on_cpu = target;

	return 0;
}

static int __init pd2408_dmu_pmu_module_init(void)
{
	int ret;

	pd2408_dmu_pmu_hp_state = cpuhp_setup_state_multi(CPUHP_AP_ONLINE_DYN,
				      "perf/phytium/dmupmu:online",
				      pd2408_dmu_pmu_online_cpu, pd2408_dmu_pmu_offline_cpu);
	if (pd2408_dmu_pmu_hp_state < 0) {
		pr_err("DMU PMU: setup hotplug, pd2408_dmu_pmu_hp_state = %d\n",
			pd2408_dmu_pmu_hp_state);
		return pd2408_dmu_pmu_hp_state;
	}

	ret = platform_driver_register(&pd2408_dmu_pmu_driver);
	if (ret)
		cpuhp_remove_multi_state(
			pd2408_dmu_pmu_hp_state);

	return ret;
}
module_init(pd2408_dmu_pmu_module_init);

static void __exit pd2408_dmu_pmu_module_exit(void)
{
	platform_driver_unregister(&pd2408_dmu_pmu_driver);
	cpuhp_remove_multi_state(pd2408_dmu_pmu_hp_state);
}
module_exit(pd2408_dmu_pmu_module_exit);

MODULE_DESCRIPTION("Phytium DMU PMU driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DMU_PERF_DRIVER_VERSION);
MODULE_AUTHOR("Hu Xianghua <huxianghua@phytium.com.cn>");
MODULE_AUTHOR("Tan Rui <tanrui2142@phytium.com.cn>");

