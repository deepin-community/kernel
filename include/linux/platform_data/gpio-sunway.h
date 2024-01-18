/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(c) 2014 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#ifndef GPIO_SUNWAY_H
#define GPIO_SUNWAY_H

struct sunway_port_property {
	struct fwnode_handle *fwnode;
	unsigned int	idx;
	unsigned int	ngpio;
	unsigned int	gpio_base;
	int		irq[32];
	bool		has_irq;
	bool		irq_shared;
};

struct sunway_platform_data {
	struct sunway_port_property *properties;
	unsigned int nports;
};

#endif
