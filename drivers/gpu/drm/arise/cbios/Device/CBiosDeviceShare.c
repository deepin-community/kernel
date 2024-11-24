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
** Device port sharing function implementation.
**
** NOTE:
**
******************************************************************************/

#include "CBiosDeviceShare.h"
#include "CBiosChipShare.h"

CBIOS_U8 FPGAHDMIEdid[256] =
{
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x21, 0x34, 0x03, 0x7D, 0x43, 0x41, 0x32, 0x01,
    0x1E, 0x11, 0x01, 0x03, 0x80, 0x34, 0x20, 0x64, 0x0A, 0xEF, 0x95, 0xA3, 0x54, 0x4C, 0x9B, 0x26,
    0x0F, 0x50, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD5, 0x09, 0x80, 0xA0, 0x20, 0xE0, 0x2D, 0x10, 0x10, 0x60,
    0xA2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x01, 0x1D, 0x00, 0x72, 0x51, 0xD0, 0x1E, 0x20,
    0x6E, 0x28, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38,
    0x2D, 0x40, 0x58, 0x2C, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC,
    0x00, 0x57, 0x32, 0x34, 0x30, 0x44, 0x20, 0x44, 0x56, 0x49, 0x0A, 0x20, 0x20, 0x20, 0x01, 0xFB,
    0x02, 0x03, 0x1C, 0x71, 0x23, 0x09, 0x07, 0x07, 0x49, 0x90, 0x14, 0x13, 0x12, 0x05, 0x04, 0x03,
    0x02, 0x01, 0x65, 0x03, 0x0C, 0x00, 0x10, 0x00, 0x83, 0x01, 0x00, 0x00, 0x8C, 0x0A, 0xD0, 0x90,
    0x20, 0x40, 0x31, 0x20, 0x0C, 0x40, 0x55, 0x00, 0x13, 0x8E, 0x21, 0x00, 0x00, 0x18, 0x01, 0x1D,
    0x80, 0x18, 0x71, 0x1C, 0x16, 0x20, 0x58, 0x2C, 0x25, 0x00, 0xC4, 0x8E, 0x21, 0x00, 0x00, 0x9E,
    0x01, 0x1D, 0x00, 0x72, 0x51, 0xD0, 0x1E, 0x20, 0x6E, 0x28, 0x55, 0x00, 0xC4, 0x8E, 0x21, 0x00,
    0x00, 0x1E, 0x8C, 0x0A, 0xD0, 0x8A, 0x20, 0xE0, 0x2D, 0x10, 0x10, 0x3E, 0x96, 0x00, 0xC4, 0x8E,
    0x21, 0x00, 0x00, 0x18, 0x8C, 0x0A, 0xD0, 0x8A, 0x20, 0xE0, 0x2D, 0x10, 0x10, 0x3E, 0x96, 0x00,
    0x13, 0x8E, 0x21, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC4,
};


