// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 *
 * (C) COPYRIGHT 2015-2024 ARM Limited. All rights reserved.
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

#include <mali_kbase.h>
#include <mali_kbase_io.h>
#include <mali_kbase_defs.h>
#include <device/mali_kbase_device.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>

#include "mali_kbase_config_platform.h"

static bool need_reset_flag = true;

static void enable_gpu_power_control(struct kbase_device *kbdev)
{
	unsigned int i;

#if defined(CONFIG_REGULATOR)
	for (i = 0; i < kbdev->nr_regulators; i++) {
		if (WARN_ON(kbdev->regulators[i] == NULL))
			;
		else if (!regulator_is_enabled(kbdev->regulators[i]))
			WARN_ON(regulator_enable(kbdev->regulators[i]));
	}
#endif
	for (i = 0; i < kbdev->bk_clocks; i++) {
		if (WARN_ON(kbdev->backup_clocks[i] == NULL))
			;
		else if (!__clk_is_enabled(kbdev->backup_clocks[i]))
			WARN_ON(clk_prepare_enable(kbdev->backup_clocks[i]));
	}

	for (i = 0; i < kbdev->nr_clocks; i++) {
		if (WARN_ON(kbdev->clocks[i] == NULL))
			;
		else if (!__clk_is_enabled(kbdev->clocks[i]))
			WARN_ON(clk_prepare_enable(kbdev->clocks[i]));
	}
}

static void disable_gpu_power_control(struct kbase_device *kbdev)
{
	unsigned int i;

	for (i = 0; i < kbdev->nr_clocks; i++) {
		if (WARN_ON(kbdev->clocks[i] == NULL))
			;
		else if (__clk_is_enabled(kbdev->clocks[i])) {
			clk_disable_unprepare(kbdev->clocks[i]);
			WARN_ON(__clk_is_enabled(kbdev->clocks[i]));
		}
	}

	for (i = 0; i < kbdev->bk_clocks; i++) {
		if (WARN_ON(kbdev->backup_clocks[i] == NULL))
			;
		else if (__clk_is_enabled(kbdev->backup_clocks[i])) {
			clk_disable_unprepare(kbdev->backup_clocks[i]);
			WARN_ON(__clk_is_enabled(kbdev->backup_clocks[i]));
		}
	}
#if defined(CONFIG_REGULATOR)
	for (i = 0; i < kbdev->nr_regulators; i++) {
		if (WARN_ON(kbdev->regulators[i] == NULL))
			;
		else if (regulator_is_enabled(kbdev->regulators[i]))
			WARN_ON(regulator_disable(kbdev->regulators[i]));
	}
#endif

}

static void execute_gpu_reset(struct kbase_device *kbdev)
{
	/* reset */
	reset_control_assert(kbdev->gpu_reset);
	usleep_range(10, 20);
	/* release reset */
	reset_control_deassert(kbdev->gpu_reset);
	dev_dbg(kbdev->dev, "sky1_gpu reset DONE\n");
}

