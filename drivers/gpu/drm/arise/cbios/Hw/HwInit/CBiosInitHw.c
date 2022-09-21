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


/*****************************************************************************
** DESCRIPTION:
** CBios hw initialization functions, mainly POST and chip initialize.
**
** NOTE:
** The functions in this file are hw layer internal functions, 
** CAN ONLY be called by files under Hw folder. 
******************************************************************************/

#include "CBiosChipShare.h"
#include "../CBiosHwShare.h"
#include "../HwBlock/CBiosDIU_HDAC.h"

#define QT_DDR3_2CH        CBIOS_FALSE
#define QT_DDR3_1CH        CBIOS_FALSE
#define FPGA_DDR2          CBIOS_FALSE
#define QT_DDR4_2CH        CBIOS_TRUE

#define CBIOS_CLOCK_MAX   1200
#define CBIOS_CLOCK_MIN   200
#define CBIOS_ECLK_DEFAULT 350
#define CBIOS_ICLK_DEFAULT 700
#define CBIOS_VCLK_DEFAULT 350

#define E3K_PMPTAG_OFFSET    0x70020
#define E3K_PMPVER_OFFSET    0x70024
#define E3K_PMPTAG_SIZE    4
#define E3K_PMPVER_SIZE    64



#if    QT_DDR3_2CH
static MMIOREGISTER Mclk_300_Timing_Desktop[]={
    {0x0000850c, 0x00000002, 32},
    {0x00005040, 0x00000000, 32},
    {0x00005044, 0xfcfcfcfc, 32},
    {0x00005100, 0xffffffff, 32},
    {0x00005104, 0xffffffff, 32},
    {0x00005108, 0x00000000, 32},
    {0x0000510c, 0x00000000, 32},

    {0x00005148, 0xc2ff3800, 32},

    {0x00005158, 0x67380503, 32},
    {0x0000515c, 0x1f83177f, 32},
    {0x00005160, 0x88008000, 32},
    {0x00005164, 0x00000000, 32},
    {0x00005168, 0x5d004e4e, 32},
    {0x0000516c, 0x48118010, 32},

    {0x00005250, 0x00ff0000, 32},
    {0x0000516c, 0x48118030, 32},
    {0x00005118, 0xba98ba98, 32},

    {0x0000511c, 0xb4, 8},
    {0x0000511d, 0x5a, 8},
    {0x0000511e, 0xb0, 8},       
    {0x00005120, 0x44, 8},
    {0x00005121, 0x82, 8},

    {0x00005126, 0x90, 8},
    {0x0000512c, 0x01, 8},
    {0x0000512d, 0x80, 8},
    {0x0000512f, 0x00, 8},

    {0x00005138, 0x00000000, 32},

    {0x0000513d, 0x00, 8},
    {0x0000513e, 0x33, 8},
    {0x0000513f, 0xa6, 8},
    {0x00005140, 0x26108804, 32},
    {0x00005145, 0x08, 8},  

    {0x00005110, 0x04040404, 32},
    {0x00005114, 0x44444444, 32},
    {0x00005148, 0xc2ff3800, 32},

    {0x0000515c, 0x1f83177f, 32},

    {0x00005161, 0x80, 8},
    {0x00005164, 0x00, 8},
    {0x00005165, 0x00, 8},

    {0x0000516a, 0x00, 8},

    {0x00005170, 0x40, 8},
    {0x00005171, 0x00, 8},
    {0x00005172, 0x15, 8},

    {0x00005176, 0x12, 8},
    {0x00005177, 0x40, 8},

    {0x00005181, 0x44, 8},
    {0x00005191, 0x00, 8},
    {0x00005203, 0x02, 8},

    {0x00005390, 0x90909090, 32},
    {0x00005394, 0x90909090, 32},
    {0x00005398, 0x90909090, 32},
    {0x0000539c, 0x90909090, 32},
    {0x000053a0, 0x90909090, 32},
    {0x000053a4, 0x90909090, 32},
    {0x000053a8, 0x90909090, 32},
    {0x000053ac, 0x90909090, 32},
    {0x000053b0, 0x90909090, 32},
    {0x000053b4, 0x90909090, 32},
    {0x000053b8, 0x90909090, 32},
    {0x000053bc, 0x90909090, 32},
    {0x000053c0, 0x90909090, 32},
    {0x000053c4, 0x90909090, 32},
    {0x000053c8, 0x90909090, 32},
    {0x000053cc, 0x90909090, 32},
    {0x000053d0, 0x90909090, 32},
    {0x000053d4, 0x90909090, 32},


    {0x00005a00, 0x48484848, 32},
    {0x00005a04, 0x48484848, 32},
    {0x00005a08, 0x48484848, 32},
    {0x00005a0c, 0x48484848, 32},
    {0x00005a10, 0x00004848, 32},
    {0x00005a24, 0x6a6a6a6a, 32},
    {0x00005a28, 0x6a6a6a6a, 32},
    {0x00005a2c, 0x6a6a6a6a, 32},
    {0x00005a30, 0x6a6a6a6a, 32},
    {0x00005a34, 0x6c6c6a6a, 32},
    {0x00005a38, 0x6c6c6c6c, 32},
    {0x00005a3c, 0x6c6c6c6c, 32},
    {0x00005a40, 0x6c6c6c6c, 32},
    {0x00005a44, 0x6c6c6c6c, 32},
    {0x00005a48, 0x48484848, 32},
    {0x00005a4c, 0x48484848, 32},
    {0x00005a50, 0x48484848, 32},
    {0x00005a54, 0x48484848, 32},
    {0x00005a58, 0x00004848, 32},
    {0x00005a6c, 0x00000000, 32},
    {0x00005a70, 0x00000000, 32},
    {0x00005a74, 0x00000000, 32},
    {0x00005a78, 0x00000000, 32},
    {0x00005a7c, 0x00000000, 32},
    {0x00005a80, 0x00000000, 32},
    {0x00005a84, 0x00000000, 32},
    {0x00005a88, 0x00000000, 32},
    {0x00005a8c, 0x00000000, 32},
    {0x00005d5c, 0x2e2e0000, 32},
    {0x00005d60, 0x00000000, 32},
    {0x00005e0c, 0x00626200, 32},
    {0x00005e18, 0x04000000, 32},
    {0x00005e1c, 0x04040404, 32},
    {0x00005e20, 0x04040404, 32},
    {0x00005e24, 0x04040404, 32},
    {0x00005e28, 0x04040404, 32},
    {0x00005e2c, 0x04040404, 32},
    {0x00005e30, 0x04040404, 32},
    {0x00005e34, 0x04040404, 32},
    {0x00005e38, 0x04040404, 32},
    {0x00005e3c, 0x00040404, 32},
    {0x00005300, 0x01010101, 32},
    {0x00005304, 0x01010101, 32},
    {0x00005308, 0x01010101, 32},
    {0x0000530c, 0x01010101, 32},
    {0x00005310, 0x01010101, 32},
    {0x00005314, 0x01010101, 32},
    {0x00005318, 0x01010101, 32},
    {0x0000531c, 0x01010101, 32},
    {0x00005320, 0x01010101, 32},
    {0x00005324, 0x01010101, 32},
    {0x00005328, 0x01010101, 32},
    {0x0000532c, 0x01010101, 32},
    {0x00005330, 0x01010101, 32},
    {0x00005334, 0x01010101, 32},
    {0x00005338, 0x01010101, 32},
    {0x0000533c, 0x01010101, 32},
    {0x00005340, 0x01010101, 32},
    {0x00005344, 0x01010101, 32},
    {0x00005348, 0x01010101, 32},
    {0x0000534c, 0x01010101, 32},
    {0x00005350, 0x01010101, 32},
    {0x00005354, 0x01010101, 32},
    {0x00005358, 0x01010101, 32},
    {0x0000535c, 0x01010101, 32},
    {0x00005360, 0x01010101, 32},
    {0x00005364, 0x01010101, 32},
    {0x00005368, 0x01010101, 32},
    {0x0000536c, 0x01010101, 32},
    {0x00005370, 0x01010101, 32},
    {0x00005374, 0x01010101, 32},
    {0x00005378, 0x01010101, 32},
    {0x0000537c, 0x01010101, 32},
    {0x00005380, 0x01010101, 32},
    {0x00005384, 0x01010101, 32},
    {0x00005388, 0x01010101, 32},
    {0x0000538c, 0x01010101, 32},
    {0x000053d8, 0x55555555, 32},
    {0x000053dc, 0x55555550, 32},
    {0x000053e0, 0x55555055, 32},
    {0x000053e4, 0x55505555, 32},
    {0x000053e8, 0x50555555, 32},
    {0x000053ec, 0x55555555, 32},
    {0x000053f0, 0x55555550, 32},
    {0x000053f4, 0x55555055, 32},
    {0x000053f8, 0x55505555, 32},
    {0x000053fc, 0x50555555, 32},
    {0x0000591d, 0x0c, 8},

    {0x0000508e, 0x01, 8},
    {0x0000508e, 0x00, 8},
    {0x0000857c, 0x80000000, 32},
    {0x00005017, 0x80, 8},
    {0x0000500b, 0x00, 8},
    {0x00008af2, 0x01, 8},
    {0x00008aa0, 0x10, 8}
};
#elif QT_DDR4_2CH
static MMIOREGISTER Mclk_300_Timing_Desktop[] = {
	{ 0x0000850c, 0x00000002, 32 },
	{ 0x00005040, 0x00000000, 32 },
	{ 0x00005044, 0x00000000, 32 },
	{ 0x00005100, 0xffff1008, 32 },
	{ 0x00005104, 0xffff1008, 32 },
	{ 0x00005108, 0xffff0800, 32 },
	{ 0x0000510c, 0xffff0800, 32 },

	{ 0x00005148, 0xc2ff3800, 32 },

	{ 0x00005158, 0x27380200, 32 },
	{ 0x0000515c, 0x32818069, 32 },
	{ 0x00005160, 0x88004040, 32 },
	{ 0x00005164, 0x00006024, 32 },
	{ 0x00005168, 0x5d184e4e, 32 },
	{ 0x0000516c, 0x08104010, 32 },

	{ 0x00005250, 0x00ff0000, 32 },
	{ 0x0000516c, 0x08104030, 32 },
	{ 0x0000516c, 0x48114030, 32 },
	{ 0x00005118, 0xba98ba98, 32 },

	{ 0x00005acc, 0x00010000, 32 },
	{ 0x00005110, 0x04040404, 32 },
	{ 0x00005114, 0x44444444, 32 },

	{ 0x0000511c, 0x00b06cd8, 32 },

	{ 0x00005120, 0x00008122, 32 },

	{ 0x00005124, 0x019007c1, 32 },

	{ 0x0000512c, 0x01, 8 },
	{ 0x0000512d, 0x80, 8 },
	{ 0x0000512f, 0x00, 8 },

	{ 0x00005138, 0x00000000, 32 },

	{ 0x0000513d, 0x00, 8 },
	{ 0x0000513e, 0x33, 8 },
	{ 0x0000513f, 0xa6, 8 },
	{ 0x00005140, 0x24908804, 32 },
	{ 0x00005145, 0x08, 8 },

	{ 0x00005148, 0xc2ff3800, 32 },

	{ 0x0000515c, 0x32828069, 32 },

	{ 0x00005161, 0x40, 8 },
	{ 0x00005164, 0x24, 8 },
	{ 0x00005165, 0x60, 8 },

	{ 0x0000516a, 0x18, 8 },

	{ 0x00005171, 0x00, 8 },
	{ 0x00005172, 0x05, 8 },

	{ 0x00005176, 0x12, 8 },
	{ 0x00005177, 0x40, 8 },

	{ 0x00005181, 0x44, 8 },
	{ 0x00005191, 0x00, 8 },
	{ 0x00005203, 0x02, 8 },

	{ 0x00005390, 0x6c6c6c6c, 32 },
	{ 0x00005394, 0x6c6c6c6c, 32 },
	{ 0x00005398, 0x6c6c6c6c, 32 },
	{ 0x0000539c, 0x6c6c6c6c, 32 },
	{ 0x000053a0, 0x6c6c6c6c, 32 },
	{ 0x000053a4, 0x6c6c6c6c, 32 },
	{ 0x000053a8, 0x6c6c6c6c, 32 },
	{ 0x000053ac, 0x6c6c6c6c, 32 },
	{ 0x000053b0, 0x6c6c6c6c, 32 },
	{ 0x000053b4, 0x6c6c6c6c, 32 },
	{ 0x000053b8, 0x6c6c6c6c, 32 },
	{ 0x000053bc, 0x6c6c6c6c, 32 },
	{ 0x000053c0, 0x6c6c6c6c, 32 },
	{ 0x000053c4, 0x6c6c6c6c, 32 },
	{ 0x000053c8, 0x6c6c6c6c, 32 },
	{ 0x000053cc, 0x6c6c6c6c, 32 },
	{ 0x000053d0, 0x6c6c6c6c, 32 },
	{ 0x000053d4, 0x6c6c6c6c, 32 },


	{ 0x00005a00, 0x48484848, 32 },
	{ 0x00005a04, 0x48484848, 32 },
	{ 0x00005a08, 0x48484848, 32 },
	{ 0x00005a0c, 0x48484848, 32 },

	{ 0x00005a10, 0x00004848, 32 },
	{ 0x00005a14, 0x00000000, 32 },
	{ 0x00005a18, 0x00000000, 32 },
	{ 0x00005a1c, 0x00000000, 32 },
	{ 0x00005a20, 0x00000000, 32 },

	{ 0x00005a24, 0x6a6a6a6a, 32 },
	{ 0x00005a28, 0x6a6a6a6a, 32 },
	{ 0x00005a2c, 0x6a6a6a6a, 32 },
	{ 0x00005a30, 0x6a6a6a6a, 32 },
	{ 0x00005a34, 0x6c6c6a6a, 32 },
	{ 0x00005a38, 0x6c6c6c6c, 32 },
	{ 0x00005a3c, 0x6c6c6c6c, 32 },
	{ 0x00005a40, 0x6c6c6c6c, 32 },
	{ 0x00005a44, 0x6c6c6c6c, 32 },
	{ 0x00005a48, 0x48484848, 32 },
	{ 0x00005a4c, 0x48484848, 32 },
	{ 0x00005a50, 0x48484848, 32 },
	{ 0x00005a54, 0x48484848, 32 },

	{ 0x00005a58, 0x00004848, 32 },
	{ 0x00005a6c, 0x00000000, 32 },
	{ 0x00005a70, 0x00000000, 32 },
	{ 0x00005a74, 0x00000000, 32 },
	{ 0x00005a78, 0x00000000, 32 },
	{ 0x00005a7c, 0x00000000, 32 },
	{ 0x00005a80, 0x00000000, 32 },
	{ 0x00005a84, 0x00000000, 32 },
	{ 0x00005a88, 0x00000000, 32 },
	{ 0x00005a8c, 0x00000000, 32 },

	{ 0x00005d5c, 0x2e2e0000, 32 },
	{ 0x00005d60, 0x00000000, 32 },
	{ 0x00005e0c, 0x00626200, 32 },
	{ 0x00005e18, 0x04000000, 32 },
	{ 0x00005e1c, 0x04040404, 32 },
	{ 0x00005e20, 0x04040404, 32 },
	{ 0x00005e24, 0x04040404, 32 },
	{ 0x00005e28, 0x04040404, 32 },
	{ 0x00005e2c, 0x04040404, 32 },
	{ 0x00005e30, 0x04040404, 32 },
	{ 0x00005e34, 0x04040404, 32 },
	{ 0x00005e38, 0x04040404, 32 },
	{ 0x00005e3c, 0x00040404, 32 },

	{ 0x00005300, 0x00000000, 32 },
	{ 0x00005304, 0x00000000, 32 },
	{ 0x00005308, 0x00000000, 32 },
	{ 0x0000530c, 0x00000000, 32 },
	{ 0x00005310, 0x00000000, 32 },
	{ 0x00005314, 0x00000000, 32 },
	{ 0x00005318, 0x00000000, 32 },
	{ 0x0000531c, 0x00000000, 32 },
	{ 0x00005320, 0x00000000, 32 },
	{ 0x00005324, 0x00000000, 32 },
	{ 0x00005328, 0x00000000, 32 },
	{ 0x0000532c, 0x00000000, 32 },
	{ 0x00005330, 0x00000000, 32 },
	{ 0x00005334, 0x00000000, 32 },
	{ 0x00005338, 0x00000000, 32 },
	{ 0x0000533c, 0x00000000, 32 },
	{ 0x00005340, 0x00000000, 32 },
	{ 0x00005344, 0x00000000, 32 },
	{ 0x00005348, 0x00000000, 32 },
	{ 0x0000534c, 0x00000000, 32 },
	{ 0x00005350, 0x00000000, 32 },
	{ 0x00005354, 0x00000000, 32 },
	{ 0x00005358, 0x00000000, 32 },
	{ 0x0000535c, 0x00000000, 32 },
	{ 0x00005360, 0x00000000, 32 },
	{ 0x00005364, 0x00000000, 32 },
	{ 0x00005368, 0x00000000, 32 },
	{ 0x0000536c, 0x00000000, 32 },
	{ 0x00005370, 0x00000000, 32 },
	{ 0x00005374, 0x00000000, 32 },
	{ 0x00005378, 0x00000000, 32 },
	{ 0x0000537c, 0x00000000, 32 },
	{ 0x00005380, 0x00000000, 32 },
	{ 0x00005384, 0x00000000, 32 },
	{ 0x00005388, 0x00000000, 32 },
	{ 0x0000538c, 0x00000000, 32 },

	{ 0x000053d8, 0x00000000, 32 },
	{ 0x000053dc, 0x00000000, 32 },
	{ 0x000053e0, 0x00000000, 32 },
	{ 0x000053e4, 0x00000000, 32 },
	{ 0x000053e8, 0x00000000, 32 },
	{ 0x000053ec, 0x00000000, 32 },
	{ 0x000053f0, 0x00000000, 32 },
	{ 0x000053f4, 0x00000000, 32 },
	{ 0x000053f8, 0x00000000, 32 },
	{ 0x000053fc, 0x00000000, 32 },
	{ 0x0000513c, 0x02, 8 },
	{ 0x0000513c, 0x06, 8 },
	{ 0x0000591d, 0x0c, 8 },

	{ 0x0000508c, 0x01, 8 },
	{ 0x0000508c, 0x00, 8 },
	{ 0x0000857c, 0x80000000, 32 },
	{ 0x00005014, 0x80, 8 },
	{ 0x0000500b, 0x00, 8 },
	{ 0x00008af2, 0x01, 8 },
	{ 0x00008aa0, 0x10, 8 }
};
#elif  QT_DDR3_1CH
static MMIOREGISTER Mclk_300_Timing_Desktop[]={
    {0x0000850c, 0x00000002, 32},
    {0x00005040, 0x00000000, 32},
    {0x00005044, 0x00000000, 32},
    {0x00005100, 0x100c0804, 32},
    {0x00005104, 0x201c1814, 32},
    {0x00005108, 0x0c080400, 32},
    {0x0000510c, 0x1c181410, 32},
    {0x00005158, 0x67380503, 32},
    {0x0000515c, 0x1f83177f, 32},
    {0x00005160, 0x88008000, 32},
    {0x00005164, 0x00000000, 32},
    {0x00005168, 0x5d004e4e, 32},
    {0x0000516c, 0x48118010, 32},
    {0x00005250, 0x00ff0000, 32},
    {0x0000516c, 0x48118030, 32},
    {0x00005118, 0xba98ba98, 32},
    {0x0000511c, 0xb4, 8},
    {0x0000511d, 0x5a, 8},
    {0x0000511e, 0xb0, 8},
    {0x00005120, 0x01, 8},
    {0x00005121, 0x80, 8},
    {0x00005126, 0x90, 8},
    {0x0000512c, 0x01, 8},
    {0x0000512d, 0x80, 8},
    {0x0000512f, 0x00, 8},
    {0x00005138, 0x00000000, 32},
    {0x0000513d, 0x00, 8},
    {0x0000513e, 0x33, 8},
    {0x0000513f, 0xa6, 8},
    {0x00005140, 0x26108804, 32},
    {0x00005145, 0x08, 8},
    {0x00005148, 0xc2ff2800, 32},
    {0x0000515c, 0x1f83177f, 32},
    {0x00005161, 0x80, 8},
    {0x00005164, 0x00, 8},
    {0x00005165, 0x00, 8},
    {0x0000516a, 0x00, 8},
    {0x00005170, 0x40, 8},
    {0x00005171, 0x00, 8},
    {0x00005172, 0x15, 8},
    {0x00005176, 0x12, 8},
    {0x00005177, 0x40, 8},
    {0x00005181, 0x44, 8},
    {0x00008191, 0x00, 8},
    {0x00005203, 0x02, 8},
    {0x00005390, 0x90909090, 32},
    {0x00005394, 0x90909090, 32},
    {0x00005398, 0x90909090, 32},
    {0x0000539c, 0x90909090, 32},
    {0x000053a0, 0x90909090, 32},
    {0x000053a4, 0x90909090, 32},
    {0x000053a8, 0x90909090, 32},
    {0x000053ac, 0x90909090, 32},
    {0x000053b0, 0x90909090, 32},
    {0x000053b4, 0x90909090, 32},
    {0x000053b8, 0x90909090, 32},
    {0x000053bc, 0x90909090, 32},
    {0x000053c0, 0x90909090, 32},
    {0x000053c4, 0x90909090, 32},
    {0x000053c8, 0x90909090, 32},
    {0x000053cc, 0x90909090, 32},
    {0x000053d0, 0x90909090, 32},
    {0x000053d4, 0x90909090, 32},

    {0x00005a00, 0x48484848, 32},
    {0x00005a04, 0x48484848, 32},
    {0x00005a08, 0x48484848, 32},
    {0x00005a0c, 0x48484848, 32},
    {0x00005a10, 0x00004848, 32},
    {0x00005a24, 0x6a6a6a6a, 32},
    {0x00005a28, 0x6a6a6a6a, 32},
    {0x00005a2c, 0x6a6a6a6a, 32},
    {0x00005a30, 0x6a6a6a6a, 32},
    {0x00005a34, 0x6c6c6a6a, 32},
    {0x00005a38, 0x6c6c6c6c, 32},
    {0x00005a3c, 0x6c6c6c6c, 32},
    {0x00005a40, 0x6c6c6c6c, 32},
    {0x00005a44, 0x6c6c6c6c, 32},
    {0x00005a48, 0x48484848, 32},
    {0x00005a4c, 0x48484848, 32},
    {0x00005a50, 0x48484848, 32},
    {0x00005a54, 0x48484848, 32},
    {0x00005a58, 0x00004848, 32},
    {0x00005a6c, 0x00000000, 32},
    {0x00005a70, 0x00000000, 32},
    {0x00005a74, 0x00000000, 32},
    {0x00005a78, 0x00000000, 32},
    {0x00005a7c, 0x00000000, 32},
    {0x00005a80, 0x00000000, 32},
    {0x00005a84, 0x00000000, 32},
    {0x00005a88, 0x00000000, 32},
    {0x00005a8c, 0x00000000, 32},
    {0x00005d5c, 0x2e2e0000, 32},
    {0x00005d60, 0x00000000, 32},
    {0x00005e0c, 0x00626200, 32},
    {0x00005e18, 0x04000000, 32},
    {0x00005e1c, 0x04040404, 32},
    {0x00005e20, 0x04040404, 32},
    {0x00005e24, 0x04040404, 32},
    {0x00005e28, 0x04040404, 32},
    {0x00005e2c, 0x04040404, 32},
    {0x00005e30, 0x04040404, 32},
    {0x00005e34, 0x04040404, 32},
    {0x00005e38, 0x04040404, 32},
    {0x00005e3c, 0x00040404, 32},
    {0x00005300, 0x01010101, 32},
    {0x00005304, 0x01010101, 32},
    {0x00005308, 0x01010101, 32},
    {0x0000530c, 0x01010101, 32},
    {0x00005310, 0x01010101, 32},
    {0x00005314, 0x01010101, 32},
    {0x00005318, 0x01010101, 32},
    {0x0000531c, 0x01010101, 32},
    {0x00005320, 0x01010101, 32},
    {0x00005324, 0x01010101, 32},
    {0x00005328, 0x01010101, 32},
    {0x0000532c, 0x01010101, 32},
    {0x00005330, 0x01010101, 32},
    {0x00005334, 0x01010101, 32},
    {0x00005338, 0x01010101, 32},
    {0x0000533c, 0x01010101, 32},
    {0x00005340, 0x01010101, 32},
    {0x00005344, 0x01010101, 32},
    {0x00005348, 0x01010101, 32},
    {0x0000534c, 0x01010101, 32},
    {0x00005350, 0x01010101, 32},
    {0x00005354, 0x01010101, 32},
    {0x00005358, 0x01010101, 32},
    {0x0000535c, 0x01010101, 32},
    {0x00005360, 0x01010101, 32},
    {0x00005364, 0x01010101, 32},
    {0x00005368, 0x01010101, 32},
    {0x0000536c, 0x01010101, 32},
    {0x00005370, 0x01010101, 32},
    {0x00005374, 0x01010101, 32},
    {0x00005378, 0x01010101, 32},
    {0x0000537c, 0x01010101, 32},
    {0x00005380, 0x01010101, 32},
    {0x00005384, 0x01010101, 32},
    {0x00005388, 0x01010101, 32},
    {0x0000538c, 0x01010101, 32},
    {0x000053d8, 0x55555555, 32},
    {0x000053dc, 0x55555550, 32},
    {0x000053e0, 0x55555055, 32},
    {0x000053e4, 0x55505555, 32},
    {0x000053e8, 0x50555555, 32},
    {0x000053ec, 0x55555555, 32},
    {0x000053f0, 0x55555550, 32},
    {0x000053f4, 0x55555055, 32},
    {0x000053f8, 0x55505555, 32},
    {0x000053fc, 0x50555555, 32},
    {0x0000513c, 0x02, 8},
    {0x0000513c, 0x06, 8},
    {0x0000508e, 0x01, 8},
    {0x0000508e, 0x00, 8},
    {0x0000857c, 0x80000000, 32},
    {0x00005017, 0x80, 8},
    {0x0000500b, 0x00, 8},
    {0x00008af2, 0x01, 8},
    {0x00008aa0, 0x10, 8},
};
#elif FPGA_DDR2
static MMIOREGISTER Mclk_300_Timing_Desktop[]={
    {0x0000516c, 0x10, 8},
    {0x00005040, 0x00000000, 32},
    {0x00005044, 0x00000000, 32},
    {0x00005100, 0x40302010, 32},
    {0x00005104, 0x80706050, 32},
    {0x00005108, 0x30201000, 32},
    {0x0000510c, 0x70605040, 32},
    {0x00005158, 0x67380202, 32},
    {0x0000515c, 0x1f83177f, 32},
    {0x00005160, 0x88008000, 32},
    {0x00005164, 0x00000000, 32},
    {0x00005168, 0x5d004e4e, 32},
    {0x0000516c, 0x48118030, 32},
    {0x00005250, 0x00ff0000, 32},
    
    {0x0000516c, 0x48118030, 32},
    {0x00005118, 0xba98ba98, 32},
    {0x0000511c, 0xd4, 8},
    {0x0000511d, 0x6c, 8},
    {0x0000511e, 0xb0, 8},
    {0x00005120, 0x01, 8},
    {0x00005121, 0x80, 8},
    {0x00005126, 0xc0, 8},
    {0x0000512c, 0x01, 8},
    {0x0000512d, 0x80, 8},
    {0x0000512f, 0x00, 8},
    {0x00005138, 0x00000000, 32},
    {0x0000513d, 0x00, 8},
    {0x0000513e, 0x33, 8},
    {0x0000513f, 0xa6, 8},
    {0x00005140, 0x26108804, 32},
    {0x00005145, 0x08, 8},
    {0x00005148, 0xc2ff2800, 32},
    {0x0000515c, 0x1f83177f, 32},
    {0x00005161, 0x80, 8},
    {0x00005164, 0x00, 8},
    {0x00005165, 0x00, 8},
    {0x0000516a, 0x00, 8},
    {0x00005170, 0x40, 8},
    {0x00005171, 0x00, 8},
    {0x00005172, 0x15, 8},
    {0x00005176, 0x12, 8},
    {0x00005177, 0x40, 8},
    {0x00005181, 0x44, 8},
    {0x00005191, 0x00, 8},
    {0x00005203, 0x02, 8},
    {0x00005390, 0x90909090, 32}, 
    {0x00005394, 0x90909090, 32}, 
    {0x00005398, 0x90909090, 32}, 
    {0x0000539c, 0x90909090, 32}, 
    {0x000053a0, 0x90909090, 32}, 
    {0x000053a4, 0x90909090, 32}, 
    {0x000053a8, 0x90909090, 32}, 
    {0x000053ac, 0x90909090, 32}, 
    {0x000053b0, 0x90909090, 32}, 
    {0x000053b4, 0x90909090, 32}, 
    {0x000053b8, 0x90909090, 32}, 
    {0x000053bc, 0x90909090, 32}, 
    {0x000053c0, 0x90909090, 32}, 
    {0x000053c4, 0x90909090, 32}, 
    {0x000053c8, 0x90909090, 32}, 
    {0x000053cc, 0x90909090, 32}, 
    {0x000053d0, 0x90909090, 32}, 
    {0x000053d4, 0x90909090, 32},  
    {0x00005a00, 0x48484848, 32}, 
    {0x00005a04, 0x48484848, 32}, 
    {0x00005a08, 0x48484848, 32}, 
    {0x00005a0c, 0x48484848, 32}, 
    {0x00005a10, 0x00004848, 32}, 
    {0x00005a24, 0x6a6a6a6a, 32}, 
    {0x00005a28, 0x6a6a6a6a, 32}, 
    {0x00005a2c, 0x6a6a6a6a, 32}, 
    {0x00005a30, 0x6a6a6a6a, 32}, 
    {0x00005a34, 0x6c6c6a6a, 32}, 
    {0x00005a38, 0x6c6c6c6c, 32}, 
    {0x00005a3c, 0x6c6c6c6c, 32}, 
    {0x00005a40, 0x6c6c6c6c, 32}, 
    {0x00005a44, 0x6c6c6c6c, 32}, 
    {0x00005a48, 0x48484848, 32}, 
    {0x00005a4c, 0x48484848, 32}, 
    {0x00005a50, 0x48484848, 32}, 
    {0x00005a54, 0x48484848, 32}, 
    {0x00005a58, 0x00004848, 32}, 
    {0x00005a6c, 0x00000000, 32}, 
    {0x00005a70, 0x00000000, 32}, 
    {0x00005a74, 0x00000000, 32}, 
    {0x00005a78, 0x00000000, 32}, 
    {0x00005a7c, 0x00000000, 32}, 
    {0x00005a80, 0x00000000, 32}, 
    {0x00005a84, 0x00000000, 32}, 
    {0x00005a88, 0x00000000, 32}, 
    {0x00005a8c, 0x00000000, 32}, 
    {0x00005d5c, 0x2e2e0000, 32}, 
    {0x00005d60, 0x00000000, 32}, 
    {0x00005e0c, 0x00626200, 32}, 
    {0x00005e18, 0x04000000, 32}, 
    {0x00005e1c, 0x04040404, 32}, 
    {0x00005e20, 0x04040404, 32}, 
    {0x00005e24, 0x04040404, 32}, 
    {0x00005e28, 0x04040404, 32}, 
    {0x00005e2c, 0x04040404, 32}, 
    {0x00005e30, 0x04040404, 32}, 
    {0x00005e34, 0x04040404, 32}, 
    {0x00005e38, 0x04040404, 32}, 
    {0x00005e3c, 0x00040404, 32}, 
    {0x00005300, 0x01010101, 32}, 
    {0x00005304, 0x01010101, 32}, 
    {0x00005308, 0x01010101, 32}, 
    {0x0000530c, 0x01010101, 32}, 
    {0x00005310, 0x01010101, 32}, 
    {0x00005314, 0x01010101, 32}, 
    {0x00005318, 0x01010101, 32}, 
    {0x0000531c, 0x01010101, 32}, 
    {0x00005320, 0x01010101, 32}, 
    {0x00005324, 0x01010101, 32}, 
    {0x00005328, 0x01010101, 32}, 
    {0x0000532c, 0x01010101, 32}, 
    {0x00005330, 0x01010101, 32}, 
    {0x00005334, 0x01010101, 32}, 
    {0x00005338, 0x01010101, 32}, 
    {0x0000533c, 0x01010101, 32}, 
    {0x00005340, 0x01010101, 32}, 
    {0x00005344, 0x01010101, 32}, 
    {0x00005348, 0x01010101, 32}, 
    {0x0000534c, 0x01010101, 32}, 
    {0x00005350, 0x01010101, 32}, 
    {0x00005354, 0x01010101, 32}, 
    {0x00005358, 0x01010101, 32}, 
    {0x0000535c, 0x01010101, 32}, 
    {0x00005360, 0x01010101, 32}, 
    {0x00005364, 0x01010101, 32}, 
    {0x00005368, 0x01010101, 32}, 
    {0x0000536c, 0x01010101, 32}, 
    {0x00005370, 0x01010101, 32}, 
    {0x00005374, 0x01010101, 32}, 
    {0x00005378, 0x01010101, 32}, 
    {0x0000537c, 0x01010101, 32}, 
    {0x00005380, 0x01010101, 32}, 
    {0x00005384, 0x01010101, 32}, 
    {0x00005388, 0x01010101, 32}, 
    {0x0000538c, 0x01010101, 32}, 
    {0x000053d8, 0x55555555, 32}, 
    {0x000053dc, 0x55555550, 32}, 
    {0x000053e0, 0x55555055, 32}, 
    {0x000053e4, 0x55505555, 32}, 
    {0x000053e8, 0x50555555, 32}, 
    {0x000053ec, 0x55555555, 32}, 
    {0x000053f0, 0x55555550, 32}, 
    {0x000053f4, 0x55555055, 32}, 
    {0x000053f8, 0x55505555, 32}, 
    {0x000053fc, 0x50555555, 32}, 
    
    {0x0000513c, 0x02, 8}, 
    
    {0x0000513c, 0x06, 8},
    {0x0000508e, 0x01, 8},
    
    {0x0000508e, 0x00, 8}, 
    {0x000490a0, 0x1fffff1f, 32}, 
    {0x0000857C, 0x80000000, 32},
    
    {0x00005017, 0x80, 8},
    {0x0000500b, 0x00, 8},
    {0x00008af1, 0x01, 8},
    {0x0000850c, 0x02, 8},
    {0x00008aa0, 0x10, 8},
    {0x00005158, 0x67380204, 32},
};
#else //not QT
static CBREGISTER Mclk_300_Timing_Desktop[]={
};
#endif

