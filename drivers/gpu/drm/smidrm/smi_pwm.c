// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.

#include "smi_drv.h"
#include "ddk768/ddk768_chip.h"
#include "smi_pwm.h"
#include <linux/pci.h>
#include <drm/drm_crtc_helper.h>


/* Clock divider values: actual_divider = 1 << divider_reg_value */
static const unsigned int smi_pwm_clk_div[] = {
	1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768
};

static inline struct smi_pwm_chip *to_smi_pwm_chip(struct pwm_chip *chip)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
	return pwmchip_get_drvdata(chip);
#else
	return container_of(chip, struct smi_pwm_chip, chip);
#endif
}


static inline void __iomem *smi_pwm_get_channel_reg(struct smi_pwm_chip *smi,
						    unsigned int hwpwm)
{
	return smi->iobase + SMI_PWM_BASE + (hwpwm * SMI_PWM_CHANNEL_OFFSET);
}

/**
 * smi_pwm_set_gpio_mux - Configure GPIO mux for PWM output
 * @smi: SMI PWM chip data
 * @hwpwm: Hardware PWM channel number (0-2)
 * @enable: true to enable PWM function, false to disable
 */
static void smi_pwm_set_gpio_mux(struct smi_pwm_chip *smi, unsigned int hwpwm,
				 bool enable)
{
	void __iomem *mux_reg = smi->iobase + SMI_GPIO_MUX;
	u32 mux_bit = SMI_GPIO_MUX_PWM0 << hwpwm;
	u32 val;

	val = readl(mux_reg);
	if (enable)
		val |= mux_bit;
	else
		val &= ~mux_bit;
	writel(val, mux_reg);
}

/**
 * smi_pwm_calc_clk_div - Calculate the best clock divider
 * @smi: SMI PWM chip data
 * @period_ns: Desired period in nanoseconds
 * @div: Pointer to store the divider index
 * @period_cycles: Pointer to store the period in cycles
 *
 * Returns 0 on success, negative error code on failure.
 */
static int smi_pwm_calc_clk_div(struct smi_pwm_chip *smi, u64 period_ns,
				unsigned int *div, unsigned int *period_cycles)
{
	u64 cycles;
	int i;

	for (i = 0; i < ARRAY_SIZE(smi_pwm_clk_div); i++) {
		cycles = smi->clk_rate;
		cycles = cycles * period_ns;
		do_div(cycles, NSEC_PER_SEC);
		do_div(cycles, smi_pwm_clk_div[i]);

		if (cycles <= SMI_PWM_MAX_COUNTER) {
			*div = i;
			*period_cycles = (unsigned int)cycles;
			return 0;
		}
	}

	return -EINVAL;
}

/**
 * smi_pwm_apply - Atomically apply a new PWM state
 * @chip: PWM chip
 * @pwm: PWM device
 * @state: New PWM state to apply
 *
 * Returns 0 on success, negative error code on failure.
 */
static int smi_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			 const struct pwm_state *state)
{
	struct smi_pwm_chip *smi = to_smi_pwm_chip(chip);
	void __iomem *reg = smi_pwm_get_channel_reg(smi, pwm->hwpwm);
	unsigned int div, period_cycles, high_cycles, low_cycles;
	u32 val;
	int ret;

	if (!state->enabled) {
		/* Disable PWM */
		val = readl(reg);
		val &= ~SMI_PWM_ENABLE;
		val |= SMI_PWM_INT_STATUS;  /* Clear interrupt status */
		writel(val, reg);

		/* Disable PWM function in GPIO mux (switch back to GPIO) */
		smi_pwm_set_gpio_mux(smi, pwm->hwpwm, false);
		return 0;
	}

	/* Calculate clock divider and period cycles */
	ret = smi_pwm_calc_clk_div(smi, state->period, &div, &period_cycles);
	if (ret) {
		return ret;
	}

	/* Calculate high and low counter values */
	if (period_cycles == 0) {
		high_cycles = 0;
		low_cycles = 0;
	} else {
		high_cycles = (unsigned int)(((u64)state->duty_cycle * period_cycles) /
					     state->period);
		low_cycles = period_cycles - high_cycles;

		if (high_cycles == 0 && state->duty_cycle > 0)
			high_cycles = 1;
		if (low_cycles == 0 && state->duty_cycle < state->period)
			low_cycles = 1;
	}

	/* Handle polarity inversion */
	if (state->polarity == PWM_POLARITY_INVERSED) {
		unsigned int tmp = high_cycles;
		high_cycles = low_cycles;
		low_cycles = tmp;
	}

	/* Configure PWM */
	val = (high_cycles << SMI_PWM_HIGH_COUNTER_SHIFT) & SMI_PWM_HIGH_COUNTER_MASK;
	val |= (low_cycles << SMI_PWM_LOW_COUNTER_SHIFT) & SMI_PWM_LOW_COUNTER_MASK;
	val |= (div << SMI_PWM_CLOCK_DIVIDE_SHIFT) & SMI_PWM_CLOCK_DIVIDE_MASK;
	val |= SMI_PWM_INT_STATUS;  /* Clear interrupt status */
	val |= SMI_PWM_ENABLE;

	/* Enable GPIO mux for PWM */
	smi_pwm_set_gpio_mux(smi, pwm->hwpwm, true);

	writel(val, reg);

	return 0;
}

