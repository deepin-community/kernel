/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2015 Weiqiang Su <David.suwq@gmail.com>
 *
 * Both AST2400D and AST2400F package variants are supported.
 */

#ifndef _ASM_SW64_AST2400_H
#define _ASM_SW64_AST2400_H

#include <linux/device.h>

/* Logical Device Numbers (LDN). */
#define AST2400_FDC		0x00 /* Floppy */
#define AST2400_PP		0x01 /* Parallel port */
#define AST2400_SP1		0x02 /* Com1 */
#define AST2400_SP2		0x03 /* Com2 & IR */
#define AST2400_KBC		0x05 /* PS/2 keyboard and mouse */
#define AST2400_CIR		0x06
#define AST2400_GPIO6789_V	0x07
#define AST2400_WDT1_GPIO01A_V	0x08
#define AST2400_GPIO1234567_V	0x09
#define AST2400_ACPI		0x0A
#define AST2400_HWM_FPLED	0x0B /* Hardware monitor & front LED */
#define AST2400_VID		0x0D
#define AST2400_CIRWKUP		0x0E /* CIR wakeup */
#define AST2400_GPIO_PP_OD	0x0F /* GPIO Push-Pull/Open drain select */
#define AST2400_SVID		0x14
#define AST2400_DSLP		0x16 /* Deep sleep */
#define AST2400_GPIOA_LDN	0x17

/* virtual LDN for GPIO and WDT */
#define AST2400_WDT1		((0 << 8) | AST2400_WDT1_GPIO01A_V)

#define AST2400_GPIOBASE	((0 << 8) | AST2400_WDT1_GPIO01A_V) //?

#define AST2400_GPIO0		((1 << 8) | AST2400_WDT1_GPIO01A_V)
#define AST2400_GPIO1		((1 << 8) | AST2400_GPIO1234567_V)
#define AST2400_GPIO2		((2 << 8) | AST2400_GPIO1234567_V)
#define AST2400_GPIO3		((3 << 8) | AST2400_GPIO1234567_V)
#define AST2400_GPIO4		((4 << 8) | AST2400_GPIO1234567_V)
#define AST2400_GPIO5		((5 << 8) | AST2400_GPIO1234567_V)
#define AST2400_GPIO6		((6 << 8) | AST2400_GPIO1234567_V)
#define AST2400_GPIO7		((7 << 8) | AST2400_GPIO1234567_V)
#define AST2400_GPIO8		((0 << 8) | AST2400_GPIO6789_V)
#define AST2400_GPIO9		((1 << 8) | AST2400_GPIO6789_V)
#define AST2400_GPIOA		((2 << 8) | AST2400_WDT1_GPIO01A_V)

#define SUPERIO_PNP_PORT	0x2E
#define SUPERIO_CHIPID		0xC333

#ifdef CONFIG_SUBARCH_C3B
#define PORT_OFFSET		0
#endif

#ifdef CONFIG_SUBARCH_C4
#define PORT_OFFSET		12
#endif

struct device_operations;
typedef struct pnp_device {
	unsigned long port;
	unsigned int device;

	struct device_operations *ops;
} *device_t;

struct pnp_mode_ops {
	void (*enter_conf_mode)(device_t dev);
	void (*exit_conf_mode)(device_t dev);
};


struct device_operations {
	void (*read_resources)(device_t dev);
	void (*set_resources)(device_t dev);
	void (*enable_resources)(device_t dev);
	void (*init)(device_t dev);
	void (*final)(device_t dev);
	void (*enable)(device_t dev);
	void (*disable)(device_t dev);

	const struct pnp_mode_ops *ops_pnp_mode;
};

/* PNP helper operations */
struct io_info {
	unsigned int mask, set;
};

struct pnp_info {
	bool enabled;		/* set if we should enable the device */
	struct pnp_device pnp_device;
	unsigned int function;	/* Must be at least 16 bits (virtual LDNs)! */
};

/* Chip operations */
struct chip_operations {
	void (*enable_dev)(struct device *dev);
	void (*init)(void *chip_info);
	void (*final)(void *chip_info);
	unsigned int initialized : 1;
	unsigned int finalized : 1;
	const char *name;
};

typedef struct superio_ast2400_device {
	struct device	*dev;
	const char	*name;
	unsigned int	enabled : 1;		/* set if we should enable the device */
	unsigned long	superio_ast2400_efir;	/* extended function index register */
	unsigned long	superio_ast2400_efdr;	/* extended function data register */
	struct chip_operations *chip_ops;
	const void	*chip_info;
} *superio_device_t;


static inline void pnp_enter_conf_mode_a5a5(device_t dev)
{
	outb(0xa5, dev->port);
	outb(0xa5, dev->port);
}

static inline void pnp_exit_conf_mode_aa(device_t dev)
{
	outb(0xaa, dev->port);
}

/* PNP config mode wrappers */

static inline void pnp_enter_conf_mode(device_t dev)
{
	if (dev->ops->ops_pnp_mode)
		dev->ops->ops_pnp_mode->enter_conf_mode(dev);
}

static inline void pnp_exit_conf_mode(device_t dev)
{
	if (dev->ops->ops_pnp_mode)
		dev->ops->ops_pnp_mode->exit_conf_mode(dev);
}

/* PNP device operations */
static inline u8 pnp_read_config(device_t dev, u8 reg)
{
	outb(reg, dev->port);
	return inb(dev->port + (1 << PORT_OFFSET));
}

static inline void pnp_write_config(device_t dev, u8 reg, u8 value)
{
	outb(reg, dev->port);
	outb(value, dev->port + (1 << PORT_OFFSET));
}

static inline void pnp_set_logical_device(device_t dev)
{
	pnp_write_config(dev, 0x07, dev->device & 0xff);
}

static inline void pnp_set_enable(device_t dev, int enable)
{
	u8 tmp;

	tmp = pnp_read_config(dev, 0x30);

	if (enable)
		tmp |= 1;
	else
		tmp &= ~1;

	pnp_write_config(dev, 0x30, tmp);
}

#endif /* _ASM_SW64_AST2400_H */
