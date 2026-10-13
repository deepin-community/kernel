#ifndef __SMI_PWM_H__
#define __SMI_PWM_H__

#include <linux/err.h>
#include <linux/io.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/types.h>

/* SMI PWM Register Definitions */
/*
 * There are 3 PWMs in the system with the same register layout:
 *   PWM 0: 0x010020
 *   PWM 1: 0x010024
 *   PWM 2: 0x010028
 */
#define SMI_PWM_BASE			0x010020
#define SMI_PWM_CHANNEL_OFFSET		0x04

/* PWM Control Register bit definitions */
#define SMI_PWM_HIGH_COUNTER_SHIFT	20
#define SMI_PWM_HIGH_COUNTER_MASK	(0xFFF << SMI_PWM_HIGH_COUNTER_SHIFT)
#define SMI_PWM_LOW_COUNTER_SHIFT	8
#define SMI_PWM_LOW_COUNTER_MASK	(0xFFF << SMI_PWM_LOW_COUNTER_SHIFT)
#define SMI_PWM_CLOCK_DIVIDE_SHIFT	4
#define SMI_PWM_CLOCK_DIVIDE_MASK	(0xF << SMI_PWM_CLOCK_DIVIDE_SHIFT)
#define SMI_PWM_INT_STATUS		BIT(3)
#define SMI_PWM_INT_ENABLE		BIT(2)
#define SMI_PWM_ENABLE			BIT(0)

/* GPIO MUX Register for PWM pin configuration */
#define SMI_GPIO_MUX			0x010010
#define SMI_GPIO_MUX_PWM0		BIT(21)
#define SMI_GPIO_MUX_PWM1		BIT(22)
#define SMI_GPIO_MUX_PWM2		BIT(23)

/* Maximum values */
#define SMI_PWM_MAX_CHANNELS		3
#define SMI_PWM_MAX_COUNTER		0xFFF	/* 12-bit counter */

/**
 * struct smi_pwm_chip - SMI PWM chip data
 * @base: Base address of PWM registers (mapped to MMIO base)
 * @clk: Clock source for PWM
 * @clk_rate: Clock rate in Hz
 */
struct smi_pwm_chip {
    struct pci_device *pdev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 9, 0)
    struct pwm_chip chip;
#endif
    void __iomem *iobase;
	unsigned long clk_rate;
};

#endif				/* __SMI_PWM_H__ */