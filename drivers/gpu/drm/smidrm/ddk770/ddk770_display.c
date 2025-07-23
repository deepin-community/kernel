/*******************************************************************
* 
*  This code is free software and distributed as is wihtout any
*  warranty.
* 
*  DISPLAY.C --- SM768 GX SDK 
*  This file contains the source code for the panel and CRT functions.
* 
*******************************************************************/
#include <linux/kernel.h>
#include <linux/delay.h>
#include "ddk770_reg.h"
#include "ddk770_chip.h"
#include "ddk770_power.h"
#include "ddk770_display.h"
#include "ddk770_timer.h"
#include "ddk770_ddkdebug.h"
#include "ddk770_help.h"
#include "ddk770_hdmi.h"
#include "ddk770_dp.h"

/* Monitor Detection RGB Default Threshold values */
#define DEFAULT_MON_DETECTION_THRESHOLD         0x64

/*
 * This function initializes the display.
 *
 * Output:
 *      0   - Success
 *      1   - Fail 
 */
long ddk770_initDisplay()
{

	ddk770_HDMI_Init(0);
	ddk770_HDMI_Init(1);
	ddk770_HDMI_Init(2);

	DP_Init(0);
	DP_Init(1);
	
    return 0;
}

/* New for Falcon: DPMS control is moved to display controller.
 * This function sets the display DPMS state 
 * It is used to set CRT monitor to On, off, or suspend states, 
 * while display channel are still active.
 */
void ddk770_setDisplayDPMS(
   disp_control_t dispControl, /* Channel 0 or Channel 1) */
   DISP_DPMS_t state /* DPMS state */
   )
{
    unsigned long ulDispCtrlAddr;
    unsigned long ulDispCtrlReg;

    /* Get the control register for channel 0 or 1. */
    
    ulDispCtrlAddr = DISPLAY_CTRL+ (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);
    ulDispCtrlReg = peekRegisterDWord(ulDispCtrlAddr);

    switch (state)
    {
       case DISP_DPMS_ON:
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, DPMS, VPHP);
        break;

       case DISP_DPMS_STANDBY:
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, DPMS, VPHN);
        break;

       case DISP_DPMS_SUSPEND:
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, DPMS, VNHP);
        break;

       case DISP_DPMS_OFF:
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, DPMS, VNHN);
        break;
    }

    pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
}

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
void ddk770_waitDispVerticalSync(disp_control_t dispControl, unsigned long vSyncCount)
{
    unsigned long ulDispCtrlAddr;
    unsigned long status;
    unsigned long ulLoopCount = 0;
    static unsigned long ulDeadLoopCount = 10;
    
    ulDispCtrlAddr = DISPLAY_CTRL + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);

    if (dispControl == CHANNEL0_CTRL)
    {
        // There is no Vsync when PLL is off
        if ((FIELD_VAL_GET(peekRegisterDWord(CLOCK_ENABLE), CLOCK_ENABLE, DC0) == CLOCK_ENABLE_DC0_OFF))
            return;
    }
    else if(dispControl == CHANNEL1_CTRL)
    {
        // There is no Vsync when PLL is off
        if ((FIELD_VAL_GET(peekRegisterDWord(CLOCK_ENABLE), CLOCK_ENABLE, DC1) == CLOCK_ENABLE_DC1_OFF))
            return;
    }
    else
    {
        // There is no Vsync when PLL is off
        if ((FIELD_VAL_GET(peekRegisterDWord(CLOCK_ENABLE), CLOCK_ENABLE, DC2) == CLOCK_ENABLE_DC2_OFF))
            return;
    
    }

    //There is no Vsync when display timing is off. 
    if ((FIELD_VAL_GET(peekRegisterDWord(ulDispCtrlAddr), DISPLAY_CTRL, TIMING) ==
         DISPLAY_CTRL_TIMING_DISABLE))
    {
            return;
    }

    /* Count number of Vsync. */
    while (vSyncCount-- > 0)
    {
        /* If VSync is active when entering this function. Ignore it and
           wait for the next.
        */
        ulLoopCount = 0;
        do
        {
            status = FIELD_VAL_GET(peekRegisterDWord(ulDispCtrlAddr), VERTICAL_SYNC, VSYNC);
            //Insert delay to reduce number of Vsync checks
            ddk770_timerWaitTicks(3, 0xffff);
            if(ulLoopCount++ > ulDeadLoopCount) break;
        }
        while (status == VERTICAL_SYNC_VSYNC_ACTIVE);

        /* Wait for end of vsync or timeout */
        ulLoopCount = 0;
        do
        {
            status = FIELD_VAL_GET(peekRegisterDWord(ulDispCtrlAddr), VERTICAL_SYNC, VSYNC);
            ddk770_timerWaitTicks(3, 0xffff);
            if(ulLoopCount++ > ulDeadLoopCount) break;
        }
        while (status == VERTICAL_SYNC_VSYNC_INACTIVE);
    }
}

