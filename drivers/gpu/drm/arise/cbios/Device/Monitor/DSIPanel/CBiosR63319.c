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
** R63319 type DSI panel descriptor definition, 
** which contains panel config and command lists.
**
** NOTE:
**
******************************************************************************/


#include "CBiosDSIPanel.h"
#include "CBiosDSI.h"

static CBIOS_U8 EnterSleepBuf[0x01] =
{
    0x10
};

static CBIOS_U8 ExitSleepBuf[0x01] =
{
    0x11
};


static CBIOS_U8 DisplayOffBuf[0x01] =
{
    0x28
};

static CBIOS_U8 DisplayOnBuf[0x01] =
{
    0x29
};


static CBIOS_U8 SetBacklightBuf[0x02] =
{
    0x51, 0xFF
};
static CBIOS_U8 GetBacklightBuf[0x01] =
{
    0x52
};

static CBIOS_U8 PanelAddrMode[0x02] =
{
    0x36, 0x40
};


static CBIOS_U8 PWMBuf[0x2] =
{
    0x53, 0x24
};


static CBIOS_U8 BrightnessCtrlBuf[0x2] =
{
    0x55, 0x00
};


static CBIOS_U8 TEEnable[0x02] =
{
    0x35, 0x00
};


static CBIOS_DSI_CMD_DESC R63319_PowerOn_CmdList[] =
{
    {1, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 120, sizeof(ExitSleepBuf), ExitSleepBuf,      {0}},

    {1, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(PWMBuf),       PWMBuf,            {0}},
    {1, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(BrightnessCtrlBuf),    BrightnessCtrlBuf, {0}},
    {1, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(TEEnable),     TEEnable,          {0}},
    {1, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(PanelAddrMode),PanelAddrMode,     {0}},

    {1, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(DisplayOnBuf), DisplayOnBuf,      {0}},
    {1, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(SetBacklightBuf), SetBacklightBuf,  {0}},
};

static CBIOS_DSI_CMD_DESC R63319_PowerOff_CmdList[] =
{
    {1, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0,   sizeof(DisplayOffBuf), DisplayOffBuf, {0}},
    {1, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 120, sizeof(EnterSleepBuf), EnterSleepBuf, {0}},
};

static CBIOS_DSI_CMD_DESC R63319_DisplayOn_CmdList[] = 
{
    {1, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 10, sizeof(DisplayOnBuf), DisplayOnBuf,   {0}},
};

static CBIOS_DSI_CMD_DESC R63319_DisplayOff_CmdList[] = 
{
    {1, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 10, sizeof(DisplayOffBuf), DisplayOffBuf, {0}},
};

static CBIOS_DSI_CMD_DESC R63319_Backlight_CmdList[] =
{
    {1, 0, CBIOS_DSI_SHORT_PACKET, CBIOS_DSI_CONTENT_DCS, 0, sizeof(SetBacklightBuf), SetBacklightBuf, {0}},
};

// important note:
//        These are indexes of below gpio ports table with corresponding usage. So they must keep synchronization with blow 
//        gpio ports table. If gpio ports table changes with usage, these indexes must be changed too.
#define R63319_INDEX_IOVDD        0
#define R63319_INDEX_DCDCEN       1
#define R63319_INDEX_RESX         2
#define R63319_INDEX_AVDD         3

// GPIO PORTS for board version A0 & A1
GPIO_PORT R63319_GPIO_PORTS_A1[] = 
{
    {CBIOS_GPIO_AUD,  18},  //IOVDD
    {CBIOS_GPIO_GFX,   3},  //DCDC_EN
    {CBIOS_GPIO_GFX,  10},  //RESX
    {CBIOS_GPIO_GFX,  11},  //AVDD
};

// GPIO PORTS for board version A2
GPIO_PORT R63319_GPIO_PORTS_A2[] = 
{
    {CBIOS_GPIO_VSUS, 48},  //IOVDD
    {CBIOS_GPIO_GFX,   2},  //DCDC_EN
    {CBIOS_GPIO_VSUS, 33},  //RESX
    {CBIOS_GPIO_GFX,   1},  //AVDD
};