/**
 * smi_pwm_get_state - Get the current PWM state
 * @chip: PWM chip
 * @pwm: PWM device
 * @state: PWM state to fill in
 *
 * Returns 0 on success.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
static int smi_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
			     struct pwm_state *state)
#else
static void smi_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
			     struct pwm_state *state)
#endif				 
{
	struct smi_pwm_chip *smi = to_smi_pwm_chip(chip);
	void __iomem *reg = smi_pwm_get_channel_reg(smi, pwm->hwpwm);
	unsigned int high_cycles, low_cycles, div_idx, total_cycles;
	u64 period_ns, duty_ns;
	u32 val;

	val = readl(reg);

	high_cycles = (val & SMI_PWM_HIGH_COUNTER_MASK) >> SMI_PWM_HIGH_COUNTER_SHIFT;
	low_cycles = (val & SMI_PWM_LOW_COUNTER_MASK) >> SMI_PWM_LOW_COUNTER_SHIFT;
	div_idx = (val & SMI_PWM_CLOCK_DIVIDE_MASK) >> SMI_PWM_CLOCK_DIVIDE_SHIFT;

	total_cycles = high_cycles + low_cycles;

	if (total_cycles == 0 || smi->clk_rate == 0) {
		state->period = 0;
		state->duty_cycle = 0;
	} else {
		/* Calculate period in nanoseconds */
		period_ns = (u64)total_cycles * smi_pwm_clk_div[div_idx] * NSEC_PER_SEC;
		do_div(period_ns, smi->clk_rate);
		state->period = period_ns;

		/* Calculate duty cycle in nanoseconds */
		duty_ns = (u64)high_cycles * smi_pwm_clk_div[div_idx] * NSEC_PER_SEC;
		do_div(duty_ns, smi->clk_rate);
		state->duty_cycle = duty_ns;
	}

	state->enabled = !!(val & SMI_PWM_ENABLE);
	state->polarity = PWM_POLARITY_NORMAL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
	return 0;
#endif
}

static const struct pwm_ops smi_pwm_ops = {
	.apply = smi_pwm_apply,
	.get_state = smi_pwm_get_state,
};

int smi_pwm_init(struct drm_device *ddev)
{
	struct device *dev;
	struct pci_dev *pdev;
	struct smi_device *smi_device = ddev->dev_private;
	struct smi_pwm_chip *smi;
    struct pwm_chip *chip;
	int ret;
	int num_pwm = SMI_PWM_MAX_CHANNELS;

	pdev = to_pci_dev(ddev->dev);
	dev = &pdev->dev;

	/* Allocate PWM chip with driver data */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
	chip = devm_pwmchip_alloc(dev, num_pwm, sizeof(*smi));
	if (IS_ERR(chip)) {
		dev_err(dev, "Failed to allocate PWM chip: %ld\n", PTR_ERR(chip));
		return PTR_ERR(chip);
	}

	smi = to_smi_pwm_chip(chip);
#else
	smi = devm_kzalloc(dev, sizeof(*smi), GFP_KERNEL);
	if (!smi)
		return -ENOMEM;
	chip = &smi->chip;
	chip->npwm = num_pwm;
	chip->dev = dev;
#endif

	smi->iobase = smi_device->rmmio;

	/* Get clock (optional) */
	smi->clk_rate = 24000000;
	

	/* Initialize PWM chip */
	chip->ops = &smi_pwm_ops;

	/* Register PWM chip */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	ret = devm_pwmchip_add(dev, chip);
#else
	ret = pwmchip_add(chip);
#endif
	if (ret < 0) {
		dev_err(dev, "Failed to register PWM chip: %d\n", ret);
		return ret;
	}

	dev_info(dev, "SMI PWM driver registered with %d channel(s), clock rate %lu Hz\n",
         num_pwm, smi->clk_rate);

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
void smi_pwm_remove(struct drm_device *ddev)
{
	struct pwm_chip *chip = NULL;
	struct pci_dev *pdev = to_pci_dev(ddev->dev);
	struct device *dev = &pdev->dev;
	chip = dev_get_drvdata(dev);

	pwmchip_remove(chip);
}
#endif


