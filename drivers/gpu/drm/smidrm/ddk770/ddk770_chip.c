/*******************************************************************
* 
*         Copyright (c) 2007 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  CHIP.C --- SMI DDK 
*  This file contains the source code for the mode table.
* 
*******************************************************************/
#include "ddk770_reg.h"
#include "ddk770_chip.h"
#include "ddk770_hardware.h"
#include "ddk770_helper.h"
#include "ddk770_power.h"
#include "ddk770_clock.h"
#include "ddk770_help.h"
#include "ddk770_ddkdebug.h"
#include "ddk770_display.h"


/*
 * This function returns frame buffer memory size in Byte units.
 */
unsigned long ddk770_getFrameBufSize(void)
{

#if 1
	unsigned long strapPin; 
	unsigned long rValue;

	strapPin = (FIELD_VAL_GET(peekRegisterDWord(STRAP_PINS2), STRAP_PINS2, MEM_SIZE) << 1 ) | FIELD_VAL_GET(peekRegisterDWord(STRAP_PINS1), STRAP_PINS1, MEM_SIZE) ;

	switch(strapPin)
	{
		case 0x02:
		rValue = MB(512);
		break;
		case 0x01:
		rValue = MB(1024);
		break;
		case 0x00:
		rValue = MB(2048);
		break;
		case 0x03:
		default: /* default size of 512MB. Don't need to do anything */
		rValue = MB(256);
		break;
	}
	return(rValue);
#else
	return SMI_MEMORY_SIZE_SM770;
#endif

}



/*
 * This function returns the logical chip type defined in chip.h.
 * Currently, it's either SM768 or unknown.
 */
logical_chip_type_t ddk770_getChipType(void)
{
    logical_chip_type_t chip;

	chip = SM770;

    return chip;
}

/*
 * Return a char string name of the current chip.
 * It's convenient for application need to display the chip name.
 */
char *ddk770_getChipTypeString(void)
{
    char * chipName;

    switch(ddk770_getChipType())
    {
        case SM768:
            chipName = "SM770";
            break;
        default:
            chipName = "Unknown";
            break;
    }

    return chipName;
}

/*
 * Initialize a single chip and environment according to input parameters.
 * Use this function when don't want to use default setting of ddk770_initChip().
 *
 * Input: initchip_param_t structure.
 *
 * Return: 0 (or NO_ERROR) if successful.
 *        -1 if fail.
 *
 */
long ddk770_initChipParamEx(initchip_param_t * pInitParam)
{
    unsigned long ulReg;

    /* Check if we know this chip */
    if (ddk770_getChipType() == SM_UNKNOWN){
        return -1;
    }
      
	if(ddk770_GetChipRev() == CHIP_REV_AB)
		pokeRegisterDWord(0x130, 0x78000000);

    if (pInitParam->setAllEngOff == 1)
    {
        ulReg = peekRegisterDWord(VIDEO_DISPLAY_CTRL);
        ulReg = FIELD_SET(ulReg, VIDEO_DISPLAY_CTRL, PLANE, DISABLE); 
        ulReg = FIELD_SET(ulReg, VIDEO_DISPLAY_CTRL, JPUP, DISABLE);
        pokeRegisterDWord(VIDEO_DISPLAY_CTRL, ulReg); /* Channel 0 */
        pokeRegisterDWord(VIDEO_DISPLAY_CTRL+CHANNEL_OFFSET, ulReg); /* Channel 1 */
		pokeRegisterDWord(VIDEO_DISPLAY_CTRL+CHANNEL_OFFSET2, ulReg); /* Channel 2 */

        /* Disable alpha plane, if a former application left it on */
        ulReg = peekRegisterDWord(ALPHA_DISPLAY_CTRL);
        ulReg = FIELD_SET(ulReg, ALPHA_DISPLAY_CTRL, PLANE, DISABLE); 
        pokeRegisterDWord(ALPHA_DISPLAY_CTRL, ulReg); /* Channel 0 */
        pokeRegisterDWord(ALPHA_DISPLAY_CTRL+CHANNEL_OFFSET, ulReg); /* Channel 1 */
		pokeRegisterDWord(ALPHA_DISPLAY_CTRL+CHANNEL_OFFSET2, ulReg); /* Channel 2 */

        /* Disable hardware cursor, if a former application left it on */
        ulReg = peekRegisterDWord(HWC_CONTROL);
        ulReg = FIELD_SET(ulReg, HWC_CONTROL, MODE, DISABLE); 
        pokeRegisterDWord(HWC_CONTROL, ulReg); /* Channel 0 */
        pokeRegisterDWord(HWC_CONTROL+CHANNEL_OFFSET, ulReg); /* Channel 1 */
		pokeRegisterDWord(HWC_CONTROL+CHANNEL_OFFSET2, ulReg); /* Channel 2 */

		clearDCOutput();
    }

    /* We can add more initialization as needed. */

        
    return 0;
}

/*
 * Initialize the chip with default parameters.
 *
 * Input:
 *        argc:
 *        argv: memory and register delay what we want.
 *
 * Return: 0 (or NO_ERROR) if successful.
 *        -1 if fail.
 */
long ddk770_initChip(void)
{
    initchip_param_t initParam;
    
    /* Initialize the chip with some default parameters */

    initParam.setAllEngOff = 1;
    
    
    return(ddk770_initChipParamEx(&initParam));

}

/*
    We may not need this function in the future.
    Just during validation, we want the code able to work
    dynamically in both FPGA and ASIC.
*/
unsigned long isFPGA(void)
{

    return 0;

}


unsigned int ddk770_GetChipRev(void)
{

	return peekRegisterDWord(CHIP_REV);
}




