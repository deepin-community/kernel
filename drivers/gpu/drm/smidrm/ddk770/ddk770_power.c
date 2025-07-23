/*******************************************************************
* 
*         Copyright (c) 2007 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  power.c --- Voyager GX SDK 
*  This file contains the source code for the power functions.
* 
*******************************************************************/
#include "ddk770_reg.h"
#include "ddk770_hardware.h"
#include "ddk770_chip.h"
#include "ddk770_power.h"
#include "ddk770_help.h"
#include "ddk770_ddkdebug.h"

/*
 *  Enable/disable jpeg decoder 1.
 */
void ddk770_enableJPU1(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, JPU1, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, JPU1, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}

/*
 *  This function enable/disable HDMI
 */
void ddk770_enableHDMI(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, HDMI, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, HDMI, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}


/*
 *  Enable/disable USB 3 device
 */
void ddk770_enableUsbDevice(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, USBS, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, USBS, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}



/*
 *  Enable/disable jpeg decoder.
 */
void ddk770_enableJPU(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, JPU, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, JPU, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}



void ddk770_enablePCIE(unsigned int enable)
{


	unsigned int regValue;

	regValue = peekRegisterDWord(CLOCK1_ENABLE);
	if (enable)
	{
		regValue = FIELD_SET(regValue, CLOCK1_ENABLE, PCIER, NORMAL);
		regValue = FIELD_SET(regValue, CLOCK1_ENABLE, PCIE, ON);
	}
	else
	{
		regValue = FIELD_SET(regValue, CLOCK1_ENABLE, PCIER, RESET);
		regValue = FIELD_SET(regValue, CLOCK1_ENABLE, PCIE, OFF);
	}
	pokeRegisterDWord(CLOCK1_ENABLE, regValue);

}



/*
 *    Enable/disable H264 video decoder.
 */ 
void ddk770_enableVPU(unsigned long enable)
{
    unsigned int regValue;

	regValue = peekRegisterDWord(0x1060); //set VPU's sram goes to sleep  mode
	if (enable)
		regValue = regValue & (~0x1U);
	else
		regValue = regValue | 0x01;

	pokeRegisterDWord(0x1060, regValue);
	

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable){
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, VPU, ON);
		regValue = FIELD_SET(regValue, CLOCK_ENABLE, VPUR, NORMAL);
    }
	else{
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, VPU, OFF);
		regValue = FIELD_SET(regValue, CLOCK_ENABLE, VPUR, RESET);
	}
    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}


/* 
 * This function enable/disable the 2D engine.
 */
void ddk770_enable2DEngine(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, DE, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, DE, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}

/* 
 * This function enable/disable the DMA Engine
 */
void ddk770_enableDMA(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, DMA, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, DMA, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}


/* 
 * This function enable/disable the DMA1 and DMA2 Engine
 */
void ddk770_enableDMA1(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, DMA1, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, DMA1, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}



/*
 *    Enable/disable UART
 */ 
void ddk770_enableUART(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, UART, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, UART, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}

/*
 *    Enable/disable I2S
 */ 
void ddk770_enableI2S(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, I2S, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, I2S, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}

/* 
 * This function enable/disable the SSP.
 */
void ddk770_enableSSP(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, SSP, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, SSP, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}


/*
 *    Enable/disable display control 2
 */ 
void ddk770_enableDC2(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, DC2, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, DC2, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}

/*
 *    Enable/disable display control 1
 */ 
void ddk770_enableDC1(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, DC1, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, DC1, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}

/*
 *    Enable/disable display control 0
 */ 
void ddk770_enableDC0(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, DC0, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, DC0, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}

/*
 *    Enable/disable ARM
 */ 
void ddk770_enableARM(unsigned long enable)
{
    unsigned long regValue;

    regValue = peekRegisterDWord(CLOCK_ENABLE);
    if (enable)
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, ARM, ON);
    else
        regValue = FIELD_SET(regValue, CLOCK_ENABLE, ARM, OFF);

    pokeRegisterDWord(CLOCK_ENABLE, regValue);
}