GPIO_PORT R63319_GPIO_PORTS_EVT[] = 
{
    {CBIOS_GPIO_VSUS, 47},  //IOVDD
    {CBIOS_GPIO_GFX,   2},  //DCDC_EN
    {CBIOS_GPIO_VSUS, 33},  //RESX
    {CBIOS_GPIO_GFX,   1},  //AVDD
};


extern CBIOS_DSI_PANEL_DESC R63319_Panel_Desc;

static CBIOS_STATUS cbR63319_GetGpioPorts(PCBIOS_VOID pvcbe, PGPIO_PORT *pGpioPorts, CBIOS_U32 *pNumPorts)
{
    CBIOS_BOARD_VERSION BoardVersion = cbGetBoardVersion(pvcbe);
    CBIOS_STATUS Status = CBIOS_OK;
    
    *pGpioPorts = CBIOS_NULL;
    *pNumPorts = 0;
    if (BoardVersion <= CBIOS_BOARD_VERSION_1)
    {
        *pGpioPorts = R63319_GPIO_PORTS_A1;
        *pNumPorts = sizeofarray(R63319_GPIO_PORTS_A1);
    }
    else if (BoardVersion == CBIOS_BOARD_VERSION_2)
    {
        *pGpioPorts = R63319_GPIO_PORTS_A2;
        *pNumPorts = sizeofarray(R63319_GPIO_PORTS_A2);
    }
    else if(BoardVersion == CBIOS_BOARD_VERSION_EVT)
    {
        *pGpioPorts = R63319_GPIO_PORTS_EVT;
        *pNumPorts = sizeofarray(R63319_GPIO_PORTS_EVT);
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "%s: invalid board version: 0x%x!\n", FUNCTION_NAME, BoardVersion));
        Status = CBIOS_ER_INTERNAL;
    }

    return Status;
}

CBIOS_STATUS cbR63319_SetBacklight(PCBIOS_VOID pvcbe, CBIOS_U32 BrightnessValue)
{
    PCBIOS_DSI_CONFIG pDSIConfig = &(R63319_Panel_Desc.DSIConfig);
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_U8*  pDataBuffer = CBIOS_NULL;
    CBIOS_U8 BlValue = (CBIOS_U8)BrightnessValue;

    if (BrightnessValue < pDSIConfig->BacklightMin)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "DSI panel backlight value is smaller than min value! \n"));
        BlValue = (CBIOS_U8)pDSIConfig->BacklightMin;
    }

    if (BrightnessValue > pDSIConfig->BacklightMax)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "DSI panel backlight value is greater than max value! \n"));
        BlValue = (CBIOS_U8)pDSIConfig->BacklightMax;
    }

    pDataBuffer = R63319_Backlight_CmdList[0].pDataBuf;
    pDataBuffer[1] = BlValue;
    Status = cbDSI_SendCmdList(pvcbe, R63319_Backlight_CmdList, sizeofarray(R63319_Backlight_CmdList));

    return Status;
}


CBIOS_STATUS cbR63319_GetBacklight(PCBIOS_VOID pvcbe, CBIOS_U32 *pBrightnessValue)
{
    CBIOS_DSI_READ_PARA_INTERNAL DSIReadPara = {0};
    CBIOS_U32 BacklightValue = 0;
    CBIOS_U32 Status;
    
    DSIReadPara.DSIIndex = DSI_INDEX1;
    DSIReadPara.VirtualCh = 0;
    DSIReadPara.ContentType = 0;
    DSIReadPara.DataLen = sizeof(GetBacklightBuf);
    DSIReadPara.pDataBuf = GetBacklightBuf;
    DSIReadPara.pReceivedPayloadBuf = (CBIOS_U8 *)&BacklightValue;
    DSIReadPara.ReceivedPayloadLen = sizeof(BacklightValue);    
    DSIReadPara.bHSModeOnly = 0;
    
    Status = cbDSI_SendReadCmd(pvcbe, &DSIReadPara);

    if (Status == CBIOS_OK)
    {
        *pBrightnessValue = BacklightValue;
    }
    else
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "Can't get backlight value! \n"));
    }
    
    return Status;
}

