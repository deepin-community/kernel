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
** HDCP hw block interface function implementation.
**
** NOTE:
**
******************************************************************************/

#include "CBiosDIU_HDCP.h"
#include "CBiosChipShare.h"
#include "../CBiosHwShare.h"

// for CTS in Quantum Data 980
CBIOS_U8 Fake_RevokedBksv[3][5] = 
{
    {0x23, 0xde, 0x5c, 0x43, 0x93},
    {0x10, 0x43, 0x44, 0x4f, 0x51},
    {0x0b, 0x37, 0x21, 0xb4, 0x7d},
};


// for internal test
#define HDCP_USE_TEST_KEY       0

CBIOS_U32 HDCP_REG_KSV1[HDCP_MODU_NUM] = {0x82B0,  0x33C5C,  0x3435C,  0x34A5C};
CBIOS_U32 HDCP_REG_CTRL_KSV[HDCP_MODU_NUM] = {0x82B4,  0x33C60,  0x34360,  0x34A60};
CBIOS_U32 HDCP_REG_CTRL2[HDCP_MODU_NUM] = {0x82B8,  0x33C64,  0x34364,  0x34A64};
CBIOS_U32 HDCP_REG_MISC1[HDCP_MODU_NUM] = {0x82C4,  0x33C6C,  0x3436C,  0x34A6C};
CBIOS_U32 HDCP_REG_MISC2[HDCP_MODU_NUM] = {0x82C8,  0x33C70,  0x34370,  0x34A70};
CBIOS_U32 HDCP_REG_2XHDMI_CTRL[HDCP_MODU_NUM] = {0x33680, 0x33C80,  0x34380,  0x34A80};
CBIOS_U32 HDCP_REG_2XCTRL[HDCP_MODU_NUM] = {0x33688, 0x33C84,  0x34384,  0x34A84};
CBIOS_U32 HDCP_REG_2XINT[HDCP_MODU_NUM] = {0x3368C, 0x33C88,  0x34388,  0x34A88};
CBIOS_U32 HDCP_REG_DP_KSV1[HDCP_MODU_NUM] = {0x8370,  0x33C74,  0x34374,  0x34A74};
CBIOS_U32 HDCP_REG_DP_CTRL_KSV[HDCP_MODU_NUM] = {0x8374,  0x33C78,  0x34378,  0x34A78};
CBIOS_U32 HDCP_REG_DP_CTRL2[HDCP_MODU_NUM] = {0x8378,  0x33C7C,  0x3437C,  0x34A7C};
CBIOS_U32 HDCP_REG_2XDP_CTRL[HDCP_MODU_NUM] = {0x33684, 0x33C90,  0x34390,  0x34A90};

#define GET_HDCP_CONTEXT(pvcbe, Device) cbHDCP_GetContext(pvcbe, Device)
static PCBIOS_HDCP_CONTEXT cbHDCP_GetContext(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);

    return pDevCommon->pHDCPContext;
}

static CBIOS_BOOL cbHDCP_CheckRevokedBksv(PCBIOS_EXTENSION_COMMON pcbe, PCBIOS_U8 pBksvList, CBIOS_U32 BufferSize1,
    PCBIOS_U8 pRevocationList, CBIOS_U32 BufferSize2)
{
    CBIOS_BOOL bRevoked = CBIOS_FALSE;
    CBIOS_U32 i, j;

    if ((pRevocationList == CBIOS_NULL) || (pBksvList == CBIOS_NULL))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pRevocationList or pBksvList is NULL!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    if ((BufferSize1 == 0) || (BufferSize1 % 5 != 0) || (BufferSize2 == 0) || (BufferSize2 % 5 != 0))
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid buffer size!\n", FUNCTION_NAME));
        return CBIOS_TRUE;
    }

    for (j = 0; j < BufferSize1/5; j++)
    {
        for (i = 0; i < BufferSize2/5; i++)
        {
            if ((pBksvList[j * 5]     == pRevocationList[i * 5]) &&
                (pBksvList[j * 5 + 1] == pRevocationList[i * 5 + 1]) &&
                (pBksvList[j * 5 + 2] == pRevocationList[i * 5 + 2]) &&
                (pBksvList[j * 5 + 3] == pRevocationList[i * 5 + 3]) &&
                (pBksvList[j * 5 + 4] == pRevocationList[i * 5 + 4]))
            {
                bRevoked = CBIOS_TRUE;
                cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Bksv: 0x%02x%02x%02x%02x%02x\n", FUNCTION_NAME,
                    pBksvList[j * 5], pBksvList[j * 5 + 1], pBksvList[j * 5 + 2], pBksvList[j * 5 + 3], pBksvList[j * 5 + 4]));
                break;
            }
        }

        if (i != BufferSize2/5)
        {
            break;
        }
    }

    return bRevoked;
}

static CBIOS_VOID cbDIU_HDMI_HDCP_EncryptionEnableDisable(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, CBIOS_BOOL bEnable)
{
    REG_MM82C4 HDCPMisc1RegValue, HDCPMisc1RegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    HDCPMisc1RegValue.Value = 0;
    HDCPMisc1RegValue.CTL = bEnable ? 9 : 1;
    HDCPMisc1RegMask.Value = 0xFFFFFFFF;
    HDCPMisc1RegMask.CTL = 0;
    cbMMIOWriteReg32(pcbe, HDCP_REG_MISC1[HDCPModuleIndex], HDCPMisc1RegValue.Value, HDCPMisc1RegMask.Value);
}

static CBIOS_VOID cbDIU_HDMI_HDCP_ReadBksv(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, PCBIOS_U8 pBksv)
{
    REG_MM82B0 HDCPKsv1RegValue;
    REG_MM82B4 HDCPCtrlKsvRegValue;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    if (pBksv == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pBksv is NULL!\n", FUNCTION_NAME));
        return;
    }

    HDCPKsv1RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_KSV1[HDCPModuleIndex]);
    HDCPCtrlKsvRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_CTRL_KSV[HDCPModuleIndex]);

    pBksv[1] = (CBIOS_U8)HDCPKsv1RegValue.KSV_39to8;
    pBksv[2] = (CBIOS_U8)(HDCPKsv1RegValue.KSV_39to8 >> 8);
    pBksv[3] = (CBIOS_U8)(HDCPKsv1RegValue.KSV_39to8 >> 16);
    pBksv[4] = (CBIOS_U8)(HDCPKsv1RegValue.KSV_39to8 >> 24);
    pBksv[0] = (CBIOS_U8)HDCPCtrlKsvRegValue.KSV_7to0;

    //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Bksv: 0x%08x%02x\n", FUNCTION_NAME, HDCPKsv1RegValue.KSV_39to8, HDCPCtrlKsvRegValue.KSV_7to0));
}

static CBIOS_BOOL cbDIU_HDMI_HDCP_IsBKSVReady(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM82B8 HDCPCtrl2RegValue;
    CBIOS_U32 wait_us = 10, timeout = 250000, count = 0;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }
    
    while(1)
    {
        HDCPCtrl2RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_CTRL2[HDCPModuleIndex]);
        if (HDCPCtrl2RegValue.KSV_Verification_Done)
        {
            //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "wait times %d\n", count));
            break;
        }

        cb_DelayMicroSeconds(wait_us);

        count++;
        if (count > timeout)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: wait time out\n", FUNCTION_NAME));
            break;
        }
    }

    return HDCPCtrl2RegValue.KSV_Verification_Done ? CBIOS_TRUE : CBIOS_FALSE;
}

static CBIOS_VOID cbDIU_HDMI_HDCP_RevocationCheckDoneAck(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM82B8 HDCPCtrl2RegValue, HDCPCtrl2RegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    HDCPCtrl2RegValue.Value = 0;
    HDCPCtrl2RegValue.KSV_Verification_Done = 0;
    HDCPCtrl2RegMask.Value = 0xFFFFFFFF;
    HDCPCtrl2RegMask.KSV_Verification_Done = 0;
    cbMMIOWriteReg32(pcbe, HDCP_REG_CTRL2[HDCPModuleIndex], HDCPCtrl2RegValue.Value, HDCPCtrl2RegMask.Value);
}