/*
 * Use panel vertical sync line as time delay function.
 * This function does not wait for the next VSync. Instead, it will wait
 * until the current line reaches the Vertical Sync line.
 * This function is really useful when flipping display to prevent tearing.
 *
 * Input: display control (CHANNEL0_CTRL or CHANNEL1_CTRL)
 */
void ddk770_waitVSyncLine(disp_control_t dispControl)
{
    unsigned long ulDispCtrlAddr;
    unsigned long value;
    mode_parameter_t modeParam;
    
	ulDispCtrlAddr = CURRENT_LINE + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);

    /* Get the current mode parameter of the specific display control */
    modeParam = ddk770_getCurrentModeParam(dispControl);
    
    do
    {
        value = FIELD_VAL_GET(peekRegisterDWord(ulDispCtrlAddr), CURRENT_LINE, LINE);
    }
    while (value < modeParam.vertical_sync_start);
}

/*
 * Get current display line number
 */
unsigned long ddk770_getDisplayLine(disp_control_t dispControl)
{
    unsigned long ulRegAddr;
    unsigned long ulRegValue;
    
    ulRegAddr = CURRENT_LINE + (dispControl> 1? DC_OFFSET2 : dispControl * DC_OFFSET);
    ulRegValue = FIELD_VAL_GET(peekRegisterDWord(ulRegAddr), CURRENT_LINE, LINE);

    return(ulRegValue);
}

/*
 * This functions uses software sequence to turn on/off the panel of the digital interface.
 */
void ddk770_swPanelPowerSequence(
    disp_control_t dispControl, 
    disp_state_t dispState, 
    disp_control_t dataPath,
    unsigned long vSyncDelay)
{
    unsigned long ulDispCtrlAddr;
    unsigned long ulDispCtrlReg;

    DDKDEBUGPRINT((DISPLAY_LEVEL, "ddk770_swPanelPowerSequence +\n"));

    ulDispCtrlAddr = DISPLAY_CTRL + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);
    ulDispCtrlReg = peekRegisterDWord(ulDispCtrlAddr);

    if (dispState == DISP_ON)
    {
        //If bit 27:24 are already ON. Don't need to set them again 
        //because setting panel seq is time consuming.
        if ((ulDispCtrlReg & 0x0f000000) == 0x0f000000) return;

        /* Turn on FPVDDEN. */
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, FPVDDEN, HIGH);
        pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
        ddk770_waitDispVerticalSync(dataPath, vSyncDelay);

        /* Turn on FPDATA. */
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, DATA, ENABLE);
        pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
        ddk770_waitDispVerticalSync(dataPath, vSyncDelay);

        /* Turn on FPVBIAS. */
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, VBIASEN, HIGH);
        pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
        ddk770_waitDispVerticalSync(dataPath, vSyncDelay);

        /* Turn on FPEN. */
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, FPEN, HIGH);
        pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
    }
    else
    {
        //If bit 27:24 are already OFF. Don't need to clear them again.
        if ((ulDispCtrlReg & 0x0f000000) == 0x00000000) return;

        /* Turn off FPEN. */
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, FPEN, LOW);
        pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
        ddk770_waitDispVerticalSync(dataPath, vSyncDelay);


        /* Turn off FPVBIASEN. */
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, VBIASEN, LOW);
        pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
        ddk770_waitDispVerticalSync(dataPath, vSyncDelay);

        /* Turn off FPDATA. */
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, DATA, DISABLE);
        pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
        ddk770_waitDispVerticalSync(dataPath, vSyncDelay);

        /* Turn off FPVDDEN. */
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, FPVDDEN, LOW);
        pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
    }
    DDKDEBUGPRINT((DISPLAY_LEVEL, "ddk770_swPanelPowerSequence -\n"));
}