static CBREGISTER SRDefaultVCP[]=
{
    {SR, (CBIOS_U8)~0xFE,0x09,0x7E},   
    {SR, (CBIOS_U8)~0x8D,0x0B,0x00},  
    {SR, (CBIOS_U8)~0xF7,0x0D,0x50},
    {SR, (CBIOS_U8)~0xFF, 0x0E, 0xC1},
    {SR, (CBIOS_U8)~0xFF, 0x0F, 0x6B},
    {SR, (CBIOS_U8)~0xDF, 0x14, 0x00},
    {SR, (CBIOS_U8)~0x6E, 0x15, 0x4A},
    {SR, (CBIOS_U8)~0x20, 0x18,0x20},
    {SR, (CBIOS_U8)~0xFF, 0x19, 0x31},
    {SR, (CBIOS_U8)~0x1F, 0x1C, 0x1C},
    {SR, (CBIOS_U8)~0x67, 0x1D, 0x67},
    {SR, (CBIOS_U8)~0x1F, 0x1E, 0x12},
    {SR, (CBIOS_U8)~0xF1, 0x1F, 0x50},
    {SR, (CBIOS_U8)~0xFF, 0x20, 0x7F},
    {SR, (CBIOS_U8)~0xFF, 0x21, 0xCE},
    {SR, (CBIOS_U8)~0xE7, 0x10, 0x84},
    {SR, (CBIOS_U8)~0x7F, 0x22, 0x0E},
    {SR, (CBIOS_U8)~0xFF, 0x23, 0xE8},
    {SR, (CBIOS_U8)~0x3F, 0x24, 0x10},
    {SR, (CBIOS_U8)~0xFF, 0x25, 0xC8},
    {SR, (CBIOS_U8)~0x80, 0x26, 0x00},
    {SR, (CBIOS_U8)~0xFF, 0x27, 0xB3},
    {SR, (CBIOS_U8)~0x3C, 0x28, 0x14},
    {SR, (CBIOS_U8)~0xFF, 0x29, 0x00},
    {SR, (CBIOS_U8)~0xFF, 0x2B, 0x00},
    {SR, (CBIOS_U8)~0xD0, 0x2D, 0x00},
    {SR, (CBIOS_U8)~0x3F, 0x30, 0x03},
    {SR, (CBIOS_U8)~0x76, 0x31, 0x00},
    {SR, (CBIOS_U8)~0xF8, 0x39, 0x98},
    {SR, (CBIOS_U8)~0x18, 0x3C, 0x18},
    {SR, (CBIOS_U8)~0x3F, 0x3F, 0x00},
    {SR, (CBIOS_U8)~0xFF, 0x42, 0x01},
    {SR, (CBIOS_U8)~0xFF, 0x43, 0x01},
    {SR, (CBIOS_U8)~0x88, 0x44, 0x80},
    {SR, (CBIOS_U8)~0x9F, 0x45, 0x00},
    {SR, (CBIOS_U8)~0xFF, 0x47, 0x01},
    {SR, (CBIOS_U8)~0xFF, 0x4F, 0x00},
    {SR, (CBIOS_U8)~0x01, 0x70, 0x00},
    {SR, (CBIOS_U8)~0x10, 0x94, 0x00},
    {SR, (CBIOS_U8)~0x10, 0x40, 0x00},
    {SR, (CBIOS_U8)~0x81, 0x37, 0x00},
    {SR, (CBIOS_U8)~0xFF, 0x38, 0x00},
    {SR, (CBIOS_U8)~0xFF, 0x3E, 0x00},
    {SR, (CBIOS_U8)~0x1C, 0x31, 0x1C},
    {SR, (CBIOS_U8)~0xFF, 0x36, 0x00},//disable dither for dp1
    {SR_B, (CBIOS_U8)~0xFF, 0x36, 0x00},//disable dither for dp2
};

