/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * i8042-phytio.h - i8042 driver platform specific header for Phytium
 *
 */
#ifndef _I8042_PHYTIO_H
#define _I8042_PHYTIO_H

#include <linux/acpi.h>
#include <linux/classtypes.h>
#include <linux/dmi.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/logic_pio.h>
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
 * Boards that publish a KBCI8042 node but leave nothing behind the legacy
 * ports: a read of 0x60/0x64 never completes, so i8042_probe() hangs.
 */
static const struct dmi_system_id phytium_i8042_skip[] = {
	{
		.matches = {
			DMI_MATCH(DMI_BOARD_NAME, "FD2000ZX200MB1"),
		},
	},
	/* 706 laptops, as listed by the Kylin LPC driver. */
	{
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "HT706"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TR4251"),
		},
	},
	{
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "HT706"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TR4252"),
		},
	},
	{
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "HT706"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TR4260"),
		},
	},
	{
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "HT706"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TR4261"),
		},
	},
	{
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "HT706"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TR4263"),
		},
	},
	{ }
};

static inline bool phytium_i8042_acpi_described(void)
{
	return IS_ENABLED(CONFIG_ACPI) &&
	       acpi_dev_present("KBCI8042", NULL, -1);
}

#ifdef CONFIG_ACPI
static int phytium_i8042_io_cb(struct acpi_resource *ares, void *data)
{
	bool *found = data;

	if (ares->type == ACPI_RESOURCE_TYPE_IO &&
	    ares->data.io.minimum <= 0x60 && ares->data.io.maximum >= 0x64)
		*found = true;

	return 1;
}

/* Does _CRS describe the controller's own ports? */
static inline bool phytium_i8042_acpi_has_ports(void)
{
	struct acpi_device *adev;
	LIST_HEAD(res);
	bool found = false;

	adev = acpi_dev_get_first_match_dev("KBCI8042", NULL, -1);
	if (!adev)
		return false;

	acpi_dev_get_resources(adev, &res, phytium_i8042_io_cb, &found);
	acpi_dev_free_resource_list(&res);
	acpi_dev_put(adev);

	return found;
}
#else
static inline bool phytium_i8042_acpi_has_ports(void)
{
	return true;
}
#endif

/*
 * Platform initialization
 */
static inline int i8042_platform_init(void)
{
	bool described = phytium_i8042_acpi_described();
	bool laptop;
	int kbd_irq, aux_irq;

	/* Check if we're running on Phytium CPU */
	if (!check_cpu_type()) {
		pr_info("Not running on Phytium CPU\n");
		return -ENODEV;
	}

	if (dmi_check_system(phytium_i8042_skip)) {
		pr_info("KBC does not answer on this board, skipping\n");
		return -ENODEV;
	}

	/*
	 * Probe like before unless the firmware documents the controller
	 * without its ports.  Laptops keep probing: an internal keyboard may
	 * be the only input.
	 */
	laptop = !IS_ENABLED(CONFIG_DMI) || chassis_types_is_laptop();
	if (described && !laptop && !phytium_i8042_acpi_has_ports()) {
		pr_info("KBC has no ports described, skipping\n");
		return -ENODEV;
	}

	/*
	 * The PIO controller hands out one Linux irq per latch bit and clears
	 * the latch itself, so the irq is not shared with the other legacy
	 * devices.
	 */
	kbd_irq = phytium_pio_irq_get(PHYTIUM_PIO_I8042_KBD_INT);
	aux_irq = phytium_pio_irq_get(PHYTIUM_PIO_I8042_AUX_INT);
	if (kbd_irq < 0 || aux_irq < 0) {
		pr_err("no irq for the KBC\n");
		return -ENODEV;
	}

	i8042_kbd_irq = kbd_irq;
	i8042_aux_irq = aux_irq;

	/* Set reset behavior */
	i8042_reset = I8042_RESET_ALWAYS;

	pr_info("irqs %d/%d\n", kbd_irq, aux_irq);
	return 0;
}

static inline void i8042_platform_exit(void)
{
	pr_info("i8042-phytio: Exited\n");
}

#endif /* _I8042_PHYTIO_H */
