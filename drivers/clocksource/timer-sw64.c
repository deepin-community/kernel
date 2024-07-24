// SPDX-License-Identifier: GPL-2.0

#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/kconfig.h>
#include <linux/percpu-defs.h>
#include <linux/sched_clock.h>
#include <linux/types.h>

#include <asm/csr.h>
#include <asm/cpu.h>
#include <asm/debug.h>
#include <asm/hmcall.h>
#include <asm/sw64_init.h>

#define DEFAULT_MCLK    25000      /* Khz */

#if defined(CONFIG_SUBARCH_C4)
static u64 read_longtime(struct clocksource *cs)
{
	return sw64_read_csr(CSR_SHTCLOCK);
}

static struct clocksource clocksource_longtime = {
	.name	= "longtime",
	.rating	= 100,
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
	.mask	= CLOCKSOURCE_MASK(64),
	.shift	= 0,
	.mult	= 0,
	.read	= read_longtime,
};

static u64 notrace read_sched_clock(void)
{
	return sw64_read_csr(CSR_SHTCLOCK);
}

static int override_mclk_khz;
static int __init setup_mclk(char *str)
{
	get_option(&str, &override_mclk_khz);

	return 0;
}
early_param("mclk_khz", setup_mclk);

void __init sw64_setup_clocksource(void)
{
	unsigned long mclk_khz;

	if (sunway_mclk_hz)
		mclk_khz = sunway_mclk_hz / 1000;
	else
		mclk_khz = *((unsigned char *)__va(MB_MCLK)) * 1000;

	if (override_mclk_khz) {
		mclk_khz = override_mclk_khz;
		pr_info("Override mclk by cmdline.\n");
	}

	if (!mclk_khz)
		mclk_khz = DEFAULT_MCLK;

	pr_info("mclk: %ldKhz\n", mclk_khz);

	clocksource_register_khz(&clocksource_longtime, mclk_khz);
	sched_clock_register(read_sched_clock, BITS_PER_LONG, mclk_khz * 1000);
}

void __init setup_sched_clock(void) { }
#elif defined(CONFIG_SUBARCH_C3B)
#ifdef CONFIG_SMP
#define OFFSET_LONG_TIME_START_EN	0x9000UL

#define OFFSET_LONG_TIME		0x180UL

#define OFFSET_GPIO_SWPORTA_DR		0x0UL
#define OFFSET_GPIO_SWPORTA_DDR		0x200UL

static u64 read_longtime(struct clocksource *cs)
{
	unsigned long node;
	void __iomem *intpu_base;

	node = __this_cpu_read(hard_node_id);
	intpu_base = misc_platform_get_intpu_base(node);

	return readq(intpu_base + OFFSET_LONG_TIME);
}

static int longtime_enable(struct clocksource *cs)
{
	void __iomem *spbu_base = misc_platform_get_spbu_base(0);
	void __iomem *gpio_base = misc_platform_get_gpio_base(0);

	switch (current_cpu_data.model) {
	case CPU_SW3231:
		writeq(0, gpio_base + OFFSET_GPIO_SWPORTA_DR);
		writeq(0xff, gpio_base + OFFSET_GPIO_SWPORTA_DDR);
		break;
	case CPU_SW831:
		writeq(0x1, spbu_base + OFFSET_LONG_TIME_START_EN);
		break;
	default:
		break;
	}

	return 0;
}

static struct clocksource clocksource_longtime = {
	.name	= "longtime",
	.rating	= 100,
	.enable	= longtime_enable,
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
	.mask	= CLOCKSOURCE_MASK(64),
	.shift	= 0,
	.mult	= 0,
	.read	= read_longtime,
};

static u64 read_vtime(struct clocksource *cs)
{
	unsigned long vtime_addr;

	vtime_addr = IO_BASE | INTPU_BASE | OFFSET_LONG_TIME;
	return rdio64(vtime_addr);
}

static int vtime_enable(struct clocksource *cs)
{
	return 0;
}

static struct clocksource clocksource_vtime = {
	.name	= "vtime",
	.rating	= 100,
	.enable	= vtime_enable,
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
	.mask	= CLOCKSOURCE_MASK(64),
	.shift	= 0,
	.mult	= 0,
	.read	= read_vtime,
};
#else /* !SMP */
static u64 read_tc(struct clocksource *cs)
{
	return rdtc();
}

