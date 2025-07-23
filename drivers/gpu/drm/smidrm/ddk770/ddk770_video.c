/*******************************************************************
* 
*         Copyright (c) 2007 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  Video.C --- Voyager GX SDK 
*  This file contains the definitions for the Video functions.
* 
*******************************************************************/
#include "ddk770_reg.h"
#include "ddk770_hardware.h"
#include "ddk770_chip.h"
#include "ddk770_mode.h"
#include "ddk770_video.h"
#include "ddk770_help.h"

/* New video function */

#define SCALE_CONSTANT                      (1 << 12)

/* Offset Adjustment for the window */
static short gWidthAdjustment = 0;
static short gHeightAdjustment = 0;

/* Source Video Width and Height */
static unsigned long gSrcVideoWidth = 0;
static unsigned long gSrcVideoHeight = 0;
static unsigned long gPerfModeStride = 0; //Performance mode parameter.

/*
 * ddk770_videoGetBufferStatus
 *      This function gets the status of the video buffer, either the buffer
 *      has been used or not.
 *
 *  Input:
 *      bufferIndex     - The index of the buffer which size to be retrieved
 *
 *  Output:
 *      0 - No flip pending
 *      1 - Flip pending
 */
unsigned long ddk770_videoGetBufferStatus(
    unsigned long bufferIndex
)
{
        return (FIELD_VAL_GET(peekRegisterDWord(VIDEO_FB_ADDRESS), VIDEO_FB_ADDRESS, STATUS));
}

/*
 * ddk770_videoGetPitch
 *      This function gets the video plane pitch
 *
 * Output:
 *      pitch   - Number of bytes per line of the video plane 
 *                specified in 128-bit aligned bytes.
 */
unsigned short ddk770_videoGetPitch()
{
    return (FIELD_VAL_GET(peekRegisterDWord(VIDEO_FB_WIDTH), VIDEO_FB_WIDTH, WIDTH));
}

/*
 * ddk770_videoGetLineOffset
 *      This function gets the video plane line offset
 *
 * Output:
 *      lineOffset  - Number of 128-bit aligned bytes per line 
 *                    of the video plane.
 */
unsigned short ddk770_videoGetLineOffset()
{
    return (FIELD_VAL_GET(peekRegisterDWord(VIDEO_FB_WIDTH), VIDEO_FB_WIDTH, OFFSET));
}

/*
 * ddk770_videoGetBufferSize
 *      This function gets the buffer size
 *
 *  Input:
 *      bufferIndex - The index of the buffer which size to be retrieved
 */
static unsigned long ddk770_videoGetBufferSize(
    unsigned long bufferIndex
)
{
    unsigned long value;
    
    if (bufferIndex == 0)
    {
        value -= (unsigned long)
            FIELD_VAL_GET(peekRegisterDWord(VIDEO_FB_ADDRESS), VIDEO_FB_ADDRESS, ADDRESS);
    }
    
    return value;
}


/*
 * ddk770_videoGetBuffer
 *      This function gets the video buffer
 *
 *  Input:
 *      bufferIndex - The index of the buffer to get
 *
 *  Output:
 *      The video buffer of the requested index.
 */
unsigned long ddk770_videoGetBuffer(
    unsigned char bufferIndex
)
{
        return (FIELD_VAL_GET(peekRegisterDWord(VIDEO_FB_ADDRESS), VIDEO_FB_ADDRESS, ADDRESS));
}

/*
 * ddk770_videoSetBufferLastAddress
 *      This function sets the video buffer last address.
 *      The value can be calculated by subtracting one line offset 
 *      from the buffer size (Total number of line offset * 
 *      source video height).
 *
 *  Input:
 *      bufferIndex         - The index of the buffer to be set
 *      bufferSize          - Size of the video buffer.
 */
static void ddk770_videoSetBufferLastAddress(
    unsigned char bufferIndex,          /* The index of the buffer to be set. */
    unsigned long bufferStart,          /* Buffer start */
    unsigned long bufferSize            /* Size of the video buffer */
)
{
	//if (ddk770_getChipType() == SM750)    
	if (ddk770_getChipType() == SM768)    
	{
	    /* Substract with one line offset to get the last address value when added
	       with the bufferStart. Somehow, this is only happen in SM750 chip */    
	    bufferSize -= (unsigned long) ddk770_videoGetLineOffset();
	}
}

/*
 * ddk770_videoGetBufferLastAddress
 *      This function gets the video buffer last address.
 *
 *  Input:
 *      bufferIndex         - The index of the buffer last address to be retrieved
 */
static unsigned long ddk770_videoGetBufferLastAddress(
    unsigned char bufferIndex           /* The index of the buffer last address to be retrieved. */
)
{
	return 0;
}

/*
 * ddk770_videoSetBuffer
 *      This function sets the video buffer
 *
 *  Input:
 *      bufferIndex         - The index of the buffer to be set
 *      bufferStartAddress  - The starting address of the buffer
 */
