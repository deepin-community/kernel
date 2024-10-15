// SPDX-License-Identifier: GPL-2.0
/*
 * Performance events support for SW64 platforms.
 *
 * This code is based upon riscv and sparc perf event code.
 */

#include <linux/perf_event.h>
#include <asm/stacktrace.h>
#include <asm/perf_event.h>

DEFINE_PER_CPU(struct cpu_hw_events, cpu_hw_events);

/*
 * A structure to hold the description of the PMCs available on a particular
 * type of SW64 CPU.
 */
struct sw64_pmu_t {
	/* generic hw/cache events table */
	const int (*cache_events)[PERF_COUNT_HW_CACHE_MAX]
		[PERF_COUNT_HW_CACHE_OP_MAX]
		[PERF_COUNT_HW_CACHE_RESULT_MAX];

	/* method used to map hw/cache events */
	const int (*map_hw_event)(u64 config);
	const int (*map_cache_event)(u64 config);

	/* The number of entries in the hw_event_map */
	int  max_events;

	/* The number of counters on this pmu */
	int  num_pmcs;

	/*
	 * The mask that isolates the PMC bits when the LSB of the counter
	 * is shifted to bit 0.
	 */
	unsigned long pmc_count_mask;

	/* The maximum period the PMC can count. */
	unsigned long pmc_max_period;

	/*
	 * The maximum value that may be written to the counter due to
	 * hardware restrictions is pmc_max_period - pmc_left.
	 */
	long pmc_left;

	/* Subroutine for checking validity of a raw event for this PMU. */
	bool (*raw_event_valid)(u64 config);
};

/*
 * The SW64 PMU description currently in operation.  This is set during
 * the boot process to the specific CPU of the machine.
 */
static const struct sw64_pmu_t *sw64_pmu;

/*
 * SW64 PMC event types
 *
 * There is no one-to-one mapping of the possible hw event types to the
 * actual codes that are used to program the PMCs hence we introduce our
 * own hw event type identifiers.
 */
#define SW64_OP_UNSUPP         (-EOPNOTSUPP)

/* Mapping of the hw event types to the perf tool interface */
static const int core4_hw_event_map[] = {
	[PERF_COUNT_HW_CPU_CYCLES]		= SW64_PMU_CYCLE,
	[PERF_COUNT_HW_INSTRUCTIONS]		= SW64_PMU_INSTRUCTIONS,
	[PERF_COUNT_HW_CACHE_REFERENCES]	= SW64_PMU_L2_REFERENCES,
	[PERF_COUNT_HW_CACHE_MISSES]		= SW64_PMU_L2_MISSES,
	[PERF_COUNT_HW_BRANCH_INSTRUCTIONS]	= SW64_PMU_BRANCH,
	[PERF_COUNT_HW_BRANCH_MISSES]		= SW64_PMU_BRANCH_MISSES,
};

/* Mapping of the hw cache event types to the perf tool interface */
#define C(x) PERF_COUNT_HW_CACHE_##x
static const int core4_cache_event_map
				[PERF_COUNT_HW_CACHE_MAX]
				[PERF_COUNT_HW_CACHE_OP_MAX]
				[PERF_COUNT_HW_CACHE_RESULT_MAX] = {
	[C(L1D)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= SW64_L1D_CACHE,
			[C(RESULT_MISS)]	= SW64_L1D_CACHE_MISSES,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_L1D_CACHE,
			[C(RESULT_MISS)]	= SW64_L1D_CACHE_MISSES,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
	},
	[C(L1I)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= SW64_L1I_CACHE,
			[C(RESULT_MISS)]	= SW64_L1I_CACHE_MISSES,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_L1I_CACHE,
			[C(RESULT_MISS)]	= SW64_L1I_CACHE_MISSES,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
	},
	[C(LL)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
	},
	[C(DTLB)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= SW64_DTB,
			[C(RESULT_MISS)]	= SW64_DTB_MISSES,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_DTB,
			[C(RESULT_MISS)]	= SW64_DTB_MISSES,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
	},
	[C(ITLB)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
	},
	[C(BPU)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
	},
	[C(NODE)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUPP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUPP,
		},
	},

};

