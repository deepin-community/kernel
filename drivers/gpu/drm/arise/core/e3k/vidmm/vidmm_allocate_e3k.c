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
#include "vidmm.h"
#include "vidmmi.h"
#include "mm_e3k.h"
#include "chip_include_e3k.h"

unsigned int __BitCountTable_e3k[] = BIT_COUNT_TABLE;
const HWFORMAT_TABLE_ENTRY_E3K g_HwFormatTable_e3k[]=
{
    //                                                                                ChannelMask                                                                      
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*HSF_UNKNOWN              = 0,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R64G64B64A64_FLOAT   = 1,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R64G64B64_FLOAT      = 2,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_TYPELESS= 3,*/
    HSF_R32G32B32A32_FLOAT, HSF_R32G32B32X32_FLOAT,DS_NONE,       CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32A32_FLOAT   = 4,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32A32_UINT    = 5,*/
    HSF_R32G32B32A32_SINT,  HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32A32_SINT    = 6,*/
    HSF_R32G32B32A32_SNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_SNORM   = 7,*/
    HSF_R32G32B32A32_SCALE, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_SCALE   = 8,*/
    HSF_R32G32B32A32_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_UNORM   = 9,*/
    HSF_R32G32B32A32_USCALE,HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_USCALE  = 10,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_FIX     = 11,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R64G64_FLOAT         = 12,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32_TYPELESS  = 13,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  1,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32_FLOAT      = 14,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  1,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32_UINT       = 15,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  1,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32_SINT       = 16,*/
    HSF_R32G32B32_UNORM,    HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  1,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32_UNORM      = 17,*/
    HSF_R32G32B32_USCALE,   HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32_USCALE     = 18,*/
    HSF_R32G32B32_SNORM,    HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  1,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32_SNORM      = 19,*/
    HSF_R32G32B32_SCALE,    HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32_SCALE      = 20,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32B32_FIX        = 21,*/
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGRA, 0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16A16_TYPELESS = 22,*/
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16A16_FLOAT   = 23,*/
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16A16_UNORM   = 24,*/
    HSF_R16G16B16A16_UINT,  HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16A16_UINT    = 25,*/
    HSF_R16G16B16A16_SNORM, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,  C_BGRA, 1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16A16_SNORM   = 26,*/
    HSF_R16G16B16A16_SINT,  HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16A16_SINT    = 27,*/
    HSF_R16G16B16A16_SCALE, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16A16_SCALE   = 28,*/
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16A16_USCALE  = 29,*/
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16A16_SSCALE  = 30,*/
    //                                                                                ChannelMask                                                                      
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R12G12B12A12_UNORM  = 31,*/
    HSF_R16G16B16_FLOAT,    HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_FLOAT      = 32,*/
    HSF_R16G16B16_SINT,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_SINT       = 33,*/
    HSF_R16G16B16_SNORM,    HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_SNORM      = 34,*/
    HSF_R16G16B16_SCALE,    HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_SCALE      = 35,*/
    HSF_R16G16B16_UINT,     HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_UINT       = 36,*/
    HSF_R16G16B16_UNORM,    HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_BGR,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_UNORM      = 37,*/
    HSF_R16G16B16_USCALE,   HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_USCALE     = 38,*/
    HSF_R32G32_FLOAT,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32_TYPELESS      = 39,*/
    HSF_R32G32_FLOAT,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32_FLOAT         = 40,*/
    HSF_R32G32_UINT,        HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32_UINT          = 41,*/
    HSF_R32G32_SINT,        HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32_SINT          = 42,*/
    HSF_R32G32_SNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32_SNORM         = 43,*/
    HSF_R32G32_SCALE,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32_SCALE         = 44,*/
    HSF_R32G32_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   0,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32_UNORM         = 45,*/
    HSF_R32G32_USCALE,      HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32_USCALE        = 46,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32G32_FIX           = 47,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,    D32_FLOAT_S8_UINT,    CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32G8X24_TYPELESS         = 48,*/
    HSF_R32_FLOAT,          HSF_UNKNOWN,    D32_FLOAT_S8_UINT,    CP_Z32,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_D32_FLOAT_S8X24_UINT      = 49,*/
    HSF_R32G32_FLOAT,       HSF_UNKNOWN,    D32_FLOAT_S8_UINT,    CP_Z32,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32_FLOAT_X8X24_TYPELESS  = 50,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,    D32_FLOAT_S8_UINT,    CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_X32_TYPELESS_G8X24_UINT   = 51,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R64_FLOAT                 = 52,*/
    HSF_R16G16B16X16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_RGBX,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R12G12B12_UNORM           = 53,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10A2_TYPELESS      = 54,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R10G10B10A2_UNORM         = 55,*/
    HSF_R10G10B10A2_UINT,   HSF_R10G10B10X2_UINT, DS_NONE,        CP_R10G10B10A2, C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R10G10B10A2_UINT          = 56,*/
    HSF_R10G10B10A2_UINT,   HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10X2_UINT          = 57,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10X2_USCALE        = 58,*/
    HSF_R10G10B10A2_UNORM,  HSF_R10G10B10X2_SNORM,DS_NONE,        CP_R10G10B10A2, C_BGRA, 0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10A2_SNORM         = 59,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10A2_SINT          = 60,*/
    //                                                                                ChannelMask                                                                      
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10X2_SNORM    = 61,*/
    HSF_B10G10R10A2_SNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_B10G10R10A2_SNORM    = 62,*/
    HSF_B10G10R10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_B10G10R10A2_UNORM    = 63,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_B10G10R10X2_UNORM    = 64,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_A2B10G10R10, C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A2B10G10R10_UNORM    = 65,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_A2B10G10R10, C_BGRA, 0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A2B10G10R10_SNORM    = 66,*/
    HSF_A2B10G10R10_USCALE, HSF_UNKNOWN,          DS_NONE,        CP_A2B10G10R10, C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A2B10G10R10_USCALE   = 67,*/
    HSF_A2B10G10R10_SSCALE, HSF_UNKNOWN,          DS_NONE,        CP_A2B10G10R10, C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A2B10G10R10_SSCALE   = 68,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_SINT       = 69,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_SNORM      = 70,*/
    HSF_R10G10B10_SCALE,    HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_SCALE      = 71,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_UNIT       = 72,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_UNORM      = 73,*/
    HSF_R10G10B10_USCALE,   HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_USCALE     = 74,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10_SINT          = 75,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10_SNORM         = 76,*/
    HSF_R10G10_SCALE,       HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10_SCALE         = 77,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10_UINT          = 78,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10_UNORM         = 79,*/
    HSF_R10G10_SCALE,       HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10_USCALE        = 80,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10_FLOAT_A2_UNORM = 81,*/
    HSF_R11G11B10_FLOAT,    HSF_UNKNOWN,          DS_NONE,        CP_R11G11B10,   C_BGR,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R11G11B10_FLOAT      = 82,*/
    HSF_B10G11R11_FLOAT,    HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_B10G11R11_FLOAT      = 83,*/
    HSF_R11G11B10_FLOAT,    HSF_UNKNOWN,          DS_NONE,        CP_R11G11B10,   C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R11G11_FLOAT         = 84,*/
    HSF_R8G8B8A8_UNORM,     HSF_R8G8B8X8_UNORM,   DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8A8_TYPELESS    = 85,*/
    HSF_R8G8B8A8_UNORM,     HSF_R8G8B8X8_UNORM,   DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8A8_UNORM       = 86,*/
    HSF_R8G8B8A8_UNORM_SRGB,HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8A8_UNORM_SRGB  = 87,*/
    HSF_R8G8B8X8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8X8_UNORM       = 88,*/
    HSF_B8G8R8A8_UNORM,     HSF_B8G8R8X8_UNORM,   DS_NONE,        CP_A8R8G8B8_L,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_B8G8R8A8_UNORM       = 89,*/
    HSF_B8G8R8A8_UNORM_SRGB,HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  4,/*HSF_B8G8R8A8_UNORM_SRGB  = 90,*/
    //                                                                                ChannelMask                                                                      
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    HSF_B8G8R8X8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_A8R8G8B8_L,  C_RGBX,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_B8G8R8X8_UNORM       = 91,*/
    HSF_B8G8R8X8_UNORM_SRGB,HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX,  1,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  4,/*HSF_B8G8R8X8_UNORM_SRGB  = 92,*/
    HSF_X8R8G8B8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_A8R8G8B8_L,  C_RGBX,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_X8R8G8B8_UNORM       = 93,*/
    HSF_X8B8G8R8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_A8R8G8B8_L,  C_RGBX,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_X8B8G8R8_UNORM       = 94,*/
    HSF_R8G8B8A8_UNORM,     HSF_X8R8G8B8_UNORM,   DS_NONE,        CP_A8R8G8B8_L,  C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A8R8G8B8_UNORM       = 95,*/
    HSF_R8G8B8A8_UNORM,     HSF_X8B8G8R8_UNORM,   DS_NONE,        CP_A8R8G8B8_L,  C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A8B8G8R8_UNORM       = 96,*/
    HSF_R8G8B8A8_UINT,      HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8A8_UINT        = 97,*/
    HSF_R8G8B8A8_SNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8A8_SNORM       = 98,*/
    HSF_R8G8B8A8_SINT,      HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8A8_SINT        = 99,*/
    HSF_R8G8B8A8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_B8G8R8A8_XNORM       = 100,*/
    HSF_R8G8B8A8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8A8_SSCALE      = 101,*/
    HSF_R8G8B8A8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8A8_USCALE      = 102,*/
    HSF_R8G8B8_SINT,        HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_SINT          = 103,*/
    HSF_R8G8B8_SNORM,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_SNORM         = 104,*/
    HSF_R8G8B8_SCALE,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_SCALE         = 105,*/
    HSF_R8G8B8_UINT,        HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_UINT          = 106,*/
    HSF_R8G8B8_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_UNORM         = 107,*/
    HSF_R8G8B8_USCALE,      HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_USCALE        = 108,*/
    HSF_R16G16_FLOAT,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16_TYPELESS      = 109,*/
    HSF_R16G16_FLOAT,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16_FLOAT         = 110,*/
    HSF_R16G16_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16_UNORM         = 111,*/
    HSF_R16G16_UINT,        HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16_UINT          = 112,*/
    HSF_R16G16_SNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16_SNORM         = 113,*/
    HSF_R16G16_SINT,        HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16_SINT          = 114,*/
    HSF_R16G16_SCALE,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16_SCALE         = 115,*/
    HSF_R16G16_FLOAT,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16_USCALE        = 116,*/
    HSF_R16G16_FLOAT,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16_SSCALE        = 117,*/
    HSF_R32_FLOAT,          HSF_UNKNOWN,          D32_FLOAT,      CP_Z32,         C_R,    0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,/*HSF_R32_TYPELESS         = 118,*/
    HSF_R32_FLOAT,          HSF_UNKNOWN,          D32_FLOAT,      CP_Z32,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_D32_FLOAT            = 119,*/
    HSF_R32_FLOAT,          HSF_UNKNOWN,          D32_FLOAT,      CP_Z32,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_D32_UNORM            = 120,*/
    //                                                                                ChannelMask                                                                      
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    HSF_R32_FLOAT,          HSF_UNKNOWN,          D32_FLOAT,      CP_Z32,         C_R,    1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32_FLOAT            = 121,*/
    HSF_R32_UINT,           HSF_UNKNOWN,          DS_NONE,        CP_Z32,         C_R,    1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32_UINT             = 122,*/
    HSF_R32_SINT,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32_SINT             = 123,*/
    HSF_R32_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_Z32,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32_UNORM            = 124,*/
    HSF_R32_USCALE,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32_USCALE           = 125,*/
    HSF_R32_SNORM,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32_SNORM            = 126,*/
    HSF_R32_SCALE,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32_SCALE             = 127,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_R,    0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R32_FIX              = 128,*/
    HSF_R32_FLOAT,          HSF_UNKNOWN,     D24_UNORM_S8_UINT,   CP_Z24,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R24G8_TYPELESS       = 129,*/
    HSF_R32_FLOAT,          HSF_UNKNOWN,     D32_FLOAT_S8_UINT,   CP_Z32,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_D32_FLOAT_S8_UINT    = 130,*/
    HSF_R32_FLOAT,          HSF_UNKNOWN,     D24_UNORM_S8_UINT,   CP_Z24,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_D24_UNORM_S8_UINT    = 131,*/
    HSF_R32_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_Z24,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R24_UNORM_X8_TYPELESS= 132,*/
    HSF_R32_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_X24_TYPELESS_G8_UINT = 133,*/
    HSF_R32_FLOAT,          HSF_UNKNOWN,          D24_UNORM,      CP_Z24,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_D24_UNORM            = 134,*/
    HSF_R32_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_Z24,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R24_FLOAT            = 135,*/
    HSF_R8G8_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8_TYPELESS        = 136,*/
    HSF_R8G8_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8_UNORM           = 137,*/
    HSF_R8G8_UINT,          HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8_UINT            = 138,*/
    HSF_R8G8_SNORM,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8_SNORM           = 139,*/
    HSF_R8G8_SINT,          HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_GR,   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8_SINT            = 140,*/
    HSF_R8G8_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8_USCALE          = 141,*/
    HSF_R8G8_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8_SSCALE          = 142,*/
    HSF_R16_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_Z16,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16_TYPELESS         = 143,*/
    HSF_R16_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_Z16,         C_R,    1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16_FLOAT            = 144,*/
    HSF_R16_UNORM,          HSF_UNKNOWN,          D16_UNORM,      CP_Z16,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_D16_UNORM            = 145,*/
    HSF_R16_UNORM,          HSF_UNKNOWN,          D16_UNORM,      CP_Z16,         C_R,    1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16_UNORM            = 146,*/
    HSF_R16_UINT,           HSF_UNKNOWN,          DS_NONE,        CP_Z16,         C_R,    1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16_UINT             = 147,*/
    HSF_R16_SNORM,          HSF_UNKNOWN,          DS_NONE,        CP_Z16,         C_R,    1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16_SNORM            = 148,*/
    HSF_R16_SINT,           HSF_UNKNOWN,          DS_NONE,        CP_Z16,         C_R,    1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16_SINT             = 149,*/
    HSF_R16_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16_USCALE           = 150,*/
    //                                                                                ChannelMask                                                                      
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    HSF_R16_FLOAT,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16_SSCALE           = 151,*/
    HSF_A16_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_A,    1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A16_UNORM            = 152,*/
    HSF_B5G6R5_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGR,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_B5G6R5_UNORM         = 153,*/
    HSF_R5G6B5_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGR,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R5G6B5_UNORM         = 154,*/
    HSF_B5G5R5A1_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGR,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_B5G5R5X1_UNORM       = 155,*/
    HSF_B5G5R5A1_UNORM,     HSF_B5G5R5X1_UNORM,   DS_NONE,        CP_R5G6B5,      C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_B5G5R5A1_UNORM       = 156,*/
    HSF_R5G5B5A1_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R5G5B5A1_UNORM       = 157,*/
    HSF_A1B5G5R5_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_A1B5G5R5_UNORM       = 158,*/
    HSF_A1R5G5B5_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_A1R5G5B5_UNORM       = 159,*/
    HSF_B4G4R4A4_UNORM,     HSF_B4G4R4X4_UNORM,   DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_B4G4R4A4_UNORM       = 160,*/
    HSF_B4G4R4A4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_B4G4R4X4_UNORM       = 161,*/
    HSF_R4G4B4A4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R4G4B4A4_UNORM       = 162,*/
    HSF_A4B4G4R4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_A4B4G4R4_UNORM       = 163,*/
    HSF_R4G4B4A4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A4R4G4B4_UNORM       = 164,*/
    HSF_R4G4B4A4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGR,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R4G4B4_UNORM         = 165,*/
    HSF_R5G5B5_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_BGR,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R5G5B5_UNORM         = 166,*/
    HSF_B5G6R5_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_U5V5L6_XNORM         = 167,*/
    HSF_R16_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_Z16,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L16_UNORM            = 168,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L8_A8_UNORM          = 169,*/
    HSF_R10_SINT,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10_SINT             = 170,*/
    HSF_R10_SNORM,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10_SNORM            = 171,*/
    HSF_R10_SCALE,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10_SCALE            = 172,*/
    HSF_R10_UINT,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10_UINT             = 173,*/
    HSF_R10_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10_UNORM            = 174,*/
    HSF_R10_USCALE,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10_USCALE           = 175,*/
    HSF_R4G4B4A4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R2G2B2A2_UNORM       = 176,*/
    HSF_R8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8_TYPELESS          = 177,*/
    HSF_R8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_R,    1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8_UNORM             = 178,*/
    HSF_R8_UINT,            HSF_UNKNOWN,          S8_UINT,        CP_R8G8B8A8_L,  C_R,    1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8_UINT              = 179,*/
    HSF_R8_SNORM,           HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_R,    1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8_SNORM             = 180,*/
    //                                                                                ChannelMask                                                                   
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    HSF_R8_SINT,            HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_R,    1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8_SINT              = 181,*/
    HSF_R8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8_USCALE            = 182,*/
    HSF_R8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8_SSCALE            = 183,*/
    HSF_A8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_A,    1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_A8_UNORM             = 184,*/
    HSF_R8_UNORM,           HSF_UNKNOWN,          S8_UINT,        CP_R8G8B8A8_L,  C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L8_UNORM             = 185,*/
    HSF_A16_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A12_UNORM            = 186,*/
    HSF_A8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A4_UNORM             = 187,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R1_UNORM             = 188,*/
    HSF_R4G4B4X4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R3G3B2_UNORM         = 189,*/
    HSF_L4A4_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_L4A4_UNORM           = 190,*/
    HSF_L4A4_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A4L4_UNORM           = 191,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L4A4_VIDEO           = 192,*/
    HSF_AYUV_VIDEO,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_AYUV_VIDEO           = 193,*/
    HSF_YUYV,               HSF_UNKNOWN,          DS_NONE,        CP_YUYV,        C_BGRA, 1,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_YUYV                 = 194,*/
    HSF_NV12,               HSF_UNKNOWN,          DS_NONE,        CP_NV12,        C_NONE, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_NV12                 = 195,*/
    HSF_P010,               HSF_UNKNOWN,          DS_NONE,        CP_NV12_10,     C_NONE, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_P010                 = 196,*/
    HSF_P016,               HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_P016                 = 197,*/
    HSF_R8G8B8A8_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R9G9B9E5_SHAREDEXP   = 198,*/
    HSF_UYVY,               HSF_UNKNOWN,          DS_NONE,        CP_UYVY,        C_BGRA, 0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8_B8G8_UNORM      = 199,*/ //!< we treat these two(199/200) as UYVY/YUYV 
    HSF_YUYV,               HSF_UNKNOWN,          DS_NONE,        CP_YUYV,        C_BGRA, 1,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_G8R8_G8B8_UNORM      = 200,*/
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC1_UNORM            = 201,*/
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC1_UNORM_SRGB       = 202,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC2_UNORM            = 203,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC2_UNORM_SRGB       = 204,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC3_UNORM            = 205,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC3_UNORM_SRGB       = 206,*/
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC4_UNORM            = 207,*/
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC4_SNORM            = 208,*/
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC4_UNORM_L          = 209,*/
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC4_SNORM_L          = 210,*/
    //                                                                                ChannelMask                                                                   
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC5_UNORM            = 211,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC5_SNORM            = 212,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC5_UNORM_LA         = 213,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC5_SNORM_LA         = 214,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC6H_UF16            = 215,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC6H_SF16            = 216,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC7_UNORM            = 217,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_BC7_UNORM_SRGB       = 218,*/
    HSF_R8_UNORM,           HSF_UNKNOWN,          S8_UINT,        CP_R8G8B8A8_L,  C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_G8_UINT              = 219,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_3DC_UNORM            = 220,*/
    HSF_YUYV,               HSF_UNKNOWN,          DS_NONE,        CP_YUYV,        C_NONE, 0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_YUY2                 = 221,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_Y216_VIDEO           = 222,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_Y210_VIDEO           = 223,*/
    HSF_A4I4_VIDEO,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A4I4_VIDEO           = 224,*/
    HSF_A8L8_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_A8L8_UNORM           = 225,*/
    HSF_L8A8_UNORM,         HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_BGRA, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L8A8_UNORM           = 226,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_ZL1                  = 227,*/
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_NONE, 0,  0,  0,  0,  1,  0,  1,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16A16_UNORM_SRGB = 228*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_YCRCB_MB_8_422       = 229,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_YCRCB_MB_16_420      = 230,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_YCRCB_MB_16_422      = 231,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_P016T                = 232,*/
    HSF_UYVA1010102_VIDEO,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_UYVA1010102_VIDEO    = 233,*/
    HSF_UYVY,               HSF_UNKNOWN,          DS_NONE,        CP_UYVY,        C_BGRA, 1,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_UYVY                 = 234,*/
    HSF_R16_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_BAYER,       C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_BAYER_12             = 235,*/
    HSF_R16_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_BAYER,       C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_BAYER                = 236,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_P016L                = 237,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_YUYV,        C_NONE, 0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_YV12                 = 238,*/
    HSF_NV12,               HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_NV12T                = 239,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_S1_UINT              = 240,*/
    //                                                                                ChannelMask                                                                   
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_S16_UINT             = 241,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_S4_UINT              = 242,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_S8_UINT              = 243,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_I12_UNORM            = 244,*/
    HSF_R16_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_I16_UNORM            = 245,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_I4_UNORM             = 246,*/
    HSF_R8_UNORM,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_I8_UNORM             = 247,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L12_UNORM            = 248,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L12_A12_UNORM        = 249,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L12_A4_UNORM         = 250,*/
    HSF_R16G16_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L16_A16_UNORM        = 251,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L4_UNORM             = 252,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L4_A4_UNORM          = 253,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L6_A2_UNORM          = 254,*/
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_RGB8_ETC2            = 255,*/
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_SRGB8_ETC2           = 256,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_RGBA8_ETC2_EAC       = 257,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_SRGB8_ALPHA8_ETC2_EAC= 258,*/
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_R11_EAC              = 259,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_RG11_EAC             = 260,*/
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_SIGNED_R11_EAC       = 261,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_SIGNED_RG11_EAC      = 262,*/
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_RGB8_PUNCHTHROUGH_ALPHA1_ETC2  = 263,*/
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  4,  4,  1,  0,/*HSF_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 264,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_ALPHA                     = 265,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_INTENSITY                 = 266,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_LUMINANCE                 = 267,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RED                       = 268,*/
    HSF_R16G16B16A16_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_COMPRESSED_RED_RGTC1                 = 269,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RG                        = 270,*/
    //                                                                                ChannelMask                                                                   
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_COMPRESSED_RG_RGTC2                  = 271,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RGB                       = 272,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RGB_BPTC_SIGNED_FLOAT     = 273,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT   = 274,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RGBA                      = 275,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_RGBA_BPTC_UNORM           = 276,*/
    HSF_R16G16B16A16_SINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_COMPRESSED_SIGNED_RED_RGTC1          = 277,*/
    HSF_R32G32B32A32_SINT , HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4,  1,  0,/*HSF_COMPRESSED_SIGNED_RG_RGTC2           = 278,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_SLUMINANCE                = 279,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_SLUMINANCE_ALPHA          = 280,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_SRGB                      = 281,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_SRGB_ALPHA                = 282,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,/*HSF_COMPRESSED_SRGB_ALPHA_BPTC_UNORM     = 283,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10A2_USCALE                   = 284,*/
    HSF_R10G10B10A2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10A2_SSCALE                   = 285,*/
    HSF_R32G32B32A32_SSCALE,HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32G32B32A32_SSCALE                  = 286,*/
    HSF_R32G32B32_SSCALE,   HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32G32B32_SSCALE                     = 287,*/
    HSF_R32G32_SSCALE,      HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32G32_SSCALE                        = 288,*/
    HSF_R16G16B16_SSCALE,   HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R16G16B16_SSCALE                     = 289,*/
    HSF_R32_SSCALE,         HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R32_SSCALE                           = 290,*/
    HSF_R8G8B8_SSCALE,      HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R8G8B8_SSCALE                        = 291,*/
    //                                                                                ChannelMask                                                                   
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 292,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 293,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 294,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 295,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 296,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 297,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 298,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 299,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 300,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 301,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 302,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 303,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 304,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 305,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 306,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 307,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 308,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 309,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 310,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 311,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 312,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 313,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 314,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 315,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 316,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 317,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 318,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 319,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*hold place              = 320,*/
    //                                                                                ChannelMask                                                                   
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  4,  4,  1,  0,/*HSF_RGBA_ASTC_4X4        = 321,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  5,  4,  1,  0,/*HSF_RGBA_ASTC_5X4        = 322,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  5,  5,  1,  0,/*HSF_RGBA_ASTC_5X5                        = 323,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  6,  5,  1,  0,/*HSF_RGBA_ASTC_6X5                        = 324,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  6,  6,  1,  0,/*HSF_RGBA_ASTC_6X6                        = 325,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  8,  5,  1,  0,/*HSF_RGBA_ASTC_8X5                        = 326,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  8,  6,  1,  0,/*HSF_RGBA_ASTC_8X6                        = 327,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  8,  8,  1,  0,/*HSF_RGBA_ASTC_8X8                        = 328,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  10, 5,  1,  0,/*HSF_RGBA_ASTC_10X5                       = 329,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  10, 6,  1,  0,/*HSF_RGBA_ASTC_10X6                       = 330,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  10, 8,  1,  0,/*HSF_RGBA_ASTC_10X8                       = 331,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  10, 10, 1,  0,/*HSF_RGBA_ASTC_10X10                      = 332,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  12, 10, 1,  0,/*HSF_RGBA_ASTC_12X10                      = 333,*/ 
    //                                                                                ChannelMask                                                                   
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  12, 12,  1,  0,/*HSF_RGBA_ASTC_12X12                      = 334,*/ 
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,/*HSF_RESERVED2            = 335,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,/*HSF_RESERVED3            = 336,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  4,  4,  1,   0,/*HSF_RGBA_ASTC_4X4_SRGB                   = 337,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  5,  4,  1,   0,/*HSF_RGBA_ASTC_5X4_SRGB                   = 338,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  5,  5,  1,   0,/*HSF_RGBA_ASTC_5X5_SRGB                   = 339,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  6,  5,  1,   0,/*HSF_RGBA_ASTC_6X5_SRGB                   = 340,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  6,  6,  1,   0,/*HSF_RGBA_ASTC_6X6_SRGB                   = 341,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  8,  5,  1,   0,/*HSF_RGBA_ASTC_8X5_SRGB                   = 342,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  8,  6,  1,   0,/*HSF_RGBA_ASTC_8X6_SRGB                   = 343,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  8,  8,  1,   0,/*HSF_RGBA_ASTC_8X8_SRGB                   = 344,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  10, 5,  1,   0,/*HSF_RGBA_ASTC_10X5_SRGB                  = 345,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  10, 6,  1,   0,/*HSF_RGBA_ASTC_10X6_SRGB                  = 346,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  10, 8,  1,   0,/*HSF_RGBA_ASTC_10X8_SRGB                  = 347,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  10, 10, 1,   0,/*HSF_RGBA_ASTC_10X10_SRGB                 = 348,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  12, 10, 1,   0,/*HSF_RGBA_ASTC_12X10_SRGB                 = 349,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  12, 12, 1,   0,/*HSF_RGBA_ASTC_12X12_SRGB                 = 350,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,/*HSF_RESERVED4            = 351,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,/*HSF_RESERVED5            = 352,*/
    //                                                                                ChannelMask                                                                   
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    //the following new added by Martina ASTC 3D
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  3,  3,  3,  0,/*HSF_RGBA_ASTC_3x3x3                        = 353,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  4,  3,  3,  0,/*HSF_RGBA_ASTC_4x3x3                        = 354,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  4,  4,  3,  0,/*HSF_RGBA_ASTC_4x4x3                        = 355,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  4,  4,  4,  0,/*HSF_RGBA_ASTC_4x4x4                        = 356,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  5,  4,  4,  0,/*HSF_RGBA_ASTC_5x4x4                        = 357,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  5,  5,  4,  0,/*HSF_RGBA_ASTC_5x5x4                        = 358,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  5,  5,  5,  0,/*HSF_RGBA_ASTC_5x5x5                        = 359,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  6,  5,  5,  0,/*HSF_RGBA_ASTC_6x5x5                        = 360,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  6,  6,  5,  0,/*HSF_RGBA_ASTC_6x6x5                        = 361,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  6,  6,  6,  0,/*HSF_RGBA_ASTC_6x6x6                        = 362,*/ 
    HSF_R16G16_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L16A16_UNORM            = 363,*/ 
    HSF_R16G16_SNORM,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_L16A16_SNORM            = 364,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_ASTC_RESERVED6            = 365,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_ASTC_RESERVED7            = 366,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_ASTC_RESERVED8            = 367,*/
    HSF_UNKNOWN,            HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_ASTC_RESERVED9            = 368,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  3,  3,  3,  0,/*HSF_RGBA_ASTC_3x3x3_SRGB                   = 369,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  4,  3,  3,  0,/*HSF_RGBA_ASTC_4x3x3_SRGB                   = 370,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  4,  4,  3,  0,/*HSF_RGBA_ASTC_4x4x3_SRGB                   = 371,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  4,  4,  4,  0,/*HSF_RGBA_ASTC_4x4x4_SRGB                   = 372,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  5,  4,  4,  0,/*HSF_RGBA_ASTC_5x4x4_SRGB                   = 373,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  5,  5,  4,  0,/*HSF_RGBA_ASTC_5x5x4_SRGB                   = 374,*/
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  5,  5,  5,  0,/*HSF_RGBA_ASTC_5x5x5_SRGB                   = 375,*/	
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  6,  5,  5,  0,/*HSF_RGBA_ASTC_6x5x5_SRGB                   = 376,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  6,  6,  5,  0,/*HSF_RGBA_ASTC_6x6x5_SRGB                   = 377,*/ 
    HSF_R32G32B32A32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  6,  6,  6,  0,/*HSF_RGBA_ASTC_6x6x6_SRGB                   = 378,*/
    HSF_R64_UINT,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64_UINT                             = 379,*/
    HSF_R64G64_UINT,        HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64G64_UINT                          = 380,*/
    //                                                                                ChannelMask                                                                   
    //                                                                                |   bRtSupport                                                               
    //                                                                                |   |   bBlockCompressed                                             
    //                                                                                |   |   |   bYUY2                                                    
    //                                                                                |   |   |   |   b96bpp                                               
    //                                                                                |   |   |   |   |   bUnorm                                           
    //                                                                                |   |   |   |   |   |   bSnorm                                       
    //                                                                                |   |   |   |   |   |   |   bSrgb                                    
    //                                                                                |   |   |   |   |   |   |   |   bTypeless                            
    //  MappedDstFormat         SuppressAlphaFormat   CompatibleDsFmt CompressFmt     |   |   |   |   |   |   |   |   |   bFP32Mode                        
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   bASTC                        
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |BlockWidth              
    //..|.......................|.....................|...............|...............|   |   |   |   |   |   |   |   |   |   |   |   |BlockHeight         
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |BlockDepth      
    //  |                       |                     |               |               |   |   |   |   |   |   |   |   |   |   |   |   |   |   |ClrLineNum     
    HSF_R64G64B64_UINT,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64G64B64_UINT                       = 381,*/
    HSF_R64G64B64A64_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64G64B64A64_UINT                    = 382,*/
    HSF_R64_SINT,           HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64_SINT                             = 383,*/
    HSF_R64G64_SINT,        HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64G64_SINT                          = 384,*/
    HSF_R64G64B64_SINT,     HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGR,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64G64B64_SINT                       = 385,*/
    HSF_R64G64B64A64_SINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_R64G64B64A64_SINT                    = 386,*/
    HSF_R4G4B4X4_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX, 1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R4G4B4X4_UNORM                       = 387,*/\
    HSF_R5G5B5X1_UNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R5G6B5,      C_RGBX,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R5G5B5X1_UNORM                       = 388,*/\
    HSF_R8G8B8X8_SNORM,     HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8X8_SNORM                       = 389,*/\
    HSF_R8G8B8X8_UINT,      HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8X8_UINT                        = 390,*/\
    HSF_R8G8B8X8_SINT,      HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8X8_SINT                        = 391,*/\
    HSF_R8G8B8X8_UNORM_SRGB,HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX,  1,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  4,/*HSF_R8G8B8X8_UNORM_SRGB                  = 392,*/\
    HSF_R10G10B10X2_UNORM,  HSF_UNKNOWN,          DS_NONE,        CP_R10G10B10A2, C_RGBX,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R10G10B10X2_UNORM                    = 393,*/\
    HSF_R16G16B16X16_UNORM, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_RGBX,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16X16_UNORM                   = 394,*/\
    HSF_R16G16B16X16_SNORM, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_RGBX,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16X16_SNORM                   = 395,*/\
    HSF_R16G16B16X16_FLOAT, HSF_UNKNOWN,          DS_NONE,      CP_R16G16B16A16_T,C_RGBX,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16X16_FLOAT                   = 396,*/\
    HSF_R16G16B16X16_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16X16_UINT                    = 397,*/\
    HSF_R16G16B16X16_SINT,  HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8,/*HSF_R16G16B16X16_SINT                    = 398,*/\
    HSF_R32G32B32X32_FLOAT, HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32X32_FLOAT                   = 399,*/\
    HSF_R32G32B32X32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32X32_UINT                    = 400,*/\
    HSF_R32G32B32X32_SINT,  HSF_UNKNOWN,          DS_NONE,        CP_R8G8B8A8_L,  C_RGBX,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  16,/*HSF_R32G32B32X32_SINT                    = 401,*/\
    HSF_R32_FLOAT,          HSF_UNKNOWN,     D24_UNORM_S8_UINT,   CP_Z24,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_S8_UINT_D24_UNORM    = 402,*/
    HSF_R32_FLOAT,          HSF_UNKNOWN,     D24_UNORM_S8_UINT,   CP_Z24,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,/*HSF_D24_UNORM_S8_UINT    = 403,*/
    HSF_R32_FLOAT,          HSF_UNKNOWN,     D32_FLOAT_S8_UINT,   CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_S8_UINT_D32_FLOAT    = 404,*/
    HSF_R24_UNORM,          HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_R,    0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R24_UNORM   = 405,*/
    HSF_R24G24_UNORM,       HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_GR,   0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R24G24_UNORM   = 406,*/
    HSF_R24G24B24A24_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R24G24B24A24_UNORM   = 407,*/
    HSF_R10G10B10A10_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_BGRA, 0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,/*HSF_R10G10B10A10_UNORM    = 408,*/
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  4,  4,  1,  0,/*HSF_BC1_TYPELESS            = 409,*/
    HSF_B8G8R8A8_TYPELESS,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,/*HSF_B8G8R8A8_TYPELESS            = 410,*/
    HSF_B8G8R8X8_TYPELESS,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,/*HSF_B8G8R8X8_TYPELESS            = 411,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  4,  4,  1,  0,/*HSF_BC2_TYPELESS            = 412,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  4,  4,  1,  0,/*HSF_BC3_TYPELESS            = 413,*/
    HSF_R16G16B16A16_UNORM, HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  4,  4,  1,  0,/*HSF_BC4_TYPELESS            = 414,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  4,  4,  1,  0,/*HSF_BC5_TYPELESS            = 415,*/
    HSF_R32G32B32A32_UINT,  HSF_UNKNOWN,          DS_NONE,        CP_OFF,         C_NONE, 0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  4,  4,  1,  0,/*DXGI_FORMAT_BC6H_TYPELESS   = 416,*/
};
const HWFORMAT_TABLE_ENTRY_E3K *g_pHwFormatTable_e3k = g_HwFormatTable_e3k;

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define E3K_VRAM_LOCAL_USAGES       (GF_USAGE_DISPLAY_SURFACE | \
                                     GF_USAGE_OVERLAY_SURFACE | \
                                     GF_USAGE_CURSOR_SURFACE | \
                                     GF_USAGE_SHADOW_SURFACE | \
                                     GF_USAGE_RENDER_TARGET   | \
                                     GF_USAGE_TEXTURE_BUFFER  | \
                                     GF_USAGE_SHADER_BUFFER)