static struct clocksource clocksource_tc = {
	.name		= "tc",
	.rating		= 300,
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
	.mask		= CLOCKSOURCE_MASK(64),
	.shift		= 22,
	.mult		= 0,  /* To be filled in */
	.read		= read_tc,
};
#endif /* SMP */

void __init sw64_setup_clocksource(void)
{
	unsigned long mclk_khz = *((unsigned char *)__va(MB_MCLK)) * 1000;

	if (!mclk_khz)
		mclk_khz = DEFAULT_MCLK;

#ifdef CONFIG_SMP
	if (is_in_host())
		clocksource_register_khz(&clocksource_longtime, mclk_khz);
	else
		clocksource_register_khz(&clocksource_vtime, mclk_khz);
#else
	clocksource_register_hz(&clocksource_tc, get_cpu_freq());
	pr_info("Setup clocksource TC, mult = %d\n", clocksource_tc.mult);
#endif
}

DECLARE_PER_CPU(u64, tc_offset);
static u64 sc_start, sc_shift, sc_multi;
DEFINE_STATIC_KEY_FALSE(use_tc_as_sched_clock);

static int __init sched_clock_setup(char *opt)
{
	if (!opt)
		return -EINVAL;

	if (!strncmp(opt, "on", 2)) {
		static_branch_enable(&use_tc_as_sched_clock);
		pr_info("Using TC instead of jiffies as source of sched_clock()\n");
	}

	return 0;
}
early_param("tc_sched_clock", sched_clock_setup);

static void __init calibrate_sched_clock(void)
{
	sc_start = rdtc();
}

void __init setup_sched_clock(void)
{
	unsigned long step;

	sc_shift = 7;
	step = 1UL << sc_shift;
	sc_multi = step * NSEC_PER_SEC / get_cpu_freq();
	calibrate_sched_clock();

	pr_info("sched_clock: sc_multi=%llu, sc_shift=%llu\n", sc_multi, sc_shift);
}

#ifdef CONFIG_GENERIC_SCHED_CLOCK
static u64 notrace read_sched_clock(void)
{
	return (rdtc() - sc_start) >> sc_shift;
}

void __init sw64_sched_clock_init(void)
{
	sched_clock_register(sched_clock_read, BITS_PER_LONG, get_cpu_freq() >> sc_shift);
}
#else /* !CONFIG_GENERIC_SCHED_CLOCK */
/*
 * scheduler clock - returns current time in nanoseconds.
 */
unsigned long long notrace sched_clock(void)
{
	if (static_branch_likely(&use_tc_as_sched_clock))
		return ((rdtc() - sc_start + __this_cpu_read(tc_offset)) >> sc_shift) * sc_multi;
	else
		return (jiffies - INITIAL_JIFFIES) * (NSEC_PER_SEC / HZ);
}

#ifdef CONFIG_DEBUG_FS
static ssize_t sched_clock_status_read(struct file *file, char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	char buf[2];

	if (static_key_enabled(&use_tc_as_sched_clock))
		buf[0] = 'Y';
	else
		buf[0] = 'N';
	buf[1] = '\n';
	return simple_read_from_buffer(user_buf, count, ppos, buf, 2);
}

static ssize_t sched_clock_status_write(struct file *file, const char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	int r;
	bool bv;
	bool val = static_key_enabled(&use_tc_as_sched_clock);

	r = kstrtobool_from_user(user_buf, count, &bv);
	if (!r) {
		if (val != bv) {
			if (bv) {
				static_branch_enable(&use_tc_as_sched_clock);
				pr_info("source of sched_clock() switched from jiffies to TC\n");
			} else {
				static_branch_disable(&use_tc_as_sched_clock);
				pr_info("source of sched_clock() switched from TC to jiffies\n");
			}
		} else {
			if (val)
				pr_info("source of sched_clock() unchanged (using TC)\n");
			else
				pr_info("source of sched_clock() unchanged (using jiffies)\n");
		}
	}

	return count;
}

static const struct file_operations sched_clock_status_fops = {
	.read		= sched_clock_status_read,
	.write		= sched_clock_status_write,
	.open		= nonseekable_open,
	.llseek		= no_llseek,
};

static int __init sched_clock_debug_init(void)
{
	struct dentry *sched_clock_status;

	if (!sw64_debugfs_dir)
		return -ENODEV;

	sched_clock_status = debugfs_create_file("tc_sched_clock",
			0644, sw64_debugfs_dir, NULL,
			&sched_clock_status_fops);

	if (!sched_clock_status)
		return -ENOMEM;

	return 0;
}
late_initcall(sched_clock_debug_init);
#endif /* CONFIG_DEBUG_FS */
#endif /* CONFIG_GENERIC_SCHED_CLOCK */