static CBIOS_VOID cbDIU_HDMI_HDCP1x_Isr(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    PCBIOS_HDCP_CONTEXT pHdcpContext = GET_HDCP_CONTEXT(pcbe, Device);
    REG_MM82C4 HDCPMisc1RegValue;
    CBIOS_U32  HDCPIntSource;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    HDCPMisc1RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_MISC1[HDCPModuleIndex]);
    HDCPIntSource = HDCPMisc1RegValue.Interrupt_Source;

    //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Interrupt_Source = %d\n", FUNCTION_NAME, HDCPIntSource));
    
    switch (HDCPIntSource)
    {
    case CBIOS_HDMI_HDCP1x_INT_KSV_READY:
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: BKSV ready\n", FUNCTION_NAME));
        if (cbDIU_HDMI_HDCP_IsBKSVReady(pcbe, HDCPModuleIndex))
        {
            cbDIU_HDMI_HDCP_ReadBksv(pcbe, HDCPModuleIndex, pHdcpContext->BKsv);
        }
        if(cbHDCP_CheckRevokedBksv(pcbe, pHdcpContext->BKsv, 5, 
            pHdcpContext->pRevocationList, pHdcpContext->RevocationListBufferSize))
        {
            //cbHDCP_OnOff(pcbe, Device, IGAIndex, CBIOS_FALSE);
            //pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        }
        else
        {
             cbDIU_HDMI_HDCP_RevocationCheckDoneAck(pcbe, HDCPModuleIndex);
        }
        break;
    case CBIOS_HDMI_HDCP1x_INT_AUTH_PASS:
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: authentication pass\n", FUNCTION_NAME));
        cbDIU_HDMI_HDCP_EncryptionEnableDisable(pcbe, HDCPModuleIndex, CBIOS_TRUE);
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_PASS;
        break;
    case CBIOS_HDMI_HDCP1x_INT_MAX_CASCADE:
    case CBIOS_HDMI_HDCP1x_INT_MAX_DEVICES:
    case CBIOS_HDMI_HDCP1x_INT_AUTH_FAIL:
    case CBIOS_HDMI_HDCP1x_INT_Ri_VERIF_FAIL:
    case CBIOS_HDMI_HDCP1x_INT_ZERO_DEVICE:
    case CBIOS_HDMI_HDCP1x_INT_BKSV_INVALID:
    case CBIOS_HDMI_HDCP1x_INT_V_LIST_CHECK_FAIL:
    case CBIOS_HDMI_HDCP1x_INT_Ri_VERIF_TIMEOUT:
    case CBIOS_HDMI_HDCP1x_INT_Pj_VERIF_FAIL:
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: %d verification fail\n", FUNCTION_NAME, HDCPIntSource));
        cbDIU_HDMI_HDCP_EncryptionEnableDisable(pcbe, HDCPModuleIndex, CBIOS_FALSE);
        cbHDCP_OnOff(pcbe, Device, HDCPModuleIndex, CBIOS_FALSE);
        cbDelayMilliSeconds(1500);
        cbHDCP_OnOff(pcbe, Device, HDCPModuleIndex, CBIOS_TRUE);
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        break;
    }
}

static CBIOS_VOID cbDIU_HDMI_HDCP1x_OnOff(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, CBIOS_BOOL bTurnOn)
{
    REG_MM82B4 HDCPCtrlKsvRegValue, HDCPCtrlKsvRegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    HDCPCtrlKsvRegValue.Value = 0x0;
    HDCPCtrlKsvRegValue.CP_EN = bTurnOn ? 1 : 0;
    HDCPCtrlKsvRegMask.Value = 0xFFFFFFFF;
    HDCPCtrlKsvRegMask.CP_EN = 0;
    cbMMIOWriteReg32(pcbe, HDCP_REG_CTRL_KSV[HDCPModuleIndex], HDCPCtrlKsvRegValue.Value, HDCPCtrlKsvRegMask.Value);
}

static CBIOS_BOOL cbDIU_HDMI_HDCP1x_IsEnabled(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM82B4 HDCPCtrlKsvRegValue;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }

    HDCPCtrlKsvRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_CTRL_KSV[HDCPModuleIndex]);

    return HDCPCtrlKsvRegValue.CP_EN ? CBIOS_TRUE : CBIOS_FALSE;
}

static CBIOS_VOID cbDIU_HDMI_HDCP2x_OnOff(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, CBIOS_BOOL bTurnOn)
{
    REG_MM33680 HDCP2xHDMICtrlRegValue, HDCP2xHDMICtrlRegMask;
    REG_MM33688 HDCP2xCtrlRegValue, HDCP2xCtrlRegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }
    
    if (bTurnOn)
    {
        HDCP2xHDMICtrlRegValue.Value = 0;
        HDCP2xHDMICtrlRegValue.STREAM_ID = 0;
        HDCP2xHDMICtrlRegValue.STREAM_TYPE = 0;
        HDCP2xHDMICtrlRegValue.RNG_OSC_MD = 0;
        HDCP2xHDMICtrlRegValue.RNG_OSC_EN_HDCP = 1;
        HDCP2xHDMICtrlRegValue.RNG_EN_HDCP = 1;
        
        HDCP2xHDMICtrlRegValue.HDCP22_AUTH_SEL = 0; // disable HDCP 2.2 re-authentication reattemp
        HDCP2xHDMICtrlRegValue.HDCP22_AUTH_TRIG = 1;
        
        HDCP2xHDMICtrlRegValue.CSM_TRIGGER = 1;
        HDCP2xHDMICtrlRegValue.AKE_Stored_km_DIS = 0; // enable AKE_Stored_Km
        HDCP2xHDMICtrlRegValue.HDCP22_VER_RD_DIS = 0; // read HDCP 2.2 version before enable HDCP 2.2
        HDCP2xHDMICtrlRegValue.TEST_MODE = 0;
        HDCP2xHDMICtrlRegValue.TEST_REPEATER = 0;
        HDCP2xHDMICtrlRegValue.CONT_STREAM_EN = 1;
        HDCP2xHDMICtrlRegValue.HDCP22_CP_EN = 1;
        HDCP2xHDMICtrlRegMask.Value = 0;
        cbMMIOWriteReg32(pcbe, HDCP_REG_2XHDMI_CTRL[HDCPModuleIndex], HDCP2xHDMICtrlRegValue.Value, HDCP2xHDMICtrlRegMask.Value);

        // HDCP 2.2 TX must set VERSION to 0x02
        HDCP2xCtrlRegValue.Value = 0;
        HDCP2xCtrlRegValue.TxCaps = 0x020000;
        HDCP2xCtrlRegMask.Value = 0xFFFFFFFF;
        HDCP2xCtrlRegMask.TxCaps = 0;
        cbMMIOWriteReg32(pcbe, HDCP_REG_2XCTRL[HDCPModuleIndex], HDCP2xCtrlRegValue.Value, HDCP2xCtrlRegMask.Value);
    }
    else
    {
        HDCP2xHDMICtrlRegValue.Value = 0;
        HDCP2xHDMICtrlRegMask.Value = 0;
        cbMMIOWriteReg32(pcbe, HDCP_REG_2XHDMI_CTRL[HDCPModuleIndex], HDCP2xHDMICtrlRegValue.Value, HDCP2xHDMICtrlRegMask.Value);
    }
}

static CBIOS_BOOL cbDIU_HDMI_HDCP2x_IsEnabled(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM33680 HDCP2xHDMICtrlRegValue;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    HDCP2xHDMICtrlRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_2XHDMI_CTRL[HDCPModuleIndex]);

    return HDCP2xHDMICtrlRegValue.HDCP22_CP_EN ? CBIOS_TRUE : CBIOS_FALSE;
}

static CBIOS_VOID cbDIU_HDMI_HDCP_ChooseProtocol(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, 
    CBIOS_U8 IGAIndex, CBIOS_HDCP_VERSION HdcpVersion, CBIOS_BOOL bHDMI)
{
    REG_MM82B4 HDCPCtrlKsvRegValue, HDCPCtrlKsvRegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }
    
    // HDMI protocol
    if (bHDMI)
    {
        HDCPCtrlKsvRegValue.Value = 0;
        HDCPCtrlKsvRegValue.Source_Select = 2; // HDMI
        HDCPCtrlKsvRegValue.Mode_Sel = 1;
        HDCPCtrlKsvRegValue.AC_EN = 0;
        HDCPCtrlKsvRegValue.Verify_Pj_Enable = 0;
        HDCPCtrlKsvRegValue.EESS_Signaling_Select = 1;
        HDCPCtrlKsvRegMask.Value = 0xFFFFFFFF;
        HDCPCtrlKsvRegMask.Source_Select = 0;
        HDCPCtrlKsvRegMask.Mode_Sel = 0;
        HDCPCtrlKsvRegMask.AC_EN = 0;
        HDCPCtrlKsvRegMask.Verify_Pj_Enable = 0;
        HDCPCtrlKsvRegMask.EESS_Signaling_Select = 0;
        cbMMIOWriteReg32(pcbe, HDCP_REG_CTRL_KSV[HDCPModuleIndex], HDCPCtrlKsvRegValue.Value, HDCPCtrlKsvRegMask.Value);
    }
    // DVI protocol
    else
    {
        HDCPCtrlKsvRegValue.Value = 0;
        HDCPCtrlKsvRegValue.Source_Select = 0; // SP1

        HDCPCtrlKsvRegValue.Mode_Sel = 0;
        HDCPCtrlKsvRegValue.AC_EN = 0;
        HDCPCtrlKsvRegValue.Verify_Pj_Enable = 0;
        if (HdcpVersion == CBIOS_HDCP_VERSION_1x)
        {
            HDCPCtrlKsvRegValue.EESS_Signaling_Select = 0; // OESS
        }
        else if (HdcpVersion == CBIOS_HDCP_VERSION_2x)
        {
            // NOTE: for HDCP 2.2, only support EESS for DVI
            HDCPCtrlKsvRegValue.EESS_Signaling_Select = 1; // EESS
        }
        HDCPCtrlKsvRegMask.Value = 0xFFFFFFFF;
        HDCPCtrlKsvRegMask.Source_Select = 0;
        HDCPCtrlKsvRegMask.Mode_Sel = 0;
        HDCPCtrlKsvRegMask.AC_EN = 0;
        HDCPCtrlKsvRegMask.Verify_Pj_Enable = 0;
        HDCPCtrlKsvRegMask.EESS_Signaling_Select = 0;
        cbMMIOWriteReg32(pcbe, HDCP_REG_CTRL_KSV[HDCPModuleIndex], HDCPCtrlKsvRegValue.Value, HDCPCtrlKsvRegMask.Value);   
    }
}

