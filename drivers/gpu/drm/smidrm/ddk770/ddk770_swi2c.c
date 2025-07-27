/*******************************************************************
* 
*         Copyright (c) 2007 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  swi2c.c --- SM750/SM718 DDK 
*  This file contains the source code for I2C using software
*  implementation.
* 
*******************************************************************/
#include "ddk770_reg.h"
#include "ddk770_os.h"
#include "ddk770_chip.h"
#include "ddk770_power.h"
#include "ddk770_swi2c.h"
#include "ddk770_timer.h"
#include "ddk770_gpio.h"
#include "ddk770_help.h"

/*******************************************************************
 * I2C Software Master Driver:   
 * ===========================
 * Each i2c cycle is split into 4 sections. Each of these section marks
 * a point in time where the SCL or SDA may be changed. 
 * 
 * 1 Cycle == |  Section I. |  Section 2. |  Section 3. |  Section 4. |
 *            +-------------+-------------+-------------+-------------+
 *            | SCL set LOW |SCL no change| SCL set HIGH|SCL no change|
 *                 
 *                                          ____________ _____________
 * SCL == XXXX _____________ ____________ /
 *                 
 * I.e. the SCL may only be changed in section 1. and section 3. while
 * the SDA may only be changed in section 2. and section 4. The table
 * below gives the changes for these 2 lines in the varios sections.
 * 
 * Section changes Table:        
 * ======================
 * blank = no change, L = set bit LOW, H = set bit HIGH
 *                       
 *                                | 1.| 2.| 3.| 4.|      
 *                 ---------------+---+---+---+---+      
 *                 Tx Start   SDA |   | H |   | L |      
 *                            SCL | L |   | H |   |      
 *                 ---------------+---+---+---+---+                
 *                 Tx Stop    SDA |   | L |   | H |      
 *                            SCL | L |   | H |   |      
 *                 ---------------+---+---+---+---+                
 *                 Tx bit H   SDA |   | H |   |   |      
 *                            SCL | L |   | H |   |      
 *                 ---------------+---+---+---+---+                
 *                 Tx bit L   SDA |   | L |   |   |      
 *                            SCL | L |   | H |   |      
 *                 ---------------+---+---+---+---+                
 *                                  
 ******************************************************************/

/* GPIO pins used for this I2C. It ranges from 0 to 31. */
static unsigned char g_i2cClockGPIO = DEFAULT_I2C0_SCL;
static unsigned char g_i2cDataGPIO = DEFAULT_I2C0_SDA;

/*
 *  Below is the variable declaration for the GPIO pin register usage
 *  for the i2c Clock and i2c Data.
 *
 *  Note:
 *      Notice that the GPIO usage for the i2c clock and i2c Data are
 *      separated. This is to make this code flexible enough when 
 *      two separate GPIO pins for the clock and data are located
 *      in two different GPIO register set (worst case).
 */

/* i2c Clock GPIO Register usage */
static unsigned long g_i2cClkGPIOMuxReg = GPIO_MUX;
static unsigned long g_i2cClkGPIODataReg = GPIO_DATA;

/* i2c Data GPIO Register usage */
static unsigned long g_i2cDataGPIOMuxReg = GPIO_MUX;
static unsigned long g_i2cDataGPIODataReg = GPIO_DATA;


/*
 *  This function puts a delay between command
 */        
static void ddk770_swI2CWait(void)
{
    //SM770 has build-in timer. Use it instead of SW loop.
    ddk770_timerWaitTicks(3, 0x3ff);
}

/*
 *  This function set/reset the SCL GPIO pin
 *
 *  Parameters:
 *      value    - Bit value to set to the SCL or SDA (0 = low, 1 = high)
 *
 *  Notes:
 *      When setting SCL to high, just set the GPIO as input where the pull up
 *      resistor will pull the signal up. Do not use software to pull up the
 *      signal because the i2c will fail when other device try to drive the
 *      signal due to SM50x will drive the signal to always high.
 */ 
void ddk770_swI2CSCL(unsigned char value)
{

	ddk770_gpioMode(g_i2cClockGPIO,0);

	ddk770_gpioWrite(g_i2cClockGPIO,value);

}