#define E3K_VRAM_NONLOCAL_USAGES   (GF_USAGE_DATA_BUFFER | \
                                     GF_USAGE_TEMP_BUFFER)

/* if such usage set, this allocation must use for display */
#define E3K_DIU_DISPLAY_USAGES      (GF_USAGE_DISPLAY_SURFACE | \
                                     GF_USAGE_OVERLAY_SURFACE | \
                                     GF_USAGE_CURSOR_SURFACE)

static int vidmmi_update_segment_id_e3k(adapter_t *adapter, vidmm_allocation_t *allocation, int segment_id)
{
    int segment_id_new = segment_id;

    //valid secure range segment, now id 4~10 stand for secure range1~7 in rm.
    //kmd only reserved 2 secure range mem.
    if(segment_id >= 4 && segment_id <= 10 && allocation->flag.secured)
    {
        segment_id_new = adapter->hw_caps.secure_range_enable ? 
             ((segment_id == 4)?SEGMENT_ID_SECURE_RANGE_E3K: SEGMENT_ID_SECURE_RANGE_E3K_1): SEGMENT_ID_LOCAL_E3K;
    }

//validate here for both (escapse,ioctl) umd should not know the details of memory of different platform
    switch(segment_id)
    {
        case 1:
            //for local, if cpu visible + no-compress + linear, must be in local. but aarm64 has alignment 
            //requirement for local memory.(by aarch64, device memory).
            if(allocation->flag.cpu_visible  && allocation->compress_format == 0 && allocation->flag.swizzled == 0)
            {
#if defined(__aarch64__)
               // this is cpu limitation, put to gart for cpu safe use.
               if (!gf_strcmp(allocation->device->pname, "glmark2")
                   && (allocation->at_type == MM_AT_VERTEXBUFFER_E3K || allocation->at_type == MM_AT_INDEXBUFFER_E3K))
               {
                   segment_id_new = SEGMENT_ID_LOCAL_E3K;
               }
               else
               {
                   segment_id_new = SEGMENT_ID_GART_SNOOPABLE_E3K;
               }
#else
               segment_id_new = SEGMENT_ID_LOCAL_E3K;
#endif
            }
            else
            {
                segment_id_new = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1;
            }

            break;
        case 2:
#if defined(__aarch64__)
                //TODO : remove this limitation when study cache
                segment_id_new = SEGMENT_ID_GART_SNOOPABLE_E3K;
 #else
                segment_id_new = SEGMENT_ID_GART_UNSNOOPABLE_E3K;
 #endif
                break;
        case 3:
            segment_id_new = adapter->hw_caps.support_snooping ? SEGMENT_ID_GART_SNOOPABLE_E3K: SEGMENT_ID_GART_UNSNOOPABLE_E3K;
            break;
            
        default:
            //escape mm alloc call does not set any prefer.
            if(allocation->flag.cpu_visible  && allocation->compress_format == 0 && allocation->flag.swizzled == 0)
            {
#if defined(__aarch64__)
                segment_id_new = SEGMENT_ID_GART_SNOOPABLE_E3K;
#else
                segment_id_new = SEGMENT_ID_LOCAL_E3K;
#endif
            }
            else
            {
                segment_id_new = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1;
            }

            break;
    }


    if (segment_id_new == SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1)
    {
        vidmm_segment_t *segment = adapter->mm_mgr->segment + SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1;
        if (segment->gpu_vm_size == 0)
        {
            segment_id_new = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K;
        }
    }


    return segment_id_new;
}