static CBIOS_VOID cbDIU_HDMI_HDCP_RevocationCheckEnableDisable(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, CBIOS_BOOL bEnable)
{
    REG_MM82B8 HDCPCtrl2RegValue, HDCPCtrl2RegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }
    
    HDCPCtrl2RegValue.Value = 0;
    HDCPCtrl2RegValue.KSV_Revocation_List_Available = bEnable ? 1 : 0;
    HDCPCtrl2RegMask.Value = 0xFFFFFFFF;
    HDCPCtrl2RegMask.KSV_Revocation_List_Available = 0;
    cbMMIOWriteReg32(pcbe, HDCP_REG_CTRL2[HDCPModuleIndex], HDCPCtrl2RegValue.Value, HDCPCtrl2RegMask.Value);
}

static CBIOS_U8 cbDIU_HDMI_HDCP_GetDeviceCount(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM82C4 HDCPMisc1RegValue;
    CBIOS_U8 DeviceCount = 0;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return 0;
    }

    // NOTE: 
    // HDCP 1.4 BKSV list HW fifo is  4 * BKSV. So supports repeater with maximum  4 receivers connected.
    // HDCP 2.2 BKSV list HW fifo is 31 * BKSV. So supports repeater with maximum 31 receivers connected.
    HDCPMisc1RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_MISC1[HDCPModuleIndex]);
    DeviceCount = (CBIOS_U8)HDCPMisc1RegValue.Device_Count;
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Device Count = %d\n", FUNCTION_NAME, DeviceCount));

    return DeviceCount;
}

static CBIOS_VOID cbDIU_HDMI_HDCP_ReadBksvList(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, 
    PCBIOS_U8 pBksvList, CBIOS_U8 DeviceCount)
{
    CBIOS_U32 i;

    if (HDCPModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    if (DeviceCount == 0)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: no receiver connected\n", FUNCTION_NAME));
        return;
    }

    if (pBksvList == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pBksvList is NULL!\n", FUNCTION_NAME));
        return;
    }

    for (i = 0; i < DeviceCount; i++)
    {
        if (cbDIU_HDMI_HDCP_IsBKSVReady(pcbe, HDCPModuleIndex))
        {
            cbDIU_HDMI_HDCP_ReadBksv(pcbe, HDCPModuleIndex, pBksvList + i * 5);
            cbDIU_HDMI_HDCP_RevocationCheckDoneAck(pcbe, HDCPModuleIndex);
        }
        else
        {
            return;
        }
    }
}

static CBIOS_BOOL cbDIU_HDMI_HDCP_CheckIfRepeater(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM82C4 HDCPMisc1RegValue;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    HDCPMisc1RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_MISC1[HDCPModuleIndex]);
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: repeater = %d\n", FUNCTION_NAME, HDCPMisc1RegValue.Repeater_Flag));

    return HDCPMisc1RegValue.Repeater_Flag ? CBIOS_TRUE : CBIOS_FALSE;
}

static CBIOS_VOID cbDIU_HDMI_HDCP_HwI2cOnOff(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, CBIOS_U8 I2CBus, CBIOS_BOOL bTurnOn)
{
    CBIOS_MODULE_I2C_PARAMS I2CParams = {0};
    REG_MM82B8 HDCPCtrl2RegValue, HDCPCtrl2RegMask;
    REG_MM82C4 HDCPMisc1RegValue, HDCPMisc1RegMask;
    REG_CRAA   RegCRAAValue, RegCRAAMask;
    REG_CRC6_B RegCRC6_BValue, RegCRC6_BMask;
    REG_CRF8_B RegCRF8_BValue, RegCRF8_BMask;
    REG_CRC5_B RegCRC5_BValue, RegCRC5_BMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    I2CParams.ConfigType = CONFIG_I2C_BY_BUSNUM;
    I2CParams.I2CBusNum = I2CBus;

    RegCRAAValue.Value = 0;
    RegCRAAMask.Value = 0xFF;
    RegCRC6_BValue.Value = 0;
    RegCRC6_BMask.Value = 0xFF;

    // config I2C module for HDMI
    if (bTurnOn)
    {
        // Enable I2C bus
        cbI2CModule_HDCPI2CEnableDisable(pcbe, &I2CParams, CBIOS_TRUE);

        // Write I2C bus number used for HDCP
        if (HDCPModuleIndex == CBIOS_MODULE_INDEX1)
        {
            RegCRAAValue.HDCP1_I2C_port_selection = (I2CParams.I2CBusNum & 0x03);
            RegCRAAMask.HDCP1_I2C_port_selection = 0;
            cbMMIOWriteReg(pcbe, CR_AA, RegCRAAValue.Value, RegCRAAMask.Value);
        }
        else if (HDCPModuleIndex == CBIOS_MODULE_INDEX2)
        {
            RegCRC6_BValue.HDCP2_I2C_port_selection = (I2CParams.I2CBusNum & 0x03);
            RegCRC6_BMask.HDCP2_I2C_port_selection = 0;
            cbMMIOWriteReg(pcbe, CR_B_C6, RegCRC6_BValue.Value, RegCRC6_BMask.Value);
        }
        else if (HDCPModuleIndex == CBIOS_MODULE_INDEX3)
        {
            RegCRF8_BValue.HDCP3_I2C_port_selection = (I2CParams.I2CBusNum & 0x03);
            RegCRF8_BMask.HDCP3_I2C_port_selection = 0;
            cbMMIOWriteReg(pcbe, CR_B_F8, RegCRF8_BValue.Value, RegCRF8_BMask.Value);
        }
        else if (HDCPModuleIndex == CBIOS_MODULE_INDEX4)
        {
            RegCRC5_BValue.HDCP4_I2C_port_selection = (I2CParams.I2CBusNum & 0x03);
            RegCRC5_BMask.HDCP4_I2C_port_selection = 0;
            cbMMIOWriteReg(pcbe, CR_B_C5, RegCRC5_BValue.Value, RegCRC5_BMask.Value);
        }

        HDCPMisc1RegValue.Value = 0;
        HDCPMisc1RegValue.I2C_Frequency_Sleect = CBIOS_HW_I2C_Freq_200kHz;
        HDCPMisc1RegValue.AUTH_FAIL_INT_SEL = 1;
        HDCPMisc1RegMask.Value = 0xFFFFFFFF;
        HDCPMisc1RegMask.I2C_Frequency_Sleect = 0;
        HDCPMisc1RegMask.AUTH_FAIL_INT_SEL = 0;
        cbMMIOWriteReg32(pcbe, HDCP_REG_MISC1[HDCPModuleIndex], HDCPMisc1RegValue.Value, HDCPMisc1RegMask.Value);

        // enable HDCP I2C function.
        HDCPCtrl2RegValue.Value = 0x0;
        HDCPCtrl2RegValue.HDCP_I2C_Function_Enable = 1;
        HDCPCtrl2RegMask.Value = 0xFFFFFFFF;
        HDCPCtrl2RegMask.HDCP_I2C_Function_Enable = 0;
        cbMMIOWriteReg32(pcbe, HDCP_REG_CTRL2[HDCPModuleIndex], HDCPCtrl2RegValue.Value, HDCPCtrl2RegMask.Value);
    }
    else
    {
        // disable HDCP I2C function.
        HDCPCtrl2RegValue.Value = 0x0;
        HDCPCtrl2RegValue.HDCP_I2C_Function_Enable = 0;
        HDCPCtrl2RegMask.Value = 0xFFFFFFFF;
        HDCPCtrl2RegMask.HDCP_I2C_Function_Enable = 0;
        cbMMIOWriteReg32(pcbe, HDCP_REG_CTRL2[HDCPModuleIndex], HDCPCtrl2RegValue.Value, HDCPCtrl2RegMask.Value);

        cbI2CModule_HDCPI2CEnableDisable(pcbe, &I2CParams, CBIOS_FALSE);
    }
}

