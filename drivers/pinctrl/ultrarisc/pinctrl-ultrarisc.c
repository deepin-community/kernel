// SPDX-License-Identifier: GPL-2.0
/* UltraRisc pinctrl driver
 *
 * Copyright(C) 2025 UltraRisc Technology Co., Ltd.
 *
 *  Author:  wangjia <wangjia@ultrarisc.com>
 */

#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/machine.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/seq_file.h>

#include "../pinctrl-utils.h"
#include "../pinmux.h"
#include "../core.h"
#include "../devicetree.h"

#include "pinctrl-ultrarisc.h"

static int ur_pin_to_desc(struct pinctrl_dev *pctldev, struct ur_pin_val *pin_val)
{
	int index = 0;
	struct ur_pinctrl *ur_pinctrl = pinctrl_dev_get_drvdata(pctldev);
	const struct ur_pinctrl_match_data *ur_match_data = ur_pinctrl->match_data;

	for (int i = 0; i < pin_val->port; i++)
		index += ur_match_data->ports[i].npins;
	index += pin_val->pin;
	dev_dbg(pctldev->dev, "port %d pin %d index %d\n", pin_val->port, pin_val->pin, index);
	return index;
}

static int ur_subnode_to_pin(struct pinctrl_dev *pctldev,
			const char *name,
			enum pinctrl_map_type type,
			struct device_node *np,
			int **pins,
			struct ur_pin_val **pin_val,
			int *pin_num)
{
	struct ur_pin_val *pin_vals;
	int rows;
	int ret = -EINVAL;
	int *group_pins;
	const char **pgnames;

	dev_dbg(pctldev->dev, "pinctrl node %s\n", np->name);
	rows = pinctrl_count_index_with_args(np, name);
	if (rows < 0) {
		dev_err(pctldev->dev, "%s count is invalid %d\n", name, rows);
		return rows;
	}

	pin_vals = devm_kcalloc(pctldev->dev, rows, sizeof(*pin_vals), GFP_KERNEL);
	if (!pin_vals) {
		return -ENOMEM;
	}

	group_pins = devm_kcalloc(pctldev->dev, rows, sizeof(*group_pins), GFP_KERNEL);
	if (!group_pins) {
		ret = -ENOMEM;
		goto free_pin_vals;
	}

	pgnames = devm_kzalloc(pctldev->dev, sizeof(*pgnames), GFP_KERNEL);
	if (!pgnames) {
		ret = -ENOMEM;
		goto free_pins;
	}

	for (int i = 0; i < rows; i++) {
		struct of_phandle_args pin_args;

		ret = pinctrl_parse_index_with_args(np, name, i, &pin_args);
		if (ret) {
			dev_err(pctldev->dev, "parse args of %s index %d failed\n", name, i);
			goto free_pgnames;
		}

		if (pin_args.args_count < 3) {
			dev_err(pctldev->dev, "invalid args_count(%d) of %s index %d/%d\n",
				pin_args.args_count, name, i, rows);
			ret = -EINVAL;
			goto free_pgnames;
		}
		pin_vals[i].port = pin_args.args[0];
		pin_vals[i].pin = pin_args.args[1];
		pin_vals[i].mode = pin_args.args[2];

		dev_dbg(pctldev->dev, "found a pinctrl: port=%d pin=%d val=0x%x\n",
			pin_vals[i].port, pin_vals[i].pin, pin_vals[i].mode);

		group_pins[i] = ur_pin_to_desc(pctldev, &pin_vals[i]);
	}

	dev_dbg(pctldev->dev, "get an pinmux of %s\n", np->name);

	ret = pinctrl_generic_add_group(pctldev, np->name, group_pins, rows, pin_vals);
	if (ret < 0) {
		dev_err(pctldev->dev, "add group %s failed\n", np->name);
		goto free_pgnames;
	}

	*pgnames = np->name;
	ret = pinmux_generic_add_function(pctldev, np->name, pgnames, 1, NULL);
	if (ret < 0) {
		dev_err(pctldev->dev, "add function %s failed\n", np->name);
		goto free_group;
	}

	dev_dbg(pctldev->dev, "add group and function of %s\n", np->name);

	*pins = group_pins;
	*pin_val = pin_vals;
	*pin_num = rows;

	return 0;

free_group:
	pinctrl_generic_remove_group(pctldev, ret);
free_pgnames:
	devm_kfree(pctldev->dev, pgnames);
free_pins:
	devm_kfree(pctldev->dev, group_pins);
free_pin_vals:
	devm_kfree(pctldev->dev, pin_vals);
	return ret;
}

