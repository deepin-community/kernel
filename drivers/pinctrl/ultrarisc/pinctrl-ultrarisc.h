// SPDX-License-Identifier: GPL-2.0
/* UltraRisc pinctrl driver
 *
 * Copyright(C) 2025 UltraRisc Technology Co., Ltd.
 *
 *  Author:  wangjia <wangjia@ultrarisc.com>
 */

#ifndef __PINCTRL_ULTRARISC_H__
#define __PINCTRL_ULTRARISC_H__

#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf-generic.h>

#define PINMUX_PROP_NAME	"pinctrl-pins"
#define PINCONF_PROP_NAME	"pinconf-pins"

struct ur_pin_conf {
	u16 pull;
	u16 drive;
};

struct ur_pin_val {
	u32 port;
	u32 pin;
	union {
		u32 mode;
		u32 conf;
	};
#define UR_FUNC_DEF			0
#define UR_FUNC0			1
#define UR_FUNC1			0x10000

#define UR_BIAS_MASK		0x0000000F
#define UR_PULL_MASK		0x0C
#define UR_PULL_DIS		0
#define UR_PULL_UP		1
#define UR_PULL_DOWN		2
#define UR_DRIVE_MASK		0x03
};

struct ur_port_desc {
	char *name;
	u32 npins;
	u32 func_offset;
	u32 conf_offset;
};

struct ur_pinctrl_match_data {
	const struct pinctrl_pin_desc *pins;
	u32 npins;
	u32 offset;
	//u32 conf_offset[];
	struct ur_port_desc ports[];
};


struct ur_pinctrl {
	struct device *dev;
	struct pinctrl_dev *pctl_dev;
	struct pinctrl_desc *pctl_desc;
	void __iomem *base;
	unsigned int ngroups;
	const char **grp_names;
	unsigned int nbanks;
	const struct ur_pinctrl_match_data *match_data;
	struct regmap		*regmap;
	raw_spinlock_t lock;
	struct mutex mutex;
	struct pinctrl_pin_desc *pins;
	u32 npins;
	u32 pkg;
};

int ur_pinctrl_probe(struct platform_device *pdev);
void ur_pinctrl_remove(struct platform_device *pdev);

#endif
