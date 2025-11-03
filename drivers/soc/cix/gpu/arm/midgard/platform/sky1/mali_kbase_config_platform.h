/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2014-2017, 2020-2022 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */
#ifndef _KBASE_CONFIG_PLATFORM_H_
#define _KBASE_CONFIG_PLATFORM_H_

#include <linux/devfreq.h>

/**
 * POWER_MANAGEMENT_CALLBACKS - Power management configuration
 *
 * Attached value: pointer to @ref kbase_pm_callback_conf
 * Default value: See @ref kbase_pm_callback_conf
 */
#define POWER_MANAGEMENT_CALLBACKS (&pm_callbacks)

/**
 * PLATFORM_FUNCS - Platform specific configuration functions
 *
 * Attached value: pointer to @ref kbase_platform_funcs_conf
 * Default value: See @ref kbase_platform_funcs_conf
 */
#if !IS_ENABLED(CONFIG_MALI_NO_MALI)
#define PLATFORM_FUNCS (&platform_funcs)
extern struct kbase_platform_funcs_conf platform_funcs;
#else
#define PLATFORM_FUNCS (NULL)
#endif

#define CLK_RATE_TRACE_OPS (&clk_rate_trace_ops)

extern struct kbase_pm_callback_conf pm_callbacks;
extern struct kbase_clk_rate_trace_op_conf clk_rate_trace_ops;
/**
 * AUTO_SUSPEND_DELAY - Autosuspend delay
 *
 * The delay time (in milliseconds) to be used for autosuspend
 */
#define AUTO_SUSPEND_DELAY (100)

/**
 * mapping relationship between core and stack:
 *
 * stack0: core_bit 0/4/8
 * stack2: core_bit 2/6/10
 * stack4: core_bit 16/20
 * stack6: core_bit 18/22
 */
#define MALI_TITAN_MC01_CORE_MASK 0x10000

#define MALI_TITAN_MC02_CORE_MASK 0x110000

#define MALI_TITAN_MC03_CORE_MASK 0x111

#define MALI_TITAN_MC04_CORE_MASK 0x550000

#define MALI_TITAN_MC05_CORE_MASK 0x110111

#define MALI_TITAN_MC06_CORE_MASK 0x555

#define MALI_TITAN_MC07_CORE_MASK 0x550111

#define MALI_TITAN_MC08_CORE_MASK 0x110555

#define MALI_TITAN_MC09_CORE_MASK 0x150555

#define MALI_TITAN_MC10_CORE_MASK 0x550555

/* gpu power model sampling calculation interval */
#define PM_POWER_MODEL_SAMPLE_INTERVAL_MS (5)

/**
 * GPU driver will periodically write the current dynamic power and
 * static power to this address, and PM firmware will also
 * periodically read the value at this address.
 */
#define PM_POWER_SHARE_ADDR 0x83BF0640

/**
 * PM writes the current clock value to this address after the clock
 * setting takes effect.
 */
#define PM_CLK_RESPONSE_ADDR 0x65A008C

/**
 * The maximum number of times that mali driver attempts to read
 * whether the PM clock config is effective.
 */
#define PM_CLK_MAX_RETRY_TIMES (1000)

#define GPU_CLOCK_100M (100000000)

/* GPU RCSU pgctrl register distribution */
#define GPU_RCSU_QCHANNEL_CLOCK_GATE_ENABLE        (1 << 0)

/*
 * For versions without voltage value in DTS or PM firmware,
 * specify a default value.
 */
#define GPU_DEFAULT_MICRO_VOLT (820000)

/**
 * enum gpu_rcsu_hwreg - Hardware rcsu registers that can be read or written.
 */
enum gpu_rcsu_hwreg {
	GPU_RCSU_HWREG_PGCTRL,
	GPU_RCSU_HWREG_STRAP_PIN1,
	GPU_RCSU_HWREG_MAX
};

#ifdef CONFIG_MALI_DEVFREQ
/**
 * sky1_gpu_clk_rate_change_notifier() - Notify clock \
 *                                             rate listeners.
 *
 * @nb:  Pointer to notifier_block struct.
 * @event:   Pass flags to the notification function.
 * @data:  Convert to devfreq_freqs struct pointer.
 *
 */
int sky1_gpu_clk_rate_change_notifier(struct notifier_block *nb,
				unsigned long event, void *data);

/**
 * sky1_gpu_init_perf_opp_table() -Get gpu powerlevels from pm firmware \
 *                                -add the perf opp table to kbase_device.
 *
 * @kbdev:  kbase_device pointer.
 * @dp:  device-specific profile to run devfreq.
 *
 */
int sky1_gpu_init_perf_opp_table(struct kbase_device *kbdev, struct devfreq_dev_profile *dp);

/**
 * sky1_remove_opp_table() - Remove all dynamically created OPPs
 *
 * This function removes all dynamically created OPPs from the pm firmware.
 */
void sky1_remove_opp_table(struct kbase_device *kbdev, struct devfreq_dev_profile *dp);
#endif /* CONFIG_MALI_DEVFREQ */

/* Enable counter values of memory read/write accesses related*/
void kbase_enable_ls_mem_counter(struct kbase_device *kbdev);

/**
 * sky1_rcsu_reg_read32() - Read GPU RCSU hardware register.
 *
 * @kbdev: Pointer to kbase_device.
 * @rcsu_reg: Which RCSU register to read.
 */
uint32_t sky1_rcsu_reg_read32(struct kbase_device *kbdev, enum gpu_rcsu_hwreg rcsu_reg);

/**
 * sky1_rcsu_reg_write32() - Write GPU RCSU hardware register.
 *
 * @kbdev: Pointer to kbase_device.
 * @rcsu_reg: Which RCSU register to write.
 * @value: value to write.
 */
void sky1_rcsu_reg_write32(struct kbase_device *kbdev,
	enum gpu_rcsu_hwreg rcsu_reg, uint32_t value);
#endif
