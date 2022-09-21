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
//#include <string.h>
#include "ddk750_defs.h"
#include "ddk750_chip.h"
#include "ddk750_clock.h"
#include "ddk750_hardware.h"
#include "ddk750_helper.h"
#include "ddk750_power.h"
#include "ddk750_mode.h"
#include "ddk750_help.h"

//#include "ddk750_os.h"

//#include "ddkdebug.h"

#define SCALE_CONSTANT                      (1 << 12)

/* Maximum panel size scaling */
#define MAX_PANEL_SIZE_WIDTH                1920
#define MAX_PANEL_SIZE_HEIGHT               1440

/* The valid signature of the user data pointer  for the setmode function. 
   The following definition is ASCII representation of the word 'USER'
 */
#define MODE_USER_DATA_SIGNATURE            0x55534552

/*
 *  Default Timing parameter for some popular modes.
 *  Note that the most timings in this table is made according to standard VESA 
 *  parameters for the popular modes.
 */
static mode_parameter_t gDefaultModeParamTable[] =
    {
        /*800x480*/
        //{0, 	800, 	0,	0,	NEG, 0, 480,  0,  0,   POS, 	0,		0,		0, NEG},
        /* 320 x 240 [4:3] */
        {352, 320, 335, 8, NEG, 265, 240, 254, 2, NEG, 5600000, 15909, 60, NEG},

        /* 400 x 300 [4:3] -- Not a popular mode */
        {528, 400, 420, 64, NEG, 314, 300, 301, 2, NEG, 9960000, 18864, 60, NEG},

        /* 480 x 272 -- Not a popular mode --> only used for panel testing */
        /* { 525, 480, 482, 41, NEG, 286, 272, 274, 10, NEG, 9009000, 17160, 60, NEG}, */

        /* 640 x 480  [4:3] */
        /* The first 2 commented lines below are taken from SM502, the rest timing are
   taken from the VESA Monitor Timing Standard */
        /* { 840, 640, 680, 64, NEG, 500, 480, 481, 3, NEG, 31500000, 37500, 75, NEG}, */
        /* { 832, 640, 700, 56, NEG, 509, 480, 481, 3, NEG, 36000000, 43269, 85, NEG}, */
        {800, 640, 656, 96, NEG, 525, 480, 490, 2, NEG, 25175000, 31469, 60, NEG},
        {840, 640, 656, 64, NEG, 500, 480, 481, 3, NEG, 31500000, 37500, 75, NEG},
        {832, 640, 696, 56, NEG, 509, 480, 481, 3, NEG, 36000000, 43269, 85, NEG},

        /* 720 x 480  [3:2] */
        {889, 720, 738, 108, POS, 525, 480, 490, 2, NEG, 28000000, 31496, 60, NEG},

        /* 720 x 540  [4:3] -- Not a popular mode */
        {886, 720, 740, 96, POS, 576, 540, 545, 2, POS, 30600000, 34537, 60, NEG},

        /* 800 x 480  [5:3] -- Not a popular mode */
        {973, 800, 822, 56, POS, 524, 480, 490, 2, NEG, 30600000, 31449, 60, NEG},

        /* 800 x 600  [4:3] */
        /* The first 2 commented lines below are taken from SM502, the rest timing are
   taken from the VESA Monitor Timing Standard */
        /* {1062, 800, 840,128, POS, 628, 600, 601, 4, POS, 40000000, 37665, 60, NEG}, */
        /* {1054, 800, 842, 64, POS, 625, 600, 601, 3, POS, 56000000, 53131, 85, NEG}, */
        {1056, 800, 840, 128, POS, 628, 600, 601, 4, POS, 40000000, 37879, 60, NEG},
        {1056, 800, 816, 80, POS, 625, 600, 601, 3, POS, 49500000, 46875, 75, NEG},
        {1048, 800, 832, 64, POS, 631, 600, 601, 3, POS, 56250000, 53674, 85, NEG},

        /* 960 x 720  [4:3] -- Not a popular mode */
        {1245, 960, 992, 64, POS, 750, 720, 721, 3, POS, 56000000, 44980, 60, NEG},

        /* 1024 x 600  [16:9] 1.7 */
        {1313, 1024, 1064, 104, POS, 622, 600, 601, 3, POS, 49000000, 37319, 60, NEG},

        /* 1024 x 768  [4:3] */
        /* The first 2 commented lines below are taken from SM502, the rest timing are
   taken from the VESA Monitor Timing Standard */
        /* {1340,1024,1060,136, NEG, 809, 768, 772, 6, NEG, 65000000, 48507, 60, NEG}, */
        /* {1337,1024,1072, 96, NEG, 808, 768, 780, 3, NEG, 81000000, 60583, 75, NEG}, */
        {1344, 1024, 1048, 136, NEG, 806, 768, 771, 6, NEG, 65000000, 48363, 60, NEG},
        {1312, 1024, 1040, 96, POS, 800, 768, 769, 3, POS, 78750000, 60023, 75, NEG},
        {1376, 1024, 1072, 96, POS, 808, 768, 769, 3, POS, 94500000, 68677, 85, NEG},

        /* 1152 x 864  [4:3] -- Widescreen eXtended Graphics Array */
        /* {1475,1152,1208, 96, NEG, 888, 864, 866, 3, NEG, 78600000, 53288, 60, NEG},*/
        {1475, 1152, 1208, 96, POS, 888, 864, 866, 3, POS, 78600000, 53288, 60, NEG},
        {1600, 1152, 1216, 128, POS, 900, 864, 865, 3, POS, 108000000, 67500, 75, NEG},

        /* 1280 x 720  [16:9] -- HDTV (WXGA) */
        {1664, 1280, 1336, 136, POS, 746, 720, 721, 3, POS, 74481000, 44760, 60, NEG},

        /* 1280 x 768  [5:3] -- Not a popular mode */
        {1678, 1280, 1350, 136, POS, 795, 768, 769, 3, POS, 80000000, 47676, 60, NEG},

        /* 1280 x 800  [8:5] -- Not a popular mode */
        {1650, 1280, 1344, 136, NEG, 824, 800, 800, 3, NEG, 81600000, 49455, 60, NEG},

        /* 1280 x 960  [4:3] */
        /* The first commented line below are taken from SM502, the rest timing are
   taken from the VESA Monitor Timing Standard */
        /* {1618,1280,1330, 96, NEG, 977, 960, 960, 2, NEG, 94500000, 59259, 60, NEG},*/
        {1800, 1280, 1376, 112, POS, 1000, 960, 961, 3, POS, 108000000, 60000, 60, NEG},
        {1728, 1280, 1344, 160, POS, 1011, 960, 961, 3, POS, 148500000, 85938, 85, NEG},

/* 1280 x 1024 [5:4] */
#if 1
        /* GTF with C = 40, M = 600, J = 20, K = 128 */
        {1712, 1280, 1360, 136, NEG, 1060, 1024, 1025, 3, POS, 108883200, 63600, 60, NEG},
        {1728, 1280, 1368, 136, NEG, 1069, 1024, 1025, 3, POS, 138542400, 80175, 75, NEG},
        {1744, 1280, 1376, 136, NEG, 1075, 1024, 1025, 3, POS, 159358000, 91375, 85, NEG},
#else
        /* VESA Standard */
        {1688, 1280, 1328, 112, POS, 1066, 1024, 1025, 3, POS, 108000000, 63981, 60, NEG},
        {1688, 1280, 1296, 144, POS, 1066, 1024, 1025, 3, POS, 135000000, 79976, 75, NEG},
        {1728, 1280, 1344, 160, POS, 1072, 1024, 1025, 3, POS, 157500000, 91146, 85, NEG},
#endif

/* 1360 x 768 [16:9] */
#if 1
        /* GTF with C = 40, M = 600, J = 20, K = 128 */
        //{1776,1360,1432,136, NEG, 795, 768, 769, 3, POS, 84715200, 47700, 60, NEG},

        /* GTF with C = 30, M = 600, J = 20, K = 128 */
        {1664, 1360, 1384, 128, NEG, 795, 768, 769, 3, POS, 79372800, 47700, 60, NEG},
#else
        /* Previous Calculation */
        {1776, 1360, 1424, 144, POS, 795, 768, 769, 3, POS, 84715000, 47700, 60, NEG},
#endif

        /* 1366 x 768  [16:9] */
        /* Previous Calculation  */
        {1722, 1366, 1424, 112, NEG, 784, 768, 769, 3, NEG, 81000000, 47038, 60, NEG},

        /* 1400 x 1050 [4:3] -- Hitachi TX38D95VC1CAH -- It is not verified yet, therefore
   temporarily disabled. */
        //{1688,1400,1448,112, NEG,1068,1050,1051, 3, NEG,108000000, 64000, 60, NEG},
        //{1688,1400,1464,112, NEG,1068,1050,1051, 3, NEG,108167040, 64080, 60, NEG},

        /* Taken from the www.tinyvga.com */
        {1880, 1400, 1488, 152, NEG, 1087, 1050, 1051, 3, POS, 122610000, 65218, 60, NEG},

        /* 1440 x 900  [8:5] -- Widescreen Super eXtended Graphics Array (WSXGA) */
        {1904, 1440, 1520, 152, NEG, 932, 900, 901, 3, POS, 106470000, 55919, 60, NEG},

        /* 1440 x 960 [3:2] -- Not a popular mode */
        {1920, 1440, 1528, 152, POS, 994, 960, 961, 3, POS, 114509000, 59640, 60, NEG},

        /* 1600 x 900 */
        {2128, 1600, 1664, 192, POS, 932, 900, 901, 3, POS, 119000000, 56000, 60, NEG},

        /* 1600 x 1200 [4:3]. -- Ultra eXtended Graphics Array */
        /* VESA */
        {2160, 1600, 1664, 192, POS, 1250, 1200, 1201, 3, POS, 162000000, 75000, 60, NEG},
        {2160, 1600, 1664, 192, POS, 1250, 1200, 1201, 3, POS, 202500000, 93750, 75, NEG},
        {2160, 1600, 1664, 192, POS, 1250, 1200, 1201, 3, POS, 229500000, 106250, 85, NEG},

        /* 
 * The timing below is taken from the www.tinyvga.com/vga-timing.
 * With the exception of 1920x1080.
 */

        /* 1680 x 1050 [8:5]. -- Widescreen Super eXtended Graphics Array Plus (WSXGA+) */
        /* The first commented timing might be used for DVI LCD Monitor timing. */
        /* {1840,1680,1728, 32, NEG,1080,1050,1053, 6, POS,119232000, 64800, 60, NEG}, */
        /* GTF with C = 30, M = 600, J = 20, K = 128 */
        {2256, 1680, 1784, 184, NEG, 1087, 1050, 1051, 3, POS, 147140000, 65222, 60, NEG},
        /*
 {2272,1680,1792,184, NEG,1093,1050,1051, 3, POS,173831000, 76510, 70, NEG},
 {2288,1680,1800,184, NEG,1096,1050,1051, 3, POS,188074000, 82200, 75, NEG},
*/

        /* 1792 x 1344 [4:3]. -- Not a popular mode */
        {2448, 1792, 1920, 200, NEG, 1394, 1344, 1345, 3, POS, 204800000, 83660, 60, NEG},
        {2456, 1792, 1888, 216, NEG, 1417, 1344, 1345, 3, POS, 261000000, 106270, 75, NEG},

        /* 1856 x 1392 [4:3]. -- Not a popular mode 
   The 1856 x 1392 @ 75Hz has not been tested due to high Horizontal Frequency
   where not all monitor can support it (including the developer monitor)
 */
        {2528, 1856, 1952, 224, NEG, 1439, 1392, 1393, 3, POS, 218300000, 86353, 60, NEG},
        /* {2560,1856,1984,224, NEG,1500,1392,1393, 3, POS,288000000,112500, 75, NEG},*/

        /* 1920 x 1080 [16:9]. This is a make-up value, need to be proven. 
   The Pixel clock is calculated based on the maximum resolution of
   "Single Link" DVI, which support a maximum 165MHz pixel clock.
   The second values are taken from:
   http://www.tek.com/Measurement/App_Notes/25_14700/eng/25W_14700_3.pdf
 */
        /* {2560,1920,2048,208, NEG,1125,1080,1081, 3, POS,172800000, 67500, 60, NEG}, */
        {2200, 1920, 2008, 44, NEG, 1125, 1080, 1081, 3, POS, 148500000, 67500, 60, NEG},

        /* 1920 x 1200 [8:5]. -- Widescreen Ultra eXtended Graphics Array (WUXGA) */
        {2592, 1920, 2048, 208, NEG, 1242, 1200, 1201, 3, POS, 193160000, 74522, 60, NEG},

        /* 1920 x 1440 [4:3]. */
        /* In the databook, it mentioned only support up to 1920x1440 @ 60Hz. 
   The timing for 75 Hz is provided here if necessary to do testing. - Some underflow
   has been noticed. */
        {2600, 1920, 2048, 208, NEG, 1500, 1440, 1441, 3, POS, 234000000, 90000, 60, NEG},
        /* {2640,1920,2064,224, NEG,1500,1440,1441, 3, POS,297000000,112500, 75, NEG}, */

        /* End of table. */
        {0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
};

#if 0
static mode_parameter_t gPrimaryModeParamTable[MAX_SMI_DEVICE][MAX_MODE_TABLE_ENTRIES] =
{
    {
    /* End of table */
     { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
    },
    {
    /* End of table */
     { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
    },
    {
    /* End of table */
     { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
    },
    {
    /* End of table */
     { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
    }
};

static mode_parameter_t gSecondaryModeParamTable[MAX_SMI_DEVICE][MAX_MODE_TABLE_ENTRIES] =
{
    {
    /* End of table */
     { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
    },
    {
    /* End of table */
     { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
    },
    {
    /* End of table */
     { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
    },
    {
    /* End of table */
     { 0, 0, 0, 0, NEG, 0, 0, 0, 0, NEG, 0, 0, 0, NEG},
    }
};
#endif

/* Static variable to store the mode information. */
static mode_parameter_t gPrimaryCurrentModeParam[MAX_SMI_DEVICE];
static mode_parameter_t gSecondaryCurrentModeParam[MAX_SMI_DEVICE];

/*
 *  getUserDataSignature
 *      This function gets the user data mode signature
 *
 *  Output:
 *      The signature to be filled in the user_data_mode_t structure to be considered
 *      a valid structure.
 */
unsigned long getUserDataSignature(void)
{
    return MODE_USER_DATA_SIGNATURE;
}

/*
 *  findModeParamFromTable
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
mode_parameter_t *findModeParamFromTable(
    unsigned long width, 
    unsigned long height, 
    unsigned long refresh_rate,
    unsigned short index,
    mode_parameter_t *pModeTable
)
{
    unsigned short modeIndex = 0;
#if 0
    unsigned short tempIndex = 0;
#endif
    /* Walk the entire mode table. */    
    while (pModeTable[modeIndex].pixel_clock != 0)
    {
        if (((width == (unsigned long)(-1)) || (pModeTable[modeIndex].horizontal_display_end == width)) &&
            ((height == (unsigned long)(-1)) || (pModeTable[modeIndex].vertical_display_end == height)) &&
            ((refresh_rate == (unsigned long)(-1)) || (pModeTable[modeIndex].vertical_frequency == refresh_rate)))
        {
#if 0
            if (tempIndex < index)
                tempIndex++;
            else
#endif
            {
                return (&pModeTable[modeIndex]);
            }
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
mode_parameter_t *findModeParam(
    disp_control_t dispCtrl,
    unsigned long width, 
    unsigned long height, 
    unsigned long refresh_rate,
    unsigned short index
)
{
    return findModeParamFromTable(width, height, refresh_rate, index, getStockModeParamTableEx(dispCtrl));
}

/*
 *  Use the
 *  Locate timing parameter for the requested mode from the default mode table.
 *  Success: return a pointer to the mode_parameter_t entry.
 *  Fail: a NULL pointer.
 */
mode_parameter_t *findVesaModeParam(
    unsigned long width, 
    unsigned long height, 
    unsigned long refresh_rate
)
{
	switch(ddk750_getChipType())
	{
		default: /* Normal SM750/SM718 */
    		return findModeParamFromTable(width, height, refresh_rate, 0, gDefaultModeParamTable);
	}
}

/*
 * (Obsolete) 
 * Return a point to the gDefaultModeParamTable.
 * Function in other files used this to get the mode table pointer.
 */
mode_parameter_t *getStockModeParamTable(void)
{
	switch(ddk750_getChipType())
	{
		default:
    		return (gDefaultModeParamTable);
	}	
}

/*
 * (Obsolete)
 * Return the size of the Stock Mode Param Table
 */
unsigned long getStockModeParamTableSize(void)
{

    return (sizeof(gDefaultModeParamTable) / sizeof(mode_parameter_t) - 1);

}

/* 
 *  getStockModeParamTableEx
 *      This function gets the mode parameters table associated to the
 *      display control (PRIMARY_CTRL or SECONDAR_CTRL).
 *
 *  Input:
 *      dispCtrl    - Display Control of the mode table that is associated to.
 *
 *  Output:
 *      Pointer to the mode table
 */
mode_parameter_t *getStockModeParamTableEx(
    disp_control_t dispCtrl
)
{
    mode_parameter_t *pModeTable;
    pModeTable = getStockModeParamTable();
#if 0 
    if (dispCtrl == PRIMARY_CTRL)
        pModeTable = (mode_parameter_t *)&gPrimaryModeParamTable[getCurrentDevice()];
    else
        pModeTable = (mode_parameter_t *)&gSecondaryModeParamTable[getCurrentDevice()];
        
    /* Check if the table exist by checking the first entry. 
       If it doesn't, then use the default mode table. */
    if (pModeTable->pixel_clock == 0)
	{
#if 1 /* Cheok_2013_0118 */
		pModeTable = getStockModeParamTable();
#else
		if (ddk750_getChipType() == SM750LE)
        	pModeTable = (mode_parameter_t *)&gSM750LEModeParamTable;
		else
        	pModeTable = (mode_parameter_t *)&gDefaultModeParamTable;
#endif
	}
#endif   
    return (pModeTable);
}

/*
 *  getStockModeParamTableSizeEx
 *      This function gets the size of the mode parameter table associated with
 *      specific display control
 *
 *  Input:
 *      dispCtrl    - Display control of the mode param table that is associated to.
 *
 *  Output:
 *      Size of the requeted mode param table.
 */
unsigned long getStockModeParamTableSizeEx(
    disp_control_t dispCtrl
)
{
    unsigned long tableSize;
    mode_parameter_t *pModeTable;
    
    /* Get the mode table */
    pModeTable = getStockModeParamTableEx(dispCtrl);
    
    /* Calculate the table size by finding the end of table entry indicated by all zeroes. */    
    tableSize = 0;
    while (pModeTable[tableSize].pixel_clock != 0)
        tableSize++;
        
    return tableSize;
}


/* 
 * This function returns the current mode.
 */
mode_parameter_t getCurrentModeParam(
    disp_control_t dispCtrl
)
{
    if (dispCtrl == PRIMARY_CTRL)
        return gPrimaryCurrentModeParam[getCurrentDevice()];
    else
        return gSecondaryCurrentModeParam[getCurrentDevice()];
}

/*
 *  Convert the timing into possible SM750 timing.
 *  If actual pixel clock is not equal to timing pixel clock.
 *  other parameter like horizontal total and sync have to be changed.
 *
 *  Input: Pointer to a mode parameters.
 *         Pointer to a an empty mode parameter structure to be filled.
 *         Actual pixel clock generated by SMI hardware.
 *
 *  Output:
 *      1) Fill up input structure mode_parameter_t with possible timing for SM750.
 */
long adjustModeParam(
mode_parameter_t *pModeParam,/* Pointer to mode parameter */
mode_parameter_t *pMode,     /* Pointer to mode parameter to be updated here */
unsigned long ulPClk         /* real pixel clock feasible by SM750 */
)
{
    unsigned long blank_width, sync_start, sync_width;

    /* Sanity check */
    if ( pModeParam == (mode_parameter_t *)0 ||
         pMode      == (mode_parameter_t *)0 ||
         ulPClk     == 0)
    {
        return -1;
    }

    /* Copy VESA mode into SM750 mode. */
    *pMode = *pModeParam;

    /* If it can generate the vesa required pixel clock, and there are a minimum of
       24 pixel difference between the horizontal sync start and the horizontal display
       end, then there is nothing to change */
    if ((ulPClk == pModeParam->pixel_clock) && 
        ((pModeParam->horizontal_sync_start - pModeParam->horizontal_display_end) > 24))
        return 0;

    pMode->pixel_clock = ulPClk; /* Update actual pixel clock into mode */

    /* Calculate the sync percentages of the VESA mode. */
    blank_width = pModeParam->horizontal_total - pModeParam->horizontal_display_end;
    sync_start = roundedDiv((pModeParam->horizontal_sync_start -
                       pModeParam->horizontal_display_end) * 100, blank_width);
    sync_width = roundedDiv(pModeParam->horizontal_sync_width * 100, blank_width);

     /* Calculate the horizontal total based on the actual pixel clock and VESA line frequency. */
    pMode->horizontal_total = roundedDiv(pMode->pixel_clock,
                                    pModeParam->horizontal_frequency);

    /* Calculate the sync start and width based on the VESA percentages. */
    blank_width = pMode->horizontal_total - pMode->horizontal_display_end;

    if (ddk750_getChipType() == SM750)
    {
        unsigned long sync_adjustment;
            
        /* There is minimum delay of 22 pixels between the horizontal display end 
           to the horizontal sync start. Therefore, the horizontal sync start value
           needs to be adjusted if the value falls below 22 pixels.
           The 22 pixels comes from the propagating delay from the CRT display to
           all the 11 display pipes inside SM750. The factor of 2 is caused by the
           double pixel support in SM750. The value used here is 24 to align to 
           8 bit character width.
         */
        sync_adjustment = roundedDiv(blank_width * sync_start, 100);
        if (sync_adjustment < 24)
            sync_adjustment = 24;
        pMode->horizontal_sync_start = pMode->horizontal_display_end + sync_adjustment;
    
        /* Check if the adjustment of the sync start will cause the sync width to go
           over the horizontal total. If it is, then reduce the width instead of
           changing the horizontal total.
         */
        /* Maximum value for sync width and back porch. */
        sync_adjustment = blank_width - sync_adjustment;
        pMode->horizontal_sync_width = roundedDiv(blank_width * sync_width, 100);
        if (sync_adjustment <= pMode->horizontal_sync_width)
            pMode->horizontal_sync_width = sync_adjustment/2;
    }
    else
    {
        /* SM718 does not have the above restriction. */
        pMode->horizontal_sync_start = pMode->horizontal_display_end + roundedDiv(blank_width * sync_start, 100);
        pMode->horizontal_sync_width = roundedDiv(blank_width * sync_width, 100);
    }
    
    /* Calculate the line and screen frequencies. */
    pMode->horizontal_frequency = roundedDiv(pMode->pixel_clock,
                                        pMode->horizontal_total);
    pMode->vertical_frequency = roundedDiv(pMode->horizontal_frequency,
                                      pMode->vertical_total);
    return 0;
}


/*
 *	This function gets the display status
 *
 *	Input:
 *		dispControl		- display control of which display status to be retrieved.
 *
 *  Output:
 *      0   - Display is pending
 *     -1   - Display is not pending
 */
long isCurrentDisplayPending(
    disp_control_t dispControl
)
{
    /* Get the display status */
    if (dispControl == PRIMARY_CTRL)
    {
        if (SMI_FIELD_GET(peekRegisterDWord(PRIMARY_FB_ADDRESS), PRIMARY_FB_ADDRESS, STATUS) == PRIMARY_FB_ADDRESS_STATUS_PENDING)
            return 0;
    }
	else if (dispControl == SECONDARY_CTRL)
    {
        if (SMI_FIELD_GET(peekRegisterDWord(SECONDARY_FB_ADDRESS), SECONDARY_FB_ADDRESS, STATUS) == SECONDARY_FB_ADDRESS_STATUS_PENDING)
            return 0;
    }

    return (-1);
}

/*
 *	This function sets the display base address
 *
 *	Input:
 *		dispControl		- display control of which base address to be set.
 *		ulBaseAddress	- Base Address value to be set.
 */
void setDisplayBaseAddress(
	disp_control_t dispControl,
	unsigned long ulBaseAddress
)
{
	if (dispControl == PRIMARY_CTRL)
	{
		/* Frame buffer base for this mode */
	    pokeRegisterDWord(PRIMARY_FB_ADDRESS,
              SMI_FIELD_SET(0, PRIMARY_FB_ADDRESS, STATUS, PENDING)
            | SMI_FIELD_SET(0, PRIMARY_FB_ADDRESS, EXT, LOCAL)
            | SMI_FIELD_VALUE(0, PRIMARY_FB_ADDRESS, ADDRESS, ulBaseAddress));
	}
	else if (dispControl == SECONDARY_CTRL)
	{
        /* Frame buffer base for this mode */
        pokeRegisterDWord(SECONDARY_FB_ADDRESS,
              SMI_FIELD_SET(0, SECONDARY_FB_ADDRESS, STATUS, PENDING)
            | SMI_FIELD_SET(0, SECONDARY_FB_ADDRESS, EXT, LOCAL)
            | SMI_FIELD_VALUE(0, SECONDARY_FB_ADDRESS, ADDRESS, ulBaseAddress));
	}
}


/* 
 * Program the hardware for a specific video mode
 */
void programModeRegisters(
mode_parameter_t *pModeParam,   /* mode information about pixel clock, horizontal total, etc. */
unsigned long ulBpp,            /* Color depth for this mode */
unsigned long ulBaseAddress,    /* Offset in frame buffer */
unsigned long ulPitch,          /* Mode pitch value in byte: no of bytes between two lines. */
pll_value_t *pPLL               /* Pre-calculated values for the PLL */
)
{
    unsigned long ulTmpValue, ulReg, ulReservedBits;
    unsigned long palette_ram;
    unsigned long offset;
	logical_chip_type_t chipType;

    /* Enable display power gate */
    ulTmpValue = peekRegisterDWord(CURRENT_GATE);
    ulTmpValue = SMI_FIELD_SET(ulTmpValue, CURRENT_GATE, DISPLAY, ON);
    setCurrentGate(ulTmpValue);

    if (pPLL->clockType == SECONDARY_PLL)
    {
   // printk("func[%s], secondary reg, ulPitch=[%d]\n", __func__, ulPitch);
        /* Secondary Display Control: SECONDARY_PLL */
        pokeRegisterDWord(SECONDARY_PLL_CTRL, formatPllReg(pPLL)); 

        /* Frame buffer base for this mode */
        //setDisplayBaseAddress(SECONDARY_CTRL, ulBaseAddress);//move by ilena

        /* Pitch value (Sometime, hardware people calls it Offset) */
       // pokeRegisterDWord(SECONDARY_FB_WIDTH,
         //     SMI_FIELD_VALUE(0, SECONDARY_FB_WIDTH, WIDTH, ulPitch)
           // | SMI_FIELD_VALUE(0, SECONDARY_FB_WIDTH, OFFSET, ulPitch));

        pokeRegisterDWord(SECONDARY_HORIZONTAL_TOTAL,
              SMI_FIELD_VALUE(0, SECONDARY_HORIZONTAL_TOTAL, TOTAL, pModeParam->horizontal_total - 1)
            | SMI_FIELD_VALUE(0, SECONDARY_HORIZONTAL_TOTAL, DISPLAY_END, pModeParam->horizontal_display_end - 1));

        pokeRegisterDWord(SECONDARY_HORIZONTAL_SYNC,
              SMI_FIELD_VALUE(0, SECONDARY_HORIZONTAL_SYNC, WIDTH, pModeParam->horizontal_sync_width)
            | SMI_FIELD_VALUE(0, SECONDARY_HORIZONTAL_SYNC, START, pModeParam->horizontal_sync_start - 1));

        pokeRegisterDWord(SECONDARY_VERTICAL_TOTAL,
              SMI_FIELD_VALUE(0, SECONDARY_VERTICAL_TOTAL, TOTAL, pModeParam->vertical_total - 1)
            | SMI_FIELD_VALUE(0, SECONDARY_VERTICAL_TOTAL, DISPLAY_END, pModeParam->vertical_display_end - 1));

        pokeRegisterDWord(SECONDARY_VERTICAL_SYNC,
              SMI_FIELD_VALUE(0, SECONDARY_VERTICAL_SYNC, HEIGHT, pModeParam->vertical_sync_height)
            | SMI_FIELD_VALUE(0, SECONDARY_VERTICAL_SYNC, START, pModeParam->vertical_sync_start - 1));

        /* Set control register value */
        ulTmpValue =        
            (pModeParam->vertical_sync_polarity == POS
            ? SMI_FIELD_SET(0, SECONDARY_DISPLAY_CTRL, VSYNC_PHASE, ACTIVE_HIGH)
            : SMI_FIELD_SET(0, SECONDARY_DISPLAY_CTRL, VSYNC_PHASE, ACTIVE_LOW))
          | (pModeParam->horizontal_sync_polarity == POS
            ? SMI_FIELD_SET(0, SECONDARY_DISPLAY_CTRL, HSYNC_PHASE, ACTIVE_HIGH)
            : SMI_FIELD_SET(0, SECONDARY_DISPLAY_CTRL, HSYNC_PHASE, ACTIVE_LOW))
          | SMI_FIELD_SET(0, SECONDARY_DISPLAY_CTRL, SELECT, SECONDARY)
          | SMI_FIELD_SET(0, SECONDARY_DISPLAY_CTRL, TIMING, ENABLE)
          | SMI_FIELD_SET(0, SECONDARY_DISPLAY_CTRL, PLANE, ENABLE) 
          | (ulBpp == 8
            ? SMI_FIELD_SET(0, SECONDARY_DISPLAY_CTRL, FORMAT, 8)
            : (ulBpp == 16
            ? SMI_FIELD_SET(0, SECONDARY_DISPLAY_CTRL, FORMAT, 16)
            : SMI_FIELD_SET(0, SECONDARY_DISPLAY_CTRL, FORMAT, 32)));

		chipType = ddk750_getChipType();

		if (chipType == SM750 || chipType == SM718)
        {
            /* TODO: Check if the auto expansion bit can be cleared here */
            ulReg = peekRegisterDWord(SECONDARY_DISPLAY_CTRL)
              & SMI_FIELD_CLEAR(SECONDARY_DISPLAY_CTRL, VSYNC_PHASE)
              & SMI_FIELD_CLEAR(SECONDARY_DISPLAY_CTRL, HSYNC_PHASE)
              & SMI_FIELD_CLEAR(SECONDARY_DISPLAY_CTRL, SELECT)
              & SMI_FIELD_CLEAR(SECONDARY_DISPLAY_CTRL, TIMING)
              & SMI_FIELD_CLEAR(SECONDARY_DISPLAY_CTRL, PLANE)
              & SMI_FIELD_CLEAR(SECONDARY_DISPLAY_CTRL, FORMAT)
              & SMI_FIELD_CLEAR(SECONDARY_DISPLAY_CTRL, LOCK_TIMING)
              & SMI_FIELD_CLEAR(SECONDARY_DISPLAY_CTRL, EXPANSION);

            pokeRegisterDWord(SECONDARY_DISPLAY_CTRL, ulTmpValue | ulReg);
        }
		

        /* Palette RAM. */
        palette_ram = SECONDARY_PALETTE_RAM;
        
        /* Save the current mode param */
        gSecondaryCurrentModeParam[getCurrentDevice()] = *pModeParam;
    }
    else 
    {
        /* Primary display control clock: PRIMARY_PLL */
        pokeRegisterDWord(PRIMARY_PLL_CTRL, formatPllReg(pPLL));

        /* Program primary PLL, if applicable */
        if (pPLL->clockType == PRIMARY_PLL)
        {
            pokeRegisterDWord(PRIMARY_PLL_CTRL, formatPllReg(pPLL));

            /* Program to Non-VGA mode when using primary PLL */
            pokeRegisterDWord(VGA_CONFIGURATION, 
                SMI_FIELD_SET(peekRegisterDWord(VGA_CONFIGURATION), VGA_CONFIGURATION, PLL, PRIMARY));
        }
        
        /* Frame buffer base for this mode */
		//setDisplayBaseAddress(PRIMARY_CTRL, ulBaseAddress);// move by iena
		//printk("func[%s], primary reg, ulPitch=[%d]\n", __func__, ulPitch);
        /* Pitch value (Sometime, hardware people calls it Offset) */
        //pokeRegisterDWord(PRIMARY_FB_WIDTH,
          //    SMI_FIELD_VALUE(0, PRIMARY_FB_WIDTH, WIDTH, ulPitch)
            //| SMI_FIELD_VALUE(0, PRIMARY_FB_WIDTH, OFFSET, ulPitch));

        pokeRegisterDWord(PRIMARY_WINDOW_WIDTH,
              SMI_FIELD_VALUE(0, PRIMARY_WINDOW_WIDTH, WIDTH, pModeParam->horizontal_display_end - 1)
            | SMI_FIELD_VALUE(0, PRIMARY_WINDOW_WIDTH, X, 0));

        pokeRegisterDWord(PRIMARY_WINDOW_HEIGHT,
              SMI_FIELD_VALUE(0, PRIMARY_WINDOW_HEIGHT, HEIGHT, pModeParam->vertical_display_end - 1)
            | SMI_FIELD_VALUE(0, PRIMARY_WINDOW_HEIGHT, Y, 0));

        pokeRegisterDWord(PRIMARY_PLANE_TL,
              SMI_FIELD_VALUE(0, PRIMARY_PLANE_TL, TOP, 0)
            | SMI_FIELD_VALUE(0, PRIMARY_PLANE_TL, LEFT, 0));

        pokeRegisterDWord(PRIMARY_PLANE_BR, 
              SMI_FIELD_VALUE(0, PRIMARY_PLANE_BR, BOTTOM, pModeParam->vertical_display_end - 1)
            | SMI_FIELD_VALUE(0, PRIMARY_PLANE_BR, RIGHT, pModeParam->horizontal_display_end - 1));

        pokeRegisterDWord(PRIMARY_HORIZONTAL_TOTAL,
              SMI_FIELD_VALUE(0, PRIMARY_HORIZONTAL_TOTAL, TOTAL, pModeParam->horizontal_total - 1)
            | SMI_FIELD_VALUE(0, PRIMARY_HORIZONTAL_TOTAL, DISPLAY_END, pModeParam->horizontal_display_end - 1));

        pokeRegisterDWord(PRIMARY_HORIZONTAL_SYNC,
              SMI_FIELD_VALUE(0, PRIMARY_HORIZONTAL_SYNC, WIDTH, pModeParam->horizontal_sync_width)
            | SMI_FIELD_VALUE(0, PRIMARY_HORIZONTAL_SYNC, START, pModeParam->horizontal_sync_start - 1));

        pokeRegisterDWord(PRIMARY_VERTICAL_TOTAL,
              SMI_FIELD_VALUE(0, PRIMARY_VERTICAL_TOTAL, TOTAL, pModeParam->vertical_total - 1)
            | SMI_FIELD_VALUE(0, PRIMARY_VERTICAL_TOTAL, DISPLAY_END, pModeParam->vertical_display_end - 1));

        pokeRegisterDWord(PRIMARY_VERTICAL_SYNC,
              SMI_FIELD_VALUE(0, PRIMARY_VERTICAL_SYNC, HEIGHT, pModeParam->vertical_sync_height)
            | SMI_FIELD_VALUE(0, PRIMARY_VERTICAL_SYNC, START, pModeParam->vertical_sync_start - 1));

        /* Set control register value */
        ulTmpValue =
            (pModeParam->clock_phase_polarity == POS
            ? SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, CLOCK_PHASE, ACTIVE_HIGH)
            : SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, CLOCK_PHASE, ACTIVE_LOW))
          | (pModeParam->vertical_sync_polarity == POS
            ? SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, VSYNC_PHASE, ACTIVE_HIGH)
            : SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, VSYNC_PHASE, ACTIVE_LOW))
          | (pModeParam->horizontal_sync_polarity == POS
            ? SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, HSYNC_PHASE, ACTIVE_HIGH)
            : SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, HSYNC_PHASE, ACTIVE_LOW))
          | SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, TIMING, ENABLE)
          | SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, PLANE, ENABLE)
          | (ulBpp == 8
            ? SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, FORMAT, 8)
            : (ulBpp == 16
            ? SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, FORMAT, 16)
            : SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, FORMAT, 32)));

        /* Added some masks to mask out the reserved bits. 
         * Sometimes, the reserved bits are set/reset randomly when 
         * writing to the PRIMARY_DISPLAY_CTRL, therefore, the register
         * reserved bits are needed to be masked out.
         */
        ulReservedBits = SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, RESERVED_1_MASK, ENABLE) |
                         SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, RESERVED_2_MASK, ENABLE) |
                         SMI_FIELD_SET(0, PRIMARY_DISPLAY_CTRL, RESERVED_3_MASK, ENABLE);

        ulReg = (peekRegisterDWord(PRIMARY_DISPLAY_CTRL) & ~ulReservedBits)
              & SMI_FIELD_CLEAR(PRIMARY_DISPLAY_CTRL, CLOCK_PHASE)
              & SMI_FIELD_CLEAR(PRIMARY_DISPLAY_CTRL, VSYNC_PHASE)
              & SMI_FIELD_CLEAR(PRIMARY_DISPLAY_CTRL, HSYNC_PHASE)
              & SMI_FIELD_CLEAR(PRIMARY_DISPLAY_CTRL, TIMING)
              & SMI_FIELD_CLEAR(PRIMARY_DISPLAY_CTRL, VERTICAL_PAN)
              & SMI_FIELD_CLEAR(PRIMARY_DISPLAY_CTRL, HORIZONTAL_PAN)
              & SMI_FIELD_CLEAR(PRIMARY_DISPLAY_CTRL, PLANE)
              & SMI_FIELD_CLEAR(PRIMARY_DISPLAY_CTRL, FORMAT);

        pokeRegisterDWord(PRIMARY_DISPLAY_CTRL, ulTmpValue | ulReg);

        /* 
         * PRIMARY_DISPLAY_CTRL register seems requiring few writes
         * before a value can be succesfully written in.
         * Added some masks to mask out the reserved bits.
         * Note: This problem happens by design. The hardware will wait for the
         *       next vertical sync to turn on/off the plane.
         */
        while((peekRegisterDWord(PRIMARY_DISPLAY_CTRL) & ~ulReservedBits) != (ulTmpValue|ulReg))
        {
            pokeRegisterDWord(PRIMARY_DISPLAY_CTRL, ulTmpValue | ulReg);
        }

        /* Palette RAM */
        palette_ram = PRIMARY_PALETTE_RAM;
        
        /* Save the current mode param */
        gPrimaryCurrentModeParam[getCurrentDevice()] = *pModeParam;
    }

    /* In case of 8-bpp, fill palette */
    if (ulBpp==8)
    {
        /* Start with RGB = 0,0,0. */
        unsigned char red = 0, green = 0, blue = 0;
        unsigned long gray = 0;
        for (offset = 0; offset < 256 * 4; offset += 4)
        {
            /* Store current RGB value. */
            pokeRegisterDWord(palette_ram + offset, gray
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
            pokeRegisterDWord(palette_ram + offset, ulTmpValue);

            /* Advance RGB by 1,1,1. */
            ulTmpValue += 0x010101;
        }
    }
}