CBIOS_STATUS cbR63319_PanelPowerOnOff(PCBIOS_VOID pvcbe, CBIOS_BOOL bTurnOn)
{
    CBIOS_BOARD_VERSION BoardVersion = CBIOS_BOARD_VERSION_MAX;
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_U32 NumPorts = 0;
    PGPIO_PORT pR63319GpioPorts = CBIOS_NULL;
    

    cbR63319_GetGpioPorts(pvcbe, &pR63319GpioPorts, &NumPorts);

    if (pR63319GpioPorts == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "%s: cannot get GPIO ports for R63319\n", FUNCTION_NAME));
        Status = CBIOS_ER_INTERNAL;
        return Status;
    }

    BoardVersion = cbGetBoardVersion(pvcbe);

    if(BoardVersion == CBIOS_BOARD_VERSION_EVT)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "-------board version: EVT-------\n"));
        if(bTurnOn)
        {
            //IOVDD high
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_IOVDD].GPIOType, pR63319GpioPorts[R63319_INDEX_IOVDD].GPIOIndex, 1);
            //IOVDD high --> AVDD high > 0ms
            cbDelayMilliSeconds(2);
            //AVDD high
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_AVDD].GPIOType, pR63319GpioPorts[R63319_INDEX_AVDD].GPIOIndex, 1);
            //AVDD high --> DCDC_EN > 2ms
            cbDelayMilliSeconds(6);
            //DCDC_EN, GFX_GPIO3
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_DCDCEN].GPIOType, pR63319GpioPorts[R63319_INDEX_DCDCEN].GPIOIndex, 1);
            //DCDC_EN --> RESX > 2ms
            cbDelayMilliSeconds(6);
            //RESX, GFX_GPIO10
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_RESX].GPIOType, pR63319GpioPorts[R63319_INDEX_RESX].GPIOIndex, 1);
            //RSP LCD driver status reset> 5ms
            cbDelayMilliSeconds(12);
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_RESX].GPIOType, pR63319GpioPorts[R63319_INDEX_RESX].GPIOIndex, 0);
            //RSP LCD driver status reset> 5ms
            cbDelayMilliSeconds(12);
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_RESX].GPIOType, pR63319GpioPorts[R63319_INDEX_RESX].GPIOIndex, 1);
            //RSP LCD driver status reset> 5ms
            cbDelayMilliSeconds(12);
        
            Status = cbDSI_SendCmdList(pvcbe, R63319_PowerOn_CmdList, sizeofarray(R63319_PowerOn_CmdList));
        }
        else
        {
            Status = cbDSI_SendCmdList(pvcbe, R63319_PowerOff_CmdList, sizeofarray(R63319_PowerOff_CmdList));
        
            //DCDC_EN low
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_DCDCEN].GPIOType, pR63319GpioPorts[R63319_INDEX_DCDCEN].GPIOIndex, 0);
            //DCDC_EN low --> AVDD low > 1ms
            cbDelayMilliSeconds(4);
            //AVDD low, GFX_GPIO11
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_AVDD].GPIOType, pR63319GpioPorts[R63319_INDEX_AVDD].GPIOIndex, 0);
            //AVDD low --> IOVDD low > 0ms
            cbDelayMilliSeconds(2);
            //IOVDD low
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_IOVDD].GPIOType, pR63319GpioPorts[R63319_INDEX_IOVDD].GPIOIndex, 0);    
        }  
    }

    else 
    {
        if(bTurnOn)
        {
            //IOVDD high
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_IOVDD].GPIOType, pR63319GpioPorts[R63319_INDEX_IOVDD].GPIOIndex, 1);
            //IOVDD high --> AVDD high > 0ms
            cbDelayMilliSeconds(1);
            //AVDD high
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_AVDD].GPIOType, pR63319GpioPorts[R63319_INDEX_AVDD].GPIOIndex, 1);
            //AVDD high --> DCDC_EN > 2ms
            cbDelayMilliSeconds(3);
            //DCDC_EN, GFX_GPIO3
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_DCDCEN].GPIOType, pR63319GpioPorts[R63319_INDEX_DCDCEN].GPIOIndex, 1);
            //DCDC_EN --> RESX > 2ms
            cbDelayMilliSeconds(3);
            //RESX, GFX_GPIO10
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_RESX].GPIOType, pR63319GpioPorts[R63319_INDEX_RESX].GPIOIndex, 1);
            //RSP LCD driver status reset> 5ms
            cbDelayMilliSeconds(6);
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_RESX].GPIOType, pR63319GpioPorts[R63319_INDEX_RESX].GPIOIndex, 0);
            //RSP LCD driver status reset> 5ms
            cbDelayMilliSeconds(6);
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_RESX].GPIOType, pR63319GpioPorts[R63319_INDEX_RESX].GPIOIndex, 1);
            //RSP LCD driver status reset> 5ms
            cbDelayMilliSeconds(6);
        
            Status = cbDSI_SendCmdList(pvcbe, R63319_PowerOn_CmdList, sizeofarray(R63319_PowerOn_CmdList));
        }
        else
        {
            Status = cbDSI_SendCmdList(pvcbe, R63319_PowerOff_CmdList, sizeofarray(R63319_PowerOff_CmdList));
        
            //DCDC_EN low
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_DCDCEN].GPIOType, pR63319GpioPorts[R63319_INDEX_DCDCEN].GPIOIndex, 0);
            //DCDC_EN low --> AVDD low > 1ms
            cbDelayMilliSeconds(2);
            //AVDD low, GFX_GPIO11
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_AVDD].GPIOType, pR63319GpioPorts[R63319_INDEX_AVDD].GPIOIndex, 0);
            //AVDD low --> IOVDD low > 0ms
            cbDelayMilliSeconds(1);
            //IOVDD low
            cbWriteGPIO(pvcbe, pR63319GpioPorts[R63319_INDEX_IOVDD].GPIOType, pR63319GpioPorts[R63319_INDEX_IOVDD].GPIOIndex, 0);
       
        }
    }
    return Status;
}