static int ur_pinmux_to_map(struct pinctrl_dev *pctldev,
			struct device_node *np,
			struct pinctrl_map *map)
{
	int ret;
	int *pins;
	struct ur_pin_val *pin_vals;
	int pin_num;

	ret = ur_subnode_to_pin(pctldev, PINMUX_PROP_NAME, PIN_MAP_TYPE_MUX_GROUP,
				np, &pins, &pin_vals, &pin_num);
	if (ret) {
		dev_err(pctldev->dev, "get pinmux data %s failed\n", np->name);
		return ret;
	}

	map->type = PIN_MAP_TYPE_MUX_GROUP;
	map->data.mux.group = np->name;
	map->data.mux.function = np->name;

	dev_dbg(pctldev->dev, "type=%d, mux.group=%s, mux.function=%s\n",
		map->type, map->data.mux.group, map->data.mux.function);

	return 0;
}

static int ur_pinconf_to_map(struct pinctrl_dev *pctldev,
			struct device_node *np,
			struct pinctrl_map *map)
{
	int ret;
	int *pins;
	struct ur_pin_val *pin;
	int pin_num;

	ret = ur_subnode_to_pin(pctldev, PINCONF_PROP_NAME, PIN_MAP_TYPE_CONFIGS_GROUP,
				np, &pins, &pin, &pin_num);
	if (ret) {
		dev_err(pctldev->dev, "get pinconf data %s failed\n", np->name);
		return ret;
	}

	dev_dbg(pctldev->dev, "get an pinconf of %s\n", np->name);
	map->type = PIN_MAP_TYPE_CONFIGS_GROUP;
	map->data.configs.group_or_pin = np->name;
	map->data.configs.configs = (unsigned long *)pin;
	map->data.configs.num_configs = pin_num;

	dev_dbg(pctldev->dev, "type=%d, config.group_or_pin=%s, configs.num_config=%d\n",
		map->type, map->data.configs.group_or_pin, map->data.configs.num_configs);

	return 0;
}

static int ur_dt_node_to_map(struct pinctrl_dev *pctldev,
			struct device_node *np,
			struct pinctrl_map **map,
			unsigned int *num_maps)
{
	int ret;
	bool mux_present = false, conf_present = false;
	struct pinctrl_map *new_map;
	unsigned int map_num = 0, prop_count = 0;

	//device_get_named_child_node(pctldev->dev, np->name);
	if (of_property_present(np, PINMUX_PROP_NAME)) {
		mux_present = true;
		prop_count++;
	}
	if (of_property_present(np, PINCONF_PROP_NAME)) {
		conf_present = true;
		prop_count++;
	}

	if (!prop_count) {
		dev_err(pctldev->dev, "no pinctrl node(%d) in %s\n", prop_count, np->name);
		return -EINVAL;
	}

	new_map = devm_kmalloc_array(pctldev->dev, prop_count, sizeof(**map), GFP_KERNEL);
	if (!new_map)
		return -ENOMEM;

	*map = new_map;
	if (mux_present) {
		ret = ur_pinmux_to_map(pctldev, np, new_map);
		if (!ret) {
			new_map++;
			map_num++;
		}
	}
	if (conf_present) {
		ret = ur_pinconf_to_map(pctldev, np, new_map);
		if (!ret)
			map_num++;
	}

	if (!map_num) {
		dev_err(pctldev->dev, "no pinctrl info of %s failed\n", np->name);
		goto free_map;
	}
	*num_maps = map_num;

	return 0;

free_map:
	devm_kfree(pctldev->dev, new_map);
	return ret;
}