void ddk770_videoSetBuffer(
	unsigned dispCtrl,
    unsigned char bufferIndex,          /* The index of the buffer to be set. */
    unsigned long bufferStartAddress    /* Video buffer with 128-bit alignment */
)
{
    unsigned long bufferSize, lastAddress;
	unsigned long regFB;

    /* Get the buffer size first */
    bufferSize = ddk770_videoGetBufferSize(bufferIndex);
    
    lastAddress = ddk770_videoGetBufferLastAddress(bufferIndex);
    
	//if (ddk770_getChipType() == SM750)    
	if (ddk770_getChipType() == SM768)    
	{
	    if (lastAddress <= (bufferStartAddress + bufferSize - ddk770_videoGetLineOffset()))
	        ddk770_videoSetBufferLastAddress(bufferIndex, bufferStartAddress, bufferSize);
    }
    else
    {
		if (lastAddress <= (bufferStartAddress + bufferSize))
	        ddk770_videoSetBufferLastAddress(bufferIndex, bufferStartAddress, bufferSize);
    }

    if (bufferIndex == 0)
    {
        regFB =  VIDEO_FB_ADDRESS+(dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);
	    pokeRegisterDWord(regFB,
	            FIELD_SET(0, VIDEO_FB_ADDRESS, STATUS, PENDING) |
	            FIELD_VALUE(0, VIDEO_FB_ADDRESS, ADDRESS, bufferStartAddress));
    }
}
/*
 * ddk770_videoSetUVBuffer
 *      This function sets the video buffer
 *
 *  Input:
 *      bufferIndex         - The index of the buffer to be set
 *      bufferStartAddress  - The starting address of the buffer
 */
void ddk770_videoSetUVBuffer(
	unsigned dispCtrl,
    unsigned long bufferStartUAddress,    /* Video buffer with 128-bit alignment */
    unsigned long bufferStartVAddress    /* Video buffer with 128-bit alignment */
)
{
    //unsigned long bufferSize;
	unsigned long regU, regV;

	
    regU = VIDEO_FB_ADDRESS_U + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);
    regV = VIDEO_FB_ADDRESS_V + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

    pokeRegisterDWord(regU,
        FIELD_VALUE(0, VIDEO_FB_ADDRESS_U, ADDRESS, bufferStartUAddress));
    pokeRegisterDWord(regV,
        FIELD_VALUE(0, VIDEO_FB_ADDRESS_V, ADDRESS, bufferStartVAddress));
}

/*
 * ddk770_videoSetPitchOffset
 *      This function sets the video plane pitch and offset
 *
 *  Input:
 *      pitch           - Number of bytes per line of the video plane 
 *                        specified in 128-bit aligned bytes.
 *      lineOffset      - Number of 128-bit aligned bytes per line 
 *                        of the video plane.
 */
void ddk770_videoSetPitchOffset(
	unsigned dispCtrl,
    unsigned short pitch,
    unsigned short lineOffset
)
{
	unsigned long regWidth;

    /* Set Video Buffer Offset (pitch) */
    regWidth = VIDEO_FB_WIDTH + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

    lineOffset = alignLineOffset(lineOffset);

	pokeRegisterDWord(regWidth,
	    FIELD_VALUE(0, VIDEO_FB_WIDTH, WIDTH, pitch) |
	    FIELD_VALUE(0, VIDEO_FB_WIDTH, OFFSET, lineOffset));
}
/*
 * ddk770_videoSetUVPitchOffset
 *      This function sets the video plane pitch and offset of U and V
 *
 *  Input:
 *      pitch           - Number of bytes per line of the video plane 
 *                        specified in 128-bit aligned bytes.
 *      lineOffset      - Number of 128-bit aligned bytes per line 
 *                        of the video plane.
 */
void ddk770_videoSetUVPitchOffset(
	unsigned dispCtrl,
    unsigned short pitch,
    unsigned short lineOffset
)
{
	unsigned long regWidthU, regWidthV;

    regWidthU = VIDEO_FB_WIDTH_U + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);
    regWidthV = VIDEO_FB_WIDTH_V + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

    lineOffset = alignLineOffset(lineOffset);

	pokeRegisterDWord(regWidthU,
	    FIELD_VALUE(0, VIDEO_FB_WIDTH_U, WIDTH, pitch) |
	    FIELD_VALUE(0, VIDEO_FB_WIDTH_U, OFFSET, lineOffset));
	pokeRegisterDWord(regWidthV,
	    FIELD_VALUE(0, VIDEO_FB_WIDTH_V, WIDTH, pitch) |
	    FIELD_VALUE(0, VIDEO_FB_WIDTH_V, OFFSET, lineOffset));
}
/*
 *  ddk770_videoSetLast 
 *      This function sets the video source last lines and width.
 *
 *  Input:
 *
 *      width      - Video source width
 *      height      - Video source height
 */
void ddk770_videoSetLast(
	unsigned dispCtrl,
    unsigned long width,
    unsigned long height
)
{
#if 0 // SM768 don't have this register. Leave empty function here.
    if (dispCtrl == CHANNEL0_CTRL)
	{
	    pokeRegisterDWord(CHANNEL0_VIDEO_LAST, 
	        FIELD_VALUE(0, CHANNEL0_VIDEO_LAST, COLUMN, width) |
	        FIELD_VALUE(0, CHANNEL0_VIDEO_LAST, ROW, height)); 
	}
	else
	{
	    pokeRegisterDWord(CHANNEL1_VIDEO_LAST, 
	        FIELD_VALUE(0, CHANNEL1_VIDEO_LAST, COLUMN, width) |
	        FIELD_VALUE(0, CHANNEL1_VIDEO_LAST, ROW, height)); 
	}													   
#endif
}
/*
 *  ddk770_videoSetWindowSize
 *      This function sets the video window size.
 *
 *  Input:
 *      width       - Video Window width
 *      height      - Video Window height
 */
