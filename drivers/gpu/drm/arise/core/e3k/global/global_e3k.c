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

#include "gf_adapter.h"
#include "global.h"
#include "register_e3k.h"

void glb_init_chip_interface(adapter_t *adapter)
{
    write_reg_e3k(adapter->mmio, CR_C, 0x01, 0xFF, 0x00); //ensure power on
    gf_info("init engine power status: 0x%x\n", read_reg_e3k(adapter->mmio, CR_C, 0x01));
}

void glb_init_power_caps(adapter_t *adapter)
{
    gf_warning("%s(): is blank, maybe implemented in other function()\n", util_remove_name_suffix(__func__));
}
