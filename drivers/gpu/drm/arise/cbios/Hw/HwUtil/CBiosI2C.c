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
** I2C module interface function implementation. 
** Mainly includes read/write data of I2C bus, GPIO simulate I2C, and DDCCIP
**
** NOTE:
** The functions in this file are hw layer internal functions, 
** CAN ONLY be called by files under Hw folder. 
******************************************************************************/

#include "CBiosChipShare.h"
#include "CBiosUtilHw.h"
#include "../HwBlock/CBiosDIU_DP.h"

/***************************************************************
Function:    cbI2CModule_SetScl

Description: Set I2C clock line to specific lineLvl

Input:       PCBIOS_EXTENSION_COMMON
             PCBIOS_I2C_REG_PARAMS
             lineLvl, can be LEVEL_HIGH or LEVEL_LOW

Output:   

Return:
***************************************************************/
static CBIOS_VOID cbI2CModule_SetScl(PCBIOS_EXTENSION_COMMON pcbe, 
                                     PCBIOS_I2C_REG_PARAMS pI2CRegParams, 
                                     CBIOS_U8 lineLvl)
{
    CBIOS_U8 ulValue = 0, ulMask = 0;

    ulValue = lineLvl << pI2CRegParams->CLineRegWriteBitNum;
    ulValue += pI2CRegParams->CLineRegWriteEnableValue;
    ulMask = 1 << pI2CRegParams->CLineRegWriteBitNum;
    ulMask = pI2CRegParams->CLineRegWriteEnableMask & (~ulMask);
    
    cbMMIOWriteReg(pcbe, pI2CRegParams->CLineRegType_Index, ulValue, ulMask);
}

/***************************************************************
Function:    cbI2CModule_GetScl

Description: Get I2C clock line level

Input:       PCBIOS_EXTENSION_COMMON
             PCBIOS_I2C_REG_PARAMS

Output:   

Return:      the line level of I2C clock line
***************************************************************/
static CBIOS_U8 cbI2CModule_GetScl(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_I2C_REG_PARAMS pI2CRegParams)
{
    CBIOS_U8 ulValue = 0, ulMask = 0, ulRet = 0;

    //first enable read function
    ulValue = pI2CRegParams->CLineRegReadEnableValue;
    ulMask = pI2CRegParams->CLineRegReadEnableMask;
    cbMMIOWriteReg(pcbe, pI2CRegParams->CLineRegType_Index, ulValue, ulMask);
    
    ulRet = cbMMIOReadReg(pcbe, pI2CRegParams->CLineRegType_Index);
    ulRet = ulRet >> pI2CRegParams->CLineRegReadBitNum;
    ulRet = ulRet & 0x01;
    return ulRet;
}

/***************************************************************
Function:    cbI2CModule_SetSda

Description: Set I2C data line to specific lineLvl

Input:       PCBIOS_EXTENSION_COMMON
             PCBIOS_MODULE_I2C_PARAMS
             lineLvl, can be LEVEL_HIGH or LEVEL_LOW

Output:   

Return:
***************************************************************/
static CBIOS_VOID cbI2CModule_SetSda(PCBIOS_EXTENSION_COMMON pcbe, 
                                     PCBIOS_I2C_REG_PARAMS pI2CRegParams, 
                                     CBIOS_U8 lineLvl)
{
    CBIOS_U8 ulValue = 0, ulMask = 0;

    ulValue = lineLvl << pI2CRegParams->DLineRegWriteBitNum;
    ulValue += pI2CRegParams->DLineRegWriteEnableValue;
    ulMask = 1 << pI2CRegParams->DLineRegWriteBitNum;
    ulMask = pI2CRegParams->DLineRegWriteEnableMask & (~ulMask);
    
    cbMMIOWriteReg(pcbe, pI2CRegParams->DLineRegType_Index, ulValue, ulMask);
}

/***************************************************************
Function:    cbI2CModule_GetSda

Description: Get I2C data line level

Input:       PCBIOS_MODULE_I2C_PARAMS
             PCBIOS_I2C_REG_PARAMS

Output:   

Return:      the line level of I2C data line
***************************************************************/
static CBIOS_U8 cbI2CModule_GetSda(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_I2C_REG_PARAMS pI2CRegParams)
{
    CBIOS_U8 ulValue = 0, ulMask = 0, ulRet = 0;

    //first enable read function
    ulValue = pI2CRegParams->DLineRegReadEnableValue;
    ulMask = pI2CRegParams->DLineRegReadEnableMask;
    cbMMIOWriteReg(pcbe, pI2CRegParams->DLineRegType_Index, ulValue, ulMask);
    
    ulRet = cbMMIOReadReg(pcbe, pI2CRegParams->DLineRegType_Index);
    ulRet = ulRet >> pI2CRegParams->DLineRegReadBitNum;
    ulRet = ulRet & 0x01;
    return ulRet;
}

/***************************************************************
Function:    cbI2CModule_CheckBusNum

Description: Check the I2C bus num is valid.

Input:       PCBIOS_EXTENSION_COMMON
             PCBIOS_MODULE_I2C_PARAMS

Output:      

Return:      CBIOS_TURE if I2C bus num is valid
             CBIOS_FALSE if I2C bus num is invalid
***************************************************************/
static CBIOS_BOOL cbI2CModule_CheckBusNum(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    CBIOS_U8   I2CBusNum = pCBModuleI2CParams->I2CBusNum;
    CBIOS_BOOL bRet = CBIOS_FALSE;
    
    //normal I2C
    if ((I2CBusNum >= MIN_NORMAL_I2C_BUS) && (I2CBusNum <= MAX_NORMAL_I2C_BUS))
    {
        bRet = CBIOS_TRUE;
    }
    //GPIO simulate I2C
    else if ((I2CBusNum >= MIN_GPIO_I2C_BUS) && (I2CBusNum <= MAX_GPIO_I2C_BUS))
    {
        bRet = CBIOS_TRUE;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbI2CModule_CheckBusNum: I2CBusNum invalid! \n"));
        bRet = CBIOS_FALSE;
    }
    return bRet;
}

/***************************************************************
Function:    cbI2CModule_Start

Description: Send the start signal to the I2C bus

Input:       PCBIOS_MODULE_I2C_PARAMS

Output:   

Return:
***************************************************************/
static CBIOS_VOID cbI2CModule_Start(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    CBIOS_U8  bySCL = 0, bySDL = 0;
    CBIOS_U32 i = 0;
    CBIOS_U32 MaxLoop = MAXI2CWAITLOOP;     
    CBIOS_U8  I2CDELAY = pcbe->I2CDelay;
    PCBIOS_I2C_REG_PARAMS pI2CRegParams = &(pCBModuleI2CParams->I2CRegParams);

    //check I2CBusNum
    if(cbI2CModule_CheckBusNum(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbI2CModule_Start: I2CBusNum invalid! \n"));
        return;
    }

    //check the SCL status
    bySCL = cbI2CModule_GetScl(pcbe, pI2CRegParams);

    //SCL Low
    if (bySCL == LEVEL_LOW)
    {
        //wait for SCL High
        for (i = 0; i < MaxLoop; i++)
        {
            cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_HIGH);
            cb_DelayMicroSeconds(I2CDELAY);

            bySCL = cbI2CModule_GetScl(pcbe, pI2CRegParams);
            if (bySCL == LEVEL_HIGH)
            {
                break;
            }
        }
    }

    //check the SDL status
    bySDL = cbI2CModule_GetSda(pcbe, pI2CRegParams);
    
    //SCL High + SDA Low
    if ((bySCL == LEVEL_HIGH) && (bySDL == LEVEL_LOW))
    {
        //set SCL Low
        cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_LOW);
        cb_DelayMicroSeconds(I2CDELAY);

        //set SDA High
        cbI2CModule_SetSda(pcbe, pI2CRegParams, LEVEL_HIGH);
        cb_DelayMicroSeconds(I2CDELAY);

        //wait for SCLHigh
        for (i = 0; i < MaxLoop; i++)
        {
            cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_HIGH);
            cb_DelayMicroSeconds(I2CDELAY);

            bySCL = cbI2CModule_GetScl(pcbe, pI2CRegParams);
            if (bySCL == LEVEL_HIGH)
            {
                break;
            }
        }
    }

    //check SCL High + SDA High
    bySCL = cbI2CModule_GetScl(pcbe, pI2CRegParams);
    bySDL = cbI2CModule_GetSda(pcbe, pI2CRegParams);
    if ((bySCL == LEVEL_HIGH) && (bySDL == LEVEL_HIGH))
    {
        //I2C start: SCL High, SDA High->Low
        cbI2CModule_SetSda(pcbe, pI2CRegParams, LEVEL_LOW);
        cb_DelayMicroSeconds(I2CDELAY);

        cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_LOW);
        cb_DelayMicroSeconds(I2CDELAY);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbI2CModule_Start: I2C start failed! \n"));
    }
}

