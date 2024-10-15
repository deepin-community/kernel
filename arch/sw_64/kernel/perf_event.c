// SPDX-License-Identifier: GPL-2.0
/*
 * Performance events support for SW64 platforms.
 *
 * This code is based upon riscv and sparc perf event code.
 */

#include <linux/perf_event.h>
#include <asm/stacktrace.h>

DEFINE_PER_CPU(struct cpu_hw_events, cpu_hw_events);

struct sw64_perf_event {
	/* pmu index */
	int counter;
	/* events selector */
	int event;
};

/*
 * A structure to hold the description of the PMCs available on a particular
 * type of SW64 CPU.
 */
struct sw64_pmu_t {
	/* generic hw/cache events table */
	const struct sw64_perf_event *hw_events;
	const struct sw64_perf_event (*cache_events)[PERF_COUNT_HW_CACHE_MAX]
		[PERF_COUNT_HW_CACHE_OP_MAX]
		[PERF_COUNT_HW_CACHE_RESULT_MAX];

	/* method used to map hw/cache events */
	const struct sw64_perf_event *(*map_hw_event)(u64 config);
	const struct sw64_perf_event *(*map_cache_event)(u64 config);

	/* The number of entries in the hw_event_map */
	int  max_events;

	/* The number of counters on this pmu */
	int  num_pmcs;

	/*
	 * All PMC counters reside in the IBOX register PCTR.  This is the
	 * LSB of the counter.
	 */
	int  pmc_count_shift[MAX_HWEVENTS];

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
#define SW64_OP_UNSUP {-1, -1}

/* Mapping of the hw event types to the perf tool interface */
static const struct sw64_perf_event core3_hw_event_map[] = {
	[PERF_COUNT_HW_CPU_CYCLES]		= {PMC_PC0, PC0_CPU_CYCLES},
	[PERF_COUNT_HW_INSTRUCTIONS]		= {PMC_PC0, PC0_INSTRUCTIONS},
	[PERF_COUNT_HW_CACHE_REFERENCES]	= {PMC_PC0, PC0_SCACHE_REFERENCES},
	[PERF_COUNT_HW_CACHE_MISSES]		= {PMC_PC1, PC1_SCACHE_MISSES},
	[PERF_COUNT_HW_BRANCH_INSTRUCTIONS]	= {PMC_PC0, PC0_BRANCH_INSTRUCTIONS},
	[PERF_COUNT_HW_BRANCH_MISSES]		= {PMC_PC1, PC1_BRANCH_MISSES},
};

/* Mapping of the hw cache event types to the perf tool interface */
#define C(x) PERF_COUNT_HW_CACHE_##x
static const struct sw64_perf_event core3_cache_event_map
				[PERF_COUNT_HW_CACHE_MAX]
				[PERF_COUNT_HW_CACHE_OP_MAX]
				[PERF_COUNT_HW_CACHE_RESULT_MAX] = {
	[C(L1D)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= {PMC_PC0, PC0_DCACHE_READ},
			[C(RESULT_MISS)]	= {PMC_PC1, PC1_DCACHE_MISSES}
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},
	[C(L1I)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= {PMC_PC0, PC0_ICACHE_READ},
			[C(RESULT_MISS)]	= {PMC_PC1, PC1_ICACHE_READ_MISSES},
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},
	[C(LL)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},
	[C(DTLB)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= {PMC_PC0, PC0_DTB_READ},
			[C(RESULT_MISS)]	= {PMC_PC1, PC1_DTB_SINGLE_MISSES},
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},
	[C(ITLB)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= {PMC_PC0, PC0_ITB_READ},
			[C(RESULT_MISS)]	= {PMC_PC1, PC1_ITB_MISSES},
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},
	[C(BPU)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},
	[C(NODE)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},

};

static const struct sw64_perf_event *core3_map_hw_event(u64 config)
{
	return &sw64_pmu->hw_events[config];
}

static const struct sw64_perf_event *core3_map_cache_event(u64 config)
{
	unsigned int cache_type, cache_op, cache_result;
	const struct sw64_perf_event *perf_event;

	cache_type = (config >> 0) & 0xff;
	if (cache_type >= PERF_COUNT_HW_CACHE_MAX)
		return ERR_PTR(-EINVAL);

	cache_op = (config >> 8) & 0xff;
	if (cache_op >= PERF_COUNT_HW_CACHE_OP_MAX)
		return ERR_PTR(-EINVAL);

	cache_result = (config >> 16) & 0xff;
	if (cache_result >= PERF_COUNT_HW_CACHE_RESULT_MAX)
		return ERR_PTR(-EINVAL);

	perf_event = &((*sw64_pmu->cache_events)[cache_type][cache_op][cache_result]);
	if (perf_event->counter == -1) /* SW64_OP_UNSUP */
		return ERR_PTR(-ENOENT);

	return perf_event;
}

/*
 * r0xx for counter0, r1yy for counter1.
 * According to the datasheet, 00 <= xx <= 0F, 00 <= yy <= 3D
 */
static bool core3_raw_event_valid(u64 config)
{
	if ((config >= PC0_RAW_BASE && config <= (PC0_RAW_BASE + PC0_MAX)) ||
		(config >= PC1_RAW_BASE && config <= (PC1_RAW_BASE + PC1_MAX)))
		return true;

	pr_info("sw64 pmu: invalid raw event config %#llx\n", config);
	return false;
}

static const struct sw64_pmu_t core3_pmu = {
	.max_events = ARRAY_SIZE(core3_hw_event_map),
	.hw_events = core3_hw_event_map,
	.map_hw_event = core3_map_hw_event,
	.cache_events = &core3_cache_event_map,
	.map_cache_event = core3_map_cache_event,
	.num_pmcs = MAX_HWEVENTS,
	.pmc_count_mask = PMC_COUNT_MASK,
	.pmc_max_period = PMC_COUNT_MASK,
	.pmc_left = 4,
	.raw_event_valid = core3_raw_event_valid,
};

/*
 * Low-level functions: reading/writing counters
 */
static void sw64_write_pmc(int idx, unsigned long val)
{
	wrperfmon(PMC_CMD_WRITE_BASE + idx, val);
}

static unsigned long sw64_read_pmc(int idx)
{
	return wrperfmon(PMC_CMD_READ, idx);
}

/* Set a new period to sample over */
static int sw64_perf_event_set_period(struct perf_event *event,
				struct hw_perf_event *hwc, int idx)
{
	long left = local64_read(&hwc->period_left);
	long period = hwc->sample_period;
	int overflow = 0;
	unsigned long value;

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