void ddk770_videoSetWindowSize(
	unsigned dispCtrl,
    unsigned long width,
    unsigned long height
)
{
    unsigned long value, startX, startY;
	unsigned long regTL, regBR;
	
    regTL =VIDEO_PLANE_TL+(dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);
    regBR =VIDEO_PLANE_BR+(dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

	value = peekRegisterDWord(regTL);
	startX = FIELD_VAL_GET(value, VIDEO_PLANE_TL, LEFT);
	startY = FIELD_VAL_GET(value, VIDEO_PLANE_TL, TOP);

	/* Set bottom and right position */
	pokeRegisterDWord(regBR,
	    FIELD_VALUE(0, VIDEO_PLANE_BR, BOTTOM, startY + height - 1 - gHeightAdjustment) |
	    FIELD_VALUE(0, VIDEO_PLANE_BR, RIGHT, startX + width - 1 - gWidthAdjustment)); 
}

/*
 *  ddk770_videoGetWindowSize
 *      This function gets the video window size.
 *
 *  Output:
 *      width       - Video Window width
 *      height      - Video Window height
 */
void ddk770_videoGetWindowSize(
	unsigned dispCtrl,
    unsigned long *pVideoWidth,
    unsigned long *pVideoHeight
)
{
    unsigned long positionTopLeft, positionRightBottom;
    unsigned long videoWidth, videoHeight;
	unsigned long regTL, regBR;
	
    regTL = VIDEO_PLANE_TL + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);
	
    regBR = VIDEO_PLANE_BR + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

	positionTopLeft = peekRegisterDWord(regTL);
	positionRightBottom = peekRegisterDWord(regBR);
	videoWidth  = FIELD_VAL_GET(positionRightBottom, VIDEO_PLANE_BR, RIGHT) - 
	              FIELD_VAL_GET(positionTopLeft, VIDEO_PLANE_TL, LEFT) + 1 +
	              gWidthAdjustment;
	videoHeight = FIELD_VAL_GET(positionRightBottom, VIDEO_PLANE_BR, BOTTOM) - 
	              FIELD_VAL_GET(positionTopLeft, VIDEO_PLANE_TL, TOP) + 1 +
	              gHeightAdjustment;

    if (pVideoWidth != ((unsigned long *)0))
        *pVideoWidth = videoWidth;
    
    if (pVideoHeight != ((unsigned long *)0))
        *pVideoHeight = videoHeight;
}

/*
 *  ddk770_videoSetPosition
 *      This function sets the video starting coordinate position.
 *
 *  Input:
 *      startX      - X Coordinate of the video window starting position
 *      startY      - Y Coordinate of the video window starting position
 */
void ddk770_videoSetPosition(
	unsigned dispCtrl,
    unsigned long startX,
    unsigned long startY
)
{
    unsigned long videoWidth, videoHeight;
	unsigned long regTL;

    regTL = VIDEO_PLANE_TL + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

    /* Get the video window width and height */
    ddk770_videoGetWindowSize(dispCtrl, &videoWidth, &videoHeight);
    
	pokeRegisterDWord(regTL,
	    FIELD_VALUE(0, VIDEO_PLANE_TL, TOP, startY) |
	    FIELD_VALUE(0, VIDEO_PLANE_TL, LEFT, startX));

    /* Set bottom and right position */    
    ddk770_videoSetWindowSize(dispCtrl, videoWidth, videoHeight);

}

/*
 *  ddk770_videoSetConstants
 *      This function sets the video constants. The actual component will be
 *      added by this constant to get the expected component value.
 *
 *  Input:
 *      yConstant       - Y Constant Value
 *      redConstant     - Red Constant Value
 *      greenConstant   - Green Constant Value
 *      blueConstant    - Blue Constant Value
 */
void ddk770_videoSetConstants(
    unsigned dispCtrl,
    unsigned char  yConstant,               /* Y Adjustment */
    unsigned char  redConstant,             /* Red Conversion constant */
    unsigned char  greenConstant,           /* Green Conversion constant */
    unsigned char  blueConstant             /* Blue Conversion constant */
)
{
	unsigned long regYUV;

    regYUV = VIDEO_YUV_CONSTANTS + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);
	pokeRegisterDWord(regYUV,
	    FIELD_VALUE(0, VIDEO_YUV_CONSTANTS, Y, yConstant) |
	    FIELD_VALUE(0, VIDEO_YUV_CONSTANTS, R, redConstant) |
	    FIELD_VALUE(0, VIDEO_YUV_CONSTANTS, G, greenConstant) |
	    FIELD_VALUE(0, VIDEO_YUV_CONSTANTS, B, blueConstant));
}