/***************************************************************
Function:    cbI2CModule_Stop

Description: Send the stop signal to the I2C bus

Input:       PCBIOS_EXTENSION_COMMON
             PCBIOS_MODULE_I2C_PARAMS

Output:      

Return:      CBIOS_TRUE for success, CBIOS_FALSE for fail
***************************************************************/
static CBIOS_BOOL cbI2CModule_Stop(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    CBIOS_U8  byTemp = 0;
    CBIOS_U32 i = 0;
    CBIOS_U32 MaxLoop = MAXI2CWAITLOOP;
    CBIOS_U8  I2CDELAY = pcbe->I2CDelay;
    PCBIOS_I2C_REG_PARAMS pI2CRegParams = &(pCBModuleI2CParams->I2CRegParams);

    //check I2CBusNum
    if(cbI2CModule_CheckBusNum(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbI2CModule_Stop: I2CBusNum invalid! \n"));
        return CBIOS_FALSE;
    }

    cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_LOW);
    cb_DelayMicroSeconds(I2CDELAY);
    cbI2CModule_SetSda(pcbe, pI2CRegParams, LEVEL_LOW);
    cb_DelayMicroSeconds(I2CDELAY);

    for (i = 0; i < MaxLoop; i++)
    {
        cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_HIGH);
        cb_DelayMicroSeconds(I2CDELAY);
        byTemp = cbI2CModule_GetScl(pcbe, pI2CRegParams);
        if (byTemp == LEVEL_HIGH)
        {
            break;
        }
    }

    if (byTemp == LEVEL_HIGH)
    {
        cbI2CModule_SetSda(pcbe, pI2CRegParams, LEVEL_HIGH);
    }

    return CBIOS_TRUE;
}

/***************************************************************
Function:    cbI2CModule_Enable

Description: enable I2C input and output

Input:       PCBIOS_MODULE_I2C_PARAMS

Output:   

Return:
***************************************************************/
static CBIOS_VOID cbI2CModule_Enable(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    CBIOS_U8  I2CDELAY = pcbe->I2CDelay;
    PCBIOS_I2C_REG_PARAMS pI2CRegParams = &(pCBModuleI2CParams->I2CRegParams);

    //check I2CBusNum
    if(cbI2CModule_CheckBusNum(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbI2CModule_Enable: I2CBusNum invalid! \n"));
        return;
    }

    //release Sda
    cbI2CModule_SetSda(pcbe, pI2CRegParams, LEVEL_HIGH);
    cb_DelayMicroSeconds(I2CDELAY);
    //release Scl
    cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_HIGH);
    cb_DelayMicroSeconds(I2CDELAY);
}

/***************************************************************
Function:    cbI2CModule_Disable

Description: disable I2C input and output

Input:       PCBIOS_MODULE_I2C_PARAMS

Output:  

Return:
***************************************************************/
static CBIOS_VOID cbI2CModule_Disable(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{

    CBIOS_U8 ulValue = 0, ulMask = 0;
    PCBIOS_I2C_REG_PARAMS pI2CRegParams = &(pCBModuleI2CParams->I2CRegParams);

    //check I2CBusNum
    if(cbI2CModule_CheckBusNum(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbI2CModule_Disable: I2CBusNum invalid! \n"));
        return;
    }
    
    //disable Scl
    ulValue = pI2CRegParams->CLineRegReadEnableValue;
    ulMask = pI2CRegParams->CLineRegReadEnableMask;
    ulValue = (~ulValue) & (~ulMask);   //cline read disable value
    cbMMIOWriteReg(pcbe, pI2CRegParams->CLineRegType_Index, ulValue, ulMask);
    ulValue = pI2CRegParams->CLineRegWriteEnableValue;
    ulMask = pI2CRegParams->CLineRegWriteEnableMask;
    ulValue = (~ulValue) & (~ulMask);   //cline write disable value
    cbMMIOWriteReg(pcbe, pI2CRegParams->CLineRegType_Index, ulValue, ulMask);

    //disable Sda
    ulValue = pI2CRegParams->DLineRegReadEnableValue;
    ulMask = pI2CRegParams->DLineRegReadEnableMask;
    ulValue = (~ulValue) & (~ulMask);   //dline read disable value
    cbMMIOWriteReg(pcbe, pI2CRegParams->DLineRegType_Index, ulValue, ulMask);
    ulValue = pI2CRegParams->DLineRegWriteEnableValue;
    ulMask = pI2CRegParams->DLineRegWriteEnableMask;
    ulValue = (~ulValue) & (~ulMask);   //dline write disable value
    cbMMIOWriteReg(pcbe, pI2CRegParams->DLineRegType_Index, ulValue, ulMask);   
}

/***************************************************************
Function:    cbI2CModule_AckRead

Description: Read the Acknowledge signal from the I2C bus.

Input:       PCBIOS_EXTENSION_COMMON
             PCBIOS_MODULE_I2C_PARAMS

Output:  

Return:      CBIOS_TRUE if read ACK successfully, 
             CBIOS_FALSE if read ACK failed
***************************************************************/
static CBIOS_BOOL cbI2CModule_AckRead(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    CBIOS_U8  byTemp = 0;
    CBIOS_U32 j = 0;
    CBIOS_U32 MaxLoop = MAXI2CWAITLOOP;
    CBIOS_BOOL bAck = CBIOS_FALSE;
    CBIOS_U8  I2CDELAY = pcbe->I2CDelay;
    PCBIOS_I2C_REG_PARAMS pI2CRegParams = &(pCBModuleI2CParams->I2CRegParams);
    
    //check I2CBusNum
    if(cbI2CModule_CheckBusNum(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbI2CModule_AckRead: I2CBusNum invalid! \n"));
        return CBIOS_FALSE;
    }

    cbI2CModule_SetScl(pcbe, pI2CRegParams,  LEVEL_LOW);
    cb_DelayMicroSeconds(I2CDELAY);
    cbI2CModule_SetSda(pcbe, pI2CRegParams,  LEVEL_HIGH);
    cb_DelayMicroSeconds(I2CDELAY);

    for (j = 0; j < MaxLoop; j++)
    {
        cbI2CModule_SetScl(pcbe, pI2CRegParams,  LEVEL_HIGH);
        cb_DelayMicroSeconds(I2CDELAY);

        byTemp = cbI2CModule_GetScl(pcbe, pI2CRegParams);

        if (byTemp == LEVEL_HIGH)
        {
            break;
        }
    }

    if (j >= MaxLoop)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbI2CModule_AckRead: can't pull up SCL! \n"));
        bAck = CBIOS_FALSE;
    }
    else
    {
        byTemp = cbI2CModule_GetSda(pcbe, pI2CRegParams);
        if (byTemp == LEVEL_LOW)
        {
            bAck = CBIOS_TRUE;
        }
        else
        {
            bAck = CBIOS_FALSE;
        }
    }

    cbI2CModule_SetScl(pcbe, pI2CRegParams,  LEVEL_LOW);
    cb_DelayMicroSeconds(I2CDELAY);
    cbI2CModule_SetSda(pcbe, pI2CRegParams,  LEVEL_HIGH);
    cb_DelayMicroSeconds(I2CDELAY);

    return bAck;
}

/***************************************************************
Function:    cbI2CModule_AckWrite

Description: Send ACKnowledgement when reading information.
             A ACK is DATA low during one clock pulse.

Input:       PCBIOS_EXTENSION_COMMON
             PCBIOS_MODULE_I2C_PARAMS

Output:  

Return:      CBIOS_TURE if write ACK successfully
             CBIOS_FALSE if write failed
***************************************************************/
static CBIOS_BOOL cbI2CModule_AckWrite(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    CBIOS_U8  byTemp = 0;
    CBIOS_U32 i = 0;
    CBIOS_U32 MaxLoop = MAXI2CWAITLOOP;
    CBIOS_BOOL bStatus = CBIOS_FALSE;
    CBIOS_U8  I2CDELAY = pcbe->I2CDelay;
    PCBIOS_I2C_REG_PARAMS pI2CRegParams = &(pCBModuleI2CParams->I2CRegParams);
    
    //check I2CBusNum
    if(cbI2CModule_CheckBusNum(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbI2CModule_AckWrite: I2CBusNum invalid! \n"));
        return CBIOS_FALSE;
    }

    cbI2CModule_SetScl(pcbe, pI2CRegParams,  LEVEL_LOW);
    cb_DelayMicroSeconds(I2CDELAY);
    cbI2CModule_SetSda(pcbe, pI2CRegParams,  LEVEL_LOW);
    cb_DelayMicroSeconds(I2CDELAY);

    for ( i = 0; i < MaxLoop; i++)
    {
        cbI2CModule_SetScl(pcbe, pI2CRegParams,  LEVEL_HIGH);
        cb_DelayMicroSeconds(I2CDELAY);

        byTemp = cbI2CModule_GetScl(pcbe, pI2CRegParams);

        if (byTemp == LEVEL_HIGH)
        {
            break;
        }
    }

    if (i < MaxLoop)
    {
        bStatus = CBIOS_TRUE;
    }
    else
    {
        bStatus = CBIOS_FALSE;
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbI2CModule_AckWrite: write ack failed! \n"));
    }

    return bStatus;
}

/***************************************************************
Function:    cbI2CModule_NackWrite

Description: Send Not ACKnowledgement when reading information.
             A NACK is DATA high during one clock pulse.

Input:       PCBIOS_EXTENSION_COMMON
             PCBIOS_MODULE_I2C_PARAMS

Output:  

Return:      CBIOS_TURE if write NACK successfully
             CBIOS_FALSE if write failed
***************************************************************/
static CBIOS_VOID cbI2CModule_NackWrite(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    CBIOS_U8  byTemp   = 0;
    CBIOS_U32 j = 0;
    CBIOS_U32 MaxLoop = MAXI2CWAITLOOP;   
    CBIOS_U8  I2CDELAY = pcbe->I2CDelay;
    PCBIOS_I2C_REG_PARAMS pI2CRegParams = &(pCBModuleI2CParams->I2CRegParams);
    
    //check I2CBusNum
    if(cbI2CModule_CheckBusNum(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbI2CModule_NackWrite: I2CBusNum invalid! \n"));
        return;
    }

    cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_LOW);
    cb_DelayMicroSeconds(I2CDELAY);
    cbI2CModule_SetSda(pcbe, pI2CRegParams, LEVEL_HIGH);
    cb_DelayMicroSeconds(I2CDELAY);

    for (j = 0; j < MaxLoop; j++)
    {
        cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_HIGH);
        cb_DelayMicroSeconds(I2CDELAY);

        byTemp = cbI2CModule_GetScl(pcbe, pI2CRegParams);

        if (byTemp == LEVEL_HIGH)
            break;
    }

    if (j >= MaxLoop)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbI2CModule_NackWrite: can't pull up SCL! \n"));
    }

    //check SDA
    byTemp = cbI2CModule_GetSda(pcbe, pI2CRegParams);
    if (byTemp == LEVEL_LOW)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbI2CModule_NackWrite: can't pull up SDA! \n"));
    }
}