#define vaildate_segment_id(id)   do {                                                \
    segment_id = allocation->preferred_segment_raw.segment_id_##id;                   \
    if(segment_id <= SEGMENT_ID_MAX_E3K)                                                               \
    {                                                                                 \
        segment_id = vidmmi_update_segment_id_e3k(adapter, allocation, segment_id); \
        if(segment_id > 0)                                                            \
        {                                                                             \
            segment_bit = 1 << segment_id;                                            \
            if(!(segment_mask & segment_bit))                                         \
            {                                                                         \
                segments[num]   = segment_id;                                         \
                segment_mask   |= segment_bit;                                        \
                num++;                                                                \
            }                                                                         \
        }                                                                             \
    }                                                                                 \
    }while(0);

#define preferred_segment_id(id)         allocation->preferred_segment.segment_id_##id
#define preferred_segment_id_raw(id)     allocation->preferred_segment_raw.segment_id_##id


static void vidmmi_validate_allocation_info_e3k(vidmm_mgr_t *mm_mgr, vidmm_allocation_t *allocation)
{
    adapter_t *adapter = mm_mgr->adapter;

    int segments[MAX_SEGMENT_ID]   = {0};
    int directions[MAX_SEGMENT_ID] = {0};
    int segment_id  = 0;
    int direction   = 0;
    int segment_bit = 0;
    int segment_mask= 0;
    int num         = 0;
    int snoop_only = adapter->hw_caps.snoop_only;
    int has_snoop  = adapter->hw_caps.support_snooping;

    //for secure debug only
    if(adapter->ctl_flags.debug_secure && 
            (allocation->at_type == MM_AT_TEXTURE_E3K    ||
            allocation->at_type == MM_AT_RENDERTARGET_E3K ||
            allocation->at_type == MM_AT_BACKBUFFER_E3K))
    {
        if(adapter->ctl_flags.debug_secure == 1)
        {
            allocation->preferred_segment_raw.segment_id_0 = 4; // 4 ->range 1, 5 ->range2
            allocation->flag.secured = 1;
        }
        else if(adapter->ctl_flags.debug_secure == 2)
        {
            allocation->preferred_segment_raw.segment_id_0 = 5; // 4 ->range 1, 5 ->range2
            allocation->flag.secured = 1;
        }
    }

    //only using prefer 0/1 from upper. update prefer sement id by prefer raw's pool type
    vaildate_segment_id(0);
    segments[0] = segment_id;
    vaildate_segment_id(1);
    segments[1] = segment_id;

    if (!allocation->flag.cpu_visible)
    {
       segments[2] = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K;
    }
    else
    {
        segments[2] = SEGMENT_ID_LOCAL_E3K;
    }

    //cannot be PCIE memory when allocation is compressed or allocation is used for display
    if(allocation->compress_format != 0 || allocation->flag.primary == 1)
    {
        segments[3] = SEGMENT_ID_LOCAL_E3K;
    }
    else
    {
#ifdef __mips64__
    segments[3] = SEGMENT_ID_GART_UNSNOOPABLE_E3K;
#else
    segments[3] = SEGMENT_ID_GART_SNOOPABLE_E3K;
#endif
    }
    segments[4] = SEGMENT_ID_LOCAL_E3K;

    //following code is used as force segemnt id case: as hw limitation, debug and etc.

    
    //local only must case
    if(allocation->flag.primary || allocation->flag.bVideoInternal|| 
        adapter->hw_caps.local_only
#ifdef __aarch64__
       || allocation->flag.bHwctxBuffer
#endif
      )
    {
         if(allocation->flag.cpu_visible)
        {
            segments[0] = SEGMENT_ID_LOCAL_E3K;
            segments[1] = 0;                
        }
        else//cpu unvisible
        {
            if(allocation->flag.bVideoInternal)//video must <4G
            {
                segments[0] = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K;
                segments[1] = 0;                
            }
            else//3d prefer >4G first, than <4G
            {
                if (mm_mgr->segment[SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1].gpu_vm_size > 0)
                {
                    segments[0] = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K_1;
                    segments[1] = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K;
                    segments[2] = SEGMENT_ID_LOCAL_E3K;
                }
                else
                {
                    segments[0] = SEGMENT_ID_LOCAL_CPU_UNVISIABLE_E3K;
                    segments[1] = SEGMENT_ID_LOCAL_E3K;
                }
            }
        }
    }

#ifdef __aarch64__
    if (!gf_strcmp(allocation->device->pname, "glxgears"))
    {
        if (allocation->at_type == MM_AT_VERTEXBUFFER_E3K ||
            allocation->at_type == MM_AT_INDEXBUFFER_E3K)
        {
            int i;
            for (i = 0; i < MAX_SEGMENT_ID; i++)
            {
                if (segments[i] == SEGMENT_ID_GART_SNOOPABLE_E3K)
                {
                    segments[i] = SEGMENT_ID_LOCAL_E3K;
                }
            }
        }
    }
    if (allocation->at_type == MM_AT_CONTEXT_E3K)
    {
        int i;
        for (i = 0; i < MAX_SEGMENT_ID; i++)
        {
            if (segments[i] == SEGMENT_ID_GART_SNOOPABLE_E3K)
            {
                segments[i] = SEGMENT_ID_LOCAL_E3K;
            }
        }
    }
    /*
    // set pcie memory to cpu visible local memory.
    if (allocation && allocation->flag.video &&
        allocation->flag.cpu_visible &&
        (allocation->hw_format == HSF_B8G8R8A8_UNORM ||
        allocation->hw_format == HSF_R10G10B10A2_UNORM ||
        allocation->hw_format == HSF_R8G8B8A8_UNORM ||
        allocation->hw_format == HSF_B10G10R10A2_UNORM ||
        allocation->hw_format == HSF_NV12 ||
        allocation->hw_format == HSF_P010 ||
        allocation->hw_format == HSF_YUYV ||
        allocation->hw_format == HSF_AYUV_VIDEO ||
        allocation->hw_format == HSF_B5G6R5_UNORM))
    {
        if (segments[0] == SEGMENT_ID_GART_SNOOPABLE_E3K)
        {
            segments[0] = SEGMENT_ID_LOCAL_E3K;
            segments[1] = 0;
        }
    }
    */
#endif
    preferred_segment_id(0) = segments[0];
    preferred_segment_id(1) = segments[1];
    preferred_segment_id(2) = segments[2];
    preferred_segment_id(3) = segments[3];
    preferred_segment_id(4) = segments[4];

    if (allocation->flag.primary)
    {
        allocation->alignment = max(0x8000, allocation->alignment);
    }
}