#if HDCP_USE_TEST_KEY
// for internal test
static CBIOS_VOID cbDIU_HDMI_HDCP_TestKeyEnableDisable(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, CBIOS_BOOL bEnable)
{
    REG_MM82B4 HDCPCtrlKsvRegValue, HDCPCtrlKsvRegMask;
    REG_MM82C8 HDCPMisc2RegValue, HDCPMisc2RegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    HDCPMisc2RegValue.Value = 0;
    HDCPMisc2RegValue.reserved = bEnable ? 1 : 0; // bit27 = 1
    HDCPMisc2RegMask.Value = 0xFFFFFFFF;
    HDCPMisc2RegMask.reserved = 0;
    cbMMIOWriteReg32(pcbe, HDCP_REG_MISC2[CBIOS_MODULE_INDEX1], HDCPMisc2RegValue.Value, HDCPMisc2RegMask.Value);

    HDCPCtrlKsvRegValue.Value = 0;
    HDCPCtrlKsvRegValue.Test_Key_Enable = bEnable ? 1 : 0; // bit26 = 1
    HDCPCtrlKsvRegMask.Value = 0xFFFFFFFF;
    HDCPCtrlKsvRegMask.Test_Key_Enable = 0;
    cbMMIOWriteReg32(pcbe, HDCP_REG_CTRL_KSV[CBIOS_MODULE_INDEX1], HDCPCtrlKsvRegValue.Value, HDCPCtrlKsvRegMask.Value);
}

#endif

static CBIOS_VOID cbDIU_DP_HDCP_RevocationCheckEnableDisable(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, CBIOS_BOOL bEnable)
{
    REG_MM8374 HDCPDPCtrlKsvRegValue, HDCPDPCtrlKsvRegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    HDCPDPCtrlKsvRegValue.Value = 0x0;
    HDCPDPCtrlKsvRegValue.KSV_Rev_List = bEnable ? 1 : 0;
    HDCPDPCtrlKsvRegMask.Value = 0xFFFFFFFF;
    HDCPDPCtrlKsvRegMask.KSV_Rev_List = 0;
    cbMMIOWriteReg32(pcbe, HDCP_REG_DP_CTRL_KSV[HDCPModuleIndex], HDCPDPCtrlKsvRegValue.Value, HDCPDPCtrlKsvRegMask.Value);
}

static CBIOS_VOID cbDIU_DP_HDCP_RevocationCheckDoneAck(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM8374 HDCPDPCtrlKsvRegValue, HDCPDPCtrlKsvRegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    HDCPDPCtrlKsvRegValue.Value = 0x0;
    HDCPDPCtrlKsvRegValue.KSV_Verifcation_Done = 0;
    HDCPDPCtrlKsvRegMask.Value = 0xFFFFFFFF;
    HDCPDPCtrlKsvRegMask.KSV_Verifcation_Done = 0;
    cbMMIOWriteReg32(pcbe, HDCP_REG_DP_CTRL_KSV[HDCPModuleIndex], HDCPDPCtrlKsvRegValue.Value, HDCPDPCtrlKsvRegMask.Value);
}

static CBIOS_BOOL cbDIU_DP_HDCP_IsBKSVReady(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM8374 HDCPDPCtrlKsvRegValue;
    CBIOS_U32 wait_us = 10, timeout = 10000, count = 0;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }
    
    while(1)
    {
        HDCPDPCtrlKsvRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_DP_CTRL_KSV[HDCPModuleIndex]);
        if (HDCPDPCtrlKsvRegValue.KSV_Verifcation_Done)
        {
            break;
        }

        cb_DelayMicroSeconds(wait_us);

        count++;
        if (count > timeout)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: wait time out\n", FUNCTION_NAME));
            break;
        }
    }

    return HDCPDPCtrlKsvRegValue.KSV_Verifcation_Done ? CBIOS_TRUE : CBIOS_FALSE;
}

static CBIOS_VOID cbDIU_DP_HDCP_ReadBksv(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, PCBIOS_U8 pBksv)
{
    REG_MM8370 HDCPDPKsv1RegValue;
    REG_MM8374 HDCPDPCtrlKsvRegValue;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    if (pBksv == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pBksv is NULL!\n", FUNCTION_NAME));
        return;
    }

    HDCPDPKsv1RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_DP_KSV1[HDCPModuleIndex]);
    HDCPDPCtrlKsvRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_DP_CTRL_KSV[HDCPModuleIndex]);

    pBksv[1] = (CBIOS_U8)HDCPDPKsv1RegValue.KSV_39to8;
    pBksv[2] = (CBIOS_U8)(HDCPDPKsv1RegValue.KSV_39to8 >> 8);
    pBksv[3] = (CBIOS_U8)(HDCPDPKsv1RegValue.KSV_39to8 >> 16);
    pBksv[4] = (CBIOS_U8)(HDCPDPKsv1RegValue.KSV_39to8 >> 24);
    pBksv[0] = (CBIOS_U8)HDCPDPCtrlKsvRegValue.KSV_7to0;

    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Bksv: 0x%08x%02x\n", FUNCTION_NAME, HDCPDPKsv1RegValue.KSV_39to8, HDCPDPCtrlKsvRegValue.KSV_7to0));
}

static CBIOS_VOID cbDIU_DP_HDCP_EncryptionEnableDisable(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, CBIOS_BOOL bEnable)
{
    REG_MM8374 HDCPDPCtrlKsvRegValue, HDCPDPCtrlKsvRegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    HDCPDPCtrlKsvRegValue.Value = 0x0;
    HDCPDPCtrlKsvRegValue.Enc_Sel = 0; // when authentication fails, HW disables encryption
    HDCPDPCtrlKsvRegValue.Enc_Con = bEnable ? 1 : 0;
    HDCPDPCtrlKsvRegMask.Value = 0xFFFFFFFF;
    HDCPDPCtrlKsvRegMask.Enc_Sel = 0;
    HDCPDPCtrlKsvRegMask.Enc_Con = 0;
    cbMMIOWriteReg32(pcbe, HDCP_REG_DP_CTRL_KSV[HDCPModuleIndex], HDCPDPCtrlKsvRegValue.Value, HDCPDPCtrlKsvRegMask.Value);
}

static CBIOS_VOID cbDIU_DP_HDCP1x_Isr(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    PCBIOS_HDCP_CONTEXT pHdcpContext = GET_HDCP_CONTEXT(pcbe, Device);
    REG_MM8378 HDCPDPCtrl2RegValue;
    CBIOS_U32  HDCPDPIntSource;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    HDCPDPCtrl2RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_DP_CTRL2[HDCPModuleIndex]);
    HDCPDPIntSource = HDCPDPCtrl2RegValue.Interrupt_Source;

    switch (HDCPDPIntSource)
    {
    case CBIOS_DP_HDCP1x_INT_KSV_READY:
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: BKSV ready\n", FUNCTION_NAME));
        if (cbDIU_DP_HDCP_IsBKSVReady(pcbe, HDCPModuleIndex))
        {
            cbDIU_DP_HDCP_ReadBksv(pcbe, HDCPModuleIndex, pHdcpContext->BKsv);
        }
        if(cbHDCP_CheckRevokedBksv(pcbe, pHdcpContext->BKsv, 5, 
            pHdcpContext->pRevocationList, pHdcpContext->RevocationListBufferSize))
        {
            //cbHDCP_OnOff(pcbe, Device, IGAIndex, CBIOS_FALSE);
            //pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        }
        else
        {
             cbDIU_DP_HDCP_RevocationCheckDoneAck(pcbe, HDCPModuleIndex);
        }
        break;
    case CBIOS_DP_HDCP1x_INT_AUTH_PASS:
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: authentication pass\n", FUNCTION_NAME));
        cbDIU_DP_HDCP_EncryptionEnableDisable(pcbe, HDCPModuleIndex, CBIOS_TRUE);
         pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_PASS;
        break;
    case CBIOS_DP_HDCP1x_INT_BKSV_INVALID:
    case CBIOS_DP_HDCP1x_INT_V_LIST_CHECK_FAIL:
    case CBIOS_DP_HDCP1x_INT_AUTH_FAIL:
    case CBIOS_DP_HDCP1x_INT_AUX_FAIL:
    case CBIOS_DP_HDCP1x_INT_NOT_HDCP_CAPABLE:
    case CBIOS_DP_HDCP1x_INT_RO_INVALID:
    case CBIOS_DP_HDCP1x_INT_KSV_VERIF_TIMEOUT:
    case CBIOS_DP_HDCP1x_INT_MAX_CASCADE:
    case CBIOS_DP_HDCP1x_INT_MAX_DEVS:
    case CBIOS_DP_HDCP1x_INT_ZERO_DEVS:
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: %d verification fail\n", FUNCTION_NAME, HDCPDPIntSource));
        cbDIU_DP_HDCP_EncryptionEnableDisable(pcbe, HDCPModuleIndex, CBIOS_FALSE);
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        break;
    }
}

