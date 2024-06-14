// SPDX-License-Identifier: GPL-2.0
/*
 *    zhaoxin pinctrl common code
 *
 *    Copyright(c) 2021 Shanghai Zhaoxin Corporation. All rights reserved.
 *
 */

#define DRIVER_VERSION "1.0.0"

#include <linux/acpi.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/log2.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/time.h>

#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>

#include "../core.h"
#include "pinctrl-zhaoxin.h"

static int pin_to_hwgpio(struct pinctrl_gpio_range *range, unsigned int pin)
{
	int offset = 0;

	if (range->pins) {
		for (offset = 0; offset < range->npins; offset++)
			if (pin == range->pins[offset])
				break;
		return range->base+offset-range->gc->base;
	} else
		return pin-range->pin_base+range->base-range->gc->base;
}

static u16 zx_pad_read16(struct zhaoxin_pinctrl *pctrl, u8 index)
{
	outb(index, pctrl->pmio_rx90+pctrl->pmio_base);
	return inw(pctrl->pmio_rx8c+pctrl->pmio_base);
}

static void zx_pad_write16(struct zhaoxin_pinctrl *pctrl, u8 index, u16 value)
{
	outb(index, pctrl->pmio_rx90+pctrl->pmio_base);
	outw(value, pctrl->pmio_rx8c+pctrl->pmio_base);
}

static int zhaoxin_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct zhaoxin_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	return pctrl->soc->ngroups;
}

static const char *zhaoxin_get_group_name(struct pinctrl_dev *pctldev, unsigned int group)
{
	struct zhaoxin_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	return pctrl->soc->groups[group].name;
}

static int zhaoxin_get_group_pins(struct pinctrl_dev *pctldev, unsigned int group,
				  const unsigned int **pins, unsigned int *npins)
{
	struct zhaoxin_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	*pins = pctrl->soc->groups[group].pins;
	*npins = pctrl->soc->groups[group].npins;

	return 0;
}

static void zhaoxin_pin_dbg_show(struct pinctrl_dev *pctldev, struct seq_file *s, unsigned int pin)
{

}

static const struct pinctrl_ops zhaoxin_pinctrl_ops = {
	.get_groups_count = zhaoxin_get_groups_count,
	.get_group_name = zhaoxin_get_group_name,
	.get_group_pins = zhaoxin_get_group_pins,
	.pin_dbg_show = zhaoxin_pin_dbg_show,
};

static int zhaoxin_get_functions_count(struct pinctrl_dev *pctldev)
{
	struct zhaoxin_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	return pctrl->soc->nfunctions;
}

static const char *zhaoxin_get_function_name(struct pinctrl_dev *pctldev, unsigned int function)
{
	struct zhaoxin_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	return pctrl->soc->functions[function].name;
}

static int zhaoxin_get_function_groups(struct pinctrl_dev *pctldev, unsigned int function,
				       const char *const **groups, unsigned int *const ngroups)
{
	struct zhaoxin_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	*groups = pctrl->soc->functions[function].groups;
	*ngroups = pctrl->soc->functions[function].ngroups;

	return 0;
}

static int zhaoxin_pinmux_set_mux(struct pinctrl_dev *pctldev, unsigned int function,
				  unsigned int group)
{
	return 0;
}

#define ZHAOXIN_PULL_UP_20K		0x80
#define ZHAOXIN_PULL_UP_10K		0x40
#define ZHAOXIN_PULL_UP_47K		0x20
#define ZHAOXIN_PULL_DOWN		0x10

#define ZHAOXIN_PULL_UP	0xe0