/*
 *  ddk770_videoSetInitialScale
 *      This function sets the video buffer initial vertical scale.
 *
 *  Input:
 *      bufferIndex         - Index of the buffer which vertical scale value
 *                            to be set.
 *      bufferInitScale     - Buffer Initial vertical scale value
 */
void ddk770_videoSetInitialScale(
	unsigned dispCtrl,
    unsigned short InitScaleHorizontal,
    unsigned short InitScaleVertical
)
{
    unsigned long value;
	unsigned long regScale;

    regScale = VIDEO_INITIAL_SCALE + (dispCtrl > 1? CHANNEL_OFFSET2: dispCtrl*CHANNEL_OFFSET);

    value = peekRegisterDWord(regScale);
    value = FIELD_VALUE(value, VIDEO_INITIAL_SCALE, VERTICAL, InitScaleVertical);
    value = FIELD_VALUE(value, VIDEO_INITIAL_SCALE, HORIZONTAL, InitScaleHorizontal);
    pokeRegisterDWord(regScale, value);
}

/*
 *  ddk770_videoGetInitialScale
 *      This function gets the video buffer initial vertical scale.
 *
 *  Input:
 *      pbuffer0InitScale   - Pointer to variable to store buffer 0 initial vertical scale
 *      pbuffer1InitScale   - Pointer to variable to store buffer 1 initial vertical scale
 */
__attribute__((unused)) static void ddk770_videoGetInitialScale(
	unsigned dispCtrl,
    unsigned short *pBufferVInitScale,
    unsigned short *pBufferHInitScale
)
{
	unsigned long regScale;

	regScale = VIDEO_INITIAL_SCALE + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

    *pBufferHInitScale = (unsigned short)
        FIELD_VAL_GET(peekRegisterDWord(regScale), VIDEO_INITIAL_SCALE, HORIZONTAL);
    *pBufferVInitScale = (unsigned short)
        FIELD_VAL_GET(peekRegisterDWord(regScale), VIDEO_INITIAL_SCALE, VERTICAL);
}

/*
 *  ddk770_videoScale
 *      This function scales the video.
 *
 *  Input:
 *      srcWidth     - The source video width
 *      srcHeight    - The source video height
 *      dstWidth     - The destination video width 
 *      dstHeight    - The destination video height
 */
static void ddk770_videoScale(
	unsigned dispCtrl,
    unsigned long srcWidth,
    unsigned long srcHeight,
    unsigned long dstWidth,
    unsigned long dstHeight
)
{
    unsigned long value = 0;
    unsigned long scaleFactor;
	unsigned long regScale;

	regScale = VIDEO_SCALE + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

	if (dstHeight > srcHeight)
	{
	    /* Calculate the factor */
	    scaleFactor = (srcHeight-1) * SCALE_CONSTANT / dstHeight;
	    value = FIELD_VALUE(value, VIDEO_SCALE , VERTICAL_SCALE, scaleFactor);
	}
	
	/* Scale the horizontal size */
	if (dstWidth > srcWidth)
	{
	    /* Calculate the factor */
	    scaleFactor = (srcWidth-1) * SCALE_CONSTANT / dstWidth;
	    value = FIELD_VALUE(value, VIDEO_SCALE, HORIZONTAL_SCALE, scaleFactor);
	}
	
	pokeRegisterDWord(regScale, value);
}


/*
 *  ddk770_videoSwapYUVByte
 *      This function swaps the YUV data byte.
 *
 *  Input:
 *      byteSwap    - Flag to enable/disable YUV data byte swap.
 */
void ddk770_videoSwapYUVByte(
	unsigned dispCtrl,
   	video_byteswap_t byteSwap  
)
{
    unsigned long value;
	unsigned long regCtrl;

	regCtrl = VIDEO_DISPLAY_CTRL + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

	value = peekRegisterDWord(regCtrl);
	if (byteSwap == SWAP_BYTE)
	    value = FIELD_SET(value, VIDEO_DISPLAY_CTRL, BYTE_SWAP, ENABLE);
	else
	    value = FIELD_SET(value, VIDEO_DISPLAY_CTRL, BYTE_SWAP, DISABLE);
	
	pokeRegisterDWord(regCtrl, value);
}


void ddk770_videoYUVPrefetch(
	unsigned dispCtrl,
   	unsigned long enable   
)
{
    unsigned long value;
	unsigned long regCtrl;

	regCtrl = VIDEO_SOURCE_SIZE + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

	value = peekRegisterDWord(regCtrl);
	if (enable)
	{   
	    value = FIELD_SET(value, VIDEO_SOURCE_SIZE, YUVPREFETCH, ENABLE);
	    value = FIELD_SET(value, VIDEO_SOURCE_SIZE, YUVARLENINC, ENABLE);
	    value = FIELD_SET(value, VIDEO_SOURCE_SIZE, YUVUVREDUCE, DISABLE);
	}
	else
	{   
	    value = FIELD_SET(value, VIDEO_SOURCE_SIZE, YUVPREFETCH, DISABLE);
	    value = FIELD_SET(value, VIDEO_SOURCE_SIZE, YUVARLENINC, DISABLE);
	    value = FIELD_SET(value, VIDEO_SOURCE_SIZE, YUVUVREDUCE, DISABLE);
	}
	
	pokeRegisterDWord(regCtrl, value);
}