static CBIOS_VOID cbDIU_DP_HDCP1x_OnOff(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, CBIOS_BOOL bTurnOn)
{
    REG_MM8374 HDCPDPCtrlKsvRegValue, HDCPDPCtrlKsvRegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    HDCPDPCtrlKsvRegValue.Value = 0x0;
    HDCPDPCtrlKsvRegValue.CP_EN = bTurnOn ? 1 : 0;
    HDCPDPCtrlKsvRegMask.Value = 0xFFFFFFFF;
    HDCPDPCtrlKsvRegMask.CP_EN = 0;
    cbMMIOWriteReg32(pcbe, HDCP_REG_DP_CTRL_KSV[HDCPModuleIndex], HDCPDPCtrlKsvRegValue.Value, HDCPDPCtrlKsvRegMask.Value);
}

static CBIOS_BOOL cbDIU_DP_HDCP1x_IsEnabled(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM8374 HDCPDPCtrlKsvRegValue;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }

    HDCPDPCtrlKsvRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_DP_CTRL_KSV[HDCPModuleIndex]);

    return HDCPDPCtrlKsvRegValue.CP_EN ? CBIOS_TRUE : CBIOS_FALSE;
}

static CBIOS_VOID cbDIU_DP_HDCP2x_OnOff(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, CBIOS_BOOL bTurnOn)
{
    REG_MM33684 HDCP2xDPCtrlRegValue, HDCP2xDPCtrlRegMask;
    REG_MM33688 HDCP2xCtrlRegValue, HDCP2xCtrlRegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }
    
    if (bTurnOn)
    {
        HDCP2xDPCtrlRegValue.Value = 0;
        //HDCP2xDPCtrlRegValue.DPHDCP22_AUTH_SEL = 1; // disable HDCP 2.2 re-authentication reattemp
        //HDCP2xDPCtrlRegValue.DPHDCP22_AUTH_TRIG = 1;
        HDCP2xDPCtrlRegValue.DPHDCP22_CSM_TRIGGER = 1;
        HDCP2xDPCtrlRegValue.DPHDCP22_AKE_Stored_km_DIS = 0;
        HDCP2xDPCtrlRegValue.DPHDCP22_CAP_EN = 1;
        HDCP2xDPCtrlRegValue.DPHDCP22_CONT_STREAM_EN = 1;
        HDCP2xDPCtrlRegValue.DPHDCP22_CP_EN = 1;
        HDCP2xDPCtrlRegMask.Value = 0;
        cbMMIOWriteReg32(pcbe, HDCP_REG_2XDP_CTRL[HDCPModuleIndex], HDCP2xDPCtrlRegValue.Value, HDCP2xDPCtrlRegMask.Value);

        // HDCP 2.2 TX must set VERSION to 0x02
        HDCP2xCtrlRegValue.Value = 0;
        HDCP2xCtrlRegValue.TxCaps = 0x020000;
        HDCP2xCtrlRegMask.Value = 0xFFFFFFFF;
        HDCP2xCtrlRegMask.TxCaps = 0;
        cbMMIOWriteReg32(pcbe, HDCP_REG_2XCTRL[HDCPModuleIndex], HDCP2xCtrlRegValue.Value, HDCP2xCtrlRegMask.Value);
    }
    else
    {
        HDCP2xDPCtrlRegValue.Value = 0;
        HDCP2xDPCtrlRegMask.Value = 0;
        cbMMIOWriteReg32(pcbe, HDCP_REG_2XDP_CTRL[HDCPModuleIndex], HDCP2xDPCtrlRegValue.Value, HDCP2xDPCtrlRegMask.Value);
    }
}

static CBIOS_BOOL cbDIU_DP_HDCP2x_IsEnabled(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM33684 HDCP2xDPCtrlRegValue;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }

    HDCP2xDPCtrlRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_2XDP_CTRL[HDCPModuleIndex]);

    return HDCP2xDPCtrlRegValue.DPHDCP22_CP_EN ? CBIOS_TRUE : CBIOS_FALSE;
}

static CBIOS_VOID cbDIU_DP_HDCP_ReadBksvList(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, 
    PCBIOS_U8 pBksvList, CBIOS_U8 DeviceCount)
{
    CBIOS_U32 i;

    if (HDCPModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    if (DeviceCount == 0)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: no receiver connected\n", FUNCTION_NAME));
        return;
    }

    if (pBksvList == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pBksvList is NULL!\n", FUNCTION_NAME));
        return;
    }

    for (i = 0; i < DeviceCount; i++)
    {
        if (cbDIU_DP_HDCP_IsBKSVReady(pcbe, HDCPModuleIndex))
        {
            cbDIU_DP_HDCP_ReadBksv(pcbe, HDCPModuleIndex, pBksvList + i * 5);
            cbDIU_DP_HDCP_RevocationCheckDoneAck(pcbe, HDCPModuleIndex);
        }
        else
        {
            return;
        }
    }
}

static CBIOS_BOOL cbDIU_DP_HDCP_CheckIfRepeater(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM8378 HDCPDPCtrl2RegValue;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return  CBIOS_FALSE;
    }

    HDCPDPCtrl2RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_DP_CTRL2[HDCPModuleIndex]);
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: repeater = %d\n", FUNCTION_NAME, HDCPDPCtrl2RegValue.Repeater_Flag));

    return HDCPDPCtrl2RegValue.Repeater_Flag ? CBIOS_TRUE : CBIOS_FALSE;
}

#if 0
static CBIOS_U8 cbDIU_DP_HDCP_GetDeviceCount(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM8378 HDCPDPCtrl2RegValue;
    CBIOS_U8 DeviceCount = 0;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return  0;
    }

    // NOTE: 
    // HDCP 1.4 BKSV list HW fifo is  4 * BKSV. So supports repeater with maximum  4 receivers connected.
    // HDCP 2.2 BKSV list HW fifo is 31 * BKSV. So supports repeater with maximum 31 receivers connected.
    HDCPDPCtrl2RegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_DP_CTRL2[HDCPModuleIndex]);
    DeviceCount = (CBIOS_U8)HDCPDPCtrl2RegValue.Device_Count;
    cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Device Count = %d\n", FUNCTION_NAME, DeviceCount));

    return DeviceCount;
}
#endif

static CBIOS_VOID cbDIU_DP_HDCP_AuxOnOff(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, CBIOS_BOOL bTurnOn)
{
    REG_MM8374 HDCPDPCtrlKsvRegValue, HDCPDPCtrlKsvRegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    if (bTurnOn)
    {
        HDCPDPCtrlKsvRegValue.Value = 0x0;
        HDCPDPCtrlKsvRegValue.AUX_Fail_Config = 6; // 15 failures
        HDCPDPCtrlKsvRegValue.AUX_Def_Config = 3; // 7 defers
        HDCPDPCtrlKsvRegValue.Disable_AUX = 0;
        HDCPDPCtrlKsvRegMask.Value = 0xFFFFFFFF;
        HDCPDPCtrlKsvRegMask.AUX_Fail_Config = 0;
        HDCPDPCtrlKsvRegMask.AUX_Def_Config = 0;
        HDCPDPCtrlKsvRegMask.Disable_AUX = 0;
        cbMMIOWriteReg32(pcbe, HDCP_REG_DP_CTRL_KSV[HDCPModuleIndex], HDCPDPCtrlKsvRegValue.Value, HDCPDPCtrlKsvRegMask.Value);
    }
    else
    {
        HDCPDPCtrlKsvRegValue.Value = 0x0;
        HDCPDPCtrlKsvRegValue.Disable_AUX = 1;
        HDCPDPCtrlKsvRegMask.Value = 0xFFFFFFFF;
        HDCPDPCtrlKsvRegMask.Disable_AUX = 0;
        cbMMIOWriteReg32(pcbe, HDCP_REG_DP_CTRL_KSV[HDCPModuleIndex], HDCPDPCtrlKsvRegValue.Value, HDCPDPCtrlKsvRegMask.Value);
    }
}

#if HDCP_USE_TEST_KEY
// for internal test
static CBIOS_VOID cbDIU_DP_HDCP_TestKeyEnableDisable(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex, CBIOS_BOOL bEnable)
{
    REG_MM8374 HDCPDPCtrlKsvRegValue, HDCPDPCtrlKsvRegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    HDCPDPCtrlKsvRegValue.Value = 0x0;
    HDCPDPCtrlKsvRegValue.Test_Key_Enable = 1;
    HDCPDPCtrlKsvRegValue.DPHDCP_Test_Mode_Select = 1;
    HDCPDPCtrlKsvRegMask.Value = 0xFFFFFFFF;
    HDCPDPCtrlKsvRegMask.Test_Key_Enable = 0;
    HDCPDPCtrlKsvRegMask.DPHDCP_Test_Mode_Select = 0;
    cbMMIOWriteReg32(pcbe, HDCP_REG_DP_CTRL_KSV[HDCPModuleIndex], HDCPDPCtrlKsvRegValue.Value, HDCPDPCtrlKsvRegMask.Value);
}
#endif