#if IS_SUPPORT_4K_MODE
CBIOS_U8 Fake4KEdid[256] =
{
    0x0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0, 0x36, 0x74, 0x30, 0x0, 0x1, 0x0, 0x0, 0x0,
    0xA, 0x16, 0x1, 0x3, 0x80, 0x73, 0x41, 0x78, 0xA, 0xCF, 0x74, 0xA3, 0x57, 0x4C, 0xB0, 0x23,
    0x9, 0x48, 0x4C, 0x21, 0x8, 0x0, 0x81, 0x80, 0x45, 0x40, 0x61, 0x40, 0x95, 0x0, 0x1, 0x1,
    0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x2, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C,
    0x45, 0x0, 0xC4, 0x8E, 0x21, 0x0, 0x0, 0x1E, 0x66, 0x21, 0x50, 0xB0, 0x51, 0x0, 0x1B, 0x30,
    0x40, 0x70, 0x36, 0x0, 0xC4, 0x8E, 0x21, 0x0, 0x0, 0x1E, 0x0, 0x0, 0x0, 0xFC, 0x0, 0x4D,
    0x53, 0x74, 0x61, 0x72, 0x20, 0x44, 0x65, 0x6D, 0x6F, 0xA, 0x20, 0x20, 0x0, 0x0, 0x0, 0xFD,
    0x0, 0x32, 0x4B, 0x1E, 0x50, 0x17, 0x0, 0xA, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x1, 0xF1,
    0x2, 0x3, 0x30, 0xF2, 0x4D, 0x1, 0x3, 0x4, 0x5, 0x7, 0x90, 0x12, 0x13, 0x14, 0x16, 0x9F,
    0x20, 0x22, 0x26, 0x9, 0x7, 0x7, 0x11, 0x17, 0x50, 0x83, 0x1, 0x0, 0x0, 0x72, 0x3, 0xC,
    0x0, 0x40, 0x0, 0xB8, 0x44, 0x20, 0xC0, 0x84, 0x1, 0x2, 0x3, 0x4, 0x1, 0x41, 0x0, 0x0,
    0x8C, 0xA, 0xD0, 0x8A, 0x20, 0xE0, 0x2D, 0x10, 0x10, 0x3E, 0x96, 0x0, 0xC4, 0x8E, 0x21, 0x0,
    0x0, 0x18, 0x8C, 0xA, 0xD0, 0x90, 0x20, 0x40, 0x31, 0x20, 0xC, 0x40, 0x55, 0x0, 0xC4, 0x8E,
    0x21, 0x0, 0x0, 0x18, 0x1, 0x1D, 0x0, 0xBC, 0x52, 0xD0, 0x1E, 0x20, 0xB8, 0x28, 0x55, 0x40,
    0xC4, 0x8E, 0x21, 0x0, 0x0, 0x1E, 0x1, 0x1D, 0x80, 0xD0, 0x72, 0x1C, 0x16, 0x20, 0x10, 0x2C,
    0x25, 0x80, 0xC4, 0x8E, 0x21, 0x0, 0x0, 0x9E, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3F,
};
#endif


