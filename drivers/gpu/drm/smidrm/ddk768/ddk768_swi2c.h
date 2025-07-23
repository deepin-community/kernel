
#ifndef _DDK768_SWI2C_H_
#define _DDK768_SWI2C_H_

#include "../smi_drv.h"

/* Default i2c CLK and Data GPIO. These are the default i2c pins */
#define DEFAULT_I2C0_SCL                     30
#define DEFAULT_I2C0_SDA                     31

#define DEFAULT_I2C1_SCL                     6
#define DEFAULT_I2C1_SDA                     7

/*
 * This function initializes the i2c attributes and bus
 *
 * Parameters:
 *      i2cClkGPIO  - The GPIO pin to be used as i2c SCL
 *      i2cDataGPIO - The GPIO pin to be used as i2c SDA
 *
 * Return Value:
 *      -1   - Fail to initialize the i2c
 *       0   - Success
 */
long ddk768_swI2CInit(
    unsigned char i2cClkGPIO, 
    unsigned char i2cDataGPIO
);

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
long ddk768_swI2CWriteReg(
    unsigned char deviceAddress, 
    unsigned char registerIndex, 
    unsigned char data
);

unsigned char ddk768_swI2CReadReg(
    unsigned char deviceAddress, 
    unsigned char registerIndex
);


long ddk768_AdaptSWI2CInit(struct smi_connector *smi_connector);

long ddk768_AdaptSWI2CCleanBus(
    struct smi_connector *connector);



#endif  /* _SWI2C_H_ */