static CBIOS_VOID cbDIU_HDCP_SwReset(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM82C4 HDCPMisc1RegValue, HDCPMisc1RegMask;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }
    
    HDCPMisc1RegValue.Value = 0;
    HDCPMisc1RegValue.HDCP_SW_Reset = 1;
    HDCPMisc1RegMask.Value = 0xFFFFFFFF;
    HDCPMisc1RegMask.HDCP_SW_Reset = 0;
    cbMMIOWriteReg32(pcbe, HDCP_REG_MISC1[CBIOS_MODULE_INDEX1], HDCPMisc1RegValue.Value, HDCPMisc1RegMask.Value);

    cb_DelayMicroSeconds(5000);//delay 5ms

    HDCPMisc1RegValue.Value = 0;
    HDCPMisc1RegValue.HDCP_SW_Reset = 0;
    HDCPMisc1RegMask.Value = 0xFFFFFFFF;
    HDCPMisc1RegMask.HDCP_SW_Reset = 0;
    cbMMIOWriteReg32(pcbe, HDCP_REG_MISC1[CBIOS_MODULE_INDEX1], HDCPMisc1RegValue.Value, HDCPMisc1RegMask.Value);
}

static CBIOS_U32 cbDIU_HDCP2x_ReadAndClearIntStatus(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    REG_MM3368C HDCP2xIntRegValue;

    if (HDCPModuleIndex >= HDCP_MODU_NUM)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return 0;
    }
    
    HDCP2xIntRegValue.Value = cb_ReadU32(pcbe->pAdapterContext, HDCP_REG_2XINT[HDCPModuleIndex]);
    // clear the interrupt
    cb_WriteU32(pcbe->pAdapterContext, HDCP_REG_2XINT[HDCPModuleIndex], HDCP2xIntRegValue.Value);

    //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: MM%x = 0x%x\n", FUNCTION_NAME, HDCP_REG_2XINT[HDCPModuleIndex], HDCP2xIntRegValue.Value));

    return HDCP2xIntRegValue.Value;
}

static CBIOS_VOID cbDIU_HDCP2x_Isr(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_MODULE_INDEX HDCPModuleIndex)
{
    PCBIOS_HDCP_CONTEXT pHdcpContext = GET_HDCP_CONTEXT(pcbe, Device);
    PCBIOS_DEVICE_COMMON  pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);
    CBIOS_U32 IntStatus = 0;

    if (HDCPModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    //init value of authstatus
    pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_ENABLE;

    IntStatus = cbDIU_HDCP2x_ReadAndClearIntStatus(pcbe, HDCPModuleIndex);

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_I2C_FAIL)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: I2C interaction fail\n", FUNCTION_NAME));
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_NOT_HDCP22)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: not a HDCP 2.2 supported device\n", FUNCTION_NAME));
        // disable HDCP 2.2 and enable HDCP 1.4
        if (cbDIU_HDMI_HDCP2x_IsEnabled(pcbe, HDCPModuleIndex))
        {
            cbDIU_HDMI_HDCP2x_OnOff(pcbe, HDCPModuleIndex, CBIOS_FALSE);
            cbDIU_HDMI_HDCP1x_OnOff(pcbe, HDCPModuleIndex, CBIOS_TRUE);
        }

        if (cbDIU_DP_HDCP2x_IsEnabled(pcbe, HDCPModuleIndex))
        {
            cbDIU_DP_HDCP2x_OnOff(pcbe, HDCPModuleIndex, CBIOS_FALSE);
            cbDIU_DP_HDCP1x_OnOff(pcbe, HDCPModuleIndex, CBIOS_TRUE);
        }

        pHdcpContext->HdcpVersion = CBIOS_HDCP_VERSION_1x;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_IS_HDCP22)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: is a HDCP 2.2 supported device\n", FUNCTION_NAME));
        pHdcpContext->HdcpVersion = CBIOS_HDCP_VERSION_2x;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_REAUTH_REQ)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: re-authentication request\n", FUNCTION_NAME));
        cbHDCP_OnOff(pcbe, Device, HDCPModuleIndex, CBIOS_FALSE);
        cbDelayMilliSeconds(120);
        cbHDCP_OnOff(pcbe, Device, HDCPModuleIndex, CBIOS_TRUE);

        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_AUTH_FAIL)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: authentication fail\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    //last ints are to repeater

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_RECID_INVALID)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: receiver id invalid\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_CERT_TIMEOUT)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: CERT timeout\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_CERT_FAIL)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: CERT fail\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_H_FAIL)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: H fail\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_H_TIMEOUT)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: H timeout\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_EKH_TIMEOUT)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: eKh timeout\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_L_FAIL)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: L fail\n", FUNCTION_NAME));
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_LC_TIMEOUT)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: LC timeout\n", FUNCTION_NAME));
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_LC_RETRY_NUMOUT)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: LC retry num out\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_MSG_ID)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: msg id\n", FUNCTION_NAME));
        cbHDCP_OnOff(pcbe, Device, HDCPModuleIndex, CBIOS_FALSE);
        cbDelayMilliSeconds(120);
        cbHDCP_OnOff(pcbe, Device, HDCPModuleIndex, CBIOS_TRUE);
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_GET_RECID)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: read receiver id\n", FUNCTION_NAME));
        if (pDevCommon->CurrentMonitorType == CBIOS_MONITOR_TYPE_DP)
        {
            if (cbDIU_DP_HDCP_IsBKSVReady(pcbe, HDCPModuleIndex))
            {
                cbDIU_DP_HDCP_ReadBksv(pcbe, HDCPModuleIndex, pHdcpContext->BKsv);
            }
            if(cbHDCP_CheckRevokedBksv(pcbe, pHdcpContext->BKsv, 5, 
                pHdcpContext->pRevocationList, pHdcpContext->RevocationListBufferSize))
            {
                pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
                return;
            }
            else
            {
                 cbDIU_DP_HDCP_RevocationCheckDoneAck(pcbe, HDCPModuleIndex);
            }
        }
        else
        {
            if (cbDIU_HDMI_HDCP_IsBKSVReady(pcbe, HDCPModuleIndex))
            {
                cbDIU_HDMI_HDCP_ReadBksv(pcbe, HDCPModuleIndex, pHdcpContext->BKsv);
            }
            if(cbHDCP_CheckRevokedBksv(pcbe, pHdcpContext->BKsv, 5, 
                pHdcpContext->pRevocationList, pHdcpContext->RevocationListBufferSize))
            {
                pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
                return;
            }
            else
            {
                 cbDIU_HDMI_HDCP_RevocationCheckDoneAck(pcbe, HDCPModuleIndex);
            }
        }
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_SEQ_NUM_M_ROLLOVER)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: seq num M roll over\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    //last ints are to repeater
    if (IntStatus & CBIOS_HDCP2x_INT_MASK_WAIT_RECID_TIMEOUT)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: wait receiver id timeout\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_V_FAIL)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: V fail\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_DEV_ZERO)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: zero device\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_MAX_DEVS)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: max devices\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_MAX_CAS)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: max cascade\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_NONZERO_SEQ_NUM_V)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: non-zero seq num V\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_SEQ_NUM_V_ROLLOVER)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: seq num V roll over\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_M_FAIL)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: M fail\n", FUNCTION_NAME));
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_M_TIMEOUT)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: M timeout\n", FUNCTION_NAME));
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_M_RETRY_NUMOUT)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: M retry num out\n", FUNCTION_NAME));
        cbHDCP_OnOff(pcbe, Device, HDCPModuleIndex, CBIOS_FALSE);
        cbDelayMilliSeconds(120);
        cbHDCP_OnOff(pcbe, Device, HDCPModuleIndex, CBIOS_TRUE);
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_FAIL;
        return;
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_CSM_FAIL)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: CSM fail\n", FUNCTION_NAME));
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_CSM_PASS)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: CSM pass\n", FUNCTION_NAME));
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_KSVFF)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: KSV fifo full\n", FUNCTION_NAME));
    }

    if (IntStatus & CBIOS_HDCP2x_INT_MASK_AUTH_PASS)
    {
        //cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: Got int: authentication pass\n", FUNCTION_NAME));
        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_PASS;
    }
}


static CBIOS_VOID cbHDCP_InitHdcpContext(PCBIOS_VOID pvcbe, PCBIOS_HDCP_CONTEXT pHdcpContext)
{
    if(pHdcpContext == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: pHdcpContext is NULL!\n", FUNCTION_NAME));
        return;
    }

    pHdcpContext->HdcpVersion = CBIOS_HDCP_VERSION_2x;
    pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_DISABLE;
    pHdcpContext->bRepeater = CBIOS_FALSE;
    pHdcpContext->DeviceCount = 0;
    pHdcpContext->bRepeaterAuthPass = CBIOS_FALSE;
    pHdcpContext->HdcpReAuthTryCount = CBIOS_HDCP_REAUTH_TRY_COUNT_MAX;

    if (pHdcpContext->pBKsvList)
    {
        cb_FreePool(pHdcpContext->pBKsvList);
        pHdcpContext->pBKsvList = CBIOS_NULL;
    }
    pHdcpContext->BKsvListBufferSize = 0;

    pHdcpContext->pRevocationList = (PCBIOS_U8)Fake_RevokedBksv;
    pHdcpContext->RevocationListBufferSize = sizeof(Fake_RevokedBksv);
}