static CBIOS_BOOL cbGetEDID(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_U8 EDIDData[], CBIOS_U32 ulReadEdidOffset, CBIOS_U32 ulBufferSize)
{
    //CBIOS_U8 edid_address[] = {0xA0,0x00,0xA2,0x20,0xA6,0x20};
    CBIOS_U8 byTemp;
    CBIOS_U8 I2CBUSNum = (CBIOS_U8)pDevCommon->I2CBus;
    CBIOS_U8 i = 0;
    CBIOS_U32 j;
    CBIOS_BOOL  bRet = CBIOS_FALSE;
    CBIOS_U32 BlockSize = 128;
    CBIOS_U32 ExtBlockCount = 0;
    CBIOS_U16 DCLK = 0, XResolution = 0, YResolution = 0;
    CBIOS_U32 ulEDIDLen;
    PCBIOS_VOID pDPMonitorContext = CBIOS_NULL;
    CBIOS_BOOL bDPMode = CBIOS_FALSE;
    CBIOS_ACTIVE_TYPE Device = pDevCommon->DeviceType;

    if(ulBufferSize == 0)
        return CBIOS_FALSE;

    cb_memset(EDIDData, 0, ulBufferSize);

    if (Device & ALL_DP_TYPES)
    {
        if (!cbDPPort_IsDeviceInDpMode(pcbe, pDevCommon))//dual mode
        {
            bDPMode = CBIOS_FALSE;
            I2CBUSNum = (CBIOS_U8)pDevCommon->I2CBus;
        }
        else//DP mode
        {
            pDPMonitorContext = cbGetDPMonitorContext(pcbe, pDevCommon);
            bDPMode = CBIOS_TRUE;
        }
    }
    else
    {
        bDPMode = CBIOS_FALSE;
    }

    //deal with special request, offset is not 0, read count is not times of 1 block.
    if((ulReadEdidOffset != 0) || (ulBufferSize % 128 != 0))
    {
        if (bDPMode)
        {
            #if DP_MONITOR_SUPPORT
            if (pDPMonitorContext)
            {
                bRet = cbDPMonitor_AuxReadEDIDOffset(pcbe, pDPMonitorContext, EDIDData, ulBufferSize, ulReadEdidOffset);
            }
            #endif
        }
        else
        {
            bRet = cbHWReadEDID(pcbe, I2CBUSNum, EDIDData, ulReadEdidOffset, ulBufferSize, 0);
        }

    }
    else//read full EDID
    {
        if (bDPMode)//DP
        {
            #if DP_MONITOR_SUPPORT
            if (pDPMonitorContext)
            {
                bRet = cbDPMonitor_AuxReadEDID(pcbe, pDPMonitorContext, EDIDData, ulBufferSize);
            }
            #endif
        }
        else//Non DP devices
        {
            // Get block 0
            bRet = cbHWReadEDID(pcbe, I2CBUSNum, EDIDData, 0, BlockSize, 0);

            if (bRet == CBIOS_FALSE || !cbEDIDModule_IsEDIDHeaderValid(EDIDData, BlockSize))
            {
                bRet = CBIOS_FALSE;
                goto Exit;
            }

            // Get block 1
            if (EDIDData[0x7E] && (ulBufferSize >= 256))
            {
                bRet = cbHWReadEDID(pcbe, I2CBUSNum, EDIDData+BlockSize, 128, BlockSize, 0);
            }

            if (EDIDData[0x7E] && ((EDIDData[0x84]&0xE0) == 0xE0) && (EDIDData[0x85] == 0x78))  // HDMI2.1 EEODB
            {
                ExtBlockCount = EDIDData[0x86];
            }
            else
            {
                ExtBlockCount = EDIDData[0x7E];
            }

            if(ExtBlockCount > CBIOS_EDIDMAXBLOCKCOUNT - 1)
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbGetEDID: ExtBlockCount > 7, need refine!\n"));
                ExtBlockCount = CBIOS_EDIDMAXBLOCKCOUNT - 1;
                //ASSERT(CBIOS_FALSE);
            }

            pDevCommon->TotalBlockNum = ExtBlockCount + 1;

            // Get block 2 ~ n
            for ( i = 2; i <= ExtBlockCount; i++)
            {
                bRet = cbHWReadEDID(pcbe, I2CBUSNum, &EDIDData[i*BlockSize], (i-2)*128, BlockSize, i/2);
            }

Exit:

            if (bRet && (ulBufferSize % 128) == 0)
            {
#if 0
                //cbDumpBuffer(pcbe, EDIDData, ulBufferSize);
                cbDumpBuffer(pcbe, EDIDData, (ExtBlockCount+1)*128);

#endif

                byTemp = 0; // Check checksum
                ulEDIDLen = EDID_BLOCK_SIZE_SPEC;      // just check first 128 bytes
                for (j = 0; j < ulEDIDLen; j ++)
                {
                    byTemp += EDIDData[j];
                }

                if (byTemp != 0)
                {
                    cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING),"cbGetEDID: EDID is got, but check sum error, check detailed timming!\n"));
                    DCLK = EDIDData[0x37];                                      //check DCLK and X/Y Resolution
                    DCLK = DCLK << 8;
                    DCLK |= EDIDData[0x36];

                    XResolution = EDIDData[0x3A] & 0xF0;
                    XResolution = XResolution << 4;
                    XResolution |= EDIDData[0x38];

                    YResolution = EDIDData[0x3D] & 0xF0;
                    YResolution = YResolution << 4;
                    YResolution |= EDIDData[0x3B];

                    if((DCLK > 0)&&(XResolution > 0)&&(YResolution > 0))
                    {
                        //Set checksum to the correct value
                        EDIDData[ulEDIDLen - 1] = (EDIDData[ulEDIDLen - 1] + 0xFF - byTemp + 1) & 0xFF;
                    }
                    else
                    {
                        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"cbGetEDID: Detailed timming is invalid, zero it!\n"));
                        bRet = CBIOS_FALSE;
                    }
                }
            }
        }

        if (!bRet)
        {
            cb_memset(EDIDData, 0, ulBufferSize);
        }
    }

    return bRet;
}

