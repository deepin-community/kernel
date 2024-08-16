// SPDX-License-Identifier: GPL-2.0-only
/*
 * Zhaoxin PMU; like Intel Architectural PerfMon-v2
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/nmi.h>

#include <asm/cpufeature.h>
#include <asm/hardirq.h>
#include <asm/apic.h>

#include "../perf_event.h"

/*
 * Zhaoxin PerfMon, used on Lujiazui and later.
 */
static u64 zx_pmon_event_map[PERF_COUNT_HW_MAX] __read_mostly = {
	[PERF_COUNT_HW_CPU_CYCLES]		= 0x0082,
	[PERF_COUNT_HW_INSTRUCTIONS]		= 0x00c0,
	[PERF_COUNT_HW_BUS_CYCLES]		= 0x0083,
	[PERF_COUNT_HW_BRANCH_INSTRUCTIONS]	= 0x0028,
	[PERF_COUNT_HW_BRANCH_MISSES]		= 0x0029,
};

static struct event_constraint zxc_event_constraints[] __read_mostly = {

	FIXED_EVENT_CONSTRAINT(0x0082, 1), /* unhalted core clock cycles */
	EVENT_CONSTRAINT_END
};

static struct event_constraint wudaokou_event_constraints[] __read_mostly = {

	FIXED_EVENT_CONSTRAINT(0x00c0, 0), /* retired instructions */
	FIXED_EVENT_CONSTRAINT(0x0082, 1), /* unhalted core clock cycles */
	FIXED_EVENT_CONSTRAINT(0x0083, 2), /* unhalted bus clock cycles */
	EVENT_CONSTRAINT_END
};

static __initconst const u64 wudaokou_hw_cache_event_ids
				[PERF_COUNT_HW_CACHE_MAX]
				[PERF_COUNT_HW_CACHE_OP_MAX]
				[PERF_COUNT_HW_CACHE_RESULT_MAX] = {
[C(L1D)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = 0x0042,
		[C(RESULT_MISS)] = 0x0538,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = 0x0043,
		[C(RESULT_MISS)] = 0x0562,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
},
[C(L1I)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = 0x0300,
		[C(RESULT_MISS)] = 0x0301,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = 0x030a,
		[C(RESULT_MISS)] = 0x030b,
	},
},
[C(LL)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
},
[C(DTLB)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = 0x0042,
		[C(RESULT_MISS)] = 0x052c,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = 0x0043,
		[C(RESULT_MISS)] = 0x0530,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = 0x0564,
		[C(RESULT_MISS)] = 0x0565,
	},
},
[C(ITLB)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = 0x00c0,
		[C(RESULT_MISS)] = 0x0534,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
},
[C(BPU)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = 0x0700,
		[C(RESULT_MISS)] = 0x0709,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
},
[C(NODE)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
},
};

static __initconst const u64 lujiazui_hw_cache_event_ids
				[PERF_COUNT_HW_CACHE_MAX]
				[PERF_COUNT_HW_CACHE_OP_MAX]
				[PERF_COUNT_HW_CACHE_RESULT_MAX] = {
[C(L1D)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = 0x0568,
		[C(RESULT_MISS)] = 0x054b,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = 0x0669,
		[C(RESULT_MISS)] = 0x0562,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
},
[C(L1I)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = 0x0300,
		[C(RESULT_MISS)] = 0x0301,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = 0x030a,
		[C(RESULT_MISS)] = 0x030b,
	},
},
[C(LL)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = 0x0,
		[C(RESULT_MISS)] = 0x0,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = 0x0,
		[C(RESULT_MISS)] = 0x0,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = 0x0,
		[C(RESULT_MISS)] = 0x0,
	},
},
[C(DTLB)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = 0x0568,
		[C(RESULT_MISS)] = 0x052c,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = 0x0669,
		[C(RESULT_MISS)] = 0x0530,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = 0x0564,
		[C(RESULT_MISS)] = 0x0565,
	},
},
[C(ITLB)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = 0x00c0,
		[C(RESULT_MISS)] = 0x0534,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
},
[C(BPU)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = 0x0028,
		[C(RESULT_MISS)] = 0x0029,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
},
[C(NODE)] = {
	[C(OP_READ)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
	[C(OP_WRITE)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
	[C(OP_PREFETCH)] = {
		[C(RESULT_ACCESS)] = -1,
		[C(RESULT_MISS)] = -1,
	},
},
};

static void zhaoxin_pmu_disable_all(void)
{
	wrmsrl(MSR_CORE_PERF_GLOBAL_CTRL, 0);
}

static void zhaoxin_pmu_enable_all(int added)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);

	wrmsrl(MSR_CORE_PERF_GLOBAL_CTRL, x86_pmu.intel_ctrl & ~cpuc->intel_ctrl_guest_mask);
}

