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

/* Long LPC window holding the interrupt latch registers. */
#define PHYTIUM_LPC_IOMEM_SIZE		0x08000000

/* LPC interrupt latch bits shared by the legacy children. */
#define PHYTIUM_PIO_I8042_KBD_INT	1
#define PHYTIUM_PIO_UART_INT		4
#define PHYTIUM_PIO_EC_INT		11
#define PHYTIUM_PIO_I8042_AUX_INT	12

struct phytium_pio {
	void __iomem *membase;
	/* Long LPC window used for interrupt status/clear accesses. */
	void __iomem *regbase;
	int irq;
	spinlock_t lock;
	u32 int_status_reg;
	u32 int_clear_reg;
	/* LPC latch bits owned by a registered child handler. */
	u32 handled_bits;
	phys_addr_t lpc_base;
	struct logic_pio_hwaddr *range;
};

bool check_cpu_type(void);
int phytium_pio_get_irq(void);
void phytium_pio_handle_interrupt_bit(unsigned int bit);
void phytium_pio_unhandle_interrupt_bit(unsigned int bit);
bool phytium_pio_clear_interrupt_bit(unsigned int bit);
bool phytium_pio_is_lpc_mem(phys_addr_t addr);
void phytium_pio_clear_interrupt(u32 val);
u32 phytium_pio_get_int_status(void);

#endif