static void ur_dt_free_map(struct pinctrl_dev *pctldev,
			struct pinctrl_map *map, unsigned int num_maps)
{
	if (map)
		devm_kfree(pctldev->dev, map);
}

static void ur_pin_dbg_show(struct pinctrl_dev *pctldev,
			struct seq_file *s, unsigned int offset)
{
	seq_printf(s, "%s", dev_name(pctldev->dev));
}

static const struct pinctrl_ops ur_pinctrl_ops = {
	.get_groups_count = pinctrl_generic_get_group_count,
	.get_group_name = pinctrl_generic_get_group_name,
	.get_group_pins = pinctrl_generic_get_group_pins,
	.dt_node_to_map = ur_dt_node_to_map,
	.dt_free_map = ur_dt_free_map,
	.pin_dbg_show = ur_pin_dbg_show,
};

static int ur_set_pin_mux(struct ur_pinctrl *pin_ctrl, struct ur_pin_val *pin_vals)
{
	unsigned long flag;
	//bool clear_mode = false;
	void __iomem *reg;
	u32 val;
	const struct ur_port_desc *port;

	port = &pin_ctrl->match_data->ports[pin_vals->port];

	reg = pin_ctrl->base + port->func_offset;

	raw_spin_lock_irqsave(&pin_ctrl->lock, flag);
	val = readl_relaxed(reg);
	val &= ~((UR_FUNC0 | UR_FUNC1)<<pin_vals->pin);
	val |= (pin_vals->mode << pin_vals->pin);
	writel_relaxed(val, reg);
	raw_spin_unlock_irqrestore(&pin_ctrl->lock, flag);

	return 0;
}

static int ur_set_mux(struct pinctrl_dev *pctldev, unsigned int func_selector,
		unsigned int group_selector)
{
	struct ur_pinctrl *ur_pinctrl = pinctrl_dev_get_drvdata(pctldev);
	struct group_desc *ur_group;
	struct ur_pin_val *pin_vals;

	dev_dbg(pctldev->dev, "set mux: func_selector=%d, group_selector=%d\n",
		func_selector, group_selector);
	ur_group = pinctrl_generic_get_group(pctldev, group_selector);
	if (!ur_group) {
		dev_err(pctldev->dev, "get group %d failed\n", group_selector);
		return -EINVAL;
	}

	dev_dbg(pctldev->dev, "get group %s, num_pins=%zu\n", ur_group->grp.name, ur_group->grp.npins);
	pin_vals = ur_group->data;
	if (!pin_vals) {
		dev_err(pctldev->dev, "data of %s is invalid\n", ur_group->grp.name);
		return -EINVAL;
	}

	for (int i = 0; i < ur_group->grp.npins; i++)
		ur_set_pin_mux(ur_pinctrl, &pin_vals[i]);

	return 0;
}

static const struct pinmux_ops ur_pinmux_ops = {
	.get_functions_count = pinmux_generic_get_function_count,
	.get_function_name   = pinmux_generic_get_function_name,
	.get_function_groups = pinmux_generic_get_function_groups,
	.set_mux = ur_set_mux,
	.strict = true,
};

