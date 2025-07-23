/*******************************************************************
* 
*         Copyright (c) 2007 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  MODE.H --- SMI DDK 
*  This file contains the definitions for the mode tables.
* 
*******************************************************************/
#ifndef _DDK770_MODE_H_
#define _DDK770_MODE_H_

#include "../hw_com.h"

/* Set the alignment to 1-bit aligned. Otherwise, the memory compare used in the
   modeext to compare timing will not work. */
#pragma pack(1)


typedef unsigned long long  u64;
typedef unsigned int    	u32;
typedef unsigned short  	u16;
typedef unsigned char  		u8;

typedef struct _cea_parameter_t
{
    u32 vic;
    u32 h_active;
    u32 h_blank;
    u32 hsync_delay;
    u32 hsync_width;
    u32 v_active;
    u32 v_blank;
    u32 vsync_delay;
    u32 vsync_width;
    u32 freq;
    u8 de_polarity;
    u8 hsync_polarity;
    u8 vsync_polarity;
    u8 vblank_osc;
    u8 progressive_nI;
    u32 hdmi_mode;
    u32 aspect_ratio;
    u32 tmds_clk;
}
cea_parameter_t;

typedef enum _mode_menu_index_t
{
    CEA_MODE_1920x1080p_60 = 0,
    CEA_MODE_1920x1080i_60,
    CEA_MODE_1280x720p_60,
    CEA_MODE_720x480p_60,
    CEA_MODE_800x600p_60,
    CEA_MODE_1024x768p_60,
    CEA_MODE_3840x2160p_30,
    CEA_MODE_3840x2160p_60,
    CEA_MODE_3840x2160p_50,
    CEA_MODE_2560x1440p_60,
    CEA_MODE_640x480p_60,
    CEA_MODE_640x480p_59,
    CEA_MODE_1440x480p_60,
    CEA_MODE_1440x240p_60,
    CEA_MODE_2880x480p_59,
    CEA_MODE_2880x480p_60,
    CEA_MODE_2880x240p_60,
    CEA_MODE_720x576p_50,
    CEA_MODE_1280x720p_50,
    CEA_MODE_1920x1080p_50,
    CEA_MODE_1440x576p_50,
    CEA_MODE_1440x288p_50,
    CEA_MODE_2880x576p_50,
    CEA_MODE_2880x288p_50,
    CEA_MODE_2880x576p_60,
    CEA_MODE_1920x1080p_24,
    CEA_MODE_1920x1080p_25,
    CEA_MODE_1920x1080p_30,
    CEA_MODE_1920x1080p_100,
    CEA_MODE_1280x720p_100,
    CEA_MODE_720x576p_100,
    CEA_MODE_1440x576p_100,
    CEA_MODE_1920x1080p_0,
    CEA_MODE_1920x1080p_120,
    CEA_MODE_1280x720p_120,
    CEA_MODE_720x480p_120,
    CEA_MODE_1440x480p_120,
    CEA_MODE_720x576p_200,
    CEA_MODE_1440x576p_200,
    CEA_MODE_720x480p_240,
    CEA_MODE_1440x480p_240,
    CEA_MODE_1280x720p_24,
    CEA_MODE_1280x720p_25,
    CEA_MODE_1280x720p_30,
    CEA_MODE_1440x480i_60,
    CEA_MODE_2880x480i_59,
    CEA_MODE_2880x480i_60,
    CEA_MODE_1920x1080i_50,
    CEA_MODE_1440x576i_50,
    CEA_MODE_2880x576i_50,
    CEA_MODE_1920x1080i_100,
    CEA_MODE_1920x1080i_120,
    CEA_MODE_1440x576i_100,
    CEA_MODE_1440x480i_120,
    CEA_MODE_720x480i_0,
    CEA_MODE_1440x576i_200,
    CEA_MODE_1440x480i_240,
    CEA_MODE_3840x2160p_25, 
    CEA_MODE_3840x2160p_24, 
    CEA_MODE_4096x2160p_24, 
    CEA_MODE_4096x2160p_60,
    CEA_MODE_2560x1440p_144
}
mode_menu_index_t, *pmode_menu_index_t;

/*
    u32 vic;
    u32 h_active;
    u32 h_blank;
    u32 hsync_delay;
    u32 hsync_width;
    u32 v_active;
    u32 v_blank;
    u32 vsync_delay;
    u32 vsync_width;
    u32 freq;
    u8 de_polarity;
    u8 hsync_polarity;
    u8 vsync_polarity;
    u8 vblank_osc;
    u8 progressive_nI;
    u32 hdmi_mode;
    u32 aspect_ratio;
    double tmds_clk;

*/


/* Restore alignment */
#pragma pack()

long Get_CEA_Mode(logicalMode_t *pLogicalMode, cea_parameter_t *cea_mode, mode_parameter_t *pModeParam, unsigned int isDP);



/*
 *  ddk770_getUserDataSignature
 *      This function gets the user data mode signature
 *
 *  Output:
 *      The signature to be filled in the user_data_mode_t structure to be considered
 *      a valid structure.
 */
unsigned long ddk770_getUserDataSignature(void);

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
);

/*
 *  getDuplicateModeIndex
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
unsigned short getDuplicateModeIndex(
    disp_control_t dispCtrl,
    mode_parameter_t *pModeParam
);

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
);

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
);

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
);

/*
 * Return a point to the gDefaultModeParamTable.
 * Function in other files used this to get the mode table pointer.
 */
mode_parameter_t *ddk770_getStockModeParamTable(void);

/*
 * Return the size of the Stock Mode Param Table
 */
unsigned long ddk770_getStockModeParamTableSize(void);

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
);

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
);

/*
 *  ddk770_getMaximumModeEntries
 *      This function gets the maximum entries that can be stored in the mode table.
 *
 *  Output:
 *      Total number of maximum entries
 */
unsigned long ddk770_getMaximumModeEntries(void);

/* 
 * This function returns the current mode.
 */
mode_parameter_t ddk770_getCurrentModeParam(
    disp_control_t dispCtrl
);

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
);

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
);

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
);

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
);

/*
 * Input pLogicalMode contains information such as x, y resolution and bpp 
 * The main difference between ddk770_setMode and setModeEx userData.
 *
 * Return: 0 (or NO_ERROR) if mode can be set successfully.
 *         -1 if any set mode error.
 */
long ddk770_setModeEx(
    logicalMode_t *pLogicalMode
);

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
);

unsigned short alignLineOffset(unsigned short lineOffset);


#endif /* _MODE_H_ */