static void vidmmi_check_allocation_flag_e3k(adapter_t *adapter, vidmm_allocation_t  *allocation, gf_create_allocation_t *hint, unsigned char compress)
{
    unsigned int     usage_mask = hint->usage_mask;
    gf_access_hint access_hint  = hint->access_hint;

    allocation->flag.swizzled   = hint->tiled;

    if(usage_mask & (E3K_DIU_DISPLAY_USAGES))
    {
       allocation->flag.primary  = 1;
       if(hint->tiled)
       {
           gf_warning("%s(): display surface must be linear. but setting as tiled.\n", util_remove_name_suffix(__func__));
       }
    }

    allocation->flag.cpu_visible = (access_hint != GF_ACCESS_GPU_ONLY);
    allocation->flag.unpagable = (hint->unpagable || !adapter->ctl_flags.paging_enable);

    if(allocation->flag.secured)
    {
        allocation->flag.unpagable = 1;
    }

    if (hint->usage_mask & (GF_USAGE_DISPLAY_SURFACE | GF_USAGE_RENDER_TARGET))
    {
        allocation->flag.force_clear = TRUE;
    }
#ifdef __aarch64__
    if (hint->usage_mask & GF_USAGE_HWCTX_BUFFER)
    {
        allocation->flag.bHwctxBuffer = 1;
    }
#endif
}