static CBREGISTER CRDefaultVCP[]=
{
    {CR, (CBIOS_U8)~0x09, 0x31, 0x09},
    {CR, (CBIOS_U8)~0xB0, 0x34, 0x00},
    {CR, (CBIOS_U8)~0x01, 0x3D, 0x00},
    {CR, (CBIOS_U8)~0x68, 0x43, 0x00},
    {CR, (CBIOS_U8)~0x0F, 0x45, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0x52, 0x00},
    {CR, (CBIOS_U8)~0x20, 0x55, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0x59, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0x5A, 0x00},
    {CR, (CBIOS_U8)~0xF8, 0x5B, 0x00},
    {CR, (CBIOS_U8)~0xD9, 0x63, 0x00},
    {CR, (CBIOS_U8)~0x02, 0x66, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0x6B, 0x01},
    {CR, (CBIOS_U8)~0xFF, 0x6C, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0x6D, 0x33},
    {CR, (CBIOS_U8)~0xFF, 0x6E, 0x00},
    {CR, (CBIOS_U8)~0x22, 0x71, 0x22},
    {CR, (CBIOS_U8)~0xFF, 0x86, 0xA8},
    {CR, (CBIOS_U8)~0xFF, 0x87, 0x68},
    {CR, (CBIOS_U8)~0xFF, 0x88, 0x20},
    {CR, (CBIOS_U8)~0xFF, 0x89, 0xE0},
    {CR, (CBIOS_U8)~0xFF, 0x8D, 0x68},
    {CR, (CBIOS_U8)~0xFF, 0xA1, 0x00},
    {CR, (CBIOS_U8)~0xFB, 0xA2, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0xA3, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0xA4, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0xA6, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0xB0, 0x55},
    {CR, (CBIOS_U8)~0x1F, 0xB1, 0x09},
    {CR, (CBIOS_U8)~0x3F, 0xB2, 0x01},
    {CR, (CBIOS_U8)~0x3F, 0xB3, 0x01},
    {CR, (CBIOS_U8)~0xFF, 0xBC, 0x20},
    {CR, (CBIOS_U8)~0xFF, 0xBD, 0xA8},
    {CR, (CBIOS_U8)~0xFF, 0xBE, 0x08},
    {CR, (CBIOS_U8)~0xFF, 0xC0, 0x24},
    {CR, (CBIOS_U8)~0xFF, 0xC1, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0xC2, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0xC3, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0xC4, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0xC5, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0xC6, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0xC7, 0x00},
    {CR, (CBIOS_U8)~0xFF, 0xC8, 0x00},
    {CR, (CBIOS_U8)~0x01, 0xCD, 0x00},
    {CR, (CBIOS_U8)~0x1F, 0xCE, 0x0F},
    {CR, (CBIOS_U8)~0x80, 0xD4, 0x80},
    {CR_B, (CBIOS_U8)~0x09, 0x31, 0x09},
    {CR_B, (CBIOS_U8)~0x22, 0x71, 0x22},
    {CR_B, (CBIOS_U8)~0x1F, 0xB1, 0x09},
    {CR_B, (CBIOS_U8)~0xFF, 0xC0, 0x7F},
    {CR_B, (CBIOS_U8)~0xFF, 0xC1, 0x7F},
    {CR_B, (CBIOS_U8)~0xFF, 0xC2, 0x00},
    {CR_B, (CBIOS_U8)~0xFF, 0xC3, 0x00},
    {CR_B, (CBIOS_U8)~0xFF, 0xC4, 0x00},
    {CR_B, (CBIOS_U8)~0x1F, 0xCE, 0x0F},
    {CR_B, (CBIOS_U8)~0x40, 0xD2, 0x40},
    {CR_B, (CBIOS_U8)~0x1F, 0xE0, 0x04},
    {CR_B, (CBIOS_U8)~0xFF, 0xF5, 0x09},
    {CR_B, (CBIOS_U8)~0xFF, 0xF6, 0x1F},
    {CR_B, (CBIOS_U8)~0xFF, 0xFC, 0x00},
    {CR_B, (CBIOS_U8)~0x80, 0xFD, 0x00},
    {CR_C, (CBIOS_U8)~0x04, 0x9D, 0x04},
    {CR_C, (CBIOS_U8)~0x1F, 0xA0, 0x02},
    {CR_C, (CBIOS_U8)~0x10, 0xA1, 0x00},
    {CR_C,  (CBIOS_U8)~0x01, 0xA2,0x00},
    {CR_C, (CBIOS_U8)~0xC0, 0xA3, 0x00},
    {CR_C, (CBIOS_U8)~0xA0, 0xA4, 0x00},
    {CR_C, (CBIOS_U8)~0x80, 0xA5, 0x00},
    {CR_C, (CBIOS_U8)~0xD0, 0xA6, 0x00},
    {CR_C, (CBIOS_U8)~0x3E, 0xC2, 0x00},
};

