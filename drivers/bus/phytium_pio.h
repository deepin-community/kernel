/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * bus/phytium_pio.h
 *
 * Copyright (C) 2021-2025, Phytium Technology Co., Ltd.
 */

#ifndef _PHYTIUM_PIO_H_
#define _PHYTIUM_PIO_H_

#include <linux/io.h>
#include <linux/logic_pio.h>

#define PHYTIUM_PIO_LENGTH_LONG	0x8000000
#define FT3000_PIO_INT_STATE	0x7FFFFC4
#define FT3000_PIO_CLR_INT	0x7FFFFC0
#define FT2000_PIO_INT_STATE	0x7FFFFF4
#define FT2000_PIO_CLR_INT	0x7FFFFF0
#define LEGACY_ISA_SIZE		0x400

struct phytium_pio {
	void __iomem *membase;
	int irq;
	spinlock_t lock;
	u32 int_status_reg;
	u32 int_clear_reg;
	struct logic_pio_hwaddr *range;
};

bool check_cpu_type(void);
int phytium_pio_get_irq(void);
void phytium_pio_clear_interrupt(u32 val);
u32 phytium_pio_get_int_status(void);

#endif


