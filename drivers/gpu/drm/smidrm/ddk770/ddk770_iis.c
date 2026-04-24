/*******************************************************************
* 
*         Copyright (c) 2014 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  This file contains the definitions for the IIS functions.
* 
*******************************************************************/
#include "ddk770_reg.h"
#include "ddk770_clock.h"
#include "ddk770_power.h"
#include "ddk770_helper.h"
#include "ddk770_chip.h"
#include "ddk770_iis.h"
#include "ddk770_gpio.h"
#include <linux/delay.h>	
#include "ddk770_help.h"


void ddk770_iis_Init(void)
{
    unsigned int val;
	
    printk("IIS Init\n");
	
    // phy soft reset 
    val = FIELD_SET(peekRegisterDWord(DP_BASE + DP_TOP1C), DP_TOP1C, REG_RESET_PHY, TRUE); // set reg0x001c[1] to 1
    pokeRegisterDWord(DP_BASE + DP_TOP1C, val);

    usleep_range(50000 , 50100);
    val = FIELD_SET(peekRegisterDWord(DP_BASE + DP_TOP1C), DP_TOP1C, REG_RESET_PHY, FALSE); // set reg0x001c[1] to 0
    pokeRegisterDWord(DP_BASE + DP_TOP1C, val);

	
    // pixl pll
    pokeRegisterDWord(DP_BASE + DP_ANALOG190, 0x10530);
    pokeRegisterDWord(DP_BASE + DP_ANALOG194, 0x1050005);
}


/*
 * Set up I2S and GPIO registers to transmit/receive data.
 */
void ddk770_iisOpen(
   unsigned long wordLength, //Number of bits in IIS data: 16 bit, 24 bit, 32 bit
   unsigned long sampleRate  //Sampling rate.
)
{
    unsigned long gpioPin, clockDivider;
    unsigned char ws;

    ddk770_enableI2S(1); //Turn on I2S clock

    /* Configure GPIO Mux for IIS output */
    gpioPin = peekRegisterDWord(GPIO_MUX);

    /* Select IIS Pins */
    pokeRegisterDWord(GPIO_MUX,                                            
        FIELD_SET(gpioPin, GPIO_MUX, 9, I2S_RX) |                          
        FIELD_SET(gpioPin, GPIO_MUX, 8, I2S_TX) |       
        FIELD_SET(gpioPin, GPIO_MUX, 7, I2S_WS) |                          
        FIELD_SET(gpioPin, GPIO_MUX, 6, I2S_CK));


    /* Make sure GPIO9(IIS RX) data direction is input */

	ddk770_gpioMode(9,1);
	

    /* IIS register set up */
    pokeRegisterDWord(I2S_TX_DATA_L, 0); //Clear Tx registers
    pokeRegisterDWord(I2S_TX_DATA_R, 0);    

    //Figure out Word Select value
    switch (wordLength)
    {
        case 32:
            ws = 2;
            break;
        case 24:
            ws = 1;
            break;
        default:
            ws = 0;
    }

	clockDivider = (24576000)/(4*sampleRate*wordLength) - 1;

    pokeRegisterDWord(I2S_CTRL, 
          FIELD_VALUE(0, I2S_CTRL, CS, ws)
        | FIELD_VALUE(0, I2S_CTRL , CDIV, clockDivider));

    pokeRegisterDWord(I2S_SRAM_DMA, 0);  //Default no DMA. Call another function to set up DMA
}

/*
 *    Turn off I2S and close GPIO 
 */
void ddk770_iisClose(void)
{
    unsigned long gpioPin;

    gpioPin = peekRegisterDWord(GPIO_MUX);
    pokeRegisterDWord(GPIO_MUX,                                            
        FIELD_SET(gpioPin, GPIO_MUX, 9, GPIO) |                          
        FIELD_SET(gpioPin, GPIO_MUX, 8, GPIO) |     
        FIELD_SET(gpioPin, GPIO_MUX, 7, GPIO) |                          
        FIELD_SET(gpioPin, GPIO_MUX, 6, GPIO));

    pokeRegisterDWord(I2S_TX_DATA_L, 0); //Clear Tx registers
    pokeRegisterDWord(I2S_TX_DATA_R, 0);    
    pokeRegisterDWord(I2S_STATUS, 0);    //Disable Tx line out
    pokeRegisterDWord(I2S_CTRL, 0);      //Clear clock setting.
    pokeRegisterDWord(I2S_SRAM_DMA, 0);  //Clear DMA setting.

    ddk770_enableI2S(0); //Turn off I2S clock
}




/*
 *  This function set up I2S to DMA data from SRAM.
 *
 *  SRAM area has max size of 2048 bytes (or 512 DWords).
 *  Max size of each I2S DMA session is 256 DWords.
 *
 *  Inputs: 
 *        offset address in SRAM to start DMA (DWord aligned)
 *        Number of bytes to DMA (DWord aligned)
 */