static const int core4_map_hw_event(u64 config)
{
	return core4_hw_event_map[config];
}

static const int core4_map_cache_event(u64 config)
{
	unsigned int cache_type, cache_op, cache_result;
	int ev;

	cache_type = (config >> 0) & 0xff;
	if (cache_type >= PERF_COUNT_HW_CACHE_MAX)
		return -EINVAL;

	cache_op = (config >> 8) & 0xff;
	if (cache_op >= PERF_COUNT_HW_CACHE_OP_MAX)
		return -EINVAL;


	cache_result = (config >> 16) & 0xff;
	if (cache_result >= PERF_COUNT_HW_CACHE_RESULT_MAX)
		return -EINVAL;

	ev = (*sw64_pmu->cache_events)[cache_type][cache_op][cache_result];

	return ev;
}

/*
 * rxyy for counterx.
 * According to the datasheet, 0 <= x < 5, 00 <= yy <= 8D
 */
static bool core4_raw_event_valid(u64 config)
{
	int idx = config >> 8;
	int event = config & 0xff;

	if (idx >= 0 && idx < MAX_HWEVENTS &&
		event >= PC_RAW_BASE && event <= (PC_RAW_BASE + PC_MAX))
		return true;

	pr_info("sw64 pmu: invalid raw event config %#llx\n", config);
	return false;
}

static const struct sw64_pmu_t core4_pmu = {
	.max_events = ARRAY_SIZE(core4_hw_event_map),
	.map_hw_event = core4_map_hw_event,
	.cache_events = &core4_cache_event_map,
	.map_cache_event = core4_map_cache_event,
	.num_pmcs = MAX_HWEVENTS,
	.pmc_count_mask = PMC_COUNT_MASK,
	.pmc_max_period = PMC_COUNT_MASK,
	.raw_event_valid = core4_raw_event_valid,
};

/* Set a new period to sample over */
static int sw64_perf_event_set_period(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	long left = local64_read(&hwc->period_left);
	long period = hwc->sample_period;
	int overflow = 0, idx = hwc->idx;
	long value;

	if (unlikely(left <= -period)) {
		left = period;
		local64_set(&hwc->period_left, left);
		hwc->last_period = period;
		overflow = 1;
	}

	if (unlikely(left <= 0)) {
		left += period;
		local64_set(&hwc->period_left, left);
		hwc->last_period = period;
		overflow = 1;
	}

	if (left > sw64_pmu->pmc_max_period)
		left = sw64_pmu->pmc_max_period;

	value = sw64_pmu->pmc_max_period - left;
	local64_set(&hwc->prev_count, value);
	switch (hwc->config) {
	case SW64_PMU_INSTRUCTIONS:
		sw64_write_csr(value, CSR_RETIC);
		break;
	case SW64_PMU_BRANCH:
		sw64_write_csr(value, CSR_BRRETC);
		break;
	case SW64_PMU_BRANCH_MISSES:
		sw64_write_csr(value, CSR_BRFAILC);
		break;
	case SW64_L1I_CACHE:
		sw64_write_csr(value, CSR_IACC);
		break;
	case SW64_L1I_CACHE_MISSES:
		sw64_write_csr(value, CSR_IMISC);
		break;
	default:
		wrperfmon(idx + PMC_CMD_WRITE_BASE, value);
	}

	perf_event_update_userpage(event);

	return overflow;
}

/*
 * Calculates the count (the 'delta') since the last time the PMC was read.
 *
 * As the PMCs' full period can easily be exceeded within the perf system
 * sampling period we cannot use any high order bits as a guard bit in the
 * PMCs to detect overflow as is done by other architectures.  The code here
 * calculates the delta on the basis that there is no overflow when ovf is
 * zero.  The value passed via ovf by the interrupt handler corrects for
 * overflow.
 *
 * This can be racey on rare occasions -- a call to this routine can occur
 * with an overflowed counter just before the PMI service routine is called.
 * The check for delta negative hopefully always rectifies this situation.
 */
