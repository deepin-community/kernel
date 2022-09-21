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

#ifndef __RM_RESOURCE_E3K_H
#define __RM_RESOURCE_E3K_H

#define E_UNIT_SIZE                  8
#define ALIGNED_2KBITS(Value)        (((Value) + 0xFF) & ~0xFF)
#define ALIGNED_128KBYTE(Value)      (((Value) + 0x1FFFF) & ~0x1FFFF)
#define HW_MAX_TARGETS           8
#define DEFAULT_RT_FORMAT HSF_R8G8B8A8_UNORM

extern const UINT SurfFmtModeTable[];
extern const UINT BitCountTable[];
extern const UINT UavFmtTable[];
#endif
