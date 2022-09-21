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


#ifndef _CBIOSDIU_CSC_H_
#define _CBIOSDIU_CSC_H_

//these maro is used for get Coefx and birghtness's 2's complement
#define CSC_COEF_MODULE 14
#define CSC_BRIGHT_MODULE 9

#define CSC_MAX_CONTRAST        2
#define CSC_MIN_CONTRAST        0
#define CSC_DEFAULT_CONTRAST    1

#define CSC_MAX_SATURATION      2
#define CSC_MIN_SATURATION      0
#define CSC_DEFAULT_SATURATION  1

#define CSC_MAX_HUE             180
#define CSC_MIN_HUE             -180
#define CSC_DEFAULT_HUE         0

#define CSC_HUE_MATRIX_EXPAND_TIMES        0x100000
#define CSC_CONTRAST_MATRIX_EXPAND_TIMES   0x80
#define CSC_SATURATION_MATRIX_EXPAND_TIMES 0x80
#define CSC_YCBCR_RGB_MATRIX_EXPAND_TIMES  0x100000

#define CSC_HUE_MATRIX_SHIFT_BITS        20
#define CSC_CONTRAST_MATRIX_SHIFT_BITS   7
#define CSC_SATURATION_MATRIX_SHIFT_BITS 7
#define CSC_YCBCR_RGB_MATRIX_SHIFT_BITS  20  //expand 0x100000 times,need shift 20bit

#define CSC_TOTAL_MATRIX_EXPAND_TIMES CSC_HUE_MATRIX_EXPAND_TIMES*CSC_CONTRAST_MATRIX_EXPAND_TIMES* \
                                        CSC_SATURATION_MATRIX_EXPAND_TIMES*CSC_YCBCR_RGB_MATRIX_EXPAND_TIMES

#define CSC_TOTAL_MAXTRIX_SHIFT_BITS   (CSC_HUE_MATRIX_SHIFT_BITS + CSC_CONTRAST_MATRIX_SHIFT_BITS+ \
                                        CSC_SATURATION_MATRIX_SHIFT_BITS + CSC_YCBCR_RGB_MATRIX_SHIFT_BITS)


#define CSC_MODE_HASH_VALUE  7

//value of CSC_XXX_TO_XXX  =  CSC_MODE_HASH_VALUE * input format + output format
#define CSC_RGB_TO_RGB                  0
#define CSC_RGB_TO_YCBCR601             4
#define CSC_RGB_TO_YCBCR709             5
#define CSC_RGB_TO_YCBCR2020            6
#define CSC_LIMITED_RGB_TO_RGB          7   
#define CSC_LIMITED_RGB_TO_LIMITED_RGB  8
#define CSC_LIMITED_RGB_TO_YCBCR601     11
#define CSC_LIMITED_RGB_TO_YCBCR709     12
#define CSC_LIMITED_RGB_TO_YCBCR2020    13
#define CSC_YCBCR601_TO_RGB             28
#define CSC_YCBCR601_TO_LIMITED_RGB     29
#define CSC_YCBCR601_TO_YCBCR601        32
#define CSC_YCBCR601_TO_YCBCR709        33
#define CSC_YCBCR601_TO_YCBCR2020       34
#define CSC_YCBCR709_TO_RGB             35
#define CSC_YCBCR709_TO_LIMITED_RGB     36
#define CSC_YCBCR709_TO_YCBCR601        39
#define CSC_YCBCR709_TO_YCBCR709        40
#define CSC_YCBCR709_TO_YCBCR2020       41
#define CSC_YCBCR2020_TO_RGB            42
#define CSC_YCBCR2020_TO_LIMITED_RGB    43
#define CSC_YCBCR2020_TO_YCBCR601       46
#define CSC_YCBCR2020_TO_YCBCR709       47
#define CSC_YCBCR2020_TO_YCBCR2020      48

extern CBIOS_S64 YCbCr601_fr_RGB[][3];
extern CBIOS_S64 RGB_fr_YCbCr601[][3];
extern CBIOS_S64 YCbCr709_fr_RGB[][3];
extern CBIOS_S64 RGB_fr_YCbCr709[][3];
extern CBIOS_S64 YCbCr2020_fr_RGB[][3];// non constant luminance only
extern CBIOS_S64 RGB_fr_YCbCr2020[][3];
extern CBIOS_S64 LimitedRGB_fr_RGB[][3];
extern CBIOS_S64 RGB_fr_LimitedRGB[][3];
extern CBIOS_S64 YCbCr709_fr_YCbCr601[][3];
extern CBIOS_S64 YCbCr601_fr_YCbCr709[][3];
extern CBIOS_S64 Balance_Times_matrix[][3];

extern CBIOS_S32  CSC_cos[];
extern CBIOS_S32  CSC_sin[];

#endif

