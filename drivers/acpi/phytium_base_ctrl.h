/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * acpi/phytium_base_ctrl.h
 *
 * Copyright (C) 2021-2024, Phytium Technology Co., Ltd.
 */

#define	base_ctrl_INT_STATE	0x7FFFFC4
#define	base_ctrl_CLR_INT	0x7FFFFC0

struct phytium_base_ctrl {
	struct device *dev;
	void __iomem *base;
	int irq;
	spinlock_t lock;
	u32 int_status_reg;
	u32 int_clear_reg;
};

int phytium_base_ctrl_irq(void);
u8 base_ctrl_readb(unsigned long offset);
u32 base_ctrl_readl(unsigned long offset);
bool phytium_check_cpu(void);
int base_ctrl_writeb(unsigned long offset, u8 value);
int base_ctrl_writel(unsigned long offset, u32 value);
int base_ctrl_read_int_status(void);
void base_ctrl_write_int_clear(int val);