	if (left > (long)sw64_pmu->pmc_max_period)
		left = sw64_pmu->pmc_max_period;

	value = sw64_pmu->pmc_max_period - left;
	local64_set(&hwc->prev_count, value);
	sw64_write_pmc(idx, value);

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
static unsigned long sw64_perf_event_update(struct perf_event *event,
					struct hw_perf_event *hwc, int idx, long ovf)
{
	long prev_raw_count, new_raw_count;
	long delta;

again:
	prev_raw_count = local64_read(&hwc->prev_count);
	new_raw_count = sw64_read_pmc(idx);

	if (local64_cmpxchg(&hwc->prev_count, prev_raw_count,
			     new_raw_count) != prev_raw_count)
		goto again;

	delta = (new_raw_count - (prev_raw_count & sw64_pmu->pmc_count_mask)) + ovf;

	/* It is possible on very rare occasions that the PMC has overflowed
	 * but the interrupt is yet to come.  Detect and fix this situation.
	 */
	if (unlikely(delta < 0))
		delta += sw64_pmu->pmc_max_period + 1;

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
 * pmu->start: start the event.
 */
static void sw64_pmu_start(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	if (WARN_ON_ONCE(!(hwc->state & PERF_HES_STOPPED)))
		return;

	if (flags & PERF_EF_RELOAD) {
		WARN_ON_ONCE(!(hwc->state & PERF_HES_UPTODATE));
		sw64_perf_event_set_period(event, hwc, hwc->idx);
	}

	hwc->state = 0;

	/* counting in selected modes, for both counters */
	wrperfmon(PMC_CMD_PM, hwc->config_base);
	wrperfmon(PMC_CMD_EVENT_BASE + hwc->idx, hwc->config);
	wrperfmon(PMC_CMD_ENABLE, PMC_ENABLE_BASE + hwc->idx);
}

/*
 * pmu->stop: stop the counter
 */
static void sw64_pmu_stop(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	if (!(hwc->state & PERF_HES_STOPPED)) {
		wrperfmon(PMC_CMD_DISABLE, PMC_DISABLE_BASE + hwc->idx);
		hwc->state |= PERF_HES_STOPPED;
		barrier();
	}

	if ((flags & PERF_EF_UPDATE) && !(hwc->state & PERF_HES_UPTODATE)) {
		sw64_perf_event_update(event, hwc, hwc->idx, 0);
		hwc->state |= PERF_HES_UPTODATE;
	}
}

/*
 * pmu->add: add the event to PMU.
 */
static int sw64_pmu_add(struct perf_event *event, int flags)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);
	struct hw_perf_event *hwc = &event->hw;
	int err = 0;
	unsigned long irq_flags;

	local_irq_save(irq_flags);

	if (__test_and_set_bit(hwc->idx, cpuc->used_mask)) {
		err = -ENOSPC;
		goto out;
	}

	cpuc->event[hwc->idx] = event;