/***************************************************************
Function:    cbI2CModule_ReadByte

Description: Read one byte data from I2C bus.

Input:       PCBIOS_EXTENSION_COMMON
             PCBIOS_MODULE_I2C_PARAMS

Output:      pData, the one byte data read

Return:      CBIOS_TURE if read data successfully
             CBIOS_FALSE if read data failed
***************************************************************/
static CBIOS_BOOL cbI2CModule_ReadByte(PCBIOS_EXTENSION_COMMON pcbe, 
                                       PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams,
                                       CBIOS_U8* pData)
{
    CBIOS_U8  byTemp;
    CBIOS_U8  data = 0;
    CBIOS_U32 i = 0;
    CBIOS_U32 j = 0;
    CBIOS_U32 MaxLoop = MAXI2CWAITLOOP;
    CBIOS_BOOL bStatus = CBIOS_TRUE;
    CBIOS_U8  I2CDELAY = pcbe->I2CDelay;
    PCBIOS_I2C_REG_PARAMS pI2CRegParams = &(pCBModuleI2CParams->I2CRegParams);
    
    //check I2CBusNum
    if(cbI2CModule_CheckBusNum(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbI2CModule_ReadByte: I2CBusNum invalid! \n"));
        return CBIOS_FALSE;
    }
   
    for (i=0; i<8; i++)
    {
        cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_LOW);
        cb_DelayMicroSeconds(I2CDELAY);
        cbI2CModule_SetSda(pcbe, pI2CRegParams, LEVEL_HIGH);
        cb_DelayMicroSeconds(I2CDELAY);

        for (j = 0; j < MaxLoop; j++)
        {
            cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_HIGH);
            cb_DelayMicroSeconds(I2CDELAY);
            byTemp = cbI2CModule_GetScl(pcbe, pI2CRegParams);

            if (byTemp == LEVEL_HIGH)
                break;
        }

        if(j == MaxLoop)
        {
            bStatus = CBIOS_FALSE;
            cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbI2CModule_ReadByte: can't pull up SCL! \n"));
            break;
        }

        data = (data << 1) + cbI2CModule_GetSda(pcbe, pI2CRegParams);

        cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_LOW);
        cb_DelayMicroSeconds(I2CDELAY);
        cbI2CModule_SetSda(pcbe, pI2CRegParams, LEVEL_HIGH);
        cb_DelayMicroSeconds(I2CDELAY);
    }

    *pData = data;
    
    return bStatus;
}

/***************************************************************
Function:    cbI2CModule_WriteByte

Description: Write one byte data to I2C bus.

Input:       PCBIOS_EXTENSION_COMMON
             PCBIOS_MODULE_I2C_PARAMS

Output:      

Return:                  
***************************************************************/
static CBIOS_VOID cbI2CModule_WriteByte(PCBIOS_EXTENSION_COMMON pcbe, 
                                        PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams,
                                        CBIOS_U8 data)
{
    CBIOS_S32 i, value;
    CBIOS_U8  byTemp = 0;
    CBIOS_U32 j = 0;
    CBIOS_U32 MaxLoop = MAXI2CWAITLOOP;
    CBIOS_U8  I2CDELAY = pcbe->I2CDelay;
    PCBIOS_I2C_REG_PARAMS pI2CRegParams = &(pCBModuleI2CParams->I2CRegParams);

    //check I2CBusNum
    if(cbI2CModule_CheckBusNum(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbI2CModule_WriteByte: I2CBusNum invalid! \n"));
        return;
    }

    //loop for 1 byte, start from MSB
    for (i = 7; i >= 0; i--)
    { 
        value = (data >> i) & 0x01;

        //set SCL low
        cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_LOW);
        cb_DelayMicroSeconds(I2CDELAY);
        //Set SDA
        if (value == 0)
        {
            cbI2CModule_SetSda(pcbe, pI2CRegParams, LEVEL_LOW);
        }
        else
        {
            cbI2CModule_SetSda(pcbe, pI2CRegParams, LEVEL_HIGH);
        }
        cb_DelayMicroSeconds(I2CDELAY);

        //wait for SCL high
        for (j = 0; j < MaxLoop; j++)
        {
            cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_HIGH);
            cb_DelayMicroSeconds(I2CDELAY);

            byTemp = cbI2CModule_GetScl(pcbe, pI2CRegParams);
            if (byTemp == LEVEL_HIGH)
            {
                break;
            }
        }
        //can't wait for SCL high, fail
        if (j == MaxLoop)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbI2CModule_WriteByte: can't pull SCL high! \n"));
            break;
        }
        if (value == 1)
        {
            //check SDA
            byTemp = cbI2CModule_GetSda(pcbe, pI2CRegParams);
            if (byTemp != LEVEL_HIGH)
            {
                //can't pull up SDA, fail
                cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "cbI2CModule_WriteByte: can't pull SDA high! \n"));
                break;
            }
        }

        cbI2CModule_SetScl(pcbe, pI2CRegParams, LEVEL_LOW);
        cb_DelayMicroSeconds(I2CDELAY);
        cbI2CModule_SetSda(pcbe, pI2CRegParams, LEVEL_HIGH);
        cb_DelayMicroSeconds(I2CDELAY);
    }

}