static inline unsigned int vidmmi_get_allocation_hw_format_e3k(gf_format format)
{
    Hw_Surf_Format hw_format = 0;

    switch(format)
    {
    case GF_FORMAT_A8_UNORM:
        hw_format = HSF_A8_UNORM;
        break;

    case GF_FORMAT_B5G6R5_UNORM:
        hw_format = HSF_B5G6R5_UNORM;
        break;

    case GF_FORMAT_B8G8R8A8_UNORM:
        hw_format = HSF_B8G8R8A8_UNORM;
        break;

    case GF_FORMAT_B8G8R8X8_UNORM:
        hw_format = HSF_B8G8R8X8_UNORM;
        break;

    case GF_FORMAT_R8G8B8A8_UNORM:
        hw_format = HSF_R8G8B8A8_UNORM;
         break;
  
    case GF_FORMAT_A8R8G8B8_UNORM:
        hw_format = HSF_A8R8G8B8_UNORM;
        break;
		
    case GF_FORMAT_R8G8B8X8_UNORM:
        hw_format = HSF_R8G8B8X8_UNORM;
        break;

    case GF_FORMAT_YUY2:
        hw_format = HSF_YUY2;
        break;

    case GF_FORMAT_NV12_LINEAR:
        hw_format = HSF_NV12;
        break;

    case GF_FORMAT_NV12_TILED:
        hw_format = HSF_YCRCB_MB_8_420;
        break;

    case GF_FORMAT_NV21_LINEAR:
        hw_format = HSF_NV12;
        break;

    case GF_FORMAT_YV12:
        hw_format = HSF_YV12;
        break;

    case GF_FORMAT_FLOAT32:
        hw_format = HSF_D32_FLOAT;
        break;

    case GF_FORMAT_UINT32:
        hw_format = HSF_R32_UINT;
        break;

    case GF_FORMAT_INT32:
        hw_format = HSF_R32_SINT;
        break;
    case GF_FORMAT_R8G8_UNORM:
        hw_format = HSF_R8G8_UNORM;
        break;
    case GF_FORMAT_R8_UNORM:
        hw_format = HSF_R8_UNORM;
        break;

    case GF_FORMAT_B10G10R10A2_UNORM:
        hw_format = HSF_A2B10G10R10_UNORM;
        break;

   case GF_FORMAT_P010:
        hw_format = HSF_P010;
        break;

    default:
        gf_error("unknow gfx format :%d.\n", format);
        gf_assert(0, "vidmmi_get_allocation_hw_format unknown gfx format ");
        break;
    }

    return hw_format;
}

