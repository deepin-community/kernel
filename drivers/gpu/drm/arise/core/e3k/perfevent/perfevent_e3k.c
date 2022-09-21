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

#include "perfeventi.h"
#include "gf_def.h"

int perf_direct_get_counter_e3k(adapter_t *adapter, gf_miu_list_item_t *miu_table, unsigned int miu_table_length,int force_read)
{
    gf_assert(0, NULL);
    return 0;
}

perf_chip_func_t perfevent_chip_func =
{
    .direct_get_miu_counter = perf_direct_get_counter_e3k
};