static CBIOS_BOOL cbGetDeviceSignature(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_DEVICE_SIGNATURE pDevSignatre)
{
    CBIOS_U32  i = 0;
    CBIOS_BOOL bResult = CBIOS_FALSE;
    CBIOS_U32  EDIDBlockNum = 0;

    if(CBIOS_NULL == pDevSignatre)
    {
        bResult = CBIOS_FALSE;
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbGetDeviceSignature: the 3rd param is a NULL pointer\n"));
    }
    else
    {
        cb_memset(pDevSignatre, 0, sizeof(CBIOS_DEVICE_SIGNATURE));

        // get the monitor ID
        if(cbGetEDID(pcbe, pDevCommon, pDevSignatre->MonitorID, MONITORIDINDEX, MONITORIDLENGTH))
        {
            // get the flag of checksum in block 0
            if(cbGetEDID(pcbe, pDevCommon, pDevSignatre->ExtFlagChecksum[0], EXTFLAGCHECKSUMINDEX, EXTFLAGCHECKSUMLENTH))
            {
                EDIDBlockNum = (CBIOS_U32)pDevSignatre->ExtFlagChecksum[0][0];

                if (0 == EDIDBlockNum) // if EDIDBlockNum = 0, just return
                {
                    bResult = CBIOS_TRUE;
                }
                else
                {
                    // get the flag of checksum in the remaining blocks
                    if (EDIDBlockNum > CBIOS_EDIDMAXBLOCKCOUNT - 1)
                    {
                        cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG),"cbGetDeviceSignature: EDIDBlockNum > 7, here read the first 8 blocks!\n"));
                        EDIDBlockNum = CBIOS_EDIDMAXBLOCKCOUNT - 1;
                    }

                    for (i = 1; i < EDIDBlockNum + 1; i++)
                    {
                        if(!cbGetEDID(pcbe, pDevCommon, pDevSignatre->ExtFlagChecksum[i], EDID_BLOCK_SIZE_SPEC * i + EXTFLAGCHECKSUMINDEX, EXTFLAGCHECKSUMLENTH))
                        {
                            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "cbGetDeviceSignature: cannot read the checksum flag in EDID block %x\n", i));
                            bResult = CBIOS_FALSE;
                            break;
                        }
                        else
                        {
                            bResult = CBIOS_TRUE;
                        }
                    }
                }
            }
            else
            {
                bResult = CBIOS_FALSE;
            }
        }
        else
        {
            bResult = CBIOS_FALSE;
        }
    }

    return bResult;
}

//if the monitor serial number is not the same as the pcbe, the device is changed.
//return: CBIOS_TRUE - device changed
static CBIOS_BOOL cbIsDeviceChanged(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    CBIOS_DEVICE_SIGNATURE DeviceSignature;
    CBIOS_BOOL             bResult = CBIOS_TRUE;
    CBIOS_U32              i = 0;
    CBIOS_S32              ChecksumFlagChanged = 0;

    //If we can not get the signature from the edid, we consider that the device is changed
    if(cbEDIDModule_IsEDIDValid(pDevCommon->EdidData))
    {
        // If the new device has not edid, clear device signature
        if (!cbGetDeviceSignature(pcbe, pDevCommon, &DeviceSignature))
        {
            cb_memset(&DeviceSignature, 0, sizeof(CBIOS_DEVICE_SIGNATURE));
        }

        //Compare the signature--checksum
        for (i = 0; i < CBIOS_EDIDMAXBLOCKCOUNT; i++)
        {
            ChecksumFlagChanged = cb_memcmp(pDevCommon->ConnectedDevSignature.ExtFlagChecksum[i], DeviceSignature.ExtFlagChecksum[i], EXTFLAGCHECKSUMLENTH);
            if (0 != ChecksumFlagChanged)
            {
                bResult = CBIOS_TRUE;
                break;
            }
            else
            {
                bResult = CBIOS_FALSE;
            }
        }

        //Compare the signature--monitor ID
        if ((0 == ChecksumFlagChanged) && (0 == cb_memcmp(pDevCommon->ConnectedDevSignature.MonitorID, DeviceSignature.MonitorID, MONITORIDLENGTH)))
        {
            bResult = CBIOS_FALSE;
        }
        else
        {
            bResult = CBIOS_TRUE;
        }
    }
    else
    {
        bResult = CBIOS_TRUE;
    }
    return bResult;
}