/* 
 * This function gets the available clock type
 *
 */
clock_type_t getClockType(disp_control_t dispCtrl)
{
    clock_type_t clockType;

    switch (dispCtrl)
    {
        case PRIMARY_CTRL:
            clockType = PRIMARY_PLL;
            break;
        default:
        case SECONDARY_CTRL:
            clockType = SECONDARY_PLL;
            break;
    }
    return clockType;
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
long setCustomMode(
	logicalMode_t *pLogicalMode, 
	mode_parameter_t *pUserModeParam
)
{
    mode_parameter_t pModeParam; /* physical parameters for the mode */
    pll_value_t pll;
    unsigned long ulActualPixelClk, ulTemp;

    /*
     * Minimum check on mode base address.
     * At least it shouldn't be bigger than the size of frame buffer.
     */
    if (ddk750_getFrameBufSize() <= pLogicalMode->baseAddress)
    	{printk("in func [%s], line[%d], return error\n", __func__, __LINE__);
        return -1;
    	}

    /*
     * Set up PLL, a structure to hold the value to be set in clocks.
     */
    pll.inputFreq = DEFAULT_INPUT_CLOCK; /* Defined in CLOCK.H */

    /* Get the Clock Type */
    pll.clockType = getClockType(pLogicalMode->dispCtrl);

    /* 
     * Call calcPllValue() to fill up the other fields for PLL structure.
     * Sometime, the chip cannot set up the exact clock required by User.
     * Return value from calcPllValue() gives the actual possible pixel clock.
     */
    ulActualPixelClk = calcPllValue(pUserModeParam->pixel_clock, &pll);
    //DDKDEBUGPRINT((DISPLAY_LEVEL, "Actual Pixel Clock: %d\n", ulActualPixelClk));

    /* 
     * Adjust Vesa mode parameter to feasible mode parameter for SMI hardware.
     */
    if (adjustModeParam(pUserModeParam, &pModeParam, ulActualPixelClk) != 0 )
    {
    //printk("in func [%s], line[%d], return error\n", __func__, __LINE__);
        return -1;
    }

    /* If calling function don't have a preferred pitch value, 
       work out a 16 byte aligned pitch value.
    */	
    //printk("func[%s], ulPitch=[%d]\n", __func__, pLogicalMode->pitch);
    if (pLogicalMode->pitch <= 0)
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

        ulTemp = (pLogicalMode->x + 15) & ~15; /* This calculation has no effect on 640, 800, 1024 and 1280. */
        pLogicalMode->pitch = ulTemp * (pLogicalMode->bpp / 8);
    }
	//printk("func[%s], ulPitch=[%d]\n", __func__, pLogicalMode->pitch);

    /* Program the hardware to set up the mode. */
    programModeRegisters( 
        &pModeParam,
        pLogicalMode->bpp, 
        pLogicalMode->baseAddress, 
        pLogicalMode->pitch, 
        &pll);
        
    return (0);
}