#define UR_CONF_BIT_PER_PIN	(4)
#define UR_CONF_PIN_PER_REG	(32/UR_CONF_BIT_PER_PIN)
static int ur_set_pin_conf(struct ur_pinctrl *pin_ctrl, struct ur_pin_val *pin_vals)
{
	const struct ur_port_desc *port_desc;
	unsigned long flag;
	void __iomem *reg;
	u32 val, conf;

	port_desc = &pin_ctrl->match_data->ports[pin_vals->port];
	reg = pin_ctrl->base + port_desc->conf_offset;
	dev_dbg(pin_ctrl->dev, "pinconf base=0x%llx, reg=0x%llx\n", (u64)pin_ctrl->base, (u64)reg);
	reg += (pin_vals->pin / UR_CONF_PIN_PER_REG) * UR_CONF_BIT_PER_PIN;
	dev_dbg(pin_ctrl->dev, "pinconf pin=0x%llx\n", (u64)reg);

	conf = pin_vals->conf << ((pin_vals->pin % UR_CONF_PIN_PER_REG) * UR_CONF_BIT_PER_PIN);
	dev_dbg(pin_ctrl->dev, "pinconf conf=0x%x\n", conf);

	raw_spin_lock_irqsave(&pin_ctrl->lock, flag);
	val = readl_relaxed(reg);
	val &= ~(UR_BIAS_MASK << ((pin_vals->pin % UR_CONF_PIN_PER_REG) * UR_CONF_BIT_PER_PIN));
	val |= conf;
	writel_relaxed(val, reg);
	raw_spin_unlock_irqrestore(&pin_ctrl->lock, flag);
	dev_dbg(pin_ctrl->dev, "pinconf val=0x%x\n", val);

	return 0;
}

static int ur_pin_config_get(struct pinctrl_dev *pctldev,
			unsigned int pin,
			unsigned long *config)
{
	dev_dbg(pctldev->dev, "%s(%d): pin=%d\n", __func__, __LINE__, pin);
	// TODO: this is call by pinconf-generic
	return -EOPNOTSUPP;
}

static int ur_pin_config_set(struct pinctrl_dev *pctldev,
			unsigned int pin,
			unsigned long *configs,
			unsigned int num_configs)
{
	struct ur_pin_val *pin_conf;
	struct ur_pinctrl *ur_pinctrl = pinctrl_dev_get_drvdata(pctldev);

	dev_dbg(pctldev->dev, "%s(%d): pin=%d, num_configs=%d\n",
		__func__, __LINE__, pin, num_configs);
	pin_conf = (struct ur_pin_val *)configs;
	for (int i = 0; i < num_configs; i++) {
		dev_dbg(pctldev->dev, "pinconf[%d], port=%d, pin=%d, conf=0x%x\n",
			i, pin_conf[i].port, pin_conf[i].pin, pin_conf[i].conf);
		ur_set_pin_conf(ur_pinctrl, &pin_conf[i]);
	}
	return 0;
}

static int ur_pin_config_group_get(struct pinctrl_dev *pctldev,
				unsigned selector,
				unsigned long *config)
{
	dev_dbg(pctldev->dev, "%s(%d): selector=%d, config=0x%lx\n",
		__func__, __LINE__, selector, *config);
	return -EOPNOTSUPP;
}

static int ur_pin_config_group_set(struct pinctrl_dev *pctldev,
			unsigned int selector,
			unsigned long *configs,
			unsigned int num_configs)
{
	struct group_desc *ur_group;
	struct ur_pin_val *pin_conf;
	struct ur_pinctrl *ur_pinctrl = pinctrl_dev_get_drvdata(pctldev);

	dev_dbg(pctldev->dev, "%s(%d): selector=%d, num_configs=%d\n",
		__func__, __LINE__, selector, num_configs);
	ur_group = pinctrl_generic_get_group(pctldev, selector);
	if (!ur_group) {
		dev_err(pctldev->dev, "Cannot get group by selector %d\n", selector);
		return -EINVAL;
	}

	dev_dbg(pctldev->dev, "get pinconf group %s\n", ur_group->grp.name);
	pin_conf = (struct ur_pin_val *)configs;
	for (int i = 0; i < num_configs; i++) {
		dev_dbg(pctldev->dev, "pinconf[%d], port=%d, pin=%d, conf=0x%x\n",
			i, pin_conf[i].port, pin_conf[i].pin, pin_conf[i].conf);
		ur_set_pin_conf(ur_pinctrl, &pin_conf[i]);
	}
	return 0;
}