static CBREGISTER ZXGModeExtRegDefault[] =
{
    {SR,(CBIOS_U8)~0xC0, 0x19, 0x00},
    {SR,(CBIOS_U8)~0x60, 0x1A, 0x00},
    {SR,(CBIOS_U8)~0xF8, 0x1B, 0x00},
    {SR,(CBIOS_U8)~0xD8, 0x47, 0x00},   
    {CR,(CBIOS_U8)~0xEF, 0x31, 0x05},
    {CR,(CBIOS_U8)~0x44, 0x32, 0x40},
    {CR,(CBIOS_U8)~0x7A, 0x33, 0x08},
    {CR,(CBIOS_U8)~0x38, 0x3A, 0x00},
    {CR, 0x00, 0x3B, 0x00},
    {CR, 0x00, 0x3C, 0x10},
    {CR,(CBIOS_U8)~0x3F, 0x42, 0x00},   
    {CR,(CBIOS_U8)~0xF1, 0x50, 0x00},
    {CR,(CBIOS_U8)~0x7F, 0x51, 0x00},
    {CR,(CBIOS_U8)~0x80, 0x53, 0x00},
    {CR, 0x00, 0x5D, 0x00},
    {CR, 0x00, 0x5E, 0xC0},	 
    {CR,(CBIOS_U8)~0x0F, 0x64, 0x00},
    {CR,(CBIOS_U8)~0xFB, 0x65, 0x00},
    {CR,(CBIOS_U8)~0x01, 0x66, 0x00},
    {CR, 0x00, 0x69, 0x00},
    {CR, 0x00, 0x6A, 0x00},
};

