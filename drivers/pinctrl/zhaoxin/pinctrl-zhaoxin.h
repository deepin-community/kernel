/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * zhaoxin pinctrl common code
 * Copyright(c) 2023 Shanghai Zhaoxin Corporation. All rights reserved.
 */

#ifndef PINCTRL_zhaoxin_H
#define PINCTRL_zhaoxin_H

#include <linux/bits.h>
#include <linux/compiler_types.h>
#include <linux/gpio/driver.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/pm.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/spinlock_types.h>

struct platform_device;
struct device;

/*
 * struct zhaoxin_pingroup pin define
 */
struct zhaoxin_pingroup {
	const char *name;
	const unsigned int *pins;
	size_t npins;
	unsigned short mode;
	const unsigned int *modes;
};

/*
 * struct zhaoxin_function
 */
struct zhaoxin_function {
	const char *name;
	const char * const *groups;
	size_t ngroups;
};

/*
 * struct zhaoxin_pin_map2_gpio
 * @zhaoxin_range_pin_base
 * @size: pin number
 * @zhaoxin_range_gpio_base
 */
struct zhaoxin_pin_map2_gpio {
	unsigned int zhaoxin_range_pin_base;
	unsigned int zhaoxin_range_pin_size;
	int zhaoxin_range_gpio_base;
};

#define MAX_GPIO		256

struct reg_cal_array {
	int pmio_offset;
	int size;
};

struct reg_calibrate {
	const struct reg_cal_array *reg;
	const int reg_cal_size;
	const int *cal_array;
	const int size;
};

struct index_cal_array {
	int reg_port_base;
	int reg_data_base;
	int index;
	int *cal_array;
	int size;
};

struct zhaoxin_pin_topology {
	const struct reg_calibrate *int_cal;
	const struct reg_calibrate *mod_sel_cal;
	const struct reg_calibrate *status_cal;
	const struct index_cal_array *gpio_in_cal;
	const struct index_cal_array *gpio_out_cal;
	const struct index_cal_array *gpio_dir_cal;
	const struct index_cal_array *trigger_cal;
};

#define TRIGGER_FALL_EDGE	0
#define TRIGGER_RISE_EDGE	1
#define TRIGGER_BOTH_EDGE	2
#define TRIGGER_LOW_LEVEL	3
#define TRIGGER_HIGH_LEVEL	4

#define	ZHAOXIN_GPIO_BASE_NOMAP -1

struct zhaoxin_pinctrl_soc_data {
	const char *uid;
	const struct pinctrl_pin_desc *pins;
	size_t npins;
	const struct zhaoxin_pingroup *groups;
	size_t ngroups;
	const struct zhaoxin_function *functions;
	size_t nfunctions;
	const struct zhaoxin_pin_topology *pin_topologys;
	const struct zhaoxin_pin_map2_gpio *zhaoxin_pin_maps;
	size_t pin_map_size;
};

const struct zhaoxin_pinctrl_soc_data *zhaoxin_pinctrl_get_soc_data(struct platform_device *pdev);

struct zhaoxin_pinctrl {
	struct device *dev;
	raw_spinlock_t lock;
	struct pinctrl_desc pctldesc;
	struct pinctrl_dev *pctldev;
	struct gpio_chip chip;
	struct irq_chip irqchip;
	const struct zhaoxin_pinctrl_soc_data *soc;
	const struct zhaoxin_pin_topology *pin_topologys;
	struct zhaoxin_pin_map2_gpio *pin_maps;
	size_t pin_map_size;
	int irq;
	int pmio_base;
	void __iomem *pm_pmio_base;
	int pmio_rx90;
	int pmio_rx8c;
};

int zhaoxin_pinctrl_probe_by_hid(struct platform_device *pdev);
int zhaoxin_pinctrl_probe_by_uid(struct platform_device *pdev);

#ifdef CONFIG_PM_SLEEP
int zhaoxin_pinctrl_suspend_noirq(struct device *dev);
int zhaoxin_pinctrl_resume_noirq(struct device *dev);
#endif

#endif /* PINCTRL_zhaoxin_H */