#endif



static int timer_next_event(unsigned long delta,
		struct clock_event_device *evt);
static int timer_set_shutdown(struct clock_event_device *evt);
static int timer_set_oneshot(struct clock_event_device *evt);

/*
 * The local apic timer can be used for any function which is CPU local.
 */
static struct clock_event_device timer_clockevent = {
	.name			= "timer",
	.features		= CLOCK_EVT_FEAT_ONESHOT,
	.shift			= 20,
	.mult			= 0,
	.set_state_shutdown	= timer_set_shutdown,
	.set_state_oneshot	= timer_set_oneshot,
	.set_next_event		= timer_next_event,
	.rating			= 300,
	.irq			= -1,
};

static int vtimer_next_event(unsigned long delta,
		struct clock_event_device *evt)
{
	hcall(HCALL_SET_CLOCKEVENT, delta, 0, 0);
	return 0;
}

static int vtimer_shutdown(struct clock_event_device *evt)
{
	hcall(HCALL_SET_CLOCKEVENT, 0, 0, 0);
	return 0;
}

static int vtimer_set_oneshot(struct clock_event_device *evt)
{
	return 0;
}
static struct clock_event_device vtimer_clockevent = {
	.name			= "vtimer",
	.features		= CLOCK_EVT_FEAT_ONESHOT,
	.shift			= 20,
	.mult			= 0,
	.set_state_shutdown	= vtimer_shutdown,
	.set_state_oneshot	= vtimer_set_oneshot,
	.set_next_event		= vtimer_next_event,
	.rating			= 300,
	.irq			= -1,
};

static DEFINE_PER_CPU(struct clock_event_device, timer_events);

/*
 * Program the next event, relative to now
 */
static int timer_next_event(unsigned long delta,
		struct clock_event_device *evt)
{
	wrtimer(delta);
	return 0;
}

static int timer_set_shutdown(struct clock_event_device *evt)
{
	wrtimer(0);
	return 0;
}

static int timer_set_oneshot(struct clock_event_device *evt)
{
	/*
	 * SW-TIMER support CLOCK_EVT_MODE_ONESHOT only, and automatically.
	 * unlike PIT and HPET, which support ONESHOT or PERIODIC by setting PIT_MOD or HPET_Tn_CFG
	 * so, nothing to do here ...
	 */
	return 0;
}

void sw64_update_clockevents(unsigned long cpu, u32 freq)
{
	struct clock_event_device *swevt = &per_cpu(timer_events, cpu);

	if (cpu == smp_processor_id())
		clockevents_update_freq(swevt, freq);
	else {
		clockevents_calc_mult_shift(swevt, freq, 4);
		swevt->min_delta_ns = clockevent_delta2ns(swevt->min_delta_ticks, swevt);
		swevt->max_delta_ns = clockevent_delta2ns(swevt->max_delta_ticks, swevt);
	}
}

/*
 * Setup the local timer for this CPU. Copy the initialized values
 * of the boot CPU and register the clock event in the framework.
 */
void sw64_setup_timer(void)
{
	unsigned long  min_delta;
	int cpu = smp_processor_id();
	struct clock_event_device *swevt = &per_cpu(timer_events, cpu);

	/* min_delta ticks => 100ns */
	min_delta = get_cpu_freq()/1000/1000/10;

	if (is_in_guest()) {
		memcpy(swevt, &vtimer_clockevent, sizeof(*swevt));
		/*
		 * CUIWEI: This value is very important.
		 * If it's too small, the timer will timeout when the IER
		 * haven't been opened.
		 */
		min_delta *= 4;
	} else {
		memcpy(swevt, &timer_clockevent, sizeof(*swevt));
	}
	swevt->cpumask = cpumask_of(cpu);
	swevt->set_state_shutdown(swevt);
	clockevents_config_and_register(swevt, get_cpu_freq(), min_delta, ULONG_MAX);
}

void sw64_timer_interrupt(void)
{
	struct clock_event_device *evt = this_cpu_ptr(&timer_events);

	if (!evt->event_handler) {
		pr_warn("Spurious local timer interrupt on cpu %d\n",
				smp_processor_id());
		timer_set_shutdown(evt);
		return;
	}

	inc_irq_stat(timer_irqs_event);

	evt->event_handler(evt);
}