static inline u64 zhaoxin_pmu_get_status(void)
{
	u64 status;

	rdmsrl(MSR_CORE_PERF_GLOBAL_STATUS, status);

	return status;
}

static inline void zhaoxin_pmu_ack_status(u64 ack)
{
	wrmsrl(MSR_CORE_PERF_GLOBAL_OVF_CTRL, ack);
}

static inline void zxc_pmu_ack_status(u64 ack)
{
	/*
	 * ZXC needs global control enabled in order to clear status bits.
	 */
	zhaoxin_pmu_enable_all(0);
	zhaoxin_pmu_ack_status(ack);
	zhaoxin_pmu_disable_all();
}

static inline void zhaoxin_set_masks(struct perf_event *event, int idx)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);

	if (event->attr.exclude_host)
		__set_bit(idx, (unsigned long *)&cpuc->intel_ctrl_guest_mask);
	if (event->attr.exclude_guest)
		__set_bit(idx, (unsigned long *)&cpuc->intel_ctrl_host_mask);
}

static inline void zhaoxin_clear_masks(struct perf_event *event, int idx)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);

	__clear_bit(idx, (unsigned long *)&cpuc->intel_ctrl_guest_mask);
	__clear_bit(idx, (unsigned long *)&cpuc->intel_ctrl_host_mask);
}

static void zhaoxin_pmu_disable_fixed(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	u64 ctrl_val, mask;
	int idx = hwc->idx;

	mask = 0xfULL << ((idx - INTEL_PMC_IDX_FIXED) * 4);
	rdmsrl(hwc->config_base, ctrl_val);
	ctrl_val &= ~mask;
	wrmsrl(hwc->config_base, ctrl_val);
}

static void zhaoxin_pmu_disable_event(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;

	zhaoxin_clear_masks(event, idx);

	if (unlikely(hwc->config_base == MSR_ARCH_PERFMON_FIXED_CTR_CTRL)) {
		zhaoxin_pmu_disable_fixed(event);
		return;
	}

	x86_pmu_disable_event(event);
}

static void zhaoxin_pmu_enable_fixed(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	u64 ctrl_val, mask, bits = 0;
	int idx = hwc->idx;

	/*
	 * Enable IRQ generation (0x8),
	 * and enable ring-3 counting (0x2) and ring-0 counting (0x1)
	 * if requested:
	 */
	bits = 0x8ULL;
	if (hwc->config & ARCH_PERFMON_EVENTSEL_USR)
		bits |= 0x2;
	if (hwc->config & ARCH_PERFMON_EVENTSEL_OS)
		bits |= 0x1;

	idx -= INTEL_PMC_IDX_FIXED;
	bits <<= (idx * 4);
	mask = 0xfULL << (idx * 4);

	rdmsrl(hwc->config_base, ctrl_val);
	ctrl_val &= ~mask;
	ctrl_val |= bits;
	wrmsrl(hwc->config_base, ctrl_val);
}

static void zhaoxin_pmu_enable_event(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;

	zhaoxin_set_masks(event, idx);

	if (unlikely(hwc->config_base == MSR_ARCH_PERFMON_FIXED_CTR_CTRL)) {
		zhaoxin_pmu_enable_fixed(event);
		return;
	}

	__x86_pmu_enable_event(hwc, ARCH_PERFMON_EVENTSEL_ENABLE);
}