static void zhaoxin_gpio_set_gpio_mode_and_pull(struct zhaoxin_pinctrl *pctrl, unsigned int pin,
						bool isup)
{
	u16 tmp = 0;
	u16 value;
	u16 value_back = 0;

	if (isup)
		tmp = ZHAOXIN_PULL_UP_10K|1;
	else
		tmp = ZHAOXIN_PULL_DOWN|1;
	value = zx_pad_read16(pctrl, pin);

	if (pin <= 0x32 && pin >= 0x29) {
		if (isup) {
			value &= (~(ZHAOXIN_PULL_DOWN));
			value |= tmp;
		} else {
			value &= (~(ZHAOXIN_PULL_UP));
			value |= tmp;
		}
		value &= ~(0x1);
		zx_pad_write16(pctrl, pin, value);
		value_back = zx_pad_read16(pctrl, pin);
	} else {
		if (isup) {
			value &= (~(ZHAOXIN_PULL_DOWN));
			value |= tmp;
		} else {
			value &= (~(ZHAOXIN_PULL_UP));
			value |= tmp;
		}
		value |= 0x1;
		zx_pad_write16(pctrl, pin, value);
		value_back = zx_pad_read16(pctrl, pin);
	}
}

static int zhaoxin_gpio_request_enable(struct pinctrl_dev *pctldev,
				       struct pinctrl_gpio_range *range, unsigned int pin)
{
	struct zhaoxin_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	zhaoxin_gpio_set_gpio_mode_and_pull(pctrl, pin, true);

	return 0;
}

static const struct pinmux_ops zhaoxin_pinmux_ops = {
	.get_functions_count = zhaoxin_get_functions_count,
	.get_function_name = zhaoxin_get_function_name,
	.get_function_groups = zhaoxin_get_function_groups,
	.set_mux = zhaoxin_pinmux_set_mux,
	.gpio_request_enable = zhaoxin_gpio_request_enable,
};

static int zhaoxin_config_get(struct pinctrl_dev *pctldev, unsigned int pin, unsigned long *config)
{
	return 0;
}

static int zhaoxin_config_set(struct pinctrl_dev *pctldev, unsigned int pin, unsigned long *configs,
			      unsigned int nconfigs)
{
	return 0;
}

static const struct pinconf_ops zhaoxin_pinconf_ops = {
	.is_generic = true,
	.pin_config_get = zhaoxin_config_get,
	.pin_config_set = zhaoxin_config_set,
};

static const struct pinctrl_desc zhaoxin_pinctrl_desc = {
	.pctlops = &zhaoxin_pinctrl_ops,
	.pmxops = &zhaoxin_pinmux_ops,
	.confops = &zhaoxin_pinconf_ops,
	.owner = THIS_MODULE,
};

static int zhaoxin_gpio_to_pin(struct zhaoxin_pinctrl *pctrl, unsigned int offset,
			       const struct zhaoxin_pin_topology **community,
			       const struct zhaoxin_pin_map2_gpio **padgrp)
{
	int i;

	for (i = 0; i < pctrl->pin_map_size; i++) {
		const struct zhaoxin_pin_map2_gpio *map = &pctrl->pin_maps[i];

		if (map->zhaoxin_range_gpio_base == ZHAOXIN_GPIO_BASE_NOMAP)
			continue;
		if (offset >= map->zhaoxin_range_gpio_base &&
		    offset < map->zhaoxin_range_gpio_base + map->zhaoxin_range_pin_size) {
			int pin;

			pin = map->zhaoxin_range_pin_base + offset - map->zhaoxin_range_gpio_base;
			if (padgrp)
				*padgrp = map;
			return pin;
		}
	}

	return -EINVAL;
}

static __maybe_unused int zhaoxin_pin_to_gpio(struct zhaoxin_pinctrl *pctrl, int pin)
{
	const struct zhaoxin_pin_map2_gpio *pin_maps;

	pin_maps = pctrl->pin_maps;
	if (!pin_maps)
		return -EINVAL;

	return pin - pin_maps->zhaoxin_range_pin_base + pin_maps->zhaoxin_range_gpio_base;
}