static unsigned long sw64_perf_event_update(struct perf_event *event)
{
	long prev_raw_count, new_raw_count;
	long delta;
	struct hw_perf_event *hwc = &event->hw;
	int idx = event->hw.idx;

again:
	prev_raw_count = local64_read(&hwc->prev_count);
	switch (hwc->config) {
	case SW64_PMU_INSTRUCTIONS:
		new_raw_count = sw64_read_csr(CSR_RETIC);
		break;
	case SW64_PMU_BRANCH:
		new_raw_count = sw64_read_csr(CSR_BRRETC);
		break;
	case SW64_PMU_BRANCH_MISSES:
		new_raw_count = sw64_read_csr(CSR_BRFAILC);
		break;
	case SW64_L1I_CACHE:
		new_raw_count = sw64_read_csr(CSR_IACC);
		break;
	case SW64_L1I_CACHE_MISSES:
		new_raw_count = sw64_read_csr(CSR_IMISC);
		break;
	default:
		new_raw_count = wrperfmon(idx + MAX_HWEVENTS, 0);
	}

	if (local64_cmpxchg(&hwc->prev_count, prev_raw_count,
			     new_raw_count) != prev_raw_count)
		goto again;

	delta = new_raw_count - prev_raw_count;

	local64_add(delta, &event->count);
	local64_sub(delta, &hwc->period_left);

	return new_raw_count;
}

/*
 * State transition functions:
 *
 * add()/del() & start()/stop()
 *
 */

/*
 * pmu->stop: stop the counter
 */
static void sw64_pmu_stop(struct perf_event *event, int flags)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;
	unsigned long value;

	if (!(hwc->state & PERF_HES_STOPPED)) {
		value = sw64_read_csr(CSR_IDR_PCCTL);
		switch (hwc->config) {
		case SW64_L1I_CACHE:
			sw64_write_csr(value & ~IACC_EN, CSR_IDR_PCCTL);
			break;
		case SW64_L1I_CACHE_MISSES:
			sw64_write_csr(value & ~IMISC_EN, CSR_IDR_PCCTL);
			break;
		case SW64_PMU_INSTRUCTIONS:
			sw64_write_csr(value & ~RETIC_EN, CSR_IDR_PCCTL);
			break;
		case SW64_PMU_BRANCH:
			sw64_write_csr(value & ~BRRETC_EN, CSR_IDR_PCCTL);
			break;
		case SW64_PMU_BRANCH_MISSES:
			sw64_write_csr(value & ~BRFAILC_EN, CSR_IDR_PCCTL);
			break;
		default:
			wrperfmon(PMC_CMD_DISABLE, idx);
		}
		cpuc->event[idx] = NULL;
		event->hw.state |= PERF_HES_STOPPED;
		barrier();
	}

	if ((flags & PERF_EF_UPDATE) && !(hwc->state & PERF_HES_UPTODATE)) {
		sw64_perf_event_update(event);
		hwc->state |= PERF_HES_UPTODATE;
	}
}

/*
 * pmu->start: start the event.
 */
