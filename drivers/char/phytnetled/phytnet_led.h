/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2022-2023 Phytium Technology Co.,Ltd.
 *
 */
struct led_data {
	struct platform_device *pdev;
	struct net_device *ndev;
	unsigned long ndev_rx, ndev_tx;
	struct gpio_desc *link, *act;
	struct delayed_work led_control_work;
	int led_stop;
	int act_val;
};