CBIOS_STATUS cbR63319_SetCABC(PCBIOS_VOID pvcbe,CBIOS_U32 CabcValue)
{
    
    return CBIOS_OK;
}

CBIOS_STATUS cbR63319_Init(PCBIOS_VOID pvcbe)
{
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_U32 Requested = 0, i = 0, NumPorts = 0;
    PGPIO_PORT pR63319GpioPorts = CBIOS_NULL;

    cbR63319_GetGpioPorts(pvcbe, &pR63319GpioPorts, &NumPorts);

    if (pR63319GpioPorts == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "%s: cannot get GPIO ports for R63319\n", FUNCTION_NAME));
        Status = CBIOS_ER_INTERNAL;
        return Status;
    }

    for (i = 0; i < NumPorts; i++)
    {
        if (cbRequestGPIO(pvcbe, pR63319GpioPorts[i].GPIOType, pR63319GpioPorts[i].GPIOIndex) == CBIOS_OK)
        {
            cbSetGPIODirectionOutput(pvcbe, pR63319GpioPorts[i].GPIOType, pR63319GpioPorts[i].GPIOIndex, 0);
            Requested |= (1 << i);
            Status = CBIOS_OK;
        }
        else
        {
            cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "Request GPIO fail! Type: %x, Index: %d\n", pR63319GpioPorts[i].GPIOType, pR63319GpioPorts[i].GPIOIndex));
            Status = CBIOS_ER_INTERNAL;
            break;
        }
    }

    if (Status != CBIOS_OK)//request fail, free requested GPIOs
    {
        for (i = 0; i < NumPorts; i++)
        {
            if (i & Requested)
            {
                cbFreeGPIO(pvcbe, pR63319GpioPorts[i].GPIOType, pR63319GpioPorts[i].GPIOIndex);
            }
        }
    }

    return Status;
}

