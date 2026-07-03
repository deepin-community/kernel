/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * i8042-phytio.h - i8042 driver platform specific header for Phytium
 *
 */
#ifndef _I8042_PHYTIO_H
#define _I8042_PHYTIO_H

#include <linux/io.h>
#include <linux/logic_pio.h>
#include <linux/errno.h>
#include <linux/printk.h>

#include "../../bus/phytium_pio.h"

/*
 * Names.
 */
#define I8042_KBD_PHYS_DESC "phytium-pio/serio0"
#define I8042_AUX_PHYS_DESC "phytium-pio/serio1"
#define I8042_MUX_PHYS_DESC "phytium-pio/serio%d"

/*
 * IRQs.
 */
static int i8042_kbd_irq = 0x41;
static int i8042_aux_irq = 0x4c;
#define I8042_KBD_IRQ   i8042_kbd_irq
#define I8042_AUX_IRQ	i8042_aux_irq

/*
 * Register numbers - standard i8042 registers
 */
#define I8042_COMMAND_REG	0x64
#define I8042_STATUS_REG	0x64
#define I8042_DATA_REG		0x60

#define EC_EVENT_BIT		(1 << 11)
#define I8042_KEY_BIT		(1 << 1)
#define I8042_TOUCH_BIT		(1 << 12)

/*
 * i8042 IO operations using Logic PIO framework
 * The Logic PIO framework will automatically call Phytium PIO's
 * hardware access functions
 */
static inline int i8042_read_data(void)
{
	return inb(I8042_DATA_REG);
}

static inline int i8042_read_status(void)
{
	return inb(I8042_STATUS_REG);
}

static inline void i8042_write_data(int val)
{
	outb(val, I8042_DATA_REG);
}

static inline void i8042_write_command(int val)
{
	outb(val, I8042_COMMAND_REG);
}

/*
 * Platform initialization
 */
static inline int i8042_platform_init(void)
{
	/* Check if we're running on Phytium CPU */
	if (!check_cpu_type()) {
		pr_info("i8042-phytio: Not running on Phytium CPU\n");
		return -ENODEV;
	}

	i8042_kbd_irq = i8042_aux_irq = phytium_pio_get_irq();

	/* Set reset behavior */
	i8042_reset = I8042_RESET_ALWAYS;

	pr_info("i8042-phytio: Initialized on Phytium platform\n");
	return 0;
}

static inline void i8042_platform_exit(void)
{
	phytium_pio_clear_interrupt(0);
	pr_info("i8042-phytio: Exited\n");
}

static inline int i8042_write_lpc_interrupt_clear(void)
{
	u32 int_status;

	int_status = phytium_pio_get_int_status();

	if (int_status & I8042_KEY_BIT) {
		phytium_pio_clear_interrupt(~I8042_KEY_BIT);
		return 0;
	}
	if (int_status & I8042_TOUCH_BIT) {
		phytium_pio_clear_interrupt(~I8042_TOUCH_BIT);
		return 0;
	}
	if (int_status & EC_EVENT_BIT) {
		phytium_pio_clear_interrupt(~EC_EVENT_BIT);
		return 0;
	}

	pr_debug("no cared bits\n");
	return 1;
}

#endif /* _I8042_PHYTIO_H */