CBIOS_BOOL cbGetDeviceEDID(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, CBIOS_BOOL *pIsDevChanged, CBIOS_U32 FullDetect)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_BOOL              bGotEDID = CBIOS_FALSE;
    CBIOS_BOOL              bNeedUpdateSignature = CBIOS_FALSE;
    CBIOS_UCHAR            *pEdidBuffer = CBIOS_NULL;
    const CBIOS_U32         EdidBufferSize = CBIOS_EDIDDATABYTE;

    if (pDevCommon == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pDevCommon is NULL!\n", FUNCTION_NAME));
        return bGotEDID;
    }

    // check if use hardcode EDID
    if (pcbe->DevicesHardcodedEdid & pDevCommon->DeviceType)
    {
        bGotEDID = CBIOS_TRUE;
        bNeedUpdateSignature= CBIOS_TRUE;
        *pIsDevChanged = CBIOS_TRUE;
        cb_memcpy(pDevCommon->EdidData, pcbe->EdidFromRegistry, EdidBufferSize);
    }
    // check if use faked edid sending from the upper layer
    else if ((pDevCommon->isFakeEdid) && (cbEDIDModule_IsEDIDValid(pDevCommon->EdidData)))
    {
        bGotEDID = CBIOS_TRUE;
        bNeedUpdateSignature = CBIOS_TRUE;
        *pIsDevChanged = CBIOS_TRUE;
    }
    // read EDID from the monitor side
    else
    {

        pEdidBuffer = cb_AllocateNonpagedPool(EdidBufferSize);
        if(pEdidBuffer == CBIOS_NULL)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pEdidBuffer allocate error.\n", FUNCTION_NAME));
            bGotEDID = CBIOS_FALSE;
            return bGotEDID;
        }

        //some hdmi cts items like HF1-23 need read full edid when monitor issue a hotplug
        if(FullDetect)
        {
            if (cbGetEDID(pcbe, pDevCommon, pEdidBuffer, 0, EdidBufferSize) && cbEDIDModule_IsEDIDValid(pEdidBuffer))
            {
                if(cbIsDeviceChangedByEdid(pcbe, pDevCommon, pEdidBuffer))
                {
                    *pIsDevChanged = CBIOS_TRUE;
                    bNeedUpdateSignature = CBIOS_TRUE;
                    bGotEDID = CBIOS_TRUE;
                    cb_memcpy(pDevCommon->EdidData, pEdidBuffer, EdidBufferSize);
                }
                else
                {
                    *pIsDevChanged = CBIOS_FALSE;
                    bNeedUpdateSignature = CBIOS_FALSE;
                    bGotEDID = CBIOS_TRUE;
                }
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "%s: cannot get edid or edid is invalid.\n", FUNCTION_NAME));
                bGotEDID = CBIOS_FALSE;
            }
        }
        else
        {
            if (cbIsDeviceChanged(pcbe, pDevCommon))
            {
                if (cbGetEDID(pcbe, pDevCommon, pEdidBuffer, 0, EdidBufferSize) && cbEDIDModule_IsEDIDValid(pEdidBuffer))
                {
                    bNeedUpdateSignature = CBIOS_TRUE;
                    *pIsDevChanged = CBIOS_TRUE;
                    bGotEDID = CBIOS_TRUE;
                }
                else
                {
                    cbDebugPrint((MAKE_LEVEL(GENERIC, DEBUG), "%s: cannot get edid or edid is invalid.\n", FUNCTION_NAME));
                    bGotEDID = CBIOS_FALSE;
                }

                if (bGotEDID)
                {
                    cb_memcpy(pDevCommon->EdidData, pEdidBuffer, EdidBufferSize);
                }
            }
            else
            {
                *pIsDevChanged = CBIOS_FALSE;
                bNeedUpdateSignature = CBIOS_FALSE;
                bGotEDID = CBIOS_TRUE;
            }
        }

        if (pEdidBuffer)
        {
            cb_FreePool(pEdidBuffer);
        }


    }

    // update the signature
    if (bNeedUpdateSignature)
    {
        cbUpdateDeviceSignature(pcbe, pDevCommon);
    }

    return bGotEDID;
}

CBIOS_BOOL cbIsDeviceChangedByEdid(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon, PCBIOS_UCHAR pEDIDData)
{
    CBIOS_BOOL bRet = CBIOS_TRUE;
    CBIOS_S32  result = 0;
    CBIOS_S32  i = 0;
    CBIOS_S32  ChecksumFlagChanged = 0;

    if (CBIOS_NULL == pEDIDData)
    {
        bRet = CBIOS_TRUE;
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: the 2nd param is a NULL pointer\n", FUNCTION_NAME));
    }
    else
    {
        result = cb_memcmp(pDevCommon->ConnectedDevSignature.MonitorID,
            &pEDIDData[MONITORIDINDEX], MONITORIDLENGTH);
        if(result == 0)
        {
            for (i = 0; i < CBIOS_EDIDMAXBLOCKCOUNT; i++)
            {
                ChecksumFlagChanged = cb_memcmp(pDevCommon->ConnectedDevSignature.ExtFlagChecksum[i],
                              &pEDIDData[i * EDID_BLOCK_SIZE_SPEC + EXTFLAGCHECKSUMINDEX], EXTFLAGCHECKSUMLENTH);
                if (0 != ChecksumFlagChanged)
                {
                    bRet = CBIOS_TRUE;
                    break;
                }
                else
                {
                    bRet = CBIOS_FALSE;
                }
            }
        }
        else
        {
            bRet = CBIOS_TRUE;
        }
    }
    return bRet;
}

