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
	.bootloader_info_reg	= 0x105ec,	/* C2PMSG_59 */
	.feature_reg		= 0x105fc,	/* C2PMSG_63 */
	.inten_reg		= 0x10610,	/* P2CMSG_INTEN */
	.intsts_reg		= 0x10614,	/* P2CMSG_INTSTS */
};

#endif

const struct sp_dev_vdata hygon_dev_vdata[] = {
	{	/* 0 */
		.bar = 2,
#ifdef CONFIG_CRYPTO_DEV_SP_CCP
		.ccp_vdata = &ccpv5a,
#endif
#ifdef CONFIG_CRYPTO_DEV_SP_PSP
		.psp_vdata = &pspv1,
#endif
	},
	{	/* 1 */
		.bar = 2,
#ifdef CONFIG_CRYPTO_DEV_SP_CCP
		.ccp_vdata = &ccpv5b,
#endif
	},
};