static void sw64_pmu_start(struct perf_event *event, int flags)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;
	unsigned long value;

	if (WARN_ON_ONCE(!(hwc->state & PERF_HES_STOPPED)))
		return;

	if (flags & PERF_EF_RELOAD) {
		WARN_ON_ONCE(!(hwc->state & PERF_HES_UPTODATE));
		sw64_perf_event_set_period(event);
	}

	cpuc->event[idx] = event;
	event->hw.state = 0;
	value = sw64_read_csr(CSR_IDR_PCCTL);

	/* counting in selected modes, for both counters */
	switch (hwc->config) {
	case SW64_L1I_CACHE:
		sw64_write_csr(value | IACC_EN, CSR_IDR_PCCTL);
		break;
	case SW64_L1I_CACHE_MISSES:
		sw64_write_csr(value | IMISC_EN, CSR_IDR_PCCTL);
		break;
	case SW64_PMU_INSTRUCTIONS:
		sw64_write_csr(value | RETIC_EN, CSR_IDR_PCCTL);
		break;
	case SW64_PMU_BRANCH:
		sw64_write_csr(value | BRRETC_EN, CSR_IDR_PCCTL);
		break;
	case SW64_PMU_BRANCH_MISSES:
		sw64_write_csr(value | BRFAILC_EN, CSR_IDR_PCCTL);
		break;
	default:
		wrperfmon(idx, hwc->config << PC_ALL_PM_SET | hwc->config_base);
		wrperfmon(PMC_CMD_ENABLE, idx);
	}
	perf_event_update_userpage(event);
}

/*
 * pmu->add: add the event to PMU.
 */
static int sw64_pmu_add(struct perf_event *event, int flags)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;
	int err = -EAGAIN;

	if (__test_and_set_bit(idx, cpuc->used_mask)) {
		idx = find_first_zero_bit(cpuc->used_mask, sw64_pmu->num_pmcs);
		if (idx == sw64_pmu->num_pmcs)
			goto out;

		__set_bit(idx, cpuc->used_mask);
		hwc->idx = idx;
	}

	event->hw.state = PERF_HES_UPTODATE | PERF_HES_STOPPED;
	if (flags & PERF_EF_START)
		sw64_pmu_start(event, PERF_EF_RELOAD);

	/* Propagate our changes to the userspace mapping. */
	perf_event_update_userpage(event);
	err = 0;
out:
	return err;
}

/*
 * pmu->del: delete the event from PMU.
 */
static void sw64_pmu_del(struct perf_event *event, int flags)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);
	struct hw_perf_event *hwc = &event->hw;

	if (cpuc->event[hwc->idx] != event)
		return;

	sw64_pmu_stop(event, PERF_EF_UPDATE);
	__clear_bit(event->hw.idx, cpuc->used_mask);

	/* Absorb the final count and turn off the event. */
	perf_event_update_userpage(event);
}

/*
 * pmu->read: read and update the counter
 */
static void sw64_pmu_read(struct perf_event *event)
{
	sw64_perf_event_update(event);
}

static bool supported_cpu(void)
{
	return true;
}

static void hw_perf_event_destroy(struct perf_event *event)
{
	/* Nothing to be done! */
}

static void __hw_perf_event_init(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->config_base = SW64_PERFCTRL_AM;

	if (!is_sampling_event(event))
		pr_debug("not sampling event\n");

	event->destroy = hw_perf_event_destroy;

	if (!hwc->sample_period) {
		hwc->sample_period = sw64_pmu->pmc_max_period;
		hwc->last_period = hwc->sample_period;
		local64_set(&hwc->period_left, hwc->sample_period);
	}
}

/*
 * Main entry point to initialise a HW performance event.
 */
static int sw64_pmu_event_init(struct perf_event *event)
{
	struct perf_event_attr *attr = &event->attr;
	struct hw_perf_event *hwc = &event->hw;
	int config = -1;

	if (!sw64_pmu)
		return -ENODEV;

	/* does not support taken branch sampling */
	if (has_branch_stack(event))
		return -EOPNOTSUPP;

	/*
	 * SW64 does not support precise ip feature, and system hang when
	 * detecting precise_ip by perf_event_attr__set_max_precise_ip
	 * in userspace
	 */
	if (attr->precise_ip != 0)
		return -EOPNOTSUPP;

	/* SW64 has fixed counter for given event type */
	switch (attr->type) {
	case PERF_TYPE_HARDWARE:
		if (attr->config >= sw64_pmu->max_events)
			return -EINVAL;
		config = sw64_pmu->map_hw_event(attr->config);
		break;
	case PERF_TYPE_HW_CACHE:
		config = sw64_pmu->map_cache_event(attr->config);
		break;
	case PERF_TYPE_RAW:
		if (!sw64_pmu->raw_event_valid(attr->config))
			return -EINVAL;
		hwc->idx = attr->config >> 8;   /* counter selector */
		config = attr->config & 0xff;   /* event selector */
		break;
	default:
		return -ENOENT;
	}

	if (config < 0)
		return config;

	/*
	 * SW64 does not have per-counter usr/os/guest/host bits
	 */
	if (attr->exclude_hv || attr->exclude_idle ||
			attr->exclude_host || attr->exclude_guest)
		return -EINVAL;

	hwc->config = config;
	/* Do the real initialisation work. */
	__hw_perf_event_init(event);

	return 0;
}

