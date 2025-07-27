/*******************************************************************
* 
*         Copyright (c) 2007 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  MODE.C --- SMI DDK 
*  This file contains the source code for the mode table.
* 
*******************************************************************/
#include "linux/string.h"
#include <linux/delay.h>	
#include "ddk770_help.h"
#include "ddk770_reg.h"
#include "ddk770_os.h"
#include "ddk770_chip.h"
#include "ddk770_clock.h"
#include "ddk770_helper.h"
#include "ddk770_power.h"
#include "ddk770_mode.h"
#include "ddk770_ddkdebug.h"

/* The valid signature of the user data pointer  for the setmode function. 
   The following definition is ASCII representation of the word 'USER'
 */
#define MODE_USER_DATA_SIGNATURE            0x55534552





/*
 *  Default Timing parameter for some popular modes.
 *  Note that the most timings in this table is made according to standard VESA 
 *  parameters for the popular modes.
 */

static cea_parameter_t cea_mode_array[] = {
{1,   640,  160,  16,   96,  480,  45, 10, 2,  60000,  1, 0, 0, 0, 1, CEA_MODE_640x480p_60,        43,  25174999     },
{2,   720,  138,  16,   62,  480,  45, 9,  6,  60000,  1, 0, 0, 0, 1, CEA_MODE_720x480p_60,        43,  27027000     },
{3,   720,  138,  16,   62,  480,  45, 9,  6,  60000,  1, 0, 0, 0, 1, CEA_MODE_720x480p_60,        169, 27027000     },
{4,   1280, 370,  110,  40,  720,  30, 5,  5,  60000,  1, 1, 1, 0, 1, CEA_MODE_1280x720p_60,       169, 74250000     },
{5,   1920, 280,  88,   44,  540,  22, 2,  5,  60000,  1, 1, 1, 1, 0, CEA_MODE_1920x1080i_60,      169, 74250000     },
{6,   1440, 276,  38,   124, 240,  22, 4,  3,  60000,  1, 0, 0, 1, 0, CEA_MODE_1440x480i_60,       43,  27027000     },
{7,   1440, 276,  38,   124, 240,  22, 4,  3,  60000,  1, 0, 0, 1, 0, CEA_MODE_1440x480i_60,       169, 27027000     },
{8,   1440, 276,  38,   124, 240,  22, 4,  3,  60000,  1, 0, 0, 0, 1, CEA_MODE_1440x240p_60,       43,  27027000     },
{9,   1440, 276,  38,   124, 240,  22, 4,  3,  60000,  1, 0, 0, 0, 1, CEA_MODE_1440x240p_60,       169, 27027000     },
{10,  2880, 552,  76,   248, 240,  22, 4,  3,  59940,  1, 0, 0, 1, 0, CEA_MODE_2880x480i_59,       43,  54000000     },
{11,  2880, 552,  76,   248, 240,  22, 4,  3,  59940,  1, 0, 0, 1, 0, CEA_MODE_2880x480i_60,       169, 54000000     },
{12,  2880, 552,  76,   248, 240,  22, 4,  3,  59940,  1, 0, 0, 0, 1, CEA_MODE_2880x240p_60,       43,  54000000     },
{13,  2880, 552,  76,   248, 240,  22, 4,  3,  59940,  1, 0, 0, 0, 1, CEA_MODE_2880x240p_60,       169, 54000000     },
{14,  1440, 276,  32,   124, 480,  45, 9,  6,  59940,  1, 0, 0, 0, 1, CEA_MODE_1440x480p_60,       43,  54000000     },
{15,  1440, 276,  32,   124, 480,  45, 9,  6,  59940,  1, 0, 0, 0, 1, CEA_MODE_1440x480p_60,       169, 54000000     },
{16,  1920, 280,  88,   44,  1080, 45, 4,  5,  60000,  1, 1, 1, 0, 1, CEA_MODE_1920x1080p_60,      169, 148500000    },
{17,  720,  144,  12,   64,  576,  49, 5,  5,  50000,  1, 0, 0, 0, 1, CEA_MODE_720x576p_50,        43,  27027000     },
{18,  720,  144,  12,   64,  576,  49, 5,  5,  50000,  1, 0, 0, 0, 1, CEA_MODE_720x576p_50,        169, 27027000     },
{19,  1280, 700,  440,  40,  720,  30, 5,  5,  50000,  1, 1, 1, 0, 1, CEA_MODE_1280x720p_50,       169, 74250000     },
{20,  1920, 720,  528,  44,  540,  22, 2,  5,  50000,  1, 1, 1, 1, 0, CEA_MODE_1920x1080i_50,      169, 74250000     },
{21,  1440, 288,  24,   126, 288,  24, 2,  3,  50000,  1, 0, 0, 1, 0, CEA_MODE_1440x576i_50,       43,  27027000     },
{22,  1440, 288,  24,   126, 288,  24, 2,  3,  50000,  1, 0, 0, 1, 0, CEA_MODE_1440x576i_50,       169, 27027000     },
{23,  1440, 288,  24,   126, 288,  24, 2,  3,  50000,  1, 0, 0, 0, 1, CEA_MODE_1440x288p_50,       43,  27027000     },
{24,  1440, 288,  24,   126, 288,  24, 2,  3,  50000,  1, 0, 0, 0, 1, CEA_MODE_1440x288p_50,       169, 27027000     },
{25,  2880, 576,  48,   252, 288,  24, 2,  3,  50000,  1, 0, 0, 1, 0, CEA_MODE_2880x576i_50,       43,  54000000     },
{26,  2880, 576,  48,   252, 288,  24, 2,  3,  50000,  1, 0, 0, 1, 0, CEA_MODE_2880x576i_50,       169, 54000000     },
{27,  2880, 576,  48,   252, 288,  24, 2,  3,  50000,  1, 0, 0, 0, 1, CEA_MODE_2880x288p_50,       43,  54000000     },
{28,  2880, 576,  48,   252, 288,  24, 2,  3,  50000,  1, 0, 0, 0, 1, CEA_MODE_2880x288p_50,       169, 54000000     },
{29,  1440, 288,  24,   128, 576,  49, 5,  5,  50000,  1, 0, 0, 0, 1, CEA_MODE_1440x576p_50,       43,  54000000     },
{30,  1440, 288,  24,   128, 576,  49, 5,  5,  50000,  1, 0, 0, 0, 1, CEA_MODE_1440x576p_50,       169, 54000000     },
{31,  1920, 720,  528,  44,  1080, 45, 4,  5,  50000,  1, 1, 1, 0, 1, CEA_MODE_1920x1080p_50,      169, 148500000    },
{32,  1920, 830,  638,  44,  1080, 45, 4,  5,  24000,  1, 1, 1, 0, 1, CEA_MODE_1920x1080p_24,      169, 74250000     },
{33,  1920, 720,  528,  44,  1080, 45, 4,  5,  25000,  1, 1, 1, 0, 1, CEA_MODE_1920x1080p_25,      169, 74250000     },
{34,  1920, 280,  88,   44,  1080, 45, 4,  5,  30000,  1, 1, 1, 0, 1, CEA_MODE_1920x1080p_30,      169, 74250000     },
{35,  2880, 552,  64,   248, 480,  45, 9,  6,  59940,  1, 0, 0, 0, 1, CEA_MODE_2880x480p_59,       43,  108000000    },
{36,  2880, 552,  64,   248, 480,  45, 9,  6,  60000,  1, 0, 0, 0, 1, CEA_MODE_2880x480p_60,       169, 108000000    },
{37,  2880, 576,  48,   256, 576,  49, 5,  5,  50000,  1, 0, 0, 0, 1, CEA_MODE_2880x576p_60,       43,  108000000    },
{38,  2880, 576,  48,   256, 576,  49, 5,  5,  50000,  1, 0, 0, 0, 1, CEA_MODE_2880x576p_60,       169, 108000000    },
{39,  1920, 384,  32,   168, 540,  85, 23, 5,  50000,  1, 1, 0, 0, 0, CEA_MODE_1920x1080i_50,      169, 74250000     },
{40,  1920, 720,  528,  44,  540,  22, 2,  5,  100000, 1, 1, 1, 1, 0, CEA_MODE_1920x1080i_100,     169, 148500000    },
{41,  1280, 700,  440,  40,  720,  30, 5,  5,  100000, 1, 1, 1, 0, 1, CEA_MODE_1280x720p_100,      169, 148500000    },
{42,  720,  144,  12,   64,  576,  49, 5,  5,  100000, 1, 0, 0, 0, 1, CEA_MODE_720x576p_100,       43,  54000000     },
{43,  720,  144,  12,   64,  576,  49, 5,  5,  100000, 1, 0, 0, 0, 1, CEA_MODE_720x576p_100,       169, 54000000     },
{44,  1440, 288,  24,   126, 288,  24, 2,  3,  100000, 1, 0, 0, 1, 0, CEA_MODE_1440x576i_100,      43,  54000000     },
{45,  1440, 288,  24,   126, 288,  24, 2,  3,  100000, 1, 0, 0, 1, 0, CEA_MODE_1440x576i_100,      169, 54000000     },
{46,  1920, 288,  88,   44,  540,  22, 2,  5,  120000, 1, 1, 1, 1, 0, CEA_MODE_1920x1080i_120,     169, 148500000    },
{47,  1280, 370,  110,  40,  720,  30, 5,  5,  120000, 1, 1, 1, 0, 1, CEA_MODE_1280x720p_120,      169, 148500000    },
{48,  720,  138,  16,   62,  480,  45, 9,  6,  120000, 1, 0, 0, 0, 1, CEA_MODE_720x480p_120,       43,  54000000     },
{49,  720,  138,  16,   62,  480,  45, 9,  6,  120000, 1, 0, 0, 0, 1, CEA_MODE_720x480p_120,       169, 54000000     },
{50,  1440, 276,  38,   124, 240,  22, 4,  3,  120000, 1, 0, 0, 1, 0, CEA_MODE_1440x480i_120,      43,  54000000     },
{51,  1440, 276,  38,   124, 240,  22, 4,  3,  120000, 1, 0, 0, 1, 0, CEA_MODE_1440x480i_120,      169, 54000000     },
{52,  720,  144,  12,   64,  576,  49, 5,  5,  200000, 1, 0, 0, 0, 1, CEA_MODE_720x480i_0,         0,   108000000    },
{53,  720,  144,  12,   64,  576,  49, 5,  5,  200000, 1, 0, 0, 0, 1, CEA_MODE_720x480i_0,         0,   108000000    },
{54,  1440, 288,  24,   126, 288,  24, 2,  3,  200000, 1, 0, 0, 1, 0, CEA_MODE_1440x576i_200,      43,  108000000    },
{55,  1440, 288,  24,   126, 288,  24, 2,  3,  200000, 1, 0, 0, 1, 0, CEA_MODE_1440x576i_200,      169, 108000000    },
{56,  720,  138,  16,   62,  480,  45, 9,  6,  240000, 1, 0, 0, 0, 1, CEA_MODE_720x480p_240,       43,  108000000    },
{57,  720,  138,  16,   62,  480,  45, 9,  6,  240000, 1, 0, 0, 0, 1, CEA_MODE_720x480p_240,       169, 108000000    },
{58,  1440, 276,  38,   124, 240,  22, 4,  3,  240000, 1, 0, 0, 1, 0, CEA_MODE_1440x480i_240,      43,  108000000    },
{59,  1440, 276,  38,   124, 240,  22, 4,  3,  240000, 1, 0, 0, 1, 0, CEA_MODE_1440x480i_240,      169, 108000000    },
{60,  1280, 2020, 1760, 40,  720,  30, 5,  5,  24000,  1, 1, 1, 0, 1, CEA_MODE_1280x720p_24,       169, 54000000     },
{61,  1280, 2680, 2420, 40,  720,  30, 5,  5,  24000,  1, 1, 1, 0, 1, CEA_MODE_1280x720p_25,       169, 74250000     },
{62,  1280, 2020, 1760, 40,  720,  30, 5,  5,  30000,  1, 1, 1, 0, 1, CEA_MODE_1280x720p_30,       169, 74250000     },
{63,  1920, 280,  88,   44,  1080, 45, 4,  5,  120000, 1, 1, 1, 0, 1, CEA_MODE_1920x1080p_120,     169, 297000000    },
{64,  1920, 720,  528,  44,  1080, 45, 4,  5,  100000, 1, 1, 1, 0, 1, CEA_MODE_1920x1080p_100,     169, 297000000    },
{95,  3840, 560,  176,  88,  2160, 90, 8,  10, 30000,  1, 1, 1, 0, 1, CEA_MODE_3840x2160p_30,      169, 297000000    },
{94,  3840, 1440, 1056, 88,  2160, 90, 8,  10, 25000,  1, 1, 1, 0, 1, CEA_MODE_3840x2160p_25,      169, 297000000    },
{93, 3840, 1660, 1276, 88,  2160, 90, 8,  10,  24000,   1, 1, 1, 0,1, CEA_MODE_3840x2160p_24,      169, 297000000    },
{96,  3840, 1440, 1056, 88,  2160, 90, 9,  10, 50000,  1, 1, 1, 0, 1, CEA_MODE_3840x2160p_50,      169, 594000000    },
{97,  3840, 560,  176,  88,  2160, 90, 8,  10, 60000,  1, 1, 1, 0, 1, CEA_MODE_3840x2160p_60,      169, 594000000    },
{98,  4096, 1404, 1024, 88,  2160, 90, 8,  10, 24000,  1, 1, 1, 0, 1, CEA_MODE_4096x2160p_24,      169, 297000000    },
{102, 4096, 304,  88,   88,  2160, 90, 8,  10, 24000,  1, 1, 1, 0, 1, CEA_MODE_4096x2160p_60, 169, 600000000    },
{4,   1024, 344,  160,	136, 768,  30, 6,  23, 60000,  1, 1, 1, 0, 1, CEA_MODE_1024x768p_60,  43,  65000000	  },
{4,   800,  128,  16,	96,  600,  29, 2,  2,  60000,  1, 1, 1, 0, 1, CEA_MODE_800x600p_60,   43,  25175000	  },
{0,   2560, 160,  48,	32,  1440, 21, 3,  5,  60000,  1, 1, 1, 0, 1, CEA_MODE_2560x1440p_60, 169, 245250000	  },
{0,   2560, 160,  48,	32,  1440, 41, 3,  5,  144000, 1, 1, 1, 0, 1, CEA_MODE_2560x1440p_144,169, 580000000	  },

{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};


static mode_parameter_t gDefaultModeParamTable[] =
{
/* cea mode  */

    /* 640 x 480 [4:3] vic 1*/
    { 800, 640, 656, 96, NEG, 525, 480, 490, 2, NEG, 25175000, 31469, 60, NEG},

    /* 720 x 480 [4:3]/[16:9]  vic 2/3*/
    { 858, 720, 736, 62, NEG, 525, 480, 489, 6, NEG, 27000000, 31469, 60, NEG},

    /* 1280 x 720 [16:9] vic 4*/
    { 1650, 1280, 1390, 40, POS, 750, 720, 725, 5, POS, 74250000, 45000, 60, NEG},

    /* 1440 x 480 [4:3]/[16:9]  vic 14/15 */
    { 1716, 1440, 1472, 124, NEG, 525, 480, 489, 6, NEG, 54000000, 31469, 60, NEG},

    /* 1920 x 1080 [16:9] vic 16*/
    { 2200, 1920, 2008, 44, POS, 1125, 1080, 1084, 5, POS, 148500000, 67500, 60, NEG},

    /* 720 x 576 [4:3]/[16:9]    vic 17/18 */
    { 864, 720, 732, 64, NEG, 625, 576, 581, 5, NEG, 27000000, 31250, 50, NEG},

    /* 1280 x 720 [16:9] vic 19*/
    { 1980, 1280, 1720, 40, POS, 750, 720, 725, 5, POS, 74250000, 37500, 50, NEG},

    /* 1920 x 1080 [16:9] vic 31*/
    { 2640, 1920, 2448, 44, POS, 1125, 1080, 1084, 5, POS, 148500000, 56250, 50, NEG},

    /* 1920 x 1080 [16:9] vic 34*/
    { 2200, 1920, 2008, 44, POS, 1125, 1080, 1084, 5, POS, 74250000, 33750, 30, NEG},

    /* 2880 x 480 [4:3]/[16:9]  vic 35/36 */
    { 3432, 2880, 2944, 248, NEG, 525, 480, 489, 6, NEG, 108000000, 31469, 60, NEG},

    /* 1280 x 720 [16:9] vic 41*/
    { 1980, 1280, 1720, 40, POS, 750, 720, 725, 5, POS, 148500000, 45000, 100, NEG},

    /* 720 x 480 [16:9] vic 49*/
    { 858, 720, 736, 62, NEG, 525, 480, 489, 6, NEG, 54000000, 62937, 120, NEG},

    /* 1280 x 720 [16:9] vic 62*/
    { 3300, 1280, 3040, 40, POS, 750, 720, 725, 5, POS, 74250000, 22500, 30, NEG},

    /* 1920 x 1080 [16:9] vic 63*/
    { 2200, 1920, 2008, 44, POS, 1125, 1080, 1084, 5, POS, 297000000, 135000, 120, NEG},

    /* 1920 x 1080 [16:9] vic 64*/
    { 2640, 1920, 2448, 44, POS, 1125, 1080, 1084, 5, POS, 297000000, 112500, 100, NEG},

    /* 2560 x 1080 [64:27] vic 88*/
    { 3520, 2560, 3328, 44, POS, 1125, 1080, 1084, 5, POS, 118800000, 33750, 30, NEG},

    /* 2560 x 1080 [64:27] vic 90*/
    { 3000, 2560, 2808, 44, POS, 1100, 1080, 1084, 5, POS, 198000000, 66000, 60, NEG},

    /* 3840 x 2160 [16:9] vic 95*/
    { 4400, 3840, 4016, 88, POS, 2250, 2160, 2168, 10, POS, 297000000, 67500, 30, NEG},

    /* 3840 x 2160 [16:9] vic 96*/
    { 5280, 3840, 4896, 88, POS, 2250, 2160, 2168, 10, POS, 594000000, 112500, 50, NEG},

    /* 3840 x 2160 [16:9] vic 97*/
    { 4400, 3840, 4016, 88, POS, 2250, 2160, 2168, 10, POS, 594000000, 135000, 60, NEG},

/* other mode  Vesa/gtf*/
    /* 640 x 480  [4:3] */
    { 840, 640, 656, 64, NEG, 500, 480, 481, 3, NEG, 31500000, 37500, 75, NEG},
    { 832, 640, 696, 56, NEG, 509, 480, 481, 3, NEG, 36000000, 43269, 85, NEG},

    /* 720 x 540  [4:3] -- Not a popular mode */
    { 886, 720, 740, 96, POS, 576, 540, 545, 2, POS, 30600000, 34537, 60, NEG},

    /* 720 x 576  [4:3] */
 
	{ 912, 720, 744, 96, POS, 597, 576, 580, 1, NEG, 32670000, 35820, 60, POS}, 

    /* 800 x 480  [5:3] -- Not a popular mode */
    /* { 973, 800, 822, 56, POS, 524, 480, 490, 2, NEG, 30600000, 31449, 60, NEG},  //By Daniel for Futaba */
    {1056, 800, 1000, 10, NEG, 525, 480, 492, 10, NEG, 33300000, 31534, 60, POS },

    /* 800 x 600  [4:3] */
    {1056, 800, 840,128, POS, 628, 600, 601, 4, POS, 40000000, 37879, 60, NEG},
    {1056, 800, 816, 80, POS, 625, 600, 601, 3, POS, 49500000, 46875, 75, NEG},
    {1048, 800, 832, 64, POS, 631, 600, 601, 3, POS, 56250000, 53674, 85, NEG},

    /* 960 x 720  [4:3] -- Not a popular mode */
    {1245, 960, 992, 64, POS, 750, 720, 721, 3, POS, 56000000, 44980, 60, NEG},
        
    /* 1024 x 600  [16:9] 1.7 */
    {1313,1024,1064,104, POS, 622, 600, 601, 3, POS, 49000000, 37319, 60, NEG},
        
    /* 1024 x 768  [4:3] */
    {1344,1024,1048,136, NEG, 806, 768, 771, 6, NEG, 65000000, 48363, 60, NEG},
    {1312,1024,1040, 96, POS, 800, 768, 769, 3, POS, 78750000, 60023, 75, NEG},
    {1376,1024,1072, 96, POS, 808, 768, 769, 3, POS, 94500000, 68677, 85, NEG},
    
    /* 1152 x 864  [4:3] -- Widescreen eXtended Graphics Array */
    {1475,1152,1208, 96, POS, 888, 864, 866, 3, POS, 78600000, 53288, 60, NEG},
    {1600,1152,1216,128, POS, 900, 864, 865, 3, POS,108000000, 67500, 75, NEG},
    
    /* 1280 x 768  [5:3] -- Not a popular mode */
    {1664, 1280, 1344, 128, NEG, 798, 768, 771, 7, POS, 79500000, 47776, 60, NEG},

    /* 1280 x 800  [8:5] -- Not a popular mode */
    {1680, 1280, 1352, 128, NEG, 831, 800, 803, 6, POS, 83500000, 49702, 60, NEG},

    /* 1280 x 960  [4:3] */
    {1800,1280,1376,112, POS,1000, 960, 961, 3, POS,108000000, 60000, 60, NEG},
    {1728,1280,1344,160, POS,1011, 960, 961, 3, POS,148500000, 85938, 85, NEG},
        
    /* 1280 x 1024 [5:4] */
    /* VESA Standard */
    {1688,1280,1328,112, POS,1066,1024,1025, 3, POS,108000000, 63981, 60, NEG},
    {1688,1280,1296,144, POS,1066,1024,1025, 3, POS,135000000, 79976, 75, NEG},
    {1728,1280,1344,160, POS,1072,1024,1025, 3, POS,157500000, 91146, 85, NEG},

    /* 1360 x 768 [16:9] */
    {1792, 1360, 1424, 112, POS, 795, 768, 771, 6, POS, 85500000, 47712, 60, NEG},
 
    /* 1366 x 768  [16:9] */
    /* Previous Calculation  */
    {1792, 1366, 1436, 143, POS, 798, 768, 771, 3, POS, 85750000, 47852, 60, NEG},
 
    /* 1400 x 1050 [4:3]  */
    {1864, 1400, 1488, 144, NEG, 1089, 1050, 1053, 4, POS, 121750000, 65317, 60, NEG},
 
    /* 1440 x 900  [8:5] -- Widescreen Super eXtended Graphics Array (WSXGA) */
    {1904, 1440, 1520, 152, NEG, 934, 900, 903, 6, POS, 106500000, 55935, 60, NEG},

    /* 1440 x 960 [3:2] -- Not a popular mode */
    {1920,1440,1528,152, POS, 994, 960, 961, 3, POS,114509000, 59640, 60, NEG},

    /* 1600 x 900 [16:9] -- ViewSonic VS2033-LED */
    {1800, 1600, 1624, 80, POS, 1000, 900, 901, 3, POS, 108000000, 60000, 60, POS},

    /* 1600 x 1200 [4:3]. -- Ultra eXtended Graphics Array */
    /* VESA */
    {2160, 1600, 1664, 192, POS, 1250, 1200, 1201, 3, POS, 162000000, 75000, 60, NEG},
    {2160, 1600, 1664, 192, POS, 1250, 1200, 1201, 3, POS, 202500000, 93750, 75, NEG},
    {2160, 1600, 1664, 192, POS, 1250, 1200, 1201, 3, POS, 229500000, 106250, 85, NEG},
 
    /* 1680 x 1050 [8:5]. -- Widescreen Super eXtended Graphics Array Plus (WSXGA+) */ 
    {2240, 1680, 1784, 176, NEG, 1089, 1050, 1053, 6, POS, 146250000, 65290, 60, NEG},
    {2272, 1680, 1800, 176, NEG, 1099, 1050, 1053, 6, POS, 187000000, 82306, 75, NEG},
 
    /* 1792 x 1344 [4:3]. -- Not a popular mode */ 
    {2448, 1792, 1920, 200, NEG, 1394, 1344, 1345, 3, POS, 204750000, 836640, 60, NEG},
 
    /* 1856 x 1392 [4:3]. -- Not a popular mode 
   The 1856 x 1392 @ 75Hz has not been tested due to high Horizontal Frequency
   where not all monitor can support it (including the developer monitor)
    */
    {2528, 1856, 1952, 224, NEG, 1439, 1392, 1393, 3, POS, 218250000, 86333, 60, NEG},
    //{2560, 1856, 1984, 224, NEG, 1500, 1392, 1393, 3, POS, 288000000, 112500, 75, NEG}, 

    {2640, 1920, 2448, 44, POS, 1125, 1080, 1084, 5, POS, 148500000, 56250, 50, NEG},

    /* 1920 x 1200 [8:5]. -- Widescreen Ultra eXtended Graphics Array (WUXGA) */
    //{2592, 1920, 2048, 208, NEG, 1242, 1200, 1201, 3, POS, 193160000, 74522, 60, NEG},
    {2592, 1920, 2056, 200, NEG, 1245, 1200, 1203, 6, POS, 193250000, 74556, 60, NEG},

    /* 1920 x 1440 [4:3]. */
    /* In the databook, it mentioned only support up to 1920x1440 @ 60Hz. 
    The timing for 75 Hz is provided here if necessary to do testing. - Some underflow
    has been noticed. */
    {2600,1920,2048,208, NEG,1500,1440,1441, 3, POS,234000000, 90000, 60, NEG},
    /* {2640,1920,2064,224, NEG,1500,1440,1441, 3, POS,297000000,112500, 75, NEG}, */

    /* 2160 x 1200 for HTC_VIVE.*/
    {2260,2160,2200,20, NEG,1464,1200,1228, 2, NEG,297000000, 131000, 90, NEG},

    /*2K@120hz*/
    {2720,2560,2608,32, POS,1525,1440,1443, 5, POS,497750000, 182996, 120, NEG},
    /* 2560 * 1440 60 */
    {2720,2560,2608,32, POS,1481,1440,1443, 5, POS,241500000, 88000, 60, NEG},
    /* 2560 * 1440 144 */
    {2720,2560,2608,32, POS,1481,1440,1443, 5, POS,586000000, 213200, 144, NEG},
	/*3840x1080@60*/
	{3920,3840,3848,32, POS,1111,1080,1097, 8,POS,261307000, 66660, 60, POS},
	 
	/*3840x1080@120*/
	{4000,3840,3888,64, POS,1144,1080,1083, 10,POS,549000000, 137250, 120, POS},

	/*3440x1440@60*/
    {3600, 3440, 3488, 32, POS, 1481, 1440, 1443, 10, NEG, 319750000, 88819, 60, POS},
    /*3440x1440@100*/
     {3600, 3440, 3488, 64, NEG, 1490, 1440, 1443, 10, POS, 536400000, 149000, 100, POS},
   // {3760, 3440, 3488, 64, NEG, 1540, 1440, 1443, 10, POS, 536400000, 149000, 100, POS},
    //add vblank and hblank for 100hz,add sync time and then have voice
    //{3760, 3440, 3488, 64, NEG, 1540, 1440, 1443, 10, POS, 536400000, 149000, 100, POS},
    /*3440x1440@75*/
    {3600, 3440, 3488, 64, POS, 1492, 1440, 1443, 10, NEG, 402750000, 111875, 75, POS},
    /* End of table. */
    { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
};


static mode_parameter_t gChannel0ModeParamTable[MAX_MODE_TABLE_ENTRIES] =
{
    /* End of table */
     { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
};

static mode_parameter_t gChannel1ModeParamTable[MAX_MODE_TABLE_ENTRIES] =
{
    /* End of table */
     { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
};

static mode_parameter_t gChannel2ModeParamTable[MAX_MODE_TABLE_ENTRIES] =
{
    /* End of table */
     { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
};


/* Static variable to store the mode information. */
static mode_parameter_t gChannel0CurrentModeParam;
static mode_parameter_t gChannel1CurrentModeParam;
static mode_parameter_t gChannel2CurrentModeParam;

/*
 *  ddk770_getUserDataSignature
 *      This function gets the user data mode signature
 *
 *  Output:
 *      The signature to be filled in the user_data_mode_t structure to be considered
 *      a valid structure.
 */
unsigned long ddk770_getUserDataSignature()
{
    return MODE_USER_DATA_SIGNATURE;
}

/*
 *  ddk770_compareModeParam
 *      This function compares two mode parameters
 *
 *  Input:
 *      pModeParam1 - Pointer to the first mode parameter to be compared
 *      pModeParam2 - Pointer to the second mode parameter to be compared
 *
 *  Output:
 *      0   - Identical mode
 *     -1   - Mode is not identical
 */
long ddk770_compareModeParam(
    mode_parameter_t *pModeParam1,
    mode_parameter_t *pModeParam2
)
{
    if ((pModeParam1 != (mode_parameter_t *)0) &&
        (pModeParam2 != (mode_parameter_t *)0))
    {
        if (memcmp((void *)pModeParam1, (void *)pModeParam2, sizeof(mode_parameter_t)) == 0)
            return 0;
    }
        
    return (-1);
}

/*
 *  ddk770_getDuplicateModeIndex
 *      This function retrieves the index of dupicate modes, but having different timing.
 *
 *  Input:
 *      dispCtrl    - Display Control where the mode table belongs to.
 *      pModeParam  - The mode parameters which index to be checked.
 *
 *  Output:
 *      The index of the given parameters among the duplicate modes.
 *          0 means that the mode param is the first mode encountered in the table
 *          1 means that the mode param is the second mode encountered in the table
 *          etc...
 */
__attribute__((unused)) static unsigned short ddk770_getDuplicateModeIndex(
    disp_control_t dispCtrl,
    mode_parameter_t *pModeParam
)
{
    unsigned short index, modeIndex;
    mode_parameter_t *pModeTable;
    
    /* Get the mode table */
    pModeTable = ddk770_getStockModeParamTableEx(dispCtrl);
    
    /* Search the current mode */
    modeIndex = 0;
    index = 0;
    while (pModeTable[modeIndex].pixel_clock != 0)
    {
        if ((pModeTable[modeIndex].horizontal_display_end == pModeParam->horizontal_display_end) &&
            (pModeTable[modeIndex].vertical_display_end == pModeParam->vertical_display_end) &&
            (pModeTable[modeIndex].vertical_frequency == pModeParam->vertical_frequency))
        {
            /* Check if this is the same/identical mode param. */
            if (ddk770_compareModeParam(&pModeTable[modeIndex], pModeParam) == 0)
                break;
                
            /* Increment the index */
            index++;
        }
        modeIndex++;
    }
    
    return index;
}

/*
 *  ddk770_findModeParamFromTable
 *      This function locates the requested mode in the given parameter table
 *
 *  Input:
 *      width           - Mode width
 *      height          - Mode height
 *      refresh_rate    - Mode refresh rate
 *      index           - Index that is used for multiple search of the same mode 
 *                        that have the same width, height, and refresh rate, 
 *                        but have different timing parameters.
 *
 *  Output:
 *      Success: return a pointer to the mode_parameter_t entry.
 *      Fail: a NULL pointer.
 */
mode_parameter_t *ddk770_findModeParamFromTable(
    unsigned long width, 
    unsigned long height, 
    unsigned long refresh_rate,
    unsigned short index,
    mode_parameter_t *pModeTable
)
{
    unsigned short modeIndex = 0, tempIndex = 0;
    
    /* Walk the entire mode table. */    
    while (pModeTable[modeIndex].pixel_clock != 0)
    {
        if (((width == (unsigned long)(-1)) || (pModeTable[modeIndex].horizontal_display_end == width)) &&
            ((height == (unsigned long)(-1)) || (pModeTable[modeIndex].vertical_display_end == height)) &&
            ((refresh_rate == (unsigned long)(-1)) || (pModeTable[modeIndex].vertical_frequency == refresh_rate)))
        {
            if (tempIndex < index)
                tempIndex++;
            else
                return (&pModeTable[modeIndex]);
        }
        
        /* Next entry */
        modeIndex++;
    }

    /* No match, return NULL pointer */
    return((mode_parameter_t *)0);
}

/*
 *  Locate in-stock parameter table for the requested mode.
 *  Success: return a pointer to the mode_parameter_t entry.
 *  Fail: a NULL pointer.
 */
mode_parameter_t *ddk770_findModeParam(
    disp_control_t dispCtrl,
    unsigned long width, 
    unsigned long height, 
    unsigned long refresh_rate,
    unsigned short index
)
{
    return ddk770_findModeParamFromTable(width, height, refresh_rate, index, ddk770_getStockModeParamTableEx(dispCtrl));
}

/*
 *  Use the
 *  Locate timing parameter for the requested mode from the default mode table.
 *  Success: return a pointer to the mode_parameter_t entry.
 *  Fail: a NULL pointer.
 */
mode_parameter_t *ddk770_findVesaModeParam(
    unsigned long width, 
    unsigned long height, 
    unsigned long refresh_rate
)
{
    return ddk770_findModeParamFromTable(width, height, refresh_rate, 0, gDefaultModeParamTable);
}

/*
 * (Obsolete) 
 * Return a point to the gDefaultModeParamTable.
 * Function in other files used this to get the mode table pointer.
 */
mode_parameter_t *ddk770_getStockModeParamTable()
{
    return(gDefaultModeParamTable);
}

/*
 * (Obsolete)
 * Return the size of the Stock Mode Param Table
 */
unsigned long ddk770_getStockModeParamTableSize()
{
    return (sizeof(gDefaultModeParamTable) / sizeof(mode_parameter_t) - 1);
}

/* 
 *  ddk770_getStockModeParamTableEx
 *      This function gets the mode parameters table associated to the
 *      display control (CHANNEL0_CTRL or SECONDAR_CTRL).
 *
 *  Input:
 *      dispCtrl    - Display Control of the mode table that is associated to.
 *
 *  Output:
 *      Pointer to the mode table
 */
mode_parameter_t *ddk770_getStockModeParamTableEx(
    disp_control_t dispCtrl
)
{
    mode_parameter_t *pModeTable;
    
    if (dispCtrl == CHANNEL0_CTRL)
        pModeTable = (mode_parameter_t *)gChannel0ModeParamTable;
    else if (dispCtrl == CHANNEL1_CTRL)
        pModeTable = (mode_parameter_t *)gChannel1ModeParamTable;
    else
        pModeTable = (mode_parameter_t *)gChannel1ModeParamTable;
        
    /* Check if the table exist by checking the first entry. 
       If it doesn't, then use the default mode table. */

    if (pModeTable->pixel_clock == 0)
    {
        pModeTable = ddk770_getStockModeParamTable();
    }
        
    return (pModeTable);
}

/*
 *  ddk770_getStockModeParamTableSizeEx
 *      This function gets the size of the mode parameter table associated with
 *      specific display control
 *
 *  Input:
 *      dispCtrl    - Display control of the mode param table that is associated to.
 *
 *  Output:
 *      Size of the requeted mode param table.
 */
unsigned long ddk770_getStockModeParamTableSizeEx(
    disp_control_t dispCtrl
)
{
    unsigned long tableSize;
    mode_parameter_t *pModeTable;
    
    /* Get the mode table */
    pModeTable = ddk770_getStockModeParamTableEx(dispCtrl);
    
    /* Calculate the table size by finding the end of table entry indicated by all zeroes. */    
    tableSize = 0;
    while (pModeTable[tableSize].pixel_clock != 0)
        tableSize++;
        
    return tableSize;
}

/*
 *  ddk770_getMaximumModeEntries
 *      This function gets the maximum entries that can be stored in the mode table.
 *
 *  Output:
 *      Total number of maximum entries
 */
unsigned long ddk770_getMaximumModeEntries()
{
    return MAX_MODE_TABLE_ENTRIES;
}

/* 
 * This function returns the current mode.
 */
mode_parameter_t ddk770_getCurrentModeParam(
    disp_control_t dispCtrl
)
{
    if (dispCtrl == CHANNEL0_CTRL)
        return gChannel0CurrentModeParam;
    else if (dispCtrl == CHANNEL1_CTRL)
        return gChannel1CurrentModeParam;
    else
        return gChannel2CurrentModeParam;
}

/*
 *  ddk770_addTiming
 *      This function adds the SM750 mode parameter timing to the specified mode table
 *
 *  Input:
 *      dispCtrl        - Display control where the mode will be associated to
 *      pNewModeList    - Pointer to a list table of SM750 mode parameter to be added 
 *                        to the current specified display control mode table.
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long ddk770_addTiming(
    disp_control_t dispCtrl,
    mode_parameter_t *pNewModeList,
    unsigned long totalList,
    unsigned char clearTable
)
{
    mode_parameter_t *pModeParamTable;
    unsigned char index;
    long returnValue = 0;
    
    /* Get the correct table */
    if (dispCtrl == CHANNEL0_CTRL)
        pModeParamTable = (mode_parameter_t *)gChannel0ModeParamTable;
    else if(dispCtrl == CHANNEL1_CTRL)
        pModeParamTable = (mode_parameter_t *)gChannel1ModeParamTable;
    else
        pModeParamTable = (mode_parameter_t *)gChannel2ModeParamTable;
    if (clearTable == 0)
    {    
        /* Find the last index where the timing will be added to */
        index = 0;
        while(pModeParamTable[index].pixel_clock != 0)
            index++;
    }
    else
    {
        /* Clear and reset the mode table first */
        for (index = 0; index < MAX_MODE_TABLE_ENTRIES; index++)
            memset((void*)&pModeParamTable[index], 0, sizeof(mode_parameter_t));
            
        /* Reset index */
        index = 0;
    }
        
    /* Update the number of modes those can be added to the current table. */
    if (totalList > (unsigned long)(MAX_MODE_TABLE_ENTRIES - index))
        totalList = (unsigned long)(MAX_MODE_TABLE_ENTRIES - index);
    else
        returnValue = (-1);
    
    /* Check if totalList is 0, which means that the table is full. */        
    if (totalList == 0)
        returnValue = (-1);
        
    /* Add the list of modes provided by the caller */
    while (totalList--)
    {
        memcpy((void *)&pModeParamTable[index], (void *)&pNewModeList[index], sizeof(mode_parameter_t));
        index++;
    }
        
    return returnValue;
}

/*
 *    This function sets the display base address
 *
 *    Input:
 *        dispControl        - display control of which base address to be set.
 *        ulBaseAddress    - Base Address value to be set.
 */
void ddk770_setDisplayBaseAddress(
    disp_control_t dispControl,
    unsigned long ulBaseAddress
)
{
    unsigned long regFB;

    regFB = FB_ADDRESS + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);

        /* Frame buffer base for this mode */
    pokeRegisterDWord(regFB,
          FIELD_SET(0, FB_ADDRESS, STATUS, PENDING)
        | FIELD_VALUE(0, FB_ADDRESS, ADDRESS, ulBaseAddress));
}

/*
 *    This function checks if change of "display base address" has effective.
 *    Change of DC base address will not effective until next VSync, SW sets pending bit to 1 during address change.
 *    HW resets pending bit when it starts to use the new address.
 *
 *    Input:
 *        dispControl        - display control of which display status to be retrieved.
 *
 *  Output:
 *      1   - Display is pending
 *      0   - Display is not pending
 */
long ddk770_isDisplayBasePending(
    disp_control_t dispControl
)
{
    unsigned long regFB;

    regFB = FB_ADDRESS + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);

    if (FIELD_VAL_GET(peekRegisterDWord(regFB), FB_ADDRESS, STATUS) == FB_ADDRESS_STATUS_PENDING)
        return 1;

    return (0);
}



/* 
 * Program the hardware for a specific video mode
 *
 * return:
 *         0 = success
 *        -1 = fail.
 */
static long ddk770_programModeRegisters(
logicalMode_t *pLogicalMode, 
mode_parameter_t *pModeParam,   /* mode information about pixel clock, horizontal total, etc. */
pll_value_t *pPLL               /* Pre-calculated values for the PLL */
)
{
    unsigned long ulTmpValue;
    unsigned long paletteRam;
    unsigned long offset, pllReg;
	unsigned long LineOffset;

	LineOffset = alignLineOffset(pLogicalMode->pitch);


    /*  Make sure normal display channel is used, not VGA channel */
    pokeRegisterDWord(VGA_CONFIGURATION,
          FIELD_SET(0, VGA_CONFIGURATION, PLL, PANEL)
        | FIELD_SET(0, VGA_CONFIGURATION, MODE, GRAPHIC));

    offset = (pLogicalMode->dispCtrl > 1)? CHANNEL_OFFSET2 : pLogicalMode->dispCtrl * CHANNEL_OFFSET;
 	if(pLogicalMode->dispCtrl==CHANNEL0_CTRL)
		pllReg = VCLK_PLL;
	else if (pLogicalMode->dispCtrl==CHANNEL1_CTRL)
		pllReg = VCLK1_PLL;
	else
		pllReg = VCLK2_PLL;
  
     /* Program PLL */
		
	SetPllReg(pllReg,pPLL);

 

    /* Frame buffer base */
    pokeRegisterDWord((FB_ADDRESS+offset),
          FIELD_SET(0, FB_ADDRESS, STATUS, PENDING)
        | FIELD_VALUE(0, FB_ADDRESS, ADDRESS, pLogicalMode->baseAddress));

    /* Pitch value (Hardware people calls it Offset) */
    pokeRegisterDWord((FB_WIDTH+offset),
          FIELD_VALUE(0, FB_WIDTH, WIDTH, pLogicalMode->pitch)
        | FIELD_VALUE(0, FB_WIDTH, OFFSET, LineOffset));

    pokeRegisterDWord((HORIZONTAL_TOTAL+offset),
          FIELD_VALUE(0, HORIZONTAL_TOTAL, TOTAL, pModeParam->horizontal_total - 1)
        | FIELD_VALUE(0, HORIZONTAL_TOTAL, DISPLAY_END, pModeParam->horizontal_display_end - 1));

    pokeRegisterDWord((HORIZONTAL_SYNC+offset),
          FIELD_VALUE(0, HORIZONTAL_SYNC, WIDTH, pModeParam->horizontal_sync_width)
        | FIELD_VALUE(0, HORIZONTAL_SYNC, START, pModeParam->horizontal_sync_start - 1));


    pokeRegisterDWord((VERTICAL_TOTAL+offset),
          FIELD_VALUE(0, VERTICAL_TOTAL, TOTAL, pModeParam->vertical_total - 1)
        | FIELD_VALUE(0, VERTICAL_TOTAL, DISPLAY_END, pModeParam->vertical_display_end - 1));

    pokeRegisterDWord((VERTICAL_SYNC+offset),
          FIELD_VALUE(0, VERTICAL_SYNC, HEIGHT, pModeParam->vertical_sync_height)
        | FIELD_VALUE(0, VERTICAL_SYNC, START, pModeParam->vertical_sync_start - 1));

    /* Set control register value */
    ulTmpValue = peekRegisterDWord(DISPLAY_CTRL+offset)
               & FIELD_CLEAR(DISPLAY_CTRL, DPMS)
               & FIELD_CLEAR(DISPLAY_CTRL, DATA_PATH)
               & FIELD_CLEAR(DISPLAY_CTRL, CLOCK_PHASE)
               & FIELD_CLEAR(DISPLAY_CTRL, VSYNC_PHASE)
               & FIELD_CLEAR(DISPLAY_CTRL, HSYNC_PHASE)
               & FIELD_CLEAR(DISPLAY_CTRL, DOUBLE_PIXEL_CLOCK)
               & FIELD_CLEAR(DISPLAY_CTRL, COLOR_KEY)
               & FIELD_CLEAR(DISPLAY_CTRL, TIMING)
               & FIELD_CLEAR(DISPLAY_CTRL, PLANE)
               & FIELD_CLEAR(DISPLAY_CTRL, FORMAT);

    ulTmpValue |=        
        FIELD_SET(0, DISPLAY_CTRL, DATA_PATH, EXTENDED)
      | (pModeParam->clock_phase_polarity == POS
        ? FIELD_SET(0, DISPLAY_CTRL, CLOCK_PHASE, ACTIVE_HIGH)
        : FIELD_SET(0, DISPLAY_CTRL, CLOCK_PHASE, ACTIVE_LOW))
      | (pModeParam->vertical_sync_polarity == POS
        ? FIELD_SET(0, DISPLAY_CTRL, VSYNC_PHASE, ACTIVE_HIGH)
        : FIELD_SET(0, DISPLAY_CTRL, VSYNC_PHASE, ACTIVE_LOW))
      | (pModeParam->horizontal_sync_polarity == POS
        ? FIELD_SET(0, DISPLAY_CTRL, HSYNC_PHASE, ACTIVE_HIGH)
        : FIELD_SET(0, DISPLAY_CTRL, HSYNC_PHASE, ACTIVE_LOW))
      | FIELD_SET(0, DISPLAY_CTRL, TIMING, ENABLE)
      | FIELD_SET(0, DISPLAY_CTRL, PLANE, ENABLE) 
      | (pLogicalMode->bpp == 8
        ? FIELD_SET(0, DISPLAY_CTRL, FORMAT, 8)
        : (pLogicalMode->bpp == 16
        ? FIELD_SET(0, DISPLAY_CTRL, FORMAT, 16)
        : FIELD_SET(0, DISPLAY_CTRL, FORMAT, 32)));

    pokeRegisterDWord((DISPLAY_CTRL+offset), ulTmpValue);

    /* Palette RAM. */
    paletteRam = PALETTE_RAM + offset;
    
    /* Save the current mode param */
    if (pLogicalMode->dispCtrl == CHANNEL0_CTRL)
        gChannel0CurrentModeParam = *pModeParam;
    else if(pLogicalMode->dispCtrl == CHANNEL1_CTRL)
        gChannel1CurrentModeParam = *pModeParam;
    else 
        gChannel2CurrentModeParam = *pModeParam;

    /* In case of 8-bpp, fill palette */
    if (pLogicalMode->bpp==8)
    {
        /* Start with RGB = 0,0,0. */
        unsigned char red = 0, green = 0, blue = 0;
        unsigned long gray = 0;
        for (offset = 0; offset < 256 * 4; offset += 4)
        {
            /* Store current RGB value. */
            pokeRegisterDWord(paletteRam + offset, gray
                                ? RGB((gray + 50) / 100,
                                      (gray + 50) / 100,
                                      (gray + 50) / 100)
                                : RGB(red, green, blue));

            if (gray)
            {
                /* Walk through grays (40 in total). */
                gray += 654;
            }

            else
            {
                /* Walk through colors (6 per base color). */
                if (blue != 255)
                {
                    blue += 51;
                }
                else if (green != 255)
                {
                    blue = 0;
                    green += 51;
                }
                else if (red != 255)
                {
                    green = blue = 0;
                    red += 51;
                }
                else
                {
                    gray = 1;
                }
            }
        }
    }
    /* For 16- and 32-bpp,  fill palette with gamma values. */
    else
    {
        /* Start with RGB = 0,0,0. */
        ulTmpValue = 0x000000;
        for (offset = 0; offset < 256 * 4; offset += 4)
        {
            pokeRegisterDWord(paletteRam + offset, ulTmpValue);

            /* Advance RGB by 1,1,1. */
            ulTmpValue += 0x010101;
        }
    }

    return 0;
}

/*
 * Input:
 *     1) pLogicalMode contains information such as x, y resolution and bpp.
 *     2) A user defined parameter table for the mode.
 *
 * This function calculate and programs the hardware to set up the
 * requested mode.
 *
 * This function allows the use of user defined parameter table if
 * predefined Vesa parameter table (gDefaultModeParamTable) does not fit.
 *
 * Return: 0 (or NO_ERROR) if mode can be set successfully.
 *         -1 if any set mode error.
 */
long ddk770_setCustomMode(
    logicalMode_t *pLogicalMode, 
    mode_parameter_t *pUserModeParam
)
{
   // mode_parameter_t pModeParam; /* physical parameters for the mode */
    pll_value_t pll;
    //unsigned long ulTemp;

    /*
     * Minimum check on mode base address.
     * At least it shouldn't be bigger than the size of frame buffer.
     */
    if (ddk770_getFrameBufSize() <= pLogicalMode->baseAddress)
    {
        return -1;
    }
        
	pll.inputFreq = 12500000;  //12.5MHz //DEFAULT_INPUT_CLOCK;
    ddk770_calcPllValue(pUserModeParam->pixel_clock, &pll);

    /* If calling function don't have a preferred pitch value, 
       work out a 16 byte aligned pitch value.
    */
    if (pLogicalMode->pitch == 0)
    {
        /* 
         * Pitch value calculation in Bytes.
         * Usually, it is (screen width) * (byte per pixel).
         * However, there are cases that screen width is not 16 pixel aligned, which is
         * a requirement for some OS and the hardware itself.
         * For standard 4:3 resolutions: 320, 640, 800, 1024 and 1280, they are all
         * 16 pixel aligned and pitch is simply (screen width) * (byte per pixel).
         *   
         * However, 1366 resolution, for example, has to be adjusted for 16 pixel aligned.
         */
        pLogicalMode->pitch = PITCH(pLogicalMode->x, pLogicalMode->bpp);
    }

    /* Program the hardware to set up the mode. */
    return( ddk770_programModeRegisters( 
            pLogicalMode, 
            pUserModeParam,
            &pll));
}

static unsigned long getActurePixelClock(unsigned long pixelClock)
{
    pll_value_t pll;
    pll.inputFreq = 12500000;  //12.5MHz //DEFAULT_INPUT_CLOCK;
    return ddk770_calcPllValue(pixelClock, &pll);
}
/*
 * Input pLogicalMode contains information such as x, y resolution and bpp 
 * The main difference between ddk770_setMode and setModeEx userData.
 *
 * Return: 0 (or NO_ERROR) if mode can be set successfully.
 *         -1 if any set mode error.
 */
long ddk770_setModeEx(
    logicalMode_t *pLogicalMode
)
{
    mode_parameter_t *pModeParam;       /* physical parameters for the mode */
    unsigned short index = 0;
    userData_t *pUserData;

    /*
     * Check the validity of the userData pointer and translate the information as necessary
     */
    pUserData = (userData_t *)pLogicalMode->userData;
    if ((pUserData != (userData_t *)0) &&
        (pUserData->signature == ddk770_getUserDataSignature()) &&
        (pUserData->size == sizeof(userData_t)))
    {
        /* Interpret the userData information */
        if (pUserData->paramList.size == sizeof(userDataParam_t))
        {
            if (pUserData->paramList.modeInfoID == MODE_INFO_INDEX)
                index = pUserData->paramList.paramInfo.index;
        }
    }
    
    /* 
     * Check if we already have physical timing parameter for this mode.
     */
    pModeParam = ddk770_findModeParam(pLogicalMode->dispCtrl, pLogicalMode->x, pLogicalMode->y, pLogicalMode->hz, index);
    if (pModeParam == (mode_parameter_t *)0)
        return -1;

    return(ddk770_setCustomMode(pLogicalMode, pModeParam));
}

/*
 * Input pLogicalMode contains information such as x, y resolution and bpp.
 * If there is no special parameters, use this function.
 * If there are special parameters, use setModeEx.
 *
 * Return: 0 (or NO_ERROR) if mode can be set successfully.
 *         -1 if any set mode error.
 */
long ddk770_setMode(
    logicalMode_t *pLogicalMode
)
{
    pLogicalMode->userData = (void *)0;

    /* Call the setModeEx to set the mode. */
    return ddk770_setModeEx(pLogicalMode);
}


unsigned short alignLineOffset(unsigned short lineOffset)
{

	if(0 == lineOffset  || lineOffset > 0xff00)
		return lineOffset;

    return (lineOffset + 255) & ~255;
    
}

long Get_CEA_Mode(logicalMode_t *pLogicalMode, cea_parameter_t *cea_mode, mode_parameter_t *pModeParam, unsigned int isDP)
{
    int i;
    unsigned long acturePixelClock;

	if (!pLogicalMode->valid_edid)
    {
	    pModeParam = ddk770_findModeParam(pLogicalMode->dispCtrl, pLogicalMode->x, pLogicalMode->y, pLogicalMode->hz, 0);
	    if (pModeParam == (mode_parameter_t *)0)
	    {
	        printk("ddk770_findModeParam: no mode found");
	        return -1;
	    }
	}
    cea_mode->de_polarity = 1;
    cea_mode->vblank_osc = 0;
    cea_mode->progressive_nI = 1;
    cea_mode->hdmi_mode = 1;
    cea_mode->aspect_ratio = 169;
    cea_mode->vic = 0;

    cea_mode->h_active = pModeParam->horizontal_display_end;
    cea_mode->h_blank = pModeParam->horizontal_total - pModeParam->horizontal_display_end;
    cea_mode->hsync_delay = pModeParam->horizontal_sync_start - pModeParam->horizontal_display_end;
    cea_mode->hsync_width = pModeParam->horizontal_sync_width;

    cea_mode->v_active = pModeParam->vertical_display_end;
    cea_mode->v_blank = pModeParam->vertical_total - pModeParam->vertical_display_end;
    cea_mode->vsync_delay = pModeParam->vertical_sync_start - pModeParam->vertical_display_end;
    cea_mode->vsync_width = pModeParam->vertical_sync_height;

    cea_mode->freq = pLogicalMode->hz * 1000;

    cea_mode->hsync_polarity = pModeParam->horizontal_sync_polarity == POS ? 1 : 0;
    cea_mode->vsync_polarity = pModeParam->vertical_sync_polarity == POS ? 1 : 0;

    if(isDP){
    	acturePixelClock = getActurePixelClock(pModeParam->pixel_clock);
        cea_mode->tmds_clk = acturePixelClock / 1000;
    }else{
		acturePixelClock = pModeParam->pixel_clock; 
        cea_mode->tmds_clk = pModeParam->pixel_clock / 1000;
	}
    printk(" acturePixelClock= %ld\n", acturePixelClock);

    for (i = 0; cea_mode_array[i].h_active != 0; i++)   //Check if the modes can be found in cea table 
    {
        if ((cea_mode->h_active == cea_mode_array[i].h_active) &&
            (cea_mode->v_active == cea_mode_array[i].v_active) &&
            (cea_mode->freq == cea_mode_array[i].freq))
        {
        	cea_mode->de_polarity = cea_mode_array[i].de_polarity;
            cea_mode->vblank_osc = cea_mode_array[i].vblank_osc;
            cea_mode->progressive_nI = cea_mode_array[i].progressive_nI;
            cea_mode->hdmi_mode = cea_mode_array[i].hdmi_mode;
            cea_mode->aspect_ratio = cea_mode_array[i].aspect_ratio;
            cea_mode->vic = cea_mode_array[i].vic;
            return 0;
        }
    }

	return 0;
}