/*
 * This handler is triggered by the local APIC, so the APIC IRQ handling
 * rules apply:
 */
static int zhaoxin_pmu_handle_irq(struct pt_regs *regs)
{
	struct perf_sample_data data;
	struct cpu_hw_events *cpuc;
	int handled = 0;
	u64 status;
	int bit;

	cpuc = this_cpu_ptr(&cpu_hw_events);
	apic_write(APIC_LVTPC, APIC_DM_NMI);
	zhaoxin_pmu_disable_all();
	status = zhaoxin_pmu_get_status();
	if (!status)
		goto done;

again:
	if (x86_pmu.enabled_ack)
		zxc_pmu_ack_status(status);
	else
		zhaoxin_pmu_ack_status(status);

	inc_irq_stat(apic_perf_irqs);

	/*
	 * CondChgd bit 63 doesn't mean any overflow status. Ignore
	 * and clear the bit.
	 */
	if (__test_and_clear_bit(63, (unsigned long *)&status)) {
		if (!status)
			goto done;
	}

	for_each_set_bit(bit, (unsigned long *)&status, X86_PMC_IDX_MAX) {
		struct perf_event *event = cpuc->events[bit];

		handled++;

		if (!test_bit(bit, cpuc->active_mask))
			continue;

		x86_perf_event_update(event);
		perf_sample_data_init(&data, 0, event->hw.last_period);

		if (!x86_perf_event_set_period(event))
			continue;

		if (perf_event_overflow(event, &data, regs))
			x86_pmu_stop(event, 0);
	}

	/*
	 * Repeat if there is more work to be done:
	 */
	status = zhaoxin_pmu_get_status();
	if (status)
		goto again;

done:
	zhaoxin_pmu_enable_all(0);
	return handled;
}

static u64 zhaoxin_pmu_event_map(int hw_event)
{
	return zx_pmon_event_map[hw_event];
}

static struct event_constraint *
zhaoxin_get_event_constraints(struct cpu_hw_events *cpuc, int idx,
			struct perf_event *event)
{
	struct event_constraint *c;

	if (x86_pmu.event_constraints) {
		for_each_event_constraint(c, x86_pmu.event_constraints) {
			if ((event->hw.config & c->cmask) == c->code)
				return c;
		}
	}

	return &unconstrained;
}

PMU_FORMAT_ATTR(event,	"config:0-7");
PMU_FORMAT_ATTR(umask,	"config:8-15");
PMU_FORMAT_ATTR(edge,	"config:18");
PMU_FORMAT_ATTR(inv,	"config:23");
PMU_FORMAT_ATTR(cmask,	"config:24-31");

static struct attribute *zx_arch_formats_attr[] = {
	&format_attr_event.attr,
	&format_attr_umask.attr,
	&format_attr_edge.attr,
	&format_attr_inv.attr,
	&format_attr_cmask.attr,
	NULL,
};

static ssize_t zhaoxin_event_sysfs_show(char *page, u64 config)
{
	u64 event = (config & ARCH_PERFMON_EVENTSEL_EVENT);

	return x86_event_sysfs_show(page, config, event);
}

static struct perf_guest_switch_msr *zhaoxin_guest_get_msrs(int *nr, void *data)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);
	struct perf_guest_switch_msr *arr = cpuc->guest_switch_msrs;

	arr[0].msr = MSR_CORE_PERF_GLOBAL_CTRL;
	arr[0].host = x86_pmu.intel_ctrl & ~cpuc->intel_ctrl_guest_mask;
	arr[0].guest = x86_pmu.intel_ctrl & ~cpuc->intel_ctrl_host_mask;
	*nr = 1;

	return arr;
}