static void vidmmi_set_allocation_compress_format_e3k(adapter_t *adapter, vidmm_allocation_t *allocation, gf_create_allocation_t *create_hint)
{
    COMPRESS_MXUFORMAT compress_format = CP_OFF;
    int can_compress = FALSE;

    if(create_hint->compressed)
    {
        switch (allocation->at_type)
        {
            case MM_AT_TEXTURE_E3K:
            case MM_AT_RENDERTARGET_E3K:
                can_compress = TRUE;
                break;

            default:
                break;
        }
    }

    if(can_compress)
    {
        compress_format = g_pHwFormatTable_e3k[allocation->hw_format].CompressFmt;
    }

    switch(compress_format)
    {
    case CP_R8G8B8A8_L:
        if(allocation->flag.swizzled)
        {
            compress_format = CP_R8G8B8A8_T;
        }
        break;

    case CP_A8R8G8B8_L:
        if(allocation->flag.swizzled)
        {
            compress_format = CP_A8R8G8B8_T;
            if (allocation->hw_format == HSF_B8G8R8A8_UNORM)
            {
                compress_format = CP_R8G8B8A8_T;
            }
        }
        break;

    case CP_TOTAL_RANGE:
       compress_format = CP_OFF;
       break;

    default:
        break;
    }

    allocation->compress_format = compress_format;
}