/*
 * Input:
 *     1) pLogicalMode contains information such as x, y resolution, bpp, xLCD, and yLCD.
 *     2) A user defined parameter table for the mode.
 *
 * This function calculate and programs the hardware to set up the
 * requested mode and scale the mode if necessary.
 *
 * This function allows the use of user defined parameter table if
 * predefined parameter table does not fit.
 *
 * Return: 0 (or NO_ERROR) if mode can be set successfully.
 *         -1 if any set mode error.
 */
long setCustomModeEx(
	logicalMode_t *pLogicalMode, 
	mode_parameter_t *pUserModeParam
)
{
    long returnValue = 0;

#if 0   /* userData field is used on DDK version 1.1 and above. Thereofre, this checking is
           not needed anymore. */
    /* For the current DDK version, the userData needs to be set to 0. If not, then return error. */
    if (pLogicalMode->userData != (void *)0)
        return (-1);
#endif

    /* Return error when the mode is bigger than the panel size. Might be temporary solution
       depending whether we will support panning or not. */
    if (((pLogicalMode->xLCD != 0) && 
         (pLogicalMode->yLCD != 0)) &&
        ((pLogicalMode->xLCD < pLogicalMode->x) ||
         (pLogicalMode->yLCD < pLogicalMode->y)))
    	{
    	//printk("in func [%s], line[%d], return error\n", __func__, __LINE__);
        return (-1);
    	}
    
    /* Return error when the panel size exceed the maximum mode that we can support. */
    if ((pLogicalMode->xLCD > MAX_PANEL_SIZE_WIDTH) ||
        (pLogicalMode->yLCD > MAX_PANEL_SIZE_HEIGHT))
    	{
    	//printk("in func [%s], line[%d], return error\n", __func__, __LINE__);
        return (-1);    /* Unsupported panel size. */
    	}
    
    /* Set the mode first */
    returnValue = setCustomMode(pLogicalMode, pUserModeParam);

    
    return (returnValue);
}

