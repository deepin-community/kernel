/*******************************************************************
* 
*         Copyright (c) 2007 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  cursor.h --- SMI DDK 
*  This file contains the definitions for the cursor functions.
* 
*******************************************************************/
#ifndef _DDk770_CURSOR_H_
#define _DDK770_CURSOR_H_

#include "ddk770_mode.h"

#define CURSOR_DISABLE	0x0
#define CURSOR_ALPHA	0x3
#define CURSOR_MONO		0x2
#define CURSOR_MASK		0x1


/*
 * This function initializes the cursor attributes.
 */
void ddk770_initCursor(
    disp_control_t dispControl,     /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned long base,             /* Base Address */ 
    unsigned long color0,           /* Monochrome cursor color 0 in RGB 8 8 8 format */
    unsigned long color1,            /* Monochrome cursor color 1 in RGB 8 8 8 format */
    unsigned long color2            /* Monochrome cursor color 2 in RGB 8 8 8 format */
);


void SetCursorPrefetch(
    disp_control_t dispControl, /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned int enable			
);


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
);

/*
 * This function set alpha cursor size
 */
void SetAlphaCursorSize(
    disp_control_t dispControl, /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned long size			
);


/*
 * This function enables/disables the cursor.
 */
void ddk770_enableCursor(
    disp_control_t dispControl,     /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned long enable
);

#endif /* _CURSOR_H_ */