/***************************************************************
Function:    cbI2CModule_GetI2CParams

Description: Get the clock and data line params of specific I2C bus.

Input:       PCBIOS_EXTENSION_COMMON
             PCBIOS_MODULE_I2C_PARAMS

Output:      

Return:      CBIOS_TURE if get params successfully
             CBIOS_FALSE if get params failed
***************************************************************/
static CBIOS_BOOL cbI2CModule_GetI2CParams(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    CBIOS_U8 I2CBusNum = pCBModuleI2CParams->I2CBusNum;
    PCBIOS_I2C_REG_PARAMS pI2CRegParams = &(pCBModuleI2CParams->I2CRegParams);

    //check I2CBusNum
    if(cbI2CModule_CheckBusNum(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "cbI2CModule_GetI2CParams: I2CBusNum invalid! \n"));
        return CBIOS_FALSE;
    }
    
    switch(I2CBusNum)
    {
    case I2CBUS0: //Normal I2C bus0
        {
            pI2CRegParams->CLineRegType_Index = CR_A0;
            pI2CRegParams->CLineRegReadEnableMask = 0xFF;
            pI2CRegParams->CLineRegReadEnableValue = 0;
            pI2CRegParams->CLineRegReadBitNum = 2;
            pI2CRegParams->CLineRegWriteBitNum = 0;
            pI2CRegParams->CLineRegWriteEnableMask = 0xEF;
            pI2CRegParams->CLineRegWriteEnableValue = 0x10;


            pI2CRegParams->DLineRegType_Index = CR_A0;
            pI2CRegParams->DLineRegReadEnableMask = 0xFF;
            pI2CRegParams->DLineRegReadEnableValue = 0;
            pI2CRegParams->DLineRegReadBitNum = 3;
            pI2CRegParams->DLineRegWriteBitNum = 1;
            pI2CRegParams->DLineRegWriteEnableMask = 0xEF;
            pI2CRegParams->DLineRegWriteEnableValue = 0x10;  
        }
        break;
    case I2CBUS1: //Normal I2C bus1
        {
            pI2CRegParams->CLineRegType_Index = CR_AA;
            pI2CRegParams->CLineRegReadEnableMask = 0xFF;
            pI2CRegParams->CLineRegReadEnableValue = 0;
            pI2CRegParams->CLineRegReadBitNum = 2;
            pI2CRegParams->CLineRegWriteBitNum = 0;
            pI2CRegParams->CLineRegWriteEnableMask = 0xEF;
            pI2CRegParams->CLineRegWriteEnableValue = 0x10;


            pI2CRegParams->DLineRegType_Index = CR_AA;
            pI2CRegParams->DLineRegReadEnableMask = 0xFF;
            pI2CRegParams->DLineRegReadEnableValue = 0;
            pI2CRegParams->DLineRegReadBitNum = 3;
            pI2CRegParams->DLineRegWriteBitNum = 1;
            pI2CRegParams->DLineRegWriteEnableMask = 0xEF;
            pI2CRegParams->DLineRegWriteEnableValue = 0x10;  
        }
        break;
    case I2CBUS2: //Normal I2C bus2
        {
            pI2CRegParams->CLineRegType_Index = CR_B_C5;
            pI2CRegParams->CLineRegReadEnableMask = 0xFF;
            pI2CRegParams->CLineRegReadEnableValue = 0;
            pI2CRegParams->CLineRegReadBitNum = 2;
            pI2CRegParams->CLineRegWriteBitNum = 0;
            pI2CRegParams->CLineRegWriteEnableMask = 0xEF;
            pI2CRegParams->CLineRegWriteEnableValue = 0x10;


            pI2CRegParams->DLineRegType_Index = CR_B_C5;
            pI2CRegParams->DLineRegReadEnableMask = 0xFF;
            pI2CRegParams->DLineRegReadEnableValue = 0;
            pI2CRegParams->DLineRegReadBitNum = 3;
            pI2CRegParams->DLineRegWriteBitNum = 1;
            pI2CRegParams->DLineRegWriteEnableMask = 0xEF;
            pI2CRegParams->DLineRegWriteEnableValue = 0x10;  
        }
        break;
    case I2CBUS3: //Normal I2C bus3
        {
            pI2CRegParams->CLineRegType_Index = CR_B_C6;
            pI2CRegParams->CLineRegReadEnableMask = 0xFF;
            pI2CRegParams->CLineRegReadEnableValue = 0;
            pI2CRegParams->CLineRegReadBitNum = 2;
            pI2CRegParams->CLineRegWriteBitNum = 0;
            pI2CRegParams->CLineRegWriteEnableMask = 0xEF;
            pI2CRegParams->CLineRegWriteEnableValue = 0x10;


            pI2CRegParams->DLineRegType_Index = CR_B_C6;
            pI2CRegParams->DLineRegReadEnableMask = 0xFF;
            pI2CRegParams->DLineRegReadEnableValue = 0;
            pI2CRegParams->DLineRegReadBitNum = 3;
            pI2CRegParams->DLineRegWriteBitNum = 1;
            pI2CRegParams->DLineRegWriteEnableMask = 0xEF;
            pI2CRegParams->DLineRegWriteEnableValue = 0x10;  
        }
        break;        
    case I2CBUS4: //Normal I2C bus4
        {
            pI2CRegParams->CLineRegType_Index = CR_B_F8;
            pI2CRegParams->CLineRegReadEnableMask = 0xFF;
            pI2CRegParams->CLineRegReadEnableValue = 0;
            pI2CRegParams->CLineRegReadBitNum = 2;
            pI2CRegParams->CLineRegWriteBitNum = 0;
            pI2CRegParams->CLineRegWriteEnableMask = 0xEF;
            pI2CRegParams->CLineRegWriteEnableValue = 0x10;


            pI2CRegParams->DLineRegType_Index = CR_B_F8;
            pI2CRegParams->DLineRegReadEnableMask = 0xFF;
            pI2CRegParams->DLineRegReadEnableValue = 0;
            pI2CRegParams->DLineRegReadBitNum = 3;
            pI2CRegParams->DLineRegWriteBitNum = 1;
            pI2CRegParams->DLineRegWriteEnableMask = 0xEF;
            pI2CRegParams->DLineRegWriteEnableValue = 0x10;  
        }
        break;      
    case I2CBUS_SCLGPIO4_SDAGPIO5: //GPIO4-SCLK   GPIO5-SDAT
        {
            pI2CRegParams->CLineRegType_Index = SR_48;
            pI2CRegParams->CLineRegReadEnableMask = 0xBF;
            pI2CRegParams->CLineRegReadEnableValue = 0x40;
            pI2CRegParams->CLineRegReadBitNum = 7;
            pI2CRegParams->CLineRegWriteBitNum = 4;
            pI2CRegParams->CLineRegWriteEnableMask = 0xDF;
            pI2CRegParams->CLineRegWriteEnableValue = 0x20;


            pI2CRegParams->DLineRegType_Index = SR_4A;
            pI2CRegParams->DLineRegReadEnableMask = 0xFB;
            pI2CRegParams->DLineRegReadEnableValue = 0x04;
            pI2CRegParams->DLineRegReadBitNum = 3;
            pI2CRegParams->DLineRegWriteBitNum = 0;
            pI2CRegParams->DLineRegWriteEnableMask = 0xFD;
            pI2CRegParams->DLineRegWriteEnableValue = 0x02;    
        }
        break;
    case I2CBUS_SCLGPIO5_SDAGPIO2: //GPIO5-SCLK GPIO2-SDAT
        {
            pI2CRegParams->CLineRegType_Index = SR_4A;
            pI2CRegParams->CLineRegReadEnableMask = 0xFB;
            pI2CRegParams->CLineRegReadEnableValue = 0x04;
            pI2CRegParams->CLineRegReadBitNum = 3;
            pI2CRegParams->CLineRegWriteBitNum = 0;
            pI2CRegParams->CLineRegWriteEnableMask = 0xFD;
            pI2CRegParams->CLineRegWriteEnableValue = 0x02;


            pI2CRegParams->DLineRegType_Index = CR_B_DA;
            pI2CRegParams->DLineRegReadEnableMask = 0xBF;
            pI2CRegParams->DLineRegReadEnableValue = 0x40;
            pI2CRegParams->DLineRegReadBitNum = 7;
            pI2CRegParams->DLineRegWriteBitNum = 4;
            pI2CRegParams->DLineRegWriteEnableMask = 0xDF;
            pI2CRegParams->DLineRegWriteEnableValue = 0x20;   
        }
        break;
    case I2CBUS_SCLGPIO3_SDAGPIO4: // GPIO3-SCLK GPIO4-SDAT
        {
            pI2CRegParams->CLineRegType_Index = SR_48;
            pI2CRegParams->CLineRegReadEnableMask = 0xFB;
            pI2CRegParams->CLineRegReadEnableValue = 0x04;
            pI2CRegParams->CLineRegReadBitNum = 3;
            pI2CRegParams->CLineRegWriteBitNum = 0;
            pI2CRegParams->CLineRegWriteEnableMask = 0xFD;
            pI2CRegParams->CLineRegWriteEnableValue = 0x02;    

            pI2CRegParams->DLineRegType_Index = SR_48;
            pI2CRegParams->DLineRegReadEnableMask = 0xBF;
            pI2CRegParams->DLineRegReadEnableValue = 0x40;
            pI2CRegParams->DLineRegReadBitNum = 7;
            pI2CRegParams->DLineRegWriteBitNum = 4;
            pI2CRegParams->DLineRegWriteEnableMask = 0xDF;
            pI2CRegParams->DLineRegWriteEnableValue = 0x20;
        }
        break;
    case I2CBUS_SCLGPIO2_SDAGPIO5://GPIO2-SCLK GPIO5-SDAT
        {
            pI2CRegParams->CLineRegType_Index = CR_B_DA;
            pI2CRegParams->CLineRegReadEnableMask = 0xBF;
            pI2CRegParams->CLineRegReadEnableValue = 0x40;
            pI2CRegParams->CLineRegReadBitNum = 7;
            pI2CRegParams->CLineRegWriteBitNum = 4;
            pI2CRegParams->CLineRegWriteEnableMask = 0xDF;
            pI2CRegParams->CLineRegWriteEnableValue = 0x20;  
            
            pI2CRegParams->DLineRegType_Index = SR_4A;
            pI2CRegParams->DLineRegReadEnableMask = 0xFB;
            pI2CRegParams->DLineRegReadEnableValue = 0x04;
            pI2CRegParams->DLineRegReadBitNum = 3;
            pI2CRegParams->DLineRegWriteBitNum = 0;
            pI2CRegParams->DLineRegWriteEnableMask = 0xFD;
            pI2CRegParams->DLineRegWriteEnableValue = 0x02;
        }
        break;
    case I2CBUS_SCLGPIO2_SDAGPIO3://GPIO2-SCLK GPIO3-SDAT
        {  
            pI2CRegParams->CLineRegType_Index = CR_B_DA;
            pI2CRegParams->CLineRegReadEnableMask = 0xBF;
            pI2CRegParams->CLineRegReadEnableValue = 0x40;     
            pI2CRegParams->CLineRegReadBitNum = 7;
            pI2CRegParams->CLineRegWriteBitNum = 4;
            pI2CRegParams->CLineRegWriteEnableMask = 0xDF;
            pI2CRegParams->CLineRegWriteEnableValue = 0x20;  
            
            pI2CRegParams->DLineRegType_Index = SR_48;
            pI2CRegParams->DLineRegReadEnableMask = 0xFB;
            pI2CRegParams->DLineRegReadEnableValue = 0x04;
            pI2CRegParams->DLineRegReadBitNum = 3;
            pI2CRegParams->DLineRegWriteBitNum = 0;
            pI2CRegParams->DLineRegWriteEnableMask = 0xFD;
            pI2CRegParams->DLineRegWriteEnableValue = 0x02;                
        }
        break;
    default:
        {
            ASSERT(CBIOS_FALSE);
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),("Not support this I2C bus number %d!\n"), I2CBusNum));
            return CBIOS_FALSE;
        }
        break;
    }
    return CBIOS_TRUE;
}