	hwc->state = PERF_HES_STOPPED | PERF_HES_UPTODATE;
	if (flags & PERF_EF_START)
		sw64_pmu_start(event, PERF_EF_RELOAD);

	/* Propagate our changes to the userspace mapping. */
	perf_event_update_userpage(event);

out:
	local_irq_restore(irq_flags);

	return err;
}

/*
 * pmu->del: delete the event from PMU.
 */
static void sw64_pmu_del(struct perf_event *event, int flags)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);
	struct hw_perf_event *hwc = &event->hw;
	unsigned long irq_flags;

	local_irq_save(irq_flags);

	sw64_pmu_stop(event, PERF_EF_UPDATE);
	cpuc->event[hwc->idx] = NULL;
	__clear_bit(event->hw.idx, cpuc->used_mask);

	/* Absorb the final count and turn off the event. */
	perf_event_update_userpage(event);

	local_irq_restore(irq_flags);
}

/*
 * pmu->read: read and update the counter
 */
static void sw64_pmu_read(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;

	sw64_perf_event_update(event, hwc, hwc->idx, 0);
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
	struct perf_event_attr *attr = &event->attr;
	struct hw_perf_event *hwc = &event->hw;

	hwc->config_base = SW64_PERFCTRL_AM;

	if (attr->exclude_user)
		hwc->config_base = SW64_PERFCTRL_KM;
	if (attr->exclude_kernel)
		hwc->config_base = SW64_PERFCTRL_UM;

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
	const struct sw64_perf_event *event_type;

	if (!sw64_pmu)
		return -ENODEV;

	/* does not support taken branch sampling */
	if (has_branch_stack(event))
		return -EOPNOTSUPP;

	if (attr->exclude_user && attr->exclude_kernel)
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
		event_type = sw64_pmu->map_hw_event(attr->config);
		hwc->idx = event_type->counter;
		hwc->config = event_type->event;
		break;
	case PERF_TYPE_HW_CACHE:
		event_type = sw64_pmu->map_cache_event(attr->config);
		if (IS_ERR(event_type))
			return PTR_ERR(event_type);
		hwc->idx = event_type->counter;
		hwc->config = event_type->event;
		break;
	case PERF_TYPE_RAW:
		if (!sw64_pmu->raw_event_valid(attr->config))
			return -EINVAL;
		hwc->idx = attr->config >> 8;	/* counter selector */
		hwc->config = attr->config & 0xff;	/* event selector */
		break;
	default:
		return -ENOENT;
	}

	/*
	 * SW64 does not have per-counter usr/os/guest/host bits,
	 * we can distinguish exclude_user and exclude_kernel by
	 * sample mode.
	 */
	if (attr->exclude_hv || attr->exclude_idle ||
			attr->exclude_host || attr->exclude_guest)
		return -EINVAL;

	/* Do the real initialisation work. */
	__hw_perf_event_init(event);

	return 0;
}

static struct pmu pmu = {
	.name		= "core3-base",
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
	unsigned long pcr0, pcr1;
	int cpu;

	if (!supported_cpu())
		return;

	local_irq_save(flags);

	cpu = smp_processor_id();

	pcr0 = wrperfmon(PMC_CMD_READ, PMC_PC0);
	pcr1 = wrperfmon(PMC_CMD_READ, PMC_PC1);

	pr_info("CPU#%d: PCTR0[%lx] PCTR1[%lx]\n", cpu, pcr0, pcr1);

	local_irq_restore(flags);
}

static void sw64_perf_event_irq_handler(unsigned long idx,
					struct pt_regs *regs)
{
	struct cpu_hw_events *cpuc;
	struct perf_sample_data data;
	struct perf_event *event;
	struct hw_perf_event *hwc;

	__this_cpu_inc(irq_pmi_count);
	cpuc = this_cpu_ptr(&cpu_hw_events);

	event = cpuc->event[idx];

	if (unlikely(!event)) {
		irq_err_count++;
		return;
	}

	hwc = &event->hw;
	sw64_perf_event_update(event, hwc, idx, sw64_pmu->pmc_max_period + 1);
	perf_sample_data_init(&data, 0, hwc->last_period);

	if (sw64_perf_event_set_period(event, hwc, idx)) {
		if (perf_event_overflow(event, &data, regs)) {
			/* Interrupts coming too quickly; "throttle" the
			 * counter, i.e., disable it for a little while.
			 */
			sw64_pmu_stop(event, 0);
		}
	}
}

/*
 * Init call to initialise performance events at kernel startup.
 */
int __init init_hw_perf_events(void)
{
	pr_info("Performance Events: ");

	if (!supported_cpu()) {
		pr_cont("Unsupported CPU type!\n");
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
	sw64_pmu = &core3_pmu;

	perf_pmu_register(&pmu, "cpu", PERF_TYPE_RAW);

	return 0;
}
early_initcall(init_hw_perf_events);