static CBIOS_VOID cbHDMI_HDCP_OnOff(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_U8 IGAIndex, CBIOS_BOOL bTurnOn)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_MODULE_INDEX  HDCPModuleIndex = CBIOS_MODULE_INDEX_INVALID;
    CBIOS_BOOL          bUseI2CModule = CBIOS_TRUE;
    PCBIOS_DEVICE_COMMON  pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);
    PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttr = CBIOS_NULL;
    PCBIOS_HDCP_CONTEXT pHdcpContext = GET_HDCP_CONTEXT(pvcbe, Device);

    cbTraceEnter(GENERIC);

    HDCPModuleIndex = cbGetModuleIndex(pcbe, Device, CBIOS_MODULE_TYPE_HDCP);
    if (HDCPModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    pMonitorAttr = &(pDevCommon->EdidStruct.Attribute);

    if ((Device == CBIOS_TYPE_MHL) && (pDevCommon->CurrentMonitorType == CBIOS_MONITOR_TYPE_MHL))
    {
        bUseI2CModule = CBIOS_FALSE;
    }

    if(bUseI2CModule & bTurnOn)
    {
        cbDIU_HDMI_HDCP_HwI2cOnOff(pcbe, HDCPModuleIndex, (CBIOS_U8)pDevCommon->I2CBus, bTurnOn);
    }

    if (bTurnOn)
    {
        // NOTE: HDCP enable/disable registers take effects right away instead of at vsync.
        //       So need to wait vblank to prevent garbage show when enable/disable HDCP.
        cbWaitVBlank(pcbe, IGAIndex);
        /* hdcp reset will lead to black screen, comment it. But we should call hdcp reset if run hdcp cts*/
        //cbDIU_HDCP_SwReset(pcbe, HDCPModuleIndex);

#if HDCP_USE_TEST_KEY
        cbDIU_HDMI_HDCP_TestKeyEnableDisable(pcbe, HDCPModuleIndex, CBIOS_TRUE);
#endif

        cbDIU_HDMI_HDCP_RevocationCheckEnableDisable(pcbe, HDCPModuleIndex, CBIOS_TRUE);
        cbDIU_HDMI_HDCP_EncryptionEnableDisable(pcbe, HDCPModuleIndex, CBIOS_TRUE);

        if (pMonitorAttr->IsCEA861HDMI)
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: choose HDMI protocol\n", FUNCTION_NAME));
            cbDIU_HDMI_HDCP_ChooseProtocol(pcbe, HDCPModuleIndex, IGAIndex, pHdcpContext->HdcpVersion, CBIOS_TRUE);
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(GENERIC, INFO), "%s: choose DVI protocol\n", FUNCTION_NAME));
            cbDIU_HDMI_HDCP_ChooseProtocol(pcbe, HDCPModuleIndex, IGAIndex, pHdcpContext->HdcpVersion, CBIOS_FALSE);
        }

        pHdcpContext->HdcpVersion = CBIOS_HDCP_VERSION_2x;
        // clear HDCP 2.2 interrupt status before HDCP 2.2 enable
        cbDIU_HDCP2x_ReadAndClearIntStatus(pcbe, HDCPModuleIndex);
        // clear BKSV ready bit
        cbDIU_HDMI_HDCP_RevocationCheckDoneAck(pcbe, HDCPModuleIndex);

        cbDIU_HDMI_HDCP2x_OnOff(pcbe, HDCPModuleIndex, bTurnOn);

        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_ENABLE;
    }
    else
    {
        cbDIU_HDMI_HDCP_EncryptionEnableDisable(pcbe, HDCPModuleIndex, CBIOS_FALSE);

        if (cbDIU_HDMI_HDCP1x_IsEnabled(pcbe, HDCPModuleIndex))
        {
            cbDIU_HDMI_HDCP1x_OnOff(pcbe, HDCPModuleIndex, bTurnOn);
        }

        if (cbDIU_HDMI_HDCP2x_IsEnabled(pcbe, HDCPModuleIndex))
        {
            cbDIU_HDMI_HDCP2x_OnOff(pcbe, HDCPModuleIndex, bTurnOn);
        }
        
        /* hdcp reset will lead to black screen, comment it. But we should call hdcp reset if run hdcp cts*/
        //cbDIU_HDCP_SwReset(pcbe, HDCPModuleIndex);

        if(bUseI2CModule)
        {
            cbDIU_HDMI_HDCP_HwI2cOnOff(pcbe, HDCPModuleIndex, (CBIOS_U8)pDevCommon->I2CBus, bTurnOn);
        }

        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_DISABLE;
        cbHDCP_InitHdcpContext(pvcbe, pHdcpContext); // reset HDCP context
    }

    cbTraceExit(GENERIC);
}

CBIOS_VOID cbDP_HDCP_OnOff(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_U8 IGAIndex, CBIOS_BOOL bTurnOn)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_MODULE_INDEX  HDCPModuleIndex = CBIOS_MODULE_INDEX_INVALID;
    PCBIOS_HDCP_CONTEXT pHdcpContext = GET_HDCP_CONTEXT(pvcbe, Device);

    cbTraceEnter(GENERIC);

    HDCPModuleIndex = cbGetModuleIndex(pcbe, Device, CBIOS_MODULE_TYPE_HDCP);
    if (HDCPModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    // NOTE: HDCP enable/disable registers take effects right away instead of at vsync.
    //       So need to wait vblank to prevent garbage show when enable/disable HDCP.
    cbWaitVBlank(pcbe, IGAIndex);

    if (bTurnOn)
    {
        //cbDIU_HDCP_SwReset(pcbe, HDCPModuleIndex);

        cbDIU_DP_HDCP_AuxOnOff(pcbe, HDCPModuleIndex, bTurnOn);

#if HDCP_USE_TEST_KEY
        cbDIU_DP_HDCP_TestKeyEnableDisable(pcbe, HDCPModuleIndex, CBIOS_TRUE);
#endif

        cbDIU_DP_HDCP_RevocationCheckEnableDisable(pcbe, HDCPModuleIndex, CBIOS_TRUE);
        cbDIU_DP_HDCP_EncryptionEnableDisable(pcbe, HDCPModuleIndex, CBIOS_TRUE);

        pHdcpContext->HdcpVersion = CBIOS_HDCP_VERSION_2x;
        
        // clear HDCP 2.2 interrupt status before HDCP 2.2 enable
        cbDIU_HDCP2x_ReadAndClearIntStatus(pcbe, HDCPModuleIndex);
        // clear BKSV ready bit
        cbDIU_DP_HDCP_RevocationCheckDoneAck(pcbe, HDCPModuleIndex);
        
        cbDIU_DP_HDCP2x_OnOff(pcbe, HDCPModuleIndex, bTurnOn);

        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_ENABLE;
    }
    else
    {
        cbDIU_DP_HDCP_EncryptionEnableDisable(pcbe, HDCPModuleIndex, CBIOS_FALSE);

        if (cbDIU_DP_HDCP1x_IsEnabled(pcbe, HDCPModuleIndex))
        {
            cbDIU_DP_HDCP1x_OnOff(pcbe, HDCPModuleIndex, bTurnOn);
        }

        if (cbDIU_DP_HDCP2x_IsEnabled(pcbe, HDCPModuleIndex))
        {
            cbDIU_DP_HDCP2x_OnOff(pcbe, HDCPModuleIndex, bTurnOn);
        }

        //cbDIU_HDCP_SwReset(pcbe, HDCPModuleIndex);

        cbDIU_DP_HDCP_AuxOnOff(pcbe, HDCPModuleIndex, bTurnOn);

        pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_DISABLE;
        cbHDCP_InitHdcpContext(pvcbe, pHdcpContext); // reset HDCP context
    }

    cbTraceExit(GENERIC);
}

CBIOS_VOID cbHDCP_OnOff(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_U8 IGAIndex, CBIOS_BOOL bTurnOn)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_MODULE_INDEX  HDCPModuleIndex = CBIOS_MODULE_INDEX_INVALID;
    CBIOS_BOOL          bDPMonitor = CBIOS_FALSE;
    PCBIOS_DEVICE_COMMON  pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);
    //PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttr = CBIOS_NULL;

    cbTraceEnter(GENERIC);

    if (pDevCommon == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pDevCommon is NULL!\n", FUNCTION_NAME));
        return;
    }

    HDCPModuleIndex = cbGetModuleIndex(pcbe, Device, CBIOS_MODULE_TYPE_HDCP);
    if (HDCPModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    //pMonitorAttr = &(pDevCommon->EdidStruct.Attribute);

    if (pDevCommon->CurrentMonitorType == CBIOS_MONITOR_TYPE_DP)
    {
        bDPMonitor = CBIOS_TRUE;
    }

    if (bDPMonitor)
    {
        cbDP_HDCP_OnOff(pcbe, Device, IGAIndex, bTurnOn);
    }
    else
    {
        cbHDMI_HDCP_OnOff(pcbe, Device, IGAIndex, bTurnOn);
    }

    cbTraceExit(GENERIC);
}

