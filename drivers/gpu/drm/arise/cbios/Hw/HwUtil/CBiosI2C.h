//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************


/*****************************************************************************
** DESCRIPTION:
** I2C module interface prototype and parameter definition.
**
** NOTE:
** This header file CAN ONLY be included by hw layer those files under Hw folder. 
******************************************************************************/

#ifndef _CBIOS_I2C_H_
#define _CBIOS_I2C_H_

#define LEVEL_HIGH              1       //line level high
#define LEVEL_LOW               0       //line level low
#define MAXI2CWAITLOOP          4000 

/* For DDC-CI */
/* I2C Commands */
#define DDCCII2C_COMMAND_NULL       0x0000
#define DDCCII2C_COMMAND_READ       0x0001
#define DDCCII2C_COMMAND_WRITE      0x0002
#define DDCCII2C_COMMAND_STATUS     0x0004
#define DDCCII2C_COMMAND_RESET      0x0008

/* The following flags are provided on a READ or WRITE command          */
#define DDCCII2C_FLAGS_START        0x0001 /* START + addx              */
#define DDCCII2C_FLAGS_STOP         0x0002 /* STOP                      */
#define DDCCII2C_FLAGS_DATACHAINING 0x0004 /* STOP, START + addx        */
#define DDCCII2C_FLAGS_ACK          0x0010 /* ACKNOWLEDGE (normally set)*/

/* The following status flags are returned on completion of the operation */
#define DDCCII2C_STATUS_NOERROR     0x0000
#define DDCCII2C_STATUS_BUSY        0x0001
#define DDCCII2C_STATUS_ERROR       0x0002

#define DDCCIMAX_CLOCK_RATE         1000*1000   /* in Hz */
#define DDCCIMIN_CLOCK_RATE         10*1000     /* in Hz */

// I2C & GPIO pair as I2C index definition
typedef enum _CBIOS_I2CBUS_INDEX
{
    I2CBUS0                     =  0,
    I2CBUS1                     =  1,
    I2CBUS2                     =  2,
    I2CBUS3                     =  3,
    I2CBUS4                     =  4,
    RSVD_I2CBUS5                =  5,
    RSVD_I2CBUS6                =  6,
    RSVD_I2CBUS7                =  7,
    I2CBUS_SCLGPIO4_SDAGPIO5    =  8,
    I2CBUS_SCLGPIO5_SDAGPIO2    =  9, 
    I2CBUS_SCLGPIO3_SDAGPIO4    = 10, 
    I2CBUS_SCLGPIO2_SDAGPIO5    = 11,
    I2CBUS_SCLGPIO2_SDAGPIO3    = 12,
    RSVD_I2CBUS13               = 13,
    RSVD_I2CBUS14               = 14,
    RSVD_I2CBUS15               = 15,
    I2CBUS_VIRTUAL_DP1          = 16,
    I2CBUS_VIRTUAL_DP2          = 17,
    I2CBUS_VIRTUAL_DAC1         = 18,   //for no-edid device to store fake edid
    I2CBUS_VIRTUAL_DAC2         = 19,   //for no-edid device to store fake edid
    I2CBUS_VIRTUAL_DVO1         = 20,   //for no-edid device to store fake edid
    I2CBUS_VIRTUAL_DVO2         = 21,   //for no-edid device to store fake edid
    I2CBUS_VIRTUAL_COMBO1       = 22,   //for no-edid device to store fake edid
    I2CBUS_VIRTUAL_COMBO2       = 23,   //for no-edid device to store fake edid
    TOTAL_I2CBUS_NUM,
}CBIOS_I2CBUS_INDEX;
#define MIN_NORMAL_I2C_BUS      I2CBUS0
#define MAX_NORMAL_I2C_BUS      I2CBUS4
#define MIN_GPIO_I2C_BUS        I2CBUS_SCLGPIO4_SDAGPIO5
#define MAX_GPIO_I2C_BUS        I2CBUS_SCLGPIO2_SDAGPIO3

typedef enum _CBIOS_I2C_CONFIGTYPE
{
    CONFIG_I2C_BY_BUSNUM        = 0,    //only set I2C bus num
    CONFIG_I2C_BY_REG           = 1     //set the Dline and Cline registers directly
}CBIOS_I2C_CONFIGTYPE;

typedef struct _CBIOS_I2C_REG_PARAMS {
    CBIOS_U16         DLineRegType_Index;
    CBIOS_U8          DLineRegReadBitNum;
    CBIOS_U8          DLineRegReadEnableValue;   // only used for GPIO, should set value to zero for I2C
    CBIOS_U8          DLineRegReadEnableMask;    // only used for GPIO, should set mask to 0xFF for I2C 
    CBIOS_U8          DLineRegWriteBitNum;
    CBIOS_U8          DLineRegWriteEnableValue;  // equal port enable value for I2C
    CBIOS_U8          DLineRegWriteEnableMask;   // equal port enable mask for I2C
    CBIOS_U16         CLineRegType_Index;
    CBIOS_U8          CLineRegReadBitNum;
    CBIOS_U8          CLineRegReadEnableValue;   // only used for GPIO, should set value to zero for I2C
    CBIOS_U8          CLineRegReadEnableMask;    // only used for GPIO, should set mask to 0xFF for I2C
    CBIOS_U8          CLineRegWriteBitNum;
    CBIOS_U8          CLineRegWriteEnableValue;  // equal port enable value for I2C
    CBIOS_U8          CLineRegWriteEnableMask;   // equal port enable mask for I2C
} CBIOS_I2C_REG_PARAMS, *PCBIOS_I2C_REG_PARAMS;

typedef struct _CBIOS_MODULE_I2C_PARAMS {
    CBIOS_U32             Size;  
    CBIOS_I2C_REG_PARAMS  I2CRegParams;
    CBIOS_U8              I2CBusNum;
    CBIOS_U8              SlaveAddress;
    CBIOS_U8              OffSet;
    CBIOS_U8*             Buffer;
    CBIOS_U32             BufferLen;
    CBIOS_U8              ConfigType;            // CONFIG_I2C_BY_REG: set the Dline and Cline registers directly
                                                 // CONFIG_I2C_BY_BUSNUM: only use I2C bus num
} CBIOS_MODULE_I2C_PARAMS, *PCBIOS_MODULE_I2C_PARAMS;

//I2C module interfaces
CBIOS_BOOL cbI2CModule_ReadData(PCBIOS_VOID pvcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams);
CBIOS_BOOL cbI2CModule_WriteData(PCBIOS_VOID pvcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams);
CBIOS_BOOL cbI2CModule_WriteBits(PCBIOS_VOID pvcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams, CBIOS_U8 I2CDataMask);
CBIOS_STATUS cbI2CModule_ReadDDCCIData(PCBIOS_VOID pvcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams, CBIOS_U32 Flags);
CBIOS_BOOL cbI2CModule_WriteDDCCIData(PCBIOS_VOID pvcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams);
CBIOS_STATUS cbI2CModule_DDCCI_OPEN(PCBIOS_VOID pvcbe, CBIOS_BOOL bOpen, PCBIOS_I2CCONTROL pI2CControl, CBIOS_U8 I2CBUSNum);
CBIOS_STATUS cbI2CModule_DDCCI_ACCESS(PCBIOS_VOID pvcbe, PCBIOS_I2CCONTROL pI2CControl, CBIOS_U8 I2CBUSNum);
CBIOS_VOID cbI2CModule_HDCPI2CEnableDisable(PCBIOS_VOID pvcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams, CBIOS_BOOL bIsEnable);

#endif