static int zhaoxin_gpio_get(struct gpio_chip *chip, unsigned int offset)
{
	struct zhaoxin_pinctrl *pctrl = gpiochip_get_data(chip);
	const struct index_cal_array *gpio_in_cal;
	int gap = offset/16;
	int bit = offset%16;
	int pin;
	int value;

	gpio_in_cal = pctrl->pin_topologys->gpio_in_cal;
	pin = zhaoxin_gpio_to_pin(pctrl, offset, NULL, NULL);
	value = zx_pad_read16(pctrl, gpio_in_cal->index+gap);
	value &= (1<<bit);

	return !!value;
}

static void zhaoxin_gpio_set(struct gpio_chip *chip, unsigned int offset, int value)
{
	struct zhaoxin_pinctrl *pctrl = gpiochip_get_data(chip);
	const struct index_cal_array *gpio_out_cal;
	unsigned long flags;
	int gap = offset / 16;
	int bit = offset % 16;
	u16 org;
	int pin;

	gpio_out_cal = pctrl->pin_topologys->gpio_out_cal;
	pin = zhaoxin_gpio_to_pin(pctrl, offset, NULL, NULL);

	raw_spin_lock_irqsave(&pctrl->lock, flags);

	org = zx_pad_read16(pctrl, gpio_out_cal->index+gap);
	if (value)
		org |= (1<<bit);
	else
		org &= (~(1<<bit));
	zx_pad_write16(pctrl, gpio_out_cal->index+gap, org);
	raw_spin_unlock_irqrestore(&pctrl->lock, flags);
}

static int zhaoxin_gpio_direction_input(struct gpio_chip *chip, unsigned int offset)
{
	return pinctrl_gpio_direction_input(chip->base + offset);
}

static int zhaoxin_gpio_direction_output(struct gpio_chip *chip, unsigned int offset, int value)
{
	return pinctrl_gpio_direction_output(chip->base + offset);
}

static int zhaoxin_gpio_request(struct gpio_chip *gc, unsigned int offset)
{
	return gpiochip_generic_request(gc, offset);
}
static void zhaoxin_gpio_free(struct gpio_chip *gc, unsigned int offset)
{
	gpiochip_generic_free(gc, offset);
}

static int zhaoxin_gpio_config(struct gpio_chip *gc, unsigned int offset, unsigned long config)
{
	return gpiochip_generic_config(gc, offset, config);
}

static const struct gpio_chip zhaoxin_gpio_chip = {
	.owner = THIS_MODULE,
	.request = zhaoxin_gpio_request,
	.free = zhaoxin_gpio_free,
	.direction_input = zhaoxin_gpio_direction_input,
	.direction_output = zhaoxin_gpio_direction_output,
	.get = zhaoxin_gpio_get,
	.set = zhaoxin_gpio_set,
	.set_config = zhaoxin_gpio_config,
};

static void zhaoxin_gpio_irq_ack(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct zhaoxin_pinctrl *pctrl = gpiochip_get_data(gc);
	const struct reg_calibrate *status_cal;
	const struct reg_cal_array *reg_off;
	int gpio = irqd_to_hwirq(d);
	int i, j;
	int offset = 0;
	int base_offset = 0;
	int bit_off = 0;
	u16 value;
	u16 value_read;

	status_cal = pctrl->pin_topologys->status_cal;
	if (gpio >= 0) {
		for (i = 0; i < status_cal->size; i++)
			if (gpio == status_cal->cal_array[i])
				break;
		for (j = 0; j < status_cal->reg_cal_size; j++) {
			if (offset > i)
				break;
			offset += status_cal->reg[j].size;
		}
		reg_off = &status_cal->reg[j-1];
		bit_off = i-(offset-reg_off->size);
		base_offset = reg_off->pmio_offset;
		value = readw(pctrl->pm_pmio_base+reg_off->pmio_offset);
		value_read = value;
		value |= (1<<bit_off);
		writew(value, pctrl->pm_pmio_base+reg_off->pmio_offset);
	}
}