static CBIOS_U32 cbI2CModule_DDCCI_StopService(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    if(cbI2CModule_Stop(pcbe, pCBModuleI2CParams) == CBIOS_FALSE) 
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_Stop failure!\n"));
        return DDCCII2C_STATUS_ERROR;
    }
    
    return DDCCII2C_STATUS_NOERROR;
}

static CBIOS_U32 cbI2CModule_DDCCI_Stop(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    cbI2CModule_NackWrite(pcbe, pCBModuleI2CParams);
    
    return cbI2CModule_DDCCI_StopService(pcbe, pCBModuleI2CParams);
}

static CBIOS_VOID cbI2CModule_DDCCI_StartService(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    cbI2CModule_Start(pcbe, pCBModuleI2CParams);
}

static CBIOS_U32 cbI2CModule_DDCCI_Null(PCBIOS_EXTENSION_COMMON pcbe, 
                                        PCBIOS_I2CCONTROL pI2CControl, 
                                        PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    if(pI2CControl->Flags & DDCCII2C_FLAGS_DATACHAINING)
    {
        if(cbI2CModule_DDCCI_StopService(pcbe, pCBModuleI2CParams) == DDCCII2C_STATUS_ERROR) 
        {
            return DDCCII2C_STATUS_ERROR;
        }

        cbI2CModule_DDCCI_StartService(pcbe, pCBModuleI2CParams);
    }

    if(pI2CControl->Flags & DDCCII2C_FLAGS_START)
    {
        cbI2CModule_DDCCI_StartService(pcbe, pCBModuleI2CParams);
    }

    if(pI2CControl->Flags & DDCCII2C_FLAGS_STOP)
    {
        if(cbI2CModule_DDCCI_Stop(pcbe, pCBModuleI2CParams) == DDCCII2C_STATUS_ERROR) 
        {
            return DDCCII2C_STATUS_ERROR;
        }
    }

    return DDCCII2C_STATUS_NOERROR;
}

static CBIOS_U32 cbI2CModule_DDCCI_Read(PCBIOS_EXTENSION_COMMON pcbe, 
                                        PCBIOS_I2CCONTROL pI2CControl, 
                                        PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    if(pI2CControl->Flags & DDCCII2C_FLAGS_DATACHAINING)
    {
        if(cbI2CModule_DDCCI_StopService(pcbe, pCBModuleI2CParams) == DDCCII2C_STATUS_ERROR) 
        {
            return DDCCII2C_STATUS_ERROR;
        }

        cbI2CModule_DDCCI_StartService(pcbe, pCBModuleI2CParams);
    }

    if(pI2CControl->Flags & DDCCII2C_FLAGS_START)
    {
        cbI2CModule_DDCCI_StartService(pcbe, pCBModuleI2CParams);
    }

    if(cbI2CModule_ReadByte(pcbe, pCBModuleI2CParams, &pI2CControl->Data) == CBIOS_FALSE) 
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_DDCCI_Read: cbI2CModule_ReadByte failure!\n"));
        return DDCCII2C_STATUS_ERROR;
    }

    if(pI2CControl->Flags & DDCCII2C_FLAGS_ACK)
    {
        if(cbI2CModule_AckWrite(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)  
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_DDCCI_Read: Ack Write error!\n"));
            return DDCCII2C_STATUS_ERROR;
        }
    }

    if(pI2CControl->Flags & DDCCII2C_FLAGS_STOP)
    {
        if(cbI2CModule_DDCCI_Stop(pcbe, pCBModuleI2CParams) == DDCCII2C_STATUS_ERROR) 
        {
            return DDCCII2C_STATUS_ERROR;
        }
    }

    return DDCCII2C_STATUS_NOERROR;
}