/*
 *  This function set/reset the SDA GPIO pin
 *
 *  Parameters:
 *      value    - Bit value to set to the SCL or SDA (0 = low, 1 = high)
 *
 *  Notes:
 *      When setting SCL to high, just set the GPIO as input where the pull up
 *      resistor will pull the signal up. Do not use software to pull up the
 *      signal because the i2c will fail when other device try to drive the
 *      signal due to SM50x will drive the signal to always high.
 */
void ddk770_swI2CSDA(unsigned char value)
{

	ddk770_gpioMode(g_i2cDataGPIO,0);

	ddk770_gpioWrite(g_i2cDataGPIO,value);

}

/*
 *  This function read the data from the SDA GPIO pin
 *
 *  Return Value:
 *      The SDA data bit sent by the Slave
 */
static unsigned char ddk770_swI2CReadSDA(void)
{
    //unsigned long ulGPIODirection;
    //unsigned long ulGPIOData;

	ddk770_gpioMode(g_i2cDataGPIO,1);

	return ddk770_gpioRead(g_i2cDataGPIO);
	
}

#ifdef _MSC_VER
#pragma optimize( "", off )
#endif

/*
 *  This function sends ACK signal
 */
static void ddk770_swI2CAck(void)
{
    return;  /* Single byte read is ok without it. */
}

/*
 *  This function sends the start command to the slave device
 */
static void ddk770_swI2CStart(void)
{
    /* Start I2C */
    ddk770_swI2CSDA(1);
    ddk770_swI2CSCL(1);
    ddk770_swI2CSDA(0);
}

/*
 *  This function sends the stop command to the slave device
 */
static void ddk770_swI2CStop(void)
{
    /* Stop the I2C */
    ddk770_swI2CSCL(1);
    ddk770_swI2CSDA(0);
    ddk770_swI2CSDA(1);
}

/*
 *  This function writes one byte to the slave device
 *
 *  Parameters:
 *      data    - Data to be write to the slave device
 *
 *  Return Value:
 *       0   - Success
 *      -1   - Fail to write byte
 */
static long ddk770_swI2CWriteByte(unsigned char data) 
{
    unsigned char value = data;
    int i;

    /* Sending the data bit by bit */
    for (i=0; i<8; i++)
    {
        /* Set SCL to low */
        ddk770_swI2CSCL(0);

        /* Send data bit */
        if ((value & 0x80) != 0)
            ddk770_swI2CSDA(1);
        else
            ddk770_swI2CSDA(0);

        ddk770_swI2CWait();

        /* Toggle clk line to one */
        ddk770_swI2CSCL(1);
        ddk770_swI2CWait();

        /* Shift byte to be sent */
        value = value << 1;
    }

    /* Set the SCL Low and SDA High (prepare to get input) */
    ddk770_swI2CSCL(0);
    ddk770_swI2CSDA(1);

    /* Set the SCL High for ack */
    ddk770_swI2CWait();
    ddk770_swI2CSCL(1);
    ddk770_swI2CWait();

    /* Read SDA, until SDA==0 */
    for(i=0; i<0xff; i++) 
    {
        if (!ddk770_swI2CReadSDA())
            break;

        ddk770_swI2CSCL(0);
        ddk770_swI2CWait();
        ddk770_swI2CSCL(1);
        ddk770_swI2CWait();
    }

    /* Set the SCL Low and SDA High */
    ddk770_swI2CSCL(0);
    ddk770_swI2CSDA(1);

    if (i<0xff)
        return 0;
    else
        return (-1);
}

/*
 *  This function reads one byte from the slave device
 *
 *  Parameters:
 *      ack    - Flag to indicate either to send the acknowledge
 *            message to the slave device or not
 *
 *  Return Value:
 *      One byte data read from the Slave device
 */
static unsigned char ddk770_swI2CReadByte(unsigned char ack)
{
    int i;
    unsigned char data = 0;

    for(i=7; i>=0; i--)
    {
        /* Set the SCL to Low and SDA to High (Input) */
        ddk770_swI2CSCL(0);
        ddk770_swI2CSDA(1);
        ddk770_swI2CWait();

        /* Set the SCL High */
        ddk770_swI2CSCL(1);
        ddk770_swI2CWait();

        /* Read data bits from SDA */
        data |= (ddk770_swI2CReadSDA() << i);
    }

    if (ack)
        ddk770_swI2CAck();

    /* Set the SCL Low and SDA High */
    ddk770_swI2CSCL(0);
    ddk770_swI2CSDA(1);

    return data;
}