static struct pmu pmu = {
	.name		= "core4-base",
	.capabilities   = PERF_PMU_CAP_NO_NMI,
	.event_init	= sw64_pmu_event_init,
	.add		= sw64_pmu_add,
	.del		= sw64_pmu_del,
	.start		= sw64_pmu_start,
	.stop		= sw64_pmu_stop,
	.read		= sw64_pmu_read,
};

void perf_event_print_debug(void)
{
	unsigned long flags;
	unsigned long pcr0, pcr1, pcr2, pcr3, pcr4;
	int cpu;

	if (!supported_cpu())
		return;

	local_irq_save(flags);

	cpu = smp_processor_id();

	pcr0 = wrperfmon(PMC_CMD_READ_PC0, 0);
	pcr1 = wrperfmon(PMC_CMD_READ_PC1, 0);
	pcr2 = wrperfmon(PMC_CMD_READ_PC2, 0);
	pcr3 = wrperfmon(PMC_CMD_READ_PC3, 0);
	pcr4 = wrperfmon(PMC_CMD_READ_PC4, 0);

	pr_info("CPU#%d: PCTR0[%lx] PCTR1[%lx]\n", cpu, pcr0, pcr1);
	pr_info("PCTR0[%lx] PCTR0[%lx] PCTR1[%lx]\n", pcr2, pcr3, pcr4);

	local_irq_restore(flags);
}

static void sw64_perf_event_irq_handler(unsigned long perfmon_num,
					struct pt_regs *regs)
{
	struct perf_sample_data data;
	struct cpu_hw_events *cpuc;
	int idx;
	u64 val;

	__this_cpu_inc(irq_pmi_count);
	cpuc = this_cpu_ptr(&cpu_hw_events);

	for (idx = 0; idx < sw64_pmu->num_pmcs; ++idx) {
		struct perf_event *event = cpuc->event[idx];

		/* Ignore if we don't have an event. */
		if (!event)
			continue;

		val = sw64_perf_event_update(event);
		/*
		 * We have a single interrupt for all counters. Check that
		 * each counter has overflowed before we process it.
		 */
		if (val & (1ULL << (64 - 1)))
			continue;

		/*
		 * event overflow
		 */
		perf_sample_data_init(&data, 0, event->hw.last_period);

		if (!sw64_perf_event_set_period(event))
			continue;

		if (perf_event_overflow(event, &data, regs))
			sw64_pmu_stop(event, 0);
	}
}

/*
 * Init call to initialise performance events at kernel startup.
 */
int __init init_hw_perf_events(void)
{
	pr_info("Performance Events: ");
	if (!supported_cpu()) {
		pr_info("Performance events: Unsupported CPU type!\n");
		return 0;
	}

	if (is_in_guest()) {
		pr_cont("No PMU driver, software events only.\n");
		return 0;
	}

	pr_cont("Supported CPU type!\n");

	/* Override performance counter IRQ vector */

	perf_irq = sw64_perf_event_irq_handler;

	/* And set up PMU specification */
	sw64_pmu = &core4_pmu;

	perf_pmu_register(&pmu, "cpu", PERF_TYPE_RAW);

	return 0;
}
early_initcall(init_hw_perf_events);
