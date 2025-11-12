// SPDX-License-Identifier: GPL-2.0
/* UltraRisc DP1000 pinctrl driver
 *
 * Copyright(C) 2025 UltraRisc Technology Co., Ltd.
 *
 *  Author:  wangjia <wangjia@ultrarisc.com>
 */

#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/machine.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include "../pinctrl-utils.h"
#include "../pinmux.h"
#include "../core.h"
#include "../devicetree.h"

#include "pinctrl-ultrarisc.h"

static const struct pinctrl_pin_desc ur_dp1000_pins[] = {
	// PA
	PINCTRL_PIN(0, "PA0"),
	PINCTRL_PIN(1, "PA1"),
	PINCTRL_PIN(2, "PA2"),
	PINCTRL_PIN(3, "PA3"),
	PINCTRL_PIN(4, "PA4"),
	PINCTRL_PIN(5, "PA5"),
	PINCTRL_PIN(6, "PA6"),
	PINCTRL_PIN(7, "PA7"),
	PINCTRL_PIN(8, "PA8"),
	PINCTRL_PIN(9, "PA9"),
	PINCTRL_PIN(10, "PA10"),
	PINCTRL_PIN(11, "PA11"),
	PINCTRL_PIN(12, "PA12"),
	PINCTRL_PIN(13, "PA13"),
	PINCTRL_PIN(14, "PA14"),
	PINCTRL_PIN(15, "PA15"),
	// PB
	PINCTRL_PIN(16, "PB0"),
	PINCTRL_PIN(17, "PB1"),
	PINCTRL_PIN(18, "PB2"),
	PINCTRL_PIN(19, "PB3"),
	PINCTRL_PIN(20, "PB4"),
	PINCTRL_PIN(21, "PB5"),
	PINCTRL_PIN(22, "PB6"),
	PINCTRL_PIN(23, "PB7"),
	// PC
	PINCTRL_PIN(24, "PC0"),
	PINCTRL_PIN(25, "PC1"),
	PINCTRL_PIN(26, "PC2"),
	PINCTRL_PIN(27, "PC3"),
	PINCTRL_PIN(28, "PC4"),
	PINCTRL_PIN(29, "PC5"),
	PINCTRL_PIN(30, "PC6"),
	PINCTRL_PIN(31, "PC7"),
	// PD
	PINCTRL_PIN(32, "PD0"),
	PINCTRL_PIN(33, "PD1"),
	PINCTRL_PIN(34, "PD2"),
	PINCTRL_PIN(35, "PD3"),
	PINCTRL_PIN(36, "PD4"),
	PINCTRL_PIN(37, "PD5"),
	PINCTRL_PIN(38, "PD6"),
	PINCTRL_PIN(39, "PD7"),
	// LPC
	PINCTRL_PIN(40, "LPC0"),
	PINCTRL_PIN(41, "LPC1"),
	PINCTRL_PIN(42, "LPC2"),
	PINCTRL_PIN(43, "LPC3"),
	PINCTRL_PIN(44, "LPC4"),
	PINCTRL_PIN(45, "LPC5"),
	PINCTRL_PIN(46, "LPC6"),
	PINCTRL_PIN(47, "LPC7"),
	PINCTRL_PIN(48, "LPC8"),
	PINCTRL_PIN(49, "LPC9"),
	PINCTRL_PIN(50, "LPC10"),
	PINCTRL_PIN(51, "LPC11"),
	PINCTRL_PIN(52, "LPC12"),
};

static struct ur_pinctrl_match_data ur_dp1000_match_data = {
	.pins = ur_dp1000_pins,
	.npins = ARRAY_SIZE(ur_dp1000_pins),
	.offset = 0x2c0,
	.num_ports = 5,
	.ports = {
		{"A", 16, 0x2c0, 0x310},
		{"B", 8, 0x2c4, 0x318},
		{"C", 8, 0x2c8, 0x31c},
		{"D", 8, 0x2cc, 0x320},
		{"LPC", 13, 0x2d0, 0x324},
	},
};

enum ur_dp1000_port_list {
	PORT_A = 0,
	PORT_B,
	PORT_C,
	PORT_D,
	PORT_LPC
};


static const struct of_device_id ur_pinctrl_of_match[] = {
	{ .compatible = "ultrarisc,dp1000-pinctrl", .data = &ur_dp1000_match_data, },
	{ }
};
MODULE_DEVICE_TABLE(of, ur_pinctrl_of_match);

static struct platform_driver ur_pinctrl_driver = {
	.driver = {
		.name = "ultrarisc-pinctrl-dp1000",
		.of_match_table = ur_pinctrl_of_match,
	},
	.probe = ur_pinctrl_probe,
	.remove = ur_pinctrl_remove,
};

module_platform_driver(ur_pinctrl_driver);
