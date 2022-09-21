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
** CBios sw initialization function.
** Initialize CBIOS_EXTENSION_COMMON structure.
**
** NOTE:
** The hw dependent initialization SHOULD NOT be added to this file.
******************************************************************************/

#include "CBiosChipShare.h"
#include "CBiosDevice.h"

extern CBIOS_HDMI_FORMAT_MTX CEAVideoFormatTable[CBIOS_HDMIFORMATCOUNTS];


static CBIOS_VOID cbCbiosChipCapsInit(PCBIOS_EXTENSION_COMMON pcbe)
{
    switch(pcbe->ChipID)
    {
        case CHIPID_E3K:
        case CHIPID_ARISE1020:
            pcbe->ChipCaps.IsSupportUpScaling         = CBIOS_TRUE;
            pcbe->ChipCaps.bNoMemoryControl             = CBIOS_FALSE;
            pcbe->ChipCaps.Is24MReferenceClock          = CBIOS_FALSE;
            pcbe->ChipCaps.IsSupport3DVideo             = CBIOS_TRUE;
            pcbe->ChipCaps.IsSupportDeepColor           = CBIOS_TRUE;
            pcbe->ChipCaps.IsSupportCEC                 = CBIOS_TRUE;
            pcbe->ChipCaps.bSupportConfigEclkByEfuse    = CBIOS_FALSE;
            pcbe->ChipCaps.bSupportScrambling           = CBIOS_TRUE;
            pcbe->ChipCaps.bSupportReadRequest          = CBIOS_FALSE;
            pcbe->ChipLimits.ulMaxPUHorSrc              = 2560;
            pcbe->ChipLimits.ulMaxPUVerSrc              = 1600;
            pcbe->ChipLimits.ulMaxPUHorDst              = 4096;
            pcbe->ChipLimits.ulMaxPUVerDst              = 4096;
            pcbe->ChipLimits.ulMaxHDMIClock             = 6000000;
            pcbe->ChipLimits.ulMaxIGAClock              = 6000000;
            break;
        default:
            pcbe->ChipCaps.IsSupportUpScaling         = CBIOS_FALSE;
            pcbe->ChipCaps.bNoMemoryControl             = CBIOS_TRUE;
            pcbe->ChipCaps.IsSupportDeepColor           = CBIOS_FALSE;
            pcbe->ChipCaps.IsSupportCEC                 = CBIOS_FALSE;
            pcbe->ChipCaps.bSupportConfigEclkByEfuse    = CBIOS_FALSE;
            pcbe->ChipCaps.bSupportScrambling           = CBIOS_FALSE;
            pcbe->ChipCaps.bSupportReadRequest          = CBIOS_FALSE;
            pcbe->ChipLimits.ulMaxPUHorSrc              = 1600;
            pcbe->ChipLimits.ulMaxPUVerSrc              = 1200;
            pcbe->ChipLimits.ulMaxPUHorDst              = 4096;
            pcbe->ChipLimits.ulMaxPUVerDst              = 4096;
            pcbe->ChipLimits.ulMaxHDMIClock             = 1650000;
            pcbe->ChipLimits.ulMaxIGAClock              = 1650000;
            break;
    }
    if(pcbe->bRunOnQT)
    {
        pcbe->ChipCaps.bNoMemoryControl = CBIOS_FALSE;
    }
}