static CBIOS_U32 cbI2CModule_DDCCI_Write(PCBIOS_EXTENSION_COMMON pcbe, 
                                         PCBIOS_I2CCONTROL pI2CControl, 
                                         PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    if(pI2CControl->Flags & DDCCII2C_FLAGS_DATACHAINING)
    {
        if(cbI2CModule_DDCCI_StopService(pcbe, pCBModuleI2CParams) == DDCCII2C_STATUS_ERROR) 
        {
            return DDCCII2C_STATUS_ERROR;
        }

        cbI2CModule_DDCCI_StartService(pcbe, pCBModuleI2CParams);
    }

    if(pI2CControl->Flags & DDCCII2C_FLAGS_START)
    {
        cbI2CModule_DDCCI_StartService(pcbe, pCBModuleI2CParams);
    }

    cbI2CModule_WriteByte(pcbe, pCBModuleI2CParams, pI2CControl->Data);

    //if(pI2CControl->Flags & DDCCII2C_FLAGS_ACK)
    {
        if(cbI2CModule_AckRead(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)  
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_DDCCI_Write: Ack Read error!\n"));
            return DDCCII2C_STATUS_ERROR;
        }
    }

    if(pI2CControl->Flags & DDCCII2C_FLAGS_STOP)
    {
        if(cbI2CModule_DDCCI_Stop(pcbe, pCBModuleI2CParams) == DDCCII2C_STATUS_ERROR) 
        {
            return DDCCII2C_STATUS_ERROR;
        }
    }

    return DDCCII2C_STATUS_NOERROR;
}

static CBIOS_BOOL cbI2CModule_DDCCI_GetCookie(PCBIOS_U32 Cookie)
{
    *Cookie = 0x12345678;
    return CBIOS_TRUE;
}

/***********************************************************************************/
/*                              I2C Module Interfaces                              */
/***********************************************************************************/


