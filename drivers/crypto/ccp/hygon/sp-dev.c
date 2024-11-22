// SPDX-License-Identifier: GPL-2.0-only
/*
 * HYGON Platform Security Processor (PSP) interface
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Zhaowei Bai <baizhaowei@hygon.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sp-dev.h"

#ifdef CONFIG_PM_SLEEP

int hygon_sp_suspend(struct sp_device *sp)
{
	if (sp->dev_vdata->ccp_vdata)
		ccp_dev_suspend(sp);

	return 0;
}

int hygon_sp_resume(struct sp_device *sp)
{
	if (sp->dev_vdata->ccp_vdata)
		ccp_dev_resume(sp);

	return 0;
}

int hygon_sp_freeze(struct sp_device *sp)
{
	if (sp->dev_vdata->ccp_vdata)
		ccp_dev_suspend(sp);

	if (sp->dev_vdata->psp_vdata)
		hygon_psp_dev_freeze(sp);

	return 0;
}

int hygon_sp_thaw(struct sp_device *sp)
{
	if (sp->dev_vdata->ccp_vdata)
		ccp_dev_resume(sp);

	if (sp->dev_vdata->psp_vdata)
		hygon_psp_dev_thaw(sp);

	return 0;
}

int hygon_sp_poweroff(struct sp_device *sp)
{
	if (sp->dev_vdata->ccp_vdata)
		ccp_dev_suspend(sp);

	return 0;
}

int hygon_sp_restore(struct sp_device *sp)
{
	if (sp->dev_vdata->ccp_vdata)
		ccp_dev_resume(sp);

	if (sp->dev_vdata->psp_vdata)
		hygon_psp_dev_restore(sp);

	return 0;
}

#endif   /* CONFIG_PM_SLEEP */
