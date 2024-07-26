// SPDX-License-Identifier: GPL-2.0
/*
 * linux/drivers/mfd/sunway_ast2400.c
 *
 * Copyright (C) 20014 - 2015 JN
 * Author: Weiqiang Su <David.suwq@gmail.com>
 *
 * Nuvoton AST2400 Super I/O chip platform driver written for
 * SUNWAY LPC controller.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/ast2400.h>

#define DRIVER_NAME "sunway_superio_ast2400"

static int superio_uart0_irq;
static int superio_uart1_irq;
static void pnp_enable(device_t dev)
{
	pnp_enter_conf_mode(dev);
	pnp_set_logical_device(dev);
	pnp_set_enable(dev, 1);
	pnp_exit_conf_mode(dev);
}

const struct pnp_mode_ops pnp_conf_mode_8787_aa = {
	.enter_conf_mode = pnp_enter_conf_mode_a5a5,
	.exit_conf_mode  = pnp_exit_conf_mode_aa,
};

static struct device_operations ops = {
	.enable           = pnp_enable,
	.ops_pnp_mode     = &pnp_conf_mode_8787_aa,
};

static struct pnp_info pnp_dev_info[] = {
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_FDC},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_PP },
	{ true,	 {SUPERIO_PNP_PORT, 0, &ops}, AST2400_SP1},
	{ true,  {SUPERIO_PNP_PORT, 0, &ops}, AST2400_SP2},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_KBC},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_CIR},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_ACPI},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_HWM_FPLED},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_VID},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_CIRWKUP},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIO_PP_OD},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_SVID},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_DSLP},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIOA_LDN},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_WDT1},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIOBASE},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIO0},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIO1},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIO2},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIO3},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIO4},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIO5},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIO6},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIO7},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIO8},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIO9},
	{ false, {SUPERIO_PNP_PORT, 0, &ops}, AST2400_GPIOA},
};

static void superio_com1_init(struct pnp_device *device)
{
	pnp_enter_conf_mode(device);
	pnp_set_logical_device(device);
	pnp_set_enable(device, 1);

	pnp_write_config(device, 0x60, 0x2);
	pnp_write_config(device, 0x61, 0xf8);

	pnp_write_config(device, 0x70, superio_uart0_irq);
	pnp_write_config(device, 0x71, 0x1);

	pnp_write_config(device, 0xf0, 0x0);

	pnp_exit_conf_mode(device);
}

static void superio_com2_init(struct pnp_device *device)
{
	pnp_enter_conf_mode(device);
	pnp_set_logical_device(device);
	pnp_set_enable(device, 1);

	pnp_write_config(device, 0x60, 0x3);
	pnp_write_config(device, 0x61, 0xf8);

	pnp_write_config(device, 0x70, superio_uart1_irq);
	pnp_write_config(device, 0x71, 0x1);

	pnp_write_config(device, 0xf0, 0x0);

	pnp_exit_conf_mode(device);
}

static void pnp_enable_devices(superio_device_t superio_device,
		struct device_operations *ops,
		unsigned int functions, struct pnp_info *info)
{
	int i = 0;
	struct pnp_info *each_info;
	struct pnp_device *each_device;

	/* Setup the ops and resources on the newly allocated devices. */
	for (i = 0; i < functions; i++) {
		each_info = info + i;
		each_device = &each_info->pnp_device;

		/* Skip logical devices this Super I/O doesn't enable. */
		if (each_info->enabled == false)
			continue;

		each_device->device = each_info->function;
		each_device->ops = ops;
		each_device->port = superio_device->superio_ast2400_efir;

		switch (each_device->device) {
		case AST2400_SP1:
			each_device->ops->init = superio_com1_init;
			break;
		case AST2400_SP2:
			each_device->ops->init = superio_com2_init;
			break;
		}

		if (each_device->ops->init)
			each_device->ops->init(each_device);
	}
}

static void superio_enable_devices(superio_device_t superio_device)
{
	pnp_enable_devices(superio_device, &ops,
			ARRAY_SIZE(pnp_dev_info), pnp_dev_info);
}

static int superio_ast2400_probe(struct platform_device *pdev)
{
	int err = 0;
	superio_device_t superio_device;
	struct resource *res;
	resource_size_t physaddr = 0;

	/* allocate space for device info */
	superio_device = kzalloc(sizeof(struct superio_ast2400_device), GFP_KERNEL);
	if (superio_device == NULL) {
		err = -ENOMEM;
		return err;
	}

	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (res) {
		physaddr = res->start;
		dev_info(&pdev->dev, "request memory region %pR\n", res);
	}

	superio_device->dev = &pdev->dev;
	superio_device->enabled = 1;
	superio_device->superio_ast2400_efir = physaddr + (SUPERIO_PNP_PORT << PORT_OFFSET);
	superio_device->superio_ast2400_efdr = physaddr + ((SUPERIO_PNP_PORT + 1) << PORT_OFFSET);
	superio_uart0_irq = platform_get_irq_byname(pdev, "uart0_irq");
	superio_uart1_irq = platform_get_irq_byname(pdev, "uart1_irq");

	superio_enable_devices(superio_device);

	platform_set_drvdata(pdev, superio_device);

	dev_info(superio_device->dev, "probe succeed !\n");

	return 0;
}

static int superio_ast2400_remove(struct platform_device *pdev)
{
	superio_device_t superio_device = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);

	kfree(superio_device);

	return 0;
}

static struct platform_driver superio_nuvoton_ast2400_driver = {
	.probe          = superio_ast2400_probe,
	.remove         = superio_ast2400_remove,
	.driver         = {
		.name   = DRIVER_NAME
	},
};

static int __init superio_nuvoton_ast2400_init(void)
{
	return platform_driver_register(&superio_nuvoton_ast2400_driver);
}

subsys_initcall_sync(superio_nuvoton_ast2400_init);

static void __exit superio_nuvoton_ast2400_exit(void)
{
	platform_driver_unregister(&superio_nuvoton_ast2400_driver);
}

module_exit(superio_nuvoton_ast2400_exit);

MODULE_DESCRIPTION("NUVOTON AST2400 Super I/O DRIVER");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("Weiqiang Su");
