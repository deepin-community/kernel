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

#ifndef __ERRNO_H_
#define __ERRNO_H_

#define S_OK                0x00000000
#define E_OUTOFMEMORY       0x80000002
#define E_NOIMPL            0x80004001
#define E_INVALIDARG        0x80070057
#define E_NOINTERFACE       0x80004002
#define E_POINTER           0x80004003
#define E_UNEXPECTED        0x8000FFFF
#define E_FAIL              0x10004005
#define E_UNSWIZZLING_APERTURE_UNSUPPORTED  0x10004006
/*page out allocation but allocation still used by GPU hw*/
#define E_PAGEOUT_ALLOCATION_BUSY  0x10004007
#define E_INSUFFICIENT_DMA_BUFFER   0xFFFF0001


#endif