//SW Initialization: init CBIOS_EXTENSION_COMMON structure, and port struct init
CBIOS_BOOL cbInitialize(PCBIOS_VOID pvcbe, PCBIOS_PARAM_INIT pCBParamInit)
{
    PCBIOS_EXTENSION_COMMON    pcbe = (PCBIOS_EXTENSION_COMMON)pvcbe;
    PCBIOS_VOID                pRomBase = pCBParamInit->RomImage; 
    PCBIOS_VOID                pVCPBase = CBIOS_NULL;
    CBIOS_U32                  i = 0;
    PVCP_INFO              pVCP = CBIOS_NULL;

    cbTraceEnter(GENERIC);
    
    pcbe->pSpinLock = pCBParamInit->pSpinLock;
    pcbe->pAuxMutex = pCBParamInit->pAuxMutex;
    pcbe->pAdapterContext = pCBParamInit->pAdapterContext;
    pcbe->bMAMMPrimaryAdapter = pCBParamInit->MAMMPrimaryAdapter;
    pcbe->PCIDeviceID = pCBParamInit->PCIDeviceID;
    pcbe->ChipID = pCBParamInit->ChipID;
    pcbe->ChipRevision = pCBParamInit->ChipRevision;
    pcbe->BoardVersion = CBIOS_BOARD_VERSION_DEFAULT;
    pcbe->bRunOnQT = pCBParamInit->bRunOnQT;
    pcbe->bDriverLoadQTiming = pCBParamInit->bDriverLoadQTiming;

    for(i = 0;i < CBIOS_MAX_I2CBUS;i++)
    {
        pcbe->pI2CMutex[i] = pCBParamInit->pI2CMutex[i];
    }
    
    cbHWInitChipAttribute((PCBIOS_VOID)pcbe, pcbe->ChipID);

    //init HDMI format table
    pcbe->pHDMIFormatTable = CEAVideoFormatTable;
    
    //init Memory Type
    pcbe->MemoryType = Default_Mem_Type; //It will be reset once when change clocks

    //Init cbios chip caps first
    cbCbiosChipCapsInit(pcbe);
    
    //Init VCP info structure.
    pVCP = cb_AllocatePagedPool(sizeof(VCP_INFO));

    if(pVCP == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(GENERIC, ERROR), "%s: pVCP allocate error\n", FUNCTION_NAME));
        return CBIOS_FALSE;
    }

    /*find VCP in RomBase,otherwise in RomBase + 64kb + 4kb*/
    pVCPBase = pRomBase;
    if(pVCPBase && cb_swab16(*(CBIOS_U16*)((CBIOS_U8*)pVCPBase+ VCP_OFFSET))!= VCP_TAG)
    {
        pVCPBase = CBIOS_NULL;          
    }

    if(!cbInitVCP(pcbe, pVCP, pVCPBase))
    {
        return CBIOS_FALSE;
    }
    else
    {
        //Init table.
        pcbe->sizeofBootDevPriority = pVCP->sizeofBootDevPriority;
        pcbe->PostRegTable[VCP_TABLE].sizeofCRDefault = pVCP->sizeofCR_DEFAULT_TABLE;
        pcbe->PostRegTable[VCP_TABLE].sizeofSRDefault = pVCP->sizeofSR_DEFAULT_TABLE;
        pcbe->PostRegTable[VCP_TABLE].sizeofModeExtRegDefault_TBL = 0;

        pcbe->pBootDevPriority = cb_AllocateNonpagedPool(sizeof(CBIOS_U16) * pVCP->sizeofBootDevPriority);
        pcbe->PostRegTable[VCP_TABLE].pCRDefault = cb_AllocateNonpagedPool(sizeof(CBREGISTER) * pVCP->sizeofCR_DEFAULT_TABLE);
        pcbe->PostRegTable[VCP_TABLE].pSRDefault = cb_AllocateNonpagedPool(sizeof(CBREGISTER) * pVCP->sizeofSR_DEFAULT_TABLE);
        pcbe->PostRegTable[VCP_TABLE].pModeExtRegDefault_TBL = CBIOS_NULL;

        cb_memcpy(pcbe->pBootDevPriority, pVCP->BootDevPriority, sizeof(CBIOS_U16) * pVCP->sizeofBootDevPriority);
        cb_memcpy(pcbe->PostRegTable[VCP_TABLE].pCRDefault, pVCP->pCR_DEFAULT_TABLE, sizeof(CBREGISTER) * pVCP->sizeofCR_DEFAULT_TABLE);
        cb_memcpy(pcbe->PostRegTable[VCP_TABLE].pSRDefault, pVCP->pSR_DEFAULT_TABLE, sizeof(CBREGISTER) * pVCP->sizeofSR_DEFAULT_TABLE);
    }

    cb_memcpy(&pcbe->FeatureSwitch, &pVCP->FeatureSwitch, sizeof(CBIOS_U32));
   
    pcbe->bUseVCP = pVCP->bUseVCP;
    pcbe->BiosVersion = pVCP->BiosVersion;
    pcbe->EClock = pVCP->EClock;
    pcbe->EVcoLow = pVCP->EVcoLow;
    pcbe->DVcoLow = pVCP->DVcoLow;
    pcbe->DeviceMgr.SupportDevices = pVCP->SupportDevices;
    pcbe->SliceNum = pVCP->SliceNum;

    for (i = IGA1; i < CBIOS_IGACOUNTS; i++)
    {
        pcbe->DispMgr.ActiveDevices[i] = CBIOS_TYPE_NONE;
    }

    //RealVision requires I2C speed up to 400kbps
    if ((pVCP->SubVendorID == 0x12EA) && (pVCP->SubSystemID == 0x0002))
    {
        pcbe->I2CDelay = 3;
    }
    else
    {
        pcbe->I2CDelay = I2C_DELAY_DEFAULT;
    }

    //initialize CEC para
    for (i = 0; i < CBIOS_CEC_INDEX_COUNT; i++)
    {
        pcbe->CECPara[i].CECEnable = CBIOS_FALSE;
        pcbe->CECPara[i].LogicalAddr = CEC_UNREGISTERED_DEVICE;
        pcbe->CECPara[i].PhysicalAddr = CEC_INVALID_PHYSICAL_ADDR;
    }

    if(((pcbe->ChipID == CHIPID_E3K) || (pcbe->ChipID == CHIPID_ARISE1020)) && pRomBase)
    {
         // get vbios version from vbios rom
         if(cb_swab16(*(CBIOS_U16*)((CBIOS_U8*)pRomBase + VCP_OFFSET))== VCP_TAG)
         {
            pcbe->BiosVersion = cb_swab32(*((PCBIOS_U32)((PCBIOS_CHAR)pRomBase + VCP_OFFSET + 4)));
         }
    }
    
    cbInitDeviceArray(pcbe, pVCP);

    cbDispMgrInit(pcbe);

    if(pVCP->pCR_DEFAULT_TABLE)
    {
        cb_FreePool(pVCP->pCR_DEFAULT_TABLE);
        pVCP->pCR_DEFAULT_TABLE = CBIOS_NULL;
    }
    if(pVCP->pSR_DEFAULT_TABLE)
    {
        cb_FreePool(pVCP->pSR_DEFAULT_TABLE);
        pVCP->pSR_DEFAULT_TABLE = CBIOS_NULL;
    }
    cb_FreePool(pVCP);

    return CBIOS_TRUE;
}