CBIOS_BOOL cbUpdateDeviceSignature(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    CBIOS_U32 EDIDBlockNum = 0, i = 0;
    CBIOS_UCHAR *pEDID = CBIOS_NULL;

    if (pDevCommon == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: the 2nd param is NULL!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    pEDID = pDevCommon->EdidData;

    cb_memset(&pDevCommon->ConnectedDevSignature, 0, sizeof(CBIOS_DEVICE_SIGNATURE));
    cb_memcpy(pDevCommon->ConnectedDevSignature.MonitorID, &pEDID[MONITORIDINDEX], MONITORIDLENGTH);
    // copy the flag of checksum in block 0
    cb_memcpy(pDevCommon->ConnectedDevSignature.ExtFlagChecksum[0],
        &pEDID[EXTFLAGCHECKSUMINDEX], EXTFLAGCHECKSUMLENTH);
    EDIDBlockNum = (CBIOS_U32)pDevCommon->ConnectedDevSignature.ExtFlagChecksum[0][0];
    if (0 < EDIDBlockNum)
    {
        // get the flag of checksum in the remaining blocks
        if (EDIDBlockNum > CBIOS_EDIDMAXBLOCKCOUNT - 1)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING),"%s: EDIDBlockNum > 3, here copy the first 4 blocks!\n", FUNCTION_NAME));
            EDIDBlockNum = CBIOS_EDIDMAXBLOCKCOUNT - 1;
        }

        // copy the flag of checksum in remaining blocks
        for (i = 1; i < EDIDBlockNum + 1; i++)
        {
            cb_memcpy(pDevCommon->ConnectedDevSignature.ExtFlagChecksum[i],
                &pEDID[i * EDID_BLOCK_SIZE_SPEC + EXTFLAGCHECKSUMINDEX], EXTFLAGCHECKSUMLENTH);
        }
    }

    return CBIOS_TRUE;
}

CBIOS_MONITOR_TYPE cbGetSupportMonitorType(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE devices)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_MONITOR_TYPE  MonitorType = CBIOS_MONITOR_TYPE_NONE;

    if(devices & CBIOS_TYPE_CRT)
    {
        MonitorType |= CBIOS_MONITOR_TYPE_CRT;
    }

    if(devices & CBIOS_TYPE_TV)
    {
        MonitorType |= CBIOS_MONITOR_TYPE_TV;
    }

    if(devices & CBIOS_TYPE_HDTV)
    {
        MonitorType |= CBIOS_MONITOR_TYPE_HDTV;
    }

    if(devices & CBIOS_TYPE_DSI)
    {
        MonitorType |= CBIOS_MONITOR_TYPE_PANEL;
    }

    if(devices & CBIOS_TYPE_DP1)
    {
        if (pcbe->FeatureSwitch.IsEDP1Enabled)
        {
            MonitorType |= CBIOS_MONITOR_TYPE_PANEL;
        }
        else
        {
            MonitorType |= CBIOS_MONITOR_TYPE_DP | CBIOS_MONITOR_TYPE_DVI | CBIOS_MONITOR_TYPE_HDMI;
        }
    }

    if(devices & CBIOS_TYPE_DP2)
    {
        if (pcbe->FeatureSwitch.IsEDP2Enabled)
        {
            MonitorType |= CBIOS_MONITOR_TYPE_PANEL;
        }
        else
        {
            MonitorType |= CBIOS_MONITOR_TYPE_DP | CBIOS_MONITOR_TYPE_DVI | CBIOS_MONITOR_TYPE_HDMI;
        }
    }

    if(devices & CBIOS_TYPE_DP3)
    {
        if (pcbe->FeatureSwitch.IsEDP3Enabled)
        {
            MonitorType |= CBIOS_MONITOR_TYPE_PANEL;
        }
        else
        {
            MonitorType |=  CBIOS_MONITOR_TYPE_DP | CBIOS_MONITOR_TYPE_DVI | CBIOS_MONITOR_TYPE_HDMI;
        }
    }

    if(devices & CBIOS_TYPE_DP4)
    {
        if (pcbe->FeatureSwitch.IsEDP4Enabled)
        {
            MonitorType |= CBIOS_MONITOR_TYPE_PANEL;
        }
        else
        {
            MonitorType |= CBIOS_MONITOR_TYPE_DP | CBIOS_MONITOR_TYPE_DVI | CBIOS_MONITOR_TYPE_HDMI;
        }
    }

    if(devices & CBIOS_TYPE_DVO)
    {
        cbDVOPort_GetSupportMonitorType(pcbe, &MonitorType);
    }

    /*
    if(devices & CBIOS_TYPE_MHL)
    {
        if (pcbe->FeatureSwitch.IsCfgMHLMode)
        {
            MonitorType |= CBIOS_MONITOR_TYPE_MHL;
        }
        else
        {
            MonitorType |= CBIOS_MONITOR_TYPE_HDMI | CBIOS_MONITOR_TYPE_DVI;
        }
    }
    */

    if(devices & CBIOS_TYPE_DSI)
    {
        MonitorType |= CBIOS_MONITOR_TYPE_PANEL;
    }

    if(MonitorType == CBIOS_MONITOR_TYPE_NONE)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, WARNING), "%s: invalid device type 0x%x!\n", FUNCTION_NAME, devices));
    }

    return MonitorType;
}