static CBREGISTER ZXGMode3SRRegTable[] =
{
    {SR,     0x00, 0x02, 0x03},
    {SR,     0x00, 0x03, 0x00},
    {SR,     0x00, 0x04, 0x02},
};

static CBREGISTER ZXGMode3CRRegTable[] =
{
    {CR,     0x00, 0x00, 0x5F},
    {CR,     0x00, 0x01, 0x4F},
    {CR,     0x00, 0x02, 0x50},
    {CR,     0x00, 0x03, 0x82},
    {CR,     0x00, 0x04, 0x55},
    {CR,     0x00, 0x05, 0x81},
    {CR,     0x00, 0x06, 0xBF},
    {CR,     0x00, 0x07, 0x1F},
    {CR,    ~0x7F, 0x08, 0x00},
    {CR,     0x00, 0x09, 0x4F},
    {CR,    ~0x3F, 0x0A, 0x0D},
    {CR,    ~0x7F, 0x0B, 0x0E},
    {CR,     0x00, 0x0C, 0x00},
    {CR,     0x00, 0x0D, 0x00},
    {CR,     0x00, 0x0E, 0x00},
    {CR,     0x00, 0x0F, 0x00},
    {CR,     0x00, 0x10, 0x9C},
    {CR,     0x00, 0x11, 0x8E},
    {CR,     0x00, 0x12, 0x8F},
    {CR,     0x00, 0x13, 0x28},
    {CR,    ~0x7F, 0x14, 0x1F},
    {CR,     0x00, 0x15, 0x96},
    {CR,     0x00, 0x16, 0xB9},
    {CR,     0x00, 0x17, 0xA3},
    {CR,     0x00, 0x18, 0xFF},
    {CR,    ~0x3F, 0x42, 0x00},
    {CR,    ~0x7F, 0x51, 0x00},
    {CR,    ~0x7F, 0x5D, 0x00},
    {CR,     0x00, 0x5E, 0xC0},	 
    {CR,     0x00, 0x6A, 0x00},
};