CBIOS_VOID cbHDCP_Isr(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    CBIOS_MODULE_INDEX HDCPModuleIndex = CBIOS_MODULE_INDEX_INVALID;

    HDCPModuleIndex = cbGetModuleIndex(pcbe, Device, CBIOS_MODULE_TYPE_HDCP);
    if (HDCPModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return;
    }

    if (cbDIU_HDMI_HDCP1x_IsEnabled(pcbe, HDCPModuleIndex))
    {
        cbDIU_HDMI_HDCP1x_Isr(pcbe, Device, HDCPModuleIndex);
    }

    if (cbDIU_DP_HDCP1x_IsEnabled(pcbe, HDCPModuleIndex))
    {
        cbDIU_DP_HDCP1x_Isr(pcbe, Device, HDCPModuleIndex);
    }

    if (cbDIU_HDMI_HDCP2x_IsEnabled(pcbe, HDCPModuleIndex) || cbDIU_DP_HDCP2x_IsEnabled(pcbe, HDCPModuleIndex))
    {
        cbDIU_HDCP2x_Isr(pcbe, Device, HDCPModuleIndex);
    }
}

CBIOS_BOOL cbHDCP_GetHDCPBKsv(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, PCBIOS_U8 pBKsv, PCBIOS_BOOL bRepeater)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_HDCP_CONTEXT pHdcpContext = GET_HDCP_CONTEXT(pcbe, Device);

    if(pHdcpContext == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: pHdcpContext is NULL!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    if(pBKsv == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: pBKsv is NULL!\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }
    
    cb_memcpy(pBKsv, pHdcpContext->BKsv, sizeof(pHdcpContext->BKsv));
    *bRepeater = pHdcpContext->bRepeater;

    return CBIOS_TRUE;
}

CBIOS_HDCP_AUTHENTICATION_STATUS cbHDCP_GetStatus(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_HDCP_CONTEXT pHdcpContext = GET_HDCP_CONTEXT(pcbe, Device);
    CBIOS_MODULE_INDEX HDCPModuleIndex = CBIOS_MODULE_INDEX_INVALID;

    if(pHdcpContext == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: pHdcpContext is NULL!\n", FUNCTION_NAME));
        return CBIOS_HDCP_AUTH_FAIL;
    }

    HDCPModuleIndex = cbGetModuleIndex(pcbe, Device, CBIOS_MODULE_TYPE_HDCP);
    if (HDCPModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
        return CBIOS_HDCP_AUTH_FAIL;
    }

    return pHdcpContext->AuthStatus;
}

CBIOS_VOID cbHDCP_WorkThread(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device, CBIOS_U8 IGAIndex)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_HDCP_CONTEXT pHdcpContext = GET_HDCP_CONTEXT(pcbe, Device);
    CBIOS_MODULE_INDEX HDCPModuleIndex = CBIOS_MODULE_INDEX_INVALID;
    CBIOS_BOOL          bDPMonitor = CBIOS_FALSE;
    PCBIOS_DEVICE_COMMON  pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);
    //PCBIOS_MONITOR_MISC_ATTRIB pMonitorAttr = CBIOS_NULL;

    if(pHdcpContext == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: pHdcpContext is NULL!\n", FUNCTION_NAME));
    }

    HDCPModuleIndex = cbGetModuleIndex(pcbe, Device, CBIOS_MODULE_TYPE_HDCP);
    if (HDCPModuleIndex == CBIOS_MODULE_INDEX_INVALID)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: invalid HDCP module index!\n", FUNCTION_NAME));
    }

    //pMonitorAttr = &(pDevCommon->EdidStruct.Attribute);

    if (pDevCommon->CurrentMonitorType == CBIOS_MONITOR_TYPE_DP)
    {
        bDPMonitor = CBIOS_TRUE;
    }
    
    if (pHdcpContext->AuthStatus == CBIOS_HDCP_AUTH_BKSV)
    {
        if (bDPMonitor)
        {
            if (cbDIU_DP_HDCP_IsBKSVReady(pcbe, HDCPModuleIndex))
            {
                cbDIU_DP_HDCP_ReadBksv(pcbe, HDCPModuleIndex, pHdcpContext->BKsv);
            }
        }
        else
        {
            if (cbDIU_HDMI_HDCP_IsBKSVReady(pcbe, HDCPModuleIndex))
            {
                cbDIU_HDMI_HDCP_ReadBksv(pcbe, HDCPModuleIndex, pHdcpContext->BKsv);
            }
        }

        if(cbHDCP_CheckRevokedBksv(pcbe, pHdcpContext->BKsv, 5, 
            pHdcpContext->pRevocationList, pHdcpContext->RevocationListBufferSize))
        {
            cbHDCP_OnOff(pcbe, Device, IGAIndex, CBIOS_FALSE);
            pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_DISABLE;
        }
        else
        {
            if (bDPMonitor)
            {
                cbDIU_DP_HDCP_RevocationCheckDoneAck(pcbe, HDCPModuleIndex);
                pHdcpContext->bRepeater = cbDIU_DP_HDCP_CheckIfRepeater(pcbe, HDCPModuleIndex);
            }
            else
            {
                cbDIU_HDMI_HDCP_RevocationCheckDoneAck(pcbe, HDCPModuleIndex);
                pHdcpContext->bRepeater = cbDIU_HDMI_HDCP_CheckIfRepeater(pcbe, HDCPModuleIndex);
            }
            pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_BKSV_VERIF_DONE;
        }
    }
    else if (pHdcpContext->AuthStatus == CBIOS_HDCP_AUTH_BKSV_LIST)
    {
        if (pHdcpContext->bRepeater)
        {
            if (pHdcpContext->bRepeater)
            {
                pHdcpContext->DeviceCount = cbDIU_HDMI_HDCP_GetDeviceCount(pcbe, HDCPModuleIndex);
            }

            if (pHdcpContext->DeviceCount > 0)
            {
                pHdcpContext->BKsvListBufferSize = pHdcpContext->DeviceCount * 5;
                if (pHdcpContext->pBKsvList == CBIOS_NULL)
                {
                    pHdcpContext->pBKsvList = cb_AllocateNonpagedPool(pHdcpContext->BKsvListBufferSize);
                }

                if (bDPMonitor)
                {
                    cbDIU_DP_HDCP_ReadBksvList(pcbe, HDCPModuleIndex, pHdcpContext->pBKsvList, pHdcpContext->DeviceCount);
                }
                else
                {
                    cbDIU_HDMI_HDCP_ReadBksvList(pcbe, HDCPModuleIndex, pHdcpContext->pBKsvList, pHdcpContext->DeviceCount);
                }

                if(cbHDCP_CheckRevokedBksv(pcbe, pHdcpContext->pBKsvList, pHdcpContext->BKsvListBufferSize, 
                    pHdcpContext->pRevocationList, pHdcpContext->RevocationListBufferSize))
                {
                    cbHDCP_OnOff(pcbe, Device, IGAIndex, CBIOS_FALSE);
                    pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_DISABLE;
                }
                else
                {
                    pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_BKSV_LIST_VERIF_DONE;
                }
            }
            else
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, INFO),"%s: no receiver connected to repeater\n", FUNCTION_NAME));
                pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_BKSV_LIST_VERIF_DONE;
            }

            if (pHdcpContext->bRepeaterAuthPass)
            {
                cbDebugPrint((MAKE_LEVEL(GENERIC, INFO),"%s: repeater auth pass\n", FUNCTION_NAME));
                pHdcpContext->AuthStatus = CBIOS_HDCP_AUTH_PASS;
            }
        }
    }
}

CBIOS_VOID cbHDCP_Init(PCBIOS_VOID pvcbe, PCBIOS_VOID *ppHdcpContext)
{
    PCBIOS_HDCP_CONTEXT pHdcpContext = CBIOS_NULL;

    pHdcpContext = cb_AllocateNonpagedPool(sizeof(CBIOS_HDCP_CONTEXT));
    if(pHdcpContext == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR),"%s: pHdcpContext allocate error!!!\n", FUNCTION_NAME));
        return;
    }

    cbHDCP_InitHdcpContext(pvcbe, pHdcpContext);

    *ppHdcpContext = pHdcpContext;
}

CBIOS_VOID cbHDCP_DeInit(PCBIOS_VOID pvcbe, CBIOS_ACTIVE_TYPE Device)
{
    PCBIOS_EXTENSION_COMMON pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_DEVICE_COMMON pDevCommon = cbGetDeviceCommon(&pcbe->DeviceMgr, Device);

    if (pDevCommon->pHDCPContext)
    {
        cb_FreePool(pDevCommon->pHDCPContext);
        pDevCommon->pHDCPContext = CBIOS_NULL;
    }
}