static int gpu_qchannel_clock_gating_switch(struct kbase_device *kbdev, bool enable)
{
	uint32_t reg;

	reg = sky1_rcsu_reg_read32(kbdev, GPU_RCSU_HWREG_PGCTRL);
	reg = enable ? (reg | GPU_RCSU_QCHANNEL_CLOCK_GATE_ENABLE) :
					(reg & ~(GPU_RCSU_QCHANNEL_CLOCK_GATE_ENABLE));
	sky1_rcsu_reg_write32(kbdev, GPU_RCSU_HWREG_PGCTRL, reg);

	dev_dbg(kbdev->dev, "gpu qchannel clock gating enable = %d", enable);

	return 0;
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	int ret = 1; /* Assume GPU has been powered off */
	int error;
	unsigned long flags;

	dev_dbg(kbdev->dev, "%s %pK\n", __func__, (void *)kbdev->dev->pm_domain);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	WARN_ON(kbase_io_is_gpu_powered(kbdev));
#if MALI_USE_CSF
	if (likely(kbdev->csf.firmware_inited)) {
		WARN_ON(!kbdev->pm.active_count);
		WARN_ON(kbdev->pm.runtime_active);
	}
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	enable_gpu_power_control(kbdev);

	/* Execute reset after the GPU is first powered on.
	 * sky1 GPU power probe sequence:
	 * Power domain on->clock enable->IP Reset assert->IP Reset de-assert
	 */
	if (need_reset_flag) {
		execute_gpu_reset(kbdev);
		gpu_qchannel_clock_gating_switch(kbdev, true);
		need_reset_flag = false;
	}
	CSTD_UNUSED(error);
#else
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

#ifdef KBASE_PM_RUNTIME
	error = pm_runtime_get_sync(kbdev->dev);
	if (error == 1) {
		/*
		 * Let core know that the chip has not been
		 * powered off, so we can save on re-initialization.
		 */
		ret = 0;
	}
	dev_dbg(kbdev->dev, "pm_runtime_get_sync returned %d\n", error);
#else
	enable_gpu_power_control(kbdev);
#endif /* KBASE_PM_RUNTIME */

#endif /* MALI_USE_CSF */

	return ret;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	unsigned long flags;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	WARN_ON(kbase_io_is_gpu_powered(kbdev));
#if MALI_USE_CSF
	if (likely(kbdev->csf.firmware_inited)) {
#ifdef CONFIG_MALI_DEBUG
		WARN_ON(kbase_csf_scheduler_get_nr_active_csgs(kbdev));
#endif
		WARN_ON(kbdev->pm.backend.mcu_state != KBASE_MCU_OFF);
	}
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	/* Power down the GPU immediately */
	disable_gpu_power_control(kbdev);
#else /* MALI_USE_CSF */
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

#ifdef KBASE_PM_RUNTIME
	pm_runtime_mark_last_busy(kbdev->dev);
	pm_runtime_put_autosuspend(kbdev->dev);
#else
	/* Power down the GPU immediately as runtime PM is disabled */
	disable_gpu_power_control(kbdev);
#endif
#endif /* MALI_USE_CSF */
}

#if MALI_USE_CSF && defined(KBASE_PM_RUNTIME)
static void pm_callback_runtime_gpu_active(struct kbase_device *kbdev)
{
	unsigned long flags;
	int error;

	lockdep_assert_held(&kbdev->pm.lock);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	WARN_ON(!kbdev->pm.active_count);
	WARN_ON(kbdev->pm.runtime_active);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	if (pm_runtime_status_suspended(kbdev->dev)) {
		error = pm_runtime_get_sync(kbdev->dev);
		dev_dbg(kbdev->dev, "pm_runtime_get_sync returned %d", error);
	} else {
		/* Call the async version here, otherwise there could be
		 * a deadlock if the runtime suspend operation is ongoing.
		 * Caller would have taken the kbdev->pm.lock and/or the
		 * scheduler lock, and the runtime suspend callback function
		 * will also try to acquire the same lock(s).
		 */
		error = pm_runtime_get(kbdev->dev);
		dev_dbg(kbdev->dev, "pm_runtime_get returned %d", error);
	}

	kbdev->pm.runtime_active = true;
}

static void pm_callback_runtime_gpu_idle(struct kbase_device *kbdev)
{
	unsigned long flags;

	lockdep_assert_held(&kbdev->pm.lock);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	WARN_ON(!kbase_io_is_gpu_powered(kbdev));
	WARN_ON(kbdev->pm.backend.l2_state != KBASE_L2_OFF);
	WARN_ON(kbdev->pm.active_count);
	WARN_ON(!kbdev->pm.runtime_active);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	pm_runtime_mark_last_busy(kbdev->dev);
	pm_runtime_put_autosuspend(kbdev->dev);
	kbdev->pm.runtime_active = false;
}
#endif

#ifdef KBASE_PM_RUNTIME
static int kbase_device_runtime_init(struct kbase_device *kbdev)
{
	int ret = 0;

	pm_runtime_set_autosuspend_delay(kbdev->dev, AUTO_SUSPEND_DELAY);
	pm_runtime_use_autosuspend(kbdev->dev);

	pm_runtime_set_active(kbdev->dev);
	pm_runtime_enable(kbdev->dev);

	if (!pm_runtime_enabled(kbdev->dev)) {
		dev_warn(kbdev->dev, "pm_runtime not enabled");
		ret = -EINVAL;
	} else if (atomic_read(&kbdev->dev->power.usage_count)) {
		dev_warn(kbdev->dev, "Warning: %s: Device runtime usage count unexpectedly non zero %d",
			 __func__, atomic_read(&kbdev->dev->power.usage_count));
	}

	return ret;
}