/*
 * This function turns on/off the display control of Channel 0 or channel 1.
 *
 * Note:
 *      Turning on/off the timing and the plane requires programming sequence.
 *      The plane can not be changed without turning on the timing. However,
 *      changing the plane has no effect when the timing (clock) is off. Below,
 *      is the description of the timing and plane combination setting.
 */
//Cheok(10/18/2013): New function similar to setDisplayControl()
void ddk770_setDisplayEnable(
    disp_control_t dispControl, /* Channel 0 or Channel 1) */
    disp_state_t dispState      /* ON or OFF */
)
{
    unsigned long ulDispCtrlAddr;
    unsigned long ulDispCtrlReg;

    ulDispCtrlAddr = DISPLAY_CTRL + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);
    ulDispCtrlReg = peekRegisterDWord(ulDispCtrlAddr);

    DDKDEBUGPRINT((DISPLAY_LEVEL, "(setDisplayControl) Display Control Reg before set: %x\n", ulDispCtrlReg));

    /* Turn on/off the Panel display control */
    if (dispState == DISP_ON)
    {
         /* Timing should be enabled first before enabling the plane because changing at the
            same time does not guarantee that the plane will also enabled or disabled. 
          */
         ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, TIMING, ENABLE);
         pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
            
         ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, PLANE, ENABLE);
         ulDispCtrlReg = FIELD_SET( ulDispCtrlReg, DISPLAY_CTRL, DATA_PATH, EXTENDED); 
         pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
    }
    else
    {
         /* When turning off, there is no rule on the programming sequence since whenever the
            clock is off, then it does not matter whether the plane is enabled or disabled.
            Note: Modifying the plane bit will take effect on the next vertical sync. Need to
                  find out if it is necessary to wait for 1 vsync before modifying the timing
                  enable bit. 
          */
         ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, PLANE, DISABLE);
         pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);

         ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, TIMING, DISABLE);
         pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);

    }

    DDKDEBUGPRINT((DISPLAY_LEVEL, "(setDisplayControl) DISPLAY_CTRL after set: %x\n", peekRegisterDWord(ulDispCtrlAddr)));
}


/*
 * This function controls monitor on/off and data path.
 * It can be used to set up any veiws: single view, clone view, dual view, output with channel swap, etc.
 * However, it needs too many input parameter.
 * There are other set view functions with less parameters, but not as flexible as this one.
 *
 */
long ddk770_setDisplayView(
    disp_control_t dispOutput,         /* Monitor 0 or 1 */
    disp_state_t dispState,                /* On or off */
    disp_control_t dataPath)            /* 24 or 48 bit digital interface (optional when OFF */
{

    ddk770_setDisplayEnable(dispOutput, dispState);
    ddk770_swPanelPowerSequence(dispOutput, dispState, dataPath, 2);  /* Turn on or off output power */

    if (dispState == DISP_ON)
        ddk770_setDisplayDPMS(dispOutput, DISP_DPMS_ON);          /* DPMS on */
	else
		ddk770_setDisplayDPMS(dispOutput, DISP_DPMS_OFF);          /* DPMS off */

    return 0;
}

/*
 * Convenient function to trun on single view 
 */
long ddk770_setSingleViewOn(disp_control_t dispOutput)
{
    ddk770_setDisplayView(
        dispOutput,             /* Output monitor */
        DISP_ON,                 /* Turn On */
        dispOutput);    /* Default to 24 bit single pixel, the most used case */

    return 0;
}

/*
 * Convenient function to trun off single view 
 */
