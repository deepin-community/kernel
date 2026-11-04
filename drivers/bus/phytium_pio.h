/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * bus/phytium_pio.h
 *
 * Copyright (C) 2021-2025, Phytium Technology Co., Ltd.
 */

#ifndef _PHYTIUM_PIO_H_
#define _PHYTIUM_PIO_H_

#include <linux/io.h>
#include <linux/irqdomain.h>
#include <linux/logic_pio.h>

#define PHYTIUM_PIO_LENGTH_LONG	0x8000000
#define FT3000_PIO_INT_STATE	0x7FFFFC4
#define FT3000_PIO_CLR_INT	0x7FFFFC0
#define FT2000_PIO_INT_STATE	0x7FFFFF4
#define FT2000_PIO_CLR_INT	0x7FFFFF0
/* Base of the SoC window holding the interrupt latch registers. */
#define PHYTIUM_PIO_D2000_BASE		0x20000000
#define PHYTIUM_PIO_D3000M_BASE		0x10000000

#define PHYTIUM_PIO_IRQ_NR		32
#define PHYTIUM_PIO_STUCK_LIMIT		1000

/* LPC interrupt latch bits, one Linux irq each. */
#define PHYTIUM_PIO_I8042_KBD_INT	1
#define PHYTIUM_PIO_UART_INT		4
#define PHYTIUM_PIO_EC_INT		11
#define PHYTIUM_PIO_I8042_AUX_INT	12

struct phytium_pio {
	/* Firmware window: indirect PIO data accesses are decoded here. */
	void __iomem *membase;
	/* Long LPC window: holds the interrupt latch registers. */
	void __iomem *regbase;
	int irq;
	spinlock_t lock;
	u32 int_status_reg;
	u32 int_clear_reg;
	struct irq_domain *domain;
	u32 stuck_acks;
	/* Physical start of the firmware window, see phytium_pio_is_lpc_mem(). */
	phys_addr_t win_base;
	struct logic_pio_hwaddr *range;
};

bool check_cpu_type(void);
int phytium_pio_irq_get(unsigned int bit);
bool phytium_pio_is_lpc_mem(phys_addr_t addr);

#endif