CBIOS_VOID cbClearEdidRelatedData(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    if (pDevCommon == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: the 2nd param is NULL!\n", FUNCTION_NAME));
        return;
    }

    //reset buffers
    if(cbEDIDModule_IsEDIDValid(pDevCommon->EdidData))
    {
        cb_memset(pDevCommon->EdidData, 0, CBIOS_EDIDDATABYTE);
    }

    cb_memset(&pDevCommon->ConnectedDevSignature, 0, sizeof(CBIOS_DEVICE_SIGNATURE));
    //cb_memset(&pDevCommon->EdidStruct, 0, sizeof(CBIOS_EDID_STRUCTURE_DATA));

    pDevCommon->isFakeEdid = CBIOS_FALSE;
}



CBIOS_BOOL cbDualModeDetect(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_U8      byI2CBus = (CBIOS_U8)pDevCommon->I2CBus;
    CBIOS_BOOL    bIsDualMode = CBIOS_FALSE;
    CBIOS_U8      byData = 0;

    //using DP dual mode char byte to see HDMI or DP
    if (cbHWReadEDID(pcbe, byI2CBus, &byData, 0x00, 1, 0))
    {
        bIsDualMode = CBIOS_TRUE;
    }
    else
    {
        bIsDualMode = CBIOS_FALSE;
    }
    return bIsDualMode;
}

PCBIOS_VOID cbGetDPMonitorContext(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_VOID pDPMonitorContext = CBIOS_NULL;

    if (pDevCommon->DeviceType & ALL_DP_TYPES)
    {
        pDPMonitorContext = cbDPPort_GetDPMonitorContext(pvcbe, pDevCommon);
    }

    return pDPMonitorContext;
}

PCBIOS_VOID cbGetHDMIMonitorContext(PCBIOS_VOID pvcbe, PCBIOS_DEVICE_COMMON pDevCommon)
{
    PCBIOS_VOID pHDMIMonitorContext = CBIOS_NULL;

    if (pDevCommon->DeviceType & ALL_DP_TYPES)
    {
        pHDMIMonitorContext = cbDPPort_GetHDMIMonitorContext(pvcbe, pDevCommon);
    }

    return pHDMIMonitorContext;
}