long ddk770_setSingleViewOff(disp_control_t dispOutput)
{
    ddk770_setDisplayView(
        dispOutput,           /* Output monitor */
        DISP_OFF,               /* Turn Off */
        dispOutput);    /* Default to 24 bit single pixel, the most used case */

    return 0;
}

/*
 * Convenient function to trun on clone view 
 */
long ddk770_setCloneViewOn(disp_control_t dataPath)
{
    ddk770_setDisplayView(
        CHANNEL0_CTRL,            /* For Clone view, monitor 0 has to be ON */
        DISP_ON, 
        dataPath);    /* Default to 24 bit single pixel, the most used case */

    ddk770_setDisplayView(
        CHANNEL1_CTRL,            /* For Clone view, monitor 1 has to be ON */
        DISP_ON, 
        dataPath);    /* Default to 24 bit single pixel, the most used case */

    ddk770_setDisplayView(
        CHANNEL2_CTRL,            /* For Clone view, monitor 1 has to be ON */
        DISP_ON, 
        dataPath);    /* Default to 24 bit single pixel, the most used case */

    return 0;
}

/*
 * Convenient function to trun on dual view 
 */
long ddk770_setDualViewOn()
{
    ddk770_setSingleViewOn(CHANNEL0_CTRL);
    ddk770_setSingleViewOn(CHANNEL1_CTRL);

    return 0;
}

/*
 * Convenient function to trun on dual view 
 */
// long setTripleViewOn()
// {
//     ddk770_setSingleViewOn(CHANNEL0_CTRL);
//     ddk770_setSingleViewOn(CHANNEL1_CTRL);
//     ddk770_setSingleViewOn(CHANNEL2_CTRL);
    
//     return 0;
// }


/*
 * Convenient function to trun off all views
 */
long ddk770_setAllViewOff()
{
    ddk770_setSingleViewOff(CHANNEL0_CTRL);    /* Turn Off monitor 0 */
    ddk770_setSingleViewOff(CHANNEL1_CTRL);    /* Turn Off monitor 1 */
	ddk770_setSingleViewOff(CHANNEL2_CTRL);    /* Turn Off monitor 1 */
   // setAllLVDSOff();
    return 0;
}


/*
 * Disable double pixel clock. 
 * This is a temporary function, used to patch for the random fuzzy font problem. 
 */
void ddk770_DisableDoublePixel(disp_control_t dispControl)
{
    unsigned long ulDispCtrlAddr;
    unsigned long ulDispCtrlReg;


	ulDispCtrlAddr = DISPLAY_CTRL + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);
	ulDispCtrlReg = peekRegisterDWord(ulDispCtrlAddr);

	ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, DOUBLE_PIXEL_CLOCK, DISABLE);

	pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);

}



void ddk770_EnableDoublePixel(disp_control_t dispControl)
{
    unsigned long ulDispCtrlAddr;
    unsigned long ulDispCtrlReg;


	ulDispCtrlAddr = DISPLAY_CTRL + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);
	ulDispCtrlReg = peekRegisterDWord(ulDispCtrlAddr);

	ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, DOUBLE_PIXEL_CLOCK, ENABLE);

	pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);

}






void ddk770_SetLVDSHalfPixel(unsigned int enable)
{

	unsigned int ulTmpValue;

	ulTmpValue = peekRegisterDWord(CLOCK3_CTL);

	if(enable){
		ulTmpValue = FIELD_SET(ulTmpValue, CLOCK3_CTL, LVDS1CLK, HALF);
		ulTmpValue = FIELD_SET(ulTmpValue, CLOCK3_CTL, LVDS0CLK, HALF);
	}else{
		ulTmpValue = FIELD_SET(ulTmpValue, CLOCK3_CTL, LVDS1CLK, NORMAL);
		ulTmpValue = FIELD_SET(ulTmpValue, CLOCK3_CTL, LVDS0CLK, NORMAL);
	}	
	pokeRegisterDWord(CLOCK3_CTL, ulTmpValue);	//Open half clock.

}