CBIOS_STATUS cbR63319_DeInit(PCBIOS_VOID pvcbe)
{
    CBIOS_STATUS Status = CBIOS_OK;
    CBIOS_U32 i = 0, NumPorts = 0;
    PGPIO_PORT pR63319GpioPorts = CBIOS_NULL;

    cbR63319_GetGpioPorts(pvcbe, &pR63319GpioPorts, &NumPorts);

    if (pR63319GpioPorts == CBIOS_NULL)
    {
        cbDebugPrint((MAKE_LEVEL(DSI, ERROR), "%s: cannot get GPIO ports for R63319\n", FUNCTION_NAME));
        Status = CBIOS_ER_INTERNAL;
        return Status;
    }

    for (i = 0; i < NumPorts; i++)
    {
        cbFreeGPIO(pvcbe, pR63319GpioPorts[i].GPIOType, pR63319GpioPorts[i].GPIOIndex);
    }
    
    return Status;
}

CBIOS_DSI_PANEL_DESC R63319_Panel_Desc =
{
    /*.VersionNum = */CBIOS_DSI_VERSION,
    /*.DSIConfig =*/
    {
        /*.DSIMode = */CBIOS_DSI_VIDEOMODE,
        /*.ClkLaneMode = */CBIOS_DSI_CLK_LANE_HARDWARE_CTL,
        /*.SyncPacketType = */CBIOS_DSI_SYNC_EVENT,
        /*.TEType = */CBIOS_DSI_TE_TRIGGER,
        /*.TurnAroundTimeout = */0x1000000,
        /*.HS_TXTimeout = */0x1000000,
        /*.LP_RXTimeout = */0x1000000,
        /*.DMAThreshold = */0x400,
        /*.BacklightMax = */255,
        /*.BacklightMin = */0,
        /*.ConfigFlags =*/ {0x3E,},
    },
    /*.DSIPanelTbl =*/
    {
        /*.LaneNum = */4,
        /*.OutBpp = */24,
        /*.PanelTiming =*/
        {
            /*.XResolution = */1536,
            /*.YResolution = */2048,
#ifdef ELT2K_DIU_FPGA
            /*.DCLK = */5400, //540000,   54MHz
#else

            /*.DCLK = */24775,    // 247.75MHz
#endif
            /*.HorTotal = */1536 + 300 + 76 + 80,
            /*.HorDisEnd = */1536,
            /*.HorBStart = */1536,
            /*.HorBEnd = */1992,
            /*.HorSyncStart = */(1536 + 300),
            /*.HorSyncEnd = */(1536 + 300 + 76),
            /*.VerTotal = */2048 + 12 + 4 + 4,
            /*.VerDisEnd = */2048,
            /*.VerBStart = */2048,
            /*.VerBEnd = */2068,
            /*.VerSyncStart = */(2048 + 12),
            /*.VerSyncEnd = */(2048 + 12 + 4),
            /*.HVPolarity = */DSI_HPOSITIVE + DSI_VPOSITIVE,
        },
    },
    /*.PowerOnCmdListSize = */sizeofarray(R63319_PowerOn_CmdList),
    /*.PowerOffCmdListSize = */sizeofarray(R63319_PowerOff_CmdList),
    /*.DisplayOnCmdListSize = */sizeofarray(R63319_DisplayOn_CmdList),
    /*.DisplayOffCmdListSize = */sizeofarray(R63319_DisplayOff_CmdList),
    /*.BacklightCmdListSize = */sizeofarray(R63319_Backlight_CmdList),
    /*.pPowerOnCmdList = */R63319_PowerOn_CmdList,
    /*.pPowerOffCmdList = */R63319_PowerOff_CmdList,
    /*.pDisplayOnCmdList = */R63319_DisplayOn_CmdList,
    /*.pDisplayOffCmdList = */R63319_DisplayOff_CmdList,
    /*.pBacklightCmdList = */R63319_Backlight_CmdList,
    /*.pFnDSIPanelOnOff = */cbR63319_PanelPowerOnOff,
    /*.pFnDSIPanelSetBacklight = */cbR63319_SetBacklight,
    /*.pFnDSIPanelGetBacklight = */cbR63319_GetBacklight,
    /*.pFnDSIPanelSetCABC = */cbR63319_SetCABC,
    /*.pFnDSIPanelInit = */cbR63319_Init,
    /*.pFnDSIPanelDeInit = */cbR63319_DeInit,
};
