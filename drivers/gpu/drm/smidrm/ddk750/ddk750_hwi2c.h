#ifndef _HWI2C_H_
#define _HWI2C_H_

#include "../smi_drv.h"


/*
 *  This function initializes the hardware i2c
 *
 *  Parameters:
 *      busSpeedMode    - I2C Bus Speed Mode
 *                       0 = Standard Mode (100kbps)
 *                       1 = Fast Mode (400kbps)
 *
 *  Return Value:
 *       0   - Success
 *      -1   - Fail to initialize i2c
 */
long ddk750_hwI2CInit(
    unsigned char busSpeedMode
);

/* 
 * This function close the hardware i2c 
 */
void ddk750_hwI2CClose(void);

/* 
 * This function read the i2c device register value
 *
 * Input:   deviceAddress   - I2C Device Address
 *          registerIndex   - Register index to be read
 *
 * Output:
 *          The value of the register being read.
 */
unsigned char ddk750_hwI2CReadReg(
    unsigned char deviceAddress, 
    unsigned char registerIndex
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
long ddk750_hwI2CWriteReg(
    unsigned char deviceAddress, 
    unsigned char registerIndex, 
    unsigned char data
);


long ddk750_AdaptHWI2CInit(struct smi_connector *connector);
long ddk750_AdaptHWI2CCleanBus(struct smi_connector *connector);



#endif  /* _HWI2C_H_ */