static void zhaoxin_gpio_irq_mask_unmask(struct irq_data *d, bool mask)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct zhaoxin_pinctrl *pctrl = gpiochip_get_data(gc);
	const struct reg_calibrate *int_cal;
	const struct reg_calibrate *mod_sel_cal;
	int gpio = irqd_to_hwirq(d);
	int i, j;
	int offset = 0;
	int base_offset = 0;
	const struct reg_cal_array *reg_off, *mod;
	int bit_off = 0;
	u16 value;
	u16 value1;

	int_cal = pctrl->pin_topologys->int_cal;
	mod_sel_cal = pctrl->pin_topologys->mod_sel_cal;

	if (gpio >= 0) {
		for (i = 0; i < int_cal->size; i++)
			if (gpio == int_cal->cal_array[i])
				break;
		for (j = 0; j < int_cal->reg_cal_size; j++) {
			if (offset > i)
				break;
			offset += int_cal->reg[j].size;
		}
		reg_off = &(int_cal->reg[j-1]);
		mod = &(mod_sel_cal->reg[j-1]);
		bit_off = i-(offset-reg_off->size);
		base_offset = reg_off->pmio_offset;

		value = inw(pctrl->pmio_base+reg_off->pmio_offset);
		if (mask)
			value &= (~(1<<bit_off));
		else
			value |= (1<<bit_off);

		outw(value, pctrl->pmio_base+reg_off->pmio_offset);
		if (mask) {
			value1 = readw(pctrl->pm_pmio_base+mod->pmio_offset);
			value1 |= (1<<bit_off);
			writew(value1, pctrl->pm_pmio_base+mod->pmio_offset);
		} else {
			value1 = readw(pctrl->pm_pmio_base+mod->pmio_offset);
			value1 |= (1<<bit_off);
			writew(value1, pctrl->pm_pmio_base+mod->pmio_offset);
		}
	}
}

static void zhaoxin_gpio_irq_mask(struct irq_data *d)
{
	zhaoxin_gpio_irq_mask_unmask(d, true);
}

static void zhaoxin_gpio_irq_unmask(struct irq_data *d)
{
	zhaoxin_gpio_irq_mask_unmask(d, false);
}

/*
 * father domain irq handle
 */
static irqreturn_t zhaoxin_gpio_irq(int irq, void *data)
{
	struct zhaoxin_pinctrl *pctrl = data;
	struct gpio_chip *gc = &pctrl->chip;
	const struct reg_calibrate *init;
	const struct reg_calibrate *stat_cal;
	unsigned int i, bit_offset;
	u16 status, enable;
	unsigned long pending;
	int index = 0;
	int ret = 0;
	int subirq;
	unsigned int hwirq;

	init = pctrl->pin_topologys->int_cal;
	stat_cal = pctrl->pin_topologys->status_cal;
	for (i = 0; i < init->reg_cal_size; i++) {
		pending = 0;
		status = readw(pctrl->pm_pmio_base + stat_cal->reg[i].pmio_offset);
		enable = inw(pctrl->pmio_base + init->reg[i].pmio_offset);
		enable &= status;
		pending = enable;
		for_each_set_bit(bit_offset, &pending, init->reg[i].size) {
			hwirq = init->cal_array[index + bit_offset] ;
			subirq = irq_find_mapping(gc->irq.domain, hwirq);
			generic_handle_irq(subirq);
		}

		ret += pending ? 1 : 0;
		index += init->reg[i].size;
	}

	return IRQ_RETVAL(ret);
}

