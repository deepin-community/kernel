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

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "gf_chip_id.h"
struct krnl_adapter_init_info_s;
extern void glb_init_chip_id(adapter_t *adapter, struct krnl_adapter_init_info_s *info);
extern void glb_get_pci_config_info(adapter_t *adapter);
extern void glb_init_chip_interface(adapter_t *adapter);
extern void glb_fini_bus_config(adapter_t *adapter);

extern void glb_init_power_caps(adapter_t *adapter);
#endif