static const struct x86_pmu zhaoxin_pmu __initconst = {
	.name			= "zhaoxin",
	.handle_irq		= zhaoxin_pmu_handle_irq,
	.disable_all		= zhaoxin_pmu_disable_all,
	.enable_all		= zhaoxin_pmu_enable_all,
	.enable			= zhaoxin_pmu_enable_event,
	.disable		= zhaoxin_pmu_disable_event,
	.hw_config		= x86_pmu_hw_config,
	.schedule_events	= x86_schedule_events,
	.eventsel		= MSR_ARCH_PERFMON_EVENTSEL0,
	.perfctr		= MSR_ARCH_PERFMON_PERFCTR0,
	.event_map		= zhaoxin_pmu_event_map,
	.max_events		= ARRAY_SIZE(zx_pmon_event_map),
	.apic			= 1,
	/*
	 * For wudaokou/lujiazui, read/write operation for PMCx MSR is 48 bits.
	 */
	.max_period		= (1ULL << 47) - 1,
	.get_event_constraints	= zhaoxin_get_event_constraints,

	.format_attrs		= zx_arch_formats_attr,
	.events_sysfs_show	= zhaoxin_event_sysfs_show,

	.guest_get_msrs		= zhaoxin_guest_get_msrs,
};

static const struct { int id; char *name; } zx_arch_events_map[] __initconst = {
	{ PERF_COUNT_HW_CPU_CYCLES, "cpu cycles" },
	{ PERF_COUNT_HW_INSTRUCTIONS, "instructions" },
	{ PERF_COUNT_HW_BUS_CYCLES, "bus cycles" },
	{ PERF_COUNT_HW_CACHE_REFERENCES, "cache references" },
	{ PERF_COUNT_HW_CACHE_MISSES, "cache misses" },
	{ PERF_COUNT_HW_BRANCH_INSTRUCTIONS, "branch instructions" },
	{ PERF_COUNT_HW_BRANCH_MISSES, "branch misses" },
};

static __init void zhaoxin_arch_events_quirk(void)
{
	int bit;

	/* disable event that reported as not present by cpuid */
	for_each_set_bit(bit, x86_pmu.events_mask, ARRAY_SIZE(zx_arch_events_map)) {
		zx_pmon_event_map[zx_arch_events_map[bit].id] = 0;
		pr_warn("CPUID marked event: \'%s\' unavailable\n",
			zx_arch_events_map[bit].name);
	}
}