/*
 * Input pLogicalMode contains information such as x, y resolution, bpp, 
 * xLCD and yLCD. The main difference between setMode and setModeEx are
 * these two parameters (xLCD and yLCD). Use this setModeEx API to set
 * expansion while setMode API for regular setmode without any expansion.
 * Refer to MODE.h for the details.
 *
 * This function calculate and programs the hardware to set up the
 * requested mode.
 *
 * Return: 0 (or NO_ERROR) if mode can be set successfully.
 *         -1 if any set mode error.
 */
long setModeEx(
	logicalMode_t *pLogicalMode
)
{
    mode_parameter_t *pModeParam;       /* physical parameters for the mode */
    unsigned short index = 0;
    unsigned long modeWidth, modeHeight;
    userData_t *pUserData;
    
    /* Conditions to set the mode when scaling is needed (xLCD and yLCD is not zeroes)
     *      1. PRIMARY_CTRL
     *          a. Set the primary display control timing to the actual display mode.
     *          b. Set the secondary display control timing to the mode that equals to
     *             the panel size.
     *      2. SECONDARY_CTRL
     *          a. Set the secondary display control timing to the mode that equals to
     *             the panel size.
     */
    if ((pLogicalMode->dispCtrl == SECONDARY_CTRL) &&
        (pLogicalMode->xLCD != 0) && (pLogicalMode->yLCD != 0))
    {
        modeWidth = pLogicalMode->xLCD;
        modeHeight = pLogicalMode->yLCD;
    }
    else
    {
        modeWidth = pLogicalMode->x;
        modeHeight = pLogicalMode->y;
    }
    
    /*
     * Check the validity of the userData pointer and translate the information as necessary
     */
    pUserData = (userData_t *)pLogicalMode->userData;
    if ((pUserData != (userData_t *)0) &&
        (pUserData->signature == getUserDataSignature()) &&
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
  //   printk("fine ctrl=[%d], width[%d](x=[%d]), height[%d], hz[%d], index[%d], disp=[%d]\n", pLogicalMode->dispCtrl, modeWidth,pLogicalMode->x,modeHeight, pLogicalMode->hz, index,pLogicalMode->dispCtrl);
    pModeParam = findModeParam(pLogicalMode->dispCtrl, modeWidth, modeHeight, pLogicalMode->hz, index);
    if (pModeParam == (mode_parameter_t *)0)
    	{
    	//printk("in func [%s], line[%d], return error\n", __func__, __LINE__);
        return -1;
    	}

    return(setCustomModeEx(pLogicalMode, pModeParam));
}

/*
 * Input pLogicalMode contains information such as x, y resolution and bpp.
 * Refer to MODE.h for the details.
 *
 * This function calculate and programs the hardware to set up the
 * requested mode.
 *
 * Return: 0 (or NO_ERROR) if mode can be set successfully.
 *         -1 if any set mode error.
 */
long setMode(
	logicalMode_t *pLogicalMode
)
{
    /* Initialize the panel size to 0 */
    pLogicalMode->xLCD = 0;
    pLogicalMode->yLCD = 0;
    pLogicalMode->userData = (void *)0;

    /* Call the setModeEx to set the mode. */
    return setModeEx(pLogicalMode);
}

/*
 *  setInterpolation
 *      This function enables/disables the horizontal and vertical interpolation
 *      for the secondary display control. Primary display control does not have
 *      this capability.
 *
 *  Input:
 *      enableHorzInterpolation - Flag to enable/disable Horizontal interpolation
 *      enableVertInterpolation - Flag to enable/disable Vertical interpolation
 */
void setInterpolation(
    unsigned long enableHorzInterpolation,
    unsigned long enableVertInterpolation
)
{
    unsigned long value;

    value = peekRegisterDWord(SECONDARY_DISPLAY_CTRL);
    
    if (enableHorzInterpolation)
        value = SMI_FIELD_SET(value, SECONDARY_DISPLAY_CTRL, HORIZONTAL_MODE, INTERPOLATE);
    else
        value = SMI_FIELD_SET(value, SECONDARY_DISPLAY_CTRL, HORIZONTAL_MODE, REPLICATE);
        
    if (enableVertInterpolation)
        value = SMI_FIELD_SET(value, SECONDARY_DISPLAY_CTRL, VERTICAL_MODE, INTERPOLATE);
    else
        value = SMI_FIELD_SET(value, SECONDARY_DISPLAY_CTRL, VERTICAL_MODE, REPLICATE);
        
    pokeRegisterDWord(SECONDARY_DISPLAY_CTRL, value);
}
