// SPDX-License-Identifier: GPL-2.0-only
/*
 * HYGON Secure Processor interface driver
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sp-dev.h"

#ifdef CONFIG_CRYPTO_DEV_SP_PSP
static const struct sev_vdata csvv1 = {
	.cmdresp_reg		= 0x10580,	/* C2PMSG_32 */
	.cmdbuff_addr_lo_reg	= 0x105e0,	/* C2PMSG_56 */
	.cmdbuff_addr_hi_reg	= 0x105e4,	/* C2PMSG_57 */
};

static const struct psp_vdata pspv1 = {
	.sev			= &csvv1,
	.feature_reg		= 0x105fc,	/* C2PMSG_63 */
	.inten_reg		= 0x10610,	/* P2CMSG_INTEN */
	.intsts_reg		= 0x10614,	/* P2CMSG_INTSTS */
#ifdef CONFIG_HYGON_PSP2CPU_CMD
	.p2c_cmdresp_reg	= 0x105e8,
	.p2c_cmdbuff_addr_lo_reg = 0x105ec,
	.p2c_cmdbuff_addr_hi_reg = 0x105f0,
#endif
};

static const struct psp_vdata pspv2 = {
	.sev			= &csvv1,
	.feature_reg		= 0x105fc,
	.inten_reg		= 0x10670,
	.intsts_reg		= 0x10674,
#ifdef CONFIG_HYGON_PSP2CPU_CMD
	.p2c_cmdresp_reg        = 0x105e8,
	.p2c_cmdbuff_addr_lo_reg = 0x105ec,
	.p2c_cmdbuff_addr_hi_reg = 0x105f0,
#endif
};

#endif

const struct sp_dev_vdata hygon_dev_vdata[] = {
	{	/* 0 */
		.bar = 2,
#ifdef CONFIG_CRYPTO_DEV_SP_CCP
		.ccp_vdata = &ccpv5a_hygon,
#endif
#ifdef CONFIG_CRYPTO_DEV_SP_PSP
		.psp_vdata = &pspv1,
#endif
	},
	{	/* 1 */
		.bar = 2,
#ifdef CONFIG_CRYPTO_DEV_SP_CCP
		.ccp_vdata = &ccpv5b_hygon,
#endif
	},
	{	/* 2 */
		.bar = 2,
#ifdef CONFIG_CRYPTO_DEV_SP_CCP
		.ccp_vdata = &ccpv5a_hygon,
#endif
#ifdef CONFIG_CRYPTO_DEV_SP_PSP
		.psp_vdata = &pspv2,
#endif
	},
};

#ifdef CONFIG_PM_SLEEP

static int hygon_sp_pci_suspend(struct device *dev)
{
	struct sp_device *sp = dev_get_drvdata(dev);

	return hygon_sp_suspend(sp);
}

static int hygon_sp_pci_resume(struct device *dev)
{
	struct sp_device *sp = dev_get_drvdata(dev);

	return hygon_sp_resume(sp);
}

static int hygon_sp_pci_freeze(struct device *dev)
{
	struct sp_device *sp = dev_get_drvdata(dev);

	return hygon_sp_freeze(sp);
}

static int hygon_sp_pci_thaw(struct device *dev)
{
	struct sp_device *sp = dev_get_drvdata(dev);

	return hygon_sp_thaw(sp);
}

static int hygon_sp_pci_poweroff(struct device *dev)
{
	struct sp_device *sp = dev_get_drvdata(dev);

	return hygon_sp_poweroff(sp);
}

static int hygon_sp_pci_restore(struct device *dev)
{
	struct sp_device *sp = dev_get_drvdata(dev);

	return hygon_sp_restore(sp);
}

static const struct dev_pm_ops hygon_pm_ops = {
	.suspend = hygon_sp_pci_suspend,
	.resume = hygon_sp_pci_resume,
	.freeze = hygon_sp_pci_freeze,
	.thaw = hygon_sp_pci_thaw,
	.poweroff = hygon_sp_pci_poweroff,
	.restore = hygon_sp_pci_restore,
};

void hygon_set_pm_cb(struct pci_driver *drv)
{
	drv->driver.pm = &hygon_pm_ops;
}

#endif   /* CONFIG_PM_SLEEP */
