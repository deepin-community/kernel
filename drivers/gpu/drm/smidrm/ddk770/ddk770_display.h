/*******************************************************************
* 
*  This code is free software and distributed as is wihtout any
*  warranty.
* 
*  display.h --- SM768 DDK
*  This file contains the function prototypes for the display.
* 
*******************************************************************/
#ifndef _DDK770_DISPLAY_H_
#define _DDK770_DISPLAY_H_

#include "ddk770_mode.h"
#include "../hw_com.h"

typedef enum _disp_format_t
{
    SINGLE_PIXEL_24BIT = 0,
    DOUBLE_PIXEL_48BIT
}
disp_format_t;

typedef enum _disp_output_t
{
    HDMI0=0,
    HDMI1,
    HDMI2,
    DP0,
    DP1,
    LVDS0,
    LVDS1,
    DEFAULT_DC,
}
disp_output_t;


/*
 * This function initializes the display.
 *
 * Output:
 *      0   - Success
 *      1   - Fail 
 */
long ddk770_initDisplay(void);

/*
 * This function sets the display DPMS state 
 * It is used to set CRT monitor to On, off, or suspend states, 
 * while display channel are still active.
 */
void ddk770_setDisplayDPMS(
   disp_control_t dispControl, /* Channel 0 or Channel 1) */
   DISP_DPMS_t state /* DPMS state */
   );

/*
 * This functions uses software sequence to turn on/off the panel.
 */
void ddk770_swPanelPowerSequence(
    disp_control_t dispControl, 
    disp_state_t dispState, 
    disp_control_t dataPath,
    unsigned long vSyncDelay);

/* 
 * This function turns on/off the DAC for CRT display control.
 * Input: On or off
 */
void enableDAC(
    disp_control_t dispControl, /* Channel 0 or Channel 1) */
    disp_state_t state);

/*
 * Wait number of Vertical Vsync
 *
 * Input:
 *          dispControl - Display Control (either channel 0 or 1)
 *          vSyncCount  - Number of vertical sync to wait.
 *
 * Note:
 *      This function is waiting for the next vertical sync.         
 */
void ddk770_waitDispVerticalSync(disp_control_t dispControl, unsigned long vSyncCount);

/*
 * Use panel vertical sync line as time delay function.
 * This function does not wait for the next VSync. Instead, it will wait
 * until the current line reaches the Vertical Sync line.
 * This function is really useful when flipping display to prevent tearing.
 *
 * Input: display control (CHANNEL0_CTRL or CHANNEL1_CTRL)
 */
void ddk770_waitVSyncLine(disp_control_t dispControl);

/*
 * Get current display line number
 */
unsigned long ddk770_getDisplayLine(disp_control_t dispControl);




/*
 * This function turns on/off the display control of Channel 0 or channel 1.
 *
 * Note:
 *      Turning on/off the timing and the plane requires programming sequence.
 *      The plane can not be changed without turning on the timing. However,
 *      changing the plane has no effect when the timing (clock) is off. Below,
 *      is the description of the timing and plane combination setting.
 */
void ddk770_setDisplayEnable(
disp_control_t dispControl, /* Channel 0 or Channel 1) */
disp_state_t dispState /* ON or OFF */
);


/*
 * This function controls monitor on/off and data path.
 * It can be used to set up any veiws: single view, clone view, dual view, output with channel swap, etc.
 * However, it needs too many input parameter.
 * There are other set view functions with less parameters, but not as flexible as this one.
 *
 */
long ddk770_setDisplayView(
    disp_control_t dispOutput,             /* Monitor 0 or 1 */
    disp_state_t dispState,                /* On or off */
    disp_control_t dataPath);            /* 24 or 48 bit digital interface (optional when OFF */

/*
 * Convenient function to trun on single view 
 */
long ddk770_setSingleViewOn(disp_control_t dispOutput);

/*
 * Convenient function to trun off single view 
 */
long ddk770_setSingleViewOff(disp_control_t dispOutput);

/*
 * Convenient function to trun on clone view 
 */
long ddk770_setCloneViewOn(disp_control_t dataPath);

/*
 * Convenient function to trun on dual view 
 */
long ddk770_setDualViewOn(void);

/*
 * Convenient function to trun off all views
 */
long ddk770_setAllViewOff(void);

void ddk770_DisableDoublePixel(disp_control_t dispControl);

void ddk770_EnableDoublePixel(disp_control_t dispControl);


void ddk770_SetLVDSHalfPixel(unsigned int enable);

void ddk770_setupLVDS(unsigned short lvds);

void ddk770_setupSingleLVDS(unsigned short lvds_ch,disp_control_t dataPath);

long ddk770_set48bitLVDS(disp_control_t dataPath);

//New for SM770

void clearDCOutput(void);

disp_control_t getOutputDC(disp_output_t dcOutput);
void setDCMUX(disp_output_t dcOutput,disp_control_t dc);
void ClearDCMUX(disp_output_t dcOutput);
unsigned char GetDCMUX(disp_output_t dcOutput);

void cloneDCOutput(disp_control_t dc);



/*
 * This function is to enable/disable display channel timing.
 *
 */
void ddk770_EnableChannelTiming(
   disp_control_t dispControl, /* Channel 0 or Channel 1) */
   unsigned short enable
);

unsigned char ddk770_isTimingEnable(disp_control_t dispControl);
#endif /* _DISPLAY_H_ */