/***************************************************************
Function:    cbI2CModule_ReadData

Description: Read data from I2C bus.

Input:       PCBIOS_VOID
             PCBIOS_MODULE_I2C_PARAMS

Output:      The data read is in pCBModuleI2CParams->Buffer

Return:      CBIOS_TURE if read data successfully
             CBIOS_FALSE if read data failed
***************************************************************/
CBIOS_BOOL cbI2CModule_ReadData(PCBIOS_VOID pvcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    CBIOS_U32 i = 0;
    CBIOS_U8  SlaveAddr = pCBModuleI2CParams->SlaveAddress;
    CBIOS_U8  Offset = pCBModuleI2CParams->OffSet;
    CBIOS_U32 BufferLen = pCBModuleI2CParams->BufferLen;
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL  bStatus = CBIOS_FALSE;

    cbTraceEnter(GENERIC);
    if (pCBModuleI2CParams->ConfigType == CONFIG_I2C_BY_BUSNUM)
    {
        if (cbI2CModule_GetI2CParams(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
        {
            return CBIOS_FALSE;
        }
        
    }
    
    cbI2CModule_Enable(pcbe, pCBModuleI2CParams);
    cbI2CModule_Start(pcbe, pCBModuleI2CParams);

    cbI2CModule_WriteByte(pcbe, pCBModuleI2CParams, SlaveAddr);
    if(cbI2CModule_AckRead(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        bStatus = CBIOS_FALSE;
        goto exit;
    }

    cbI2CModule_WriteByte(pcbe, pCBModuleI2CParams, Offset);
    if(cbI2CModule_AckRead(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        bStatus = CBIOS_FALSE;
        goto exit;
    }

    cbI2CModule_Start(pcbe, pCBModuleI2CParams);
    
    cbI2CModule_WriteByte(pcbe, pCBModuleI2CParams, SlaveAddr + 1);  //LSB=1, indicate read
    if(cbI2CModule_AckRead(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        bStatus = CBIOS_FALSE;
        goto exit;
    }

    for(i = 0; i < BufferLen-1; i++)
    {
        if(cbI2CModule_ReadByte(pcbe, pCBModuleI2CParams, &(pCBModuleI2CParams->Buffer[i])) == CBIOS_FALSE) 
        {
            bStatus = CBIOS_FALSE;
            goto exit;
        }
        if(cbI2CModule_AckWrite(pcbe, pCBModuleI2CParams) == CBIOS_FALSE) 
        {
            bStatus = CBIOS_FALSE;
            goto exit;
        }
    }
    if(cbI2CModule_ReadByte(pcbe, pCBModuleI2CParams, &(pCBModuleI2CParams->Buffer[i])) == CBIOS_FALSE) 
    {
        bStatus = CBIOS_FALSE;
        goto exit;
    }
           
    cbI2CModule_NackWrite(pcbe, pCBModuleI2CParams);

    if(cbI2CModule_Stop(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        bStatus = CBIOS_FALSE;
        goto exit;
    }
    bStatus = CBIOS_TRUE;
    
exit:
    if(!bStatus)
    {
        cbI2CModule_Stop(pcbe, pCBModuleI2CParams);
    }
    cbI2CModule_Disable(pcbe, pCBModuleI2CParams);
    cbTraceExit(GENERIC);
    return bStatus;
    
}

/***************************************************************
Function:    cbI2CModule_WriteData

Description: Write data to I2C bus.

Input:       PCBIOS_VOID
             PCBIOS_MODULE_I2C_PARAMS

Output:      

Return:      CBIOS_TURE if write data successfully
             CBIOS_FALSE if write data failed
***************************************************************/
CBIOS_BOOL cbI2CModule_WriteData(PCBIOS_VOID pvcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    CBIOS_U32 i = 0;
    CBIOS_U8  SlaveAddr = pCBModuleI2CParams->SlaveAddress;
    CBIOS_U8  Offset = pCBModuleI2CParams->OffSet;
    CBIOS_U32 BufferLen = pCBModuleI2CParams->BufferLen;
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL bStatus = CBIOS_FALSE;
  
    cbTraceEnter(GENERIC);
    if (pCBModuleI2CParams->ConfigType == CONFIG_I2C_BY_BUSNUM)
    {
        if (cbI2CModule_GetI2CParams(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
        {
            return CBIOS_FALSE;
        }
    }
    
    cbI2CModule_Enable(pcbe, pCBModuleI2CParams);
    cbI2CModule_Start(pcbe, pCBModuleI2CParams);

    cbI2CModule_WriteByte(pcbe, pCBModuleI2CParams, SlaveAddr);
    if(cbI2CModule_AckRead(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        bStatus = CBIOS_FALSE;
        goto exit;
    }

    cbI2CModule_WriteByte(pcbe, pCBModuleI2CParams, Offset);
    if(cbI2CModule_AckRead(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        bStatus = CBIOS_FALSE;
        goto exit;
    }

    for(i=0; i< BufferLen; i++)
    {
        cbI2CModule_WriteByte(pcbe, pCBModuleI2CParams, pCBModuleI2CParams->Buffer[i]);
        if(cbI2CModule_AckRead(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
        {
            bStatus = CBIOS_FALSE;
            goto exit;
        }
    }
    
    
    if(cbI2CModule_Stop(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        bStatus = CBIOS_FALSE;
        goto exit;
    }

    bStatus = CBIOS_TRUE;
    
exit:
    if(!bStatus)
    {
        cbI2CModule_Stop(pcbe, pCBModuleI2CParams);
    }
    cbI2CModule_Disable(pcbe, pCBModuleI2CParams);
    cbTraceExit(GENERIC);
    return bStatus;
    
}

/***************************************************************
Function:    cbI2CModule_WriteBits

Description: Write I2C data Bits(support both GPIO i2c and I2C port)

Input:       PCBIOS_VOID
             PCBIOS_MODULE_I2C_PARAMS
             I2CDataMask(1's in the mask byte indicate bits to protect, can not change)

Output:      

Return:      CBIOS_TURE if write bits successfully
             CBIOS_FALSE if write bits failed
***************************************************************/
CBIOS_BOOL cbI2CModule_WriteBits(PCBIOS_VOID pvcbe,
                                 PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams, 
                                 CBIOS_U8 I2CDataMask)
{
    CBIOS_U8 oldData, I2CData;
    CBIOS_U8* bufferTemp;
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    if (pCBModuleI2CParams->BufferLen != 1)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_WriteBits: can't write bits for more than one byte!\n"));
        ASSERT(CBIOS_FALSE);
    }

    bufferTemp = pCBModuleI2CParams->Buffer;   //store the old buffer
    I2CData = pCBModuleI2CParams->Buffer[0];

    cbTraceEnter(GENERIC);
    if (pCBModuleI2CParams->ConfigType == CONFIG_I2C_BY_BUSNUM)
    {
        if (cbI2CModule_GetI2CParams(pcbe, pCBModuleI2CParams) == CBIOS_FALSE) return CBIOS_FALSE;
    }

    pCBModuleI2CParams->Buffer = &oldData;
    pCBModuleI2CParams->BufferLen = 1;    //read/write one byte
    
    if (cbI2CModule_ReadData(pcbe, pCBModuleI2CParams))
    {
        CBIOS_U8 newData = (I2CData & (~I2CDataMask)) | (oldData & I2CDataMask);
        pCBModuleI2CParams->Buffer = bufferTemp;
        pCBModuleI2CParams->Buffer[0] = newData;
        return cbI2CModule_WriteData(pcbe, pCBModuleI2CParams);
    }
    else
    {
        return CBIOS_FALSE;
    }
    cbTraceExit(GENERIC);
}

/***************************************************************
Function:    cbI2CModule_ReadDDCCIData

Description: Read DDC-CI data from I2C bus.

Input:       PCBIOS_VOID
             PCBIOS_MODULE_I2C_PARAMS
             Flags

Output:      

Return:      CBIOS_TURE if read data successfully
             CBIOS_FALSE if read data failed
***************************************************************/
CBIOS_STATUS cbI2CModule_ReadDDCCIData(PCBIOS_VOID pvcbe, 
                                       PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams, 
                                       CBIOS_U32 Flags)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32     ulLength = 0, i = 0;
    CBIOS_STATUS  bStatus = CBIOS_ER_INTERNAL;
    CBIOS_U8      I2CBusNum = pCBModuleI2CParams->I2CBusNum;
    CBIOS_U8      SlaveAddress = pCBModuleI2CParams->SlaveAddress;
    CBIOS_U32     BufferLen = pCBModuleI2CParams->BufferLen;
    CBIOS_U8*     buffer = pCBModuleI2CParams->Buffer;
    CBIOS_STATUS  Status = CBIOS_ER_IO;
    
    cbTraceEnter(GENERIC);

    if ((I2CBusNum == I2CBUS_VIRTUAL_DP1) || (I2CBusNum == I2CBUS_VIRTUAL_DP2))
    {
        CBIOS_ACTIVE_TYPE    Device;
        PCBIOS_DEVICE_COMMON pDevCommon = CBIOS_NULL;
        CBIOS_MODULE_INDEX   DPModuleIndex = CBIOS_MODULE_INDEX_INVALID;

        if (I2CBusNum == I2CBUS_VIRTUAL_DP1)
        {
            Device = CBIOS_TYPE_DP1;
        }
        else if (I2CBusNum == I2CBUS_VIRTUAL_DP2)
        {
            Device = CBIOS_TYPE_DP2;
        }

        pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);
        DPModuleIndex = cbGetModuleIndex(pcbe, Device, CBIOS_MODULE_TYPE_DP);

        if (cbDPPort_IsDeviceInDpMode(pcbe, pDevCommon))
        {
#if DP_MONITOR_SUPPORT
            bStatus = cbDIU_DP_ReadDDCCIData(pvcbe, DPModuleIndex, pCBModuleI2CParams, Flags);
            return bStatus;
#endif
        }
        else // Dual Mode, just switch to normal I2C operation
        {
            pCBModuleI2CParams->I2CBusNum = (CBIOS_U8)pDevCommon->I2CBus;
        }
    }

    if (pCBModuleI2CParams->ConfigType == CONFIG_I2C_BY_BUSNUM)
    {
        if (cbI2CModule_GetI2CParams(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_ReadDDCCIData: get I2C params failed!\n"));
            return CBIOS_ER_INTERNAL;
        }
    }
    cbI2CModule_Enable(pcbe, pCBModuleI2CParams);
    cbI2CModule_Start(pcbe, pCBModuleI2CParams);

    cbI2CModule_WriteByte(pcbe, pCBModuleI2CParams, SlaveAddress);
    if(cbI2CModule_AckRead(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        Status = CBIOS_ER_IO;
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_ReadDDCCIData: Ack Read error!\n"));
        goto exit;
    }
    for(i = 0; i < BufferLen-1; i++)
    {
        if(cbI2CModule_ReadByte(pcbe, pCBModuleI2CParams, &buffer[i]) == CBIOS_FALSE)
        {
            Status = CBIOS_ER_IO;
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_ReadDDCCIData: cbI2CModule_ReadByte failure!\n"));
            goto exit;
        }    
        if(cbI2CModule_AckWrite(pcbe, pCBModuleI2CParams) == CBIOS_FALSE) 
        {
            Status = CBIOS_ER_IO;
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_ReadDDCCIData: Ack Write error!\n"));
            goto exit;
        }
        if ((i == 1) && (Flags & 0x000000001))
        {
            ulLength = buffer[i] & ~0x80;
            if (ulLength > (BufferLen - 3))
            {
                cbI2CModule_Stop(pcbe, pCBModuleI2CParams);
                cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_ReadDDCCIData: buffer is too small!\n"));
                Status = CBIOS_ER_BUFFER_TOO_SMALL;
                goto exit;
            }
            BufferLen = ulLength + 3;
        }
    }
    if(cbI2CModule_ReadByte(pcbe, pCBModuleI2CParams, &buffer[i]) == CBIOS_FALSE) 
    {
        Status = CBIOS_ER_IO;
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_ReadDDCCIData: cbI2CModule_ReadByte failure!\n"));
        goto exit;
    }
           
    cbI2CModule_NackWrite(pcbe, pCBModuleI2CParams);

    if(cbI2CModule_Stop(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
    {
        Status = CBIOS_ER_IO;
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_ReadDDCCIData: cbI2CModule_Stop failure!\n"));
        goto exit;
    }
    Status = CBIOS_OK;
exit:
    cbI2CModule_Stop(pcbe, pCBModuleI2CParams);
    cbI2CModule_Disable(pcbe, pCBModuleI2CParams);
    cbTraceExit(GENERIC);
    return Status;
    
}

/***************************************************************
Function:    cbI2CModule_WriteDDCCIData

Description: Write DDC-CI data to I2C bus.

Input:       PCBIOS_VOID
             PCBIOS_MODULE_I2C_PARAMS

Output:      

Return:      CBIOS_TURE if write data successfully
             CBIOS_FALSE if write data failed
***************************************************************/
CBIOS_BOOL cbI2CModule_WriteDDCCIData(PCBIOS_VOID pvcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U32       i;
    CBIOS_BOOL      bStatus = CBIOS_FALSE;
    CBIOS_U8        I2CBusNum = pCBModuleI2CParams->I2CBusNum;
    CBIOS_U8        SlaveAddress = pCBModuleI2CParams->SlaveAddress;

    cbTraceEnter(GENERIC);
    if ((I2CBusNum == I2CBUS_VIRTUAL_DP1) || (I2CBusNum == I2CBUS_VIRTUAL_DP2))
    {
        CBIOS_ACTIVE_TYPE    Device;
        PCBIOS_DEVICE_COMMON pDevCommon = CBIOS_NULL;
        CBIOS_MODULE_INDEX   DPModuleIndex = CBIOS_MODULE_INDEX_INVALID;

        if (I2CBusNum == I2CBUS_VIRTUAL_DP1)
        {
            Device = CBIOS_TYPE_DP1;
        }
        else if (I2CBusNum == I2CBUS_VIRTUAL_DP2)
        {
            Device = CBIOS_TYPE_DP2;
        }

        pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);
        DPModuleIndex = cbGetModuleIndex(pcbe, Device, CBIOS_MODULE_TYPE_DP);

        if (cbDPPort_IsDeviceInDpMode(pcbe, pDevCommon))
        {
#if DP_MONITOR_SUPPORT
            bStatus = cbDIU_DP_WriteDDCCIData(pvcbe, DPModuleIndex, pCBModuleI2CParams);
            return bStatus;
#endif
        }
        else // Dual Mode, just switch to normal I2C operation
        {
            pCBModuleI2CParams->I2CBusNum = (CBIOS_U8)pDevCommon->I2CBus;
        }
    }

    if (pCBModuleI2CParams->ConfigType == CONFIG_I2C_BY_BUSNUM)
    {
        if (cbI2CModule_GetI2CParams(pcbe, pCBModuleI2CParams) == CBIOS_FALSE)
        {
            return CBIOS_FALSE;
        }
    }

    cbI2CModule_Enable(pcbe, pCBModuleI2CParams);
    cbI2CModule_Start(pcbe, pCBModuleI2CParams);

    //write slave address
    cbI2CModule_WriteByte(pcbe, pCBModuleI2CParams, SlaveAddress);
    if(cbI2CModule_AckRead(pcbe, pCBModuleI2CParams) == CBIOS_FALSE) 
    {
        bStatus = CBIOS_FALSE;
        goto exit;
    }

    for(i=0; i< pCBModuleI2CParams->BufferLen - 1; i++)
    {
        cbI2CModule_WriteByte(pcbe, pCBModuleI2CParams, pCBModuleI2CParams->Buffer[i]);
        if(cbI2CModule_AckRead(pcbe, pCBModuleI2CParams) == CBIOS_FALSE) 
        {
            bStatus = CBIOS_FALSE;
            goto exit;
        }
    }    
    cbI2CModule_WriteByte(pcbe, pCBModuleI2CParams, pCBModuleI2CParams->Buffer[i]);
    cbI2CModule_AckRead(pcbe, pCBModuleI2CParams);
    if(SlaveAddress != 0x60)
    {
        if(cbI2CModule_Stop(pcbe, pCBModuleI2CParams) == CBIOS_FALSE) 
        {
            bStatus = CBIOS_FALSE;
            goto exit;
        }
    }
    bStatus = CBIOS_TRUE;
exit:
    cbI2CModule_Disable(pcbe, pCBModuleI2CParams);
    cbTraceExit(GENERIC);
    return bStatus;
}

CBIOS_STATUS cbI2CModule_DDCCI_OPEN(PCBIOS_VOID pvcbe, CBIOS_BOOL bOpen, PCBIOS_I2CCONTROL pI2CControl, CBIOS_U8 I2CBUSNum)
{
    CBIOS_MODULE_I2C_PARAMS I2CParams;
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    cb_memset(&I2CParams, 0, sizeof(CBIOS_MODULE_I2C_PARAMS));
    I2CParams.I2CBusNum = I2CBUSNum;
    
#ifdef CHECK_CHIPENABLE
    if (!cbHWIsChipEnable(pcbe))
        return CBIOS_ER_CHIPDISABLE;
#endif
    
    pI2CControl->Status = DDCCII2C_STATUS_ERROR;

    if ((pI2CControl->ClockRate > DDCCIMAX_CLOCK_RATE) || (pI2CControl->ClockRate == 0))
    {
        pI2CControl->ClockRate = DDCCIMAX_CLOCK_RATE;
    }
    
    if (cbI2CModule_GetI2CParams(pcbe, &I2CParams))
    {
        if (bOpen)
        {
            cbI2CModule_Enable(pcbe, &I2CParams);
            if (cbI2CModule_DDCCI_GetCookie(&pI2CControl->dwCookie))
            {
                pI2CControl->Status = DDCCII2C_STATUS_NOERROR;
            }
        }
        else
        {
            cbI2CModule_Disable(pcbe, &I2CParams);
            pI2CControl->dwCookie = 0;
            pI2CControl->Status = DDCCII2C_STATUS_NOERROR;
        }

        if(!(pI2CControl->Status == DDCCII2C_STATUS_NOERROR)) 
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_DDCCI_OPEN: funciton failure!\n"));
        return pI2CControl->Status;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_DDCCI_OPEN: Not support this I2C bus number!\n"));
        return DDCCII2C_STATUS_ERROR;
    }
}

CBIOS_STATUS cbI2CModule_DDCCI_ACCESS(PCBIOS_VOID pvcbe, PCBIOS_I2CCONTROL pI2CControl, CBIOS_U8 I2CBUSNum)
{
    CBIOS_MODULE_I2C_PARAMS I2CParams;
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    
    cb_memset(&I2CParams, 0, sizeof(CBIOS_MODULE_I2C_PARAMS));
    I2CParams.I2CBusNum = I2CBUSNum;
    
#ifdef CHECK_CHIPENABLE
    if (!cbHWIsChipEnable(pcbe))
        return CBIOS_ER_CHIPDISABLE;
#endif
    
    pI2CControl->Status = DDCCII2C_STATUS_NOERROR;
    
    if ((pI2CControl->ClockRate > DDCCIMAX_CLOCK_RATE) || (pI2CControl->ClockRate == 0))
    {
        pI2CControl->ClockRate = DDCCIMAX_CLOCK_RATE;
    }

    if (cbI2CModule_GetI2CParams(pcbe, &I2CParams))
    {
        switch(pI2CControl->Command)
        {
        case DDCCII2C_COMMAND_NULL:
            pI2CControl->Status = cbI2CModule_DDCCI_Null(pcbe, pI2CControl, &I2CParams);
            break;

        case DDCCII2C_COMMAND_READ:
            pI2CControl->Status = cbI2CModule_DDCCI_Read(pcbe, pI2CControl, &I2CParams);
            break;

        case DDCCII2C_COMMAND_WRITE:
            pI2CControl->Status = cbI2CModule_DDCCI_Write(pcbe, pI2CControl, &I2CParams);
            break;

        case DDCCII2C_COMMAND_RESET:
            pI2CControl->Status = cbI2CModule_DDCCI_Stop(pcbe, &I2CParams);
            break;

        case DDCCII2C_COMMAND_STATUS:
            break;

        default:
            pI2CControl->Status = DDCCII2C_STATUS_ERROR;
            break;
        }

        if(!(pI2CControl->Status == DDCCII2C_STATUS_NOERROR)) 
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_DDCCI_ACCESS: funciton failure!\n"));
        return pI2CControl->Status;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbI2CModule_DDCCI_ACCESS: Not support this I2C bus number!\n"));
        return DDCCII2C_STATUS_ERROR;
    }
}

CBIOS_VOID cbI2CModule_HDCPI2CEnableDisable(PCBIOS_VOID pvcbe, PCBIOS_MODULE_I2C_PARAMS pCBModuleI2CParams, CBIOS_BOOL bIsEnable)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;

    if (pCBModuleI2CParams->ConfigType == CONFIG_I2C_BY_BUSNUM)
    {
        cbI2CModule_GetI2CParams(pcbe, pCBModuleI2CParams);
    }

    if (bIsEnable)
    {
        cbI2CModule_Enable(pcbe, pCBModuleI2CParams);
    }
    else
    {
        cbI2CModule_Disable(pcbe, pCBModuleI2CParams);
    }
}