void ddk770_setupLVDS(unsigned short lvds_ch)
{
    //unsigned int ulTmpValue;
    unsigned int lvdsAddr;

    switch(lvds_ch){
        case 0:
            lvdsAddr=0x3400;
            break;
        case 1:
            lvdsAddr=0x3800;
            break; 
    }
    printk("LVDS%d initiation start\n",lvds_ch);
    pokeRegisterDWord(lvdsAddr + (0x15 << 2), 0x40);
    
    // Set Lvds PLL register
    pokeRegisterDWord(lvdsAddr + (0x0B << 2), 0xf8);
    pokeRegisterDWord(lvdsAddr + (0x01 << 2), 0x12);
    // Wait Lvds PLL lock
    while(1)
    {
        if ((peekRegisterDWord(0x3fec) & (0x1<<lvds_ch)) != 0)
        {
            break;
        }
        usleep_range(100, 200);
    }
    pokeRegisterDWord(lvdsAddr + (0x01 << 2), 0x92);

	usleep_range(1000, 1100);
   

}

void ddk770_setupSingleLVDS(unsigned short lvds_ch,disp_control_t dataPath)
{
    unsigned long ulTmpValue;


	ddk770_DisableDoublePixel(dataPath);


	if (dataPath == 0)
    {
        ddk770_enableDC0(0);        
    }
    else if (dataPath == 1)
    {
        ddk770_enableDC1(0);       
    }
    else if (dataPath == 2)
    {
        ddk770_enableDC2(0);
    }

	ddk770_SetLVDSHalfPixel(0);

	
	if (dataPath == 0)
    {
        ddk770_enableDC0(1);        
    }
    else if (dataPath == 1)
    {
        ddk770_enableDC1(1);       
    }
    else if (dataPath == 2)
    {
        ddk770_enableDC2(1);
    }


	if(lvds_ch == 0)
		setDCMUX(LVDS0,dataPath);
	else
		setDCMUX(LVDS1,dataPath);

	ddk770_setupLVDS(lvds_ch);

     // Lvds serdes turn on
    ulTmpValue = peekRegisterDWord(LVDS_SET);
	if(lvds_ch){
		ulTmpValue = FIELD_SET(ulTmpValue, LVDS_SET, LVDS1_ENABLE, ON);
		ulTmpValue = FIELD_SET(ulTmpValue, LVDS_SET, LVDS1_PD, OFF);
	}else{
		ulTmpValue = FIELD_SET(ulTmpValue, LVDS_SET, LVDS0_ENABLE, ON);
		ulTmpValue = FIELD_SET(ulTmpValue, LVDS_SET, LVDS0_PD, OFF);
	}
    pokeRegisterDWord(LVDS_SET, ulTmpValue); 
    printk("Single LVDS%d initiation end\n",lvds_ch);
}
/*
 *  Set up 48 bit LVDS output with the input DC channel.
 */
long ddk770_set48bitLVDS(disp_control_t dataPath)
{
    unsigned long ulTmpValue;

	ddk770_EnableDoublePixel(dataPath);

	if (dataPath == 0)
    {
        ddk770_enableDC0(0);        
    }
    else if (dataPath == 1)
    {
        ddk770_enableDC1(0);       
    }
    else if (dataPath == 2)
    {
        ddk770_enableDC2(0);
    }

	
	ddk770_SetLVDSHalfPixel(1);


	if (dataPath == 0)
    {
        ddk770_enableDC0(1);        
    }
    else if (dataPath == 1)
    {
        ddk770_enableDC1(1);       
    }
    else if (dataPath == 2)
    {
        ddk770_enableDC2(1);
    }


	
	setDCMUX(LVDS0,dataPath);
	setDCMUX(LVDS1,dataPath);
    ddk770_setupLVDS(0);
    ddk770_setupLVDS(1);
	// Lvds serdes turn on
    ulTmpValue = peekRegisterDWord(LVDS_SET);
	ulTmpValue = FIELD_SET(ulTmpValue, LVDS_SET, LVDS1_ENABLE, ON);
	ulTmpValue = FIELD_SET(ulTmpValue, LVDS_SET, LVDS1_PD, OFF);
	ulTmpValue = FIELD_SET(ulTmpValue, LVDS_SET, LVDS0_ENABLE, ON);
	ulTmpValue = FIELD_SET(ulTmpValue, LVDS_SET, LVDS0_PD, OFF);

	pokeRegisterDWord(LVDS_SET, ulTmpValue); 

    printk("Dual LVDS initiation end\n");



	return 0;
}


