/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include "mwv207.h"
#include "mwv207_irq.h"

static irqreturn_t mwv207_isr(int irq_unused, void *dev_id)
{
	struct mwv207_device *jdev = dev_id;
	int i, j, hwirq, virq, ret = IRQ_NONE;
	u32 stat;

	for (i = 0; i < 2; ++i) {
		stat = jdev_read(jdev, (0x009A802c) + i * 4);
		stat &= jdev->irq_enable_reg[i];
		while ((j = ffs(stat))) {
			j--;
			hwirq = i * 32 + j;
			virq = irq_find_mapping(jdev->irq_domain, hwirq);
			if (virq) {
				ret = generic_handle_irq(virq);
				if (ret < 0)
					pr_warn("mwv207: hwirq(%d) handled with %d", hwirq, ret);
			} else
				pr_warn("mwv207: no irq mapping set on %d", hwirq);

			jdev_write(jdev, (0x009A802c) + i * 4, (1UL << j));
			stat &= ~(1UL << j);
			ret = IRQ_HANDLED;
		}
	}

	return ret;
}

static void mwv207_irq_mask(struct irq_data *d)
{
	struct mwv207_device *jdev = irq_data_get_irq_chip_data(d);
	int irq = irqd_to_hwirq(d);
	unsigned long flags;
	u32 reg;

	reg = (0x009A8020) + (irq / 32) * 4;

	spin_lock_irqsave(&jdev->irq_lock, flags);
	jdev_modify(jdev, reg, 1 << (irq % 32), 0);
	jdev->irq_enable_reg[irq / 32] = jdev_read(jdev, reg);
	spin_unlock_irqrestore(&jdev->irq_lock, flags);
}

static void mwv207_irq_unmask(struct irq_data *d)
{
	struct mwv207_device *jdev = irq_data_get_irq_chip_data(d);
	int irq = irqd_to_hwirq(d);
	unsigned long flags;
	u32 reg;

	reg = (0x009A8020) + (irq / 32) * 4;

	spin_lock_irqsave(&jdev->irq_lock, flags);
	jdev_modify(jdev, reg, 1 << (irq % 32), 1 << (irq % 32));
	jdev->irq_enable_reg[irq / 32] = jdev_read(jdev, reg);
	spin_unlock_irqrestore(&jdev->irq_lock, flags);
}

void mwv207_irq_suspend(struct mwv207_device *jdev)
{
	int i;

	for (i = 0; i < 2; ++i)
		jdev->irq_enable_reg[i] = jdev_read(jdev,
				(0x009A8020) + i * 4);
}

void mwv207_irq_resume(struct mwv207_device *jdev)
{
	int i;

	for (i = 0; i < 2; ++i) {
		jdev_write(jdev, (0x009A802c) + i * 4, 0xffffffff);
		jdev_write(jdev, (0x009A8020) + i * 4,
				jdev->irq_enable_reg[i]);
	}
}

static struct irq_chip mwv207_irq_chip = {
	.name		= "mwv207",
	.irq_mask	= mwv207_irq_mask,
	.irq_unmask	= mwv207_irq_unmask,
};

static int mwv207_irq_domain_map(struct irq_domain *d, unsigned int irq,
				 irq_hw_number_t hwirq)
{
	struct mwv207_device *jdev = d->host_data;

	if (hwirq >= 64)
		return -EPERM;

	irq_set_chip_and_handler(irq, &mwv207_irq_chip, handle_simple_irq);
	irq_set_chip_data(irq, jdev);

	return 0;
}

static const struct irq_domain_ops mwv207_irq_domain_ops = {
	.map = mwv207_irq_domain_map,
};

int mwv207_irq_init(struct mwv207_device *jdev)
{
	struct pci_dev *pdev = jdev->pdev;
	int ret, i;

	for (i = 0; i < 2; ++i) {
		jdev_write(jdev, (0x009A802c) + i * 4, 0xffffffff);
		jdev_write(jdev, (0x009A8020) + i * 4, 0);
	}

	spin_lock_init(&jdev->irq_lock);

	jdev->irq_domain = irq_domain_add_linear(NULL, 64,
			&mwv207_irq_domain_ops, jdev);
	if (!jdev->irq_domain)
		return -ENODEV;

	for (i = 0; i < 64; ++i)
		irq_create_mapping(jdev->irq_domain, i);

	ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSI | PCI_IRQ_LEGACY);
	if (ret < 1) {
		ret = -ENODEV;
		goto free_mapping;
	}

	ret = request_irq(pdev->irq, mwv207_isr, IRQF_SHARED, "mwv207_isr", jdev);
	if (ret)
		goto free_vec;

	return 0;

free_vec:
	pci_free_irq_vectors(pdev);
free_mapping:
	for (i = 0; i < 64; ++i) {
		int irq = irq_find_mapping(jdev->irq_domain, i);

		irq_dispose_mapping(irq);
	}

	irq_domain_remove(jdev->irq_domain);

	return ret;
}

void mwv207_irq_fini(struct mwv207_device *jdev)
{
	struct pci_dev *pdev = jdev->pdev;
	int i;

	free_irq(pdev->irq, jdev);
	pci_free_irq_vectors(pdev);

	for (i = 0; i < 64; ++i) {
		int irq = irq_find_mapping(jdev->irq_domain, i);

		irq_dispose_mapping(irq);
	}

	irq_domain_remove(jdev->irq_domain);
}