static void kbase_device_runtime_disable(struct kbase_device *kbdev)
{
	if (atomic_read(&kbdev->dev->power.usage_count))
		dev_warn(kbdev->dev, "%s: Device runtime usage count unexpectedly non zero %d",
			 __func__, atomic_read(&kbdev->dev->power.usage_count));

	pm_runtime_disable(kbdev->dev);
}
#endif /* KBASE_PM_RUNTIME */

static int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	/* RESETn resets the entire GPU. RESETn is controlled by GPU reset
	 * register in reset controller and GPU TOP power gating reset.
	 * GPU TOP power gating reset will be triggered when entering the
	 * runtime power on/off state. It is safer to re-enable the dynamic
	 * clock gating after the power gating reset.
	 */
	gpu_qchannel_clock_gating_switch(kbdev, true);
#if !MALI_USE_CSF
	enable_gpu_power_control(kbdev);
#endif
	return 0;
}

static void pm_callback_runtime_off(struct kbase_device *kbdev)
{
	gpu_qchannel_clock_gating_switch(kbdev, false);
#if !MALI_USE_CSF
	disable_gpu_power_control(kbdev);
#endif
}

static void pm_callback_resume(struct kbase_device *kbdev)
{
	enable_gpu_power_control(kbdev);
	/* gpu_reset signal will be reset after enter str
	 * needs to be set to high level again.
	 */
	execute_gpu_reset(kbdev);
	gpu_qchannel_clock_gating_switch(kbdev, true);
}

static void pm_callback_suspend(struct kbase_device *kbdev)
{
	gpu_qchannel_clock_gating_switch(kbdev, false);
	disable_gpu_power_control(kbdev);
}

static int pm_callback_soft_reset(struct kbase_device *kbdev)
{

	/* PWR_OVERRIDE1 register can program pdc_power_args with the system specific value.
	 * The value passed to PDC_Adapter module in pdc_power_args whenever a power
	 * transition request is made.
	 *
	 * TODO: Setting this latency value to the maximum is the safest way to power on.
	 * But we still need to tuning and find the most appropriate latency value later.
	 */
	dev_dbg(kbdev->dev, "%s set PWR_OVERRIDE1 register with 0xFFFFFF\n", __func__);
	kbase_reg_write32(kbdev, GPU_CONTROL_ENUM(PWR_KEY), 0x2968A819);
	kbase_reg_write32(kbdev, GPU_CONTROL_ENUM(PWR_OVERRIDE1), 0xFFFFFF);

	kbase_reg_write32(kbdev, GPU_CONTROL_ENUM(GPU_COMMAND), GPU_COMMAND_SOFT_RESET);

	return 0;
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback = pm_callback_suspend,
	.power_resume_callback = pm_callback_resume,
	.soft_reset_callback = pm_callback_soft_reset,
#ifdef KBASE_PM_RUNTIME
	.power_runtime_init_callback = kbase_device_runtime_init,
	.power_runtime_term_callback = kbase_device_runtime_disable,
	.power_runtime_on_callback = pm_callback_runtime_on,
	.power_runtime_off_callback = pm_callback_runtime_off,
#else /* KBASE_PM_RUNTIME */
	.power_runtime_init_callback = NULL,
	.power_runtime_term_callback = NULL,
	.power_runtime_on_callback = NULL,
	.power_runtime_off_callback = NULL,
#endif /* KBASE_PM_RUNTIME */

#if MALI_USE_CSF && defined(KBASE_PM_RUNTIME)
	.power_runtime_gpu_idle_callback = pm_callback_runtime_gpu_idle,
	.power_runtime_gpu_active_callback = pm_callback_runtime_gpu_active,
#else
	.power_runtime_gpu_idle_callback = NULL,
	.power_runtime_gpu_active_callback = NULL,
#endif
};