static CBREGISTER ZXGMode3ARRegTable[] =
{
    {AR,  0x00, 0x00, 0x00},
    {AR,  0x00, 0x01, 0x01},
    {AR,  0x00, 0x02, 0x02},
    {AR,  0x00, 0x03, 0x03},
    {AR,  0x00, 0x04, 0x04},
    {AR,  0x00, 0x05, 0x05},
    {AR,  0x00, 0x06, 0x14},
    {AR,  0x00, 0x07, 0x07},
    {AR,  0x00, 0x08, 0x38},
    {AR,  0x00, 0x09, 0x39},
    {AR,  0x00, 0x0A, 0x3A},
    {AR,  0x00, 0x0B, 0x3B},
    {AR,  0x00, 0x0C, 0x3C},
    {AR,  0x00, 0x0D, 0x3D},
    {AR,  0x00, 0x0E, 0x3E},
    {AR,  0x00, 0x0F, 0x3F}, 

    {AR,  0x00, 0x10, 0x0C},
    {AR,  0x00, 0x11, 0x00},
    {AR,  0x00, 0x12, 0x0F},
    {AR,  0x00, 0x13, 0x08},
    {AR,  0x00, 0x14, 0x00},

    {GR,  0x00, 0x00, 0x00},
    {GR,  0x00, 0x01, 0x00},
    {GR,  0x00, 0x02, 0x00},
    {GR,  0x00, 0x03, 0x00},
    {GR,  0x00, 0x04, 0x00},
    {GR,  0x00, 0x05, 0x10},
    {GR,  0x00, 0x06, 0x0E},
    {GR,  0x00, 0x07, 0x00},
    {GR,  0x00, 0x08, 0xFF},
};

static CBREGISTER ZXGMode3RegOnIGA2[] =
{
    {CR, 0x00, 0x00, 0x5F},
    {CR, 0x00, 0x01, 0x4F},
    {CR, 0x00, 0x02, 0x50},
    {CR, 0x00, 0x03, 0x82},
    {CR, 0x00, 0x04, 0x52},
    {CR, 0x00, 0x05, 0x9E},
    {CR, 0x00, 0x06, 0x0B},
    {CR, 0x00, 0x07, 0x3E},
    {CR, 0x00, 0x08, 0x00},
    {CR, 0x00, 0x09, 0x40},
    {CR, 0x00, 0x0D, 0x00},    
    {CR, 0x00, 0x10, 0xEA},
    {CR, 0x00, 0x11, 0x0C},
    {CR, 0x00, 0x12, 0xDF},
    {CR, 0x00, 0x15, 0xE7},
    {CR, 0x00, 0x16, 0x05},
    {CR,(CBIOS_U8)~0x80, 0x17, 0x80},
    {CR, 0x00, 0x5D, 0x00},
    {CR, 0x00, 0x5E, 0xC0},	
    {CR,(CBIOS_U8)~0xC0, 0x63, 0x00},
};

/*****************************************************************************/
/* ------------------------------------- HW Init --------------------------------------- */
/*****************************************************************************/

static CBIOS_STATUS cbChipEnable(PCBIOS_EXTENSION_COMMON pcbe)
{ 
    CBIOS_STATUS status = CBIOS_OK;

    // 850C bit 1 should be used to wake chip.
    cb_WriteU8(pcbe->pAdapterContext, 0x850C, 0x2);
    
    //Wakeup chip, 3C3 bit0 = 1
    cb_WriteU8(pcbe->pAdapterContext, 0x83C3, 0x01);

    // 3C2 MISCELLANEOUS reg
    // Bit 0 Color emulation. Address base at 0x3Dx.
    // Bit 1 Enable access of CPU
    cb_WriteU8(pcbe->pAdapterContext, 0x83C2, 0x03);

    cbUnlockCR(pcbe);
    cbUnlockSR(pcbe);

    cbMMIOWriteReg(pcbe, SR_2A, 0x00,  0x00);
    cbMMIOWriteReg(pcbe, SR_26, 0x40,  0x00);  

#if CBIOS_CHECK_HARDWARE_STATUS
    // If MMIO failure, try to use standard IO.
    if (cbIsMMIOWell(pcbe) == CBIOS_FALSE)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "\n\nMMIO not enabled!!!\n"));

        return CBIOS_ER_MMIO;
    }