#define GET_BIT(val, bit) ((val & (1 << bit)) >> bit)
/* ELD: EDID-Like Data, for passing sink device's audio EDID info to audio software */
CBIOS_STATUS cbGetDeviceELD(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE DeviceType, PCBIOS_ELD_MEM_STRUCT pELD)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, DeviceType);
    PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttrib = &(pDevCommon->EdidStruct.Attribute);
    CBIOS_U8    *pEldPayLoad    = CBIOS_NULL;
    CBIOS_U32   Length  =   0;
    CBIOS_U32   i  =   0;

    cb_memset(pELD, 0, sizeof(CBIOS_ELD_MEM_STRUCT));

    pELD->ELD_Data.HDCP = 1;
    pELD->ELD_Data.S_AI = 0;
    pELD->ELD_Data.ELD_Ver = ELD_VER_CEA_861D;

    cb_memcpy(pELD->ELD_Data.ManufactureName, pMonitorAttrib->ManufactureName, 0x02); /* Manufacture name */
    cb_memcpy(pELD->ELD_Data.ProductCode, pMonitorAttrib->ProductCode, 0x02); /* ProductCode */

    pEldPayLoad = pELD->ELD_Data.private_data;
    while (i < 16 && (pMonitorAttrib->MonitorName[i] != 0x00))
    {
        *(pEldPayLoad++) = pMonitorAttrib->MonitorName[i];  /* Monitor Name */
        Length++;
        i++;
    }
    pELD->ELD_Data.MNL = (CBIOS_U8)Length;  /* Monitor Name Length */

    if (pDevCommon->CurrentMonitorType & (CBIOS_MONITOR_TYPE_DP | CBIOS_MONITOR_TYPE_PANEL))
    {
        pELD->ELD_Data.Conn_Type = 0x01; // connect type is DP
    }
    else
    {
        pELD->ELD_Data.Conn_Type = 0; // HDMI
    }

    if (pMonitorAttrib->IsCEA861Monitor)
    {
        pELD->ELD_Data.CEA_EDID_Ver = pMonitorAttrib->RevisionNumber;

        if (!pMonitorAttrib->IsCEA861Audio)
        {
            cbDebugPrint((MAKE_LEVEL(HDMI, INFO), "%s: Not support basic audio!\n", FUNCTION_NAME));
        }

        pELD->ELD_Data.SAD_Count = pMonitorAttrib->SAD_Count;
        if  (pELD->ELD_Data.SAD_Count != 0x00)
        {
            for (i = 0; i < pELD->ELD_Data.SAD_Count; i++)
            {
                *(pEldPayLoad++) = pMonitorAttrib->CEA_SADs[i][0];
                *(pEldPayLoad++) = pMonitorAttrib->CEA_SADs[i][1];
                *(pEldPayLoad++) = pMonitorAttrib->CEA_SADs[i][2];
                Length += 3;
            }
        }
        else
        {
            /*
             * From High-Definitiddon Multimedia Interface Specification Version 1.4b  7.3(Page 125):
             *
             * For CEA-861-D references to Sources:
             *  "Basic Audio" is defined as two channel L-PCM audio at sample rates of 32kHZ,44.1Khz, or 48kHZ,
             *   With a sample size of at least 16 bits
             *
             *   CEA short Audio Descriptor for audio code = 1(LPCM)
             * --------------------------------------------------------------------------------
             *  Byte offset  |                         BIts                                   |
             *  -------------------------------------------------------------------------------
             *               |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0    |
             *  -------------------------------------------------------------------------------
             *       1       | F17=0 |   Audio Format Code = 0001    | Max_num_channels - 1   |
             *  -------------------------------------------------------------------------------
             *       2       | F27=0 |192kHz |176.4kHz|96kHz |88.2kHz| 48kHz | 44.1kHz| 32kHz |
             *  -------------------------------------------------------------------------------
             *       3       | F37=0 | F36=0 | F35=0  | F34=0| F33=0 | 24bit | 20bit  | 16bit |
             *  -----------------------------------------------------------------------------
             */
            pELD->ELD_Data.SAD_Count = 1;
            *(pEldPayLoad++) = 0x9;     //Linear PCM
            *(pEldPayLoad++) = 0x7;     //48kHZ, 44.1Khz, 32kHZ
            *(pEldPayLoad++) = 0x3;     //20 bit, 16 bit
            Length += 3;
        }

        pELD->ELD_Data.BaseLine_Eld_len = (16 + Length + 3) / 4;
        pELD->Size = pELD->ELD_Data.BaseLine_Eld_len * 4 + 4;

        pELD->ELD_Data.FLR  =   GET_BIT(pMonitorAttrib->SpeakerAllocationData, 0);
        pELD->ELD_Data.LFE  =   GET_BIT(pMonitorAttrib->SpeakerAllocationData, 1);
        pELD->ELD_Data.FC   =   GET_BIT(pMonitorAttrib->SpeakerAllocationData, 2);
        pELD->ELD_Data.RLR  =   GET_BIT(pMonitorAttrib->SpeakerAllocationData, 3);
        pELD->ELD_Data.RC   =   GET_BIT(pMonitorAttrib->SpeakerAllocationData, 4);
        pELD->ELD_Data.FLRC =   GET_BIT(pMonitorAttrib->SpeakerAllocationData, 5);
        pELD->ELD_Data.RLRC =   GET_BIT(pMonitorAttrib->SpeakerAllocationData, 6);
    }

    return CBIOS_OK;
}