#define E3K_32K_ALIGNMENT_USAGES    (GF_USAGE_DISPLAY_SURFACE | \
                                     GF_USAGE_SHADOW_SURFACE | \
                                     GF_USAGE_OVERLAY_SURFACE | \
                                     GF_USAGE_CURSOR_SURFACE  | \
                                     GF_USAGE_RENDER_TARGET)

#define E3K_4K_ALIGNMENT_USAGES     (GF_USAGE_DATA_BUFFER | \
                                     GF_USAGE_SHADER_BUFFER)

//port from mmGetAllocationAlingment_e3k
static unsigned int vidmmi_get_allocation_alignment_e3k(adapter_t *adapter, vidmm_allocation_t *allocation, unsigned int at_type)
{
    unsigned int alignment = 0;

    switch (at_type)
    {
    case MM_AT_TEXTURE_E3K:
    case MM_AT_RENDERTARGET_E3K:
        if(allocation->compress_format)
        {
            alignment = 0x8000;
        }
        else if(allocation->flag.swizzled)
        {
            alignment= 0x10000;
        }
        else
        {
            alignment = 0x100;
        }
        break;
    case MM_AT_VERTEXBUFFER_E3K:
    case MM_AT_EUPFSHADER_E3K:
        alignment = 0x0100;
        break;
    case MM_AT_GENERIC_E3K:
        alignment = 0x8000;
        break;
    case MM_AT_MIUCOUNTER_E3K:
        alignment = 0x10000;
        break;
    default:
        alignment = 0x8000;
        gf_info("unknown at_type. set default aligned to 32K.\n");
        break;
    }

    return alignment;
}


static inline unsigned int vidmmi_get_allocation_pool_type_e3k(adapter_t *adapter, unsigned int usage_mask, gf_access_hint access_hint, int try_secure)
{
    vidmm_segment_preference_t segment = {0};
    int snoop_only = adapter->hw_caps.snoop_only;
    
    //to same as cil2 mm logic
    //1 means to allocate in local video memory
    //2 means to allocate in non-local non-snoop video memory
    //3 means to allocate in non-local snoop video memory.

    if(try_secure && adapter->hw_caps.secure_range_enable)
    {
        segment.segment_id_0 = SEGMENT_ID_SECURE_RANGE_E3K;
    }
    else if(usage_mask & GF_USAGE_FORCE_PCIE)
    {
        segment.segment_id_0 = 0x3; // snoop for cpu fast access
    }
    //local video memory case
    else if(usage_mask & E3K_VRAM_LOCAL_USAGES)
    {
        segment.segment_id_0 = 0x1;
    }
    //non local video memory case
    else if(usage_mask & E3K_VRAM_NONLOCAL_USAGES)
    {
        if(access_hint == GF_ACCESS_CPU_ALMOST)
        {
            segment.segment_id_0 = 3;
        }
        else
        {
            segment.segment_id_0 = 2;
        }
    }
    //default: not care memory location case
    else
    {
#if defined(__aarch64__)
        segment.segment_id_0 = 0x3;//for safe to set cpu visible.
#else
        //2d not considering cacheable case, only set to non local.
        segment.segment_id_0 = 0x3;
        segment.segment_id_1 = 0x2;
#endif
    }

    return segment.value;
}

//port from rmiCalcResourcePitch_e3k() and  rmiCalcHwLinearPittch_e3k()
void vidmmi_calc_allocation_pitch_e3k(vidmm_allocation_t *allocation)
{
    unsigned int  BitCount;
    unsigned int  Width0, Height0;
    unsigned int  TileWidth, TileHeight;
    unsigned int  UnitWidth, UnitHeight;
    unsigned int  P2Width0, P2Height0;
    unsigned int  UnitSize;
    unsigned int  Pitch, Pitch1 = 0;
    unsigned int  WidthAligned;
    unsigned int  HeightAligned, yHeightAligned = 0;

    BitCount   = allocation->bit_count;
    TileWidth  = calcTileWidth_e3k(BitCount);
    TileHeight = calcTileHeight_e3k(BitCount);
    UnitSize   = UNIT_SIZE_E3K;
    UnitWidth  = calcUnitWidth_e3k(BitCount);
    UnitHeight = calcUnitHeight_e3k(BitCount);

    Width0     = allocation->width;
    Height0    = allocation->height;

    // overflow check
    if ( Width0            >= (0xFFFFFFFF / Height0) ||
        (Width0 * Height0) >= (0xFFFFFFFF / BitCount))
    {
        gf_error("invlad argument, width or height too big. width:%d, height:%d.\n", Width0, Height0);
        return;
    }

    if (allocation->flag.swizzled)
    {
        P2Width0  = (Width0 & (Width0 - 1)) ?
                util_get_msb_position(Width0) + 1:
                util_get_msb_position(Width0);
        P2Width0  = 1 << P2Width0;

        P2Height0 = (Height0&(Height0-1))?
                    util_get_msb_position(Height0) + 1:
                    util_get_msb_position(Height0);
        P2Height0 = 1 << P2Height0;

        P2Width0  = max(P2Width0, UnitWidth);
        P2Height0 = max(P2Height0, UnitHeight);

        //Folding is disabled for NV12 tiled surface, we need make NV12 surface tiled aligned.
        if (allocation->hw_format == HSF_NV12 || allocation->hw_format == HSF_P010)
        {
            P2Width0  = P2Width0 >= TileWidth ? P2Width0 : TileWidth;
            P2Height0 = P2Height0 >= TileHeight ? P2Height0 : TileHeight;
        }

        if ((P2Width0 >= TileWidth) && (P2Height0 >= TileHeight))
        {
            //pResource->Flags.TileAligned = 1;

            WidthAligned  = calcWidthInTiles_e3k(Width0, BitCount) * TileWidth;
            HeightAligned = calcHeightInTiles_e3k(Height0, BitCount) * TileHeight;
        }
        // P2 Aligned
        else
        {
            //pResource->Flags.TileAligned = 0;
            WidthAligned   = P2Width0;
            HeightAligned  = P2Height0;
        }
        Pitch =((WidthAligned * BitCount +7)/8);
    }
    else
    {
        //port rmiCalcHwLinearPitch_e3k()
        // TODO: how to calc the pitch for linear source
        WidthAligned  = (Width0) ? Width0 : 1;
        Pitch = ALIGNED_2KBits((WidthAligned * BitCount  + 7)/8);

        WidthAligned   = Pitch * 8 / BitCount;
        HeightAligned  = allocation->height;
    }

    //Get Pitch Size
    allocation->tiled_width    = TileWidth;
    allocation->tiled_height   = TileHeight;
    allocation->aligned_width  = WidthAligned;
    allocation->aligned_height = HeightAligned;

    allocation->orig_size      = Pitch * HeightAligned;

    //allocation->orig_size = ALIGNED_2KBits( allocation->orig_size);
    allocation->pitch          = Pitch;
}