static const struct pinconf_ops ur_pinconf_ops = {
	.pin_config_get = ur_pin_config_get,
	.pin_config_set = ur_pin_config_set,
	.pin_config_group_get = ur_pin_config_group_get,
	.pin_config_group_set = ur_pin_config_group_set,
#ifdef CONFIG_GENERIC_PINCONF
	.is_generic = true,
#endif
};

int ur_pinctrl_probe(struct platform_device *pdev)
{
	struct pinctrl_desc *ur_pinctrl_desc;
	const struct ur_pinctrl_match_data *pins_data;
	struct ur_pinctrl *ur_pinctrl;
	int ret;

	pins_data = of_device_get_match_data(&pdev->dev);
	if (!pins_data)
		return -ENODEV;

	ur_pinctrl_desc = devm_kzalloc(&pdev->dev, sizeof(*ur_pinctrl_desc), GFP_KERNEL);
	if (!ur_pinctrl_desc) {
		dev_err(&pdev->dev, "pinctrl desc alloc failed\n");
		return -ENOMEM;
	}

	ur_pinctrl = devm_kzalloc(&pdev->dev, sizeof(*ur_pinctrl), GFP_KERNEL);
	if (!ur_pinctrl) {
		dev_err(&pdev->dev, "pinctrl alloc failed\n");
		ret = -ENOMEM;
		goto free_pinctrl_desc;
	}
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev_dbg(&pdev->dev, "iomem start=0x%llx\n", res->start);
	ur_pinctrl->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(ur_pinctrl->base)) {
		dev_err(&pdev->dev, "get ioremap resource failed\n");
		ret = -EINVAL;
		goto free_pinctrl_desc;
	}
	dev_dbg(&pdev->dev, "pinctrl base=0x%p\n", ur_pinctrl->base);
	ur_pinctrl_desc->name = dev_name(&pdev->dev);
	ur_pinctrl_desc->owner = THIS_MODULE;
	ur_pinctrl_desc->pins = pins_data->pins;
	ur_pinctrl_desc->npins = pins_data->npins;
	ur_pinctrl_desc->pctlops = &ur_pinctrl_ops;
	ur_pinctrl_desc->pmxops = &ur_pinmux_ops;
	ur_pinctrl_desc->confops = &ur_pinconf_ops;

	ur_pinctrl->dev = &pdev->dev;
	ur_pinctrl->match_data = pins_data;
	ur_pinctrl->pctl_desc = ur_pinctrl_desc;
	raw_spin_lock_init(&ur_pinctrl->lock);
	mutex_init(&ur_pinctrl->mutex);
	
	ret = devm_pinctrl_register_and_init(&pdev->dev, ur_pinctrl_desc,
			ur_pinctrl, &ur_pinctrl->pctl_dev);
	if (ret) {
		dev_err(&pdev->dev, "pinctrl register failed\n");
		goto free_pinctrl;
	}

	platform_set_drvdata(pdev, ur_pinctrl);

	return pinctrl_enable(ur_pinctrl->pctl_dev);

free_pinctrl:
	devm_kfree(&pdev->dev, ur_pinctrl);
free_pinctrl_desc:
	devm_kfree(&pdev->dev, ur_pinctrl_desc);
	return ret;
}


void ur_pinctrl_remove(struct platform_device *pdev)
{
	struct ur_pinctrl *ur_pinctrl = platform_get_drvdata(pdev);

	if (ur_pinctrl->pctl_dev)
		devm_pinctrl_unregister(&pdev->dev, ur_pinctrl->pctl_dev);
}
