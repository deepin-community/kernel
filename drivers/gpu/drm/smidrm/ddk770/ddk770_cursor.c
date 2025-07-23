/*******************************************************************
* 
*         Copyright (c) 2007 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  CURSOR.C --- Voyager GX SDK 
*  This file contains the definitions for the Panel cursor functions.
* 
*******************************************************************/
#include "ddk770_reg.h"
#include "ddk770_cursor.h"
#include "ddk770_help.h"
#include "ddk770_display.h"



static int makeEven(int num) {

    return num & (~1U);
}



/*
 * This function set alpha cursor size
 */
void SetAlphaCursorSize(
    disp_control_t dispControl, /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned long size			
)
{
    unsigned long cursorRegister, value;

    cursorRegister = HWC_LOCATION + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);
    
	value = peekRegisterDWord(cursorRegister);
    value = FIELD_VALUE(value, HWC_LOCATION, SIZE, size);
    
    pokeRegisterDWord(cursorRegister, value);
}



/*
 * This function set alpha cursor size
 */
void SetCursorPrefetch(
    disp_control_t dispControl, /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned int enable			
)
{
    unsigned long cursorRegister, value;

    cursorRegister = HWC_LOCATION + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);
    
	value = peekRegisterDWord(cursorRegister);
	if(enable)
    	value = FIELD_SET(value, HWC_LOCATION, PREFETCH, ENABLE);
    else
		value = FIELD_SET(value, HWC_LOCATION, PREFETCH, DISABLE);
	
    pokeRegisterDWord(cursorRegister, value);
    ddk770_waitDispVerticalSync(dispControl,2);
}





/*
 * This function initializes the cursor attributes.
 */
void ddk770_initCursor(
    disp_control_t dispControl,     /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned long base,             /* Base Address */ 
    unsigned long color0,           /* Monochrome cursor color 0 in RGB 8 8 8 format */
    unsigned long color1,            /* Monochrome cursor color 1 in RGB 8 8 8 format */
    unsigned long color2            /* Monochrome cursor color 2 in RGB 8 8 8 format */
)
{
    /*
     * 1. Set the cursor source address 
     */
    pokeRegisterDWord(
        HWC_CONTROL + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET),
        
        FIELD_VALUE(0, HWC_CONTROL, ADDRESS, base));
        
    /*
     * 2. Set the cursor color composition 
     */
    pokeRegisterDWord(
        HWC_COLOR0 + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET),
        FIELD_VALUE(0, HWC_COLOR0, RGB888, color0));

    pokeRegisterDWord(
        HWC_COLOR1 + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET),
        FIELD_VALUE(0, HWC_COLOR1, RGB888, color1));


	SetAlphaCursorSize(dispControl,0);
	
}

/*
 * This function sets the cursor position.
 */
void ddk770_setCursorPosition(
    disp_control_t dispControl,     /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned long dx,               /* X Coordinate of the cursor */
    unsigned long dy,               /* Y Coordinate of the cursor */
    unsigned char topOutside,       /* Top Boundary Select: either partially outside (= 1) 
                                       or within the screen top boundary (= 0) */
    unsigned char leftOutside,       /* Left Boundary Select: either partially outside (= 1) 
                                       or within the screen left boundary (= 0) */
    unsigned int enable	
)
{  
    unsigned long cursorRegister, value;

	cursorRegister = HWC_LOCATION + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);

	value = peekRegisterDWord(cursorRegister);

    /* Set the XY coordinate */
    value = FIELD_VALUE(value, HWC_LOCATION, X, dx);
    value = FIELD_VALUE(value, HWC_LOCATION, Y, makeEven(dy));
    
    /* Set the top boundary select either partially outside the top boundary
       screen or inside */
    if (topOutside)
        value = FIELD_SET(value, HWC_LOCATION, TOP, OUTSIDE);
    else         
        value = FIELD_SET(value, HWC_LOCATION, TOP, INSIDE);

    /* Set the left boundary select either partially outside the left boundary
       screen or inside */
    if (leftOutside){
        value = FIELD_SET(value, HWC_LOCATION, LEFT, OUTSIDE);
    }
    else{        
        value = FIELD_SET(value, HWC_LOCATION, LEFT, INSIDE);
    }

	value = FIELD_VALUE(value, HWC_LOCATION, SIZE, 0);

    if(enable)
    	value = FIELD_SET(value, HWC_LOCATION, PREFETCH, ENABLE);
    else
		value = FIELD_SET(value, HWC_LOCATION, PREFETCH, DISABLE);
    /* Set the register accordingly, either Panel cursor or CRT cursor */
    pokeRegisterDWord(cursorRegister, value);
    
    //ddk770_waitDispVerticalSync(dispControl,2);
}




/*
 * This function enables/disables the cursor.
 */
void ddk770_enableCursor(
    disp_control_t dispControl, /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned long mode			/* Cursor type - 00 disable, 0x01 mask cursor, 0x02 mono, 0x03 alpha cursor */
)
{
    unsigned long cursorRegister, value;


#if 0
	cursorRegister = HWC_LOCATION + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);
	value = peekRegisterDWord(cursorRegister);
	value = FIELD_SET(value, HWC_LOCATION, PREFETCH, DISABLE);
	pokeRegisterDWord(cursorRegister, value);
#endif

	cursorRegister = HWC_CONTROL + (dispControl> 1? CHANNEL_OFFSET2 : dispControl * CHANNEL_OFFSET);
	value = peekRegisterDWord(cursorRegister);
    value = FIELD_VALUE(value, HWC_CONTROL, MODE, mode);
    
    pokeRegisterDWord(cursorRegister, value);

}
