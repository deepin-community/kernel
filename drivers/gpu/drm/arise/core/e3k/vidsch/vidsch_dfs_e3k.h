//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd..
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China.
//*****************************************************************************

#ifndef __PM_DFS_E3K_H__

#include "gf_adapter.h"
#include "vidsch.h"
#include "vidschi.h"

int vidsch_power_clock_on_off_e3k(vidsch_mgr_t *sch_mgr, unsigned int off);
int vidsch_power_clock_on_off_vcp(vidsch_mgr_t *sch_mgr, unsigned int off);
void vidsch_power_tuning_e3k(adapter_t *adapter, unsigned int gfx_only);
#endif