static int zhaoxin_gpio_irq_type(struct irq_data *d, unsigned int type)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct zhaoxin_pinctrl *pctrl = gpiochip_get_data(gc);
	unsigned int gpio = irqd_to_hwirq(d);
	const struct index_cal_array *trigger_cal;
	unsigned int pin;
	unsigned long flags;
	u8 index;
	int position, point;
	u16 value;
	bool isup = true;

	trigger_cal = pctrl->pin_topologys->trigger_cal;
	pin = zhaoxin_gpio_to_pin(pctrl, irqd_to_hwirq(d), NULL, NULL);
	if (type & IRQ_TYPE_EDGE_FALLING)
		isup = true;
	else if (type & IRQ_TYPE_EDGE_RISING)
		isup = true;
	else if (type & IRQ_TYPE_LEVEL_LOW)
		isup = true;
	else if (type & IRQ_TYPE_LEVEL_HIGH)
		isup = false;

	zhaoxin_gpio_set_gpio_mode_and_pull(pctrl, pin, isup);

	for (position = 0; position < trigger_cal->size; position++)
		if (trigger_cal->cal_array[position] == gpio)
			break;

	index = trigger_cal->index + ALIGN(position+1, 4)/4-1;
	point = position % 4;

	raw_spin_lock_irqsave(&pctrl->lock, flags);

	value = zx_pad_read16(pctrl, index);

	if ((type & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH)
		value |= TRIGGER_BOTH_EDGE << (point*4);
	else if (type & IRQ_TYPE_EDGE_FALLING)
		value |= TRIGGER_FALL_EDGE << (point*4);
	else if (type & IRQ_TYPE_EDGE_RISING)
		value |= TRIGGER_RISE_EDGE << (point*4);
	else if (type & IRQ_TYPE_LEVEL_LOW)
		value |= TRIGGER_LOW_LEVEL << (point*4);
	else if (type & IRQ_TYPE_LEVEL_HIGH)
		value |= TRIGGER_HIGH_LEVEL << (point*4);
	else
		pr_debug(pctrl, "%s wrang type\n", __func__);

	zx_pad_write16(pctrl, index, value);

	if (type & IRQ_TYPE_EDGE_BOTH)
		irq_set_handler_locked(d, handle_edge_irq);
	else if (type & IRQ_TYPE_LEVEL_MASK)
		irq_set_handler_locked(d, handle_level_irq);
	raw_spin_unlock_irqrestore(&pctrl->lock, flags);

	return 0;
}

static int zhaoxin_gpio_irq_wake(struct irq_data *d, unsigned int on)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct zhaoxin_pinctrl *pctrl = gpiochip_get_data(gc);
	unsigned int pin;

	pin = zhaoxin_gpio_to_pin(pctrl, irqd_to_hwirq(d), NULL, NULL);
	if (pin) {
		if (on)
			enable_irq_wake(pctrl->irq);
		else
			disable_irq_wake(pctrl->irq);
	}

	return 0;
}

static int zhaoxin_gpio_add_pin_ranges(struct gpio_chip *gc)
{
	struct zhaoxin_pinctrl *pctrl = gpiochip_get_data(gc);
	int ret, i;

	for (i = 0; i < pctrl->pin_map_size; i++) {
		struct zhaoxin_pin_map2_gpio *map = &pctrl->pin_maps[i];

		if (map->zhaoxin_range_gpio_base == ZHAOXIN_GPIO_BASE_NOMAP)
			continue;
		ret = gpiochip_add_pin_range(&pctrl->chip, dev_name(pctrl->dev),
					     map->zhaoxin_range_gpio_base,
					     map->zhaoxin_range_pin_base,
					     map->zhaoxin_range_pin_size);
		if (ret) {
			dev_err(pctrl->dev, "failed to add GPIO pin range\n");
			return ret;
		}
	}

	return 0;
}