/*
 *  ddk770_videoSetInterpolation
 *      This function enables/disables the horizontal and vertical interpolation.
 *
 *  Input:
 *      enableHorzInterpolation   - Flag to enable/disable Horizontal interpolation
 *      enableVertInterpolation   - Flag to enable/disable Vertical interpolation
 */
void ddk770_videoSetInterpolation(
	unsigned dispCtrl,
    unsigned long enableHorzInterpolation,
    unsigned long enableVertInterpolation
)
{
    unsigned long value;
	unsigned long regCtrl;

	regCtrl = VIDEO_DISPLAY_CTRL + (dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

	value = peekRegisterDWord(regCtrl);
	
	if (enableHorzInterpolation)
	    value = FIELD_SET(value, VIDEO_DISPLAY_CTRL, HORIZONTAL_MODE, INTERPOLATE);
	else
	    value = FIELD_SET(value, VIDEO_DISPLAY_CTRL, HORIZONTAL_MODE, REPLICATE);
	    
	if (enableVertInterpolation)
	    value = FIELD_SET(value, VIDEO_DISPLAY_CTRL, VERTICAL_MODE, INTERPOLATE);
	else
	    value = FIELD_SET(value, VIDEO_DISPLAY_CTRL, VERTICAL_MODE, REPLICATE);
	    
	pokeRegisterDWord(regCtrl, value);
}

/*
 *  ddk770_videoGetInterpolation
 *      This function gets the horizontal and vertical interpolation enable status.
 *
 *  Input:
 *      pHorzInterpolationStatus	- Pointer to store the horizontal interpolation status
 *      pVertInterpolationStatus	- Pointer to store the vertical interpolation status
 */
void ddk770_videoGetInterpolation(
    unsigned long *pHorzInterpolationStatus,
    unsigned long *pVertInterpolationStatus
)
{
    unsigned long value;
    
    value = peekRegisterDWord(VIDEO_DISPLAY_CTRL);
    if (pHorzInterpolationStatus != (unsigned long *)0)
	{
		if (FIELD_VAL_GET(value, VIDEO_DISPLAY_CTRL, HORIZONTAL_MODE) == VIDEO_DISPLAY_CTRL_HORIZONTAL_MODE_INTERPOLATE)
        	*pHorzInterpolationStatus = 1;
		else
			*pHorzInterpolationStatus = 0;
	}
        
    if (pHorzInterpolationStatus != (unsigned long *)0)
	{
		if (FIELD_VAL_GET(value, VIDEO_DISPLAY_CTRL, VERTICAL_MODE) == VIDEO_DISPLAY_CTRL_VERTICAL_MODE_INTERPOLATE)
			*pVertInterpolationStatus = 1;
		else
			*pVertInterpolationStatus = 0;
	}
}

/*
 *  ddk770_videoSetStartPanningPixel
 *      This function sets the starting pixel number for smooth pixel panning.
 *
 *  Input:
 *      startPixel  - Starting pixel number for smooth pixel panning
 */
void ddk770_videoSetStartPanningPixel(
    unsigned char startPixel
)
{
    pokeRegisterDWord(VIDEO_DISPLAY_CTRL, 
                      peekRegisterDWord(VIDEO_DISPLAY_CTRL) | 
                      FIELD_VALUE(0, VIDEO_DISPLAY_CTRL, PIXEL, startPixel));    
}

/*
 *  videoSetGamma
 *      This function enables/disables gamma control.
 *
 *  Input:
 *      enableGammaCtrl - The gamma enable control
 *
 *  NOTE:
 *      The gamma can only be enabled in RGB565 and RGB888. Enable this gamma
 *      without proper format will have no effect.
 */
void ddk770_videoSetGammaCtrl(
    unsigned dispCtrl,
    unsigned long enableGammaCtrl
)
{
    unsigned long value;
	unsigned long regCtrl;

	regCtrl = VIDEO_DISPLAY_CTRL+(dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

	value = peekRegisterDWord(regCtrl);
	
	if (enableGammaCtrl)
	    value = FIELD_SET(value, VIDEO_DISPLAY_CTRL, GAMMA, ENABLE);
	else
	    value = FIELD_SET(value, VIDEO_DISPLAY_CTRL, GAMMA, DISABLE);
	    
	pokeRegisterDWord(regCtrl, value);    
}

/*
 *  ddk770_isVideoEnable
 *      This function check whether the video plane is already enabled or not.
 *
 *  Output:
 *      0   - Disable
 *      1   - Enable
 */
unsigned char ddk770_isVideoEnable()
{
    unsigned long value;
    
    value = peekRegisterDWord(VIDEO_DISPLAY_CTRL);
    
    return ((FIELD_VAL_GET(value, VIDEO_DISPLAY_CTRL, PLANE) == VIDEO_DISPLAY_CTRL_PLANE_ENABLE) ? 1 : 0);
}

/*
 *  videoSetCtrl
 *      This function enable/disable the video plane.
 *
 *  Input:
 *      videoCtrl   - Enable/Disable video
 */
static void videoSetCtrl(
    disp_control_t dispCtrl,
    video_ctrl_t videoCtrl
)
{
    unsigned long value;
	unsigned long regCtrl;

	regCtrl = VIDEO_DISPLAY_CTRL+(dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

	value = peekRegisterDWord(regCtrl);
	
	if (videoCtrl == VIDEO_ON)
	    value = FIELD_SET(value, VIDEO_DISPLAY_CTRL, PLANE, ENABLE);
	else
	    value = FIELD_SET(value, VIDEO_DISPLAY_CTRL, PLANE, DISABLE);
	            
	pokeRegisterDWord(regCtrl, value); 
}

/*
 *  videoSetFormat
 *      This function sets the video format.
 *
 *  Input:
 *      videoFormat - The video content format
 *                    * FORMAT_RGB565 - 16-bit RGB 5:6:5 mode
 *                    * FORMAT_YUYV - 16-bit YUYV mode
 */
static void videoSetFormat(
    unsigned dispCtrl,
    video_format_t  videoFormat
)
{
    unsigned long value;
	unsigned long regCtrl;

	regCtrl = VIDEO_DISPLAY_CTRL+(dispCtrl> 1? CHANNEL_OFFSET2 : dispCtrl * CHANNEL_OFFSET);

	value = peekRegisterDWord(regCtrl);
	switch (videoFormat)
	{
	    default:
		case FORMAT_YUV444:
	    case FORMAT_RGB565:
	        value = FIELD_SET(value,  VIDEO_DISPLAY_CTRL , FORMAT, 16);
	        break;
	    case FORMAT_RGB888:
	        value = FIELD_SET(value,  VIDEO_DISPLAY_CTRL , FORMAT, 32);
	        break;
	    case FORMAT_YUYV:
	        value = FIELD_SET(value,  VIDEO_DISPLAY_CTRL , FORMAT, YUV422);
	        break;
	    case FORMAT_YUV420:
	        value = FIELD_SET(value,  VIDEO_DISPLAY_CTRL , FORMAT, YUV420);
	        break;
	}

	if(videoFormat == FORMAT_YUV444)
		 value = FIELD_SET(value,  VIDEO_DISPLAY_CTRL , YUV444FMT, ENABLE);
	else
		 value = FIELD_SET(value,  VIDEO_DISPLAY_CTRL , YUV444FMT, DISABLE);
	
	pokeRegisterDWord(regCtrl, value);
}

/*
 *  ddk770_videoGetEdgeDetection
 *      This function gets the information whether the edge detection is enabled or not.
 *      It also outputs the edge detection value if required.
 *      This function only works in SM718. SM750 does not support this feature.
 *
 *  Input:
 *      pEdgeDetectValue    - Pointer to a buffer to store the edge detection value.
 *
 *  Note:
 *      0   - Edge Detection is disabled
 *      1   - Edge Detection is enabled
 */
unsigned long ddk770_videoGetEdgeDetection(
    unsigned long *pEdgeDetectValue
)
{ 
    return 0;
}

/*
 *  ddk770_videoSetup
 *      This function setups the video. This function only works in SM718. 
 *      SM750 does not support edge detection feature. If calling this function
 *      in SM750, set edgeDetect flag to 0
 *
 *  Input:
 *      x               - X Coordinate of the video window
 *      y               - Y Coordinate of the video window
 *      srcWidth        - The source video width
 *      srcHeight       - The source video height
 *      dstWidth        - The destination video width
 *      dstHeight       - The destination video height
 *      doubleBuffer    - Double Buffer enable flag
 *      srcAddress0     - The source of the video buffer 0 to display
 *      srcAddress1     - The source of the video buffer 1 to display
 *                        (only for double buffering).
 *      srcPitch        - The source video plane pitch in bytes
 *      srcLineOffset   - The source video plane line offset in bytes.
 *                        In normal usage, set it the same as the srcBufferPitch
 *      videoFormat     - Source video format
 *      edgeDetect      - Edge Detection enable flag (can only works with vertical upscaling)
 *                              0 - Disable
 *                              1 - Always Enable (alwasy enabled regardless horizontal scaling condition)
 *                              2 - Auto Enable (only enabled when no horizontal shrink)
 *      edgeDetectValue - Edge Detection value
 *
 *  Output:
 *      0   - Success
 *     -1  - Fail
 */
unsigned char ddk770_videoSetupEx(
	unsigned dispCtrl,
    unsigned long x,                /* X Coordinate of the video window */
    unsigned long y,                /* Y Coordinate of the video window */
    unsigned long srcWidth,         /* The source video width */
    unsigned long srcHeight,        /* The source video height */
    unsigned long dstWidth,         /* The destination video width */
    unsigned long dstHeight,        /* The destination video height */
    unsigned long yuvprefetch,     /* YUV Prefetch enable flag */
    unsigned long srcAddress0,      /* The source of the video buffer 0 to display */
    unsigned long sUAddress,            /* U Source Base Address (not used in RGB Space) */
    unsigned long sVAddress,            /* V Source Base Address (not used in RGB Space) */
    unsigned long sUVPitch,             /* UV plane pitch value in bytes (not used in */ 
    unsigned long srcPitch,         /* The source video plane pitch in bytes */
    unsigned long srcLineOffset,    /* The source video plane line offset in bytes.
                                       Set it the same as srcPitch in normal
                                       usage. */
    video_format_t videoFormat,      /* Source video format */
    unsigned long edgeDetect,       /* Edge Detection enable flag */
    unsigned long edgeDetectValue   /* Edge Detection value. SM718 only use bit 9 to 0 */
)
{
    unsigned long enableEdgeDetect;
    /* Save the source video width and height */
    gSrcVideoWidth = srcWidth;
    gSrcVideoHeight = srcHeight;

	
    /* Disable the video plane first */
    videoSetCtrl(dispCtrl, VIDEO_OFF);
    
    /* Set the video position */
    ddk770_videoSetPosition(dispCtrl, x, y);
    
    /* Set the scale factor */
    ddk770_videoScale(dispCtrl, srcWidth, srcHeight, dstWidth, dstHeight);
    
    /* Set the video format */
    videoSetFormat(dispCtrl, videoFormat);


	/* Enable 4K Prefetch for bandwidth */

    if ((videoFormat == FORMAT_RGB888) || (videoFormat == FORMAT_RGB565) || (videoFormat == FORMAT_YUV444))
        ddk770_videoYUVPrefetch(dispCtrl,0);
    else
        ddk770_videoYUVPrefetch(dispCtrl,1);
    
    /* Set the buffer pitch */
    ddk770_videoSetPitchOffset(dispCtrl, srcPitch, srcLineOffset);
    /* Set the UV buffer pitch */
    ddk770_videoSetUVPitchOffset(dispCtrl, sUVPitch, sUVPitch);
    
    /* Enable double buffer */
//    videoEnableDoubleBuffer(doubleBuffer);
    
    /* Set the video buffer 0 and 1 */
    ddk770_videoSetBuffer(dispCtrl, 0, srcAddress0);
//    ddk770_videoSetBuffer(dispCtrl, 1, srcAddress1);
   
    /* Set the video buffer U and V */
    ddk770_videoSetUVBuffer(dispCtrl, sUAddress, sVAddress);
        
    /* Set the destination video window */
    ddk770_videoSetWindowSize(dispCtrl, dstWidth, dstHeight);

    /* Set the last line */
    ddk770_videoSetLast(dispCtrl, srcWidth, srcHeight);
    
    /* Set the edge detection enable bit and its value (if applicable) */
    if (edgeDetect == 0)
        enableEdgeDetect = 0;
    else if (edgeDetect == 1)
        enableEdgeDetect = 1;
    else
    {
        /* Only enable the edgeDetection when scaling up vertically and no 
           shrinking on the horizontal. */
        if ((dstHeight > srcHeight) && (dstWidth >= srcWidth))
            enableEdgeDetect = 1;
        else
            enableEdgeDetect = 0;
    }

    return 0;
}

/*
 *  ddk770_videoSetup
 *      This function setups the video.
 *
 *  Input:
 *      x               - X Coordinate of the video window
 *      y               - Y Coordinate of the video window
 *      srcWidth        - The source video width
 *      srcHeight       - The source video height
 *      dstWidth        - The destination video width
 *      dstHeight       - The destination video height
 *      doubleBuffer    - Double Buffer enable flag
 *      srcAddress0     - The source of the video buffer 0 to display
 *      srcAddress1     - The source of the video buffer 1 to display
 *                        (only for double buffering).
 *      srcPitch        - The source video plane pitch in bytes
 *      srcLineOffset   - The source video plane line offset in bytes.
 *                        In normal usage, set it the same as the srcBufferPitch
 *      videoFormat     - Source video format
 *
 *  Output:
 *      0   - Success
 *     -1  - Fail
 */
unsigned char ddk770_videoSetup(
    disp_control_t dispCtrl,
    unsigned long x,                /* X Coordinate of the video window */
    unsigned long y,                /* Y Coordinate of the video window */
    unsigned long srcWidth,         /* The source video width */
    unsigned long srcHeight,        /* The source video height */
    unsigned long dstWidth,         /* The destination video width */
    unsigned long dstHeight,        /* The destination video height */
    unsigned long yuvprefetch,     /* Double Buffer enable flag */
    unsigned long srcAddress0,      /* The source of the video buffer 0 to display */
    unsigned long srcAddress1,      /* The source of the video buffer 1 to display
                                       (only for double buffering).
                                     */
    unsigned long srcPitch,         /* The source video plane pitch in bytes */
    unsigned long srcLineOffset,    /* The source video plane line offset in bytes.
                                       Set it the same as srcPitch in normal
                                       usage. */
    video_format_t videoFormat      /* Source video format */
)
{
    return ddk770_videoSetupEx(dispCtrl, x, y, srcWidth, srcHeight, dstWidth, dstHeight, yuvprefetch, 
                        srcAddress0, 0, 0, 0,srcPitch, srcLineOffset, videoFormat,
                        0, 0);
    
}

/*
 *  ddk770_startVideo
 *      This function starts the video.
 */
void ddk770_startVideo( 
unsigned dispCtrl
)
{
    /* Enable the video plane */
    videoSetCtrl(dispCtrl, VIDEO_ON);
}

/*
 *  ddk770_stopVideo
 *      This function stops the video.
 */
void ddk770_stopVideo(unsigned dispCtrl)
{
    /* Just disable the video plane */
    videoSetCtrl(dispCtrl, VIDEO_OFF);
}


// Called by other function to get overlay's offset value in performance mode.
// If return 0, it means performance mode disable.
unsigned long videoPerformanceModeStride()
{
	return gPerfModeStride;
}

//Return overlay YUV buffers location of performance mode.
void videoPerformanceModeBuf(
unsigned long *Cy, unsigned long *Cr, unsigned long *Cb,
unsigned long sX, unsigned long sY)
{
   unsigned long offset;
   offset = sY * gPerfModeStride;

	*Cy = CYADDR + offset + sX;
	*Cr = CBADDR + (offset >> 1 ) + (sX >> 1);
	*Cb = CRADDR + (offset >> 1 ) + (sX >> 1);
}

//Cheok: Performance mode
//This functon sets up performance mode for JPU and overlay
//The location for YUV stream in DDR should be pre-calculate during SW architect
//In this DDK example, we set it at 96M, 110M and 120M
void videoPerformanceModeEnable(logicalMode_t *pLogicalMode)
{
    unsigned long dcOffset, bufOffset, yAddr, uAddr, vAddr, videoCtrl;

    dcOffset = pLogicalMode->dispCtrl> 1? DC_OFFSET2 : pLogicalMode->dispCtrl * DC_OFFSET;

    yAddr = CYADDR; //CyAddr
    uAddr = CRADDR; //CbAddr
    vAddr = CBADDR; //CrAddr

    videoCtrl = peekRegisterDWord(VIDEO_DISPLAY_CTRL+dcOffset);

    if (pLogicalMode->x <= PMODE_2K)
    {
        bufOffset = gPerfModeStride = PMODE_2K;

        //Set JPU to 2K performance mode
        pokeRegisterDWord(JPU_PERFORMANCE_MODE,
          FIELD_SET(0, JPU_PERFORMANCE_MODE, JPU0, HD)
        | FIELD_SET(0, JPU_PERFORMANCE_MODE, JPU1, HD));

        //Set overlay to 2K performance mode
        videoCtrl = FIELD_SET(videoCtrl, VIDEO_DISPLAY_CTRL, JPUP, HD);
    }
    else
    {
        bufOffset = gPerfModeStride = PMODE_4K;

        //Set JPU to 4K performance mode
        pokeRegisterDWord(JPU_PERFORMANCE_MODE,
          FIELD_SET(0, JPU_PERFORMANCE_MODE, JPU0, UHD)
        | FIELD_SET(0, JPU_PERFORMANCE_MODE, JPU1, UHD));

        //Set overlay to 4K performance mode
        videoCtrl = FIELD_SET(videoCtrl, VIDEO_DISPLAY_CTRL, JPUP, UHD);
    }

    pokeRegisterDWord(VIDEO_FB_ADDRESS  +dcOffset, yAddr);
    pokeRegisterDWord(VIDEO_FB_WIDTH    +dcOffset, (pLogicalMode->x << 16 | bufOffset));
    pokeRegisterDWord(VIDEO_FB_ADDRESS_U+dcOffset, uAddr);
    pokeRegisterDWord(VIDEO_FB_WIDTH_U  +dcOffset, (pLogicalMode->x << 16 | bufOffset));
    pokeRegisterDWord(VIDEO_FB_ADDRESS_V+dcOffset, vAddr);
    pokeRegisterDWord(VIDEO_FB_WIDTH_V  +dcOffset, (pLogicalMode->x << 16 | bufOffset));
    pokeRegisterDWord(VIDEO_PLANE_TL    +dcOffset, 0);
    pokeRegisterDWord(VIDEO_PLANE_BR    +dcOffset, ((pLogicalMode->y) << 16 | pLogicalMode->x));
    pokeRegisterDWord(VIDEO_SCALE       +dcOffset, 0x0ffc0ffc);

    pokeRegisterDWord(VIDEO_DISPLAY_CTRL+dcOffset,
      videoCtrl
    | FIELD_SET(0, VIDEO_DISPLAY_CTRL, PLANE, ENABLE)
    | FIELD_SET(0, VIDEO_DISPLAY_CTRL, FORMAT, YUV420));
}

void videoPerformanceModeDisable(logicalMode_t *pLogicalMode)
{
    unsigned long dcOffset, videoCtrl;

    gPerfModeStride = 0;

    pokeRegisterDWord(JPU_PERFORMANCE_MODE,
      FIELD_SET(0, JPU_PERFORMANCE_MODE, JPU0, DISABLE)
    | FIELD_SET(0, JPU_PERFORMANCE_MODE, JPU1, DISABLE));

    dcOffset = (pLogicalMode->dispCtrl > 1)? DC_OFFSET2 : pLogicalMode->dispCtrl * DC_OFFSET;
    videoCtrl = peekRegisterDWord(VIDEO_DISPLAY_CTRL+dcOffset);
    videoCtrl = FIELD_SET(videoCtrl, VIDEO_DISPLAY_CTRL, JPUP, DISABLE);
    pokeRegisterDWord(VIDEO_DISPLAY_CTRL+dcOffset, videoCtrl);
}

