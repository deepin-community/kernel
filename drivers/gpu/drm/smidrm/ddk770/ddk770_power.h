/*******************************************************************
* 
*         Copyright (c) 2007 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  power.h --- Voyager GX SDK 
*  This file contains the definitions for the power functions.
* 
*******************************************************************/
#ifndef _DDK770_POWER_H_
#define _DDK770_POWER_H_

/*
 *  Enable/disable jpeg decoder 1.
 */
void ddk770_enableJPU1(unsigned long enable);


void ddk770_enablePCIE(unsigned int enable);


/*
 *  This function enable/disable HDMI
 */
void ddk770_enableHDMI(unsigned long enable);


/*
 *  Enable/disable USB 3 device
 */
void ddk770_enableUsbDevice(unsigned long enable);

/* 
 * This function enable/disable the ZV Port.
 */
void ddk770_enableZVPort(unsigned long enable);

/*
 *  Enable/disable jpeg decoder.
 */
void ddk770_enableJPU(unsigned long enable);

/*
 *    Enable/disable H264 video decoder.
 */ 
void ddk770_enableVPU(unsigned long enable);

/* 
 * This function enable/disable the 2D engine.
 */
void ddk770_enable2DEngine(unsigned long enable);

/* 
 * This function enable/disable the DMA Engine
 */
void ddk770_enableDMA(unsigned long enable);


void ddk770_enableDMA1(unsigned long enable);


/*
 *    Enable/disable UART
 */ 
void ddk770_enableUART(unsigned long enable);

/*
 *    Enable/disable I2S
 */ 
void ddk770_enableI2S(unsigned long enable);

/* 
 * This function enable/disable the SSP.
 */
void ddk770_enableSSP(unsigned long enable);

/*
 *    Enable/disable display control 2
 */ 
void ddk770_enableDC2(unsigned long enable);

/*
 *    Enable/disable display control 1
 */ 
void ddk770_enableDC1(unsigned long enable);

/*
 *    Enable/disable display control 0
 */ 
void ddk770_enableDC0(unsigned long enable);

/*
 *    Enable/disable ARM
 */ 
void ddk770_enableARM(unsigned long enable);

#endif /* _POWER_H_ */