#endif


    return status;
}

#if 0

static CBIOS_VOID cbPOSTSetClock(PCBIOS_EXTENSION_COMMON pcbe)
{
    CBIOS_U32 dwEClock, dwIClock, dwVppClock;

    cbMMIOWriteReg(pcbe, SR_01, 0x20, 0xDF); //screen off

    dwEClock = pcbe->SysBiosInfo.ECLK;
    dwIClock = pcbe->SysBiosInfo.ICLK;
    dwVppClock = pcbe->SysBiosInfo.VCLK;

    if(dwEClock < CBIOS_CLOCK_MIN || dwEClock > CBIOS_CLOCK_MAX)
    {
        dwEClock = CBIOS_ECLK_DEFAULT;
    }
    if(dwIClock < CBIOS_CLOCK_MIN || dwIClock > CBIOS_CLOCK_MAX)
    {
        dwIClock = CBIOS_ICLK_DEFAULT;
    }
     if(dwVppClock < CBIOS_CLOCK_MIN || dwVppClock > CBIOS_CLOCK_MAX)
    {
        dwVppClock = CBIOS_VCLK_DEFAULT;
    }
     
    dwEClock *= 10000;
    dwIClock *= 10000;
    dwVppClock *= 10000;

    cbProgClock(pcbe,dwEClock, ECLKTYPE_Post, IGA1);
    //cbProgClock(pcbe,dwIClock, ICLKTYPE_Post, IGA1);
    cbProgClock(pcbe,dwVppClock, CBIOS_VCLKTYPE, IGA1);
}

#endif

static CBIOS_STATUS cbLoadDefaultTables(PCBIOS_EXTENSION_COMMON pcbe)
{   
    CBIOS_STATUS status = CBIOS_OK;

    //For QT only, load default MIU table.
    if(pcbe->bRunOnQT && !pcbe->bDriverLoadQTiming)// || !pcbe->ChipCaps.bNoMemoryControl)
    {
        cbLoadMemoryTimingTbl(pcbe, Mclk_300_Timing_Desktop,
            sizeofarray(Mclk_300_Timing_Desktop));

        cbLoadtable(pcbe,
            SRDefaultVCP,
            sizeofarray(SRDefaultVCP),
            CBIOS_NOIGAENCODERINDEX);

        cbLoadtable(pcbe,
            CRDefaultVCP,
            sizeofarray(CRDefaultVCP),
            CBIOS_NOIGAENCODERINDEX);
    }

    cbLoadtable(pcbe, 
        pcbe->PostRegTable[VCP_TABLE].pSRDefault, 
        pcbe->PostRegTable[VCP_TABLE].sizeofSRDefault, 
        CBIOS_NOIGAENCODERINDEX);

    cbLoadtable(pcbe, 
        pcbe->PostRegTable[VCP_TABLE].pCRDefault, 
        pcbe->PostRegTable[VCP_TABLE].sizeofCRDefault, 
        CBIOS_NOIGAENCODERINDEX);

    return status;
}

static CBIOS_VOID cbEndOfPost(PCBIOS_EXTENSION_COMMON pcbe)
{ 

    REG_CR6B    RegCR6BValue;
    REG_CR6B    RegCR6BMask;
    REG_CR6C    RegCR6CValue;
    REG_CR6C    RegCR6CMask;

    // Clear active device in scratch pad.
    RegCR6BValue.Value = 0;
    RegCR6BValue.BIOS_Flag_3 = 0;
    RegCR6BMask.Value = 0xFF;
    RegCR6BMask.BIOS_Flag_3 = 0;
    cbMMIOWriteReg(pcbe,CR_6B, RegCR6BValue.Value, RegCR6BMask.Value);
    RegCR6CValue.Value = 0;
    RegCR6CValue.BIOS_Flag_4 = 0;
    RegCR6CMask.Value = 0xFF;
    RegCR6CMask.BIOS_Flag_4 = 0;
    cbMMIOWriteReg(pcbe,CR_6C, RegCR6CValue.Value, RegCR6CMask.Value);    

    cbUnlockSR(pcbe);
    cbUnlockCR(pcbe);
}

static CBIOS_VOID  cbSetupVGA(PCBIOS_EXTENSION_COMMON pcbe)
{
    cb_WriteU8(pcbe->pAdapterContext, CB_CRT_ADDR_REG, 0x41);
    cb_WriteU8(pcbe->pAdapterContext, CB_CRT_DATA_REG, 0x03);  //mode 3

     cbLoadtable(pcbe, ZXGModeExtRegDefault,
                 sizeofarray(ZXGModeExtRegDefault),
                 CBIOS_NOIGAENCODERINDEX);           
     // do sync reset
     cb_WriteU16(pcbe->pAdapterContext, CB_SEQ_ADDR_REG, 0x0100);
     cb_WriteU16(pcbe->pAdapterContext, CB_SEQ_ADDR_REG, 0x2001);
     cb_WriteU16(pcbe->pAdapterContext, CB_SEQ_ADDR_REG, 0x2001);   // Do this two times

     cb_WriteU8(pcbe->pAdapterContext, CB_MISC_OUTPUT_WRITE_REG,0x67);
     cb_WriteU16(pcbe->pAdapterContext, CB_SEQ_ADDR_REG, 0x0300);

     cb_DelayMicroSeconds( 150);

     cbLoadtable(pcbe, 
                     ZXGMode3SRRegTable,
                     sizeofarray(ZXGMode3SRRegTable),
                     CBIOS_NOIGAENCODERINDEX); 
     cb_WriteU16(pcbe->pAdapterContext, CB_CRT_ADDR_REG, 0x0011);  // Unlock writing to CRT

     cbLoadtable(pcbe, 
                     ZXGMode3CRRegTable,
                     sizeofarray(ZXGMode3CRRegTable),
                     CBIOS_NOIGAENCODERINDEX);
     cbLoadtable(pcbe, 
                     ZXGMode3RegOnIGA2, 
                     sizeofarray(ZXGMode3RegOnIGA2), 
                     IGA2);

     cbLoadtable(pcbe, 
                     ZXGMode3RegOnIGA2, 
                     sizeofarray(ZXGMode3RegOnIGA2), 
                     IGA3);

     cbLoadtable(pcbe, 
                  ZXGMode3RegOnIGA2, 
                  sizeofarray(ZXGMode3RegOnIGA2), 
                  IGA4);


     cb_ReadU8(pcbe->pAdapterContext, CB_INPUT_STATUS_1_REG);  // lock writing to CRT

     cbLoadtable(pcbe, 
                     ZXGMode3ARRegTable,
                     sizeofarray(ZXGMode3ARRegTable),
                     CBIOS_NOIGAENCODERINDEX);       
}

CBIOS_STATUS cbPost(PCBIOS_VOID pvcbe)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_STATUS    status = CBIOS_OK;
    REG_CR68    RegCR68Value, RegCR68Mask;

    /*this is a patch, we must write something to 3c6h,3c7h,3c8h or 3c9h 
    to generate hblank vblank signal for CRT*/
    cb_WriteU8(pcbe->pAdapterContext, 0x83C8, 0);
    
    cbWritePort80(pcbe,CBIOS_PORT80_ID_CBiosPost_Enter);

    status =cbChipEnable(pcbe);
    cbWritePort80(pcbe, CBIOS_PORT80_ID_AfterChipEnable);
    if( status != CBIOS_OK)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"Chip is not enable!\n"));
        cbPrintMsgToFile(DBG_LEVEL_ERROR_MSG, "cbPost: Chip is not enable!\n", CBIOS_NULL, 0);
        return status;  
    }

    RegCR68Value.Value = 0;
    RegCR68Value.EOCLK_Source = 0;
    RegCR68Mask.Value = 0xFF;
    RegCR68Mask.EOCLK_Source = 0;
    cbMMIOWriteReg(pcbe, CR_68, RegCR68Value.Value, RegCR68Mask.Value);       //force internal Eclk

    cbMMIOWriteReg(pcbe, CR_C_F3, 0x04, ~0x04); //reset will not check PLL lock
    //Just keep default clock now and GFX will adjust it dynamically
    //cbPOSTSetClock(pcbe);
    cbWritePort80(pcbe,CBIOS_PORT80_ID_AfterPrePost);
    
    cbLoadDefaultTables(pcbe);   
    cbWritePort80(pcbe,CBIOS_PORT80_ID_AfterLoadDefautTables);

    cbWritePort80(pcbe,CBIOS_PORT80_ID_AfterInitMemory);   

    cbWritePort80(pcbe,CBIOS_PORT80_ID_AfterIsMemoryWriteable);

    if(!pcbe->bRunOnQT)
    {
        cbSetupVGA(pcbe);
    }

    // Per Henry, this code should come AFTER the MIU reset
    cbEndOfPost(pcbe);
    
    cbWritePort80(pcbe,CBIOS_PORT80_ID_CBiosPost_Exit);
    
    pcbe->DbgDataToFile[0] = status;
    cbPrintMsgToFile(DBG_LEVEL_CHIP_INFO, "Exit cbPost and return status", pcbe->DbgDataToFile, 4);

    cbHWSetChipPostFlag(pcbe);

    return status;
}