static unsigned int zhaoxin_gpio_ngpio(const struct zhaoxin_pinctrl *pctrl)
{
	const struct zhaoxin_pin_map2_gpio *pin_maps;
	unsigned int ngpio = 0;
	int i;

	for (i = 0; i < pctrl->pin_map_size; i++) {
		pin_maps = &pctrl->pin_maps[i];
		if (pin_maps->zhaoxin_range_gpio_base == ZHAOXIN_GPIO_BASE_NOMAP)
			continue;
		if (pin_maps->zhaoxin_range_gpio_base + pin_maps->zhaoxin_range_pin_size > ngpio)
			ngpio = pin_maps->zhaoxin_range_gpio_base +
				pin_maps->zhaoxin_range_pin_size;
	}

	return ngpio;
}

static int zhaoxin_gpio_probe(struct zhaoxin_pinctrl *pctrl, int irq)
{
	int ret;
	struct gpio_irq_chip *girq;

	pctrl->chip = zhaoxin_gpio_chip;

	pctrl->chip.ngpio = zhaoxin_gpio_ngpio(pctrl);
	pctrl->chip.label = dev_name(pctrl->dev);
	pctrl->chip.parent = pctrl->dev;
	pctrl->chip.base = -1;
	pctrl->chip.add_pin_ranges = zhaoxin_gpio_add_pin_ranges;

	pctrl->irq = irq;

	pctrl->irqchip.name = dev_name(pctrl->dev);
	pctrl->irqchip.irq_ack = zhaoxin_gpio_irq_ack;
	pctrl->irqchip.irq_mask = zhaoxin_gpio_irq_mask;
	pctrl->irqchip.irq_unmask = zhaoxin_gpio_irq_unmask;
	pctrl->irqchip.irq_set_type = zhaoxin_gpio_irq_type;
	pctrl->irqchip.irq_set_wake = zhaoxin_gpio_irq_wake;
	pctrl->irqchip.flags = IRQCHIP_MASK_ON_SUSPEND;
	/*
	 * father domain irq
	 */
	ret = devm_request_irq(pctrl->dev, irq, zhaoxin_gpio_irq, IRQF_SHARED | IRQF_NO_THREAD,
			       dev_name(pctrl->dev), pctrl);
	if (ret) {
		dev_err(pctrl->dev, "failed to request interrupt\n");
		return ret;
	}
	girq = &pctrl->chip.irq;
	girq->chip = &pctrl->irqchip;
	/* This will let us handle the IRQ in the driver */
	girq->parent_handler = NULL;
	girq->num_parents = 0;
	girq->default_type = IRQ_TYPE_NONE;
	girq->handler = handle_bad_irq;
	ret = devm_gpiochip_add_data(pctrl->dev, &pctrl->chip, pctrl);
	if (ret) {
		dev_err(pctrl->dev, "failed to register gpiochip\n");
		return ret;
	}

	return 0;
}

static int zhaoxin_pinctrl_pm_init(struct zhaoxin_pinctrl *pctrl)
{
	return 0;
}

