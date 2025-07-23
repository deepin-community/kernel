/*******************************************************************
* 
*         Copyright (c) 2008 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  hwi2c.H --- SMI DDK 
*  This file contains the definitions for Hardware I2C.
* 
*******************************************************************/
#ifndef _DDK770_HWI2C_H_
#define _DDK770_HWI2C_H_

#define MAX_HWI2C_FIFO 16
#define HWI2C_WAIT_TIMEOUT 0x1000 //0x7FF

#include "../smi_drv.h"
/*
 *  This function initializes the hardware i2c
 *
 *  Return Value:
 *       0   - Success
 *      -1   - Fail to initialize i2c
 */
long ddk770_hwI2CInit(
    unsigned char i2cNumber //I2C0 or I2C1
);

/* 
 * This function close the hardware i2c 
 */
void ddk770_hwI2CClose(
    unsigned char i2cNumber //I2C0 or I2C1
);

/*
 *  This function writes data to the i2c slave device registers.
 *
 *  Parameters:
 *      deviceAddress   - i2c Slave device address
 *      length          - Total number of bytes to be written to the device
 *      pBuffer         - The buffer that contains the data to be written to the
 *                     i2c device.   
 *
 *  Return Value:
 *      Total number of bytes those are actually written.
 */
unsigned long ddk770_hwI2CWriteData(
    unsigned char i2cNumber, //I2C0 or I2C1
    unsigned char deviceAddress,
    unsigned long length,
    unsigned char *pBuffer
);

/*
 *  This function reads data from the slave device and stores them
 *  in the given buffer
 *
 *  Parameters:
 *      deviceAddress   - i2c Slave device address
 *      length          - Total number of bytes to be read
 *      pBuffer         - Pointer to a buffer to be filled with the data read
 *                     from the slave device. It has to be the same size as the
 *                     length to make sure that it can keep all the data read.   
 *
 *  Return Value:
 *      Total number of actual bytes read from the slave device
 */
unsigned long ddk770_hwI2CReadData(
    unsigned char i2cNumber, //I2C0 or I2C1
    unsigned char deviceAddress,
    unsigned long length,
    unsigned char *pBuffer
);

/* 
 * This function read the i2c device register value
 *
 * Input:   deviceAddress   - I2C Device Address
 *          registerIndex   - Register index to be read
 *
 * Output:
 *          The value of the register being read.
 */
unsigned char ddk770_hwI2CReadReg(
    unsigned char i2cNumber, //I2C0 or I2C1
    unsigned char deviceAddress, 
    unsigned char registerIndex
);

/* 
 * This function read the i2c 16 bit device register value
 *
 * Input:   deviceAddress   - I2C Device Address
 *          registerIndex   - Register index to be read
 *
 * Output:
 *          The value of the register being read.
 */
unsigned short hwI2CReadReg_16bit(
    unsigned char i2cNumber, //I2C0 or I2C1
    unsigned char deviceAddress, 
    unsigned short registerIndex
);

/* 
 * This function writes a value to the i2c device register.
 *
 * Input:   deviceAddress   - I2C Device Address
 *          registerIndex   - Register index to be written to
 *          data            - Data to be written to
 *
 * Output:
 *          0   - Success
 *         -1   - Fail
 */
long ddk770_hwI2CWriteReg(
    unsigned char i2cNumber, //I2C0 or I2C1
    unsigned char deviceAddress, 
    unsigned char registerIndex, 
    unsigned char data
);

/* 
 * This function writes a value to the i2c 16bit device register.
 *
 * Input:   deviceAddress   - I2C Device Address
 *          registerIndex   - Register index to be written to
 *          data            - Data to be written to
 *
 * Output:
 *          0   - Success
 *         -1   - Fail
 */
long hwI2CWriteReg_16bit(
    unsigned char i2cNumber, //I2C0 or I2C1
    unsigned char deviceAddress, 
    unsigned short registerIndex, 
    unsigned short data
);

#endif  /* _HWI2C_H_ */