static unsigned int vidmmi_get_allocation_type_e3k(unsigned int usage_mask)
{
    unsigned int at_type = MM_AT_GENERIC_E3K;

    if(usage_mask & GF_USAGE_MIU_COUNTER)
    {
        at_type = MM_AT_MIUCOUNTER_E3K;
    }
    else if(usage_mask & (GF_USAGE_DISPLAY_SURFACE |
                     GF_USAGE_OVERLAY_SURFACE |
                     GF_USAGE_SHADOW_SURFACE  |
                     GF_USAGE_CURSOR_SURFACE  |
                     GF_USAGE_RENDER_TARGET))
    {
        at_type = MM_AT_RENDERTARGET_E3K;
    }
    else if(usage_mask & GF_USAGE_TEXTURE_BUFFER)
    {
        at_type = MM_AT_TEXTURE_E3K;
    }
    else if(usage_mask & GF_USAGE_SHADER_BUFFER)
    {
        at_type = MM_AT_EUPFSHADER_E3K;
    }
    else if(usage_mask & GF_USAGE_DATA_BUFFER)
    {
        at_type = MM_AT_VERTEXBUFFER_E3K;
    }
    else if(usage_mask & GF_USAGE_TEMP_BUFFER)
    {
        at_type = MM_AT_GENERIC_E3K;
    }

    return at_type;
}

int vidmm_describe_allocation_e3k(adapter_t *adapter, vidmm_describe_allocation_t *desc_info)
{
    vidmm_allocation_t *allocation = desc_info->allocation;

    int y_tile = 0;
    int status = S_OK;

    if(desc_info->create_info != NULL)
    {
        gf_create_alloc_info_t *desc = desc_info->create_info;

        y_tile = TILE_SIZE_Y(allocation->bit_count);
        allocation->aligned_width  = (allocation->pitch * 8)/allocation->bit_count;
        allocation->tiled_width    = NUM_OF_X_TILES(allocation->aligned_width, allocation->bit_count);//tile num of x
        allocation->aligned_height = (allocation->height + y_tile -1) & ~(y_tile - 1);
        allocation->tiled_height   = NUM_OF_Y_TILES(allocation->height, allocation->bit_count);
        allocation->compress_format= desc->HWCompressFormat;
        allocation->flag.secured   = desc->bSecurity;
        allocation->flag.bVideoInternal = desc->bVideoInternal;        

        vidmmi_validate_allocation_info_e3k(adapter->mm_mgr, allocation);

        if(allocation->at_type == MM_AT_RT_TEXTURE_E3K    ||
            allocation->at_type == MM_AT_RENDERTARGET_E3K  ||
            allocation->at_type == MM_AT_STENCILBUFFER_E3K ||
            allocation->at_type == MM_AT_DEPTHBUFFER_E3K ||
            allocation->at_type == MM_AT_OCL_SPECIAL_BUFFER_E3K)
        {
            allocation->flag.force_clear = 1;
        }

        vidmm_trace("create_allocation: allocation:%p, handle: %x, width:%x, height:%x,pitch: %x. y_tile:%x, size:%x\n", 
            allocation, allocation->handle, allocation->width, allocation->height, allocation->pitch, y_tile, allocation->orig_size);
    }
    else if(desc_info->create_gf != NULL)
    {
        gf_create_allocation_t *create_hint = desc_info->create_gf->create_data;

        allocation->width           = create_hint->width;
        allocation->height           = create_hint->height;
        allocation->priority         = PMAX;
        allocation->at_type         = vidmmi_get_allocation_type_e3k(create_hint->usage_mask);
        allocation->hw_format    = vidmmi_get_allocation_hw_format_e3k(create_hint->format);
        allocation->alignment     = vidmmi_get_allocation_alignment_e3k(adapter, allocation, allocation->at_type);
        allocation->bit_count      = __BitCountTable_e3k[allocation->hw_format];

        /******************************************************************
        * 1. modify allocation flags according usage, such as cpu visible *
        * 2. check confict about usage and create flags.                  *
        ******************************************************************/
        vidmmi_check_allocation_flag_e3k(adapter, allocation, create_hint, allocation->compress_format);

        //calc pitch according to tile or linear.
        vidmmi_calc_allocation_pitch_e3k(allocation);

        //if compress, set compress format, e3k only setting rgba_compress now
        vidmmi_set_allocation_compress_format_e3k(adapter, allocation, create_hint);//set compress format

        /***********************************************************************
        * get allocation's pool type according to usage,access hint and hw caps*
        * pool type 1 means local video memory                                 *
        * pool type 2 means non-local non-snoop video memory                   *
        * pool type 3 means non-local snoop video memory.                      *
        ***********************************************************************/
        allocation->preferred_segment_raw.value = 
            vidmmi_get_allocation_pool_type_e3k(adapter, create_hint->usage_mask, create_hint->access_hint, create_hint->secured);


        /*********************************************************************
        * 1. update segment id accoding to pool type, no usage/hint will     *
             be used, only considering allocation flags and hw caps to update*
        * 2. validate segment id according by hw limitation and etc          *
        *********************************************************************/
        vidmmi_validate_allocation_info_e3k(adapter->mm_mgr, allocation);

        vidmm_trace("gf_create_allocation: allocation:%p, handle: %x, width:%x, height:%x,pitch: %x. y_tile:%x, ori size:%x\n",
            allocation, allocation->handle, allocation->width, allocation->height, allocation->pitch, y_tile, allocation->orig_size);
    }
    else
    {
        status = E_INVALIDARG;
    }
    
    return status;
}

int vidmm_get_allocation_info_e3k(adapter_t *adapter, vidmm_get_allocation_info_t *info)
{
    vidmm_allocation_t    *allocation = info->allocation;

    info->ForceLocal = (allocation->preferred_segment.segment_id_0 == SEGMENT_ID_LOCAL_E3K) &&
        (allocation->preferred_segment.segment_id_1 == 0 || allocation->preferred_segment.segment_id_1 == SEGMENT_ID_LOCAL_E3K) &&
        (allocation->preferred_segment.segment_id_2 == 0 || allocation->preferred_segment.segment_id_2 == SEGMENT_ID_LOCAL_E3K) &&
        (allocation->preferred_segment.segment_id_3 == 0 || allocation->preferred_segment.segment_id_3 == SEGMENT_ID_LOCAL_E3K) &&
        (allocation->preferred_segment.segment_id_4 == 0 || allocation->preferred_segment.segment_id_4 == SEGMENT_ID_LOCAL_E3K);
    info->snoop = (allocation->segment_id == SEGMENT_ID_GART_SNOOPABLE_E3K) ? 1 : 0;
    return S_OK;
}