static int zhaoxin_pinctrl_probe(struct platform_device *pdev,
				 const struct zhaoxin_pinctrl_soc_data *soc_data)
{
	struct zhaoxin_pinctrl *pctrl;
	int  ret, i, irq;
	struct resource *res;
	void __iomem *regs;

	pctrl = devm_kzalloc(&pdev->dev, sizeof(*pctrl), GFP_KERNEL);
	if (!pctrl)
		return -ENOMEM;
	pctrl->dev = &pdev->dev;
	pctrl->soc = soc_data;
	raw_spin_lock_init(&pctrl->lock);
	pctrl->pin_topologys = pctrl->soc->pin_topologys;
	pctrl->pin_map_size = pctrl->soc->pin_map_size;
	pctrl->pin_maps = devm_kcalloc(&pdev->dev, pctrl->pin_map_size, sizeof(*pctrl->pin_maps),
				       GFP_KERNEL);
	if (!pctrl->pin_maps)
		return -ENOMEM;
	for (i = 0; i < pctrl->pin_map_size; i++) {
		struct zhaoxin_pin_map2_gpio *community = &pctrl->pin_maps[i];
		*community = pctrl->soc->zhaoxin_pin_maps[i];
	}
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs))
		return PTR_ERR(regs);

	pctrl->pm_pmio_base = regs;
	pctrl->pmio_base = 0x800;
	pctrl->pmio_rx90 = 0x90;
	pctrl->pmio_rx8c = 0x8c;
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	ret = zhaoxin_pinctrl_pm_init(pctrl);
	if (ret)
		return ret;
	pctrl->pctldesc = zhaoxin_pinctrl_desc;
	pctrl->pctldesc.name = dev_name(&pdev->dev);
	pctrl->pctldesc.pins = pctrl->soc->pins;
	pctrl->pctldesc.npins = pctrl->soc->npins;
	pctrl->pctldev = devm_pinctrl_register(&pdev->dev, &pctrl->pctldesc, pctrl);
	if (IS_ERR(pctrl->pctldev)) {
		dev_err(&pdev->dev, "failed to register pinctrl driver\n");
		return PTR_ERR(pctrl->pctldev);
	}
	ret = zhaoxin_gpio_probe(pctrl, irq);

	if (ret)
		return ret;
	platform_set_drvdata(pdev, pctrl);
	return 0;
}

int zhaoxin_pinctrl_probe_by_hid(struct platform_device *pdev)
{
	const struct zhaoxin_pinctrl_soc_data *data;

	data = device_get_match_data(&pdev->dev);
	if (!data)
		return -ENODATA;

	return zhaoxin_pinctrl_probe(pdev, data);
}
EXPORT_SYMBOL_GPL(zhaoxin_pinctrl_probe_by_hid);

int zhaoxin_pinctrl_probe_by_uid(struct platform_device *pdev)
{
	const struct zhaoxin_pinctrl_soc_data *data;

	data = zhaoxin_pinctrl_get_soc_data(pdev);
	if (IS_ERR(data))
		return PTR_ERR(data);

	return zhaoxin_pinctrl_probe(pdev, data);
}
EXPORT_SYMBOL_GPL(zhaoxin_pinctrl_probe_by_uid);


const struct zhaoxin_pinctrl_soc_data *zhaoxin_pinctrl_get_soc_data(struct platform_device *pdev)
{
	const struct zhaoxin_pinctrl_soc_data *data = NULL;
	const struct zhaoxin_pinctrl_soc_data **table;
	struct acpi_device *adev;
	unsigned int i;

	adev = ACPI_COMPANION(&pdev->dev);
	if (adev) {
		const void *match = device_get_match_data(&pdev->dev);

		table = (const struct zhaoxin_pinctrl_soc_data **)match;
		for (i = 0; table[i]; i++) {
			if (!strcmp(adev->pnp.unique_id, table[i]->uid)) {
				data = table[i];
				break;
			}
		}
	} else {
		const struct platform_device_id *id;

		id = platform_get_device_id(pdev);
		if (!id)
			return ERR_PTR(-ENODEV);

		table = (const struct zhaoxin_pinctrl_soc_data **)id->driver_data;
		data = table[pdev->id];
	}

	return data ?: ERR_PTR(-ENODATA);
}
EXPORT_SYMBOL_GPL(zhaoxin_pinctrl_get_soc_data);

#ifdef CONFIG_PM_SLEEP
int zhaoxin_pinctrl_suspend_noirq(struct device *dev)
{
	return 0;
}
EXPORT_SYMBOL_GPL(zhaoxin_pinctrl_suspend_noirq);

int zhaoxin_pinctrl_resume_noirq(struct device *dev)
{
	return 0;
}
EXPORT_SYMBOL_GPL(zhaoxin_pinctrl_resume_noirq);
#endif /* CONFIG_PM_SLEEP */

MODULE_AUTHOR("www.zhaoxin.com");
MODULE_DESCRIPTION("Shanghai Zhaoxin pinctrl driver");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL v2");