void clearDCOutput(void)
{
     pokeRegisterDWord(DC_MUX, 0x0);
}

disp_control_t getOutputDC(disp_output_t dcOutput)
 {
     unsigned int output=peekRegisterDWord(DC_MUX);
     output=output>>(dcOutput*4);
     if((output&0xf)==1)
         return CHANNEL0_CTRL;
     else if ((output&0xf)==2)
        return CHANNEL1_CTRL;
     else if ((output&0xf)==4)
         return CHANNEL2_CTRL;
     else
    {
        printk("No DC output to this mode\n");
        return -1;
    }

 }
void setDCMUX(disp_output_t dcOutput,disp_control_t dc)
{
    unsigned int output=peekRegisterDWord(DC_MUX);

    if(DEFAULT_DC==dcOutput)
        return;

    output &=~(0xf<<(dcOutput*4)); // clear this bit before set
    if(dc==CHANNEL0_CTRL)
    {
        output|=0x1<<(dcOutput*4); // set this bet
    }
    else if(dc==CHANNEL1_CTRL)   
    {
        output|=0x2<<(dcOutput*4);
    } 
    else if(dc==CHANNEL2_CTRL)
    {
        output|=0x4<<(dcOutput*4); 
    }   
    else
    {
        return;
    }
       
	printk("0x3ffc value will be %x\n",output);
	
    pokeRegisterDWord(DC_MUX, output);
}

void ClearDCMUX(disp_output_t dcOutput)
{
    unsigned int output=peekRegisterDWord(DC_MUX);

    if(DEFAULT_DC==dcOutput)
        return;

    output &=~(0xf<<(dcOutput*4)); // clear this bit
    pokeRegisterDWord(DC_MUX, output);
}

unsigned char GetDCMUX(disp_output_t dcOutput)
{
    unsigned int output;

    output = peekRegisterDWord(DC_MUX);
    return output & (0xf<<(dcOutput*4)); // get this byte
}

void cloneDCOutput(disp_control_t dc)
{
    unsigned int output;
    if(dc==CHANNEL0_CTRL)
        output=0x1111111;

    else if(dc==CHANNEL1_CTRL)    
        output =0x2222222;

    else
        output=0x4444444; 

    pokeRegisterDWord(DC_MUX, output);
}





/*
 * This function is to enable/disable display channel timing.
 *
 */
void ddk770_EnableChannelTiming(
   disp_control_t dispControl, /* Channel 0 or Channel 1) */
   unsigned short enable
)
{
    unsigned long ulDispCtrlAddr;
    unsigned long ulDispCtrlReg;

    //DDKDEBUGPRINT((DISPLAY_LEVEL, "DisableChannelTiming + \n"));

    ulDispCtrlAddr = DISPLAY_CTRL + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);
    ulDispCtrlReg = peekRegisterDWord(ulDispCtrlAddr); 

    if(enable)
    {
        /* Turn on the display Panel.*/          
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, TIMING, ENABLE);
        
    }
    else
    {
        /* Turn off the display Panel.*/
        ulDispCtrlReg = FIELD_SET(ulDispCtrlReg, DISPLAY_CTRL, TIMING, DISABLE);
    }

    pokeRegisterDWord(ulDispCtrlAddr, ulDispCtrlReg);
}

unsigned char ddk770_isTimingEnable(disp_control_t dispControl)
{
    unsigned long ulDispCtrlAddr;
    unsigned long ulDispCtrlReg;

    ulDispCtrlAddr = DISPLAY_CTRL + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);
    ulDispCtrlReg = peekRegisterDWord(ulDispCtrlAddr); 
   
    return ((FIELD_VAL_GET(ulDispCtrlReg, DISPLAY_CTRL, TIMING) == DISPLAY_CTRL_TIMING_ENABLE) ? 1 : 0);
}