__init int zhaoxin_pmu_init(void)
{
	union cpuid10_edx edx;
	union cpuid10_eax eax;
	union cpuid10_ebx ebx;
	struct event_constraint *c;
	unsigned int unused;
	int version;

	pr_info("Welcome to zhaoxin pmu!\n");

	/*
	 * Check whether the Architectural PerfMon supports
	 * hw_event or not.
	 */
	cpuid(10, &eax.full, &ebx.full, &unused, &edx.full);

	if (eax.split.mask_length < ARCH_PERFMON_EVENTS_COUNT - 1)
		return -ENODEV;

	version = eax.split.version_id;
	if (version != 2)
		return -ENODEV;

	x86_pmu = zhaoxin_pmu;
	pr_info("Version check pass!\n");

	x86_pmu.version			= version;
	x86_pmu.num_counters		= eax.split.num_counters;
	x86_pmu.cntval_bits		= eax.split.bit_width;
	x86_pmu.cntval_mask		= (1ULL << eax.split.bit_width) - 1;
	x86_pmu.events_maskl		= ebx.full;
	x86_pmu.events_mask_len		= eax.split.mask_length;

	x86_pmu.num_counters_fixed = edx.split.num_counters_fixed;
	x86_add_quirk(zhaoxin_arch_events_quirk);

	switch (boot_cpu_data.x86) {
	case 0x06:
		/*
		 * Support Zhaoxin CPU from ZXC series, exclude Nano series through FMS.
		 * Nano FMS: Family=6, Model=F, Stepping=[0-A][C-D]
		 * ZXC FMS: Family=6, Model=F, Stepping=E-F OR Family=6, Model=0x19, Stepping=0-3
		 */
		if ((boot_cpu_data.x86_model == 0x0f && boot_cpu_data.x86_stepping >= 0x0e) ||
			boot_cpu_data.x86_model == 0x19) {

			x86_pmu.max_period = x86_pmu.cntval_mask >> 1;

			/* Clearing status works only if the global control is enable on zxc. */
			x86_pmu.enabled_ack = 1;

			x86_pmu.event_constraints = zxc_event_constraints;
			zx_pmon_event_map[PERF_COUNT_HW_INSTRUCTIONS] = 0;
			zx_pmon_event_map[PERF_COUNT_HW_CACHE_REFERENCES] = 0;
			zx_pmon_event_map[PERF_COUNT_HW_CACHE_MISSES] = 0;
			zx_pmon_event_map[PERF_COUNT_HW_BUS_CYCLES] = 0;
			zx_pmon_event_map[PERF_COUNT_HW_BRANCH_INSTRUCTIONS] = 0;
			zx_pmon_event_map[PERF_COUNT_HW_BRANCH_MISSES] = 0;

			pr_cont("ZXC events, ");
			break;
		}
		return -ENODEV;

	case 0x07:
		zx_pmon_event_map[PERF_COUNT_HW_STALLED_CYCLES_FRONTEND] =
			X86_CONFIG(.event = 0x01, .umask = 0x01, .inv = 0x01, .cmask = 0x01);

		zx_pmon_event_map[PERF_COUNT_HW_STALLED_CYCLES_BACKEND] =
			X86_CONFIG(.event = 0x0f, .umask = 0x04, .inv = 0, .cmask = 0);

		switch (boot_cpu_data.x86_model) {
		case 0x1b:
			memcpy(hw_cache_event_ids, wudaokou_hw_cache_event_ids,
			       sizeof(hw_cache_event_ids));

			x86_pmu.event_constraints = wudaokou_event_constraints;

			zx_pmon_event_map[PERF_COUNT_HW_CACHE_REFERENCES] = 0x0515;
			zx_pmon_event_map[PERF_COUNT_HW_CACHE_MISSES] = 0x051a;

			zx_pmon_event_map[PERF_COUNT_HW_BRANCH_INSTRUCTIONS] = 0x0700;
			zx_pmon_event_map[PERF_COUNT_HW_BRANCH_MISSES] = 0x0709;

			pr_cont("Wudaokou events, ");
			break;
		case 0x3b:
			memcpy(hw_cache_event_ids, lujiazui_hw_cache_event_ids,
			       sizeof(hw_cache_event_ids));

			x86_pmu.event_constraints = wudaokou_event_constraints;

			zx_pmon_event_map[PERF_COUNT_HW_CACHE_REFERENCES] = 0x0515;
			zx_pmon_event_map[PERF_COUNT_HW_CACHE_MISSES] = 0x051a;

			pr_cont("Lujiazui events, ");
			break;
		case 0x5b:
		case 0x6b:
			zx_pmon_event_map[PERF_COUNT_HW_STALLED_CYCLES_FRONTEND] =
				X86_CONFIG(.event = 0x02, .umask = 0x01, .inv = 0x01,
					   .cmask = 0x01);

			memcpy(hw_cache_event_ids, lujiazui_hw_cache_event_ids,
			       sizeof(hw_cache_event_ids));

			x86_pmu.event_constraints = wudaokou_event_constraints;

			zx_pmon_event_map[PERF_COUNT_HW_CACHE_REFERENCES] = 0x051a;
			zx_pmon_event_map[PERF_COUNT_HW_CACHE_MISSES] = 0;

			if (boot_cpu_data.x86_model == 0x5b)
				pr_cont("Yongfeng events, ");

			if (boot_cpu_data.x86_model == 0x6b)
				pr_cont("Shijidadao events, ");

			break;
		default:
			return -ENODEV;
		}
		break;

	default:
		return -ENODEV;
	}

	x86_pmu.intel_ctrl = (1 << (x86_pmu.num_counters)) - 1;
	x86_pmu.intel_ctrl |= ((1LL << x86_pmu.num_counters_fixed)-1) << INTEL_PMC_IDX_FIXED;

	if (x86_pmu.event_constraints) {
		for_each_event_constraint(c, x86_pmu.event_constraints) {
			c->idxmsk64 |= (1ULL << x86_pmu.num_counters) - 1;
			c->weight += x86_pmu.num_counters;
		}
	}

	return 0;
}