static CBIOS_VOID cbGetPMPInfo(PCBIOS_VOID pvcbe)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_UCHAR     PMPVer[E3K_PMPVER_SIZE];
    CBIOS_UCHAR     PMPTag[E3K_PMPTAG_SIZE];
    CBIOS_U8    DateIndex = 0, TimeIndex = 0;
    CBIOS_U8    Count = 0;
    CBIOS_U8 i = 0;

    if(pcbe->bRunOnQT)
    {
        return;
    }

    cb_memset((PCBIOS_VOID)(PMPVer), 0, sizeof(PMPVer));
    cb_memset((PCBIOS_VOID)(PMPTag), 0, sizeof(PMPTag));


    for(i = 0; i < E3K_PMPTAG_SIZE; i++)
    {
        PMPTag[i] = cb_ReadU8(pcbe->pAdapterContext, E3K_PMPTAG_OFFSET+i);
    }

    if(!cb_strcmp(PMPTag, (CBIOS_UCHAR*)"pmp"))
    {
        for(i = 0; i < E3K_PMPVER_SIZE; i++)
        {
            PMPVer[i] = cb_ReadU8(pcbe->pAdapterContext, E3K_PMPVER_OFFSET+i);
            if(PMPVer[i] == '\0' && Count < 2)
            {
                if(Count == 0)
                {
                    DateIndex = i+1;
                }
                else
                {
                    TimeIndex = i+1;
                }
                Count++;
            }
        }
        cb_memcpy(pcbe->PMPVer, PMPVer, E3K_PMPVER_SIZE);
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO),"Firmware timing table version:%s Build Time: %s %s \n", PMPVer, PMPVer+DateIndex, PMPVer+TimeIndex));
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO),"Can't get Firmware timing table version\n"));
    }
}

CBIOS_STATUS cbInitChip(PCBIOS_VOID pvcbe)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32       SupportDevices = 0, Index = 0, CurDev = 0;
    PCBIOS_DEVICE_COMMON    pDevCommon = CBIOS_NULL;
    CBIOS_GAMMA_PARA   GammaSet = {0};

    if((pcbe->ChipID == CHIPID_E3K) || (pcbe->ChipID == CHIPID_ARISE1020))
    {
        cbGetPMPInfo(pcbe);
    }

    SupportDevices = pcbe->DeviceMgr.SupportDevices;
    while (SupportDevices)
    {
        CurDev = GET_LAST_BIT(SupportDevices);
        pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, CurDev);

        cbDevDeviceHwInit(pcbe, pDevCommon);
        SupportDevices &= (~CurDev);
    }

    cbDIU_HDAC_SetStatus(pcbe);

    GammaSet.Flags.bDisableGamma = 1;
    for(Index = 0; Index < pcbe->DispMgr.IgaCount; Index++)
    {
        //patch for Issue 137273: if Dac color mode = 4bpp/8bpp, Ps will go though LUT even gamma is disabled!
        cbBiosMMIOWriteReg(pcbe, CR_67, 0x40, ~0x70, Index);
        //patch for Issue137093, shut down screen to prevent garbage.
        cbBiosMMIOWriteReg(pcbe, CR_71, 0x20, ~0x20, Index);
        //disable stream
        cbDisableStream(pcbe, Index);

        //disable gamma
        GammaSet.IGAIndex = Index;
        cbSetGamma(pcbe, &GammaSet);
    }

    if((pcbe->ChipID == CHIPID_E3K) || (pcbe->ChipID == CHIPID_ARISE1020))
    {
        cbMMIOWriteReg32(pcbe, 0x33524, 0xfffff, ~0xfffff); // enable all security_r of DIU
    }

    if((pcbe->ChipID == CHIPID_E3K) || (pcbe->ChipID == CHIPID_ARISE1020))
    {
       //threshold
       cbMMIOWriteReg32(pcbe, 0x8454,0xa0a0a0a0,  0);
       cbMMIOWriteReg32(pcbe, 0x33928,0xa0a0a0a0, 0);
       cbMMIOWriteReg32(pcbe, 0x34028,0xa0a0a0a0, 0);
       cbMMIOWriteReg32(pcbe, 0x34728,0xa0a0a0a0, 0);

       cbMMIOWriteReg32(pcbe, 0x3346c,0xa0a0a0a0, 0);
       cbMMIOWriteReg32(pcbe, 0x33a9c,0xa0a0a0a0, 0);
       cbMMIOWriteReg32(pcbe, 0x3419c,0xa0a0a0a0, 0);
       cbMMIOWriteReg32(pcbe, 0x3489c,0xa0a0a0a0, 0);

       cbMMIOWriteReg32(pcbe, 0x33428,0xa0a0a0a0, 0);
       cbMMIOWriteReg32(pcbe, 0x33b18,0xa0a0a0a0, 0);
       cbMMIOWriteReg32(pcbe, 0x34218,0xa0a0a0a0, 0);
       cbMMIOWriteReg32(pcbe, 0x34918,0xa0a0a0a0, 0);

       //timeout
       cbMMIOWriteReg32(pcbe, 0x3328c,0x08080808, 0);
       cbMMIOWriteReg32(pcbe, 0x332b0,0x08080808, 0);
       cbMMIOWriteReg32(pcbe, 0x332b4,0x08080808, 0);
       cbMMIOWriteReg32(pcbe, 0x332b8,0x08080808, 0);

       //force high & enable new feature
       cbMMIOWriteReg(pcbe, CR_A5, 0x08, 0x00);

       //prefetch 8 line

       cb_WriteU8(pcbe->pAdapterContext, CB_CRT_ADDR_REG, 0x8f);
       cb_WriteU8(pcbe->pAdapterContext, CB_CRT_DATA_REG, 0x80);


       cbBiosMMIOWriteReg(pcbe, CR_8F, 0x80, (CBIOS_U8)~0xc0, IGA2);
       cbBiosMMIOWriteReg(pcbe, CR_8F, 0x80, (CBIOS_U8)~0xc0, IGA3);
       cbBiosMMIOWriteReg(pcbe, CR_8F, 0x80, (CBIOS_U8)~0xc0, IGA4);

       // set HPD signal to high level active if this feature enabled
       if(pcbe->FeatureSwitch.HPDActiveHighEnable)
       {
           cbMMIOWriteReg32(pcbe, 0x334e0, 0, 0x7fffffff);
           cbMMIOWriteReg32(pcbe, 0x33d54, 0, 0x7fffffff);
           cbMMIOWriteReg32(pcbe, 0x34454, 0, 0x7fffffff);
           cbMMIOWriteReg32(pcbe, 0x34b54, 0, 0x7fffffff);
       }
       else
       {
           cbMMIOWriteReg32(pcbe, 0x334e0, 0x80000000, 0x7fffffff);
           cbMMIOWriteReg32(pcbe, 0x33d54, 0x80000000, 0x7fffffff);
           cbMMIOWriteReg32(pcbe, 0x34454, 0x80000000, 0x7fffffff);
           cbMMIOWriteReg32(pcbe, 0x34b54, 0x80000000, 0x7fffffff);       
       }

       if(pcbe->FeatureSwitch.BIUBackdoorEnable)
       {
           cbMMIOWriteReg(pcbe,CR_C_70, 0x00, 0xF7);
       }
       else
       {
           cbMMIOWriteReg(pcbe,CR_C_70, 0x08, 0xF7);
       }
       
       //Enable linear address access.
        cbMMIOWriteReg(pcbe,CR_C_A0,0x10, ~0x10);    
    }

    return CBIOS_OK;
}