#ifdef _MSC_VER
#pragma optimize( "", on )
#endif

/*
 * This function initializes the i2c attributes and bus
 *
 * Parameters:
 *      i2cClkGPIO      - The GPIO pin to be used as i2c SCL
 *      i2cDataGPIO     - The GPIO pin to be used as i2c SDA
 *
 * Return Value:
 *      -1   - Fail to initialize the i2c
 *       0   - Success
 */
long ddk770_swI2CInit(
    unsigned char i2cClkGPIO, 
    unsigned char i2cDataGPIO
)
{
    int i;
    
    /* Return 0 if the GPIO pins to be used is out of range. The range is only from [0..63] */
    if ((i2cClkGPIO > 60) || (i2cDataGPIO > 60))
        return (-1);

   
    /* Initialize the GPIO pin for the i2c Clock Register */
    g_i2cClkGPIOMuxReg = GPIO_MUX;   
    g_i2cClkGPIODataReg = GPIO_DATA;    
    
    /* Initialize the Clock GPIO Offset */
    g_i2cClockGPIO = i2cClkGPIO;
    
    /* Initialize the GPIO pin for the i2c Data Register */
    g_i2cDataGPIOMuxReg = GPIO_MUX;    
    g_i2cDataGPIODataReg = GPIO_DATA;    
    
    /* Initialize the Data GPIO Offset */
    g_i2cDataGPIO = i2cDataGPIO;

    /* Enable the GPIO pins for the i2c Clock and Data (GPIO MUX) */
    pokeRegisterDWord(g_i2cClkGPIOMuxReg, 
                      peekRegisterDWord(g_i2cClkGPIOMuxReg) & ~(1 << g_i2cClockGPIO));
    pokeRegisterDWord(g_i2cDataGPIOMuxReg, 
                      peekRegisterDWord(g_i2cDataGPIOMuxReg) & ~(1 << g_i2cDataGPIO));

    /* Clear the i2c lines. */
    for(i=0; i<9; i++) 
        ddk770_swI2CStop();

    return 0;
}

/*
 *  This function reads the slave device's register
 *
 *  Parameters:
 *      deviceAddress   - i2c Slave device address which register
 *                        to be read from
 *      registerIndex   - Slave device's register to be read
 *
 *  Return Value:
 *      Register value
 */
unsigned char ddk770_swI2CReadReg(
    unsigned char deviceAddress, 
    unsigned char registerIndex
)
{
    unsigned char data;

    /* Send the Start signal */
    ddk770_swI2CStart();

    /* Send the device address */
    ddk770_swI2CWriteByte(deviceAddress);                                                  

    /* Send the register index */
    ddk770_swI2CWriteByte(registerIndex);               

    /* Get the bus again and get the data from the device read address */
    ddk770_swI2CStart();
    ddk770_swI2CWriteByte(deviceAddress + 1);
    data = ddk770_swI2CReadByte(1);

    /* Stop swI2C and release the bus */
    ddk770_swI2CStop();

    return data;
}

/*
 *  This function writes a value to the slave device's register
 *
 *  Parameters:
 *      deviceAddress   - i2c Slave device address which register
 *                        to be written
 *      registerIndex   - Slave device's register to be written
 *      data            - Data to be written to the register
 *
 *  Result:
 *          0   - Success
 *         -1   - Fail
 */
long ddk770_swI2CWriteReg(
    unsigned char deviceAddress, 
    unsigned char registerIndex, 
    unsigned char data
)
{
    long returnValue = 0;
    
    /* Send the Start signal */
    ddk770_swI2CStart();

    /* Send the device address and read the data. All should return success
       in order for the writing processed to be successful
     */
    if ((ddk770_swI2CWriteByte(deviceAddress) != 0) ||
        (ddk770_swI2CWriteByte(registerIndex) != 0) ||
        (ddk770_swI2CWriteByte(data) != 0))
    {
        returnValue = -1;
    }
    
    /* Stop i2c and release the bus */
    ddk770_swI2CStop();

    return returnValue;
}