void ddk770_iisTxDmaSetup(
    unsigned long offset, /* Offset from start of SRAM to start DMA */
    unsigned long len     /* Number of bytes to DMA */
    )
{
    unsigned long dmaPointer;

    offset >>= 2; //I2S DMA register requires offset to be expressed in DWord
    len >>= 2;
    len--; //I2S DMA register requires length to be expressed as DWord - 1.

    dmaPointer = FIELD_VAL_GET(peekRegisterDWord(I2S_SRAM_DMA), I2S_SRAM_DMA, ADDRESS);

    //If DMA pointer already at the requested offset. Just set up the length.
    if (dmaPointer == offset)
    {
        pokeRegisterDWord(I2S_SRAM_DMA,
          FIELD_SET(0, I2S_SRAM_DMA, STATE, ENABLE)
        | FIELD_VALUE(0, I2S_SRAM_DMA, SIZE, len)
        | FIELD_VALUE(0, I2S_SRAM_DMA, ADDRESS, offset));

        return;
    }

    //Position DMA pointer to the new base pointer (or offset).
    //Note that DMA reload base pointer only when it gets to end of SRAM.
    //Therefore, we need to advance DMA from current position to the end.
    pokeRegisterDWord(I2S_SRAM_DMA,
          FIELD_SET(0, I2S_SRAM_DMA, STATE, ENABLE)
        | FIELD_VALUE(0, I2S_SRAM_DMA, SIZE, (0x1FF - dmaPointer))
        | FIELD_VALUE(0, I2S_SRAM_DMA, ADDRESS, dmaPointer));

    ddk770_iisStartNoTx();//Start DMA without output the old data from Tx line.

    //Once DMA starts, make the new base pointer ready.
    pokeRegisterDWord(I2S_SRAM_DMA,
          FIELD_SET(0, I2S_SRAM_DMA, STATE, ENABLE)
        | FIELD_VALUE(0, I2S_SRAM_DMA, SIZE, len)
        | FIELD_VALUE(0, I2S_SRAM_DMA, ADDRESS, offset));

    // When DMA get to the end of SRAM, it loads the new base pointer.
    do
    {
      dmaPointer = FIELD_VAL_GET(peekRegisterDWord(I2S_SRAM_DMA), I2S_SRAM_DMA, ADDRESS);
    } while(dmaPointer != offset);

    ddk770_iisStop();
}

/*
 * Return current IIS DMA position.
 */
unsigned long ddk770_iisDmaPointer(void)
{
    return(FIELD_VAL_GET(peekRegisterDWord(I2S_SRAM_DMA), I2S_SRAM_DMA, ADDRESS));
}

/*
 * This function start IIS without enabling Tx line.
 * It can be used to flush left over SRAM data without
 * sending them to Codec.
 */
void ddk770_iisStartNoTx(void)
{
    unsigned long value;

    value = FIELD_SET(peekRegisterDWord(I2S_CTRL), I2S_CTRL, MODE, MASTER);
    pokeRegisterDWord(I2S_CTRL, value);
}

/*
 * This function is needed only when I2S is intended to operate in master mode.
 *
 * For slave mode, just use iisOpen() is enough, because I2S will start
 * functioning as soon as an external clock is detected after iisOpen().
 *
 */

void ddk770_iisStart(void)
{
    unsigned long value;

    pokeRegisterDWord(I2S_STATUS, FIELD_SET(0, I2S_STATUS, TX, ENABLE));  //Enable Tx line out

    value = FIELD_SET(peekRegisterDWord(I2S_CTRL), I2S_CTRL, MODE, MASTER);
    pokeRegisterDWord(I2S_CTRL, value);
}

/*
 * This function is useful only when I2S is operating in master mode.
 *
 * For slave mode, clock is external and cannot be stopped by IIS
 * control register.
 *
 */
void ddk770_iisStop(void)
{
    unsigned long value;

    value = FIELD_SET(peekRegisterDWord(I2S_CTRL), I2S_CTRL, MODE, SLAVE);
    pokeRegisterDWord(I2S_CTRL, value);

    pokeRegisterDWord(I2S_STATUS, FIELD_SET(0, I2S_STATUS, TX, DISABLE));  //Disable Tx line out

}


/*
 * Set values for left Tx and right Tx register.
 */
void ddk770_iisSetTx(
    unsigned long left, //Data for left channel Tx
    unsigned long right //Data for right channel Tx
    )
{
    pokeRegisterDWord(I2S_TX_DATA_L, left);
    pokeRegisterDWord(I2S_TX_DATA_R, right);
}


/*
 * This function clears the RAW interrupt status of I2S.
 * 
 * When I2S completes sending data, the raw interrupt bit will be set.
 * It has to be cleared, in order to distinguish between different sessions of countdown.
 * 
 */
void ddk770_iisClearRawInt(void)
{
    /* Read I2S Control & TX to clear INT when IIS get data from Tx & Rx. */
    peekRegisterDWord(I2S_STATUS);

    /* Write 0 to I2S SRAM DMA status when IIS get data from SRAM */
    pokeRegisterDWord(I2S_SRAM_DMA_STATUS, 0);
}

/* 
 * This function returns the INT mask for IIS.
 *
 */
unsigned long ddk770_iisIntMask(void)
{
    unsigned long mask = 0;

    mask |= FIELD_SET(0, INT_MASK, I2S, ENABLE);

    return mask;
}


